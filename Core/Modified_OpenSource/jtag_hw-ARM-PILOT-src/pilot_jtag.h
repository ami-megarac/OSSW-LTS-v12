#ifndef _PILOT_JTAG_MASTER_H
#define _PILOT_JTAG_MASTER_H

#define PILOT_JTAG_REG_BASE 0x40429100
#define PILOT_JTAG_PORT_NUM 1

#define GPIO_43_OFFSET 	0x40420330
#define GPDO_OFFSET		0x08

#define JTAG_CONF_A		0x00
#define JTAG_CONF_B		0x04
#define JTAG_STATUS		0x08
#define JTAG_COMMAND	0x0c
#define JTAG_TDO		0x10
#define JTAG_TDI		0x14
#define JTAG_COUNTER	0x18
#define JTAG_DIS_CTRL	0x1c


//CONF_A
#define JTAG_OUTPUT_DISABLE	0x20

#define JTAG_NO_LOOPBACK	0x00
#define JTAG_TMS_LOOPBACK	0x08
#define JTAG_TDO_LOOPBACK	0x10

#define JTAG_AUTO_OR_FREE_TCK_MODE  0x00
#define JTAG_AUTO_OR_GATED_TCK_MODE 0x01
#define JTAG_DIS_CTRL_MODE			0x02


//STATUS
#define TDI_BUFFER_NOT_EMPTY	0x80
#define TDO_BUFFER_IS_FULL		0x40
#define COUNTER_LOAD_COMPLETE	0x20
#define UPDATE_IR				0x0f
#define EXIT2_IR				0x0e
#define PAUSE_IR				0x0d
#define EXIT1_IR				0x0c
#define SHIFT_IR				0x0b
#define CAPTURE_IR				0x0a
#define SELECT_IR_SCAN			0x09
#define RUN_TEST_IDLE			0x08
#define EXIT2_DR				0x06
#define PAUSE_DR				0x05
#define EXIT1_DR				0x04
#define SHIFT_DR				0x03
#define CAPTURE_DR				0x02
#define SELECT_DR_SCAN			0x01
#define TEST_LOGIC_RESET		0x00


//COMMAND
#define SOFTWARE_RESET				0x80
#define OUTPUT_LOW_TO_TRSTn			0x40
#define FINISH_IN_RUN_TEST_IDLE		0x10
#define EXEC_RECIRCULATE_DR_SCAN 	0x0f
#define EXEC_RECIRCULATE_IR_SCAN 	0x0e
#define EXEC_OUTPUT_ONLY_DR_SCAN 	0x0d
#define EXEC_OUTPUT_ONLY_IR_SCAN 	0x0c
#define EXEC_INPUT_ONLY_DR_SCAN		0x0b
#define EXEC_INPUT_ONLY_IR_SCAN 	0x0a
#define EXEC_DR_SCAN				0x09
#define EXEC_IR_SCAN				0x08


//DISCRETE CONTROL
#define DNTR	0x08	//output high to TSRTn
#define DTMS	0x04	//output high to TMS
#define DTDI	0x02	//TDI data received is a logic 1
#define DTDO	0x01	//output high to TDO

/*----------------------------------------------------*/
extern int g_is_Support_LoopFunc;
extern void get_status(void);
extern void pilot_jtag_reset (void);
extern void jtag_sdr(unsigned short bits, unsigned int *TDO,unsigned int *TDI);
extern void jtag_sir(unsigned short bits, unsigned int TDO);
extern void jtag_runtest_idle(unsigned int tcks, unsigned int min_mSec);
extern void jtag_dr_pause(unsigned int min_mSec);

extern int jtag_io(int pin,int act);

/* -------------------------------------------------- */
//#define JTAG_PROGRAM_DEBUG_NOT_SVF
#ifdef JTAG_PROGRAM_DEBUG_NOT_SVF
  #define JTAG_PROGRAM_DEBUG
#endif
/* -------------------------------------------------- */
#define JTAG_IDLE_OFF_TCK

#define DEBUG
#endif	/* _PILOT_JTAG_MASTER_H_*/
