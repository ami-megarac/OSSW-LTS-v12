--- /home/hnguyen/workspace/work/mds/13.0/Jade_RTP/.backup/bootloader/uboot/board/ast2500evb/ast2500evb.c	Thu Dec 10 04:29:17 2020
+++ uboot/board/ast2500evb/ast2500evb.c	Thu Dec 10 04:30:18 2020
@@ -171,6 +171,14 @@
 	/* Drive GPIO pins MUX to route the UART to CPU by default */
 	*(volatile u32 *)(0x1e780024) |= 0x0F000000; /* output mode */
 	*(volatile u32 *)(0x1e780020) &= ~(0x0F000000); /* data */
+	/* GPIOAC2: Drive GPIO pins to route the SPI path to CPU by default */
+	*(volatile u32 *)(AST_SCU_BASE + 0xAC) &= ~(0x04); /* GPIO function */
+	*(volatile u32 *)(0x1e7801EC) |= 0x04; /* output mode */
+	*(volatile u32 *)(0x1e7801E8) |= 0x04; /* data high */
+	/* GPIOAC4: configure BMC is not ready for communication */
+	*(volatile u32 *)(AST_SCU_BASE + 0xAC) &= ~(0x10); /* GPIO function */
+	*(volatile u32 *)(0x1e7801EC) |= 0x10; /* output mode */
+	*(volatile u32 *)(0x1e7801E8) &= ~(0x10); /* data low */
 
 	*((volatile ulong*) SCU_KEY_CONTROL_REG) = 0; /* lock SCU */
 #ifdef CONFIG_AST2500
