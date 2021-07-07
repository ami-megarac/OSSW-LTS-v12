--- uboot/arch/arm/mach-aspeed/board.c	2020-05-19 14:42:05.074624209 +0800
+++ uboot.new/arch/arm/mach-aspeed/board.c	2020-05-19 14:45:52.570860945 +0800
@@ -11,6 +11,20 @@
 #include <linux/err.h>
 #include <dm/uclass.h>
 
+
+
+
+#define AST_LPC_BASE                   0x1E789000    /* Not in AST3100   */
+#define AST_SCU_BASE                   0x1E6E2000
+#define SCU_KEY_CONTROL_REG			    (AST_SCU_VA_BASE +  0x00)
+#define AST_SCU_VA_BASE                 AST_SCU_BASE
+#define AST_PWM_FAN_BASE                0x1E786000
+
+
+
+#define AST_PCIE_REG_BASE	0x1e6ed000
+#define AST_PCIE_KEY_CONTROL	0x7C
+#define AST_PCIE_L1_ENTRY_TIMER	0x68
 /*
  * Second Watchdog Timer by default is configured
  * to trigger secondary boot source.
@@ -23,8 +37,74 @@
  */
 #define AST_FLASH_ADDR_DETECT_WDT	2
 
+
+//#define BIT(x)		(1 << (x))
+#define ESPI_CTRL                 0x00
+#define ESPI_CTRL_SW_RESET             GENMASK(31, 24)
+#define ESPI_CTRL_OOB_CHRDY            BIT(4)
+#define ESPI_ISR                  0x08
+#define ESPI_ISR_HW_RESET              BIT(31)
+#define ESPI_ISR_VW_SYS_EVT1           BIT(22)
+#define ESPI_ISR_VW_SYS_EVT            BIT(8)
+#define ESPI_IER                  0x0C
+#define ESPI_SYS_IER              0x94
+#define ESPI_SYS_EVENT            0x98
+#define ESPI_SYS_INT_T0           0x110
+#define ESPI_SYS_INT_T1           0x114
+#define ESPI_SYS_INT_T2           0x118
+#define ESPI_SYS_ISR              0x11C
+#define ESPI_SYSEVT_HOST_RST_ACK       BIT(27)
+#define ESPI_SYSEVT_SLAVE_BOOT_STATUS  BIT(23)
+#define ESPI_SYSEVT_SLAVE_BOOT_DONE    BIT(20)
+#define ESPI_SYSEVT_OOB_RST_ACK        BIT(16)
+#define ESPI_SYSEVT_HOST_RST_WARN      BIT(8)
+#define ESPI_SYSEVT_OOB_RST_WARN       BIT(6)
+#define ESPI_SYSEVT_PLT_RST_N          BIT(5)
+#define ESPI_SYS1_IER             0x100
+#define ESPI_SYS1_EVENT           0x104
+#define ESPI_SYS1_INT_T0          0x120
+#define ESPI_SYS1_INT_T1          0x124
+#define ESPI_SYS1_INT_T2          0x128
+#define ESPI_SYS1_ISR             0x12C
+#define ESPI_SYSEVT1_SUS_ACK           BIT(20)
+#define ESPI_SYSEVT1_SUS_WARN          BIT(0)
+#define AST_ESPI_REG_BASE               0x1E6EE000
+#define AST_LPC_REG_BASE                0x1E789000
+
+#define SCROSIO                         0x170
+
+/* Setup Interrupt Type/Enable of System Event from Master
+ *                                 T2 T1 T0
+ *  1). HOST_RST_WARN : Dual Edge   1  0  0
+ *  2). OOB_RST_WARN  : Dual Edge   1  0  0
+ *  3). PLTRST_N      : Dual Edge   1  0  0
+ */
+#define ESPI_SYS_INT_T0_SET        0x00000000
+#define ESPI_SYS_INT_T1_SET        0x00000000
+#define ESPI_SYS_INT_T2_SET \
+(ESPI_SYSEVT_HOST_RST_WARN | ESPI_SYSEVT_OOB_RST_WARN)
+#define ESPI_SYS_INT_SET \
+(ESPI_SYSEVT_HOST_RST_WARN | ESPI_SYSEVT_OOB_RST_WARN)
+
+
 DECLARE_GLOBAL_DATA_PTR;
 
+/* Setup Interrupt Type/Enable of System Event 1 from Master
+ *                                 T2 T1 T0
+ *  1). SUS_WARN    : Rising Edge   0  0  1
+ */
+#define ESPI_SYS1_INT_T0_SET        ESPI_SYSEVT1_SUS_WARN
+#define ESPI_SYS1_INT_T1_SET        0x00000000
+#define ESPI_SYS1_INT_T2_SET        0x00000000
+#define ESPI_SYS1_INT_SET           ESPI_SYSEVT1_SUS_WARN
+#define CONFIG_EARLYBOOT_ESPI_MONITOR_TIMEOUT 5
+#define CONFIG_EARLYBOOT_ESPI_HANDSHAKE_TIMEOUT 5
+#define ESPI_HANDSHAKE_TIMEOUT      CONFIG_EARLYBOOT_ESPI_MONITOR_TIMEOUT
+#define ESPI_WAIT_TIMEOUT           CONFIG_EARLYBOOT_ESPI_HANDSHAKE_TIMEOUT 
+bool oob_evt = 0, sys_evt = 0, sys_evt1 = 0, host_evt = 0;
+int count = 0;
+
+
 #if 0
 void lowlevel_init(void)
 {
@@ -52,6 +132,135 @@
 }
 #endif
 
+void aspeed_espi_slave_sys_event(void)
+{
+        u32 sts, evt;
+
+        sts = readl(AST_ESPI_REG_BASE + ESPI_SYS_ISR);
+        evt = readl(AST_ESPI_REG_BASE + ESPI_SYS_EVENT);
+
+		// ACK the Host_Rst_Warn
+        if (sts & ESPI_SYSEVT_HOST_RST_WARN)
+        {
+			if (evt & ESPI_SYSEVT_HOST_RST_WARN)
+			{
+				writel(ESPI_SYSEVT_HOST_RST_ACK, AST_ESPI_REG_BASE + ESPI_SYS_EVENT);
+				host_evt = 1;
+			}
+        }
+
+		// ACK the OOB_Rst_Warn
+        if (sts & ESPI_SYSEVT_OOB_RST_WARN)
+        {
+	    if (evt & ESPI_SYSEVT_OOB_RST_WARN)
+            {
+                writel(ESPI_SYSEVT_OOB_RST_ACK, AST_ESPI_REG_BASE + ESPI_SYS_EVENT);
+                oob_evt = 1;
+            }
+        }
+
+		// clear status, if any
+        writel(sts, AST_ESPI_REG_BASE + ESPI_SYS_ISR);
+}
+
+void aspeed_espi_slave_sys1_event(void)
+{
+        u32 sts, evt;
+ 
+        sts = readl(AST_ESPI_REG_BASE + ESPI_SYS1_ISR);
+
+        if (sts & ESPI_SYSEVT1_SUS_WARN)
+		{
+			evt = readl(AST_ESPI_REG_BASE + ESPI_SYS1_EVENT);
+
+			// ACK the Suspend Warn
+			if (evt & ESPI_SYSEVT1_SUS_ACK)
+			{
+				writel(ESPI_SYSEVT1_SUS_ACK, AST_ESPI_REG_BASE + ESPI_SYS1_EVENT);
+				host_evt = 1;
+			}
+		}
+
+		// clear the System Event 1 interrupt status, if any
+        writel(sts, AST_ESPI_REG_BASE + ESPI_SYS1_ISR);
+}
+
+void aspeed_espi_event_handling(void)
+{
+	u32 sts;
+
+	sts = readl(AST_ESPI_REG_BASE + ESPI_ISR);
+
+	if (sts & (ESPI_ISR_VW_SYS_EVT | ESPI_ISR_VW_SYS_EVT1))
+	{
+		if (sts & ESPI_ISR_VW_SYS_EVT)
+			aspeed_espi_slave_sys_event();
+
+		if (sts & ESPI_ISR_VW_SYS_EVT1)
+			aspeed_espi_slave_sys1_event();
+
+		// clear the interrupt status
+		writel(sts, AST_ESPI_REG_BASE + ESPI_IER);
+	}
+
+}
+
+void aspeed_espi_slave_boot_ack(void)
+{
+u32 evt;
+	evt = readl(AST_ESPI_REG_BASE + ESPI_SYS_EVENT);
+	if((evt & ESPI_SYSEVT_SLAVE_BOOT_STATUS) == 0)
+	{
+		evt = evt | ESPI_SYSEVT_SLAVE_BOOT_STATUS | ESPI_SYSEVT_SLAVE_BOOT_DONE;
+		writel(evt, AST_ESPI_REG_BASE + ESPI_SYS_EVENT);
+		host_evt = 1;
+	}
+	evt = readl(AST_ESPI_REG_BASE + ESPI_SYS1_EVENT);
+	if((evt & ESPI_SYSEVT1_SUS_WARN) != 0)
+	{
+		evt |= ESPI_SYSEVT1_SUS_ACK;
+		writel(evt, AST_ESPI_REG_BASE + ESPI_SYS1_EVENT);
+		host_evt = 1;
+	}
+}
+void eSPI_config_handshake(void)
+{
+	u32 reg;
+
+	// setting up the eSPI ID
+	reg = readl(AST_LPC_REG_BASE + SCROSIO);
+	reg &= ~0x000000ff;
+	writel(reg, AST_LPC_REG_BASE + SCROSIO);
+	reg = readl(AST_LPC_REG_BASE + SCROSIO);
+	reg |= 0xa8;
+	writel(reg, AST_LPC_REG_BASE + SCROSIO);
+	// set System Event interrupts for Host Reset Warn and OOB Reset Warn
+    writel(ESPI_SYS_INT_T0_SET, AST_ESPI_REG_BASE + ESPI_SYS_INT_T0);
+    writel(ESPI_SYS_INT_T1_SET, AST_ESPI_REG_BASE + ESPI_SYS_INT_T1);
+    writel(ESPI_SYS_INT_T2_SET, AST_ESPI_REG_BASE + ESPI_SYS_INT_T2);
+    writel(ESPI_SYS_INT_SET, AST_ESPI_REG_BASE + ESPI_SYS_IER);
+	// set System Event 1 interrupts for Suspend Warn
+	writel(ESPI_SYS1_INT_T0_SET, AST_ESPI_REG_BASE + ESPI_SYS1_INT_T0);
+    writel(ESPI_SYS1_INT_T1_SET, AST_ESPI_REG_BASE + ESPI_SYS1_INT_T1);
+    writel(ESPI_SYS1_INT_T2_SET, AST_ESPI_REG_BASE + ESPI_SYS1_INT_T2);
+   writel(ESPI_SYS1_INT_SET, AST_ESPI_REG_BASE + ESPI_SYS1_IER);
+	// enable VW System Event interrupt
+	writel(ESPI_ISR_VW_SYS_EVT, AST_ESPI_REG_BASE + ESPI_IER);
+	while ((count < ESPI_HANDSHAKE_TIMEOUT && oob_evt == 0 && host_evt == 0 )
+		|| (count < ESPI_WAIT_TIMEOUT && oob_evt == 0 && host_evt == 1) )
+	{
+		aspeed_espi_slave_boot_ack();
+	aspeed_espi_event_handling();
+		count++;
+		udelay(1000000);
+	}
+ printf("pass 13\n");
+
+}
+
+
+
+
 int board_init(void)
 {
 	struct udevice *dev;
@@ -74,8 +283,63 @@
 			break;
 	}
 
+    if ( ((*(volatile u32 *)(AST_SCU_BASE + 0x70)) & 0x2000000) > 0)
+    {
+//        printf("eSPI Handshake started\r\n");
+        eSPI_config_handshake(); 
+//        printf("eSPI Handshake complete\r\n");
+    }
 	return 0;
 }
+int misc_init_r (void)
+{
+//		printf("Calling misc_init_r\r\n");
+
+        *((volatile ulong*) 0x1e6e2000) = 0x1688A8A8;   /* unlock SCU */
+
+
+        /* Disable below for security concern */
+        /* iLPC2AHB Pt1, and SCU70[20] = 0 */
+        *(volatile u32 *)(AST_LPC_BASE + 0x80) &= ~(0x100);
+        *(volatile u32 *)(AST_LPC_BASE + 0x100) |= 0x40;
+        /* iLPC2AHB Pt2, and HICR5[8] and HICRB[6] are disable */
+        /* Use hardware strapping to decide SCU70[20], here software does nothing on it */
+        /* PCI VGA P2A (PCI2AHB) bridge */
+        *(volatile u32 *)(AST_SCU_BASE + 0x180) &= ~(0x2);
+        /* UART-based SoC Debug interface */
+        *(volatile u32 *)(AST_SCU_BASE + 0x2C) |= 0x400;
+        /* DMA from/to arbitrary BMC Address-space via X-DMA */
+        *(volatile u32 *)(AST_SCU_BASE + 0x4) |= 0x2000000;
+        *(volatile u32 *)(AST_SCU_BASE + 0x180) &= ~(0x4040);
+        /* LPC2AHB bridge, and HICR5[8] are disable */
+        *(volatile u32 *)(AST_SCU_BASE + 0x2C) |= 0x1;
+        *(volatile u32 *)(AST_LPC_BASE + 0x80) &= ~(0x400);
+        /* PCI BMC Device */
+        *(volatile u32 *)(AST_SCU_BASE + 0x180) &= ~(0x100);
+
+        *(volatile u32 *)(0x1e6e2080) &= 0xFF00FFFF; /* Disable UART3, configure GPIO */
+        *(volatile u32 *)(0x1e6e2084) |= 0xF0000000; /* Enable UART2 */
+        *(volatile u32 *)(0x1e6e2070) |= 0x02400000; /* Enable GPIOE Passthrough and eSPI mode */
+
+        *(volatile u32 *)(0x1e6e2004) &= ~(0x200); // Clear reset PWM controller
+
+       // *((volatile ulong*) SCU_KEY_CONTROL_REG) = 0; /* lock SCU */
+
+        /* Set all PWM to the highest duty cycle by default */
+        *(volatile u32 *)(AST_PWM_FAN_BASE + 0x0)  |= 0x00000F01;// Enable PWM A~D
+        *(volatile u32 *)(AST_PWM_FAN_BASE + 0x8)   = 0x0;
+        *(volatile u32 *)(AST_PWM_FAN_BASE + 0xC)   = 0x0;
+        *(volatile u32 *)(AST_PWM_FAN_BASE + 0x40) |= 0x00000F00;// Enable PWM E~H
+        *(volatile u32 *)(AST_PWM_FAN_BASE + 0x48)  = 0x0;
+        *(volatile u32 *)(AST_PWM_FAN_BASE + 0x4C)  = 0x0;
+
+	//*((volatile ulong*) AST_PCIE_REG_BASE + AST_PCIE_KEY_CONTROL) = 0xA8;   /* unlock PCIE */
+       //// *((volatile ulong*) AST_PCIE_REG_BASE + AST_PCIE_L1_ENTRY_TIMER) = 0xc81f0a;  /* L1 entry timer */
+       // *((volatile ulong*) AST_PCIE_REG_BASE + AST_PCIE_KEY_CONTROL) = 0x0;    /* lock PCIE */
+//printf("END of misc_init_r \r\n");
+        return (0);
+}
+
 
 int dram_init(void)
 {
