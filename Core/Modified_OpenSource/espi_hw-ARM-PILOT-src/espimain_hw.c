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
 * File name: espimain_hw.c
 * eSPI hardware driver is implemented for hardware controller.
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
#include "espi.h"
#include "espi_ioctl.h"
#include "espi_hw.h"


static void *pilot_espi_virt_base;

static spinlock_t espivw_lock;
static int pilot_espi_hal_id;
static espi_core_funcs_t *espi_core_ops;

static unsigned char first_vw_triggered = 0;
static unsigned char espi_reset_irq_requested = 0;

static irqreturn_t espi_handler_peripheral( int this_irq, void *dev_id);
static irqreturn_t espi_handler_vwire( int this_irq, void *dev_id);
static irqreturn_t espi_reset_handler(int irq, void *dev_id);

static int pilot_send_vwire(unsigned char index, unsigned char value);

struct HostToBMC_VW_INDEX
{
    const unsigned char index;
    unsigned char prev_value;
    unsigned char curr_value;
    int espi_reset_type;
};

struct HostToBMC_VW_INDEX Host2BMC_VW_INDEX[] = {
    {0x02, 0, 0, ESPI_NONE},
    {0x03, 0, 0, ESPI_RESET},
    {0x07, 0, 0, ESPI_PLTRST | ESPI_RESET},
    {0x41, 0, 0, ESPI_RESET},
    {0x42, 0, 0, ESPI_NONE},
    {0x43, 0, 0, ESPI_RESET},
    {0x44, 0, 0, ESPI_RESET},
    {0x47, 0, 0, ESPI_PLTRST | ESPI_RESET}
};

static unsigned char BMC2Host_VW_INDEX[] = {
    0x4,
    0x5,
    0x6,
    0x40,
    0x45,
    0x46
};

#define H2B_ARRAY_SIZE    (sizeof(Host2BMC_VW_INDEX)/(sizeof(struct HostToBMC_VW_INDEX)))
#define B2H_ARRAY_SIZE    (sizeof(BMC2Host_VW_INDEX)/(1*sizeof(unsigned char)))


int espi_irq[ PILOT_ESPI_CHANNEL_NUM ] =
{
    PILOT_ESPI_IRQ,
    IRQ_ESPI_VWIRE,
};

typedef irqreturn_t (* HANDLER) ( int this_irq, void *dev_id);

static HANDLER espi_irq_handlers[ PILOT_ESPI_CHANNEL_NUM ] =
{
    espi_handler_peripheral,
    espi_handler_vwire,
};

/* -------------------------------------------------- */

inline uint8_t pilot_espi_read8_reg(uint16_t offset)
{
    return ioread8((void * __iomem)pilot_espi_virt_base + offset);
}

inline void pilot_espi_write8_reg(uint8_t value, uint16_t offset)
{
    iowrite8(value, (void * __iomem)pilot_espi_virt_base + offset);
}

inline uint32_t pilot_espi_read32_reg(uint32_t offset)
{
    return( ioread32( (void * __iomem)pilot_espi_virt_base + offset ) );
}

inline void pilot_espi_write32_reg(uint32_t value, uint32_t offset) 
{
    iowrite32(value, (void * __iomem)pilot_espi_virt_base + offset);
}
/* -------------------------------------------------- */

static unsigned char pilot_espi_num_ch(void)
{
    return PILOT_ESPI_CHANNEL_NUM;
}

void pilot_espi_reset (void)
{
    // Reset eSPI controller
    printk(KERN_WARNING "%s: pilot_espi_reset\n", PILOT_ESPI_DRIVER_NAME);
}

static void pilot_espi_set_vw_bmc2host_group_val(u8 ind, u8 val)
{
    switch(ind)
    {
        case 0x04: /* System Event */
            pilot_espi_write32_reg(val, PILOT_ESPI_VW_INDEX4);
            break;

        case 0x05: /* System Event */
            pilot_espi_write32_reg(val, PILOT_ESPI_VW_INDEX5);
            break;

        case 0x06: /* System Event */
            pilot_espi_write32_reg(val, PILOT_ESPI_VW_INDEX6);
            break;

        case 0x40:
            pilot_espi_write32_reg(val, PILOT_ESPI_VW_INDEX40);
            break;

        case 0x45:
        case 0x46:
            /* No Application */
            break;
    }
}

static void pilot_espi_get_vw_host2bmc_group_val(u8 ind, u8 *buf)
{
    switch(ind)
    {
        case 0x02: /* System Event */
            *buf = pilot_espi_read32_reg(PILOT_ESPI_VW_INDEX2);
            break;

        case 0x03: /* System Event */
            *buf = pilot_espi_read32_reg(PILOT_ESPI_VW_INDEX3);
            break;

        case 0x07: /* System Event */
            *buf = pilot_espi_read32_reg(PILOT_ESPI_VW_INDEX7);
            break;

        case 0x41:
            *buf = pilot_espi_read32_reg(PILOT_ESPI_VW_INDEX41);
            break;

        case 0x43:
            *buf = pilot_espi_read32_reg(PILOT_ESPI_VW_INDEX43);
            break;

        case 0x47:
            *buf = pilot_espi_read32_reg(PILOT_ESPI_VW_INDEX47);
            break;

        case 0x44:
            /* No Application */
        default:
            /* Failure or not in the range */
            *buf = 0xFF;
            break;
    }
}

void handle_espi_resets(int reset_type)
{
    int i = 0;

    for (i = 0;i < H2B_ARRAY_SIZE; i++)
    {
        if ((reset_type & Host2BMC_VW_INDEX[i].espi_reset_type) == reset_type)
        {
            Host2BMC_VW_INDEX[i].prev_value = 0;
            Host2BMC_VW_INDEX[i].curr_value = 0;
        }
    }

    if (reset_type == ESPI_RESET)
    {
        first_vw_triggered = 0;
    }
}

static void send_acks_if_needed(void)
{
    int i = 0;
    unsigned char value;

   
    for (i = 0; i < H2B_ARRAY_SIZE; i++)
    {
        if ((Host2BMC_VW_INDEX[i].index == VW_INDEX_41h) && ((Host2BMC_VW_INDEX[i].curr_value & 0x10) == 0x10))
        {
            value = Host2BMC_VW_INDEX[i].curr_value & 0x1;
            if (pilot_send_vwire(VW_INDEX_40h, SET_VWIRE(SUSACK_N, value)) == 0)
            {
                // clear the valid bits for this interrupt and make it as previous
                Host2BMC_VW_INDEX[i].prev_value = Host2BMC_VW_INDEX[i].curr_value;
                Host2BMC_VW_INDEX[i].curr_value &= 0xEF;
            }
        }

        if ((Host2BMC_VW_INDEX[i].index == VW_INDEX_03h) && ((Host2BMC_VW_INDEX[i].curr_value & 0x40) == 0x40))
        {
            value = (Host2BMC_VW_INDEX[i].curr_value >> 2) & 0x1;
            if (pilot_send_vwire(VW_INDEX_04h, SET_VWIRE(OOB_RST_ACK, value)) == 0)
            {
                // clear the valid bit for this interrupt and make it as previous
                Host2BMC_VW_INDEX[i].prev_value = Host2BMC_VW_INDEX[i].curr_value;
                Host2BMC_VW_INDEX[i].curr_value &= 0xBF;
            }
        }

        if ((Host2BMC_VW_INDEX[i].index == VW_INDEX_07h) && ((Host2BMC_VW_INDEX[i].curr_value & 0x10) == 0x10))
        {
            value = Host2BMC_VW_INDEX[i].curr_value & 0x1;
            if (pilot_send_vwire(VW_INDEX_06h, SET_VWIRE(HOST_RST_ACK, value)) == 0)
            {
                // clear the valid bit for this interrupt and make it as previous
                Host2BMC_VW_INDEX[i].prev_value = Host2BMC_VW_INDEX[i].curr_value;
                Host2BMC_VW_INDEX[i].curr_value &= 0xEF;
            }
        }
    }
}

static void pilot_espi_get_channel_status(u32 *gen_status, u32 *ch_status, u32 *PeripheralChMaxReadReqSize, u32 *PeripheralChMaxPayloadSizeSelected, u32 *PeripheralChMaxPayloadSizeSupported )
{    
    *gen_status = pilot_espi_read32_reg(PILOT_ESPI_GEN_CAPCONF);
    
    *ch_status = 0;
    
    if (pilot_espi_read32_reg(PILOT_ESPI_CH0_CAPCONF) & 0x3)
        *ch_status |= PERIPHERAL_CHANNEL_SUPPORTED;

    *PeripheralChMaxReadReqSize = (pilot_espi_read32_reg(PILOT_ESPI_CH0_CAPCONF) & 0x7000) >> 12;
    *PeripheralChMaxPayloadSizeSelected = (pilot_espi_read32_reg(PILOT_ESPI_CH0_CAPCONF) & 0x700) >> 8;
    *PeripheralChMaxPayloadSizeSupported = (pilot_espi_read32_reg(PILOT_ESPI_CH0_CAPCONF) & 0x70) >> 4;

    if (pilot_espi_read32_reg(PILOT_ESPI_CH1_CAPCONF) & 0x3)
        *ch_status |= VWIRE_CHANNEL_SUPPORTED;
    
#ifdef ESPI_DEBUG
    // Slave Registers
    printk("PILOT_ESPI_GEN_CAPCONF    (08h) 0x%x\n", (unsigned int)pilot_espi_read32_reg(PILOT_ESPI_GEN_CAPCONF));
    printk("PILOT_ESPI_CH0_CAPCONF    (10h) 0x%x\n", (unsigned int)pilot_espi_read32_reg(PILOT_ESPI_CH0_CAPCONF));
    printk("PILOT_ESPI_CH1_CAPCONF    (20h) 0x%x\n", (unsigned int)pilot_espi_read32_reg(PILOT_ESPI_CH1_CAPCONF));
    printk("PILOT_ESPI_CH2_CAPCONF    (30h) 0x%x\n", (unsigned int)pilot_espi_read32_reg(PILOT_ESPI_CH2_CAPCONF));
    printk("PILOT_ESPI_CH3_CAPCONF    (40h) 0x%x\n", (unsigned int)pilot_espi_read32_reg(PILOT_ESPI_CH3_CAPCONF));
#endif
}

static int auto_ack_enabled(unsigned char index, unsigned char value)
{
    if (index == VW_INDEX_05h)
    {
        if (((value & (1 << (SLAVE_BOOT_LOAD_STATUS + 4))) == (1 << (SLAVE_BOOT_LOAD_STATUS + 4))) &&
            (( (*(volatile u32 *)(IO_ADDRESS(ESPI_VW_HW_ACK_CTL))) & ESPI_HW_SLV_BLS) == 0))
        return 1;

        if (((value & (1 << (SLAVE_BOOT_LOAD_DONE + 4))) == (1 << (SLAVE_BOOT_LOAD_DONE + 4))) &&
            (( (*(volatile u32 *)(IO_ADDRESS(ESPI_VW_HW_ACK_CTL))) & ESPI_HW_SLV_BLD) == 0))
        return 1;
    }

    if ((index == VW_INDEX_40h) &&
        ((value & (1 << (SUSACK_N + 4))) == (1 << (SUSACK_N + 4))) &&
        (( (*(volatile u32 *)(IO_ADDRESS(ESPI_VW_HW_ACK_CTL))) & ESPI_HW_SUSACK) == 0))
        return 1;

    if ((index == VW_INDEX_04h) &&
        ((value & (1 << (OOB_RST_ACK + 4))) == (1 << (OOB_RST_ACK + 4))) &&
        (( (*(volatile u32 *)(IO_ADDRESS(ESPI_VW_HW_ACK_CTL))) & ESPI_HW_OOBACK) == 0))
        return 1;

    if ((index == VW_INDEX_06h) &&
        ((value & (1 << (HOST_RST_ACK + 4))) == (1 << (HOST_RST_ACK + 4))) &&
        (( (*(volatile u32 *)(IO_ADDRESS(ESPI_VW_HW_ACK_CTL))) & ESPI_HW_HSTACK) == 0))
        return 1;

    return 0;
}

static int pilot_send_vwire(unsigned char index, unsigned char value)
{
    int i = 0;

    if ((pilot_espi_read32_reg(PILOT_ESPI_CH1_CAPCONF) & CH1_VW_CHANNEL_EN) != CH1_VW_CHANNEL_EN)
    {
        printk(KERN_WARNING "%s: cannot send vwire(0x%x,0x%x), channel disabled\n", PILOT_ESPI_DRIVER_NAME, index, value);
        return -EAGAIN;
    }
    
    // Check if chip revision is A2
    if (((*(volatile u32 *)(IO_ADDRESS(SE_SYSCLK_VA_BASE + 0x50)) & 0xF) >= 2) && auto_ack_enabled(index, value))
    {
        printk(KERN_WARNING "%s: HW ACK enabled, returning\n", PILOT_ESPI_DRIVER_NAME);
        return 0;
    }

    if (!first_vw_triggered)
    {
        // Exception to this condition is (1 <<SLAVE_BOOT_LOAD_DONE) and
        // (1 << SLAVE_BOOT_LOAD_STATUS) vwires
        if (!((index == VW_INDEX_05h) &&
             (((value & ((1 << SLAVE_BOOT_LOAD_DONE) << 4)) == ((1 << SLAVE_BOOT_LOAD_DONE) << 4)) ||
             ((value & ((1 << SLAVE_BOOT_LOAD_STATUS) << 4)) == ((1 << SLAVE_BOOT_LOAD_STATUS) << 4)))))
        {
            printk(KERN_DEBUG "ESPIVW: cannot send vwire(0x%x,0x%x), host not up\n", index, value);
            return -EAGAIN;
        }
    }

    for (i = 0; i < B2H_ARRAY_SIZE; i++)
    {
        if (BMC2Host_VW_INDEX[i] == index)
        {
            pilot_espi_write8_reg(value, (PILOT_ESPI_VW_INDEX0 + index));
            //printk("put_vwire index:0x%x value:0x%x\n", index, value);
            break;
        }
    }

    if (i == B2H_ARRAY_SIZE)
    {
        printk(KERN_WARNING "%s: Unsupported B2H vwire index :%x\n", PILOT_ESPI_DRIVER_NAME, index);
        return -EINVAL;
    }

    return 0;
}

static void pilot_vw_system_event(void)
{
    int i = 0, j = 0;
    volatile unsigned char vw_status = 0;
    unsigned char temp = 0;

    for (i = 0; i < H2B_ARRAY_SIZE; i++)
    {
        vw_status = pilot_espi_read8_reg(PILOT_ESPI_VW_INDEX0 + Host2BMC_VW_INDEX[i].index);

        // Check if valid is set for any of the bits
        if ((vw_status & VW_IDX_VALID) == VW_IDX_VALID)
        {
            first_vw_triggered = 1;
            Host2BMC_VW_INDEX[i].prev_value = Host2BMC_VW_INDEX[i].curr_value;
            for (j = 0; j < 4; j++)
            {
                if ((Host2BMC_VW_INDEX[i].prev_value & (1 << j)) != (vw_status & (1 << j)))
                {
                    temp |= (1 << j);
                }

                // Check if the current vwire was triggered due to PLTRST.
                if ((Host2BMC_VW_INDEX[i].index == VW_INDEX_03h) && (j == 1))
                {
                    // When PLTRST is asserted some of the vwires are cleared
                    if ((vw_status & 0x2) == 0x0)
                    {
                        printk("ESPI PLTRST# 0\n");
                        handle_espi_resets(ESPI_PLTRST);
                    }
                }
            }
            Host2BMC_VW_INDEX[i].curr_value &= ~temp;
            Host2BMC_VW_INDEX[i].curr_value |= (((temp & 0xF) << 4) | (vw_status & 0xF));
            //printk("vwire_valid Index(%x) hwsts:0x%x swsts:0x%x\n", Host2BMC_VW_INDEX[i].index, vw_status, Host2BMC_VW_INDEX[i].curr_value);
        }
        temp = 0;
    }

    send_acks_if_needed(); 
}

static void pilot_espi_hw_init(void) 
{
    unsigned char intr_s;
    int i;

    spin_lock_init(&espivw_lock);

   // espi hardware initialization
    if ( pilot_send_vwire( VW_INDEX_05h, (unsigned char)(SET_VWIRE(SLAVE_BOOT_LOAD_DONE, 1) | SET_VWIRE(SLAVE_BOOT_LOAD_STATUS, 1)) ) != 0)
        printk(KERN_WARNING "%s: failed to send virtual wire\n", PILOT_ESPI_DRIVER_NAME);

    // If BMC is coming out of soft reset
    if (*(volatile unsigned char*)(SE_SCRATCH_128_VA_BASE + 0x7C) == 1)
    {
        printk(KERN_DEBUG "ESPIVW: Soft reset\n");

        // When the system is coming up for the first time the driver only report the VW values
        // and will not report about the valid bits since it does not know which VWIRE changed.
        // This value will be used to compare when subsequent interrupts happen and report
        // changes in a particular vwire.
        // If a SUSACK has to be sent it will be done by just looking at the valid and values.
        // i.e., see if bit 8, bit 0 & bit 1 are set in a VW Index_41(best assumption).
        // Not taking spin lock since IRQ is not enabled at this point anyways.
        for (i = 0;i < H2B_ARRAY_SIZE; i++)
        {
            Host2BMC_VW_INDEX[i].prev_value = 0;
            Host2BMC_VW_INDEX[i].curr_value = pilot_espi_read8_reg(PILOT_ESPI_VW_INDEX0 + Host2BMC_VW_INDEX[i].index);

            // If valid is set then clear the valid bits and just populate the current values
            if ((Host2BMC_VW_INDEX[i].curr_value & 0xF0) != 0)
            {
                if (Host2BMC_VW_INDEX[i].index == VW_INDEX_41h)
                {
                    // See if Index_41 valid bit is set and BIT0 is set. If so set valids for
                    // SUS_WARN# (best assumption) to aid SUSACK generation.
                    // And clear other valid bits
                    if ((Host2BMC_VW_INDEX[i].curr_value & ESPI_VW_SUS_WARN) == ESPI_VW_SUS_WARN)
                        Host2BMC_VW_INDEX[i].curr_value |= (ESPI_VW_SUS_WARN << 4);

                    Host2BMC_VW_INDEX[i].curr_value &= 0x1F;
                }
                else if (Host2BMC_VW_INDEX[i].index == VW_INDEX_03h)
                {
                    // See if Index_03 valid bit is set and BIT2 is set. If so set valids for
                    // OOB_RST_ACK(best assumption) to aid OOB_RST_ACK generation.
                    // And clear other valid bits
                    if ((Host2BMC_VW_INDEX[i].curr_value & ESPI_VW_OOB_RST_WARN) == ESPI_VW_OOB_RST_WARN)
                        Host2BMC_VW_INDEX[i].curr_value |= (ESPI_VW_OOB_RST_WARN << 4);

                    Host2BMC_VW_INDEX[i].curr_value &= 0x4F;
                }
                else if (Host2BMC_VW_INDEX[i].index == VW_INDEX_07h)
                {
                    // See if Index_07 valid bit is set and BIT2 is set. If so set valids for
                    // OOB_RST_ACK(best assumption) to aid OOB_RST_ACK generation.
                    // And clear other valid bits
                    if ((Host2BMC_VW_INDEX[i].curr_value & ESPI_VW_HST_RST_WARN) == ESPI_VW_HST_RST_WARN)
                        Host2BMC_VW_INDEX[i].curr_value |= (ESPI_VW_HST_RST_WARN<< 4);

                    Host2BMC_VW_INDEX[i].curr_value &= 0x1F;
                }
                else
                    Host2BMC_VW_INDEX[i].curr_value = 0x0F;
            }

            // If the value is 1 for any of the vwire then it means that the host has triggered
            // atleast once vwire
            if (Host2BMC_VW_INDEX[i].index == 0x02 && ((Host2BMC_VW_INDEX[i].curr_value & 0x4) == 0x4))
            {
                printk(KERN_DEBUG "Host is up\n");
                first_vw_triggered = 1;
            }
        }

        send_acks_if_needed();
    }
    else
        pilot_vw_system_event();

    // Request IRQ, Peripheral & Virtual Wire
    for (i = 1; i < PILOT_ESPI_CHANNEL_NUM; i++)
    {
        if( request_irq( espi_irq[i], espi_irq_handlers[i], IRQF_DISABLED, "pilot_espi", (pilot_espi_virt_base + i)) < 0 )
            printk(KERN_WARNING "%s: request irq %d failed\n", PILOT_ESPI_DRIVER_NAME, espi_irq[i]);
    }

    if (request_irq(IRQ_SIO_PSR, espi_reset_handler, IRQF_SHARED, "espi_reset", &espi_reset_handler) < 0)
    {
        printk("failed to request irq %d", IRQ_SIO_PSR);
    }

    espi_reset_irq_requested = 1;

    /* Enable eSPI Reset Interrupt */
    intr_s = *(unsigned char *)(SE_PILOT_SPEC_VA_BASE + 0x23);
    intr_s = intr_s | ESPI_RST_INTR_EN;
    intr_s = intr_s & 0xf7; //do not touch lock bit
    *(unsigned char *)(SE_PILOT_SPEC_VA_BASE + 0x23) = intr_s;

    /* Enable In-band reset . Do RMW so as to not overwrite other bits in this register */
    *(unsigned char *)(SE_PILOT_SPEC_VA_BASE + 0x25) |= (ESPI_INBAND_EN);

    // Enable interrupts
    pilot_espi_write32_reg(pilot_espi_read32_reg(PILOT_ESPI_VW_CTRL) | VW_INTR_EN, PILOT_ESPI_VW_CTRL);
}

/* -------------------------------------------------- */
static espi_hal_operations_t pilot_espi_hw_ops = {
    .num_espi_ch                = pilot_espi_num_ch,
    .reset_espi                 = pilot_espi_reset,
    .set_vw_bmc2host_group_val  = pilot_espi_set_vw_bmc2host_group_val,
    .get_vw_host2bmc_group_val  = pilot_espi_get_vw_host2bmc_group_val,
    .get_channel_status         = pilot_espi_get_channel_status,
};

static hw_hal_t pilot_espi_hw_hal = {
    .dev_type       = EDEV_TYPE_ESPI,
    .owner          = THIS_MODULE,
    .devname        = PILOT_ESPI_DRIVER_NAME,
    .num_instances  = PILOT_ESPI_CHANNEL_NUM,
    .phal_ops       = (void *) &pilot_espi_hw_ops
};

static int pilot_espi_reset_handler(void)
{
    int i = 0;

    // VW clears all index except 02 on a eSPI reset
    for (i = 0;i < H2B_ARRAY_SIZE; i++)
    {
        // Index 2 and 42 are not reset with eSPI reset
        if ((Host2BMC_VW_INDEX[i].index == 0x2) || (Host2BMC_VW_INDEX[i].index == 0x42))
            continue;
    
        Host2BMC_VW_INDEX[i].prev_value = 0;
        Host2BMC_VW_INDEX[i].curr_value = 0;
    }
    
    // Set SLAVE_BOOT_LOAD_DONE
    pilot_send_vwire(VW_INDEX_05h, (unsigned char)(SET_VWIRE(SLAVE_BOOT_LOAD_DONE, 1) | SET_VWIRE(SLAVE_BOOT_LOAD_STATUS, 1)) );
    
    first_vw_triggered = 0;
    
    return 0;
}

static irqreturn_t espi_reset_handler(int irq, void *dev_id)
{
    unsigned char sio_intr;
    unsigned long flags = 0;

    sio_intr = *(unsigned char*)(SE_PILOT_SPEC_VA_BASE + 0x23);

    /* If interrupte not generated by RESET edge, return */
    if (sio_intr & 0x02)
    {
        printk(KERN_DEBUG "eSPI Reset:0x%x\n", sio_intr);
        /* write back  to clear reset interrupt */
        sio_intr &= 0xf7; //do not touch lock bit
        *(unsigned char *)(SE_PILOT_SPEC_VA_BASE + 0x23) = sio_intr;

        spin_lock_irqsave(&espivw_lock, flags);
        handle_espi_resets(ESPI_RESET);
        spin_unlock_irqrestore(&espivw_lock, flags);
    }

    if ((*(unsigned char *)(SE_PILOT_SPEC_VA_BASE + 0x24) & ESPI_INBAND_STS) == ESPI_INBAND_STS)
    {
        printk(KERN_INFO "eSPI In-band reset\n");
        *(unsigned char *)(SE_PILOT_SPEC_VA_BASE + 0x24) |= ESPI_INBAND_STS;
    }

    return (IRQ_HANDLED);
}

static irqreturn_t espi_handler_peripheral( int this_irq, void *dev_id)
{
    printk(KERN_WARNING "%s: espi_handler_peripheral interrupted\n", PILOT_ESPI_DRIVER_NAME);

    return IRQ_HANDLED;
}

static irqreturn_t espi_handler_vwire( int this_irq, void *dev_id)
{
    unsigned long flags=0;

    //printk(KERN_WARNING "%s: espi_handler_vwire interrupted\n", PILOT_ESPI_DRIVER_NAME);
    spin_lock_irqsave(&espivw_lock, flags);

    // If this is the first time vwire is triggered then generate ESPI_VW_SLAVE_BOOT_LOAD_DONE and
    // ESPI_VW_SLAVE_BOOT_LOAD_STATUS. This is to handle the case where the channel was not
    // enabled at the time of driver init or after espi reset
    if (!first_vw_triggered)
    {
        // Set SLAVE_BOOT_LOAD_DONE
        pilot_send_vwire(VW_INDEX_05h, (unsigned char)(SET_VWIRE(SLAVE_BOOT_LOAD_DONE, 1) | SET_VWIRE(SLAVE_BOOT_LOAD_STATUS, 1)));
    }
    pilot_vw_system_event();
    spin_unlock_irqrestore(&espivw_lock, flags);

    return IRQ_HANDLED;
}

int pilot_espi_init(void)
{
    uint32_t status;

    extern int espi_core_loaded;
    if (!espi_core_loaded)
        return -1;

    status = *(volatile u32 *)(IO_ADDRESS(SE_SYSCLK_VA_BASE + 0x0C));
    if(!(status & (0x1 << 14)))
    {
        printk(KERN_WARNING "%s: eSPI mode is not enable 0x%x\n", PILOT_ESPI_DRIVER_NAME, (unsigned int)status);
        return -EIO;
    }

    pilot_espi_hal_id = register_hw_hal_module(&pilot_espi_hw_hal, (void **) &espi_core_ops);
    if (pilot_espi_hal_id < 0) {
        printk(KERN_WARNING "%s: register HAL HW module failed\n", PILOT_ESPI_DRIVER_NAME);
        return pilot_espi_hal_id;
    }

    pilot_espi_virt_base = ioremap_nocache(PILOT_ESPI_REG_BASE, SZ_4K);
    if (!pilot_espi_virt_base) {
        printk(KERN_WARNING "%s: ioremap failed\n", PILOT_ESPI_DRIVER_NAME);
        unregister_hw_hal_module(EDEV_TYPE_ESPI, pilot_espi_hal_id);
        return -ENOMEM;
    }

    pilot_espi_hw_init();

    install_reset_handler(pilot_espi_reset_handler);

    printk("The eSPI HW Driver is loaded successfully.\n" );
    return 0;
}

void pilot_espi_exit(void)
{
    int i;

    for (i = 1; i < PILOT_ESPI_CHANNEL_NUM; i++)
        free_irq(espi_irq[i], (pilot_espi_virt_base + i));

    if (espi_reset_irq_requested)
    {
        free_irq(IRQ_SIO_PSR, &espi_reset_handler);
        espi_reset_irq_requested = 0;
    }
    iounmap (pilot_espi_virt_base);

    uninstall_reset_handler(pilot_espi_reset_handler);

    unregister_hw_hal_module(EDEV_TYPE_ESPI, pilot_espi_hal_id);

    return;
}

module_init (pilot_espi_init);
module_exit (pilot_espi_exit);

MODULE_AUTHOR("American Megatrends Inc.");
MODULE_DESCRIPTION("PILOT SoC eSPI Driver");
MODULE_LICENSE ("GPL");
