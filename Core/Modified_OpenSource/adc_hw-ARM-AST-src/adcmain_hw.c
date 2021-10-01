/****************************************************************
 ****************************************************************
 **                                                            **
 **    (C)Copyright 2009-2015, American Megatrends Inc.        **
 **                                                            **
 **            All Rights Reserved.                            **
 **                                                            **
 **        6145-F, Northbelt Parkway, Norcross,                **
 **                                                            **
 **        Georgia - 30071, USA. Phone-(770)-246-8600.         **
 **                                                            **
 ****************************************************************
 ****************************************************************/
/*****************************************************************
 *
 * adc_hw.c
 * ADC driver hardware functions for AST SoC
 *
 *****************************************************************/
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/time.h>
#include <linux/poll.h>
#include <linux/pci.h>
#include <linux/delay.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/wait.h>
#include <mach/platform.h>
#include <asm/io.h>
#include <asm/irq.h>
#include "helper.h"
#include "driver_hal.h"

#include "adc_hw.h"
#include "adc.h"

#define ADC_HW_MAX_INST	1

static void *ast_adc_virt_base = NULL;

static int m_dev_id = 0;
static int compensating_value = 0;
static uint16_t compensating_mode = 0;

static int adc_hw_module_init(void);
static void adc_hw_module_exit(void);
static int enable_adc_channel(uint16_t adc_channel_selection);

//static int disable_adc_channel(uint16_t adc_channel_selection);
//static int set_adc_clock(uint16_t adc_clock_divisor, uint16_t inverse);
int adc_read_compensating_value(void);

static	int adc_read_channel (uint16_t *adc_value, int channel);
static	int adc_get_resolution (uint16_t *adc_resolution);
static	int adc_get_reference_voltage (uint16_t *adc_ref_volatge);
static	int adc_reboot_notifier (void);
static int adc_set_compensating_mode(uint16_t mode);

extern int register_adc_hw_module (ADC_HW* AdcHw);

static adc_core_funcs_t *padc_core_funcs;

static adc_hal_operations_t adc_hw_ops = {
	adc_read_channel,
	adc_get_resolution,
	adc_get_reference_voltage,
	adc_reboot_notifier,
	adc_set_compensating_mode,
};

static hw_hal_t hw_hal = {
	.dev_type = EDEV_TYPE_ADC,
	.owner = THIS_MODULE,
	.devname = ADC_HW_DEV_NAME,
	.num_instances = ADC_HW_MAX_INST,
	.phal_ops = (void *)&adc_hw_ops
};



inline uint32_t ast_adc_read_reg(uint32_t reg)
{
	return ioread32((void * __iomem)ast_adc_virt_base + reg);
}

inline void ast_adc_write_reg(uint32_t data, uint32_t reg)
{
	iowrite32(data, (void * __iomem)ast_adc_virt_base + reg);
}


int enable_adc_channel(uint16_t adc_channel_selection)
{
	uint32_t reg;
	reg = ast_adc_read_reg(AST_ADC_ENGINE_CONTROL_REG);
	if ( !(reg & (0x01 << (AST_ADC_CHANNEL_ENABLE + adc_channel_selection))) )
	{
		reg |=  0x01 << (AST_ADC_CHANNEL_ENABLE + adc_channel_selection);
		ast_adc_write_reg (reg, AST_ADC_ENGINE_CONTROL_REG);
	}

	return 1;
}

/*
int disable_adc_channel(uint16_t adc_channel_selection)
{
	uint32_t reg;
	reg = ast_adc_read_reg(AST_ADC_ENGINE_CONTROL_REG);
	reg &= ~(0x01 << (AST_ADC_CHANNEL_ENABLE + adc_channel_selection));
	ast_adc_write_reg (reg, AST_ADC_ENGINE_CONTROL_REG);
	return 1;
}
*/

/*Refer to ASPEED application note Ref No. A2300-08.  Voltage Measurement Accuracy. 
* Improve the accuracy form 2.5% FS to 1% FS.
* 1.Set ADC00[5:0] to 0x2F, ADC into compensating mode.
* 2.Set ADC00 to 0x1000006F, enable compensation channel
* 3.Store the compensation value.
*/
static void adc_audo_compensating_sensing_mode(void)
{
	uint32_t reg;

#if defined(CONFIG_SOC_AST2300)	
	//Into compensating mode and enable compensation channel
	ast_adc_write_reg(0x2F, AST_ADC_ENGINE_CONTROL_REG);
	msleep(50);
	ast_adc_write_reg(0x1000006F, AST_ADC_ENGINE_CONTROL_REG);
	msleep(1);
	
	reg = ast_adc_read_reg(AST_ADC_ENGINE_CONTROL_REG + 0x28);
	compensating_value = (reg &0x3FF);

	if(compensating_value & 0x100)
	{
		compensating_value = (0x200 - compensating_value);
	}
	else
	{
		compensating_value = (0x0 - compensating_value);
	}
	
	//Disable compensation channel
	ast_adc_write_reg(0x2F, AST_ADC_ENGINE_CONTROL_REG);
#elif defined(CONFIG_SOC_AST2400) || defined(CONFIG_SOC_AST1250)
	msleep(50);     
    //Into compensating mode and enable compensation channel
    ast_adc_write_reg(0x0000001F, AST_ADC_ENGINE_CONTROL_REG);
    msleep(50);
    ast_adc_write_reg(0x0001001F, AST_ADC_ENGINE_CONTROL_REG); 
    msleep(50);

	reg = ast_adc_read_reg(AST_ADC_ENGINE_CONTROL_REG + 0x10);
	compensating_value = (reg &0x3FF);
    compensating_value = (0x200 - compensating_value);
        
	//Disable compensation channel
	ast_adc_write_reg(0x0000000F, AST_ADC_ENGINE_CONTROL_REG);
#else
	msleep(50);     
	//Auto compensating sensing mode 
    ast_adc_write_reg(0x0000002F, AST_ADC_ENGINE_CONTROL_REG);
    while(1)
    {
    	msleep(50);
    	//Wait for BIT5 reset to 0
    	reg = ast_adc_read_reg(AST_ADC_ENGINE_CONTROL_REG);
    	if((reg & 0x00000020) == 0x0)
    		break;
    }

	reg = ast_adc_read_reg(AST_ADC_ENGINE_CONTROL_REG + 0xC4);
	compensating_value = ((reg >> 16) & 0x3FF);
    compensating_value = (0x200 - compensating_value);
	
#endif
	printk( "ADC auto compensating value: %d\n", compensating_value );	

}

#if 0
#define SCU_HPLL_PARAM                (AST_SCU_VA_BASE + 0x24)
#define SCU_CLK_SELECTION             (AST_SCU_VA_BASE + 0x08)

static int adc_get_delaytime(void)
{
	uint32_t reg, delaytime;
	int t1, t2;
	int P, M, N, HPLL, PCLK, PDivider, PDividerID;

	reg = ioread32((void * __iomem)SCU_HPLL_PARAM);
	N = reg & 0x1F;
	M = (reg >> 5) & 0xFF;
	P = (reg >> 13) & 0x2F;
	HPLL = 24000000 * ((M+1) / (N+1)) / (P+1);
	
	reg = ioread32((void * __iomem)SCU_CLK_SELECTION);
	PDividerID = (reg >> 23) & 0x7;
	PDivider = (PDividerID * 4) + 4;
	PCLK = HPLL / PDivider;
	
	reg |= (1 << 23); /* Reset ADC */
	iowrite32(reg, (void * __iomem)SCU_SYS_RESET_REG);
	reg &= ~(1 << 23);

	//sensing cycle = 12 period of PCLK * 2 * (ADC0C[31:17]+1) * (ADC0C[9:0]+1)
	reg = ast_adc_read_reg(AST_ADC_ENGINE_CONTROL_REG + 0x0C);
	t1 = ((reg >> 17) & 0x7FFF) + 1;
	t2 = (reg & 0x3FF) + 1;
	PCLK = PCLK / 1000000;
	delaytime = 12 * 2 * t1 * t2 / PCLK;
	printk( "ADC sensing cycle delay time: %d ns\n", delaytime );	
	// convert to ms
	delaytime = (delaytime / 1000) + 1;
	if( delaytime < 2 )
		delaytime = 2;
	return delaytime;
}
#endif

static void adc_compensating_sensing_mode(void)
{
	uint32_t i, reg, value, sum;

    sum = 0;
	msleep(50);     
	//compensating sensing mode 
    ast_adc_write_reg(0x0000001F, AST_ADC_ENGINE_CONTROL_REG);
	msleep(50);
    ast_adc_write_reg(0x0001001F, AST_ADC_ENGINE_CONTROL_REG);	
    for( i = 0; i < 10; i++ )
    {
		//waitting the sensing cycle = 12 period of PCLK * 2 * (ADC0C[31:17]+1) * (ADC0C[9:0]+1)
		// if PCLK = 24.75MHz, and the sensing cycle will under 0.5 ms, delay 2 ms is safe enough
		msleep(2);
		reg = ast_adc_read_reg(AST_ADC_ENGINE_CONTROL_REG + 0x10);
		value = (reg & 0x3FF);
		value = (0x200 - value);
		sum += value;
	}
    ast_adc_write_reg(0x0000000F, AST_ADC_ENGINE_CONTROL_REG);
    compensating_value = sum / 10;
    printk( "ADC compensating value: %d\n", compensating_value );	
}

int adc_read_compensating_value(void)
{	
	if( compensating_mode == 0 )
		adc_compensating_sensing_mode();
	else
		adc_audo_compensating_sensing_mode();
	return 0;
}


int adc_measure(uint16_t adc_channel_selection)
{
	volatile unsigned long raw_data;
	raw_data = ast_adc_read_reg(AST_ADC_DATA_REG + 4 * (adc_channel_selection / 2));
	if ((adc_channel_selection % 2)) {
		raw_data = (raw_data >> 16) & 0x3ff;
	}
	else {
		raw_data &= 0x3ff;
	}

        /* Add compensating value to every channel reading, only if the reading is non-zero. */

        if((raw_data) && (compensating_value))
	{
		raw_data = (int64_t)raw_data + compensating_value;
        if((long)raw_data < 0)
            raw_data = 0;
	}
	return raw_data;
}

/*
int set_adc_clock(uint16_t adc_clock_divisor, uint16_t inverse)
{
	uint32_t reg;
	reg = ast_adc_read_reg(AST_ADC_CLOCK_CONTROL_REG);
	reg &= ~((0x01 << AST_ADC_CLOCK_INVERSE) + AST_ADC_DIVISOR_OF_ADC_CLOCK);
	reg |= (adc_clock_divisor | (inverse << AST_ADC_CLOCK_INVERSE));

	return 1;
}
*/

/*
 * adc_read_channel
 */
int
adc_read_channel(uint16_t *adc_value, int channel)
{	
	enable_adc_channel(channel);
	msleep(1);
	*adc_value = adc_measure(channel);
	//disable_adc_channel(channel);

	return 0;
}

/*
 * adc_get_resolution
 */
int
adc_get_resolution(uint16_t *adc_resolution)
{
	*adc_resolution = ADC_NUM_BITS_FOR_RESOLUTION;
	return 0;
}

/*
 * adc_get_resolution
 */
int
adc_get_reference_voltage(uint16_t *adc_ref_volatge)
{
	*adc_ref_volatge = ADC_REF_VOLTAGE_IN_MVOLTS;
	return 0;
}

/*
 * adc_reboot_notifier
 */
int
adc_reboot_notifier(void)
{
	return 0;
}

static int adc_set_compensating_mode(uint16_t mode)
{
	uint32_t reg;
	int i = 0;
	int max_adc_channels = CONFIG_SPX_FEATURE_GLOBAL_MAX_ADC_CHANNELS;
	
	if( mode == compensating_mode )
		return 0;
	compensating_mode = mode;

	/* unlock SCU */
	iowrite32(AST_SCU_UNLOCK_MAGIC, (void * __iomem)SCU_KEY_CONTROL_REG); /* unlock SCU */
	reg = ioread32((void * __iomem)SCU_SYS_RESET_REG);
	reg |= (1 << 23); /* Reset ADC */
	iowrite32(reg, (void * __iomem)SCU_SYS_RESET_REG);
	reg &= ~(1 << 23);
	iowrite32(reg, (void * __iomem)SCU_SYS_RESET_REG);
	iowrite32(0, (void * __iomem)SCU_KEY_CONTROL_REG); /* lock SCU */

	//Engine Clock
	ast_adc_write_reg(0x140, AST_ADC_CLOCK_CONTROL_REG);
#if defined(SOC_AST2500)
    //Initialize Sequence, enable Engine and Normal Operation Mode
    ast_adc_write_reg(0x0000f, AST_ADC_ENGINE_CONTROL_REG);
    while(1)
    {
        msleep(1);
        //Wait for BIT8 reset to 1
        reg = ast_adc_read_reg(AST_ADC_ENGINE_CONTROL_REG);
        if((reg & 0x00000100) != 0x0)
            break;
    }
#else
	//Enable Engine, Channel0 and Normal Operation Mode
	ast_adc_write_reg(0x1000f, AST_ADC_ENGINE_CONTROL_REG);
#endif
	
	adc_read_compensating_value();
	
	// Enable maximum ADC channels
	for (i = 0; i < max_adc_channels; i++)
		enable_adc_channel(i);
	
	return 1;
}
/*
 * adc_hw_module_exit
 */
static void
adc_hw_module_exit(void)
{
	unregister_hw_hal_module (EDEV_TYPE_ADC, m_dev_id);

	if (ast_adc_virt_base) iounmap(ast_adc_virt_base);
	ast_adc_virt_base = NULL;

	printk ("AST2300/AST2400 ADC HW module unloaded\n");
}


/*
 * adc_hw_module_init
 */
static int adc_hw_module_init(void)
{
	int ret;
	uint32_t reg;
	int i = 0;
	int max_adc_channels = CONFIG_SPX_FEATURE_GLOBAL_MAX_ADC_CHANNELS;

	extern int adc_core_loaded;
	if (!adc_core_loaded)
			return -1;

	if ((ret = register_hw_hal_module (&hw_hal, (void **)&padc_core_funcs)) < 0)
	{
		printk (KERN_ERR "%s: failed to register adc hal module\n", ADC_HW_DEV_NAME);
		return ret;
	}
	m_dev_id = ret;

	if((ast_adc_virt_base = ioremap(AST_ADC_REG_BASE, AST_ADC_REG_SIZE)) == NULL)
	{
		printk("failed to map adc  IO space to memory\n");
		unregister_hw_hal_module (EDEV_TYPE_ADC, m_dev_id);
		return -EIO;
	}

	/*
	   //1. The recommend ADC divisor of clock is from 120Khz ~ 6Mhz in voltage and 120Khz ~ 1Mhz in temperature
	   //2. ADC measure will use 12T
	   //3. We will enable ADC channel0, Normal operation mode and divisor of clock to 0x40 as default in this sample (it's around 200Khz)
	*/
	/* unlock SCU */
	iowrite32(AST_SCU_UNLOCK_MAGIC, (void * __iomem)SCU_KEY_CONTROL_REG); /* unlock SCU */
	reg = ioread32((void * __iomem)SCU_SYS_RESET_REG);
	reg |= (1 << 23); /* Reset ADC */
	iowrite32(reg, (void * __iomem)SCU_SYS_RESET_REG);
	reg &= ~(1 << 23);
	iowrite32(reg, (void * __iomem)SCU_SYS_RESET_REG);
	iowrite32(0, (void * __iomem)SCU_KEY_CONTROL_REG); /* lock SCU */

	//Engine Clock
	ast_adc_write_reg(0x140, AST_ADC_CLOCK_CONTROL_REG);
#if defined(SOC_AST2500)
    //Initialize Sequence, enable Engine and Normal Operation Mode
    ast_adc_write_reg(0x0000f, AST_ADC_ENGINE_CONTROL_REG);
    while(1)
    {
        msleep(1);
        //Wait for BIT8 reset to 1
        reg = ast_adc_read_reg(AST_ADC_ENGINE_CONTROL_REG);
        if((reg & 0x00000100) != 0x0)
            break;
    }
#else
	//Enable Engine, Channel0 and Normal Operation Mode
	ast_adc_write_reg(0x1000f, AST_ADC_ENGINE_CONTROL_REG);
#endif

	adc_read_compensating_value();
	
	// Enable maximum ADC channels
	for (i = 0; i < max_adc_channels; i++)
		enable_adc_channel(i);

	printk ("ADC HW module loaded\n");

	return 0;

}

module_init(adc_hw_module_init);
module_exit(adc_hw_module_exit);

MODULE_AUTHOR("American Megatrends Inc");
MODULE_DESCRIPTION("ADC HW driver module.");
MODULE_LICENSE("GPL");
