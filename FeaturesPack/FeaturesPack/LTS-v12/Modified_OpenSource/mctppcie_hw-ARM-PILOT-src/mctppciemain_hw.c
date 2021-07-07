/*
 ****************************************************************
 **                                                            **
 **    (C)Copyright 2006-2009, American Megatrends Inc.        **
 **                                                            **
 **            All Rights Reserved.                            **
 **                                                            **
 **        5555 Oakbrook Pkwy Suite 200, Norcross,             **
 **                                                            **
 **        Georgia - 30093, USA. Phone-(770)-246-8600.         **
 **                                                            **
 ****************************************************************
 */

/*
 * PECI Hardware Specific Driver
 *
 * Copyright (C) 2009 American Megatrends Inc.
 *
 * This driver provides hardware specific layer for the PECI driver.
 */

#include <linux/version.h>
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,28))
#ifndef AUTOCONF_INCLUDED
#include <linux/config.h>
#endif
#endif
#include <linux/kernel.h>	
#include <linux/version.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/errno.h>
#include <linux/sched.h>
#include <linux/interrupt.h>
#include <linux/kthread.h>
#include <linux/delay.h>
#include <asm/uaccess.h>
#include <asm/delay.h>
#include <asm/io.h>

#include "mctppcie_hw.h"
#include "mctppcie.h"
#include "mctppcieifc.h"

#include "driver_hal.h"
#include "helper.h"
#include "reset.h"

#define MCTP_PCIE_HW_DEV_NAME	"mctppcie_hw"
#define MAX_MCTP_PCIE_HW_CHANNELS	MCTP_PCIE_HW_MAX_INST

unsigned int IRQ_NUMBER;

volatile long int g_ds_byte_count = 0;
volatile long int g_ds_interrupt_count = 0;
volatile long int g_ds_overflow_count = 0;
volatile long int g_cur_us_desc = 0;
volatile long int g_slen = 0;
volatile long int g_len = 0;
volatile long int g_chip_rev = PILOT_CHIP_A1;

mctp_device mctp_dev;
mctp_ringbuf g_mrbuf;

DECLARE_WAIT_QUEUE_HEAD(mctp_pcieint_wq);

static int __init mctppcie_hw_init (void);
static void __exit mctppcie_hw_exit (void);

static int downstream_mctp_pkt(mctp_iodata *io);
static int upstream_mctp_pkt(mctp_iodata * io);
static int mctp_check_reset(mctp_iodata *io);
static int enable_mctp_pkt(void);

static mctppcie_hal_operations_t mctppcie_hw_ops = {
    downstream_mctp_pkt	:       downstream_mctp_pkt,
    upstream_mctp_pkt	:       upstream_mctp_pkt,
    enable_mctp_pkt	:	enable_mctp_pkt,
    mctp_check_reset    :       mctp_check_reset,
};	

static hw_hal_t hw_hal = {
	.dev_type = EDEV_TYPE_MCTP_PCIE,
	.owner = THIS_MODULE,
	.devname = MCTP_PCIE_HW_DEV_NAME,
	.num_instances = MAX_MCTP_PCIE_HW_CHANNELS,
	.phal_ops = (void *)&mctppcie_hw_ops
};

static pcie_core_funcs_t *pmctppcie_core_funcs;

static int m_dev_id = 0;

/**
	@brief
	This function restores MCTP registers to (disabled) state - call
	this function during clean up.
*/
static void mctp_restore_pcie_control_regs ( void )
{
	u32 pciectl_reg = 0;

	// TODO: Reset the count values and status bits also.
	iowrite32 ( 0, ( void * ) MCTP_STATUS_REG );

	pciectl_reg = ioread32 ( ( void * ) PCIECTL_REG );
	pciectl_reg &= 0x1FFFFFFF;
	iowrite32 ( pciectl_reg, ( void * ) PCIECTL_REG );
}


static void * mctp_alloc_ds_buf ( mctp_device * dev, u32 sz )
{
	u16 order = get_order ( sz );
	// printk ( "Allocating %lu pages.\n", (1 << order) );
	return ( (void *)(__get_free_pages ( GFP_KERNEL, order ) ) );
}

static void mctp_free_ds_buf ( mctp_device * dev, u32 sz )
{
	u16 order = get_order ( sz );
	free_pages ( (unsigned long)dev->mod_ds_buf, order );
	dev->mod_ds_buf = NULL;
}

/**
	@brief
	This functions programs the PCIE Control register to accept MCTP
	downstream packets.
*/
static void mctp_enable_pcie_control_regs ( void )
{
	u32 reg = 0;

	reg = ioread32 (( void * ) PCIECTL_REG );
	reg = reg | PCIECTL_MCTP_ENABLE | PCIECTL_VDM_DECODE_DISABLE | PCIECTL_DMTFID_DECODE_DISABLE;
	iowrite32 ( reg, ( void * ) PCIECTL_REG );
}


static void mctp_enable_interrupts ( void )
{
	u32 status = 0;

	// TODO: Read modify update this register
	//status = STATUS_DS_VALID | STATUS_DS_INT_ENABLE | STATUS_US_INT_ENABLE;
	status = STATUS_DS_VALID | STATUS_DS_INT_ENABLE;
	iowrite32 ( status, ( void * ) MCTP_STATUS_REG );
}
/**
	@brief
	This function handles whatever is necessary once the DS copy is done.
	This function can be called from LDMA ISR or non DMA transfers.
*/
static void mctp_ds_complete ( void )
{
	mctp_ringbuf * sdbuf = &g_mrbuf;

	iowrite32 ( g_slen, ( void * ) DS_RD_CNTR );

	sdbuf->d_head = ( ( sdbuf->d_head + g_len ) % sdbuf->d_size );
	wake_up_interruptible(&mctp_pcieint_wq);
	// Enable MCTP interrupts
	mctp_enable_interrupts ();
	//printk ( "DS transfer complete.\n" );
}


static int mctp_read_device_buffer ( mctp_ringbuf * sdbuf  )
{
	u8 *sbuf = NULL, *dbuf = NULL;
	u32 read_ctr = 0, write_ctr = 0, stail = 0;

	u32 slen = 0, dfree = 0;
	u32 d1 = 0;

	sbuf = (u8 *)(sdbuf->s_buf);  // Source buffer (PCIe buffer)
	dbuf = (u8 *)(sdbuf->d_buf);  // Destination buffer (kmalloc)
	write_ctr = ioread32 ( ( void * ) DS_WR_CNTR );
	read_ctr = ioread32 ( ( void * ) DS_RD_CNTR );

	//spin_lock ( &sdbuf->lock );
	if ( read_ctr > write_ctr )
		sdbuf->s_len = ( 4 * ( MAX_DS_DWORD_COUNT - read_ctr ) );
	else
		sdbuf->s_len = ( 4 * ( write_ctr - read_ctr ) );

	if ( ( sdbuf->s_tail + sdbuf->s_len ) > sdbuf->s_size )
		sdbuf->s_len = sdbuf->s_size - sdbuf->s_tail;

	slen = sdbuf->s_len;
	stail = sdbuf->s_tail;
	//spin_unlock ( &sdbuf->lock );

	// If head == tail then the buffer is empty - we can optimize a little
	if ( sdbuf->d_head == sdbuf->d_tail )
		sdbuf->d_head = sdbuf->d_tail = 0;

	if ( 0 == slen )
	{
		mctp_enable_interrupts ();
		return 0;
	}

	// mctp_print_sdbuf ( sdbuf, "Tasklet : Before copy" );

	if ( sdbuf->d_tail <= sdbuf->d_head )
	{
		d1 = sdbuf->d_size - sdbuf->d_head;
		dfree = ( d1 + sdbuf->d_tail ) - 1;	// Compensate for tail == head
	}
	else
	{
		d1 = sdbuf->d_tail - sdbuf->d_head;
		dfree = d1 - 1;	// Compensate for tail == head
	}

	if ( slen > dfree )
	{
		printk("Destination buffer overflow.");
		mctp_enable_interrupts ();
		return -1;
	}

	if ( slen > d1 )
	{
		sdbuf->s_len = d1;
		slen = d1;
	}

	// printk ( "Bytes to transfer %u.\n", slen );

	sdbuf->s_tail = ( ( sdbuf->s_tail + slen ) % sdbuf->s_size );
	g_len = slen;
	g_slen = ( ( read_ctr + ( slen / 4 ) ) % MAX_DS_DWORD_COUNT );

	g_ds_byte_count += slen;

	memcpy ((dbuf + sdbuf->d_head), (sbuf + stail), slen);
	mctp_ds_complete ();

	return 0;
}

void mctp_empty_buffer_tasklet ( unsigned long arg )
{
	// u32 status = 0;
	mctp_read_device_buffer ( &g_mrbuf );
}

DECLARE_TASKLET( g_mctp_tasklet, mctp_empty_buffer_tasklet, 0 );
u32 g_data_avail = 0;
struct task_struct *g_ds_ts = NULL;
DECLARE_WAIT_QUEUE_HEAD(ds_thread_queue);

int mctp_ds_thread (void *arg)
{
	int ret = 0;
	mctp_ringbuf *sdbuf = &g_mrbuf;
	unsigned long flags;

	while (1)
	{
		ret = wait_event_interruptible_timeout (ds_thread_queue, (g_data_avail == 1), msecs_to_jiffies(1000));
		if (ret >= 0)
		{
			spin_lock_irqsave (&sdbuf->lock, flags);
			g_data_avail = 0;
			mctp_empty_buffer_tasklet (0);
			spin_unlock_irqrestore (&sdbuf->lock, flags);
		}

		if (kthread_should_stop())
			break;
	}

	return 0;
}

#if defined(SOC_PILOT_IV)

/**
	@brief
	This function tries to generate an overflow which resets the module
	to initial state
	Once the hack is done, the MCTP module generates an overflow interrupt
	the ISR handles this interupt and hopefully we are good to go again.
*/
static void mctp_do_reset (void)
{
	volatile u32 creg = 0;
	volatile u32 SyssrerH2Temp=0;
	volatile u32 SyssrerL2Temp=0;

	SyssrerL2Temp = ioread32 (( void * )SYSSRERL2);
	SyssrerH2Temp = ioread32 (( void * )SYSSRERH2);

	// Set just this bit to reset the AHB2PCI module only
	iowrite32 ((0x08000000), ( void * )SYSSRERL2);
	iowrite32 (0x00, ( void * )SYSSRERH2);

	creg = ioread32 (( void * )SYSRCR);
	creg |= 0x01;
	iowrite32 (creg, ( void * )SYSRCR);	// Let it reset

	udelay (500);

	//disable the AHB2PCI module only
    iowrite32 ((SyssrerL2Temp & 0xF7FFFFFF), ( void * )SYSSRERL2);
	iowrite32 (SyssrerH2Temp, ( void * )SYSSRERH2);

	g_cur_us_desc = 0;
	spin_lock (&g_mrbuf.lock);
	g_mrbuf.s_head = g_mrbuf.s_tail = g_mrbuf.d_head = g_mrbuf.d_tail = 0;
	spin_unlock (&g_mrbuf.lock);
}

#endif 

/**
	@brief
	Handles the overflow case
*/
static void mctp_handle_overflow ( mctp_ringbuf * sdbuf  )
{
	u32 w_ctr = 0, r_ctr = 0;
	u32 status = 0, mask = 0;

	printk ( "===========[ Handling overflow ]===========\n" );

	w_ctr = ioread32 ( ( void * ) DS_WR_CNTR );
	r_ctr = ioread32 ( ( void * ) DS_RD_CNTR );
	rmb ();
	// printk ( "Before clearing ds_valid: w_ctr:%u r_ctr:%u\n", w_ctr, r_ctr );

	status = ioread32 ( ( void * ) MCTP_STATUS_REG );
	mask = 0xFFFFFFFF ^ (STATUS_DS_OVERFLOW | STATUS_US_VDM_DONE);
	status = ( ( status & mask ) | ( STATUS_DS_VALID | STATUS_CLR_WR_CNT ) );
	iowrite32 ( status, ( void * ) MCTP_STATUS_REG );

	w_ctr = ioread32 ( ( void * ) DS_WR_CNTR );
	r_ctr = ioread32 ( ( void * ) DS_RD_CNTR );
	rmb ();
	//printk ( "After clearing ds_valid: w_ctr:%u r_ctr:%u\n", w_ctr, r_ctr );
	if ( 0 == w_ctr ) {
		iowrite32 ( 0, ( void * ) DS_RD_CNTR );
	}
	else {
		printk ( "w_ctr != 0\n" );
	}

	r_ctr = ioread32 ( ( void * ) MCTP_STATUS_REG );
	// printk ( "After clearing ds_valid: w_ctr:%u r_ctr:%u\n", w_ctr, r_ctr );

	sdbuf->s_head = 0;
	sdbuf->s_tail = 0;

	// So that the app will get new data
	sdbuf->d_head = 0;
	sdbuf->d_tail = 0;

	g_ds_overflow_count++;

	// Clear the overflow flag and set write count to 0
	// printk ( "Overflow count = %lu\n", g_ds_overflow_count );
	status = STATUS_DS_OVERFLOW | STATUS_DS_INT_ENABLE | STATUS_US_INT_ENABLE;
	iowrite32 ( status, ( void * ) MCTP_STATUS_REG );

#if defined(SOC_PILOT_IV)
	if(g_chip_rev == PILOT_CHIP_A2)
		mctp_do_reset ();
#endif
	// printk ( "===========[ Overflow Handled ]===========\n" );
}



static int 
downstream_mctp_pkt(mctp_iodata *io)
{
	u32 len = 0, blen = 0, d_head = 0, d_tail = 0;
	unsigned long flags;
	u8 *sbuf = NULL;
	//int i;
	mctp_ringbuf * sdbuf = &g_mrbuf;

	sbuf = (u8 *)(sdbuf->d_buf);	// Source buffer (Driver's buffer)

	len = io->len;

	while ( 1 )
	{
		// Nothing to copy
		if ( sdbuf->d_tail == sdbuf->d_head )
		{
			if ( wait_event_interruptible_timeout ( mctp_pcieint_wq, (sdbuf->d_tail != sdbuf->d_head),msecs_to_jiffies(MCTP_PCIE_READ_TIMEOUT)) <= 0 ){
				io->status = MCTP_STATUS_ERROR;
				io->len = 0;

				return -ERESTARTSYS;
			}

			// TODO: Can we be woken up for other reasons
			// continue;
		}

		spin_lock_irqsave ( &sdbuf->lock, flags );
		d_head = sdbuf->d_head;
		d_tail = sdbuf->d_tail;
		
		if ( d_head == d_tail )
		{
			spin_unlock_irqrestore ( &sdbuf->lock, flags );
			continue;
		}
		if ( d_tail < d_head )
		{
			blen = d_head - d_tail;
		}
		else
		{
			blen = sdbuf->d_size - d_tail;
		}

		if ( len < blen )
			blen = len;

		sdbuf->d_tail = ( ( sdbuf->d_tail + blen ) % sdbuf->d_size );

		memcpy(io->data, (sbuf + d_tail), blen);

		spin_unlock_irqrestore ( &sdbuf->lock, flags );

		io->status = MCTP_STATUS_SUCCESS;
		io->len = blen;

		break;
	}

	return 0;
}


/**
	@brief
	Check the PCIe link state

	@return	0	if the link is up
	@return	1	if the link is down
*/
static int mctp_linkdn ( void )
{
	// TODO: Implement this
	u32 temp = 0;
	temp = ioread32(( void * ) IOADDR_MCTP_LINKDN);
	//printk("mctp_linkdn %x\n",temp);
	if(temp & STATIS_LINKDN)
		return 1;
	else
		return 0;
}


/**
	@brief
	This function copies the provided buffer to upstream buffer
	and sets the us_pktX_valid and us_pktX_len in the descriptors
	Note: the length is expressed in double words.

	@param	dev		mctp device struture
	@param	buf		Which descriptor 0 or 1
	@param	data	Buffer to be transmitted (from the user mode app)
	@param	len		Length of the data to be transmitted (in double words)
*/
static int transmit_us_packet ( mctp_device * dev, u32 buf, void * data, int len )
{
	void __iomem *ioaddr;
	void __iomem *descaddr;
	u32 temp = 0;
	u32 len_dw = 0;

	len_dw = len >> 2;

	ioaddr = dev->us_buf[buf].ioaddr;
	descaddr = dev->us_buf[buf].descaddr;

	temp = ioread32 ( descaddr );
	if ( temp & 0x01 || mctp_linkdn () )
	{
		printk ( "Ebusy: tx buffers filled/link down.\n" );
		return -EBUSY;
	}

	memcpy ( ioaddr, data, len );

	temp = 1 | ( len_dw << 4 );	// Data valid and length
	iowrite32 ( temp, descaddr );

	return 0;
}



static int 
upstream_mctp_pkt(mctp_iodata * io)
{
	int ret;
	u32 desc, attempt = 20; // b = 0;
	unsigned long flags;
	mctp_device * dev = &mctp_dev;

	// printk ( "mctp_do_write called current desc:%lu\n", g_cur_us_desc );

	while ( attempt )
	{
		spin_lock_irqsave ( &dev->usbuf_lock, flags );
		desc = ioread32 ( dev->us_buf[g_cur_us_desc].descaddr );
		spin_unlock_irqrestore ( &dev->usbuf_lock, flags );
		//printk("us_buf[%ld] = descaddr %x\n",g_cur_us_desc,desc);
		if ( 0 == ( desc & 0x01 ) )
		{
			io->status = 0;
			// Workaround -- H/W needs us to set these bits for routing type
			io->data[7] = (io->data[3] & 0x03);

			ret = transmit_us_packet ( dev, g_cur_us_desc, io->data, io->len );
			if ( 0 != ret )
			{
				io->status = MCTP_STATUS_ERROR;
				return MCTP_STATUS_ERROR;
			}

			g_cur_us_desc++;
			g_cur_us_desc %= MAX_US_BUF;

			// printk ( "Tx done. Next desc:%lu\n", g_cur_us_desc );

			return MCTP_STATUS_SUCCESS;
		}

		// Retry after a short sleep. TODO:put process to interruptible sleep
		// printk ( "Attempt [%d]\n", ( 20 - attempt ) );
		udelay(2000);

		attempt--;
	}
	//g_cur_us_desc++;
	//g_cur_us_desc %= MAX_US_BUF;
	//mctp_linkdn();

	printk ( "MCTP Upstream Busy.\n" );
	return -EBUSY;

}

static int 
enable_mctp_pkt()
{
	u32 status = 0;

	status = STATUS_DS_INT_ENABLE | STATUS_US_INT_ENABLE;
	iowrite32 ( status, ( void * ) MCTP_STATUS_REG );

	mctp_enable_pcie_control_regs ();
	return 0;
}

static int mctp_check_reset(mctp_iodata *io)
{ 
    io->data[0] = mctp_linkdn();
    io->len = 1;
    io->status = MCTP_STATUS_SUCCESS;
    return 0;
}

static irqreturn_t 
mctppcie_handler (int irq, void *dev_id)
{
	u32 status = 0, s2 = 0, mask = 0;

	status = ioread32 ( ( void * )MCTP_STATUS_REG );
	s2 = status;

	if ( status & STATUS_DS_VALID )		// What if overflow is also set??
	{
		g_ds_interrupt_count++;

		if ( s2 & STATUS_DS_OVERFLOW )
		{
			printk ( "MCTP ISR: DS + overflow\n" );
			mctp_handle_overflow ( &g_mrbuf );

			g_ds_interrupt_count--;
			//printk ( "ds byte #: %lu | ds_int # %lu\n", g_ds_byte_count, g_ds_interrupt_count );
			g_ds_byte_count = 0;
			g_ds_interrupt_count = 0;
			g_len = 0;
			return IRQ_HANDLED;
		}

		// Disable the interrupts
		mask = 0xFFFFFFFF ^ STATUS_DS_INT_ENABLE;
		status = status & ( mask );
		iowrite32 ( status, ( void * )MCTP_STATUS_REG );

		g_data_avail = 1;
		wake_up_interruptible (&ds_thread_queue);
		//mctp_empty_buffer_tasklet (0);

		return IRQ_HANDLED;
	}
	else if ( status & STATUS_DS_OVERFLOW )
	{
		printk ( "MCTP ISR: overflow\n" );
		mctp_handle_overflow ( &g_mrbuf );
		return IRQ_HANDLED;
	}
	else if ( status & STATUS_US_VDM_DONE )
	{
		//s2 = (status & STATUS_DS_INT_ENABLE) | STATUS_US_VDM_DONE | STATUS_US_INT_ENABLE;
		s2 = (status & STATUS_DS_INT_ENABLE) | STATUS_US_VDM_DONE;
		//printk ( "MCTP ISR: VDM Txd: sts = 0x%x | write 0x%x \n", status, s2 );
		iowrite32 ( s2, ( void * ) MCTP_STATUS_REG );
		return IRQ_HANDLED;
	}
	else
	{
		// Why were we interrupted?
		printk ( "MCTP ISR: status = %x\n", status );
	}

	return ( IRQ_HANDLED );

}

#if defined(SOC_PILOT_IV)
/**
	@brief
	This function writes a value to the read count register so that
	the difference b/w read counter and write counter is large enough
	to generate an overflow. Then handles the overflow
*/
static int mctp_hack_overflow ( void )
{
	u32 w_ctr, r_ctr = 0, status = 0, attempt = 0, mask = 0;

	// Get the current reading of the read and write counters
	w_ctr = ioread32 ( ( void * )DS_WR_CNTR );
	r_ctr = ioread32 ( ( void * )DS_RD_CNTR );
	rmb ();
	// printk ( "w_ctr = 0x%x r_ctr = 0x%x\n", w_ctr, r_ctr );

	// Calculate and write a value to read counter to generate an overflow
	r_ctr = ( ( w_ctr - 250 ) % 256 );
	// printk ( "Writing read counter %x.\n", r_ctr );
	iowrite32 ( r_ctr, ( void * )DS_RD_CNTR );

	// Wait for overflow to occur
	while ( 1 )
	{
		msleep ( 5 );

		status = ioread32 ( ( void * )MCTP_STATUS_REG );
		if ( status & MCTP_STATUS_OVERFLOW )
			break;

		attempt++;
		if ( attempt > 4 )
		{
			printk ( "Overflow flag not set. Module not reset.\n" );
			return -1;
		}
	}

	// Now handle the overflow
	status = ioread32 ( ( void * )MCTP_STATUS_REG );
	w_ctr = ioread32 ( ( void * )DS_WR_CNTR );
	r_ctr = ioread32 ( ( void * )DS_RD_CNTR );
	rmb ();
	// printk ( "Before clearing: status:%x w_ctr:%u r_ctr:%u\n", status, w_ctr, r_ctr );

	mask = 0xFFFFFFFF ^ (STATUS_DS_OVERFLOW);
	status = ( ( status & mask ) | ( STATUS_DS_VALID | STATUS_CLR_WR_CNT ) );
	iowrite32 ( status, ( void * )MCTP_STATUS_REG );
	wmb ();

	w_ctr = ioread32 ( ( void * )DS_WR_CNTR );
	r_ctr = ioread32 ( ( void * )DS_RD_CNTR );

	if ( 0 == w_ctr ) {
		iowrite32 ( 0, ( void * )DS_RD_CNTR );
	}
	else {
		printk ( "w_ctr != 0\n" );
		iowrite32 ( 0, ( void * )DS_RD_CNTR );
	}

	status = ioread32 ( ( void * )MCTP_STATUS_REG );
	rmb ();

	// printk ( "After clearing: status:%x w_ctr:%u r_ctr:%u\n", status, w_ctr, r_ctr );

	status = STATUS_DS_OVERFLOW;
	iowrite32 ( status, ( void * )MCTP_STATUS_REG );

	return 0;
}
#endif

static int __init
mctppcie_hw_init(void)
{
	int ret = 0;
#if defined(SOC_PILOT_IV)
	u32 temp = 0;
#endif

	mctp_ringbuf * sdbuf = &g_mrbuf;

	printk("MCTP PCIE HW Driver, (c) 2014 American Megatrends Inc.\n");

	if ((ret = register_hw_hal_module (&hw_hal, (void **)&pmctppcie_core_funcs)) < 0)
	{
		printk (KERN_ERR "%s: failed to register mctp peci hal module\n", MCTP_PCIE_HW_DEV_NAME);
		return ret;
	}

	m_dev_id = ret;

#if defined(SOC_PILOT_IV)
	g_chip_rev = PILOT_CHIP_A1;
	if (((*(volatile u8 *)(CHIP_REV_REG)) & 0x0F) >= 2)
	{
		printk ("Running on A2.\n");
		g_chip_rev = PILOT_CHIP_A2;
	}
	// Try to reset the module
	if (PILOT_CHIP_A1 == g_chip_rev)
	{
		temp = ioread32( ( void * )MCTP_STATUS_REG);
		g_cur_us_desc = ((temp & 0x80000000) >> 31);
		mctp_hack_overflow ();
	}
	else
	{
		mctp_do_reset ();
	}
#endif

	dbgprint("\nLoading PILOT-III MCTP PCIE Hardware Module.......\n");

	spin_lock_init ( &mctp_dev.usbuf_lock );
	spin_lock_init ( &mctp_dev.lock );

	memset ( sdbuf, 0, sizeof ( mctp_ringbuf ) );

	spin_lock_init ( &sdbuf->lock );

	mctp_dev.mod_ds_buf_size = MODULE_DS_BUFFER_SIZE;
	mctp_dev.mod_ds_buf = mctp_alloc_ds_buf (&mctp_dev, MODULE_DS_BUFFER_SIZE);
	if ( NULL == mctp_dev.mod_ds_buf )
	{
		printk ( "kmalloc MODULE_BUFFER_SIZE failed.\n" );
		ret = -EIO;
		goto fail;
	}
	//Registering the interrupt service routine
#if defined(SOC_PILOT_III)
	IRQ_NUMBER = IRQ_AHB2PCI;
#elif defined(SOC_PILOT_IV)
	//IRQ_NUMBER = IRQ_AHB2PCI_INT;
	IRQ_NUMBER = MCTP_IRQ;
#endif

	if ((ret = request_irq(IRQ_NUMBER, mctppcie_handler, IRQF_DISABLED, MCTP_PCIE_DEV_NAME, 0)) < 0)
	{
		printk("ERROR: Failed to request PECI irq %d name %s (err %d)",IRQ_NUMBER,MCTP_PCIE_DEV_NAME, ret);
		ret = -EIO;
		goto fail;
	}
	mctp_dev.dev_buf = ioremap_nocache ( MCTP_DEV_BUF_ADDR, MCTP_DEV_BUF_LEN );
	if ( NULL == mctp_dev.dev_buf )
	{
		printk ( "ioremap failed.\n" );
		ret = -EIO;
		goto fail;
	}

	sdbuf->s_buf  = mctp_dev.dev_buf;
	sdbuf->s_size = MCTP_DEV_DS_LEN;

	sdbuf->d_buf  = mctp_dev.mod_ds_buf;
	sdbuf->d_size = mctp_dev.mod_ds_buf_size;

	// Initialize the upstream buffer structures
	spin_lock_init ( &mctp_dev.usbuf_lock );
#if defined(SOC_PILOT_III)
	mctp_dev.us_buf[0].ioaddr = (void * )(mctp_dev.dev_buf + 0x300);
#elif defined(SOC_PILOT_IV)
	mctp_dev.us_buf[0].ioaddr = (void * )(mctp_dev.dev_buf + 0x1800);
#endif	
	mctp_dev.us_buf[0].descaddr = (void * )US_PKT1_DESC;
	mctp_dev.us_buf[0].inuse = 0;

#if defined(SOC_PILOT_III)
	mctp_dev.us_buf[1].ioaddr = (void *)(mctp_dev.dev_buf + 0x380);
#elif defined(SOC_PILOT_IV)
	mctp_dev.us_buf[1].ioaddr = (void *)(mctp_dev.dev_buf + 0x1C00);
#endif	
	mctp_dev.us_buf[1].descaddr = (void *)US_PKT2_DESC;
	mctp_dev.us_buf[1].inuse = 0;

	g_ds_ts = kthread_run (mctp_ds_thread, &mctp_dev, "MCTP Downstream Thread");

	return 0;

fail:
	mctppcie_hw_exit();
	return -1;

}

static void __exit
mctppcie_hw_exit(void)
{
	dbgprint("Unloading PILOT-III MCTP PCIE Hardware Module ..............\n");

	unregister_hw_hal_module (EDEV_TYPE_MCTP_PCIE, m_dev_id);

	/* free irq */
	free_irq(IRQ_NUMBER,0);

	kthread_stop (g_ds_ts);

	if (  mctp_dev.mod_ds_buf )
		mctp_free_ds_buf ( &mctp_dev, MODULE_DS_BUFFER_SIZE );

	if ( mctp_dev.dev_buf )
		iounmap ( mctp_dev.dev_buf );

	mctp_dev.mod_ds_buf = NULL;
	mctp_dev.dev_buf = NULL;

	mctp_restore_pcie_control_regs ();

	if (PILOT_CHIP_A1 == g_chip_rev)
	{
		if ( g_cur_us_desc ) {
		iowrite32 ( 0x80000000, ( void * ) MCTP_STATUS_REG );
		}
		else {
		iowrite32 ( 0x00, ( void * ) MCTP_STATUS_REG );
		}
	}

	return;
}

MODULE_AUTHOR("American Megatrends Inc.");
MODULE_DESCRIPTION("Pilot PCIE controller driver");
MODULE_LICENSE("GPL");

module_init (mctppcie_hw_init);
module_exit (mctppcie_hw_exit);
