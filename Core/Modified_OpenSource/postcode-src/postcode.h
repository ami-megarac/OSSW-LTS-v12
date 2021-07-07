/*****************************************************************
 **                                                             **
 **     (C) Copyright 2009-2015, American Megatrends Inc.       **
 **                                                             **
 **             All Rights Reserved.                            **
 **                                                             **
 **         5555 Oakbrook Pkwy Suite 200, Norcross,             **
 **                                                             **
 **         Georgia - 30093, USA. Phone-(770)-246-8600.         **
 **                                                             **
 ****************************************************************/

#ifndef __POSTCODE_H__
#define __POSTCODE_H__

#define POSTCODE_BUFSIZE    2048


/*
 * define compatibility "struct postcode_kfifo" for dynamic allocated fifos
 */
struct postcode_kfifo __STRUCT_KFIFO_PTR(u32, 0, void);

typedef struct
{
	void (*get_postcode_core_data) (struct postcode_kfifo ** ppost_codes, int dev_id, int ch_num);
} postcode_core_funcs_t;


#endif

