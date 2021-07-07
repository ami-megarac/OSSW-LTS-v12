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
 * ast_postcode.h
 * ASPEED AST2100/2050/2200/2150 LPC controller Postcode-related
 * definitions, macros, prototypes
 *
*****************************************************************/

#ifndef __AST_POSTCODE_H__
#define __AST_POSTCODE_H__

#if defined(SOC_AST2300)
#define GROUP_AST2300 1
#endif
#if defined(SOC_AST2400)
#define GROUP_AST2300_PLUS 1
#endif
#if defined(CONFIG_SPX_FEATURE_BMCCOMPANIONDEVICE_AST1070)
#define GROUP_AST1070_COMPANION 1
#elif defined(SOC_AST1250)
#define GROUP_AST2300_PLUS 1
#endif
#if defined(SOC_AST1050_1070)
#define GROUP_AST1070_COMPANION 1
#endif

#define AST_POST_CODE_DMA_BUFFER_SIZE_VAL               0x400
#define AST_POST_CODE_DMA_BUFFER_BYTE_SIZE_VAL          (AST_POST_CODE_DMA_BUFFER_SIZE_VAL * 4)
#define AST_POST_CODE_DMA_DATA_LEN_MASK                 (AST_POST_CODE_DMA_BUFFER_BYTE_SIZE_VAL -1)

#define AST_POSTCODE_REG_BASE		0x1E789000
#define AST_POSTCODE_REG_SIZE		SZ_4K
#define AST_POSTCODE_IRQ			8

#define AST_POSTCODE_PORT_NUM		1
#define AST_POSTCODE_ADDR_0		0x80
#define AST_POSTCODE_ADDR_1		0x81


/* PostCode-related registers of AST LPC controller */
#define AST_LPC_PCCR0                       0x130
#define AST_LPC_PCCR1                       0x134
#define AST_LPC_PCCR2                       0x138
#define AST_LPC_PCCR3                       0x13C
#define AST_LPC_PCCR4                       0xD0
#define AST_LPC_PCCR5                       0xD4
#define AST_LPC_PCCR6                       0xC4

#define AST_LPC_LHCR5                       0xB4
#define AST_LPC_LHCRA                       0xC8

#define AST_LPC_LHCR6                       0xB8
#define AST_LPC_LHCRB                       0xCC


/* bits of LHCR5&6 */
#define AST_LPC_LHCR_PATTERN_IV_MASK        0xFF000000 /* bits[31:24] */
#define AST_LPC_LHCR_PATTERN_IV_SHIFT       24
#define AST_LPC_LHCR_PATTERN_III_MASK       0x00FF0000 /* bits[23:16] */
#define AST_LPC_LHCR_PATTERN_III_SHIFT      16
#define AST_LPC_LHCR_PATTERN_II_MASK        0x0000FF00 /* bits[15:8] */
#define AST_LPC_LHCR_PATTERN_II_SHIFT       8
#define AST_LPC_LHCR_PATTERN_I_MASK         0x000000FF /* bits[7:0] */
#define AST_LPC_LHCR_PATTERN_I_SHIFT        0

/* bits of LHCRA&B */
#define AST_LPC_LHCR_WR_CYCLE_MASK          0x00010000 /* bits[16] */
#define AST_LPC_LHCR_WR_CYCLE_SHIFT         16
#define AST_LPC_LHCR_ADDR_MASK              0x0000FFFF /* bits[15:0] */
#define AST_LPC_LHCR_ADDR_SHIFT             0

/* bits of PCCR0 */
#define AST_LPC_PCCR0_PATTERN_B_INT_MASK    0x00800000 /* bits[23] */
#define AST_LPC_PCCR0_PATTERN_B_INT_SHIFT   23
#define AST_LPC_PCCR0_PATTERN_B_MASK        0x00400000 /* bits[22] */
#define AST_LPC_PCCR0_PATTERN_B_SHIFT       22
#define AST_LPC_PCCR0_PATTERN_A_INT_MASK    0x00200000 /* bits[21] */
#define AST_LPC_PCCR0_PATTERN_A_INT_SHIFT   21
#define AST_LPC_PCCR0_PATTERN_A_MASK        0x00100000 /* bits[20] */
#define AST_LPC_PCCR0_PATTERN_A_SHIFT       20

#define AST_LPC_PCCR0_POSTCODE_DMA_MASK     0x00004000 /* bits[14] */
#define AST_LPC_PCCR0_POSTCODE_DMA_SHIFT    14

#define AST_LPC_PCCR0_ADDR_SEL_MASK         0x00003000 /* bits[13:12] */
#define AST_LPC_PCCR0_ADDR_SEL_SHIFT        12

#define AST_LPC_PCCR0_ADDR_SEL_0            0 /* Record SerIRQ */
#define AST_LPC_PCCR0_ADDR_SEL_1            1 /* Address Bit[5:4] */
#define AST_LPC_PCCR0_ADDR_SEL_2            2 /* Address Bit[7:6] */
#define AST_LPC_PCCR0_ADDR_SEL_3            3 /* Address Bit[9:8] */

#define AST_LPC_PCCR0_DATA_MODE_MASK        0x00000030 /* bits[5:4] */
#define AST_LPC_PCCR0_DATA_MODE_SHIFT       4

#define AST_LPC_PCCR0_DATA_MODE_1B          0
#define AST_LPC_PCCR0_DATA_MODE_2B          1
#define AST_LPC_PCCR0_DATA_MODE_4B          2
#define AST_LPC_PCCR0_DATA_MODE_FULL        3

#define AST_LPC_PCCR0_POSTCODE_EN_MASK      0x00000001 /* bits[0] */
#define AST_LPC_PCCR0_POSTCODE_EN_SHIFT     0

#define AST_LPC_PCCR0_DMATIMER_SEL_MASK     0x60000000 /* bits[30:29] */
#define AST_LPC_PCCR0_DMATIMER_SEL_SHIFT    29

#define AST_LPC_PCCR0_DMATIMER_SEL_1        1


/* bits of PCCR1 */
#define AST_LPC_PCCR1_DONT_CARE_ADDR_MASK   0x003F0000 /* bits[21:16] */
#define AST_LPC_PCCR1_DONT_CARE_ADDRE_SHIFT 16

#define AST_LPC_PCCR1_DONT_CARE_ADDR_1      0x1
#define AST_LPC_PCCR1_DONT_CARE_ADDR_3      0x3
#define AST_LPC_PCCR1_DONT_CARE_ADDR_7      0x7
#define AST_LPC_PCCR1_DONT_CARE_ADDR_F      0xF
#define AST_LPC_PCCR1_DONT_CARE_ADDR_1F     0x1F
#define AST_LPC_PCCR1_DONT_CARE_ADDR_3F     0x3F
#define AST_LPC_PCCR1_DONT_CARE_ADDR_7F     0x7F

#define AST_LPC_PCCR1_LPC_CAP_ADDR_MASK     0x000FFFF /* bits[15:0] */
#define AST_LPC_PCCR1_LPC_CAP_ADDR_SHIFT    0

/* bits of PCCR2 */
#define AST_LPC_PCCR2_RESET_STAT_PATTERN_A_MASK     0x0000200 /* bits[9] */
#define AST_LPC_PCCR2_RESET_STAT_PATTERN_A_SHIFT    9
#define AST_LPC_PCCR2_INT_STAT_PATTERN_A_MASK       0x0000100 /* bits[8] */
#define AST_LPC_PCCR2_INT_STAT_PATTERN_A_SHIFT      8
#define AST_LPC_PCCR2_DATA_READY_MASK               0x0000010 /* bits[4] */
#define AST_LPC_PCCR2_DATA_READY_SHIFT              4

#if (LINUX_VERSION_CODE > KERNEL_VERSION(5,0,0))
#define IO_BASE			0xF0000000                 // VA of IO
#define IO_ADDRESS(x)   ((x&0x0fffffff)+IO_BASE)
#define AST_IC_BASE                    0x1E6C0000
#define AST_IC_VA_BASE                  IO_ADDRESS(AST_IC_BASE)
#define INTERRUPT_SENS_REG				(AST_IC_VA_BASE + 0x24)
#define INTERRUPT_EVENT_REG             (AST_IC_VA_BASE + 0x2C)
#define VIC_SENSE_VA                   INTERRUPT_SENS_REG
#define VIC_EVENT_VA                   INTERRUPT_EVENT_REG
#define IRQ_SET_LEVEL_TRIGGER(irq_no)   *((volatile unsigned long*)VIC_SENSE_VA) |= 1 << (irq_no)
#define IRQ_SET_HIGH_LEVEL(irq_no)      *((volatile unsigned long*)VIC_EVENT_VA) |= 1 << (irq_no)
#define IRQ_SET_LOW_LEVEL(irq_no)       *((volatile unsigned long*)VIC_EVENT_VA) &= ~(1 << (irq_no))
#endif

#endif /* ! __AST_POSTCODE_H__ */

