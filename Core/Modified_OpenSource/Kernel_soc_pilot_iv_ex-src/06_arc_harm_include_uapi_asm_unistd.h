--- linux_org/arch/arm/include/uapi/asm/unistd.h	2019-08-14 14:46:51.343196007 +0800
+++ linux/arch/arm/include/uapi/asm/unistd.h	2019-08-14 14:28:57.237631046 +0800
@@ -408,6 +408,7 @@
 #define __NR_finit_module		(__NR_SYSCALL_BASE+379)
 #define __NR_sched_setattr		(__NR_SYSCALL_BASE+380)
 #define __NR_sched_getattr		(__NR_SYSCALL_BASE+381)
+#define __NR_reset_mac_reg		(__NR_SYSCALL_BASE+382)
 
 /*
  * This may need to be greater than __NR_last_syscall+1 in order to
