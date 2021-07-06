diff -Naur linux_org/drivers/net/ethernet/pilot_tulip/tulip_core.c linux/drivers/net/ethernet/pilot_tulip/tulip_core.c
--- linux_org/drivers/net/ethernet/pilot_tulip/tulip_core.c	2020-02-05 17:25:45.747548824 +0800
+++ linux/drivers/net/ethernet/pilot_tulip/tulip_core.c	2020-02-06 15:28:35.311002159 +0800
@@ -43,7 +43,12 @@
 #endif
 
 #include "pilot_phy.h"
+#define TXPBL           0x0
+#define RXPBL           0x0
+#define I2C_SMB_CTRL_2  0x40100A08
 
+/* By default enable DDMA for both Tx and Rx */
+static unsigned int ddma = 3;	//ddma=1->tx, 2->rx, 3->both
 static char version[] =
 	"Linux PilotIII Tulip driver version " DRV_VERSION " (" DRV_RELDATE ")\n";
 
@@ -127,6 +132,7 @@
 module_param(csr0, int, 0);
 module_param_array(options, int, NULL, 0);
 module_param_array(full_duplex, int, NULL, 0);
+module_param(ddma, int, 0); 
 
 #define PFX DRV_NAME ": "
 
@@ -552,6 +558,10 @@
 	unsigned int MACL;
 	unsigned int MACH;
 #endif
+
+	unsigned int ddma_en=0;
+	u32 rxpbl=0;
+	u32 txpbl=0;
 	volatile int timeout = 1000000;
 
 #ifdef CONFIG_TULIP_NAPI
@@ -628,6 +638,28 @@
 #endif
 
 /* Pilot-III Addition End */
+
+	if(ddma)
+	{
+		//	printk("CSR5=%x\n",(ioread32(ioaddr+CSR5))); //To check if TX/RX process is running
+		txpbl = ioread32(ioaddr+CSR0);
+		txpbl = (txpbl & 0xFFFFC0FF) | (TXPBL << 8);
+		iowrite32(txpbl, ioaddr + CSR0); //Tx pbl 
+
+		rxpbl = ioread32((volatile u32 *)IO_ADDRESS(I2C_SMB_CTRL_2));
+		if((u32)tp->base_addr == IOADDR_INTERFACE_ETH_A)
+			rxpbl = (rxpbl & 0xFFC0FFFF) | (RXPBL << 16);
+		if((u32)tp->base_addr == IOADDR_INTERFACE_ETH_B)
+			rxpbl = (rxpbl & 0xC0FFFFFF) | (RXPBL << 24);
+		iowrite32(rxpbl, (volatile u32 *)IO_ADDRESS(I2C_SMB_CTRL_2)); //Rx pbl
+
+		ddma_en = ddma << 30;                       //ddma=1->tx, 2->rx, 3->both
+		iowrite32( 0xC0000000, (ioaddr + CSR21) ); //Enabling DDMA
+	}
+
+	//Enable Statistical counters Accumulation 
+	iowrite32( (ioread32(ioaddr+CSR22) | (1<<31)), (ioaddr + CSR22) ); 
+
 	if (tulip_debug > 1)
 		printk(KERN_DEBUG "%s: tulip_up(), irq==%d.\n", dev->name, dev->irq);
 
diff -Naur linux_org/drivers/net/ethernet/pilot_tulip/tulip.h linux/drivers/net/ethernet/pilot_tulip/tulip.h
--- linux_org/drivers/net/ethernet/pilot_tulip/tulip.h	2020-02-05 17:25:58.190863226 +0800
+++ linux/drivers/net/ethernet/pilot_tulip/tulip.h	2020-02-06 15:28:39.290783976 +0800
@@ -139,8 +139,9 @@
 	CSR17 = 0x88,
 	CSR18 = 0x90,
 	CSR19 = 0x98,
-	CSR20 = 0xA0
-
+	CSR20 = 0xA0,
+	CSR21 = 0xA8,
+	CSR22 = 0xB0
 };
 
 /* register offset and bits for CFDD PCI config reg */
