--- uboot/board/ast2500evb/ast2500evb.c	2019-07-02 17:52:51.627076661 +0800
+++ uboot.new/board/ast2500evb/ast2500evb.c	2019-07-02 17:52:21.063076661 +0800
@@ -111,17 +111,17 @@
 
 int misc_init_r (void)
 {
-	
+    u32 temp=0;
 #ifdef CONFIG_HDMI_SUPPORT			/* AST 3100 & AST3200? */
 	/* Enable HDMI Recevier if required */
 	extern int hdmi_init(void); 
 	hdmi_init();
 #endif	
-	
-	*((volatile ulong*) 0x1e6e2000) = 0x1688A8A8;	/* unlock SCU */
-
-#ifdef CONFIG_AST3100EVB	
 
+	*((volatile ulong*) 0x1e6e2000) = 0x1688A8A8;	/* unlock SCU */
+	
+#ifdef CONFIG_AST3100EVB
+	
 	/* Set OSC output to 12 MHz for Codec */
 	*(volatile u32 *)(0x1e6e2010) |= 0x00000030;
 	mdelay(1);									/* Wait for SCU14 becomes 0 */
@@ -141,9 +141,9 @@
 	/* Drive GPIO pins for LED, Power On, USB HUB Reset */
      *(volatile u32 *)(0x1e780004) |= 0x0000000e;	
      *(volatile u32 *)(0x1e780000) |= 0x0000000f;
-
-#endif
 	
+#endif
+
 	/* Disable below for security concern */
 	/* iLPC2AHB Pt1, and SCU70[20] = 0 */
 	*(volatile u32 *)(AST_LPC_BASE + 0x80) &= ~(0x100);
@@ -162,8 +162,19 @@
 	*(volatile u32 *)(AST_LPC_BASE + 0x80) &= ~(0x400);
 	/* PCI BMC Device */
 	*(volatile u32 *)(AST_SCU_BASE + 0x180) &= ~(0x100);
-
+	
+    /* Base on AMD Diesel Platform hardware design.
+     * The SOL data streaming is existed between CPU (P0) UART1 and 
+     * AST2500 SOC UART1 */
+	*((volatile ulong*) 0x1e6e2084) |= 0x00ff0000;	/* enable UART1 ttyS0 */
 	*((volatile ulong*) SCU_KEY_CONTROL_REG) = 0; /* lock SCU */
+	*((volatile ulong*) 0x1e6ed07c) = 0x000000A8;	/* unlock PCIE */
+	*((volatile ulong*) 0x1e6ed034) |= 0x00001000;	/* PCIE register data */
+	*((volatile ulong*) 0x1e6ed07c) = 0;	/* lock PCIE */
+
+	temp = *((volatile ulong*) 0x1e789098);
+	temp &= (~0x10);
+	*((volatile ulong*) 0x1e789098) = temp;
 
 	*((volatile ulong*) AST_PCIE_REG_BASE + AST_PCIE_KEY_CONTROL) = 0xA8;	/* unlock PCIE */
 	*((volatile ulong*) AST_PCIE_REG_BASE + AST_PCIE_L1_ENTRY_TIMER) = 0xc81f0a;  /* L1 entry timer */
