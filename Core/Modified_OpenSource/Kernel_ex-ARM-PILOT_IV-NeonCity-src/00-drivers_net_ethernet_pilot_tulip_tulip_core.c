--- linux_org/drivers/net/ethernet/pilot_tulip/tulip_core.c	2019-08-15 17:03:22.354224132 -0400
+++ linux/drivers/net/ethernet/pilot_tulip/tulip_core.c	2019-08-15 17:19:44.748814555 -0400
@@ -358,12 +358,13 @@
 #else
     	phyid = tulip_mdio_read(dev, tp->mii.phy_id, MII_PHYSID2);
 #endif
-	if ((phyid == 0) || (phyid == 0xFFFF))	/* Possible NCSI */
-	{
-		tp->phys[tp->phy_index] = INVALID_PHYID;
-		tp->mii.phy_id = INVALID_PHYID;
-		return -2;
-	}
+	
+	//if ((phyid == 0) || (phyid == 0xFFFF))	/* Possible NCSI */
+	//{
+	//	tp->phys[tp->phy_index] = INVALID_PHYID;
+	//	tp->mii.phy_id = INVALID_PHYID;
+	//	return -2;
+	//}
 
   	if (tulip_debug > 4)
 		printk("%s : PHY ID  = 0x%x\n",dev->name,tp->phys[tp->phy_index]-1);
