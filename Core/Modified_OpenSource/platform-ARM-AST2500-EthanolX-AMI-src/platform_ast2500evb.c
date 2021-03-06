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
 * platform-specific initialization module for AST2500EVB
 *
 ****************************************************************/

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/delay.h>
#include <linux/ioport.h>
#include <asm/io.h>
#include <asm/uaccess.h>
#include <mach/platform.h>

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

int __init init_module(void)
{
	uint32_t reg;

	/* multi-function pin configuration */
	iowrite32(0x1688A8A8, (void __iomem*)SCU_KEY_CONTROL_REG); /* unlock SCU */

	reg = ioread32((void __iomem*)AST_SCU_VA_BASE + 0x88);
	reg |= AST_SCU_MULTI_PIN_MAC1_MDIO | AST_SCU_MULTI_PIN_MAC1_MDC;
	iowrite32(reg, (void __iomem*)AST_SCU_VA_BASE + 0x88);

	reg = ioread32((void __iomem*)AST_SCU_VA_BASE + 0x90);
	reg |= AST_SCU_MULTI_PIN_MAC2_MD
		| AST_SCU_MULTI_PIN_I2C3 | AST_SCU_MULTI_PIN_I2C4 | AST_SCU_MULTI_PIN_I2C5 | AST_SCU_MULTI_PIN_I2C6
		| AST_SCU_MULTI_PIN_I2C7 | AST_SCU_MULTI_PIN_I2C8 | AST_SCU_MULTI_PIN_I2C9 
		| AST_SCU_MULTI_PIN_I2C12 | AST_SCU_MULTI_PIN_I2C13 | AST_SCU_MULTI_PIN_I2C14;
	iowrite32(reg, (void __iomem*)AST_SCU_VA_BASE + 0x90);

#if defined(SOC_AST2530)
	reg = ioread32((void __iomem*)AST_SCU_VA_BASE + 0x0C);  // SCU0C: Clock Stop Control Register
	reg &= ~((uint32_t)(AST_SCU_STOP_TX_CLK | AST_SCU_EN_REF_CLK_DIV));
	iowrite32(reg, (void __iomem*)AST_SCU_VA_BASE + 0x0C);

    reg = ioread32((void __iomem*)AST_SCU_VA_BASE + 0xDC);  // SCUDC: Clock Stop Control Register Set 2
	reg &= ~((uint32_t)(AST_SCU_STOP_TX_CLK | AST_SCU_EN_REF_CLK_DIV));
	iowrite32(reg, (void __iomem*)AST_SCU_VA_BASE + 0xDC);

	reg = ioread32((void __iomem*)AST_SCU_VA_BASE + 0x90);  // SCU90: Multi-function Pin Control #5
	reg |= AST_SCU_MULTI_PIN_UART6;
	iowrite32(reg, (void __iomem*)AST_SCU_VA_BASE + 0x90);

	reg = ioread32((void __iomem*)AST_SCU_VA_BASE + 0x94); // SCU94: Multi-function Pin Control #6
    reg |= AST_SCU_MULTI_PIN_UART7 | AST_SCU_MULTI_PIN_UART8
		| AST_SCU_MULTI_PIN_UART9 | AST_SCU_MULTI_PIN_UART10 | AST_SCU_MULTI_PIN_UART11
		| AST_SCU_MULTI_PIN_UART12 | AST_SCU_MULTI_PIN_UART13;
	iowrite32(reg, (void __iomem*)AST_SCU_VA_BASE + 0x94);
#endif

	reg = ioread32((void __iomem*)AST_SCU_VA_BASE + 0x80);  // AST_SCU_MULTI_FUNC_1 = 0x80
    reg |= 0x00006000;                      // GPIO_B5, GPIO_B6 set to function1, E0 to E7 set to GPIO
	iowrite32(reg, (void __iomem*)AST_SCU_VA_BASE + 0x80);	 // enable UART 3

	reg = ioread32((void __iomem*)AST_SCU_VA_BASE + 0x84);  // AST_SCU_MULTI_FUNC_2 = 0x84
	reg |= 0x00ff0000;                       // AST_SCU_MULTI_PIN_UART1 = 0x00ff0000
	iowrite32(reg, (void __iomem*)AST_SCU_VA_BASE + 0x84);	 // enable UART 1

	reg = ioread32((void __iomem*)AST_SCU_VA_BASE + 0x88);	 //Set SCU88[0]=1  
	reg |= 0xFC0000FF;						 //Enable PWM 0 - PWM 7 and SPI2 control for FPGA SPI
	iowrite32(reg, (void __iomem*)AST_SCU_VA_BASE + 0x88);	 //Enable PWM 0 - 7

	reg = ioread32((void __iomem*)AST_SCU_VA_BASE + 0x2C);  // AST_SCU_MULTI_FUNC_3 = 0x2C
	reg |= 0x20000000;                       // SCU2C [29] =1, set GPIOQ7 as PEWAKE
	iowrite32(reg, (void __iomem*)AST_SCU_VA_BASE + 0x2C);

	iowrite32(0, (void __iomem*)SCU_KEY_CONTROL_REG); /* lock SCU */

	/* GPIO value initialization */
	reg = ioread32((void __iomem*)GPIO_DATA_REG + 0x00);  // GPIO_ABCD
    reg |= 0x1f000000;  //Data of GPIO_ABCD or Host unbootable
	iowrite32(reg, (void __iomem*)GPIO_DATA_REG);
	
	iowrite32(0, (void __iomem*)GPIO_DATA_REG + 0x20);  // GPIO_EFGH

	reg = ioread32((void __iomem*)GPIO_DATA_REG + 0x78);  // GPIO_MNOP
    reg &= 0x11111100;                                  // Data of GPIO_M set to 0
	iowrite32(reg, (void __iomem*)GPIO_DATA_REG + 0x78);

    /* GPIO direction initialization */

    reg = ioread32((void __iomem*)GPIO_DIRECTION_REG + 0x00);  // GPIO_ABCD
    reg &= 0x0;
    reg |= 0x7f00000c;  //Direction GPIO_ABCD
	iowrite32(reg, (void __iomem*)GPIO_DIRECTION_REG + 0x00);

    reg = ioread32((void __iomem*)GPIO_DIRECTION_REG + 0x20);  // GPIO_EFGH
    reg &= 0x0;
    reg |= 0x00000000;  //Direction GPIO_EFGH 
	iowrite32(reg, (void __iomem*)GPIO_DIRECTION_REG + 0x20);

    reg = ioread32((void __iomem*)GPIO_DIRECTION_REG + 0x70);  // GPIO_IJKL
    reg &= 0x0;
    reg |= 0x00000000;  //Direction GPIO_IJKL
	iowrite32(reg, (void __iomem*)GPIO_DIRECTION_REG + 0x70);

    reg = ioread32((void __iomem*)GPIO_DIRECTION_REG + 0x78);  // GPIO_MNOP
    reg &= 0x0;
    reg |= 0x000000ff;  //Direction GPIO_MNOP
	iowrite32(reg, (void __iomem*)GPIO_DIRECTION_REG + 0x78); 

    reg = ioread32((void __iomem*)GPIO_DIRECTION_REG + 0x80);  // GPIO_QRST
    reg &= 0x0;
    reg |= 0x00000000;  //Direction GPIO_QRST
	iowrite32(reg, (void __iomem*)GPIO_DIRECTION_REG + 0x80);

    reg = ioread32((void __iomem*)GPIO_DIRECTION_REG + 0x88);  // GPIO_UVWX
    reg &= 0x0;
    reg |= 0x00000000;  //Direction GPIO_UVWX
	iowrite32(reg, (void __iomem*)GPIO_DIRECTION_REG + 0x88);

    reg = ioread32((void __iomem*)GPIO_DIRECTION_REG + 0x1e0);  // GPIO_YZAAAB
    reg &= 0x0;
    reg |= 0x00000000;  //Direction GPIO_YZAAAB
	iowrite32(reg, (void __iomem*)GPIO_DIRECTION_REG + 0x1e0);

    reg = ioread32((void __iomem*)GPIO_DIRECTION_REG + 0x1e8);  // GPIO_AC
    reg &= 0x0;
    reg |= 0x00000000;  //Direction GPIO_AC
	iowrite32(reg, (void __iomem*)GPIO_DIRECTION_REG + 0x1e8);

	return 0;
}

void __exit cleanup_module(void)
{
	/* do nothing */
}

MODULE_AUTHOR("American Megatrends Inc.");
MODULE_DESCRIPTION("platform-specific initialization module for AST2500EVB");
MODULE_LICENSE("GPL");
