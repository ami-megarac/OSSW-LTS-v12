--- linux.new/drivers/char/hw_random/pilot_rng.c	2018-10-19 11:24:27.409426031 +0800
+++ linux/drivers/char/hw_random/pilot_rng.c	2018-10-18 18:37:09.389134090 +0800
@@ -0,0 +1,76 @@
+/****************************************************************
+ **                                                            **   
+ **    (C)Copyright 2009-2015, American Megatrends Inc.        **
+ **                                                            **
+ **            All Rights Reserved.                            **
+ **                                                            **
+ **        5555 Oakbrook Pkwy Suite 200, Norcross              **
+ **                                                            **
+ **        Georgia - 30093, USA. Phone-(770)-246-8600.         **
+ **                                                            **
+ ****************************************************************/
+
+/****************************************************************
+ *
+ * pilot_rng.c
+ * PILOT HW random number generator driver
+ *
+*****************************************************************/
+
+#include <linux/module.h>
+#include <linux/random.h>
+#include <linux/platform_device.h>
+#include <linux/hw_random.h>
+#include <linux/io.h>
+
+#define PILOT_RNG_NAME    "pilot_rng"
+#define BYTES_READ        4
+
+#define RAND_BASE         0x40426000
+#define PILOT_SPEC_REG    (RAND_BASE + 0x500)
+#define RNG_REG           (PILOT_SPEC_REG + 0x8)
+#define RNG_REG_VA        IO_ADDRESS(RNG_REG)  //Virtual address of the IO Registers
+
+
+int pilot_rng_data_read(struct hwrng *rng, u32 *data)
+{
+    *data = ioread32((void __iomem*)RNG_REG_VA);
+    return BYTES_READ;
+}
+
+static struct hwrng pilot_rng_ops = {
+    .name		= PILOT_RNG_NAME,
+    .data_read	= pilot_rng_data_read,
+};
+
+
+static int pilot_rng_probe(void)
+{
+    return hwrng_register(&pilot_rng_ops);
+}
+
+static void pilot_rng_remove(void)
+{
+    hwrng_unregister(&pilot_rng_ops);
+}
+
+
+static int __init pilot_rng_init(void)
+{	
+    int i = 0,temp = 0;
+    // Random number generator register has to be read 32 times inorder
+    // to let it generate true random numbers.
+    for (i = 0;i < 32; i++)
+        temp = *(volatile unsigned int*)(RNG_REG_VA);
+
+    // Random number initialized
+    temp = temp; // to avoid warning
+    return pilot_rng_probe();
+}
+
+module_init(pilot_rng_init); 
+module_exit(pilot_rng_remove); 
+
+MODULE_LICENSE("GPL");
+MODULE_AUTHOR("American Megatrends Inc.");
+MODULE_DESCRIPTION("HW random number generator driver");
