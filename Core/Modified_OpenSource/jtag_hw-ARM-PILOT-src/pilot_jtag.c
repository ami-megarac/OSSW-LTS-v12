#include <linux/module.h>
#include <linux/pci.h>
#include <linux/fs.h>
#include <asm/io.h>
#include <asm/uaccess.h>
#include <linux/delay.h>

#include "driver_hal.h"
#include "jtag.h"
#include "jtag_ioctl.h"
#include "pilot_jtag.h"


#define PILOT_JTAG_DRIVER_NAME	"pilot_jtag"
#define bit_delay() udelay(2)

volatile u8 *gpio_43_v_add = 0;

volatile u8 *pilot_jtag_v_add = 0;
static int pilot_jtag_hal_id;
static jtag_core_funcs_t *jtag_core_ops;

int g_is_Support_LoopFunc = 0;

//jtag pin set for Altera
#define TDI_PIN 0 
#define TDO_PIN	1 
#define TCK_PIN	2 
#define TMS_PIN	3 

typedef enum {
	SET_LOW = 0,
	SET_HIGH,
	
	READ_VAL,
	
	FORCE_SET_LOW = 0x10,
	FORCE_SET_HIGH	
	
}eJtagPinactions;

int jtag_io(int pin, int act){
	// TDI_PIN 0(w), TDO_PIN 1(r), TCK_PIN 2(w), TMS_PIN 3(w)
	static unsigned char Pins[4]={0,};
	int reVal = 0;
    unsigned int op_code = 0;
    unsigned int force_action = 0;
    if (pin > 4 || pin < 0) return -1;
    switch(act){
    
    	case FORCE_SET_LOW:
       	case FORCE_SET_HIGH:
       		force_action = 1;
    	case SET_LOW:
    	case SET_HIGH:
			if(force_action == 1 || Pins[pin] != act) 
			{
				Pins[pin] =  act;

				if (Pins[TMS_PIN])
					op_code |= DTMS;
				if (Pins[TDI_PIN])
					op_code |= DTDO;

				iowrite8(DNTR | op_code, (void * __iomem)pilot_jtag_v_add + JTAG_DIS_CTRL);
				if (Pins[TCK_PIN]) 
					ioread8((void * __iomem)pilot_jtag_v_add + JTAG_DIS_CTRL);
			}
		break;
    	case READ_VAL:
    		if( pin == TDO_PIN)
    			reVal = (ioread16((void * __iomem)gpio_43_v_add + GPDO_OFFSET) >> 8) & 0x1;
		break;
    }
    return reVal;
}


void jtag_sir(unsigned short bits, unsigned int TDI){

	int i = 0;
	
	//pilot_jtag_reset();
	
	//DR SCAN
	iowrite8(DNTR | DTMS | DTDO, (void * __iomem)pilot_jtag_v_add + JTAG_DIS_CTRL);
	bit_delay();
	ioread8((void * __iomem)pilot_jtag_v_add + JTAG_DIS_CTRL);
	bit_delay();
	//IR SCAN
	iowrite8(DNTR | DTMS | DTDO, (void * __iomem)pilot_jtag_v_add + JTAG_DIS_CTRL);
	bit_delay();
	ioread8((void * __iomem)pilot_jtag_v_add + JTAG_DIS_CTRL);
	bit_delay();
	//Capture IR
	iowrite8(DNTR | DTDO, (void * __iomem)pilot_jtag_v_add + JTAG_DIS_CTRL);
	bit_delay();
	ioread8((void * __iomem)pilot_jtag_v_add + JTAG_DIS_CTRL);
	bit_delay();
	//Shift IR
	iowrite8(DNTR | DTDO, (void * __iomem)pilot_jtag_v_add + JTAG_DIS_CTRL);
	bit_delay();
	ioread8((void * __iomem)pilot_jtag_v_add + JTAG_DIS_CTRL);
	bit_delay();
	
	for(i = 0; i < bits; i++){
		if(i == (bits - 1)){
			if(TDI & (0x1 << i)){
				iowrite8(DNTR | DTMS | DTDO, (void * __iomem)pilot_jtag_v_add + JTAG_DIS_CTRL);
				bit_delay();
				ioread8((void * __iomem)pilot_jtag_v_add + JTAG_DIS_CTRL);
				bit_delay();
			}
			else {
				iowrite8(DNTR | DTMS, (void * __iomem)pilot_jtag_v_add + JTAG_DIS_CTRL);
				bit_delay();
				ioread8((void * __iomem)pilot_jtag_v_add + JTAG_DIS_CTRL);
				bit_delay();
			}
		}
		else {
			if(TDI & (0x1 << i)){
				iowrite8(DNTR | DTDO, (void * __iomem)pilot_jtag_v_add + JTAG_DIS_CTRL);
				bit_delay();
				ioread8((void * __iomem)pilot_jtag_v_add + JTAG_DIS_CTRL);
				bit_delay();
			}
			else {
				iowrite8(DNTR, (void * __iomem)pilot_jtag_v_add + JTAG_DIS_CTRL);
				bit_delay();
				ioread8((void * __iomem)pilot_jtag_v_add + JTAG_DIS_CTRL);
				bit_delay();
			}
		}
	}
	
	//Update IR
	iowrite8(DNTR | DTMS | DTDO, (void * __iomem)pilot_jtag_v_add + JTAG_DIS_CTRL);
	bit_delay();
	ioread8((void * __iomem)pilot_jtag_v_add + JTAG_DIS_CTRL);
	bit_delay();
	//IDLE
	iowrite8(DNTR | DTDO, (void * __iomem)pilot_jtag_v_add + JTAG_DIS_CTRL);
	bit_delay();
	ioread8((void * __iomem)pilot_jtag_v_add + JTAG_DIS_CTRL);
	bit_delay();
	iowrite8(DNTR | DTDO, (void * __iomem)pilot_jtag_v_add + JTAG_DIS_CTRL);
	bit_delay();	
	
}	

void jtag_sdr(unsigned short bits, unsigned int *TDI, unsigned int *TDO){

	unsigned int index = 0;
	u32 shift_bits = 0;
	u32 dr_data;
	
	//DR SCAN
	iowrite8(DNTR | DTMS | DTDO, (void * __iomem)pilot_jtag_v_add + JTAG_DIS_CTRL);
	bit_delay();
	ioread8((void * __iomem)pilot_jtag_v_add + JTAG_DIS_CTRL);
	bit_delay();
	//Capture DR
	iowrite8(DNTR | DTDO, (void * __iomem)pilot_jtag_v_add + JTAG_DIS_CTRL);
	bit_delay();
	ioread8((void * __iomem)pilot_jtag_v_add + JTAG_DIS_CTRL);
	bit_delay();
	//Shift DR
	iowrite8(DNTR | DTDO, (void * __iomem)pilot_jtag_v_add + JTAG_DIS_CTRL);
	bit_delay();
	ioread8((void * __iomem)pilot_jtag_v_add + JTAG_DIS_CTRL);
	bit_delay();
	
	while(bits){
		if( !(TDO) ){
			//Write
			dr_data = (TDI[index] >> (shift_bits % 32)) & (0x1);
			if(bits == 1){
				//Exit1 DR
				iowrite8(DNTR | DTMS, (void * __iomem)pilot_jtag_v_add + JTAG_DIS_CTRL);
				bit_delay();
				ioread8((void * __iomem)pilot_jtag_v_add + JTAG_DIS_CTRL);
				bit_delay();
			}
			else {
				iowrite8(DNTR | dr_data, (void * __iomem)pilot_jtag_v_add + JTAG_DIS_CTRL);
				bit_delay();
				ioread8((void * __iomem)pilot_jtag_v_add + JTAG_DIS_CTRL);
				bit_delay();
			}
		}
		else {
			//Read
			if(bits == 1){
				//Exit1 DR
				iowrite8(DNTR | DTMS, (void * __iomem)pilot_jtag_v_add + JTAG_DIS_CTRL);
				bit_delay();
				ioread8((void * __iomem)pilot_jtag_v_add + JTAG_DIS_CTRL);
				bit_delay();
			}
			else {
				iowrite8(DNTR, (void * __iomem)pilot_jtag_v_add + JTAG_DIS_CTRL);
				bit_delay();
				ioread8((void * __iomem)pilot_jtag_v_add + JTAG_DIS_CTRL);
				bit_delay();
			}
			if((ioread16((void * __iomem)gpio_43_v_add + GPDO_OFFSET) >> 8)& 0x1){	//Get GPDI value
				dr_data = 1;
				TDO[index] |= (0x1 << (shift_bits % 32));
			}
			else {
				dr_data = 0;
				TDO[index] &= ~(0x1 << (shift_bits % 32));
			}
		}
		shift_bits ++;
		bits --;
		if((shift_bits % 32) == 0)
			index ++;
	}
	//UPDATE DR
	iowrite8(DNTR | DTMS | DTDO, (void * __iomem)pilot_jtag_v_add + JTAG_DIS_CTRL);
	bit_delay();
	ioread8((void * __iomem)pilot_jtag_v_add + JTAG_DIS_CTRL);
	bit_delay();
	//IDLE
	iowrite8(DNTR | DTDO, (void * __iomem)pilot_jtag_v_add + JTAG_DIS_CTRL);
	bit_delay();
	ioread8((void * __iomem)pilot_jtag_v_add + JTAG_DIS_CTRL);
	bit_delay();
	iowrite8(DNTR | DTDO, (void * __iomem)pilot_jtag_v_add + JTAG_DIS_CTRL);
	bit_delay();

}
   

void pilot_jtag_reset(void){
	
	int i = 0;
	
	iowrite8(JTAG_DIS_CTRL_MODE, (void * __iomem)pilot_jtag_v_add + JTAG_CONF_A);	//Enter Discrete Mode
	
	iowrite8(DNTR | DTMS | DTDO, (void * __iomem)pilot_jtag_v_add + JTAG_DIS_CTRL);
	bit_delay();
	//Send 10 TMS to force State Machine = Test Logic Reset
	for(i=0;i<10;i++){
		ioread8((void * __iomem)pilot_jtag_v_add + JTAG_DIS_CTRL); 						//Read Discrete Control Register to set TCK high
		bit_delay();
		iowrite8(DNTR | DTMS | DTDO, (void * __iomem)pilot_jtag_v_add + JTAG_DIS_CTRL);	//Write Discrete Control Register to set TCK low
		bit_delay();
	}
	ioread8((void * __iomem)pilot_jtag_v_add + JTAG_DIS_CTRL);
	bit_delay();
	//Run Test Idle
	iowrite8(DNTR | DTDO, (void * __iomem)pilot_jtag_v_add + JTAG_DIS_CTRL);
	bit_delay();
	ioread8((void * __iomem)pilot_jtag_v_add + JTAG_DIS_CTRL);
	bit_delay();
	iowrite8(DNTR | DTDO, (void * __iomem)pilot_jtag_v_add + JTAG_DIS_CTRL);
	bit_delay();

}

void jtag_runtest_idle(unsigned int tcks, unsigned int min_mSec){	
	int i =0;
	static unsigned int idle_count = 0;
	
	for(i = 0; i< tcks; i++){
		iowrite8(DNTR | DTDO, (void * __iomem)pilot_jtag_v_add + JTAG_DIS_CTRL);
		bit_delay();
		ioread8((void * __iomem)pilot_jtag_v_add + JTAG_DIS_CTRL);
		bit_delay();
		iowrite8(DNTR | DTMS| DTDO, (void * __iomem)pilot_jtag_v_add + JTAG_DIS_CTRL);
		bit_delay();
	}
	
	if (min_mSec != 0){ 
		mdelay(min_mSec);
	}

	if((idle_count ++ ) % 128 == 0){
		msleep(0);
 	}	
}

void jtag_dr_pause(unsigned int min_mSec){
	//DRSCAN
	iowrite8(DNTR | DTMS| DTDO, (void * __iomem)pilot_jtag_v_add + JTAG_DIS_CTRL);
	bit_delay();
	ioread8((void * __iomem)pilot_jtag_v_add + JTAG_DIS_CTRL);
	bit_delay();
	//Capture DR
	iowrite8(DNTR | DTDO, (void * __iomem)pilot_jtag_v_add + JTAG_DIS_CTRL);
	bit_delay();
	ioread8((void * __iomem)pilot_jtag_v_add + JTAG_DIS_CTRL);
	bit_delay();
	//Exit1DR
	iowrite8(DNTR | DTMS| DTDO, (void * __iomem)pilot_jtag_v_add + JTAG_DIS_CTRL);
	bit_delay();
	ioread8((void * __iomem)pilot_jtag_v_add + JTAG_DIS_CTRL);
	bit_delay();
	//Pause DR
	iowrite8(DNTR | DTDO, (void * __iomem)pilot_jtag_v_add + JTAG_DIS_CTRL);
	bit_delay();
	ioread8((void * __iomem)pilot_jtag_v_add + JTAG_DIS_CTRL);
	bit_delay();
	
	if(min_mSec != 0)
		mdelay(min_mSec);
	
	//Exit2DR
	iowrite8(DNTR | DTMS| DTDO, (void * __iomem)pilot_jtag_v_add + JTAG_DIS_CTRL);
	bit_delay();
	ioread8((void * __iomem)pilot_jtag_v_add + JTAG_DIS_CTRL);
	bit_delay();
	//UpdateDR
	iowrite8(DNTR | DTMS| DTDO, (void * __iomem)pilot_jtag_v_add + JTAG_DIS_CTRL);
	bit_delay();
	ioread8((void * __iomem)pilot_jtag_v_add + JTAG_DIS_CTRL);
	bit_delay();
	//IDLE
	iowrite8(DNTR | DTDO, (void * __iomem)pilot_jtag_v_add + JTAG_DIS_CTRL);
	bit_delay();
	ioread8((void * __iomem)pilot_jtag_v_add + JTAG_DIS_CTRL);
	bit_delay();
	iowrite8(DNTR | DTDO, (void * __iomem)pilot_jtag_v_add + JTAG_DIS_CTRL);
	bit_delay();
}

static jtag_hal_operations_t pilot_jtag_hw_ops = {
	.reset_jtag = pilot_jtag_reset,
};
	
static hw_hal_t pilot_jtag_hw_hal = {
	.dev_type = EDEV_TYPE_JTAG,
	.owner = THIS_MODULE,
	.devname = PILOT_JTAG_DRIVER_NAME,
	.num_instances = PILOT_JTAG_PORT_NUM,
	.phal_ops = (void *) &pilot_jtag_hw_ops
};

int pilot_jtag_init(void)
{
	extern int jtag_core_loaded;
	if(!jtag_core_loaded)
		return -1;
	
	pilot_jtag_hal_id = register_hw_hal_module(&pilot_jtag_hw_hal, (void **) &jtag_core_ops);
	if (pilot_jtag_hal_id < 0) {
		printk(KERN_WARNING "%s: register HAL HW module failed\n", PILOT_JTAG_DRIVER_NAME);
		return pilot_jtag_hal_id;
	}
	
	pilot_jtag_v_add = ioremap_nocache(PILOT_JTAG_REG_BASE, 0x80);
	if(!pilot_jtag_v_add) {
		printk(KERN_WARNING "%s: ioremap failed\n", PILOT_JTAG_DRIVER_NAME);
		unregister_hw_hal_module(EDEV_TYPE_JTAG, pilot_jtag_hal_id);
		return -ENOMEM;
	}
	
	gpio_43_v_add = ioremap_nocache(GPIO_43_OFFSET, 0x40);
	if(!gpio_43_v_add) {
		printk(KERN_WARNING "GPIO43: ioremap failed\n");
		return -ENOMEM;
	}
	
	iowrite8(JTAG_AUTO_OR_FREE_TCK_MODE, (void * __iomem)pilot_jtag_v_add + JTAG_CONF_A);
	iowrite8(0x1c, (void * __iomem)gpio_43_v_add);				//Set GPIO_43_0 Event Debounce Disable
	
	*(volatile u32 *)(IO_ADDRESS(0x40100910)) = (0x00000100);	//Set GPIO_43_0 as GPIO Pin
	
	pilot_jtag_reset();
	
	return 0;
	
}

void pilot_jtag_exit(void)
{
	iowrite8(JTAG_OUTPUT_DISABLE, (void * __iomem)pilot_jtag_v_add + JTAG_CONF_A);
	
	iounmap (pilot_jtag_v_add);
	unregister_hw_hal_module(EDEV_TYPE_JTAG, pilot_jtag_hal_id);
	
	return;
}

EXPORT_SYMBOL(g_is_Support_LoopFunc);
EXPORT_SYMBOL(pilot_jtag_reset);
EXPORT_SYMBOL(jtag_runtest_idle);
EXPORT_SYMBOL(jtag_dr_pause);
EXPORT_SYMBOL(jtag_sir);
EXPORT_SYMBOL(jtag_sdr);
EXPORT_SYMBOL(jtag_io);

module_init (pilot_jtag_init);
module_exit (pilot_jtag_exit);

MODULE_AUTHOR("American Megatrends Inc.");
MODULE_DESCRIPTION("Pilot SoC JTAG Driver");
MODULE_LICENSE ("GPL");
