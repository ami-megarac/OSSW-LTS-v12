--- uboot_old/arch/arm/cpu/pilotcommon/espi_boot.c	2019-02-21 17:56:04.811523551 +0800
+++ uboot_new/arch/arm/cpu/pilotcommon/espi_boot.c	2019-02-21 13:55:32.806687171 +0800
@@ -0,0 +1,525 @@
+#include <common.h>
+#include <asm/errno.h>
+#include <linux/compiler.h>
+#include <pilot_hw.h>
+
+
+#define ESPI_HANDSHAKE_TIMEOUT      CONFIG_EARLYBOOT_ESPI_MONITOR_TIMEOUT * 1000
+#define ESPI_WAIT_TIMEOUT           CONFIG_EARLYBOOT_ESPI_HANDSHAKE_TIMEOUT * 1000
+
+
+static unsigned char first_vw_triggered = 0;
+
+#define MAX_XFER_BUFF_SIZE              0xFFF        //4096
+
+#define PILOT_ESPI_REG_BASE             0x40423000
+#define PILOT_ESPI_IRQ                  0
+
+#define PILOT_ESPI_CHANNEL_NUM          2
+
+#define PILOT_ESPI_PERI_CHAN_ID         0
+#define PILOT_ESPI_VW_CHAN_ID           1
+#define PILOT_ESPI_OOB_CHAN_ID          2
+#define PILOT_ESPI_FLASH_CHAN_ID        3
+
+#define SE_SYSCLK_VA_BASE               0x40100100
+
+// Pilot Specific Registors (PSR)
+#define ESPI_INBAND_EN                  (1 << 7)
+#define ESPI_INBAND_STS                 (1 << 0)
+#define ESPI_RST_INTR_EN                (1 << 2)
+
+#define ESPI_VW_HW_ACK_CTL              (0x40426500 + 0x25) //SIOCFG5  //0x25unaligned-accesses
+#define ESPI_HW_SLV_BLS                 (1 << 0)
+#define ESPI_HW_SLV_BLD                 (1 << 1)
+#define ESPI_HW_SUSACK                  (1 << 2)
+#define ESPI_HW_OOBACK                  (1 << 3)
+#define ESPI_HW_HSTACK                  (1 << 4)
+
+// Slave Registers
+#define PILOT_ESPI_GEN_DEVID            0x04
+#define PILOT_ESPI_GEN_CAPCONF          0x08
+#define PILOT_ESPI_CH0_CAPCONF          0x10
+#define PILOT_ESPI_CH1_CAPCONF          0x20
+#define PILOT_ESPI_CH2_CAPCONF          0x30
+#define PILOT_ESPI_CH3_CAPCONF          0x40
+
+// eSPI register
+#define PILOT_ESPI_VW_INDEX0            (0xC00 + 0x00)
+#define PILOT_ESPI_VW_INDEX1            (0xC00 + 0x01)
+#define PILOT_ESPI_VW_INDEX2            (0xC00 + 0x02)
+#define PILOT_ESPI_VW_INDEX3            (0xC00 + 0x03)
+#define PILOT_ESPI_VW_INDEX4            (0xC00 + 0x04)
+#define PILOT_ESPI_VW_INDEX5            (0xC00 + 0x05)
+#define PILOT_ESPI_VW_INDEX6            (0xC00 + 0x06)
+#define PILOT_ESPI_VW_INDEX7            (0xC00 + 0x07)
+#define PILOT_ESPI_VW_INDEX40           (0xC00 + 0x40)
+#define PILOT_ESPI_VW_INDEX41           (0xC00 + 0x41)
+#define PILOT_ESPI_VW_INDEX42           (0xC00 + 0x42)
+#define PILOT_ESPI_VW_INDEX43           (0xC00 + 0x43)
+#define PILOT_ESPI_VW_INDEX44           (0xC00 + 0x44)
+#define PILOT_ESPI_VW_INDEX45           (0xC00 + 0x45)
+#define PILOT_ESPI_VW_INDEX46           (0xC00 + 0x46)
+#define PILOT_ESPI_VW_INDEX47           (0xC00 + 0x47)
+
+#define PILOT_ESPI_VW_CTRL              (0xC00 + 0xC0)
+#define PILOT_ESPI_VW_STATUS0           (0xC00 + 0xD0)
+#define PILOT_ESPI_VW_STATUS5           (0xC00 + 0xD5)
+
+#define PILOT_ESPI_OOB_CTRL             (0x800 + 0x00)
+#define PILOT_ESPI_OOB_STATUS           (0x800 + 0x04)
+#define PILOT_ESPI_OOB_SIZE             (0x800 + 0x08) /* 8 bit*/
+#define PILOT_ESPI_OOB_DATA             (0x800 + 0x0C) /* 8 bit*/
+#define PILOT_ESPI_OOB_CYC              (0x800 + 0x10) /* 8 bit*/
+#define PILOT_ESPI_OOB_TAG              (0x800 + 0x14) /* 8 bit*/
+#define PILOT_ESPI_OOB_LEN              (0x800 + 0x18)
+#define PILOT_ESPI_OOB_ADD              (0x800 + 0x1C)
+#define PILOT_ESPI_OOB_PSR              (0x800 + 0x20)
+
+#define PILOT_ESPI_FLASH_CTRL           (0xA00 + 0x00)
+#define PILOT_ESPI_FLASH_STATUS         (0xA00 + 0x04)
+#define PILOT_ESPI_FLASH_SIZE           (0xA00 + 0x08) /* 8 bit*/
+#define PILOT_ESPI_FLASH_DATA           (0xA00 + 0x0C) /* 8 bit*/
+#define PILOT_ESPI_FLASH_CYC            (0xA00 + 0x10) /* 8 bit*/
+#define PILOT_ESPI_FLASH_TAG            (0xA00 + 0x14) /* 8 bit*/
+#define PILOT_ESPI_FLASH_LEG            (0xA00 + 0x18)
+#define PILOT_ESPI_FLASH_ADD            (0xA00 + 0x1C)
+#define PILOT_ESPI_FLASH_PSR            (0xA00 + 0x20)
+
+#define GET_CYCLE_TYPE(x)               (x & 0xff)
+#define GET_TAG(x)                      ((x >> 8) & 0xf)
+#define GET_LEN(x)                      ((x >> 12) & 0xfff)
+
+// register bit value
+// General CAP
+#define PILOT_ESPI_OP_FREQ_MASK         (0x7 << 20)
+
+#define PILOT_ESPI_OP_FREQ_20MHZ        (0x0 << 20)
+#define PILOT_ESPI_OP_FREQ_25MHZ        (0x1 << 20)
+#define PILOT_ESPI_OP_FREQ_33MHZ        (0x2 << 20)
+#define PILOT_ESPI_OP_FREQ_50MHZ        (0x3 << 20)
+#define PILOT_ESPI_OP_FREQ_66MHZ        (0x4 << 20)
+
+// Channel#1 CAP
+#define CH1_VW_CHANNEL_EN               (1 << 0)
+
+// OOB/FLASH CTRL 00h
+#define B2H_PACKET_VALID                (1 << 0)
+#define B2H_INTR_EN                     (1 << 1)
+#define B2H_WRPTR_CLEAR                 (1 << 2)
+#define B2H_RDPTR_CLEAR                 (1 << 3)
+#define H2B_INTR_EN                     (1 << 8)
+#define H2B_WRPTR_CLEAR                 (1 << 9)
+#define H2B_RDPTR_CLEAR                 (1 << 10)
+
+// OOB/FLASH STS 04h
+#define B2H_BUSY                        (1 << 0)
+#define B2H_FIFO_EMPTY                  (1 << 1)
+#define B2H_INTR_STATUS                 (1 << 2)
+#define H2B_INTR_STATUS                 (1 << 5)
+#define H2B_FIFO_EMPTY                  (1 << 6)
+#define H2B_PACKET_VALID                (1 << 7)
+
+// VW Reset Type
+#define ESPI_NONE                       (1 << 0)
+#define ESPI_RESET                      (1 << 1)
+#define ESPI_PLTRST                     (1 << 2)
+#define ESPI_INBAND_RST                 (1 << 3)
+
+// VW CTRL C0h
+#define VW_INTR_EN                      (1 << 0)
+
+// VW List
+// VW Index
+#define VW_INDEX_03h                    0x3
+#define VW_INDEX_04h                    0x4
+#define VW_INDEX_05h                    0x5
+#define VW_INDEX_06h                    0x6
+#define VW_INDEX_07h                    0x7
+#define VW_INDEX_40h                    0x40
+#define VW_INDEX_41h                    0x41
+
+#define SET_VWIRE(bit, value)           ((value << bit) | (1 << (bit + 4)))
+
+#define VW_IDX_VALID                    (1 << 7)
+// index 04h
+#define OOB_RST_ACK                     0
+// index 05h
+#define SLAVE_BOOT_LOAD_DONE            0
+#define SLAVE_BOOT_LOAD_STATUS          3
+#define SLAVE_BOOT_LOAD_DONE_VALID      (1 << 4)
+#define SLAVE_BOOT_LOAD_STATUS_VALID    (1 << 7)
+// index 06h
+#define HOST_RST_ACK                    3
+// index 40h
+#define SUSACK_N                        0
+
+
+#define ESPI_VW_SUS_WARN                (1 << 0)
+#define ESPI_VW_OOB_RST_WARN            (1 << 2)
+#define ESPI_VW_HST_RST_WARN            (1 << 0)
+
+
+int pltRst=0;
+int handshake_start= 0 ;
+unsigned long  handshake_time =0;
+
+struct HostToBMC_VW_INDEX
+{
+    const unsigned char index;
+    unsigned char prev_value;
+    unsigned char curr_value;
+    int espi_reset_type;
+};
+static unsigned char BMC2Host_VW_INDEX[] = {
+    0x4,
+    0x5,
+    0x6,
+    0x40,
+    0x45,
+    0x46
+};
+struct HostToBMC_VW_INDEX Host2BMC_VW_INDEX[] = {
+    {0x02, 0, 0, ESPI_NONE},
+    {0x03, 0, 0, ESPI_RESET},
+    {0x07, 0, 0, ESPI_PLTRST | ESPI_RESET},
+    {0x41, 0, 0, ESPI_RESET},
+    {0x42, 0, 0, ESPI_NONE},
+    {0x43, 0, 0, ESPI_RESET},
+    {0x44, 0, 0, ESPI_RESET},
+    {0x47, 0, 0, ESPI_PLTRST | ESPI_RESET}
+};
+#define H2B_ARRAY_SIZE    (sizeof(Host2BMC_VW_INDEX)/(sizeof(struct HostToBMC_VW_INDEX)))
+#define B2H_ARRAY_SIZE    (sizeof(BMC2Host_VW_INDEX)/(1*sizeof(unsigned char)))
+
+
+/* -------------------------------------------------- */
+
+static uint8_t pilot_espi_read8_reg(volatile uint16_t offset)
+{
+	return( *((volatile uint8_t *)(PILOT_ESPI_REG_BASE + offset)) );
+}
+
+static void pilot_espi_write8_reg(volatile uint8_t value,volatile  uint16_t offset)
+{
+	*((volatile uint8_t *) (PILOT_ESPI_REG_BASE + offset)) = value;
+}
+static uint32_t pilot_espi_read32_reg(volatile uint16_t offset)
+{
+	return( *((volatile uint32_t *)(PILOT_ESPI_REG_BASE + offset)) );
+}
+
+static void pilot_espi_write32_reg(volatile uint32_t value,volatile uint16_t offset) 
+{
+	*((volatile uint32_t *) (PILOT_ESPI_REG_BASE + offset)) = value;
+}
+/*---------------------------------------------------------------------------*/
+
+
+static int pilot_auto_ack_enabled(unsigned char index, unsigned char value)
+{
+    if (index == VW_INDEX_05h)
+    {
+        if (((value & (1 << (SLAVE_BOOT_LOAD_STATUS + 4))) == (1 << (SLAVE_BOOT_LOAD_STATUS + 4))) &&
+            (( (*(volatile uint8_t *)(ESPI_VW_HW_ACK_CTL)) & ESPI_HW_SLV_BLS) == 0))
+        return 1;
+
+        if (((value & (1 << (SLAVE_BOOT_LOAD_DONE + 4))) == (1 << (SLAVE_BOOT_LOAD_DONE + 4))) &&
+            (( (*(volatile uint8_t *)(ESPI_VW_HW_ACK_CTL)) & ESPI_HW_SLV_BLD) == 0))
+        return 1;
+    }
+    if ((index == VW_INDEX_40h) &&
+        ((value & (1 << (SUSACK_N + 4))) == (1 << (SUSACK_N + 4))) &&
+        (( (*(volatile uint8_t *)(ESPI_VW_HW_ACK_CTL)) & ESPI_HW_SUSACK) == 0))
+        return 1;
+    if ((index == VW_INDEX_04h) &&
+        ((value & (1 << (OOB_RST_ACK + 4))) == (1 << (OOB_RST_ACK + 4))) &&
+        (( (*(volatile uint8_t *)(ESPI_VW_HW_ACK_CTL)) & ESPI_HW_OOBACK) == 0))
+        return 1;
+    if ((index == VW_INDEX_06h) &&
+        ((value & (1 << (HOST_RST_ACK + 4))) == (1 << (HOST_RST_ACK + 4))) &&
+        (( (*(volatile uint8_t *)(ESPI_VW_HW_ACK_CTL)) & ESPI_HW_HSTACK) == 0))
+        return 1;
+
+    return 0;
+}
+static int pilot_send_vwire(unsigned char index, unsigned char value)
+{
+    int i = 0;
+    if ((pilot_espi_read32_reg(PILOT_ESPI_CH1_CAPCONF) & CH1_VW_CHANNEL_EN) != CH1_VW_CHANNEL_EN)
+    {
+       return -1;
+    }
+    // Check if chip revision is A2
+    if (((*(volatile uint32_t *)(SE_SYSCLK_VA_BASE + 0x50) & 0xF) >= 2) && pilot_auto_ack_enabled(index, value))
+    {
+        return 0;
+    }
+    if (!first_vw_triggered)
+    {
+        // Exception to this condition is (1 <<SLAVE_BOOT_LOAD_DONE) and
+        // (1 << SLAVE_BOOT_LOAD_STATUS) vwires
+        if (!((index == VW_INDEX_05h) &&
+             (((value & ((1 << SLAVE_BOOT_LOAD_DONE) << 4)) == ((1 << SLAVE_BOOT_LOAD_DONE) << 4)) ||
+             ((value & ((1 << SLAVE_BOOT_LOAD_STATUS) << 4)) == ((1 << SLAVE_BOOT_LOAD_STATUS) << 4)))))
+        {
+            printf("ESPIVW: cannot send vwire(0x%x,0x%x), host not up\n", index, value);
+            return -1;
+        }
+    }
+    for (i = 0; i < B2H_ARRAY_SIZE; i++)
+    {
+        if (BMC2Host_VW_INDEX[i] == index)
+        {
+            pilot_espi_write8_reg(value, (PILOT_ESPI_VW_INDEX0 + index));
+            printf("put_vwire index:0x%x value:0x%x\n", index, value);
+            break;
+        }
+    }
+    if (i == B2H_ARRAY_SIZE)
+    {
+        printf("Unsupported B2H vwire index :%x\n",index);
+        return -1;
+    }
+    return 0;
+}
+
+
+/* -------------------------------------------------- */
+static void pilot_send_acks_if_needed(void)
+{
+    int i = 0;
+    unsigned char value;
+
+   
+    for (i = 0; i < H2B_ARRAY_SIZE; i++)
+    {
+        if ((Host2BMC_VW_INDEX[i].index == VW_INDEX_41h) && ((Host2BMC_VW_INDEX[i].curr_value & 0x10) == 0x10))
+        {
+            value = Host2BMC_VW_INDEX[i].curr_value & 0x1;
+            if (pilot_send_vwire(VW_INDEX_40h, SET_VWIRE(SUSACK_N, value)) == 0)
+            {
+                // clear the valid bits for this interrupt and make it as previous
+                Host2BMC_VW_INDEX[i].prev_value = Host2BMC_VW_INDEX[i].curr_value;
+                Host2BMC_VW_INDEX[i].curr_value &= 0xEF;
+                if(handshake_start ==0){
+                    handshake_time = get_timer(0);
+                    handshake_start =1 ;
+                }
+            }
+        }
+
+        if ((Host2BMC_VW_INDEX[i].index == VW_INDEX_03h) && ((Host2BMC_VW_INDEX[i].curr_value & 0x40) == 0x40))
+        {
+            value = (Host2BMC_VW_INDEX[i].curr_value >> 2) & 0x1;
+            if (pilot_send_vwire(VW_INDEX_04h, SET_VWIRE(OOB_RST_ACK, value)) == 0)
+            {
+                // clear the valid bit for this interrupt and make it as previous
+                Host2BMC_VW_INDEX[i].prev_value = Host2BMC_VW_INDEX[i].curr_value;
+                Host2BMC_VW_INDEX[i].curr_value &= 0xBF;
+                if(handshake_start ==0){
+                    handshake_time = get_timer(0);
+                    handshake_start =1 ;
+                }
+            }
+        }
+
+        if ((Host2BMC_VW_INDEX[i].index == VW_INDEX_07h) && ((Host2BMC_VW_INDEX[i].curr_value & 0x10) == 0x10))
+        {
+            value = Host2BMC_VW_INDEX[i].curr_value & 0x1;
+            if (pilot_send_vwire(VW_INDEX_06h, SET_VWIRE(HOST_RST_ACK, value)) == 0)
+            {
+                // clear the valid bit for this interrupt and make it as previous
+                Host2BMC_VW_INDEX[i].prev_value = Host2BMC_VW_INDEX[i].curr_value;
+                Host2BMC_VW_INDEX[i].curr_value &= 0xEF;
+                if(handshake_start ==0){
+                    handshake_time = get_timer(0);
+                    handshake_start =1 ;
+                }
+            }
+        }
+    }
+}
+static void pilot_handle_espi_resets(int reset_type)
+{
+    int i = 0;
+
+    for (i = 0;i < H2B_ARRAY_SIZE; i++)
+    {
+        if ((reset_type & Host2BMC_VW_INDEX[i].espi_reset_type) == reset_type)
+        {
+            Host2BMC_VW_INDEX[i].prev_value = 0;
+            Host2BMC_VW_INDEX[i].curr_value = 0;
+        }
+    }
+
+    if (reset_type == ESPI_RESET)
+    {
+        first_vw_triggered = 0;
+    }
+}
+static void pilot_vw_system_event(void)
+{
+    int i = 0, j = 0;
+    volatile unsigned char vw_status = 0;
+    unsigned char temp = 0;
+
+    for (i = 0; i < H2B_ARRAY_SIZE; i++)
+    {
+        vw_status = pilot_espi_read8_reg(PILOT_ESPI_VW_INDEX0 + Host2BMC_VW_INDEX[i].index);
+
+        // Check if valid is set for any of the bits
+        if ((vw_status & VW_IDX_VALID) == VW_IDX_VALID)
+        {
+            first_vw_triggered = 1;
+            Host2BMC_VW_INDEX[i].prev_value = Host2BMC_VW_INDEX[i].curr_value;
+            for (j = 0; j < 4; j++)
+            {
+                if ((Host2BMC_VW_INDEX[i].prev_value & (1 << j)) != (vw_status & (1 << j)))
+                {
+                    temp |= (1 << j);
+                }
+
+                // Check if the current vwire was triggered due to PLTRST.
+                if ((Host2BMC_VW_INDEX[i].index == VW_INDEX_03h) && (j == 1))
+                {
+                    // When PLTRST is asserted some of the vwires are cleared
+                    if ((vw_status & 0x2) == 0x0)
+                    {
+                        printf("UBOOT ESPI PLTRST# 0\n");
+                        pltRst = 1;
+                        pilot_handle_espi_resets(ESPI_PLTRST);
+                    }
+                }
+            }
+            Host2BMC_VW_INDEX[i].curr_value &= ~temp;
+            Host2BMC_VW_INDEX[i].curr_value |= (((temp & 0xF) << 4) | (vw_status & 0xF));
+            //printk("vwire_valid Index(%x) hwsts:0x%x swsts:0x%x\n", Host2BMC_VW_INDEX[i].index, vw_status, Host2BMC_VW_INDEX[i].curr_value);
+        }
+        temp = 0;
+    }
+
+    pilot_send_acks_if_needed(); 
+}
+
+
+
+
+
+void pilot_espi_hw_init(void) 
+{
+    unsigned char intr_s;
+    int i;
+    
+   // espi hardware initialization
+    pilot_send_vwire(VW_INDEX_05h, (unsigned char)(SET_VWIRE(SLAVE_BOOT_LOAD_DONE, 1) | SET_VWIRE(SLAVE_BOOT_LOAD_STATUS, 1)) ) ;
+
+    // If BMC is coming out of soft reset
+
+    if (*(volatile unsigned char*)(SE_SCRATCH_128_VA_BASE + 0x7C) == 1)
+    {
+        printf("ESPIVW: Soft reset\n");
+        // When the system is coming up for the first time the driver only report the VW values
+        // and will not report about the valid bits since it does not know which VWIRE changed.
+        // This value will be used to compare when subsequent interrupts happen and report
+        // changes in a particular vwire.
+        // If a SUSACK has to be sent it will be done by just looking at the valid and values.
+        // i.e., see if bit 8, bit 0 & bit 1 are set in a VW Index_41(best assumption).
+        // Not taking spin lock since IRQ is not enabled at this point anyways.
+        for (i = 0;i < H2B_ARRAY_SIZE; i++)
+        {
+            Host2BMC_VW_INDEX[i].prev_value = 0;
+            Host2BMC_VW_INDEX[i].curr_value = *(volatile unsigned char*)(PILOT_ESPI_VW_INDEX0 + Host2BMC_VW_INDEX[i].index);
+
+            // If valid is set then clear the valid bits and just populate the current values
+            if ((Host2BMC_VW_INDEX[i].curr_value & 0xF0) != 0)
+            {
+                if (Host2BMC_VW_INDEX[i].index == VW_INDEX_41h)
+                {
+                    // See if Index_41 valid bit is set and BIT0 is set. If so set valids for
+                    // SUS_WARN# (best assumption) to aid SUSACK generation.
+                    // And clear other valid bits
+                    if ((Host2BMC_VW_INDEX[i].curr_value & ESPI_VW_SUS_WARN) == ESPI_VW_SUS_WARN)
+                        Host2BMC_VW_INDEX[i].curr_value |= (ESPI_VW_SUS_WARN << 4);
+
+                    Host2BMC_VW_INDEX[i].curr_value &= 0x1F;
+                }
+                else if (Host2BMC_VW_INDEX[i].index == VW_INDEX_03h)
+                {
+                    // See if Index_03 valid bit is set and BIT2 is set. If so set valids for
+                    // OOB_RST_ACK(best assumption) to aid OOB_RST_ACK generation.
+                    // And clear other valid bits
+                    if ((Host2BMC_VW_INDEX[i].curr_value & ESPI_VW_OOB_RST_WARN) == ESPI_VW_OOB_RST_WARN)
+                        Host2BMC_VW_INDEX[i].curr_value |= (ESPI_VW_OOB_RST_WARN << 4);
+
+                    Host2BMC_VW_INDEX[i].curr_value &= 0x4F;
+                }
+                else if (Host2BMC_VW_INDEX[i].index == VW_INDEX_07h)
+                {
+                    // See if Index_07 valid bit is set and BIT2 is set. If so set valids for
+                    // OOB_RST_ACK(best assumption) to aid OOB_RST_ACK generation.
+                    // And clear other valid bits
+                    if ((Host2BMC_VW_INDEX[i].curr_value & ESPI_VW_HST_RST_WARN) == ESPI_VW_HST_RST_WARN)
+                        Host2BMC_VW_INDEX[i].curr_value |= (ESPI_VW_HST_RST_WARN<< 4);
+
+                    Host2BMC_VW_INDEX[i].curr_value &= 0x1F;
+                }
+                else
+                    Host2BMC_VW_INDEX[i].curr_value = 0x0F;
+            }
+
+            // If the value is 1 for any of the vwire then it means that the host has triggered
+            // atleast once vwire
+            if (Host2BMC_VW_INDEX[i].index == 0x02 && ((Host2BMC_VW_INDEX[i].curr_value & 0x4) == 0x4))
+            {
+
+                first_vw_triggered = 1;
+            }
+        }
+
+        pilot_send_acks_if_needed();
+    }
+    else{
+        pilot_vw_system_event();
+    }
+
+
+    /* Enable eSPI Reset Interrupt */
+    intr_s = *(unsigned char *)(SE_PILOT_SPEC_VA_BASE + 0x23);
+    intr_s = intr_s | ESPI_RST_INTR_EN;
+    intr_s = intr_s & 0xf7; //do not touch lock bit
+    *(unsigned char *)(SE_PILOT_SPEC_VA_BASE + 0x23) = intr_s;
+
+    /* Enable In-band reset . Do RMW so as to not overwrite other bits in this register */
+    *(unsigned char *)(SE_PILOT_SPEC_VA_BASE + 0x25) |= (ESPI_INBAND_EN);
+
+    // Enable interrupts
+    pilot_espi_write32_reg(pilot_espi_read32_reg(PILOT_ESPI_VW_CTRL) | VW_INTR_EN, PILOT_ESPI_VW_CTRL);
+}
+
+void  pilot_espi_handler_vwire()
+{
+
+	unsigned long start_time;
+	start_time = get_timer(0);//us
+	pltRst = 0;
+	handshake_time = 0 ;
+	
+	pilot_espi_hw_init();
+	
+	while( ((handshake_start==0)&(get_timer(start_time)<ESPI_HANDSHAKE_TIMEOUT)||(handshake_start==1)&(get_timer(handshake_time)<ESPI_WAIT_TIMEOUT))&& (pltRst==0) )
+	{
+		// If this is the first time vwire is triggered then generate ESPI_VW_SLAVE_BOOT_LOAD_DONE and
+		// ESPI_VW_SLAVE_BOOT_LOAD_STATUS. This is to handle the case where the channel was not
+		// enabled at the time of driver init or after espi reset
+		if (!first_vw_triggered)
+		{
+			// Set SLAVE_BOOT_LOAD_DONE
+			pilot_send_vwire(VW_INDEX_05h, (unsigned char)(SET_VWIRE(SLAVE_BOOT_LOAD_DONE, 1) | SET_VWIRE(SLAVE_BOOT_LOAD_STATUS, 1)));
+		}
+		pilot_vw_system_event();
+
+		
+	}
+	printf("ESPI U-boot handshake complete\r\n");
+}
