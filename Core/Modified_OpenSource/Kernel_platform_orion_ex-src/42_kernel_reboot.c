--- linux/kernel/reboot.c	2018-11-25 21:47:24.690867000 +0800
+++ linux/kernel/reboot.c	2018-11-25 21:55:02.039426999 +0800
@@ -122,6 +122,16 @@
 	set_cpus_allowed_ptr(current, cpumask_of(cpu));
 }
 
+#include <linux/delay.h>
+extern int kill_something_info_for_reboot(int sig, struct siginfo *info, pid_t pid);
+static void kill_processes(void)
+{
+	struct siginfo info;
+	kill_something_info_for_reboot(SIGTERM, &info, -1);
+	mdelay(1);
+	kill_something_info_for_reboot(SIGKILL, &info, -1);
+}
+
 /**
  *	kernel_restart - reboot the system
  *	@cmd: pointer to buffer containing command to execute for restart
@@ -132,6 +142,7 @@
  */
 void kernel_restart(char *cmd)
 {
+	kill_processes();// Kill all process to avoid reboot hang code.(For EIP439645)
 	kernel_restart_prepare(cmd);
 	migrate_to_reboot_cpu();
 	syscore_shutdown();
