--- old/include/common.h	2013-07-23 19:58:13.000000000 +0800
+++ new/include/common.h	2019-02-21 17:24:22.775575893 +0800
@@ -121,6 +121,9 @@
 #define _DEBUG	0
 #endif
 
+#ifdef CONFIG_EARLYBOOT_ESPI_HANDSHAKE
+extern void pilot_espi_handler_vwire(void);
+#endif
 /*
  * Output a debug text when condition "cond" is met. The "cond" should be
  * computed by a preprocessor in the best case, allowing for the best
