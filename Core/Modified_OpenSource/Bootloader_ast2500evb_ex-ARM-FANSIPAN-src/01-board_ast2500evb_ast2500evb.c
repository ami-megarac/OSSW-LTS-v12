--- uboot_old/board/ast2500evb/ast2500evb.c	Tue Oct  6 22:40:22 2020
+++ uboot_new/board/ast2500evb/ast2500evb.c	Tue Oct  6 22:53:20 2020
@@ -168,6 +168,10 @@
 	*(volatile u32 *)(AST_LPC_BASE + 0x80) &= ~(0x400);
 	/* PCI BMC Device */
 	*(volatile u32 *)(AST_SCU_BASE + 0x180) &= ~(0x100);
+	/* Drive GPIO pins MUX to route the UART to CPU by default */
+	*(volatile u32 *)(0x1e780024) |= 0xFF000000; /* output mode */
+	*(volatile u32 *)(0x1e780020) &= ~(0x55000000); /* data */
+	*(volatile u32 *)(0x1e780020) |= 0x55000000; /* data */
 
 	*((volatile ulong*) SCU_KEY_CONTROL_REG) = 0; /* lock SCU */
 #ifdef CONFIG_AST2500
