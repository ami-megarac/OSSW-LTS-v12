--- uboot/arch/arm/mach-aspeed/board.c	2019-12-04 20:44:03.041467972 +0800
+++ uboot.new/arch/arm/mach-aspeed/board.c	2019-12-12 18:16:03.925521507 +0800
@@ -287,7 +289,9 @@
 }
 int misc_init_r (void)
 {
-//		printf("Calling misc_init_r\r\n");
+		u32 temp=0;
+    
+		printf("Calling misc_init_r\r\n");
 
         *((volatile ulong*) 0x1e6e2000) = 0x1688A8A8;   /* unlock SCU */
 
@@ -311,6 +315,7 @@
         /* PCI BMC Device */
         *(volatile u32 *)(AST_SCU_BASE + 0x180) &= ~(0x100);
 
+		#if 0
         *(volatile u32 *)(0x1e6e2080) &= 0xFF00FFFF; /* Disable UART3, configure GPIO */
         *(volatile u32 *)(0x1e6e2070) |= 0x02400000; /* Enable GPIOE Passthrough and eSPI mode */
 
@@ -325,12 +330,30 @@
         *(volatile u32 *)(AST_PWM_FAN_BASE + 0x40) |= 0x00000F00;// Enable PWM E~H
         *(volatile u32 *)(AST_PWM_FAN_BASE + 0x48)  = 0x0;
         *(volatile u32 *)(AST_PWM_FAN_BASE + 0x4C)  = 0x0;
+        #endif
+
+	*(volatile u32 *)(AST_SCU_BASE + 0x90) &= ~(0x3);
+
+    /* Base on AMD Diesel Platform hardware design.
+     * The SOL data streaming is existed between CPU (P0) UART1 and 
+     * AST2500 SOC UART1 */
+#if 0     
+	*((volatile ulong*) 0x1e6e2084) |= 0x00ff0000;	/* enable UART1 ttyS0 */
+	*((volatile ulong*) 0x1e6e2000) = 0;            /* lock SCU */
+#endif
+
+	*((volatile ulong*) 0x1e6ed07c) = 0x000000A8;	/* unlock PCIE */
+	*((volatile ulong*) 0x1e6ed034) |= 0x00001000;	/* PCIE register data */
+	*((volatile ulong*) 0x1e6ed07c) = 0;	        /* lock PCIE */
+	temp = *((volatile ulong*) 0x1e789098);
+	temp &= (~0x10);
+	*((volatile ulong*) 0x1e789098) = temp;
+
+	*((volatile ulong*) AST_PCIE_REG_BASE + AST_PCIE_KEY_CONTROL) = 0xA8;   /* unlock PCIE */
+    *((volatile ulong*) AST_PCIE_REG_BASE + AST_PCIE_L1_ENTRY_TIMER) = 0xc81f0a;  /* L1 entry timer */
+    *((volatile ulong*) AST_PCIE_REG_BASE + AST_PCIE_KEY_CONTROL) = 0x0;    /* lock PCIE */
 
-	//*((volatile ulong*) AST_PCIE_REG_BASE + AST_PCIE_KEY_CONTROL) = 0xA8;   /* unlock PCIE */
-       //// *((volatile ulong*) AST_PCIE_REG_BASE + AST_PCIE_L1_ENTRY_TIMER) = 0xc81f0a;  /* L1 entry timer */
-       // *((volatile ulong*) AST_PCIE_REG_BASE + AST_PCIE_KEY_CONTROL) = 0x0;    /* lock PCIE */
-//printf("END of misc_init_r \r\n");
-        return (0);
+    return (0);
 }
 
 
