diff -Naur linux_orig/drivers/mtd/mtdchar.c linux/drivers/mtd/mtdchar.c
--- linux_orig/drivers/mtd/mtdchar.c	2020-01-29 19:05:39.498411319 +0530
+++ linux/drivers/mtd/mtdchar.c	2020-01-29 17:39:00.981730044 +0530
@@ -537,7 +537,7 @@
 	case BLKPG_ADD_PARTITION:
 
 		/* Only master mtd device must be used to add partitions */
-		if (mtd_is_partition(mtd))
+		if (!mtd_is_partition(mtd))
 			return -EINVAL;
 
 		return mtd_add_partition(mtd, p.devname, p.start, p.length);
