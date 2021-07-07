/****************************************************************
 **                                                            **
 **    (C)Copyright 2009-2015, American Megatrends Inc.        **
 **                                                            **
 **            All Rights Reserved.                            **
 **                                                            **
 **        5555 Oakbrook Pkwy Suite 200, Norcross              **
 **                                                            **
 **        Georgia - 30093, USA. Phone-(770)-246-8600.         **
 **                                                            **
 ****************************************************************/

/****************************************************************
 *
 * ast_postcode.c
 * ASPEED AST2100/2050/2200/2150 LPC postcode driver
 *
*****************************************************************/

#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/errno.h>
#include <linux/ioport.h>
#include <linux/kfifo.h>

#include <asm/irq.h>
#include <asm/io.h>
#include <asm/uaccess.h>

#include "driver_hal.h"
#include "postcode.h"
#include "ast_postcode.h"
#include <linux/dma-mapping.h>
#include "helper.h"

#define AST_POSTCODE_DRIVER_NAME	"ast_postcode"

#define AST_POSTCODE_REG_VA_BASE         IO_ADDRESS(AST_POSTCODE_REG_BASE)
#define AST_POSTCODE_DEV_ID_OFFSET       2  

/** Default values in these arrays are for the as **/
static u32 postcode_as_io_base[ AST_POSTCODE_PORT_NUM ] = {
    AST_POSTCODE_REG_VA_BASE,
};
static postcode_core_funcs_t *postcode_core_ops;
static int ast_postcode_hal_id;

struct postcode_kfifo *post_codes[AST_POSTCODE_PORT_NUM];

dma_addr_t PostCodeDmaAddr;
unsigned int *postcode_dma_buf;
uint32_t postcode_cur_addr=0, postcode_pre_addr=0;
unsigned int postcode_result=0;

#if (LINUX_VERSION_CODE > KERNEL_VERSION(5,0,0))
static int postcode_irq;
#endif

inline uint32_t ast_postcode_read_reg(int ch_num, uint32_t reg)
{
	return ioread32((void * __iomem)postcode_as_io_base[ch_num] + reg);
}

inline void ast_postcode_write_reg(int ch_num, uint32_t data, uint32_t reg)
{
	iowrite32(data, (void * __iomem)postcode_as_io_base[ch_num] + reg);
}


static void ast_postcode_enable_port(int ch_num)
{
	uint32_t reg;

	reg = ast_postcode_read_reg(ch_num, AST_LPC_PCCR0);
	reg |= AST_LPC_PCCR0_POSTCODE_EN_MASK;
	ast_postcode_write_reg(ch_num, reg, AST_LPC_PCCR0);
}

static void ast_postcode_disable_port(int ch_num)
{
	uint32_t reg;

	reg = ast_postcode_read_reg(ch_num, AST_LPC_PCCR0);
	reg &= ~(AST_LPC_PCCR0_POSTCODE_EN_MASK);
	ast_postcode_write_reg(ch_num, reg, AST_LPC_PCCR0);
}

static void ast_postcode_configure_port_addr(int ch_num)
{
	uint32_t reg;

	reg = ast_postcode_read_reg(ch_num, AST_LPC_PCCR1);
	reg &= ~(AST_LPC_PCCR1_LPC_CAP_ADDR_MASK);
	reg |= (AST_POSTCODE_ADDR_0 << AST_LPC_PCCR1_LPC_CAP_ADDR_SHIFT);
	ast_postcode_write_reg(ch_num, reg, AST_LPC_PCCR1);
}

static void ast_postcode_configuration(int ch_num)
{
	uint32_t reg;

	reg = 0;
	ast_postcode_write_reg(ch_num, reg, AST_LPC_LHCR5);

	reg = ast_postcode_read_reg(ch_num, AST_LPC_LHCRA);
	reg = (AST_LPC_LHCR_WR_CYCLE_MASK | (AST_POSTCODE_ADDR_0 << AST_LPC_LHCR_ADDR_SHIFT));
	ast_postcode_write_reg(ch_num, reg, AST_LPC_LHCRA);

	reg = ast_postcode_read_reg(ch_num, AST_LPC_PCCR1);
	reg &= ~( AST_LPC_PCCR1_DONT_CARE_ADDR_MASK);
	reg |= (AST_LPC_PCCR1_DONT_CARE_ADDR_3 << AST_LPC_PCCR1_DONT_CARE_ADDRE_SHIFT);
	ast_postcode_write_reg(ch_num, reg, AST_LPC_PCCR1);

	reg = ast_postcode_read_reg(ch_num, AST_LPC_PCCR2);
	reg |= (AST_LPC_PCCR2_RESET_STAT_PATTERN_A_MASK | AST_LPC_PCCR2_INT_STAT_PATTERN_A_MASK | AST_LPC_PCCR2_DATA_READY_MASK | AST_LPC_PCCR2_DATA_READY_MASK);
	ast_postcode_write_reg(ch_num, reg, AST_LPC_PCCR2);

	reg = ast_postcode_read_reg(ch_num, AST_LPC_PCCR0);
	reg &= ~(AST_LPC_PCCR0_DATA_MODE_MASK | AST_LPC_PCCR0_ADDR_SEL_MASK | AST_LPC_PCCR0_DMATIMER_SEL_MASK);
	reg |= ((AST_LPC_PCCR0_DATA_MODE_2B << AST_LPC_PCCR0_DATA_MODE_SHIFT) |
	        (AST_LPC_PCCR0_ADDR_SEL_1 << AST_LPC_PCCR0_ADDR_SEL_SHIFT) | 
	        (AST_LPC_PCCR0_DMATIMER_SEL_1 << AST_LPC_PCCR0_DMATIMER_SEL_SHIFT) | 
	        AST_LPC_PCCR0_POSTCODE_DMA_MASK |
	        AST_LPC_PCCR0_PATTERN_A_MASK |
	        AST_LPC_PCCR0_PATTERN_A_INT_MASK);
	ast_postcode_write_reg(ch_num, reg, AST_LPC_PCCR0);
}

static void ast_postcode_init_hw(int ch_num)
{
	ast_postcode_disable_port(ch_num);

 	ast_postcode_configure_port_addr(ch_num);
    postcode_dma_buf = (unsigned int *)dma_alloc_coherent(NULL, AST_POST_CODE_DMA_BUFFER_BYTE_SIZE_VAL, &PostCodeDmaAddr, GFP_KERNEL);
    ast_postcode_write_reg(ch_num, PostCodeDmaAddr, AST_LPC_PCCR4);
    ast_postcode_write_reg(ch_num, AST_POST_CODE_DMA_BUFFER_SIZE_VAL, AST_LPC_PCCR5);
	ast_postcode_configuration(ch_num);

	ast_postcode_enable_port(ch_num);
}

static int isPostCodeAInterruptOccurred(int ch_num)
{
	uint32_t reg;
#if defined(GROUP_AST1070_COMPANION)
#else
    reg = ast_postcode_read_reg(ch_num, AST_LPC_PCCR2);

    return (reg & (AST_LPC_PCCR2_RESET_STAT_PATTERN_A_MASK | AST_LPC_PCCR2_INT_STAT_PATTERN_A_MASK | AST_LPC_PCCR2_DATA_READY_MASK));
#endif
}

static void ast_PostCodeA_clear_interrupt_flg(int ch_num)
{
	uint32_t reg=0;

#if defined(GROUP_AST1070_COMPANION)
#else
    reg |= (AST_LPC_PCCR2_RESET_STAT_PATTERN_A_MASK | AST_LPC_PCCR2_INT_STAT_PATTERN_A_MASK | AST_LPC_PCCR2_DATA_READY_MASK);
    ast_postcode_write_reg(ch_num, reg, AST_LPC_PCCR2);
#endif
}


static hw_hal_t ast_postcode_hw_hal = {
	.dev_type = EDEV_TYPE_POSTCODE,
	.owner = THIS_MODULE,
	.devname = AST_POSTCODE_DRIVER_NAME,
	.num_instances = AST_POSTCODE_PORT_NUM,
	.phal_ops = (void *) NULL
};

static int Convert2PostCode(unsigned short *ptrSrc, unsigned int *ptrResult, int *cnt)
{
    int Ret = -1;
    unsigned short *ptrSrcTmp=ptrSrc;
    int data_offset = 0, data=0;
    int pre_offset = -1, bcnt = 0, bflag = 0;

    while (1)
    {
        if (*ptrSrcTmp == 0) break;
        data_offset = (*ptrSrcTmp & 0x0F00) >> 8;
        data = (*ptrSrcTmp & 0xFF);
        if( (data_offset - pre_offset) != 1 )
			bflag = 1;
        postcode_result &= ~(0xFF << (data_offset*8));
        postcode_result |= (unsigned int) (data << (data_offset*8));
        (*cnt)++;
        bcnt++;
        pre_offset = data_offset;
        if ((*(++ptrSrcTmp) & 0x0F00) == 0)
        {
            Ret = 0;
            break;
        }
    }

    if (Ret == 0)
    {
        *ptrResult = postcode_result;        
        if( (bcnt < 2) || bflag )
			Ret = -1;
        postcode_result = 0;
	}

    return Ret;
}

static irqreturn_t ast_postcode_irq_handler(int irq, void *dev_id)
{
    int ch=0;

    if (isPostCodeAInterruptOccurred(ch))
    {
        int postcode_ind=0, postcode_cnt=0;
        unsigned int postcode_data=0;

	    /* clear interrupt flag */
        ast_PostCodeA_clear_interrupt_flg(ch);

        postcode_cur_addr = ast_postcode_read_reg(ch, AST_LPC_PCCR6);
        postcode_cur_addr &= AST_POST_CODE_DMA_DATA_LEN_MASK;

        if (postcode_cur_addr != postcode_pre_addr)
        {
            if (postcode_cur_addr < postcode_pre_addr)
            {
                for (postcode_ind=(postcode_pre_addr/2), postcode_cnt=0; postcode_ind < (AST_POST_CODE_DMA_BUFFER_BYTE_SIZE_VAL/2); postcode_ind+=postcode_cnt)
                {
                    postcode_cnt=0;
                    if (Convert2PostCode((unsigned short *)postcode_dma_buf+postcode_ind,&postcode_data,&postcode_cnt) == 0)
                    {
                        memcpy ((unsigned int *)post_codes[ch]->kfifo.data+post_codes[ch]->kfifo.in,(void *)&postcode_data,sizeof(unsigned int));
                 
                        post_codes[ch]->kfifo.in = (post_codes[ch]->kfifo.in + 1) % POSTCODE_BUFSIZE;

                        if ( post_codes[ch]->kfifo.mask == (POSTCODE_BUFSIZE-1))
                        {
                            post_codes[ch]->kfifo.mask++;
                            post_codes[ch]->kfifo.out = (post_codes[ch]->kfifo.out + 1) % POSTCODE_BUFSIZE;
                        }
                        else if ( post_codes[ch]->kfifo.mask == POSTCODE_BUFSIZE)
                            post_codes[ch]->kfifo.out = (post_codes[ch]->kfifo.out + 1) % POSTCODE_BUFSIZE;
                        else
                            post_codes[ch]->kfifo.mask++;
                    }
                    else
                        break;
                }
                postcode_pre_addr = 0;
            }

            for (postcode_ind=(postcode_pre_addr/2), postcode_cnt=0; postcode_ind < (postcode_cur_addr/2); postcode_ind+=postcode_cnt)
            {
                postcode_cnt=0;
                if (Convert2PostCode((unsigned short *)postcode_dma_buf+postcode_ind,&postcode_data,&postcode_cnt) == 0)
                {
                    memcpy ((unsigned int *)post_codes[ch]->kfifo.data+post_codes[ch]->kfifo.in,(void *)&postcode_data,sizeof(unsigned int));

                    post_codes[ch]->kfifo.in = (post_codes[ch]->kfifo.in + 1) % POSTCODE_BUFSIZE;

                    if ( post_codes[ch]->kfifo.mask == (POSTCODE_BUFSIZE-1))
                    {
                        post_codes[ch]->kfifo.mask++;
                        post_codes[ch]->kfifo.out = (post_codes[ch]->kfifo.out + 1) % POSTCODE_BUFSIZE;
                    }
                    else if ( post_codes[ch]->kfifo.mask == POSTCODE_BUFSIZE)
                        post_codes[ch]->kfifo.out = (post_codes[ch]->kfifo.out + 1) % POSTCODE_BUFSIZE;
                    else
                        post_codes[ch]->kfifo.mask++;
                }
                else
                    break;
            }
        }

        postcode_pre_addr = postcode_cur_addr;
    }

	return IRQ_NONE;
}

static int ast_postcode_module_init(void)
{
	int ret;
    int ch=0;

	extern int postcode_core_loaded;
	if (!postcode_core_loaded)
			return -1;

    ast_postcode_hal_id = register_hw_hal_module(&ast_postcode_hw_hal, (void **) &postcode_core_ops);
    if (ast_postcode_hal_id < 0) {
        printk(KERN_WARNING "%s%d: register HAL HW module failed\n", AST_POSTCODE_DRIVER_NAME, ch);
        return ast_postcode_hal_id;
    }

    IRQ_SET_LEVEL_TRIGGER(AST_POSTCODE_IRQ);
    IRQ_SET_HIGH_LEVEL(AST_POSTCODE_IRQ);

#if (LINUX_VERSION_CODE < KERNEL_VERSION(5,0,0))
    ret = request_irq(AST_POSTCODE_IRQ, ast_postcode_irq_handler, IRQF_SHARED, AST_POSTCODE_DRIVER_NAME, (void *)(postcode_as_io_base[ch]+AST_POSTCODE_DEV_ID_OFFSET));
#else
	postcode_irq = GetIrqFromDT("ami_postcode", AST_POSTCODE_IRQ);	
    ret = request_irq(postcode_irq, ast_postcode_irq_handler, IRQF_SHARED, AST_POSTCODE_DRIVER_NAME, (void *)(postcode_as_io_base[ch]+AST_POSTCODE_DEV_ID_OFFSET));
#endif

    if (ret) {
        printk(KERN_WARNING "%s%d: request irq failed\n", AST_POSTCODE_DRIVER_NAME,ch);
        goto out_register_hal;
    }

    postcode_core_ops->get_postcode_core_data(&post_codes[ch], ast_postcode_hal_id, ch);
    ast_postcode_init_hw(ch);

	return 0;

out_register_hal:
	unregister_hw_hal_module(EDEV_TYPE_POSTCODE, ast_postcode_hal_id);

	return ret;
}

static void ast_postcode_module_exit(void)
{
    int ch=0;

    ast_postcode_disable_port(ch);

#if (LINUX_VERSION_CODE < KERNEL_VERSION(5,0,0))
    free_irq(AST_POSTCODE_IRQ, (void *)(postcode_as_io_base[ch]+AST_POSTCODE_DEV_ID_OFFSET));
#else
    free_irq(postcode_irq, (void *)(postcode_as_io_base[ch]+AST_POSTCODE_DEV_ID_OFFSET));
#endif

	unregister_hw_hal_module(EDEV_TYPE_POSTCODE, ast_postcode_hal_id);
}

module_init(ast_postcode_module_init);
module_exit(ast_postcode_module_exit);

MODULE_AUTHOR("American Megatrends Inc.");
MODULE_DESCRIPTION("AST2500/2530 LPC postcode driver");
MODULE_LICENSE("GPL");
