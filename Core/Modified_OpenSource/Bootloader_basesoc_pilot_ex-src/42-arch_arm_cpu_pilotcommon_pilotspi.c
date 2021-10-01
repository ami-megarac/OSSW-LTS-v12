--- uboot.old/arch/arm/cpu/pilotcommon/pilotspi.c	2021-06-02 08:28:11.521514016 -0400
+++ uboot/arch/arm/cpu/pilotcommon/pilotspi.c	2021-06-02 08:41:29.193114614 -0400
@@ -260,6 +260,7 @@
 #endif
 
 #ifdef __UBOOT__	
+#if 0
 typedef struct 
 {
 	ulong length;
@@ -280,6 +281,7 @@
 
 /*Align at 16 Byte Boundary */
 GDMA_DESCRIPTOR gdma_desc __attribute__ ((aligned (16)));
+#endif
 
 #define PILOT_3_SPI_MAX_CHIP_SELECT	4
 const unsigned int ChipSelectBit[PILOT_3_SPI_MAX_CHIP_SELECT] = {0x00000000, 0x40000000, 0x80000000, 0xC0000000};
@@ -289,10 +291,12 @@
 pilot3spi_split_burst_read(int bank, unsigned char addrmode, unsigned char *cmd,int cmdlen, SPI_DIR dir, 
 				unsigned char *data, unsigned long datalen, unsigned long flashsize)
 {
-	dma_addr_t dmabuff;
 	unsigned long src;
+#if 0
+	dma_addr_t dmabuff;
 	//unsigned long timeout =100000;
 	unsigned long timeout =800000;
+#endif
 
 	/* Fast Read Bit */
 	// TODO
@@ -313,6 +317,7 @@
 		src = cmd[0] << 24 | cmd[1] << 16 | cmd[2] << 8 | cmd[3];
 	}
 
+#if 0
 	/* Convert Buffer to DMA Buffer */
 	dmabuff = dma_map_single(NULL,data,datalen,DMA_FROM_DEVICE);
 	if (dmabuff == 0)
@@ -348,11 +353,19 @@
 		printk("ERROR: pilot3spi_burst_read timeout for read at 0x%08lx for %ld bytes\n", src, datalen);
 		return -1;
 	}
+#else
+	memcpy(data, (void *)src, datalen);
+#endif
 
 	return 0;
 }
 
+#if 0
 #define MAX_BURST_READ 0xFFF
+#else
+#define MAX_BURST_READ 0x4000000
+#endif
+
 
 static
 int  
