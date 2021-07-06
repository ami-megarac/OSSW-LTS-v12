--- linux_org/drivers/net/ethernet/pilot_tulip/tulip_core.c	2019-09-26 15:34:18.587279345 +0530
+++ linux/drivers/net/ethernet/pilot_tulip/tulip_core.c	2019-09-26 15:37:52.499730172 +0530
@@ -288,8 +288,11 @@
 static void tulip_reset(int interface)
 {
 	struct net_device *dev = pilot3_devices[interface];
-	tulip_close(dev);
-	tulip_open(dev);
+	if( netif_running(dev) )
+	{
+		tulip_close(dev);
+		tulip_open(dev);
+	}
 	return; 
 }
 
