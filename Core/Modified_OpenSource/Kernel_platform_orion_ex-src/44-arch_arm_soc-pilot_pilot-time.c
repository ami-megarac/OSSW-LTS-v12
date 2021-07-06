--- linux/arch/arm/soc-pilot/pilot-time.c	2019-08-01 19:54:10.044373782 +0800
+++ new-linux/arch/arm/soc-pilot/pilot-time.c	2019-08-02 00:58:04.759208413 +0800
@@ -240,6 +240,7 @@
 	*((volatile unsigned long *)(SE_TIMER_VA_BASE+0x08 + 0x28)) = ctrl;
 	printk("Timer3 100ms periodic timer startedd\n");
 }
+
 static struct clock_event_device pilot_clockevent = {
 	.name		= "pilot_sys_timer1",
 	.features	= CLOCK_EVT_FEAT_PERIODIC | CLOCK_EVT_FEAT_ONESHOT,
@@ -248,27 +249,43 @@
 	.rating		= 300,
 };
 
-static cycle_t notrace pilot4_read_cycles(struct clocksource *cs)
+unsigned int  notrace read_raw_cycle(void)
 {
-        return (cycle_t) (0xffffffff - *((volatile unsigned long *)(SE_TIMER_VA_BASE + 0x14 +0x4)));
+        return (u32)*((volatile unsigned long *)(SE_TIMER_VA_BASE + 0x14 +0x4));
 }
+EXPORT_SYMBOL(read_raw_cycle);
 
-cycle_t notrace pilot4_read_raw_cycle(void)
+static u64 pilot_gt_counter_read(void)
 {
-        return (cycle_t) (0xffffffff - *((volatile unsigned long *)(SE_TIMER_VA_BASE + 0x14 +0x4)));
+        u64 counter;
+        u32 lower;
+        u32 upper, old_upper;
+ 
+        upper = readl_relaxed((volatile void *)(IO_ADDRESS(0x40460200) + 0x04));
+	 
+        do {
+                old_upper = upper;
+                lower = readl_relaxed((volatile void *)(IO_ADDRESS(0x40460200) + 0x00));
+                upper = readl_relaxed((volatile void *)(IO_ADDRESS(0x40460200) + 0x04));
+        } while (upper != old_upper);
+ 
+        counter = upper;
+        counter <<= 32;
+        counter |= lower;
+        return counter;
 }
 
-unsigned int  notrace read_raw_cycle(void)
+static cycle_t pilot_gt_clocksource_read(struct clocksource *cs)
 {
-        return (u32)*((volatile unsigned long *)(SE_TIMER_VA_BASE + 0x14 +0x4));
+        return pilot_gt_counter_read();
 }
-EXPORT_SYMBOL(read_raw_cycle);
-static struct clocksource pilot_clk_src = {
-        .name           = "pilot4_timer2",
-        .rating         = 200,
-        .read           = pilot4_read_cycles,
-        .mask           = CLOCKSOURCE_MASK(32),
-        .flags          = CLOCK_SOURCE_IS_CONTINUOUS,
+ 
+static struct clocksource pilot_gt_clocksource = {
+        .name   = "pilot_arm_global_timer",
+        .rating = 300,
+        .read   = pilot_gt_clocksource_read,
+        .mask   = CLOCKSOURCE_MASK(64),
+        .flags  = CLOCK_SOURCE_IS_CONTINUOUS,
 };
 
 
@@ -276,19 +293,15 @@
 static DEFINE_TWD_LOCAL_TIMER(twd_local_timer,
 			      0x40460600, 29);
 #endif
-static void __init clk_src_pilot_init(void)
-{
 
-       /* timer load already set up */
-       /* Disable timer and interrups */
-       *((volatile unsigned long *)(SE_TIMER_VA_BASE+0x08 +0x14)) = 0x0;
-       /* Load counter values */
-       *((volatile unsigned long *)(SE_TIMER_VA_BASE + 0x14)) = (0xffffffff);
-        printk("Registering clock source timercrnt addr %x\n", (SE_TIMER_VA_BASE + 0x14 +0x4));
-       *((volatile unsigned long *)(SE_TIMER_VA_BASE + 0x8 + 0x14)) = 5;//no interrupt ,free running , start
-        clocksource_register_hz(&pilot_clk_src, 25000000);
+static void __init clk_src_pilot_arm_global_init(void)
+{
+     /* Just enable the timer*/
+     *((volatile unsigned long *) IO_ADDRESS(0x40460208)) = 1;
+     clocksource_register_hz(&pilot_gt_clocksource, 250000000);
 }
 
+
 /*SDK Update - BMC CPU Reset Workaround*/
 void start_pilot_timer3(void)
  {
@@ -317,6 +330,7 @@
 	if (err)
 		pr_err("twd_local_timer_register failed %d\n", err);
 #endif
-	clk_src_pilot_init();
+	
+	clk_src_pilot_arm_global_init();
 	return;
 }
