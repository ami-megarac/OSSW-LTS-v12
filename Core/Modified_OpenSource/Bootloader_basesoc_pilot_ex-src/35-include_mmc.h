--- uboot/include/mmc.h	2013-07-23 04:58:13.000000000 -0700
+++ uboot.new/include/mmc.h	2017-06-20 23:50:46.259506460 -0700
@@ -46,6 +46,7 @@
 #define MMC_VERSION_4_3		(MMC_VERSION_MMC | 0x403)
 #define MMC_VERSION_4_41	(MMC_VERSION_MMC | 0x429)
 #define MMC_VERSION_4_5		(MMC_VERSION_MMC | 0x405)
+#define MMC_VERSION_5_0		(MMC_VERSION_MMC | 0x500)
 
 #define MMC_MODE_HS		0x001
 #define MMC_MODE_HS_52MHz	0x010