--- linux_org/arch/arm/kernel/calls.S	2019-08-14 14:47:14.118128413 +0800
+++ linux/arch/arm/kernel/calls.S	2019-08-14 14:29:14.696810710 +0800
@@ -391,6 +391,7 @@
 		CALL(sys_finit_module)
 /* 380 */	CALL(sys_sched_setattr)
 		CALL(sys_sched_getattr)
+		CALL(sys_reset_mac_reg)
 #ifndef syscalls_counted
 .equ syscalls_padding, ((NR_syscalls + 3) & ~3) - NR_syscalls
 #define syscalls_counted
