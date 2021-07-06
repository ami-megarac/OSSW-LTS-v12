--- linux/kernel/signal.c	2018-11-28 12:53:46.623152000 +0800
+++ linux/kernel/signal.c	2018-11-28 13:00:13.588538000 +0800
@@ -1468,6 +1468,11 @@
 	return ret;
 }
 
+int kill_something_info_for_reboot(int sig, struct siginfo *info, pid_t pid)
+{
+	return kill_something_info(sig, info, pid);
+}
+
 /*
  * These are for backward compatibility with the rest of the kernel source.
  */
