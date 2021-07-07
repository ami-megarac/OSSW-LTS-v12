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

 /*
 * File name: espimafsmain_hw.c
 * eSPI FLASH MAFS hardware driver is implemented for hardware controller.
 */

#include <linux/module.h>
#include <linux/pci.h>
#include <linux/fs.h>
#include <asm/io.h>
#include <asm/uaccess.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/partitions.h>

#include "driver_hal.h"
#include "reset.h"
#include "espimafs_ioctl.h"
#include "espimafs.h"
#include "espimafs_hw.h"


static void *pilot_espimafs_virt_base;

static int pilot_espimafs_hal_id;
static espimafs_core_funcs_t *espimafs_core_ops;

struct espimafs_ch_data        mafs_rx_channel;
struct espimafs_ch_data        mafs_tx_channel;

static void pilot_espimafs_flash_tx(int tx_payload_size);
static int espimafs_rx_wait_for_int( int ms_timeout );

#define PILOT_ESPI_MAFS_HEADER_SIZE     3 /* the size of "cyc type + tag + len" */

unsigned char tag = 0;

/* -------------------------------------------------- */

inline uint8_t pilot_espimafs_read8_reg(uint16_t offset)
{
    return ioread8((void * __iomem)pilot_espimafs_virt_base + offset);
}

inline void pilot_espimafs_write8_reg(uint8_t value, uint16_t offset)
{
    iowrite8(value, (void * __iomem)pilot_espimafs_virt_base + offset);
}

inline uint32_t pilot_espimafs_read32_reg(uint32_t offset)
{
    return( ioread32( (void * __iomem)pilot_espimafs_virt_base + offset ) );
}

inline void pilot_espimafs_write32_reg(uint32_t value, uint32_t offset) 
{
    iowrite32(value, (void * __iomem)pilot_espimafs_virt_base + offset);
}
/* -------------------------------------------------- */

static unsigned char pilot_espimafs_num_ch(void)
{
    return PILOT_ESPIMAFS_CHANNEL_NUM;
}

static void flashmafs_rx (struct espimafs_data_t *espimafs_data)
{
    if (espimafs_rx_wait_for_int(1000) == 0)
    {
        espimafs_data->header = mafs_rx_channel.header;
        espimafs_data->buf_len = mafs_rx_channel.buf_len;
        memcpy(espimafs_data->buffer, mafs_rx_channel.buff, mafs_rx_channel.buf_len);
    }
    else
    {
        espimafs_data->header = 0;
        espimafs_data->buf_len = 0;
        memset(espimafs_data->buffer, 0, MAX_XFER_BUFF_SIZE);
    }
    /* Clear RX Data */
    memset(mafs_rx_channel.buff, 0, mafs_rx_channel.buf_len);
    mafs_rx_channel.header = 0;
    mafs_rx_channel.buf_len = 0;
}

static void flashmafs_tx (struct espimafs_data_t *espimafs_data)
{
    mafs_tx_channel.header = espimafs_data->header;
    mafs_tx_channel.buf_len = espimafs_data->buf_len;
    memcpy(mafs_tx_channel.buff, espimafs_data->buffer, mafs_tx_channel.buf_len);
    pilot_espimafs_flash_tx(PILOT_ESPI_MAFS_HEADER_SIZE + espimafs_data->buf_len); /* 3 (cyc + tag + len) + data len*/
}

static int flashmafs_read (uint32_t addr, uint32_t *len, u8 *data)
{
    struct espimafs_data_t espi_data;
    int ret = -1;

    if ((pilot_espimafs_read32_reg(PILOT_ESPI_FLASH_STATUS) & B2H_BUSY) == B2H_BUSY)
    {
        printk(KERN_DEBUG "flashmafs_read : B2H Busy\n");
        return -EAGAIN;
    }

    if (*len > MAX_XFER_BUFF_SIZE)
        return (ret);

    mafs_tx_channel.header = (*len << 12) | ((tag & 0xf) << 8) | ESPI_CYCLE_TYPE_FLASH_READ;
    mafs_tx_channel.buf_len = ESPIMAFS_ADDR_LEN;
    mafs_tx_channel.buff[0] = (addr >> 24) & 0xff;
    mafs_tx_channel.buff[1] = (addr >> 16) & 0xff;
    mafs_tx_channel.buff[2] = (addr >> 8) & 0xff;
    mafs_tx_channel.buff[3] = addr & 0xff;
    pilot_espimafs_flash_tx(PILOT_ESPI_MAFS_HEADER_SIZE + ESPIMAFS_ADDR_LEN); /* 3 (cyc + tag + len) + data len */

    tag ++;
    tag %= 0xf;

    memset(&espi_data, 0 ,sizeof(struct espimafs_data_t));
    espi_data.buffer = data;
    flashmafs_rx (&espi_data);
    *len = GET_LEN(espi_data.header);

    switch(GET_CYCLE_TYPE(espi_data.header))
    {
        case SUCCESS_COMP_WO_DATA:
            //printk("FLASH_RX_COMPLETION SUCCESS_COMP_WO_DATA \n");
            ret = 0;
            break;

        case SUCCESS_COMP_W_DATA_MIDDLE:
            //printk("FLASH_RX_COMPLETION SUCCESS_COMP_W_DATA_MIDDLE \n");
            ret = 0;
            break;

        case SUCCESS_COMP_W_DATA_FIRST:
            //printk("FLASH_RX_COMPLETION SUCCESS_COMP_W_DATA_FIRST \n");
            ret = 0;
            break;

        case SUCCESS_COMP_W_DATA_LAST:
            //printk("FLASH_RX_COMPLETION SUCCESS_COMP_W_DATA_LAST \n");
            ret = 0;
            break;

        case SUCCESS_COMP_W_DATA_ONLY:
            //printk("FLASH_RX_COMPLETION SUCCESS_COMP_W_DATA_ONLY \n");
            ret = 0;
            break;

        case USUCCESS_COMP_WO_DATA_LAST:
            //printk("FLASH_RX_COMPLETION USUCCESS_COMP_WO_DATA_LAST \n");
            break;

        case USUCCESS_COMP_WO_DATA_ONLY:
            //printk("FLASH_RX_COMPLETION USUCCESS_COMP_WO_DATA_ONLY \n");
            break;
    }

    return (ret);
}
static int flashmafs_write (uint32_t addr, uint32_t len, u8 *data)
{
    struct espimafs_data_t espi_data;
    unsigned char cmd[ESPIMAFS_ADDR_LEN] = {0};
    int ret = -1;

    if ((pilot_espimafs_read32_reg(PILOT_ESPI_FLASH_STATUS) & B2H_BUSY) == B2H_BUSY)
    {
        printk(KERN_DEBUG "flashmafs_write : B2H Busy\n");
        return -EAGAIN;
    }

    if (len > MAX_XFER_BUFF_SIZE)
        return (ret);

    mafs_tx_channel.header = (len << 12) | ((tag & 0xf) << 8) | ESPI_CYCLE_TYPE_FLASH_WRITE;
    mafs_tx_channel.buf_len = len + ESPIMAFS_ADDR_LEN;
    mafs_tx_channel.buff[0] = (addr >> 24) & 0xff;
    mafs_tx_channel.buff[1] = (addr >> 16) & 0xff;
    mafs_tx_channel.buff[2] = (addr >> 8) & 0xff;
    mafs_tx_channel.buff[3] = addr & 0xff;
    memcpy(&mafs_tx_channel.buff[ESPIMAFS_ADDR_LEN], data, len);
    pilot_espimafs_flash_tx(PILOT_ESPI_MAFS_HEADER_SIZE + len + ESPIMAFS_ADDR_LEN); /* 3 + (cyc + tag + len) + data len */

    tag ++;
    tag %= 0xf;

    memset(&espi_data, 0 ,sizeof(struct espimafs_data_t));
    espi_data.buffer = cmd;
    flashmafs_rx (&espi_data);

    switch(GET_CYCLE_TYPE(espi_data.header))
    {
        case SUCCESS_COMP_WO_DATA:
            //printk("FLASH_RX_COMPLETION SUCCESS_COMP_WO_DATA \n");
            ret = 0;
            break;

        case SUCCESS_COMP_W_DATA_MIDDLE:
            //printk("FLASH_RX_COMPLETION SUCCESS_COMP_W_DATA_MIDDLE \n");
            ret = 0;
            break;

        case SUCCESS_COMP_W_DATA_FIRST:
            //printk("FLASH_RX_COMPLETION SUCCESS_COMP_W_DATA_FIRST \n");
            ret = 0;
            break;

        case SUCCESS_COMP_W_DATA_LAST:
            //printk("FLASH_RX_COMPLETION SUCCESS_COMP_W_DATA_LAST \n");
            ret = 0;
            break;

        case SUCCESS_COMP_W_DATA_ONLY:
            //printk("FLASH_RX_COMPLETION SUCCESS_COMP_W_DATA_ONLY \n");
            ret = 0;
            break;

        case USUCCESS_COMP_WO_DATA_LAST:
            //printk("FLASH_RX_COMPLETION USUCCESS_COMP_WO_DATA_LAST \n");
            break;

        case USUCCESS_COMP_WO_DATA_ONLY:
            //printk("FLASH_RX_COMPLETION USUCCESS_COMP_WO_DATA_ONLY \n");
            break;
    }

    return (ret);
}

static int flashmafs_erase (uint32_t addr, uint32_t eraseblocksize)
{
    struct espimafs_data_t espi_data;
    unsigned char cmd[ESPIMAFS_ADDR_LEN] = {0};
    int ret = -1;

    if ((pilot_espimafs_read32_reg(PILOT_ESPI_FLASH_STATUS) & B2H_BUSY) == B2H_BUSY)
    {
        printk(KERN_DEBUG "flashmafs_erase : B2H Busy\n");
        return -EAGAIN;
    }

    mafs_tx_channel.header = (eraseblocksize << 12) | ((tag & 0xf) << 8) | ESPI_CYCLE_TYPE_FLASH_ERASE;
    mafs_tx_channel.buf_len = ESPIMAFS_ADDR_LEN;
    mafs_tx_channel.buff[0] = (addr >> 24) & 0xff;
    mafs_tx_channel.buff[1] = (addr >> 16) & 0xff;
    mafs_tx_channel.buff[2] = (addr >> 8) & 0xff;
    mafs_tx_channel.buff[3] = addr & 0xff;
    pilot_espimafs_flash_tx(PILOT_ESPI_MAFS_HEADER_SIZE + ESPIMAFS_ADDR_LEN); /* 3 (cyc + tag + len) + data len */

    tag ++;
    tag %= 0xf;

    memset(&espi_data, 0 ,sizeof(struct espimafs_data_t));
    espi_data.buffer = cmd;
    flashmafs_rx (&espi_data);

    switch(GET_CYCLE_TYPE(espi_data.header))
    {
        case SUCCESS_COMP_WO_DATA:
            //printk("FLASH_RX_COMPLETION SUCCESS_COMP_WO_DATA \n");
            ret = 0;
            break;

        case SUCCESS_COMP_W_DATA_MIDDLE:
            //printk("FLASH_RX_COMPLETION SUCCESS_COMP_W_DATA_MIDDLE \n");
            ret = 0;
            break;

        case SUCCESS_COMP_W_DATA_FIRST:
            //printk("FLASH_RX_COMPLETION SUCCESS_COMP_W_DATA_FIRST \n");
            ret = 0;
            break;

        case SUCCESS_COMP_W_DATA_LAST:
            //printk("FLASH_RX_COMPLETION SUCCESS_COMP_W_DATA_LAST \n");
            ret = 0;
            break;

        case SUCCESS_COMP_W_DATA_ONLY:
            //printk("FLASH_RX_COMPLETION SUCCESS_COMP_W_DATA_ONLY \n");
            ret = 0;
            break;

        case USUCCESS_COMP_WO_DATA_LAST:
            //printk("FLASH_RX_COMPLETION USUCCESS_COMP_WO_DATA_LAST \n");
            break;

        case USUCCESS_COMP_WO_DATA_ONLY:
            //printk("FLASH_RX_COMPLETION USUCCESS_COMP_WO_DATA_ONLY \n");
            break;
    }

    return (ret);
}

static void get_flash_channel_status(uint32_t *gen_status, uint32_t *ch_status, uint32_t *FlashAccChMaxReadReqSize, uint32_t *FlashAccChMaxPayloadSizeSelected, uint32_t *FlashAccChMaxPayloadSizeSupported, uint32_t *FlashAccChEraseSizeType )
{
    *gen_status = pilot_espimafs_read32_reg(PILOT_ESPI_GEN_CAPCONF);
    
    *ch_status = 0;
    if (pilot_espimafs_read32_reg(PILOT_ESPI_CH3_CAPCONF) & 0x3)
        *ch_status |= FLASH_CHANNEL_SUPPORTED;

    *FlashAccChMaxReadReqSize = (pilot_espimafs_read32_reg(PILOT_ESPI_CH3_CAPCONF) & 0x7000) >> 12;
    *FlashAccChMaxPayloadSizeSelected = (pilot_espimafs_read32_reg(PILOT_ESPI_CH3_CAPCONF) & 0x700) >> 8;
    *FlashAccChMaxPayloadSizeSupported = (pilot_espimafs_read32_reg(PILOT_ESPI_CH3_CAPCONF) & 0xE0) >> 5;
    *FlashAccChEraseSizeType = (pilot_espimafs_read32_reg(PILOT_ESPI_CH3_CAPCONF) & 0x1C) >> 2;

#ifdef ESPI_DEBUG
    // Slave Registers
    printk("PILOT_ESPI_GEN_CAPCONF    (08h) 0x%x\n", (unsigned int)pilot_espioob_read32_reg(PILOT_ESPI_GEN_CAPCONF));
    printk("PILOT_ESPI_CH0_CAPCONF    (10h) 0x%x\n", (unsigned int)pilot_espioob_read32_reg(PILOT_ESPI_CH0_CAPCONF));
    printk("PILOT_ESPI_CH1_CAPCONF    (20h) 0x%x\n", (unsigned int)pilot_espioob_read32_reg(PILOT_ESPI_CH1_CAPCONF));
    printk("PILOT_ESPI_CH2_CAPCONF    (30h) 0x%x\n", (unsigned int)pilot_espioob_read32_reg(PILOT_ESPI_CH2_CAPCONF));
    printk("PILOT_ESPI_CH3_CAPCONF    (40h) 0x%x\n", (unsigned int)pilot_espioob_read32_reg(PILOT_ESPI_CH3_CAPCONF));
#endif
}

static int espimafs_rx_wait_for_int( int ms_timeout )
{
    int status = 0;
    if (wait_event_timeout(mafs_rx_channel.as_wait, mafs_rx_channel.op_status, (ms_timeout*HZ/1000)) == 0)
    {
        status = -1;
        //printk("!!! time out !!!!\n");
    }
    mafs_rx_channel.op_status = 0;

    return status;
}

static void pilot_espimafs_flash_completion(unsigned int header, unsigned char data_len,unsigned char *data)
{
    struct espimafs_data_t espimafs_data;

    espimafs_data.header = header;
    espimafs_data.buf_len = data_len;
    if(data)
        espimafs_data.buffer = data;
    else
        espimafs_data.buffer = NULL;    

    flashmafs_tx(&espimafs_data);
}

static void flash_read_completion(unsigned int header, unsigned char *data)
{
    int i = 0;
    int len = GET_LEN(header);
    //printk("FLASH_COMPLETION cycle type = %x , tag = %x, len = %d byte  \n", GET_CYCLE_TYPE(header), GET_TAG(header), GET_LEN(header));
    if(len <= 64) {
        //single - only
        pilot_espimafs_flash_completion(header |SUCCESS_COMP_W_DATA_ONLY, GET_LEN(header), data);
    } else {
        //split  ---
        header &= 0xffffff00;
        for(i = 0; i < len; i+=64) {
            if(i ==0) {
                pilot_espimafs_flash_completion(header |SUCCESS_COMP_W_DATA_FIRST, 64, data);
            } else if (i + 64 >= len) {
                pilot_espimafs_flash_completion(header |SUCCESS_COMP_W_DATA_LAST, header | 64, data);
            } else {
                pilot_espimafs_flash_completion(header |SUCCESS_COMP_W_DATA_MIDDLE, header | (len - i), data);
            }
        }
    }

}

static void flash_write_completion(unsigned int tag)
{
    //single - only
    pilot_espimafs_flash_completion((tag << 8)  |SUCCESS_COMP_WO_DATA, 0, NULL);

}

static void flash_erase_completion(unsigned int tag)
{
    //single - only
    pilot_espimafs_flash_completion((tag << 8) |SUCCESS_COMP_WO_DATA, 0, NULL);
}

// Channel 3: Runtime Flash
static void pilot_espimafs_flash_rx_completion(void)
{
    struct espimafs_ch_data *flash_rx = &mafs_rx_channel;

    //printk("FLASH_RX_COMPLETION cycle type = %x , tag = %x, len = %d byte  \n", GET_CYCLE_TYPE(flash_rx->header), GET_TAG(flash_rx->header), GET_LEN(flash_rx->header));

    switch(GET_CYCLE_TYPE(flash_rx->header))
    {
        case ESPI_CYCLE_TYPE_FLASH_READ:
            //printk("RX COMPLETION FLASH_READ len %d \n", GET_LEN(flash_rx->header));
            flash_read_completion(flash_rx->header , NULL);
            break;

        case ESPI_CYCLE_TYPE_FLASH_WRITE:
            //printk("RX COMPLETION FLASH_WRITE len %d \n", GET_LEN(flash_rx->header));
            flash_write_completion(GET_TAG(flash_rx->header));
            break;

        case ESPI_CYCLE_TYPE_FLASH_ERASE:
            //printk("RX COMPLETION FLASH_ERASE len %d \n", GET_LEN(flash_rx->header));
            flash_erase_completion(GET_TAG(flash_rx->header));
            break;

        case SUCCESS_COMP_WO_DATA:
            //printk("FLASH_RX_COMPLETION SUCCESS_COMP_WO_DATA \n");
            break;

        case SUCCESS_COMP_W_DATA_MIDDLE:
            //printk("FLASH_RX_COMPLETION SUCCESS_COMP_W_DATA_MIDDLE \n");
            break;

        case SUCCESS_COMP_W_DATA_FIRST:
            //printk("FLASH_RX_COMPLETION SUCCESS_COMP_W_DATA_FIRST \n");
            break;

        case SUCCESS_COMP_W_DATA_LAST:
            //printk("FLASH_RX_COMPLETION SUCCESS_COMP_W_DATA_LAST \n");
            break;

        case SUCCESS_COMP_W_DATA_ONLY:
            //printk("FLASH_RX_COMPLETION SUCCESS_COMP_W_DATA_ONLY \n");
            break;

        case USUCCESS_COMP_WO_DATA_LAST:
            //printk("FLASH_RX_COMPLETION USUCCESS_COMP_WO_DATA_LAST \n");
            break;

        case USUCCESS_COMP_WO_DATA_ONLY:
            //printk("FLASH_RX_COMPLETION USUCCESS_COMP_WO_DATA_ONLY \n");
            break;
    }
}

static void pilot_espimafs_flash_rx(void)
{
    int rx_cnt = 0;
    struct espimafs_ch_data *flash_rx = &mafs_rx_channel;
    u32 ctrl = ((pilot_espimafs_read32_reg(PILOT_ESPI_FLASH_LEN) & 0xfff) << 12) |
               ((pilot_espimafs_read8_reg(PILOT_ESPI_FLASH_TAG) & 0xf) << 8) |
               (pilot_espimafs_read8_reg(PILOT_ESPI_FLASH_CYC) & 0xff);
    //printk("flash_rx cycle type = %x , tag = %x, len = %d byte \n", GET_CYCLE_TYPE(ctrl), GET_TAG(ctrl), GET_LEN(ctrl));

    flash_rx->full = 1;
    flash_rx->header = ctrl;
    if((GET_CYCLE_TYPE(ctrl) == 0x00) || (GET_CYCLE_TYPE(ctrl) == 0x02))
        flash_rx->buf_len = 4;
    else if (GET_CYCLE_TYPE(ctrl) == 0x01)
        flash_rx->buf_len = GET_LEN(ctrl) + 4;
    else if ((GET_CYCLE_TYPE(ctrl) & 0x09) == 0x09)
        flash_rx->buf_len = GET_LEN(ctrl);
    else
        flash_rx->buf_len = 0;

    rx_cnt = 0;
    while ((pilot_espimafs_read32_reg(PILOT_ESPI_FLASH_STATUS) & 0x40) == 0x00)
        flash_rx->buff[rx_cnt++] = pilot_espimafs_read8_reg(PILOT_ESPI_FLASH_DATA);

    if (flash_rx->buf_len != rx_cnt)
    {
        printk("pilot_espimafs_flash_rx : the received data len mismatch\n");
        flash_rx->buf_len = rx_cnt;
    }

    // Wake up RX process
    mafs_rx_channel.op_status = 1;
    wake_up( &mafs_rx_channel.as_wait );
}

static void pilot_espimafs_flash_tx(int tx_payload_size)
{
    int i=0;    

    //printk("flash_tx cycle type = %x , tag = %x, len = %d byte, tx payload size = %d byte \n", GET_CYCLE_TYPE(mafs_tx_channel.header), GET_TAG(mafs_tx_channel.header), GET_LEN(mafs_tx_channel.header), tx_payload_size);

    pilot_espimafs_write32_reg(pilot_espimafs_read32_reg(PILOT_ESPI_FLASH_CTRL) | (B2H_WRPTR_CLEAR | B2H_RDPTR_CLEAR | H2B_WRPTR_CLEAR | H2B_RDPTR_CLEAR), PILOT_ESPI_FLASH_CTRL);

    mafs_rx_channel.op_status = 0;

    // Size of Tx payload. (Including header(Cycle Type + TAG + LENGTH) + address(option) + data)
    pilot_espimafs_write8_reg(tx_payload_size, PILOT_ESPI_FLASH_SIZE);

    // Send Header information (Cycle Type, TAG and LEN)
    // Byte 0(Cycle Type)
    pilot_espimafs_write8_reg(GET_CYCLE_TYPE(mafs_tx_channel.header), PILOT_ESPI_FLASH_DATA);
    // Byte 1(Tag + Length[11:8])
    pilot_espimafs_write8_reg((unsigned char) (((GET_LEN(mafs_tx_channel.header) >> 8) & 0xf) | ((GET_TAG(mafs_tx_channel.header) & 0xf) << 4)), PILOT_ESPI_FLASH_DATA );
    // Byte 2 Length[7:0]
    pilot_espimafs_write8_reg((unsigned char) (GET_LEN(mafs_tx_channel.header) & 0xff), PILOT_ESPI_FLASH_DATA );
    // Byte 3+x address(option) + data
    for(i = 0;i < mafs_tx_channel.buf_len; i++)
        pilot_espimafs_write8_reg ( (unsigned char) mafs_tx_channel.buff[i], PILOT_ESPI_FLASH_DATA );

    // Generate alert to host
    pilot_espimafs_write32_reg(pilot_espimafs_read32_reg(PILOT_ESPI_FLASH_CTRL) | 0x1, PILOT_ESPI_FLASH_CTRL);
}


static void pilot_espimafs_hw_init(void) 
{
    // Enable interrupts
    pilot_espimafs_write32_reg(pilot_espimafs_read32_reg(PILOT_ESPI_FLASH_CTRL) | (H2B_INTR_EN | B2H_INTR_EN), PILOT_ESPI_FLASH_CTRL);
}

/* -------------------------------------------------- */
static espimafs_hal_operations_t pilot_espimafs_hw_ops = {
    .num_espimafs_ch        = pilot_espimafs_num_ch,
    .flashmafs_read 		= flashmafs_read,
    .flashmafs_write 		= flashmafs_write,
    .flashmafs_erase 		= flashmafs_erase,
    .get_channel_status     = get_flash_channel_status,
};

static hw_hal_t pilot_espimafs_hw_hal = {
    .dev_type           = EDEV_TYPE_ESPIMAFS,
    .owner              = THIS_MODULE,
    .devname            = PILOT_ESPIMAFS_DRIVER_NAME,
    .num_instances      = PILOT_ESPIMAFS_CHANNEL_NUM,
    .phal_ops           = (void *) &pilot_espimafs_hw_ops
};

static int pilot_espimafs_reset_handler(void)
{
    printk(KERN_DEBUG "pilot_espimafs_reset_handler: got eSPI RESET\n");

    // clear the RD/WR PTRs
    pilot_espimafs_write32_reg(pilot_espimafs_read32_reg(PILOT_ESPI_FLASH_CTRL) | (B2H_WRPTR_CLEAR | B2H_RDPTR_CLEAR | H2B_WRPTR_CLEAR | H2B_RDPTR_CLEAR), PILOT_ESPI_FLASH_CTRL);

    return 0;
}

static irqreturn_t pilot_espimafs_handler (int this_irq, void *dev_id)
{
    unsigned int handled = 0;
    unsigned long status = 0;

    status = pilot_espimafs_read32_reg(PILOT_ESPI_FLASH_STATUS);

    if (status & B2H_BUSY) {
        //printk("B2H_BUSY \n");
        pilot_espimafs_write32_reg(B2H_BUSY, PILOT_ESPI_FLASH_STATUS);
        handled |= B2H_BUSY;
    }

    if (status & B2H_FIFO_EMPTY) {
        //printk("B2H_FIFO_EMPTY \n");
        pilot_espimafs_write32_reg(B2H_FIFO_EMPTY, PILOT_ESPI_FLASH_STATUS);
        handled |= B2H_FIFO_EMPTY;
    }

    if (status & B2H_INTR_STATUS) {
        //printk("B2H_INTR_STATUS \n");
        pilot_espimafs_write32_reg(B2H_INTR_STATUS, PILOT_ESPI_FLASH_STATUS);
        handled |= B2H_INTR_STATUS;
    }

    if (status & H2B_INTR_STATUS) {
        //printk("H2B_INTR_STATUS \n");
        pilot_espimafs_flash_rx();
        pilot_espimafs_write32_reg((H2B_PACKET_VALID | H2B_INTR_STATUS), PILOT_ESPI_FLASH_STATUS);
        handled |= (H2B_PACKET_VALID | H2B_INTR_STATUS);
		pilot_espimafs_flash_rx_completion();
    }

    if (status & H2B_FIFO_EMPTY) {
        //printk("H2B_FIFO_EMPTY \n");
        pilot_espimafs_write32_reg(H2B_FIFO_EMPTY, PILOT_ESPI_FLASH_STATUS);
        handled |= H2B_FIFO_EMPTY;
    }

    if ((status & (~handled)) == 0)
        return IRQ_HANDLED;
    else
        return IRQ_NONE;
}

int pilot_espimafs_init(void)
{
    uint32_t status;

    extern int espi_core_loaded;
    if (!espi_core_loaded)
        return -1;

    status = *(volatile u32 *)(IO_ADDRESS(SE_SYSCLK_VA_BASE + 0x0C));
    if(!(status & (0x1 << 14)))
    {
        printk(KERN_WARNING "%s: eSPI mode is not enable 0x%x\n", PILOT_ESPIMAFS_DRIVER_NAME, (unsigned int)status);
        return -EIO;
    }

    status = *(volatile u32 *)(IO_ADDRESS(PILOT_ESPI_REG_BASE + PILOT_ESPI_CH3_CAPCONF));
    if(status & (0x1 << 11))
    {
        printk(KERN_WARNING "%s: eSPI not in MAFS mode 0x%x\n", PILOT_ESPIMAFS_DRIVER_NAME, (unsigned int)status);
        return -EIO;
    }

    pilot_espimafs_virt_base = ioremap_nocache(PILOT_ESPI_REG_BASE, SZ_4K);
    if (!pilot_espimafs_virt_base) {
        printk(KERN_WARNING "%s: ioremap failed\n", PILOT_ESPIMAFS_DRIVER_NAME);
        return -ENOMEM;
    }
    
    pilot_espimafs_hal_id = register_hw_hal_module(&pilot_espimafs_hw_hal, (void **) &espimafs_core_ops);
    if (pilot_espimafs_hal_id < 0) {
        printk(KERN_WARNING "%s: register HAL HW module failed\n", PILOT_ESPIMAFS_DRIVER_NAME);
        iounmap (pilot_espimafs_virt_base);
        return pilot_espimafs_hal_id;
    }
    
    // Request IRQ of Flash
    if( request_irq( IRQ_ESPI_SAFS, pilot_espimafs_handler, IRQF_DISABLED, "pilot_espimafs", (pilot_espimafs_virt_base + PILOT_ESPI_FLASH_CHAN_ID)) < 0 )
    {
        printk(KERN_WARNING "failed to request irq %d", IRQ_ESPI_SAFS);
        unregister_hw_hal_module(EDEV_TYPE_ESPIMAFS, pilot_espimafs_hal_id);
        iounmap (pilot_espimafs_virt_base);
        return -EBUSY;
    }

    mafs_rx_channel.buff = kmalloc(MAX_XFER_BUFF_SIZE, GFP_KERNEL);
    mafs_tx_channel.buff = kmalloc(MAX_XFER_BUFF_SIZE, GFP_KERNEL);
    mafs_rx_channel.op_status = 0;
    mafs_tx_channel.op_status = 0;
    init_waitqueue_head( &(mafs_rx_channel.as_wait));

    pilot_espimafs_hw_init();

    install_reset_handler(pilot_espimafs_reset_handler);

    printk("The eSPI MAFS HW Driver is loaded successfully.\n" );
    return 0;
}

void pilot_espimafs_exit(void)
{
    kfree(mafs_tx_channel.buff);
    kfree(mafs_rx_channel.buff);

    free_irq(IRQ_ESPI_SAFS, (pilot_espimafs_virt_base + PILOT_ESPI_FLASH_CHAN_ID));

    iounmap (pilot_espimafs_virt_base);

    uninstall_reset_handler(pilot_espimafs_reset_handler);

    unregister_hw_hal_module(EDEV_TYPE_ESPIMAFS, pilot_espimafs_hal_id);

    return;
}

module_init (pilot_espimafs_init);
module_exit (pilot_espimafs_exit);

MODULE_AUTHOR("American Megatrends Inc.");
MODULE_DESCRIPTION("PILOT SoC eSPI MAFS Driver");
MODULE_LICENSE ("GPL");
