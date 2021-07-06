diff -Naur uboot.old/arch/arm/cpu/astcommon/ast_wdt.c uboot/arch/arm/cpu/astcommon/ast_wdt.c
--- uboot.old/arch/arm/cpu/astcommon/ast_wdt.c	2020-03-05 17:24:39.955370040 +0000
+++ uboot/arch/arm/cpu/astcommon/ast_wdt.c	2020-03-05 17:25:07.755360557 +0000
@@ -19,7 +19,7 @@
 #include <common.h>
 
 #define WDT_ENABLE              0x01
-#define WDT_TIMEOUT             0x11E1A300              // 5 Minutes (300 secs * 1MHz)
+#define WDT_TIMEOUT             0x23C34600              // 10 Minutes (600 secs * 1MHz)
 #define WDT_TRIGGER             0x00004755              // Trigger Passcode
 #define WDT_CLOCK               0x10                    // 1MHz Clock for WDT
 #define WDT_RESET_SYSTEM        0x02                    // Enable System-Reset-On-Timeout
