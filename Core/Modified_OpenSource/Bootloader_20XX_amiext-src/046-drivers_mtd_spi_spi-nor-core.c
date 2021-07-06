--- uboot/drivers/mtd/spi/spi-nor-core.c	2021-01-06 13:33:18.404466975 +0800
+++ uboot_new/drivers/mtd/spi/spi-nor-core.c	2021-01-06 13:41:36.564466975 +0800
@@ -328,6 +328,7 @@
 		need_wren = true;
 	case SNOR_MFR_MACRONIX:
 	case SNOR_MFR_WINBOND:
+	case SNOR_MFR_GIGADEVICE:
 		if (need_wren)
 			write_enable(nor);
 
