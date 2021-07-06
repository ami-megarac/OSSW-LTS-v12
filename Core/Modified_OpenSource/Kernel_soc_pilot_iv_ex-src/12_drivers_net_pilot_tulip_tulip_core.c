--- linux_org/drivers/net/ethernet/pilot_tulip/tulip_core.c	2020-03-20 16:14:43.395430517 +0800
+++ linux/drivers/net/ethernet/pilot_tulip/tulip_core.c	2020-03-20 16:15:01.098461426 +0800
@@ -47,6 +47,8 @@
 #define RXPBL           0x0
 #define I2C_SMB_CTRL_2  0x40100A08
 
+#define SE_SYS_CLK_VA_BASE				IO_ADDRESS(SE_SYS_CLK_BASE)
+
 /* By default enable DDMA for both Tx and Rx */
 static unsigned int ddma = 3;	//ddma=1->tx, 2->rx, 3->both
 static char version[] =
@@ -406,6 +408,11 @@
 		case SPEED_1000:
 			if (verbose)
 	       			printk("%s : Auto Negotiated 1000-Base-TX %s\n",dev->name,(ecmd.duplex == DUPLEX_FULL)?"Full Duplex":"Half Duplex");
+			//Set PILOT4 MACRGMIICTL register to 0x200, Otherwise we will see
+			//Tx issues on MAC0 in 1000Mbps speed	
+			if(PHY_MICREL == (tp->phy_oui >> 6))
+				*((volatile unsigned long *)(SE_SYS_CLK_VA_BASE + 0x88)) = 0x200;
+			
 			tp->csr6 |= (0x1 << 17);
 			tp->csr6 |= (0x1 << 16);
 			tp->csr6 &= ~TxThreshold;
