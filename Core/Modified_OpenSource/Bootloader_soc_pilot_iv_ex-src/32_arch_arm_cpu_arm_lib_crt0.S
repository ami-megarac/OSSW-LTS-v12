--- uboot/arch/arm/lib/crt0.S	2018-07-18 23:47:23.748652168 -0500
+++ uboot.new/arch/arm/lib/crt0.S	2018-07-18 23:31:30.715511556 -0500
@@ -107,5 +107,9 @@
 	add	lr, lr, r0
 	ldr	r0, [r8, #GD_RELOCADDR]		/* r0 = gd->relocaddr */
+
+	mov	r4, sp			/* Save SP for later restore */
+	mov	r6, r0			/* r6 <- relocation destination */
+
 	b	relocate_code
 here:
 
