--- uboot_old/arch/arm/cpu/pilotcommon/piloti2c.c	2018-07-05 02:43:09.788170075 -0700
+++ uboot/arch/arm/cpu/pilotcommon/piloti2c.c	2018-07-04 23:27:55.010995800 -0700
@@ -0,0 +1,773 @@
+#include <common.h>
+#include <i2c.h>
+#include "soc_hw.h"
+
+
+#define PILOT_I2C_REG_BASE		0x40433000 // I2C controller registers base address
+
+// I2C device registers offset
+#define I2C_CON_REG				(i2c_bus_base_addr + 0x00)
+#define I2C_TAR_REG				(i2c_bus_base_addr + 0x04)
+#define I2C_SAR_REG				(i2c_bus_base_addr + 0x08)
+#define I2C_HS_MADDR_REG		(i2c_bus_base_addr + 0x0C)
+#define I2C_DATA_CMD_REG		(i2c_bus_base_addr + 0x10)
+#define I2C_SS_SCL_HCNT_REG		(i2c_bus_base_addr + 0x14)
+#define I2C_SS_SCL_LCNT_REG		(i2c_bus_base_addr + 0x18)
+#define I2C_FS_SCL_HCNT_REG		(i2c_bus_base_addr + 0x1C)
+#define I2C_FS_SCL_LCNT_REG		(i2c_bus_base_addr + 0x20)
+#define I2C_HS_SCL_HCNT_REG		(i2c_bus_base_addr + 0x24)
+#define I2C_HS_SCL_LCNT_REG		(i2c_bus_base_addr + 0x28)
+#define I2C_INTR_STAT_REG		(i2c_bus_base_addr + 0x2C)
+#define I2C_INTR_MASK_REG		(i2c_bus_base_addr + 0x30)
+#define I2C_RAW_INTR_STAT_REG	(i2c_bus_base_addr + 0x34)
+#define I2C_RX_TL_REG			(i2c_bus_base_addr + 0x38)
+#define I2C_TX_TL_REG			(i2c_bus_base_addr + 0x3C)
+#define I2C_CLR_INTR_REG		(i2c_bus_base_addr + 0x40)
+#define I2C_CLR_RX_UNDER_REG	(i2c_bus_base_addr + 0x44)
+#define I2C_CLR_RX_OVER_REG		(i2c_bus_base_addr + 0x48)
+#define I2C_CLR_TX_OVER_REG		(i2c_bus_base_addr + 0x4C)
+#define I2C_CLR_RD_REQ_REG		(i2c_bus_base_addr + 0x50)
+#define I2C_CLR_TX_ABRT_REG		(i2c_bus_base_addr + 0x54)
+#define I2C_CLR_RX_DONE_REG		(i2c_bus_base_addr + 0x58)
+#define I2C_CLR_ACTIVITY_REG	(i2c_bus_base_addr + 0x5C)
+#define I2C_CLR_STOP_DET_REG	(i2c_bus_base_addr + 0x60)
+#define I2C_CLR_START_DET_REG	(i2c_bus_base_addr + 0x64)
+#define I2C_CLR_GEN_CALL_REG	(i2c_bus_base_addr + 0x68)
+#define I2C_ENABLE_REG			(i2c_bus_base_addr + 0x6C)
+#define I2C_STATUS_REG			(i2c_bus_base_addr + 0x70)
+#define I2C_TXFLR_REG			(i2c_bus_base_addr + 0x74)
+#define I2C_RXFLR_REG			(i2c_bus_base_addr + 0x78)
+#define I2C_SRESET_REG			(i2c_bus_base_addr + 0x7C)
+#define I2C_TX_ABORT_SOURCE_REG	(i2c_bus_base_addr + 0x80)
+#define I2C_COMP_PARAMS_REG		(i2c_bus_base_addr + 0xf4)
+#define I2C_COMP_VERSION_REG	(i2c_bus_base_addr + 0xf8)
+
+// IC_CON		
+#define MASTER_MODE				(1<<0)	//1=MASTER ENABLED ,0=MASTER DISABLED
+#define IC_RESTART_EN			(1<<5)
+#define STOP_DET_FILTER			(1<<7)
+#define RXFIFO_CLK_STRETCH		(1<<9)
+
+// IC_DATA_CMD
+#define CMDREAD					(1<<8)  //1=READ,0=WRITE
+#define CMDWRITE				(0<<8)
+#define CMDSTOP					(1<<9)
+
+// IC_INTR_STAT, IC_INTR_MASK, IC_INTR_STAT
+#define RX_UNDER				(1<<0)
+#define RX_OVER					(1<<1)
+#define RX_FULL					(1<<2)
+#define TX_OVER					(1<<3)
+#define TX_EMPTY				(1<<4)
+#define RD_REQ					(1<<5)
+#define TX_ABRT					(1<<6)
+#define RX_DONE					(1<<7)
+#define ACTIVITY				(1<<8)
+#define STOP_DET				(1<<9)
+#define START_DET				(1<<10)
+#define GEN_CALL				(1<<11)
+
+// I2C Time Outs
+#define SMB_MTO					(1)
+#define SMB_STO					(1<<1)
+#define SMB_GTO					(1<<2)
+#define SMB_TFIFO_TO			(1<<3)
+#define I2C_TO_BITS				((SMB_TFIFO_TO)|(SMB_MTO)|(SMB_STO)|(SMB_GTO))
+
+#define EN_STOP_DET				(1)
+#define EN_BUS_HANG_FIX			(1<<1)
+#define EN_CLK_STREACHING		(1<<2)
+#define EN_TXFIFO_TO			(1<<3)
+#define EN_P3_FEATURES			((EN_TXFIFO_TO)|(EN_CLK_STREACHING)|(EN_BUS_HANG_FIX)|(EN_STOP_DET))
+
+#define I2CPCT0					0x24
+#define I2CPCT1					0x28
+#define I2CPCT2					0x910
+#define I2C_TO_INTR_EN			0x938
+#define I2C_TO_INTR_EN_1		0x96C
+#define I2C_COUNTER_STS_1		0x974
+#define I2C_TO_EN				0x93C
+#define I2C_TO_EN_1				0x970
+#define I2C_TO_INTR_STS			0x940
+#define I2C0_P3_FEATURE_EN		0x948
+#define I2C1_P3_FEATURE_EN		0x949
+#define I2C2_P3_FEATURE_EN		0x94A
+#define I2C3_P3_FEATURE_EN		0x94B
+#define I2C4_P3_FEATURE_EN		0x94C
+#define I2C5_P3_FEATURE_EN		0x94D
+#define I2C6_P3_FEATURE_EN		0x94E
+#define I2C7_P3_FEATURE_EN		0x94F
+#define I2C8_P3_FEATURE_EN		0x97C
+#define I2C9_P3_FEATURE_EN		0x97E
+#define I2CPCT3					0x960
+
+#define DEFAULT_RX_THRESHOLD	17 //RX interrupt after 1 bytes in RX Fifo
+#define DEFAULT_TX_THRESHOLD	24 //TX interrupt when TX fifo is empty
+
+#define SCL_RISE_TIME_NS		100 // Nanoseconds.
+#define SCL_FALL_TIME_NS		10  // Nanoseconds.
+#define FILTER_CLOCKS			1   // No. of filter clock cycles.
+
+/*----------------------------------------------------------------------------------------------*/
+/*------------------------------------  Common stuff for All SOC -------------------------------*/ 
+/*----------------------------------------------------------------------------------------------*/
+
+static void real_i2c_init (int speed, int slaveadd);
+static int i2c_init_pending= 1;
+static int i2c_bus_base_addr   = 0;
+static int i2c_bus_num = 0;
+
+#ifdef CONFIG_SYS_I2C_SLAVE
+static int i2c_bus_slaveaddr   = CONFIG_SYS_I2C_SLAVE;
+#else
+static int i2c_bus_slaveaddr   = 0x20; //Default BMC Address
+#endif
+#ifdef CONFIG_SYS_I2C_SPEED
+static int i2c_bus_speed  = CONFIG_SYS_I2C_SPEED;
+#else
+static int i2c_bus_speed  = 100000; //Default 100Khz
+#endif
+
+
+unsigned int i2c_get_bus_speed(void)
+{
+	return i2c_bus_speed;	
+}
+
+int 
+i2c_set_bus_speed(unsigned int speed)
+{
+	//i2c_bus_speed = speed; // Use Default Speed
+	speed = speed;
+	
+	real_i2c_init(i2c_bus_speed,i2c_bus_slaveaddr);
+	return 0;
+}
+
+unsigned int i2c_get_bus_num(void)
+{
+	return i2c_bus_num;
+}
+
+int i2c_set_bus_num(unsigned int bus)
+{
+	i2c_bus_num = bus;
+	
+	real_i2c_init(i2c_bus_speed,i2c_bus_slaveaddr);
+	return 0;
+}
+
+/*----------------------------------------------------------------------------------------------*/
+/* ---------------------  i2c interface functions (SOC Specific) --------------------------------*/ 
+/*----------------------------------------------------------------------------------------------*/
+
+static int i2c_scan(unsigned char chip);
+static void i2c_start(unsigned char chip);
+static unsigned long i2c_interrupt_status(void);
+static int i2c_send_byte(unsigned char byte, int last);
+static int i2c_receive_byte(unsigned char *byte, int last);
+
+
+void i2c_init (int speed, int slaveadd)
+{
+	// This is called before relocation and hence we cannot access global variables
+	return;
+}
+
+static void reset_i2c_pin_control_reg0 (unsigned long bitmask )
+{
+	volatile unsigned long i2c_pin_ctrl0;
+	unsigned int i=0;
+	
+	i2c_pin_ctrl0 = *(unsigned long *)(SE_SYS_CLK_VA_BASE +I2CPCT0);
+	
+	// I2C Module Reset, TAR Update without disabling the I2C module
+	i2c_pin_ctrl0 |= bitmask;
+	
+	* (unsigned long *)(SE_SYS_CLK_VA_BASE +I2CPCT0 )= i2c_pin_ctrl0 ;
+	for(i=0;i<0x20;i++);
+	
+	// Write back 0 in Module Reset bit to complete the reset. At the same time set I2C pins controlled by I2C state Machine
+	i2c_pin_ctrl0 &= ~bitmask;
+	* (unsigned long *)(SE_SYS_CLK_VA_BASE +I2CPCT0 )= i2c_pin_ctrl0;
+}
+
+static void reset_i2c_pin_control_reg1 (unsigned long bitmask )
+{
+	volatile unsigned long i2c_pin_ctrl1;
+	unsigned int i=0;
+	
+	i2c_pin_ctrl1 = *(unsigned long *)(SE_SYS_CLK_VA_BASE + I2CPCT1);
+	
+	// I2C Module Reset, TAR Update without disabling the I2C module
+	i2c_pin_ctrl1 |= bitmask;
+	
+	* (unsigned long *)(SE_SYS_CLK_VA_BASE +I2CPCT1 )= i2c_pin_ctrl1;
+	for(i=0;i<0x20;i++);
+	
+	// Write back 0 in Module Reset bit to complete the reset. At the same time set I2C pins controlled by I2C state Machine
+	i2c_pin_ctrl1 &= ~bitmask;
+	* (unsigned long *)(SE_SYS_CLK_VA_BASE + I2CPCT1 )= i2c_pin_ctrl1;
+}
+
+static void reset_i2c_pin_control_reg2 (unsigned long bitmask )
+{
+	volatile unsigned long i2c_pin_ctrl2;
+	unsigned int i=0;
+	
+	i2c_pin_ctrl2 = *(unsigned long *)(SE_SYS_CLK_VA_BASE + I2CPCT2);
+	
+	// I2C Module Reset, TAR Update without disabling the I2C module
+	i2c_pin_ctrl2 |= bitmask;
+	
+	* (unsigned long *)(SE_SYS_CLK_VA_BASE +I2CPCT2 )= i2c_pin_ctrl2;
+	for(i=0;i<0x20;i++);
+	
+	// Write back 0 in Module Reset bit to complete the reset. At the same time set I2C pins controlled by I2C state Machine
+	i2c_pin_ctrl2 &= ~bitmask;
+	* (unsigned long *)(SE_SYS_CLK_VA_BASE + I2CPCT2 )= i2c_pin_ctrl2;
+}
+
+static void reset_i2c_pin_control_reg3 (unsigned long bitmask )
+{
+	volatile unsigned long i2c_pin_ctrl3;
+	unsigned int i=0;
+	
+	i2c_pin_ctrl3 = *(unsigned long *)(SE_SYS_CLK_VA_BASE + I2CPCT3);
+	
+	// I2C Module Reset, TAR Update without disabling the I2C module
+	i2c_pin_ctrl3 |= bitmask;
+	
+	* (unsigned long *)(SE_SYS_CLK_VA_BASE +I2CPCT3 )= i2c_pin_ctrl3;
+	for(i=0;i<0x20;i++);
+	
+	// Write back 0 in Module Reset bit to complete the reset. At the same time set I2C pins controlled by I2C state Machine
+	i2c_pin_ctrl3 &= ~bitmask;
+	* (unsigned long *)(SE_SYS_CLK_VA_BASE + I2CPCT3 )= i2c_pin_ctrl3;
+}
+
+void i2c_pilot_ii_reset(int bus)
+{
+	unsigned long i2c0_bitmask= (1<<7);
+	unsigned long i2c1_bitmask= (1<<15);
+	unsigned long i2c2_bitmask= (1<<23);
+	unsigned long i2c3_bitmask= (1<<31);
+	unsigned long i2c4_bitmask= (1<<7);
+	unsigned long i2c5_bitmask= (1<<15);
+	unsigned long i2c6_bitmask= (1<<7);
+	unsigned long i2c7_bitmask= (1<<15);
+	unsigned long i2c8_bitmask= (1<<7);
+	unsigned long i2c9_bitmask= (1<<15);
+	
+	// Hit the reset bit
+	if ( bus == 0)
+		reset_i2c_pin_control_reg0(i2c0_bitmask); 
+	if ( bus == 1)
+		reset_i2c_pin_control_reg0(i2c1_bitmask);
+	if ( bus == 2)
+		reset_i2c_pin_control_reg0(i2c2_bitmask);
+	if ( bus == 3)
+		reset_i2c_pin_control_reg0(i2c3_bitmask);
+	if ( bus == 4)
+		reset_i2c_pin_control_reg1(i2c4_bitmask);
+	if ( bus == 5)
+		reset_i2c_pin_control_reg1(i2c5_bitmask);
+	if ( bus == 6)
+		reset_i2c_pin_control_reg2(i2c6_bitmask);
+	if ( bus == 7)
+		reset_i2c_pin_control_reg2(i2c7_bitmask);
+	if ( bus == 8)
+		reset_i2c_pin_control_reg3(i2c8_bitmask);
+	if ( bus == 9)
+		reset_i2c_pin_control_reg3(i2c9_bitmask);
+}
+
+void i2c_disable_time_out_interrupt(int bus)
+{
+	// Clear Intr Sts and Disbale Time out interrupts
+	if ((bus >= 0) && (bus <= 7))
+	{
+		*(volatile unsigned int *)(SE_SYS_CLK_VA_BASE + I2C_TO_INTR_STS) = (I2C_TO_BITS << (bus*4)) ;
+		*(volatile unsigned int *)(SE_SYS_CLK_VA_BASE + I2C_TO_INTR_EN) &= (~(I2C_TO_BITS << (bus*4)));
+	}
+	else if ((bus == 8) || (bus == 9))
+	{
+		*(volatile unsigned int *)(SE_SYS_CLK_VA_BASE + I2C_COUNTER_STS_1) = (I2C_TO_BITS << ((bus-8)*4)) ;
+		*(volatile unsigned int *)(SE_SYS_CLK_VA_BASE + I2C_TO_INTR_EN_1) &= (~(I2C_TO_BITS << ((bus-8)*4)));
+	}
+}
+
+void i2c_disable_to_counters(int bus)
+{
+	// Disable Time out counters
+	if ((bus >= 0) && (bus <= 7))
+		*(volatile unsigned int *)(SE_SYS_CLK_VA_BASE + I2C_TO_EN) &= (~(I2C_TO_BITS <<( bus*4)));
+	else if ((bus == 8) || (bus == 9))
+		*(volatile unsigned int *)(SE_SYS_CLK_VA_BASE + I2C_TO_EN_1) &= (~(I2C_TO_BITS << ((bus-8)*4)));
+}	
+
+void i2c_pilot_ii_enable_interrupt(unsigned long mask)
+{
+	unsigned long current_mask;
+	
+	current_mask = *((volatile unsigned long *) I2C_INTR_MASK_REG);
+	*((volatile unsigned long *) I2C_INTR_MASK_REG) = (current_mask|mask);
+	
+	return;
+}
+
+void i2c_pilot_ii_disable_interrupt(unsigned long mask)
+{
+	unsigned long current_mask;
+	
+	current_mask = *((volatile unsigned long *) I2C_INTR_MASK_REG);
+	*((volatile unsigned long *) I2C_INTR_MASK_REG) = (current_mask & ~mask);
+	
+	return;
+}
+
+void i2c_pilot_ii_disable_all_interrupts(void)
+{
+	*((volatile unsigned long *) I2C_INTR_MASK_REG) = 0;
+}
+
+void i2c_enable_bus(void)
+{
+	unsigned short temp=0;
+	// Enable I2C interface
+	temp = *((volatile unsigned long *) I2C_ENABLE_REG);
+	*((volatile unsigned long *) I2C_ENABLE_REG) = (temp|1);
+}
+
+void i2c_disable_bus(void)
+{
+	unsigned short temp=0;
+	// Disable I2C interface
+	temp = *((volatile unsigned long *) I2C_ENABLE_REG);
+	temp &= 0xfffe;
+	*((volatile unsigned long *) I2C_ENABLE_REG) = temp;
+}
+
+static void update_i2c_i2cpct_reg0 (int bus)
+{
+  volatile unsigned long i2c_pin_ctrl0;
+  unsigned long OR_bitmask = (1 << (4 + (bus*8)));
+  i2c_pin_ctrl0 = *(unsigned long *)(SE_SYS_CLK_VA_BASE +I2CPCT0);
+  
+  // Set the TAR bit
+  i2c_pin_ctrl0 |= OR_bitmask;
+  
+  // Clear bit-bang
+  i2c_pin_ctrl0 &= ~(1 << (bus*8));
+  * (unsigned long *)(SE_SYS_CLK_VA_BASE +I2CPCT0)= i2c_pin_ctrl0;
+}
+
+static void update_i2c_i2cpct_reg1 (int bus)
+{
+	volatile unsigned long i2c_pin_ctrl1;
+	unsigned long OR_bitmask = (1 << (4 + (bus*8)));
+	
+	i2c_pin_ctrl1 = *(unsigned long *)(SE_SYS_CLK_VA_BASE +I2CPCT1);
+	
+	// Set the TAR bit
+	i2c_pin_ctrl1 |= OR_bitmask;
+	
+	// Clear bit-bang
+	i2c_pin_ctrl1 &= ~(1 << (bus*8));
+	* (unsigned long *)(SE_SYS_CLK_VA_BASE +I2CPCT1 )= i2c_pin_ctrl1;
+}
+
+static void update_i2c_i2cpct_reg2(int bus)
+{
+	volatile unsigned long i2c_pin_ctrl2;
+	unsigned long OR_bitmask = (1 << (4 + (bus*8)));
+	
+	i2c_pin_ctrl2 = *(unsigned long *)(SE_SYS_CLK_VA_BASE +I2CPCT2);
+	
+	// Set the TAR bit
+	i2c_pin_ctrl2 |= OR_bitmask;
+	
+	// Clear bit-bang
+	i2c_pin_ctrl2 &= ~(1 << (bus*8));
+	* (unsigned long *)(SE_SYS_CLK_VA_BASE +I2CPCT2 )= i2c_pin_ctrl2;
+}
+
+static void update_i2c_i2cpct_reg3(int bus)
+{
+	volatile unsigned long i2c_pin_ctrl3;
+	unsigned long OR_bitmask = (1 << (4 + (bus*8)));
+	
+	i2c_pin_ctrl3 = *(unsigned long *)(SE_SYS_CLK_VA_BASE +I2CPCT3);
+	
+	// Set the TAR bit
+	i2c_pin_ctrl3 |= OR_bitmask;
+	
+	// Clear bit-bang
+	i2c_pin_ctrl3 &= ~(1 << (bus*8));
+	* (unsigned long *)(SE_SYS_CLK_VA_BASE +I2CPCT3 )= i2c_pin_ctrl3;
+}
+
+static void i2c_pilot_ii_init_i2cpct(int bus)
+{
+	if(bus < 4)
+		update_i2c_i2cpct_reg0(bus);
+	else if( (bus>=4) && (bus<=5) )
+		update_i2c_i2cpct_reg1(bus-4);
+	else if( (bus>=6) && (bus<=7) )
+		update_i2c_i2cpct_reg2(bus-6);
+	else if( (bus>=8) && (bus<=9) )
+		update_i2c_i2cpct_reg3(bus-8);
+}
+
+int calcHCNT( int speed_kHz, int SCL_HighTime_ns, int highSpeed_i2c )
+{
+    int hcnt;
+    int baseCLK_time_ns = highSpeed_i2c ? 10 : 20;
+	
+    hcnt = (SCL_HighTime_ns - SCL_FALL_TIME_NS)/baseCLK_time_ns - FILTER_CLOCKS - 7;
+    if     ( speed_kHz ==  100 ) hcnt = highSpeed_i2c ? 0x1C6 : 0xE3;
+    else if( speed_kHz ==  400 ) hcnt = highSpeed_i2c ?  0x4C : 0x26;
+    else if( speed_kHz == 3400 ) hcnt = 0x06;
+
+    return hcnt;
+}
+
+int calcLCNT( int speed_kHz, int SCL_LowTime_ns, int highSpeed_i2c )
+{
+    int lcnt;
+    int baseCLK_time_ns = highSpeed_i2c ? 10 : 20;
+	
+	lcnt = (SCL_LowTime_ns - SCL_RISE_TIME_NS + SCL_FALL_TIME_NS)/baseCLK_time_ns - 1;
+	if     ( speed_kHz ==  100 ) lcnt = highSpeed_i2c ? 0x20C : 0x106;
+	else if( speed_kHz ==  400 ) lcnt = highSpeed_i2c ?  0x92 :  0x49;
+	else if( speed_kHz == 3400 ) lcnt = 0x10;
+	
+	return lcnt;
+}
+int isHighSpeedI2C( int bus )
+{
+    return ( bus==5 || bus==6 || bus==7 || bus==8 ) ? 1 : 0;
+}
+
+int set_speed( int bus, int speed_kHz )
+{
+    int speed_mode, high_speed_i2c, SCL_Period_ns;
+    int hcnt_val, lcnt_val;
+    unsigned int lcnt_regs[] = { 0x00,                I2C_SS_SCL_LCNT_REG,
+                                 I2C_FS_SCL_LCNT_REG, I2C_HS_SCL_LCNT_REG };
+    unsigned int hcnt_regs[] = { 0x00,                I2C_SS_SCL_HCNT_REG,
+                                 I2C_FS_SCL_HCNT_REG, I2C_HS_SCL_HCNT_REG };
+    //
+    // Calculate the speed mode.
+    //
+    high_speed_i2c = isHighSpeedI2C( bus );
+    if( speed_kHz <= 0 )  {             // Error
+        return 1;
+    }
+    else if( speed_kHz <= 100 )  {      // SS = standard speed.
+        speed_mode = 1;
+    }
+    else if( speed_kHz <= 400 )  {      // FS = fast speed.
+        speed_mode = 2;
+    }
+    else if( high_speed_i2c ) {         // HS = high speed.
+        speed_mode = 3;
+    } else {                            // Error
+        return 1;
+    }
+
+    //
+    // Calculate the timing values.
+    // SCL_Period / [ns] = (1/(speed_kHz*1000))/1e-9 = 1e6/speed_kHz;
+    //
+    SCL_Period_ns  = (1000000 + speed_kHz/2) / speed_kHz;   // Rround up.
+    
+    hcnt_val = calcHCNT( speed_kHz, SCL_Period_ns/2, high_speed_i2c );
+    lcnt_val = calcLCNT( speed_kHz, SCL_Period_ns/2, high_speed_i2c );
+    
+    //
+    // Write calculated values to appropriate hardware registers
+    // - speed_mode value range is [1|2|3].
+    //
+    i2c_disable_bus();
+    {
+        unsigned short x = (*((volatile unsigned long *) I2C_CON_REG) & 0xfff9);
+        *((volatile unsigned long *) I2C_CON_REG) = (x | (speed_mode<<1));
+
+        *((volatile unsigned long *) lcnt_regs[speed_mode]) = lcnt_val;
+        *((volatile unsigned long *) hcnt_regs[speed_mode]) = hcnt_val;
+    }
+    i2c_enable_bus();
+     
+    return 0;
+}
+
+void i2c_init_bus(int bus, int speed)
+{
+	u32 controlbits;
+	
+	//clear to interrupt status
+	i2c_disable_time_out_interrupt(bus);
+	
+	//disable time out counters
+	i2c_disable_to_counters( bus);
+
+	switch(bus)
+	{
+		case 0:
+			*(volatile char *)(SE_SYS_CLK_VA_BASE + I2C0_P3_FEATURE_EN) = EN_P3_FEATURES;
+			break;
+		case 1:
+			*(volatile char *)(SE_SYS_CLK_VA_BASE + I2C1_P3_FEATURE_EN) = EN_P3_FEATURES;
+			break;
+		case 2:
+			*(volatile char *)(SE_SYS_CLK_VA_BASE + I2C2_P3_FEATURE_EN) = EN_P3_FEATURES;
+			break;
+		case 3:
+			*(volatile char *)(SE_SYS_CLK_VA_BASE + I2C3_P3_FEATURE_EN) = EN_P3_FEATURES;
+			break;
+		case 4:
+			*(volatile char *)(SE_SYS_CLK_VA_BASE + I2C4_P3_FEATURE_EN) = EN_P3_FEATURES;
+			break;
+		case 5:
+			*(volatile char *)(SE_SYS_CLK_VA_BASE + I2C5_P3_FEATURE_EN) = EN_P3_FEATURES;
+			break;
+		case 6:
+			*(volatile char *)(SE_SYS_CLK_VA_BASE + I2C6_P3_FEATURE_EN) = EN_P3_FEATURES;
+			break;
+		case 7:
+			*(volatile char *)(SE_SYS_CLK_VA_BASE + I2C7_P3_FEATURE_EN) = EN_P3_FEATURES;
+			break;
+		case 8:
+			*(volatile char *)(SE_SYS_CLK_VA_BASE + I2C8_P3_FEATURE_EN) = EN_P3_FEATURES;
+			break;
+		case 9:
+			*(volatile char *)(SE_SYS_CLK_VA_BASE + I2C9_P3_FEATURE_EN) = EN_P3_FEATURES;
+			break;
+		default:
+			break;
+	}
+	
+	// Disable all interrupts on the bus
+	i2c_pilot_ii_disable_all_interrupts();
+	
+	// Init I2CPCT registers for TAR bit and I2C mode
+	i2c_pilot_ii_init_i2cpct(bus);
+	
+	//Disable the I2C interface first to write control register
+	i2c_disable_bus();
+	
+	// Set the slave address again
+	*((volatile unsigned long *) I2C_SAR_REG) = i2c_bus_slaveaddr;
+	
+	// Set the TX fifo threshold level for interrupt generation
+	*((volatile unsigned long *) I2C_TX_TL_REG) = DEFAULT_TX_THRESHOLD;
+	
+	// Set the RX fifo threshold level for interrupt generation
+	*((volatile unsigned long *) I2C_RX_TL_REG) = DEFAULT_RX_THRESHOLD;
+	
+	// Enable restart when acting as a master
+	controlbits = 0x00 | IC_RESTART_EN;
+	
+	// Configure as master
+	controlbits |= MASTER_MODE;
+	
+	// Clock stretching is enabled though the IC_CON register on Pilot 4
+	controlbits |= RXFIFO_CLK_STRETCH | STOP_DET_FILTER;
+	
+	// Set Speed, Addressing, Master
+	*((volatile unsigned long *) I2C_CON_REG) = controlbits;
+	
+	// Set the i2c bus freq to 100kHz
+	set_speed(bus, (speed/1000));
+	
+	i2c_pilot_ii_enable_interrupt(STOP_DET|TX_ABRT|RX_OVER|RX_FULL|RD_REQ);
+	
+	// Enable I2C interface
+	i2c_enable_bus();
+
+}
+
+static void real_i2c_init (int speed, int slaveadd)
+{
+	// Calculate bus address based on bus number
+	if (i2c_bus_num < 8)
+		i2c_bus_base_addr = (PILOT_I2C_REG_BASE + (0x1000 * (i2c_bus_num)));
+	else
+		i2c_bus_base_addr = (PILOT_I2C_REG_BASE + (0x1000 * (i2c_bus_num)) + 0x1000 );
+	
+	// Do sw reset
+	i2c_pilot_ii_reset(i2c_bus_num); 
+	
+	// Init bus
+	i2c_init_bus(i2c_bus_num, speed); 
+	
+	i2c_init_pending = 0;
+}
+
+int i2c_probe(uchar chip)
+{
+	int ret = 0;
+    
+	real_i2c_init(i2c_bus_speed,i2c_bus_slaveaddr);
+    
+	ret = i2c_scan(chip);
+	
+	return ret;
+}
+
+int i2c_read(uchar chip, uint addr, int alen, uchar *buffer, int len)
+{
+	int i;
+	int last;
+	int ret;
+	
+	ret = 0;
+	
+	if (i2c_init_pending)
+		real_i2c_init(i2c_bus_speed,i2c_bus_slaveaddr);
+	
+	i2c_start(chip);
+	
+	for (i = 0; i < alen; i ++) 
+	{
+		i2c_send_byte(((addr >> ((alen -1 - i) * 8)) & 0xFF), 0);
+	}
+	
+	for (i = 0; i < len; i ++) 
+	{
+		last = (i < (len - 1)) ? 0 : 1;
+		
+		if (i2c_receive_byte(&(buffer[i]), last))
+		{
+			return 1;
+		}
+	}
+	
+	return ret;
+}
+
+int i2c_write(uchar chip, uint addr, int alen, uchar *buffer, int len)
+{
+	int i;
+	int last;
+	int ret;
+	
+	ret = 0;
+
+	if (i2c_init_pending)
+		real_i2c_init(i2c_bus_speed,i2c_bus_slaveaddr);
+
+	i2c_start(chip);
+	
+	for (i = 0; i < alen; i ++) 
+	{
+		i2c_send_byte(((addr >> ((alen -1 - i) * 8)) & 0xFF), 0);
+	}
+	
+	if (i2c_interrupt_status() & 0x40)
+		return 1;
+	
+	for (i = 0; i < len; i ++) 
+	{
+		last = (i < (len - 1)) ? 0 : 1;
+		
+		i2c_send_byte(buffer[i], last);
+	}
+	
+	return ret;
+}
+
+/*----------------------------------------------------------------------------------------------*/
+/* ---------------------  i2c internal fuctions for SOC         --------------------------------*/ 
+/*----------------------------------------------------------------------------------------------*/
+static int i2c_send_byte(unsigned char byte, int last)
+{
+	if (last == 1)
+		*((volatile unsigned long *) I2C_DATA_CMD_REG) = ((byte & 0x000000FF) | CMDSTOP);
+	else
+		*((volatile unsigned long *) I2C_DATA_CMD_REG) = byte & 0x000000FF;
+	
+    return 0;
+}
+
+static int i2c_receive_byte(unsigned char *byte, int last)
+{
+	if (last == 0)
+		(*((volatile unsigned long *) I2C_DATA_CMD_REG)) = CMDREAD;
+	else if (last == 1)
+		(*((volatile unsigned long *) I2C_DATA_CMD_REG)) = (CMDREAD | CMDSTOP);
+	
+	if (i2c_interrupt_status() & 0x40)
+		return 1;
+	
+	*byte = (*((volatile unsigned long *) I2C_DATA_CMD_REG));
+	
+	return 0;
+}
+
+static int i2c_scan(unsigned char chip)
+{
+	int ret = 0;
+	unsigned long interrupt_status;
+	// load address to buffer and specify read/write
+	i2c_disable_bus();
+	*((volatile unsigned long *) I2C_TAR_REG) = chip; 
+	i2c_enable_bus();
+	
+	i2c_pilot_ii_enable_interrupt(TX_EMPTY);
+	*((volatile unsigned long *) I2C_DATA_CMD_REG) = (CMDWRITE | CMDSTOP);
+	i2c_pilot_ii_disable_interrupt(TX_EMPTY);
+	
+	interrupt_status = i2c_interrupt_status();
+	if ((interrupt_status & 0x200) && !(interrupt_status & 0x40))
+		ret = 0;
+	else
+		ret = -1;
+	
+	return ret;
+}
+
+static void i2c_start(unsigned char chip)
+{
+	// load address to buffer and specify read/write
+	i2c_disable_bus();
+	*((volatile unsigned long *) I2C_TAR_REG) = chip; 
+	i2c_enable_bus();
+}
+
+static unsigned long i2c_interrupt_status(void)
+{
+	unsigned long clr_isr = 0;
+	unsigned long intr_status = 0;
+	long i;
+
+	for (i = 0; i < 100000; i++) { // poll device interrupt status
+		intr_status = *((volatile unsigned long *) I2C_INTR_STAT_REG);
+
+		if (intr_status != 0) { // interrupt occurs 
+			clr_isr = clr_isr;
+			break;
+		}
+	}
+	
+	if (intr_status & STOP_DET)
+	{
+		clr_isr = *((volatile unsigned long *) I2C_CLR_STOP_DET_REG);
+	}
+	if (intr_status & RX_FULL)
+	{
+		clr_isr = *((volatile unsigned long *) I2C_CLR_RX_OVER_REG);
+	}
+	if (!(intr_status & RX_FULL))
+	{
+		clr_isr = *((volatile unsigned long *) I2C_RXFLR_REG);
+	}
+	if(intr_status & TX_ABRT)
+	{
+		clr_isr = *((volatile unsigned long *) I2C_CLR_TX_ABRT_REG);
+	}
+	
+	return intr_status;
+}
+
