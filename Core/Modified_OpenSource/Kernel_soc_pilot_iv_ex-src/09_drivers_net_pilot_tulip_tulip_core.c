--- linux_org/drivers/net/ethernet/pilot_tulip/tulip_core.c	2019-08-14 14:47:24.701632302 +0800
+++ linux/drivers/net/ethernet/pilot_tulip/tulip_core.c	2019-08-14 14:58:29.650463206 +0800
@@ -285,6 +285,33 @@
 static void poll_tulip(struct net_device *dev);
 #endif
 
+static void tulip_reset(int interface)
+{
+	struct net_device *dev = pilot3_devices[interface];
+	tulip_close(dev);
+	tulip_open(dev);
+	return; 
+}
+
+asmlinkage long sys_reset_mac_reg(void)
+{
+
+#ifdef CONFIG_PILOT_SWAP_MAC
+	    	tulip_reset(INTERFACE_ETH_B);
+#if (CONFIG_PILOT_NET_COUNT == 2)
+	   	tulip_reset(INTERFACE_ETH_A);
+#endif
+
+#else	// CONFIG_PILOT_SWAP_MAC
+
+	    	tulip_reset(INTERFACE_ETH_A);
+#if (CONFIG_PILOT_NET_COUNT == 2)
+		tulip_reset(INTERFACE_ETH_B);
+#endif
+#endif // CONFIG_PILOT_SWAP_MAC
+	return 0;
+}
+
 static void tulip_set_power_state (struct tulip_private *tp,
 				   int sleep, int snooze)
 {
