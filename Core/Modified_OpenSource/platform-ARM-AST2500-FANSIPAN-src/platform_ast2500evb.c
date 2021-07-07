/****************************************************************
 ****************************************************************
 **                                                            **
 **    (C)Copyright 2006-2009, American Megatrends Inc.        **
 **                                                            **
 **            All Rights Reserved.                            **
 **                                                            **
 **        5555 Oakbrook Pkwy Suite 200, Norcross              **
 **                                                            **
 **        Georgia - 30093, USA. Phone-(770)-246-8600.         **
 **                                                            **
 ****************************************************************
 ****************************************************************/

/****************************************************************
 *
 * platform_ast2500evb.c
 * platform-specific initialization module for FANSIPAN
 *
 ****************************************************************/

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/delay.h>
#include <linux/ioport.h>
#include <asm/io.h>
#include <asm/uaccess.h>

#define AST_SCU_MULTI_PIN_I2C14			0x08000000 /* bit 27 */
#define AST_SCU_MULTI_PIN_I2C13			0x04000000 /* bit 26 */
#define AST_SCU_MULTI_PIN_I2C12			0x02000000 /* bit 25 */
#define AST_SCU_MULTI_PIN_I2C11			0x01000000 /* bit 24 */
#define AST_SCU_MULTI_PIN_I2C10			0x00800000 /* bit 23 */
#define AST_SCU_MULTI_PIN_I2C9			0x00400000 /* bit 22 */
#define AST_SCU_MULTI_PIN_I2C8			0x00200000 /* bit 21 */
#define AST_SCU_MULTI_PIN_I2C7			0x00100000 /* bit 20 */
#define AST_SCU_MULTI_PIN_I2C6			0x00080000 /* bit 19 */
#define AST_SCU_MULTI_PIN_I2C5			0x00040000 /* bit 18 */
#define AST_SCU_MULTI_PIN_I2C4			0x00020000 /* bit 17 */
#define AST_SCU_MULTI_PIN_I2C3			0x00010000 /* bit 16 */

#define AST_SCU_MULTI_PIN_MAC2_MD		0x00000004 /* bit 2 */

#define AST_SCU_MULTI_PIN_MAC1_MDIO		0x80000000 /* bit 31 */
#define AST_SCU_MULTI_PIN_MAC1_MDC		0x40000000 /* bit 30 */

#define AST_SCU_MULTI_PIN_SDHC2			0x00000002 /* bit 1 */
#define AST_SCU_MULTI_PIN_SDHC1			0x00000001 /* bit 0 */

#if defined(SOC_AST2530)
#define AST_SCU_STOP_TX_CLK     	    0x00000008 /* bit 3 */
#define AST_SCU_EN_REF_CLK_DIV     	    0x00000010 /* bit 4 */

#define AST_SCU_MULTI_PIN_UART6		    0x00000080 /* bit 7 */

#define AST_SCU_MULTI_PIN_UART7		    0x00000020 /* bit 5 */
#define AST_SCU_MULTI_PIN_UART8		    0x00000040 /* bit 6 */
#define AST_SCU_MULTI_PIN_UART9		    0x00000080 /* bit 7 */
#define AST_SCU_MULTI_PIN_UART10	    0x00000100 /* bit 8 */
#define AST_SCU_MULTI_PIN_UART11	    0x00000200 /* bit 9 */
#define AST_SCU_MULTI_PIN_UART12	    0x00000400 /* bit 10 */
#define AST_SCU_MULTI_PIN_UART13	    0x00000800 /* bit 11 */
#endif

#define AST_SCU_MULTI_PIN_SD2       0x00000002 /* bit 1 */
#define AST_SCU_DISABLE_SPI_INF     0xFFFFCFFF /* bits [13:12] = 00 */
#define AST_SCU_ENABLE_SPI_MASTER   0x00001000 /* bits [13:12] = 01 */
#define AST_SCU_ENABLE_LPC_MODE     0x00000000 /* bit 25 = 0 */
#define AST_SCU_DISABLE_LPC_MODE    0x02000000 /* bit 25 = 1 */

#define AST_SCU_ENABLE_LPC_LPCRST   0x00000080 /* bit 7 */
#define AST_SCU_ENABLE_LPC_LSIRQ    0x00000040 /* bit 6 */
#define AST_SCU_ENABLE_LPC_LFRAME   0x00000020 /* bit 5 */
#define AST_SCU_ENABLE_LPC_LCLK     0x00000010 /* bit 4 */
#define AST_SCU_ENABLE_LPC_LAD3     0x00000008 /* bit 3 */
#define AST_SCU_ENABLE_LPC_LAD2     0x00000004 /* bit 2 */
#define AST_SCU_ENABLE_LPC_LAD1     0x00000002 /* bit 1 */
#define AST_SCU_ENABLE_LPC_LAD0     0x00000001 /* bit 0 */

int __init init_module(void)
{
	uint32_t reg;

	/* multi-function pin configuration */
	iowrite32(0x1688A8A8, (void __iomem*)SCU_KEY_CONTROL_REG); /* unlock SCU */

#ifdef CONFIG_SPX_FEATURE_ONLINE_HOST_FWUPDATE
	/* Configure following multi pin function
	 * configure MAC#1 MDC1/MDIO1 to GPIO pins
	 */
	reg = ioread32((void __iomem*)AST_SCU_VA_BASE + 0x88);
	reg &= ~(AST_SCU_MULTI_PIN_MAC1_MDIO | AST_SCU_MULTI_PIN_MAC1_MDC);
	iowrite32(reg, (void __iomem*)AST_SCU_VA_BASE + 0x88);
#else
	/* Configure following multi pin function
	 * Enable MAC#1 MDC1/MDIO1 function pins
	 */
	reg = ioread32((void __iomem*)AST_SCU_VA_BASE + 0x88);
	reg |= AST_SCU_MULTI_PIN_MAC1_MDIO | AST_SCU_MULTI_PIN_MAC1_MDC;
	iowrite32(reg, (void __iomem*)AST_SCU_VA_BASE + 0x88);
#endif //CONFIG_SPX_FEATURE_ONLINE_HOST_FWUPDATE

	/* Configure following multi pin function
	 * Enable SD2 function pins
	 * Enable MAC#2 MDC2/MDIO2 function pins
	 * Enable I2C pin configuration for I2C-[3:9] and I2C-14
	 */
	reg = ioread32((void __iomem*)AST_SCU_VA_BASE + 0x90);
	reg |= AST_SCU_MULTI_PIN_MAC2_MD | AST_SCU_MULTI_PIN_SD2 |
		AST_SCU_MULTI_PIN_I2C3 | AST_SCU_MULTI_PIN_I2C4 |
		AST_SCU_MULTI_PIN_I2C5 | AST_SCU_MULTI_PIN_I2C6 |
		AST_SCU_MULTI_PIN_I2C7 | AST_SCU_MULTI_PIN_I2C8 |
		AST_SCU_MULTI_PIN_I2C9 | AST_SCU_MULTI_PIN_I2C10 |
		AST_SCU_MULTI_PIN_I2C14;
	iowrite32(reg, (void __iomem*)AST_SCU_VA_BASE + 0x90);

	/* Configure following multi pin function
	 * GPIOAC0_ESPID0_LAD0 to GPIO mode for BMC_GPIOAC0_SPARE_S
     * GPIOAC2_ESPID2_LAD2 to GPIO mode for BMC_SPI0_PROGRAM_SEL
     * GPIOAC3_ESPID3_LAD3 to GPIO mode for BMC_SPI0_BACKUP_SEL
     * GPIOAC4_ESPICK_LCLK to GPIO mode for BMC_OK
     * GPIOAC5_ESPICS#_LFRAME to GPIO mode for BMC_READY
     * GPIOAC6_ESPIALT#_LSIRQ# to GPIO mode for S1_BMC_GPIOAC6_SLAVE_PRESENT_L
     * GPIOAC7_ESPIRST#_LPCRST# to GPIO mode for BMC_OCP_PG
     * Condition 1: STRAP[25] =0
     * Condition 2: SCUAC[2:5] =0 and SCUAC[6] = 0
     */
    reg = ioread32((void __iomem *)AST_SCU_VA_BASE +  0x70);
    reg &= ~AST_SCU_DISABLE_LPC_MODE;
    iowrite32(reg, (void __iomem*)AST_SCU_VA_BASE + 0x70);

    reg = ioread32((void __iomem *)AST_SCU_VA_BASE +  0xAC);
    reg &= ~(AST_SCU_ENABLE_LPC_LPCRST | AST_SCU_ENABLE_LPC_LSIRQ |
    		 AST_SCU_ENABLE_LPC_LCLK | AST_SCU_ENABLE_LPC_LAD3 |
    		 AST_SCU_ENABLE_LPC_LAD2 | AST_SCU_ENABLE_LPC_LAD0 |
		 AST_SCU_ENABLE_LPC_LFRAME);
    iowrite32(reg, (void __iomem*)AST_SCU_VA_BASE + 0xAC);

    /* Configure following multi pin function
     * GPIOI[4:7] to SPI mode for BMC SPI1 controller
     * Condition 1: COND1 (SCU90[6]=0) (this bit always be 0)
     * Condition 2: STRAP[13:12] = 01 (Enable SPI master mode)
     */
    reg = ioread32((void __iomem *)AST_SCU_VA_BASE +  0x70);
    reg &= AST_SCU_DISABLE_SPI_INF;
    reg |= AST_SCU_ENABLE_SPI_MASTER;
    iowrite32(reg, (void __iomem*)AST_SCU_VA_BASE + 0x70);

	/*
	 * Configure following multi pin function to enable UART3/4
	 * Condition 1: SCU80[16:31] = 1
	 * Condition 2: COND2 (SCU94[1:0]=0) (this bit always be 0)
	 */
    reg = ioread32((void __iomem*)AST_SCU_VA_BASE + 0x80);
    reg |= 0xffff0000;
	/*
	 * TODO:
	 * Currently, GPIOF[0:5] are used for GPIO pin function, including:
	 * + SYS_ATX_ON
	 * + S0/S1_DDR_SAVE
	 * + ATX_PG
	 * + BMC_GPIOF0/F1_SPARE
	 * So must configure these bits to 0 (GPIO Mode)
	 * With this setting BMC UART4 will have the same setting as BMC UART1.
	 * Will check the SOL function for BMC UART4 later and update if there
	 * are any issue
	 */
	/*
	 * Force BMC UART3 to 2-pin mode because current host ATF configures
	 * UART4 in 2-pin mode
	 */
    reg &= ~0x3F3F0000;
    iowrite32(reg, (void __iomem*)AST_SCU_VA_BASE + 0x80);

	/*
	 * Configure following multi pin function to enable UART1/2
	 * Condition 1: SCU84[16:31] = 1
	 * Condition 2: COND2 (SCU94[1:0]=0) (this bit always be 0)
	 */
	reg = ioread32((void __iomem*)AST_SCU_VA_BASE + 0x84);
	reg |= 0xffff0000;
	/*
	 * FIXME:
	 * BMC UART1 is configured 4-pin mode, meanwhile current host BIOS
	 * configures UART0 in 2-pin mode. This makes Serial Over LAN fail,
	 * can only read but cannot write.
	 * Force BMC UART1 to 2-pin mode to be compatible with host sw.
	 * This must be removed when BIOS supports UART 4-pin mode.
	 *
	 * BMC UART2 is configured 4-pin mode, meanwhile current host SCP
	 * configures UART1 in 2-pin mode. This makes Serial Over LAN fail,
	 * can only read but cannot write.
	 * Force BMC UART2 to 2-pin mode to be compatible with host sw.
	 * This must be removed when SCP support UART 4-pin mode
	 */
	reg &= ~0x3F3F0000;
	iowrite32(reg, (void __iomem*)AST_SCU_VA_BASE + 0x84);

	iowrite32(0, (void __iomem*)SCU_KEY_CONTROL_REG); /* lock SCU */

	return 0;
}

void __exit cleanup_module(void)
{
	/* do nothing */
}

MODULE_AUTHOR("American Megatrends Inc.");
MODULE_DESCRIPTION("platform-specific initialization module for FANSIPAN");
MODULE_LICENSE("GPL");
