/****************************************************************
 **                                                            **
 **    (C)Copyright 2006-2009, American Megatrends Inc.        **
 **                                                            **
 **            All Rights Reserved.                            **
 **                                                            **
 **        5555 Oakbrook Pkwy Suite 200, Norcross              **
 **                                                            **
 **        Georgia - 30093, USA. Phone-(770)-246-8600.         **
 **                                                            **
****************************************************************/

#ifndef _PILOT_ESPI_MAFS_H_
#define _PILOT_ESPI_MAFS_H_

#define PILOT_ESPIMAFS_DRIVER_NAME      "pilot_espimafs"

#define MAX_XFER_BUFF_SIZE              0xFFF        //4096

#define PILOT_ESPI_REG_BASE             0x40423000
#define PILOT_ESPI_IRQ                  0

#define PILOT_ESPIMAFS_CHANNEL_NUM      1

#define PILOT_ESPI_PERI_CHAN_ID         0
#define PILOT_ESPI_VW_CHAN_ID           1
#define PILOT_ESPI_MAFS_CHAN_ID         2
#define PILOT_ESPI_FLASH_CHAN_ID        3

#define SE_SYSCLK_VA_BASE               0x40100100

// Pilot Specific Registors (PSR)
#define ESPI_INBAND_EN                  (1 << 7)
#define ESPI_INBAND_STS                 (1 << 0)
#define ESPI_RST_INTR_EN                (1 << 2)

#define ESPI_VW_HW_ACK_CTL              (0x40426500 + 0x25) //SIOCFG5
#define ESPI_HW_SLV_BLS                 (1 << 0)
#define ESPI_HW_SLV_BLD                 (1 << 1)
#define ESPI_HW_SUSACK                  (1 << 2)
#define ESPI_HW_MAFSACK                 (1 << 3)
#define ESPI_HW_HSTACK                  (1 << 4)

// Slave Registers
#define PILOT_ESPI_GEN_DEVID            0x04
#define PILOT_ESPI_GEN_CAPCONF          0x08
#define PILOT_ESPI_CH0_CAPCONF          0x10
#define PILOT_ESPI_CH1_CAPCONF          0x20
#define PILOT_ESPI_CH2_CAPCONF          0x30
#define PILOT_ESPI_CH3_CAPCONF          0x40

// eSPI register
#define PILOT_ESPI_VW_INDEX0            (0xC00 + 0x00)
#define PILOT_ESPI_VW_INDEX1            (0xC00 + 0x01)
#define PILOT_ESPI_VW_INDEX2            (0xC00 + 0x02)
#define PILOT_ESPI_VW_INDEX3            (0xC00 + 0x03)
#define PILOT_ESPI_VW_INDEX4            (0xC00 + 0x04)
#define PILOT_ESPI_VW_INDEX5            (0xC00 + 0x05)
#define PILOT_ESPI_VW_INDEX6            (0xC00 + 0x06)
#define PILOT_ESPI_VW_INDEX7            (0xC00 + 0x07)
#define PILOT_ESPI_VW_INDEX40           (0xC00 + 0x40)
#define PILOT_ESPI_VW_INDEX41           (0xC00 + 0x41)
#define PILOT_ESPI_VW_INDEX42           (0xC00 + 0x42)
#define PILOT_ESPI_VW_INDEX43           (0xC00 + 0x43)
#define PILOT_ESPI_VW_INDEX44           (0xC00 + 0x44)
#define PILOT_ESPI_VW_INDEX45           (0xC00 + 0x45)
#define PILOT_ESPI_VW_INDEX46           (0xC00 + 0x46)
#define PILOT_ESPI_VW_INDEX47           (0xC00 + 0x47)

#define PILOT_ESPI_VW_CTRL              (0xC00 + 0xC0)
#define PILOT_ESPI_VW_STATUS0           (0xC00 + 0xD0)
#define PILOT_ESPI_VW_STATUS5           (0xC00 + 0xD5)

#define PILOT_ESPI_OOB_CTRL             (0x800 + 0x00)
#define PILOT_ESPI_OOB_STATUS           (0x800 + 0x04)
#define PILOT_ESPI_OOB_SIZE             (0x800 + 0x08) /* 8 bit*/
#define PILOT_ESPI_OOB_DATA             (0x800 + 0x0C) /* 8 bit*/
#define PILOT_ESPI_OOB_CYC              (0x800 + 0x10) /* 8 bit*/
#define PILOT_ESPI_OOB_TAG              (0x800 + 0x14) /* 8 bit*/
#define PILOT_ESPI_OOB_LEN              (0x800 + 0x18)
#define PILOT_ESPI_OOB_ADD              (0x800 + 0x1C)
#define PILOT_ESPI_OOB_PSR              (0x800 + 0x20)

#define PILOT_ESPI_FLASH_CTRL           (0xA00 + 0x00)
#define PILOT_ESPI_FLASH_STATUS         (0xA00 + 0x04)
#define PILOT_ESPI_FLASH_SIZE           (0xA00 + 0x08) /* 8 bit*/
#define PILOT_ESPI_FLASH_DATA           (0xA00 + 0x0C) /* 8 bit*/
#define PILOT_ESPI_FLASH_CYC            (0xA00 + 0x10) /* 8 bit*/
#define PILOT_ESPI_FLASH_TAG            (0xA00 + 0x14) /* 8 bit*/
#define PILOT_ESPI_FLASH_LEN            (0xA00 + 0x18)
#define PILOT_ESPI_FLASH_ADD            (0xA00 + 0x1C)
#define PILOT_ESPI_FLASH_PSR            (0xA00 + 0x20)

#define GET_CYCLE_TYPE(x)               (x & 0xff)
#define GET_TAG(x)                      ((x >> 8) & 0xf)
#define GET_LEN(x)                      ((x >> 12) & 0xfff)

// register bit value
// Channel#1 CAP
#define CH1_VW_CHANNEL_EN               (1 << 0)

// MAFS/FLASH CTRL 00h
#define B2H_PACKET_VALID                (1 << 0)
#define B2H_INTR_EN                     (1 << 1)
#define B2H_WRPTR_CLEAR                 (1 << 2)
#define B2H_RDPTR_CLEAR                 (1 << 3)
#define H2B_INTR_EN                     (1 << 8)
#define H2B_WRPTR_CLEAR                 (1 << 9)
#define H2B_RDPTR_CLEAR                 (1 << 10)

// MAFS/FLASH STS 04h
#define B2H_BUSY                        (1 << 0)
#define B2H_FIFO_EMPTY                  (1 << 1)
#define B2H_INTR_STATUS                 (1 << 2)
#define H2B_INTR_STATUS                 (1 << 5)
#define H2B_FIFO_EMPTY                  (1 << 6)
#define H2B_PACKET_VALID                (1 << 7)

// VW Reset Type
#define ESPI_NONE                       (1 << 0)
#define ESPI_RESET                      (1 << 1)
#define ESPI_PLTRST                     (1 << 2)
#define ESPI_INBAND_RST                 (1 << 3)

// VW CTRL C0h
#define VW_INTR_EN                      (1 << 0)

// VW List
// VW Index
#define VW_INDEX_03h                    0x3
#define VW_INDEX_04h                    0x4
#define VW_INDEX_05h                    0x5
#define VW_INDEX_06h                    0x6
#define VW_INDEX_07h                    0x7
#define VW_INDEX_40h                    0x40
#define VW_INDEX_41h                    0x41

#define SET_VWIRE(bit, value)           ((value << bit) | (1 << (bit + 4)))

#define VW_IDX_VALID                    (1 << 7)
// index 04h
#define MAFS_RST_ACK                    0
// index 05h
#define SLAVE_BOOT_LOAD_DONE            0
#define SLAVE_BOOT_LOAD_STATUS          3
#define SLAVE_BOOT_LOAD_DONE_VALID      (1 << 4)
#define SLAVE_BOOT_LOAD_STATUS_VALID    (1 << 7)
// index 06h
#define HOST_RST_ACK                    3
// index 40h
#define SUSACK_N                        0


#define ESPI_VW_SUS_WARN                (1 << 0)
#define ESPI_VW_MAFS_RST_WARN           (1 << 2)
#define ESPI_VW_HST_RST_WARN            (1 << 0)


struct espimafs_ch_data {
    volatile u32 op_status;
    wait_queue_head_t as_wait;
    int         full;
    u32         header;        
    u32         buf_len;    
    u8          *buff;
};


#endif /* _PILOT_ESPI_MAFS_H_ */
