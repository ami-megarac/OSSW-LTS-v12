--- old/common/main.c	2019-02-21 17:13:38.919593612 +0800
+++ new/common/main.c	2019-02-21 17:22:49.787578452 +0800
@@ -530,6 +530,9 @@
 #endif
 
 	bootstage_mark_name(BOOTSTAGE_ID_MAIN_LOOP, "main_loop");
+#ifdef CONFIG_EARLYBOOT_ESPI_HANDSHAKE
+	pilot_espi_handler_vwire();	
+#endif
 
 #ifdef CONFIG_MODEM_SUPPORT
 	debug("DEBUG: main_loop:   do_mdm_init=%d\n", do_mdm_init);
