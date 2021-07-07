--- linux_org/include/linux/syscalls.h	2019-08-14 14:47:04.294588899 +0800
+++ linux/include/linux/syscalls.h	2019-08-14 14:29:07.401153501 +0800
@@ -855,4 +855,5 @@
 asmlinkage long sys_kcmp(pid_t pid1, pid_t pid2, int type,
 			 unsigned long idx1, unsigned long idx2);
 asmlinkage long sys_finit_module(int fd, const char __user *uargs, int flags);
+asmlinkage long sys_reset_mac_reg(void);
 #endif
