--- uboot_old/drivers/spi/aspeed_spi.c	2021-03-31 16:33:43.593764471 +0800
+++ uboot/drivers/spi/aspeed_spi.c	2021-03-30 14:23:32.254241806 +0800
@@ -16,6 +16,7 @@
 #include <linux/ioport.h>
 
 #define ASPEED_SPI_MAX_CS		3
+#define FLASH_CALIBRATION_LEN   0x400
 
 struct aspeed_spi_regs {
 	u32 conf;			/* 0x00 CE Type Setting */
@@ -26,7 +27,9 @@
 	u32 _reserved0[5];		/* .. */
 	u32 segment_addr[ASPEED_SPI_MAX_CS];
 					/* 0x30 .. 0x38 Segment Address */
-	u32 _reserved1[17];		/* .. */
+	u32 _reserved1[5];		/* .. */
+	u32 soft_rst_cmd_ctrl;	/* 0x50 Auto Soft-Reset Command Control */
+	u32 _reserved2[11];		/* .. */
 	u32 dma_ctrl;			/* 0x80 DMA Control/Status */
 	u32 dma_flash_addr;		/* 0x84 DMA Flash Side Address */
 	u32 dma_dram_addr;		/* 0x88 DMA DRAM Side Address */
@@ -68,6 +71,7 @@
 
 /* CEx Control Register */
 #define CE_CTRL_IO_MODE_MASK		GENMASK(31, 28)
+#define CE_CTRL_IO_QPI_DATA			BIT(31)
 #define CE_CTRL_IO_DUAL_DATA		BIT(29)
 #define CE_CTRL_IO_DUAL_ADDR_DATA	(BIT(29) | BIT(28))
 #define CE_CTRL_IO_QUAD_DATA		BIT(30)
@@ -97,6 +101,9 @@
 #define	  CE_CTRL_WRITEMODE		0x2
 #define	  CE_CTRL_USERMODE		0x3
 
+/* Auto Soft-Reset Command Control */
+#define SOFT_RST_CMD_EN     GENMASK(1, 0)
+
 /*
  * The Segment Register uses a 8MB unit to encode the start address
  * and the end address of the AHB window of a SPI flash device.
@@ -117,23 +124,34 @@
 #define G6_SEGMENT_ADDR_START(reg)		(reg & 0xffff)
 #define G6_SEGMENT_ADDR_END(reg)		((reg >> 16) & 0xffff)
 #define G6_SEGMENT_ADDR_VALUE(start, end)					\
-	((((start) >> 16) & 0xffff) | (((end) - 0x100000) & 0xfff00000))
+	((((start) >> 16) & 0xffff) | (((end) - 0x100000) & 0xffff0000))
 
 /* DMA Control/Status Register */
 #define DMA_CTRL_DELAY_SHIFT		8
 #define DMA_CTRL_DELAY_MASK		0xf
+#define G6_DMA_CTRL_DELAY_MASK		0xff
 #define DMA_CTRL_FREQ_SHIFT		4
+#define G6_DMA_CTRL_FREQ_SHIFT		16
+
 #define DMA_CTRL_FREQ_MASK		0xf
 #define TIMING_MASK(div, delay)					   \
 	(((delay & DMA_CTRL_DELAY_MASK) << DMA_CTRL_DELAY_SHIFT) | \
 	 ((div & DMA_CTRL_FREQ_MASK) << DMA_CTRL_FREQ_SHIFT))
+#define G6_TIMING_MASK(div, delay)					   \
+	(((delay & G6_DMA_CTRL_DELAY_MASK) << DMA_CTRL_DELAY_SHIFT) | \
+	 ((div & DMA_CTRL_FREQ_MASK) << G6_DMA_CTRL_FREQ_SHIFT))
 #define DMA_CTRL_CALIB			BIT(3)
 #define DMA_CTRL_CKSUM			BIT(2)
 #define DMA_CTRL_WRITE			BIT(1)
 #define DMA_CTRL_ENABLE			BIT(0)
 
+/* for ast2600 setting */
+#define SPI_3B_AUTO_CLR_REG   0x1e6e2510
+#define SPI_3B_AUTO_CLR       BIT(9)
+
+
 /*
- *
+ * flash related info
  */
 struct aspeed_spi_flash {
 	u8		cs;
@@ -189,26 +207,25 @@
 		15, 7, 14, 6, 13, 5, 12, 4, 11, 3, 10, 2, 9, 1, 8, 0
 	};
 	u8 base_div = 0;
-//	int done = 0;
-	u32 i, j;
+	int done = 0;
+	u32 i, j = 0;
 	u32 hclk_div_setting = 0;
 
-	//for ast2600 spi freq = hclk / (([27:24] * 16) + [11:8])
-	for (j = 0; j < ARRAY_SIZE(hclk_masks); i++) {
+	for (j = 0; j < 0xf; i++) {
 		for (i = 0; i < ARRAY_SIZE(hclk_masks); i++) {
-			if (max_hz >= (hclk_rate / ((i + 1) + (j * 16)))) {
-				base_div = j * 16;
-//					done = 1;
+			base_div = j * 16;
+			if (max_hz >= (hclk_rate / ((i + 1) + base_div))) {
+				
+				done = 1;
 				break;
 			}
 		}
-//			if (done)
-//				break;
-		if( j == 0) break;	// todo check
+			if (done)
+				break;
 	}
 
-	debug("hclk=%d required=%d base_div is %d (mask %x, base_div %x) speed=%d\n",
-		  hclk_rate, max_hz, i + 1, hclk_masks[i], base_div, hclk_rate / ((i + 1) + base_div));
+	debug("hclk=%d required=%d h_div %d, divisor is %d (mask %x) speed=%d\n",
+		  hclk_rate, max_hz, j, i + 1, hclk_masks[i], hclk_rate / (i + 1 + base_div));
 
 	hclk_div_setting = ((j << 4) | hclk_masks[i]);
 
@@ -245,7 +262,7 @@
 				   u8 delay)
 {
 	u32 flash_addr = (u32)priv->ahb_base + 0x10000;
-	u32 flash_len = 0x200;
+	u32 flash_len = FLASH_CALIBRATION_LEN;
 	u32 dma_ctrl;
 	u32 checksum;
 
@@ -257,10 +274,13 @@
 	 * Control Register and the data input delay cycles in the
 	 * Read Timing Compensation Register are replaced by bit[11:4].
 	 */
-	dma_ctrl = DMA_CTRL_ENABLE | DMA_CTRL_CKSUM | DMA_CTRL_CALIB |
-		TIMING_MASK(div, delay);
+	if(priv->new_ver)
+		dma_ctrl = DMA_CTRL_ENABLE | DMA_CTRL_CKSUM | DMA_CTRL_CALIB |
+			G6_TIMING_MASK(div, delay);
+	else		
+		dma_ctrl = DMA_CTRL_ENABLE | DMA_CTRL_CKSUM | DMA_CTRL_CALIB |
+			TIMING_MASK(div, delay);
 	writel(dma_ctrl, &priv->regs->dma_ctrl);
-
 	while (!(readl(&priv->regs->intr_ctrl) & INTR_CTRL_DMA_STATUS))
 		;
 
@@ -269,7 +289,6 @@
 	checksum = readl(&priv->regs->dma_checksum);
 
 	writel(0x0, &priv->regs->dma_ctrl);
-
 	return checksum;
 }
 
@@ -293,68 +312,116 @@
 static int aspeed_spi_timing_calibration(struct aspeed_spi_priv *priv)
 {
 	/* HCLK/5 .. HCLK/1 */
-	const u8 hclk_masks[] = { 13, 6, 14, 7, 15 };
-	u32 timing_reg = 0;
+	const u8 hclk_masks[] = {13, 6, 14, 7, 15};
+	u32 timing_reg;
 	u32 checksum, gold_checksum;
-	int i, hcycle;
+	int i, hcycle, delay_ns;
+
+	/* Use the ctrl setting in aspeed_spi_flash_init() to
+	 * implement calibration process.
+	 */
+	timing_reg = readl(&priv->regs->timings);
+	if (timing_reg != 0)
+		return 0;
 
 	debug("Read timing calibration :\n");
 
 	/* Compute reference checksum at lowest freq HCLK/16 */
 	gold_checksum = aspeed_spi_read_checksum(priv, 0, 0);
 
-	/*
-	 * Set CE0 Control Register to FAST READ command mode. The
-	 * HCLK divisor will be set through the DMA Control Register.
-	 */
-	writel(CE_CTRL_CMD(0xb) | CE_CTRL_DUMMY(1) | CE_CTRL_FREADMODE,
-	       &priv->regs->ce_ctrl[0]);
-
 	/* Increase HCLK freq */
 	if (priv->new_ver) {
 		for (i = 0; i < ARRAY_SIZE(hclk_masks) - 1; i++) {
 			u32 hdiv = 5 - i;
-			u32 hshift = (hdiv - 1) << 2;
+			u32 hshift = (hdiv - 2) * 8;
 			bool pass = false;
 			u8 delay;
+			u16 first_delay = 0;
+			u16 end_delay = 0;
+			u32 cal_tmp;
+			u32 max_window_sz = 0;
+			u32 cur_window_sz = 0;
+			u32 tmp_delay;
 
+			debug("hdiv %d, hshift %d\n", hdiv, hshift);
 			if (priv->hclk_rate / hdiv > priv->max_hz) {
 				debug("skipping freq %ld\n", priv->hclk_rate / hdiv);
 				continue;
 			}
 
-			/* Increase HCLK cycles until read succeeds */
-			for (hcycle = 0; hcycle <= TIMING_DELAY_HCYCLE_MAX; hcycle++) {
-				/* Try first with a 4ns DI delay */
-				delay = TIMING_DELAY_DI_4NS | hcycle;
-				checksum = aspeed_spi_read_checksum(priv, hclk_masks[i],
-								    delay);
-				pass = (checksum == gold_checksum);
-				debug(" HCLK/%d, 4ns DI delay, %d HCLK cycle : %s\n",
-				      hdiv, hcycle, pass ? "PASS" : "FAIL");
-
-				/* Try again with more HCLK cycles */
-				if (!pass)
-					continue;
-
-				/* Try without the 4ns DI delay */
-				delay = hcycle;
-				checksum = aspeed_spi_read_checksum(priv, hclk_masks[i],
-								    delay);
-				pass = (checksum == gold_checksum);
-				debug(" HCLK/%d,  no DI delay, %d HCLK cycle : %s\n",
-				      hdiv, hcycle, pass ? "PASS" : "FAIL");
+			/* Try without the 4ns DI delay */
+			hcycle = delay = 0;
+			debug("** Dealy Disable **\n");
+			checksum = aspeed_spi_read_checksum(priv, hclk_masks[i], delay);
+			pass = (checksum == gold_checksum);
+			debug("HCLK/%d, no DI delay, %d HCLK cycle : %s\n",
+				  hdiv, hcycle, pass ? "PASS" : "FAIL");
+
+			/* All good for this freq  */
+			if (pass)
+				goto next_div;
 
-				/* All good for this freq  */
-				if (pass)
-					break;
+			/* Try each hcycle delay */
+			for (hcycle = 0; hcycle <= TIMING_DELAY_HCYCLE_MAX; hcycle++) {
+				/* Increase DI delay by the step of 0.5ns */
+				debug("** Delay Enable : hcycle %x ** \n", hcycle);
+				for (delay_ns = 0; delay_ns < 0xf; delay_ns++) {
+					tmp_delay = TIMING_DELAY_DI_4NS | hcycle | (delay_ns << 4);
+					checksum = aspeed_spi_read_checksum(priv, hclk_masks[i],
+									    tmp_delay);
+					pass = (checksum == gold_checksum);
+					debug("HCLK/%d, DI delay, %d HCLK cycle, %d delay_ns : %s\n",
+					      hdiv, hcycle, delay_ns, pass ? "PASS" : "FAIL");
+
+					if (!pass) {
+						if (!first_delay)
+							continue;
+						else {
+							end_delay = (hcycle << 4) | (delay_ns);
+							end_delay = end_delay - 1;
+							/* Larger window size is found */
+							if (cur_window_sz > max_window_sz) {
+								max_window_sz = cur_window_sz;
+								cal_tmp = (first_delay + end_delay) / 2;
+								delay = TIMING_DELAY_DI_4NS |
+										((cal_tmp & 0xf) << 4) |
+										(cal_tmp >> 4);
+							}
+							debug("find end_delay %x %d %d\n", end_delay,
+									hcycle, delay_ns);
+
+							first_delay = 0;
+							end_delay = 0;
+							cur_window_sz = 0;
+
+							break;
+						}
+					} else {
+						if (!first_delay) {
+							first_delay = (hcycle << 4) | delay_ns;
+							debug("find first_delay %x %d %d\n", first_delay, hcycle, delay_ns);
+						}
+						/* Record current pass window size */
+						cur_window_sz++;
+					}
+				}
 			}
 
 			if (pass) {
-				timing_reg &= ~(0xfu << hshift);
-				timing_reg |= delay << hshift;
+				if (cur_window_sz > max_window_sz) {
+					max_window_sz = cur_window_sz;
+					end_delay = ((hcycle - 1) << 4) | (delay_ns - 1);
+					cal_tmp = (first_delay + end_delay) / 2;
+					delay = TIMING_DELAY_DI_4NS |
+							((cal_tmp & 0xf) << 4) |
+							(cal_tmp >> 4);
+				}
 			}
-		}		
+next_div:
+			timing_reg &= ~(0xfu << hshift);
+			timing_reg |= delay << hshift;
+			debug("timing_reg %x, delay %x, hshift bit %d\n",timing_reg, delay, hshift);
+		}
 	} else {
 		for (i = 0; i < ARRAY_SIZE(hclk_masks); i++) {
 			u32 hdiv = 5 - i;
@@ -400,18 +467,16 @@
 			}
 		}
 	}
+
 	debug("Read Timing Compensation set to 0x%08x\n", timing_reg);
 	writel(timing_reg, &priv->regs->timings);
 
-	/* Reset CE0 Control Register */
-	writel(0x0, &priv->regs->ce_ctrl[0]);
-
 	return 0;
 }
 
 static int aspeed_spi_controller_init(struct aspeed_spi_priv *priv)
 {
-	int cs, ret;
+	int cs;
 
 	/*
 	 * Enable write on all flash devices as USER command mode
@@ -421,14 +486,6 @@
 		     CONF_ENABLE_W2 | CONF_ENABLE_W1 | CONF_ENABLE_W0);
 
 	/*
-	 * Set the Read Timing Compensation Register. This setting
-	 * applies to all devices.
-	 */
-	ret = aspeed_spi_timing_calibration(priv);
-	if (ret)
-		return ret;
-
-	/*
 	 * Set safe default settings for each device. These will be
 	 * tuned after the SPI flash devices are probed.
 	 */
@@ -444,14 +501,16 @@
 					debug("cs0 mem-map : %x \n", (u32)flash->ahb_base);
 					break;
 				case 1:
-					flash->ahb_base = priv->flashes[0].ahb_base + 0x8000000;	//cs0 + 128Mb
-					debug("cs1 mem-map : %x end %x \n", (u32)flash->ahb_base, (u32)flash->ahb_base + 0x8000000);
-					addr_config = G6_SEGMENT_ADDR_VALUE((u32)flash->ahb_base, (u32)flash->ahb_base + 0x8000000); //add 128Mb
+					flash->ahb_base = priv->flashes[0].ahb_base + 0x8000000;	//cs0 + 128Mb : use 64MB
+					debug("cs1 mem-map : %x end %x \n", (u32)flash->ahb_base, (u32)flash->ahb_base + 0x4000000);
+					addr_config = G6_SEGMENT_ADDR_VALUE((u32)flash->ahb_base, (u32)flash->ahb_base + 0x4000000); //add 512Mb
 					writel(addr_config, &priv->regs->segment_addr[cs]);
 					break;
 				case 2:
-					flash->ahb_base = priv->flashes[0].ahb_base + 0xc000000;	//cs0 + 196Mb
-					printf("cs2 mem-map : %x \n", (u32)flash->ahb_base);
+					flash->ahb_base = priv->flashes[0].ahb_base + 0xc000000;	//cs0 + 192Mb : use 64MB
+					debug("cs2 mem-map : %x end %x \n", (u32)flash->ahb_base, (u32)flash->ahb_base + 0x4000000);
+					addr_config = G6_SEGMENT_ADDR_VALUE((u32)flash->ahb_base, (u32)flash->ahb_base + 0x4000000); //add 512Mb
+					writel(addr_config, &priv->regs->segment_addr[cs]);
 					break;
 			}
 			flash->cs = cs;
@@ -552,12 +611,22 @@
 	aspeed_spi_write_to_ahb(flash->ahb_base, write_buf, len);
 	aspeed_spi_stop_user(priv, flash);
 
+	debug("=== write opcode [%x] ==== \n", opcode);
 	switch(opcode) {
 		case SPINOR_OP_EN4B:
-			writel(readl(&priv->regs->ctrl) | BIT(flash->cs), &priv->regs->ctrl);
+			/* For ast2600, if 2 chips ABR mode is enabled,
+			 * turn on 3B mode auto clear in order to avoid
+			 * the scenario where spi controller is at 4B mode
+			 * and flash site is at 3B mode after 3rd switch.
+			 */
+			if (priv->new_ver == 1 && (readl(SPI_3B_AUTO_CLR_REG) & SPI_3B_AUTO_CLR))
+				writel(readl(&priv->regs->soft_rst_cmd_ctrl) | SOFT_RST_CMD_EN,
+						&priv->regs->soft_rst_cmd_ctrl);
+
+			writel(readl(&priv->regs->ctrl) | (0x11 << flash->cs), &priv->regs->ctrl);
 			break;
 		case SPINOR_OP_EX4B:
-			writel(readl(&priv->regs->ctrl) & ~BIT(flash->cs), &priv->regs->ctrl);
+			writel(readl(&priv->regs->ctrl) & ~(0x11 << flash->cs), &priv->regs->ctrl);
 			break;
 	}
 	return 0;
@@ -574,6 +643,9 @@
 	/* First, send the opcode */
 	aspeed_spi_write_to_ahb(flash->ahb_base, &cmdbuf[0], 1);
 
+	if(flash->iomode == CE_CTRL_IO_QUAD_ADDR_DATA)
+		writel(flash->ce_ctrl_user | flash->iomode, &priv->regs->ce_ctrl[flash->cs]);
+
 	/*
 	 * The controller is configured for 4BYTE address mode. Fix
 	 * the address width and send an extra byte if the SPI Flash
@@ -623,8 +695,13 @@
 {
 	aspeed_spi_start_user(priv, flash);
 
-	/* cmd buffer = cmd + addr */
+	/* cmd buffer = cmd + addr : normally cmd is use signle mode*/
 	aspeed_spi_send_cmd_addr(priv, flash, cmdbuf, cmdlen);
+
+	/* data will use io mode */
+	if(flash->iomode == CE_CTRL_IO_QUAD_DATA)
+		writel(flash->ce_ctrl_user | flash->iomode, &priv->regs->ce_ctrl[flash->cs]);
+
 	aspeed_spi_write_to_ahb(flash->ahb_base, write_buf, len);
 
 	aspeed_spi_stop_user(priv, flash);
@@ -659,10 +736,15 @@
 					      cmdlen - (flash->spi->read_dummy/8));
 
 	/*
-	 * Switch to USER command mode if the AHB window configured
-	 * for the device is too small for the read operation
+	 * Switch to USER command mode:
+	 * - if the AHB window configured for the device is
+	 *   too small for the read operation
+	 * - if read offset is smaller than the decoded start address
+	 *   and the decoded range is not multiple of flash size
 	 */
-	if (offset + len >= flash->ahb_size) {
+	if ((offset + len >= flash->ahb_size) || \
+		(offset < ((int)flash->ahb_base & 0x0FFFFFFF) && \
+		(((int)flash->ahb_base & 0x0FFFFFFF) % flash->spi->size) != 0)) {
 		return aspeed_spi_read_user(priv, flash, cmdlen, cmdbuf,
 					    len, read_buf);
 	}
@@ -786,10 +868,12 @@
 				 struct aspeed_spi_flash *flash,
 				 struct udevice *dev)
 {
+	int ret;
 	struct spi_flash *spi_flash = dev_get_uclass_priv(dev);
 	struct spi_slave *slave = dev_get_parent_priv(dev);
 	u32 read_hclk;
 
+
 	/*
 	 * The SPI flash device slave should not change, so initialize
 	 * it only once.
@@ -823,15 +907,20 @@
 	else
 		read_hclk = aspeed_spi_hclk_divisor(priv, slave->speed);
 
-	if (slave->mode & (SPI_RX_DUAL | SPI_TX_DUAL)) {
-		debug("CS%u: setting dual data mode\n", flash->cs);
-		flash->iomode = CE_CTRL_IO_DUAL_DATA;
-		flash->spi->read_opcode = SPINOR_OP_READ_1_1_2;
-	} else if (slave->mode & (SPI_RX_QUAD | SPI_TX_QUAD)) {
-		flash->iomode = CE_CTRL_IO_QUAD_DATA;
-		flash->spi->read_opcode = SPINOR_OP_READ_1_4_4;
-	} else {
-		debug("normal read \n");
+	switch(flash->spi->read_opcode) {
+		case SPINOR_OP_READ_1_1_2:
+		case SPINOR_OP_READ_1_1_2_4B:
+			flash->iomode = CE_CTRL_IO_DUAL_DATA;
+			break;
+		case SPINOR_OP_READ_1_1_4:
+		case SPINOR_OP_READ_1_1_4_4B:
+			flash->iomode = CE_CTRL_IO_QUAD_DATA;
+			break;
+		case SPINOR_OP_READ_1_4_4:
+		case SPINOR_OP_READ_1_4_4_4B:
+			flash->iomode = CE_CTRL_IO_QUAD_ADDR_DATA;
+			printf("need modify dummy for 3 bytes");
+			break;
 	}
 
 	if(priv->new_ver) {
@@ -848,6 +937,9 @@
 			CE_CTRL_FREADMODE;
 	}
 
+	if (flash->spi->addr_width == 4)
+		writel(readl(&priv->regs->ctrl) | 0x11 << flash->cs, &priv->regs->ctrl);
+
 	debug("CS%u: USER mode 0x%08x FREAD mode 0x%08x\n", flash->cs,
 	      flash->ce_ctrl_user, flash->ce_ctrl_fread);
 
@@ -857,6 +949,14 @@
 	/* Set Address Segment Register for direct AHB accesses */
 	aspeed_spi_flash_set_segment(priv, flash);
 
+	/*
+	 * Set the Read Timing Compensation Register. This setting
+	 * applies to all devices.
+	 */
+	ret = aspeed_spi_timing_calibration(priv);
+	if (ret != 0)
+		return ret;
+
 	/* All done */
 	flash->init = true;
 
@@ -897,8 +997,10 @@
 	debug("%s: setting mode to %x\n", bus->name, mode);
 
 	if (mode & (SPI_RX_QUAD | SPI_TX_QUAD)) {
+#ifndef CONFIG_ASPEED_AST2600
 		pr_err("%s invalid QUAD IO mode\n", bus->name);
 		return -EINVAL;
+#endif
 	}
 
 	/* The CE Control Register is set in claim_bus() */
@@ -1016,6 +1118,7 @@
 	{ .compatible = "aspeed,ast2600-spi", .data = 0 },
 	{ .compatible = "aspeed,ast2500-fmc", .data = 1 },
 	{ .compatible = "aspeed,ast2500-spi", .data = 0 },
+	{ .compatible = "aspeed,ast2400-fmc", .data = 1 },
 	{ }
 };
 
