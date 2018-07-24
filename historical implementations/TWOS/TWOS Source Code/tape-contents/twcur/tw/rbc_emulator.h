/*
 * $Log:	rbc_emulator.h,v $
 * Revision 1.2  91/06/03  12:26:18  configtw
 * Tab conversion.
 * 
 * Revision 1.1  91/03/26  10:33:47  pls
 * Initial revision
 * 
*/

/*
 * Copyright (C) 1991, Integrated Parallel Technology, All Rights Reserved.
 * U. S. Government Sponsorship under SBIR Contract NAS7-1102 is acknowledged.
 */

/* rbc_emulator.h */

/*
 * state_address and cmd_address are local to rbc_init_start.
 * define STATE_ADDRESS and CMD_ADDRESS so the include files
 * below will not redefine them (so we don't get the defaults).
 */

#define STATE_ADDRESS state_address
#define CMD_ADDRESS cmd_address

#define RBC_NUM_KB      64

typedef char kilo_mem[1024];
/* kilo_mem kilo_memory[RBC_NUM_KB] but malloced */
extern kilo_mem * kilo_memory;
