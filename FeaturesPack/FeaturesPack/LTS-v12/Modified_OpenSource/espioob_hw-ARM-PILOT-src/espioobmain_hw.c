/****************************************************************
 **                                                            **
 **    (C)Copyright 2006-2009, American Megatrends Inc.        **
 **                                                            **
 **            All Rights Reserved.                            **
 **                                                            **
 **        5555 Oakbrook Pkwy Suite 200, Norcross              **
 **                                                            **
 **        Georgia - 30093, USA. Phone-(770)-246-8600.         **
 **                                                            **
****************************************************************/

 /*
 * File name: espioobmain_hw.c
 * eSPI OOB hardware driver is implemented for hardware controller.
 */

#include <linux/module.h>
#include <linux/pci.h>
#include <linux/fs.h>
#include <asm/io.h>
#include <asm/uaccess.h>
#include <linux/delay.h>
#include <linux/interrupt.h>

#include "driver_hal.h"
#include "reset.h"
#include "espioob_ioctl.h"
#include "espioob.h"
#include "espioob_hw.h"


static void *pilot_espioob_virt_base;

static int pilot_espioob_hal_id;
static espioob_core_funcs_t *espioob_core_ops;

static void pilot_espi_oob_tx(int tx_payload_size);
static int espioob_rx_wait_for_int( int ms_timeout );

struct espioob_ch_data        oob_tx_channel;
struct espioob_ch_data        oob_rx_channel;

#define PILOT_ESPI_OOB_HEADER_SIZE     3 /* the size of "cyc type + tag + len" */

unsigned char tag = 0;

/* -------------------------------------------------- */

inline uint8_t pilot_espioob_read8_reg(uint16_t offset)
{
    return ioread8((void * __iomem)pilot_espioob_virt_base + offset);
}

inline void pilot_espioob_write8_reg(uint8_t value, uint16_t offset)
{
    iowrite8(value, (void * __iomem)pilot_espioob_virt_base + offset);
}

inline uint32_t pilot_espioob_read32_reg(uint32_t offset)
{
    return( ioread32( (void * __iomem)pilot_espioob_virt_base + offset ) );
}

inline void pilot_espioob_write32_reg(uint32_t value, uint32_t offset) 
{
    iowrite32(value, (void * __iomem)pilot_espioob_virt_base + offset);
}
/* -------------------------------------------------- */

static unsigned char pilot_espioob_num_ch(void)
{
    return PILOT_ESPIOOB_CHANNEL_NUM;
}

static void oob_read(struct espioob_data_t *espioob_data)
{
    if (espioob_rx_wait_for_int(1000) == 0)
    {
        espioob_data->header = oob_rx_channel.header;
        espioob_data->buf_len = oob_rx_channel.buf_len;
        memcpy(espioob_data->buffer, oob_rx_channel.buff, oob_rx_channel.buf_len);
    }
    else
    {
        espioob_data->header = 0;
        espioob_data->buf_len = 0;
        memset(espioob_data->buffer, 0, MAX_XFER_BUFF_SIZE);
    }
    /* Clear RX Data */
    memset(oob_rx_channel.buff, 0, oob_rx_channel.buf_len);
    oob_rx_channel.header = 0;
    oob_rx_channel.buf_len = 0;
}

static void oob_write (struct espioob_data_t *espioob_data)
{
    unsigned int    header = 0;
    header = espioob_data->header;
    oob_tx_channel.header = (GET_LEN(header) << 12) | ((tag & 0xf) << 8) | GET_CYCLE_TYPE(header);
    oob_tx_channel.buf_len = espioob_data->buf_len;
    memcpy(oob_tx_channel.buff, espioob_data->buffer, oob_tx_channel.buf_len);
    pilot_espi_oob_tx(PILOT_ESPI_OOB_HEADER_SIZE + espioob_data->buf_len); /* 3 (cyc + tag + len) + data len*/

    tag ++;
    tag %= 0xf;
}

static void oob_writeread(struct espioob_data_t *espioob_data)
{
    // Transmitting
    oob_write(espioob_data);
    // Receiving
    oob_read(espioob_data);
}

static void get_oob_channel_status(u32 *gen_status, u32 *ch_status, u32 *OOBChMaxPayloadSizeSelected, u32 *OOBChMaxPayloadSizeSupported )
{    
    *gen_status = pilot_espioob_read32_reg(PILOT_ESPI_GEN_CAPCONF);

    *ch_status = 0;
    
    if (pilot_espioob_read32_reg(PILOT_ESPI_CH2_CAPCONF) & 0x3)
        *ch_status |= OOB_CHANNEL_SUPPORTED;

    *OOBChMaxPayloadSizeSelected = (pilot_espioob_read32_reg(PILOT_ESPI_CH2_CAPCONF) & 0x700) >> 8;
    *OOBChMaxPayloadSizeSupported = (pilot_espioob_read32_reg(PILOT_ESPI_CH2_CAPCONF) & 0x70) >> 4;

#ifdef ESPI_DEBUG
    // Slave Registers
    printk("PILOT_ESPI_GEN_CAPCONF    (08h) 0x%x\n", (unsigned int)pilot_espioob_read32_reg(PILOT_ESPI_GEN_CAPCONF));
    printk("PILOT_ESPI_CH0_CAPCONF    (10h) 0x%x\n", (unsigned int)pilot_espioob_read32_reg(PILOT_ESPI_CH0_CAPCONF));
    printk("PILOT_ESPI_CH1_CAPCONF    (20h) 0x%x\n", (unsigned int)pilot_espioob_read32_reg(PILOT_ESPI_CH1_CAPCONF));
    printk("PILOT_ESPI_CH2_CAPCONF    (30h) 0x%x\n", (unsigned int)pilot_espioob_read32_reg(PILOT_ESPI_CH2_CAPCONF));
    printk("PILOT_ESPI_CH3_CAPCONF    (40h) 0x%x\n", (unsigned int)pilot_espioob_read32_reg(PILOT_ESPI_CH3_CAPCONF));
#endif
}

static int espioob_rx_wait_for_int( int ms_timeout )
{
    int status = 0;
    if (wait_event_timeout(oob_rx_channel.as_wait, oob_rx_channel.op_status, (ms_timeout*HZ/1000)) == 0)
    {
        status = -1;
        //printk("!!! time out !!!!\n");
    }
    oob_rx_channel.op_status = 0;

    return status;
}

// Channel 2: OOB
static void pilot_espi_oob_rx(void)
{
    int rx_cnt = 0;

    oob_rx_channel.header = ((pilot_espioob_read32_reg(PILOT_ESPI_OOB_LEN) & 0xfff) << 12) |
                            ((pilot_espioob_read8_reg(PILOT_ESPI_OOB_TAG) & 0xf) << 8) |
                            (pilot_espioob_read8_reg(PILOT_ESPI_OOB_CYC) & 0xff);

    rx_cnt = 0;
    while ((pilot_espioob_read32_reg(PILOT_ESPI_OOB_STATUS) & H2B_FIFO_EMPTY) == 0x00)
        oob_rx_channel.buff[rx_cnt++] = pilot_espioob_read8_reg(PILOT_ESPI_OOB_DATA);

    oob_rx_channel.buf_len = rx_cnt;

    // Wake up RX process
    oob_rx_channel.op_status = 1;
    wake_up( &oob_rx_channel.as_wait );
}

static void pilot_espi_oob_tx(int tx_payload_size)
{
    int tx_cnt = 0;

    pilot_espioob_write32_reg(pilot_espioob_read32_reg(PILOT_ESPI_OOB_CTRL) | (B2H_WRPTR_CLEAR | B2H_RDPTR_CLEAR | H2B_WRPTR_CLEAR | H2B_RDPTR_CLEAR), PILOT_ESPI_OOB_CTRL);

    pilot_espioob_write8_reg(tx_payload_size, PILOT_ESPI_OOB_SIZE);

    //printk("pilot_espi_oob_tx : CYC 0x%x LEN 0x%x TAG 0x%x\n", GET_CYCLE_TYPE(oob_tx_channel.header), GET_LEN(oob_tx_channel.header), GET_TAG(oob_tx_channel.header));
    // Byte 0(Cycle Type)
    pilot_espioob_write8_reg(GET_CYCLE_TYPE(oob_tx_channel.header), PILOT_ESPI_OOB_DATA);
    // Byte 1(Tag + Length[11:8])
    pilot_espioob_write8_reg((unsigned char) (((GET_LEN(oob_tx_channel.header) >> 8) & 0xf) | ((GET_TAG(oob_tx_channel.header) & 0xf) << 4)), PILOT_ESPI_OOB_DATA );
    // Byte 2 Length[7:0]
    pilot_espioob_write8_reg((unsigned char) (GET_LEN(oob_tx_channel.header) & 0xff), PILOT_ESPI_OOB_DATA );
    // Byte 3+x address(option) + data
    for(tx_cnt = 0;tx_cnt < oob_tx_channel.buf_len; tx_cnt++)
        pilot_espioob_write8_reg ( (unsigned char) oob_tx_channel.buff[tx_cnt], PILOT_ESPI_OOB_DATA );

    // Generate alert to host
    pilot_espioob_write32_reg(pilot_espioob_read32_reg(PILOT_ESPI_OOB_CTRL) | 0x1,   PILOT_ESPI_OOB_CTRL);
}

static void pilot_espioob_hw_init(void) 
{
    // Enable interrupts
    pilot_espioob_write32_reg(pilot_espioob_read32_reg(PILOT_ESPI_OOB_CTRL) | H2B_INTR_EN,   PILOT_ESPI_OOB_CTRL);
}

/* -------------------------------------------------- */
static espioob_hal_operations_t pilot_espioob_hw_ops = {
    .num_espioob_ch         = pilot_espioob_num_ch,
    .oob_read               = oob_read,
    .oob_write              = oob_write,
    .oob_writeread          = oob_writeread,
    .get_channel_status     = get_oob_channel_status,
};

static hw_hal_t pilot_espioob_hw_hal = {
    .dev_type        = EDEV_TYPE_ESPIOOB,
    .owner            = THIS_MODULE,
    .devname        = PILOT_ESPIOOB_DRIVER_NAME,
    .num_instances    = PILOT_ESPIOOB_CHANNEL_NUM,
    .phal_ops        = (void *) &pilot_espioob_hw_ops
};

static int pilot_espioob_reset_handler(void)
{
    printk(KERN_DEBUG "pilot_espioob_reset_handler: got eSPI RESET\n");

    // clear the RD/WR PTRs
    pilot_espioob_write32_reg(pilot_espioob_read32_reg(PILOT_ESPI_OOB_CTRL) | (B2H_WRPTR_CLEAR | B2H_RDPTR_CLEAR | H2B_WRPTR_CLEAR | H2B_RDPTR_CLEAR), PILOT_ESPI_OOB_CTRL);

    return 0;
}

static irqreturn_t pilot_espioob_handler(int this_irq, void *dev_id)
{
    unsigned int handled = 0;
    unsigned long status = 0;

    status = pilot_espioob_read32_reg(PILOT_ESPI_OOB_STATUS);

    if (status & B2H_BUSY) {
        //printk("B2H_BUSY \n");
        pilot_espioob_write32_reg(B2H_BUSY, PILOT_ESPI_OOB_STATUS);
        handled |= B2H_BUSY;
    }

    if (status & B2H_FIFO_EMPTY) {
        //printk("B2H_FIFO_EMPTY \n");
        pilot_espioob_write32_reg(B2H_FIFO_EMPTY, PILOT_ESPI_OOB_STATUS);
        handled |= B2H_FIFO_EMPTY;
    }

    if (status & B2H_INTR_STATUS) {
        //printk("B2H_INTR_STATUS \n");
        pilot_espioob_write32_reg(B2H_INTR_STATUS, PILOT_ESPI_OOB_STATUS);
        handled |= B2H_INTR_STATUS;
    }

    if (status & H2B_INTR_STATUS) {
        //printk("H2B_INTR_STATUS \n");
        pilot_espi_oob_rx();
        pilot_espioob_write32_reg((H2B_PACKET_VALID | H2B_INTR_STATUS), PILOT_ESPI_OOB_STATUS);
        handled |= (H2B_PACKET_VALID | H2B_INTR_STATUS);
    }

    if (status & H2B_FIFO_EMPTY) {
        //printk("H2B_FIFO_EMPTY \n");
        pilot_espioob_write32_reg(H2B_FIFO_EMPTY, PILOT_ESPI_OOB_STATUS);
        handled |= H2B_FIFO_EMPTY;
    }

    if ((status & (~handled)) == 0)
        return IRQ_HANDLED;
    else
        return IRQ_NONE;
}

static irqreturn_t oobdummy_irqhandler(int irq, void *dev_id)
{
    printk("%s: oobdummy handler\n", PILOT_ESPIOOB_DRIVER_NAME);
    return IRQ_HANDLED;
}

int pilot_espioob_init(void)
{
    uint32_t status;

    extern int espi_core_loaded;
    if (!espi_core_loaded)
        return -1;
    
    status = *(volatile u32 *)(IO_ADDRESS(SE_SYSCLK_VA_BASE + 0x0C));
    if(!(status & (0x1 << 14)))
    {
        printk(KERN_WARNING "%s: eSPI mode is not enable 0x%x\n", PILOT_ESPIOOB_DRIVER_NAME, (unsigned int)status);
        return -EIO;
    }
    
    pilot_espioob_hal_id = register_hw_hal_module(&pilot_espioob_hw_hal, (void **) &espioob_core_ops);
    if (pilot_espioob_hal_id < 0) {
        printk(KERN_WARNING "%s: register HAL HW module failed\n", PILOT_ESPIOOB_DRIVER_NAME);
        return pilot_espioob_hal_id;
    }
    
    pilot_espioob_virt_base = ioremap_nocache(PILOT_ESPI_REG_BASE, SZ_4K);
    if (!pilot_espioob_virt_base) {
        printk(KERN_WARNING "%s: ioremap failed\n", PILOT_ESPIOOB_DRIVER_NAME);
        unregister_hw_hal_module(EDEV_TYPE_ESPIOOB, pilot_espioob_hal_id);
        return -ENOMEM;
    }

    // Need it for prior A2 chips 
    if ((*(volatile u32 *)(IO_ADDRESS(SE_SYSCLK_VA_BASE + 0x50)) & 0xF) < 2)
    {
        if (request_irq(IRQ_SIO_PSR, oobdummy_irqhandler, IRQF_SHARED, "pilot_espioob_oobdummy", &oobdummy_irqhandler) < 0)
        {
            printk(KERN_WARNING "%s: Failed to request irq %d", PILOT_ESPIOOB_DRIVER_NAME, IRQ_SIO_PSR);
            unregister_hw_hal_module(EDEV_TYPE_ESPIOOB, pilot_espioob_hal_id);
            iounmap (pilot_espioob_virt_base);
            return -EBUSY;
        }
    }

    // Request IRQ of OOB
    if( request_irq( IRQ_ESPI_OOB, pilot_espioob_handler, IRQF_DISABLED, "pilot_espioob", (pilot_espioob_virt_base + PILOT_ESPI_OOB_CHAN_ID)) < 0 )
    {
        printk(KERN_WARNING "failed to request irq %d", IRQ_ESPI_OOB);
        free_irq(IRQ_SIO_PSR, &oobdummy_irqhandler);
        unregister_hw_hal_module(EDEV_TYPE_ESPIOOB, pilot_espioob_hal_id);
        iounmap (pilot_espioob_virt_base);
        return -EBUSY;
    }

    oob_rx_channel.buff = kmalloc(MAX_XFER_BUFF_SIZE, GFP_KERNEL);
    oob_tx_channel.buff = kmalloc(MAX_XFER_BUFF_SIZE, GFP_KERNEL);
    oob_rx_channel.op_status = 0;
    oob_tx_channel.op_status = 0;
    init_waitqueue_head( &(oob_rx_channel.as_wait));

    pilot_espioob_hw_init();

    install_reset_handler(pilot_espioob_reset_handler);

    printk("The eSPI OOB HW Driver is loaded successfully.\n" );
    return 0;
}

void pilot_espioob_exit(void)
{
    kfree(oob_tx_channel.buff);
    kfree(oob_rx_channel.buff);

    free_irq(IRQ_ESPI_OOB, (pilot_espioob_virt_base + PILOT_ESPI_OOB_CHAN_ID));

    if ((*(volatile u32 *)(IO_ADDRESS(SE_SYSCLK_VA_BASE + 0x50)) & 0xF) < 2)
        free_irq(IRQ_SIO_PSR, &oobdummy_irqhandler);

    iounmap (pilot_espioob_virt_base);

    uninstall_reset_handler(pilot_espioob_reset_handler);

    unregister_hw_hal_module(EDEV_TYPE_ESPIOOB, pilot_espioob_hal_id);

    return;
}

module_init (pilot_espioob_init);
module_exit (pilot_espioob_exit);

MODULE_AUTHOR("American Megatrends Inc.");
MODULE_DESCRIPTION("PILOT SoC eSPI OOB Driver");
MODULE_LICENSE ("GPL");
