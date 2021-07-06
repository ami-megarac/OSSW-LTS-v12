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

#ifndef __MCTP_PCIE_HW_H__
#define __MCTP_PCIE_HW_H__


//#include "mctppcie.h"

/* Pilot-iii PCIE Hardware Instance No.*/
#define MCTP_PCIE_HW_MAX_INST	1
	
/* This is to Enable and Disable the debug statement statically  */
#if defined (DEBUG)
#define dbgprint	printk
#else
#define dbgprint(str,...)
#endif

typedef struct __mctp_ringbuf__{
	u8 * s_buf;
	u8 * d_buf;

	u32 s_size;
	u32 s_head;
	u32 s_tail;
	u32 s_len;

	u32 d_size;
	u32 d_head;
	u32 d_tail;

	spinlock_t lock;
}mctp_ringbuf;

#define MCTP_PCIE_READ_TIMEOUT  2000  /* 2000 milliseconds */

//extern void*  pilotii_pcie_addr ;

// Let's keep 101 for now.
#define	MAJOR_NUMBER			100
#define	N_MINORS				1
#define	DRIVER_NAME				"mctp_module"

// Toplevel Pin definitions (base:0x40100900 offset(PCIECTL):0x1c)
#define	PCIECTL_REG					(IO_ADDRESS(0x4010091C))
#define	PCIECTL_MCTP_ENABLE				0x80000000
#define	PCIECTL_VDM_DECODE_DISABLE		0x40000000
#define	PCIECTL_DMTFID_DECODE_DISABLE	0x20000000

// System Debug and Reset control Registers
#define SYS_DEBUG_RESET_CONTROL        IO_ADDRESS(0x40100850)
#define SYSRCR                         IO_ADDRESS(0x40100850)
#define SYSSRERL2                      IO_ADDRESS(0x40100854)
#define SYSSRERH2                      IO_ADDRESS(0x40100858)

// Chip revision related defines
#define CHIP_REV_REG       (IO_ADDRESS(0x40100150))
#define PILOT_CHIP_A1      0x01
#define PILOT_CHIP_A2      0x02

// The physical address of the device buffer and its length.
#if defined(SOC_PILOT_III)
 #define	MCTP_DEV_BUF_ADDR		0x10007800
#elif defined(SOC_PILOT_IV)
 #define	MCTP_DEV_BUF_ADDR		0x10009000
#endif

// MCTP Downstream buffer -- Starts from DEV_BUF_ADDR for now.
#define	IOADDR_MCTP_DSBUF		(IO_ADDRESS(MCTP_DEV_BUF_ADDR))

// MCTP Upstream Buffer 1 and 2 
#if defined(SOC_PILOT_III)
//(128 bytes each)
 #define	IOADDR_MCTP_US_BUF1		(IO_ADDRESS(0x10007B00))
 #define	IOADDR_MCTP_US_BUF2		(IO_ADDRESS(0x10007B80))
#elif defined(SOC_PILOT_IV)
 #define	IOADDR_MCTP_US_BUF1		(IO_ADDRESS(0x1000A800))
 #define	IOADDR_MCTP_US_BUF2		(IO_ADDRESS(0x1000AC00))
#endif

#if defined(SOC_PILOT_III)
 #define	MCTP_DEV_BUF_LEN		1024
#elif defined(SOC_PILOT_IV)
 #define    MCTP_DEV_BUF_LEN        8192    // // 6K + 1K + 1K
#endif

#if defined(SOC_PILOT_III)
 #define	MCTP_DEV_DS_LEN			768
 #define	MCTP_DEV_US_LEN			256		// Two 128-byte buffers
#elif defined(SOC_PILOT_IV)
 #define	MCTP_DEV_DS_LEN			6144
 #define	MCTP_DEV_US_LEN			2048
#endif

#define	IOADDR_MCTP_LINKDN		(IO_ADDRESS(0x4044A040))

// Mailbox register base and offsets
#define	IOADDR_MCTP_MAILBOX		(IO_ADDRESS(0x40B00440))
#define	MCTP_STATUS_REG			(IOADDR_MCTP_MAILBOX + 0x00)
#define	DS_WR_CNTR				(IOADDR_MCTP_MAILBOX + 0x04)
#define	DS_RD_CNTR				(IOADDR_MCTP_MAILBOX + 0x08)
#define	US_PKT1_DESC			(IOADDR_MCTP_MAILBOX + 0x0C)
#define	US_PKT2_DESC			(IOADDR_MCTP_MAILBOX + 0x10)

#define	MCTP_IRQ				110
#define	MAX_US_BUF				2
// Status word definitions
#define	STATUS_DS_VALID			0x00000001
#define	STATUS_DS_OVERFLOW		0x00000002
#define	STATUS_CLR_WR_CNT		0x00000004
#define	STATUS_DS_INT_ENABLE	0x00000008
#define	STATUS_US_VDM_DONE		0x00000010
#define	STATUS_US_INT_ENABLE	0x00000020
#define STATIS_LINKDN			0x00040000

#define	MODULE_DS_BUFFER_SIZE	1024 * 1024

// Maximum upstream buffer length 
#if defined(SOC_PILOT_III)
//( 32 Double words = 128 bytes )
 #define	MAX_US_BUF_LEN			124
 #define	MAX_DS_DWORD_COUNT		256
#elif defined(SOC_PILOT_IV)
 #define	MAX_US_BUF_LEN			1024
 #define	MAX_DS_DWORD_COUNT		4096
#endif
#define	PCIE_MTU_SIZE			MAX_US_BUF_LEN

/**
	This sturcture provides the base address information about
	upstream buffers and their mailbox registers. 
*/
typedef struct __pci_vdm_us_buf__{
	void __iomem *ioaddr;
	void __iomem *descaddr;
	u32 inuse;
}pci_vdm_us_buf;


typedef struct __mctp_device__ {
	spinlock_t lock;
	int inuse;
	u32 mod_ds_buf_size;
	u8 * mod_ds_buf;	// Modules downstream buffer
	u8 * mod_ds_phy;	// Physical address of this buffer
	// u8 * us_buf;	// Upstream buffer

	void * dev_buf;	// This region should be ioremapped
	spinlock_t usbuf_lock;
	pci_vdm_us_buf us_buf[MAX_US_BUF];

	// Thread related data
	struct task_struct *mctp_empty_thread_id;
}mctp_device;

#endif
