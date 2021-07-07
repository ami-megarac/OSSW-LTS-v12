--- linux/drivers/char/random.c	2019-08-01 19:54:08.087395782 +0800
+++ new-linux/drivers/char/random.c	2019-08-02 11:55:06.632478153 +0800
@@ -658,7 +658,7 @@
 		r->entropy_total = 0;
 		if (r == &nonblocking_pool) {
 			prandom_reseed_late();
-			pr_notice("random: %s pool is initialized\n", r->name);
+			//pr_notice("random: %s pool is initialized\n", r->name);
 		}
 	}
 
