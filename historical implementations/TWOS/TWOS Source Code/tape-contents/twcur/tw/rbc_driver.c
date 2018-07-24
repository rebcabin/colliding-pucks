/*
 * $Log:	rbc_driver.c,v $
 * Revision 1.2  91/07/22  13:02:18  steve
 * Driver must clear seg_encode_table on start up.
 * 
 * Revision 1.1  91/03/26  10:32:17  pls
 * Initial revision
 * 
*/

/*
 * Copyright (C) 1991, Integrated Parallel Technology, All Rights Reserved.
 * U. S. Government Sponsorship under SBIR Contract NAS7-1102 is acknowledged.
 */

#ifndef NO_EMULATOR
#define EMULATOR
/*
   Emulator is the default. Note that the emulator is only used if
   the rollback chip is not found by the rbc_init_start routine.
   this allows nodes without the rollback chip to be used with nodes
   which have the hardware.
*/
#endif

#ifdef BF_MACH
#define TWOS
#endif
#ifdef SUN
#define TWOS
#endif

#ifdef EMULATOR
/* needs to be included first */
#include "rbc_emulator.h"

int emulator_emulating;
#endif

#include "rbc_public.h"
#include "rbc_private.h"

#define DEBUG

RBC_register * cmd_register;

unsigned long * seg_def_table;
unsigned long * seg_frame_counters;
unsigned long * seg_encode_table;
unsigned int rbc_num_KB;
unsigned long rbc_memory_start;
unsigned long rbc_memory_end;

rbc_rollback ( seg_no, num_frames )
unsigned int seg_no, num_frames;
{
	unsigned long status_word;

	if ( ( seg_no > MAX_SEG )
		|| ( num_frames > MAX_FRAME ) || ( num_frames == 0 ) )
	{
#ifdef DEBUG
		printf ( "rbc_rollback error: seg %d frames %d\n",
			seg_no, num_frames );
#endif 
		return BAD_PARAMETER;
	}

	cmd_register->rollback = ( seg_no << SEG_SHIFT ) + num_frames;

#ifdef EMULATOR
	if ( emulator_emulating )
		emulate_rollback();
#endif

	status_word = cmd_register->status
		& (COMMAND_ABORT | HARDWARE_ERROR | ROLLBACK_MASK);

	if ( ! status_word )
		return RBC_SUCCESS; /* success */

	else if ( status_word & COMMAND_ABORT )
		return ABORTED_COMMAND;

	else if ( status_word & ROLLBACK_MASK )
	{
#ifdef DEBUG
		printf ( "rbc_rollback frame OV: status %d\n", status_word);
#endif
		return FRAME_OVERFLOW;
	}

	else /* if ( status_word & HARDWARE_ERROR ) */
	{
#ifdef DEBUG
		printf ( "rbc_rollback h/w error: status %d\n", status_word);
#endif
		return HARD_ERROR;
	}

	/* NOT REACHED */
}

rbc_advance ( seg_no, num_frames )
unsigned int seg_no, num_frames;
{
	unsigned long status_word;

	if ( ( seg_no > MAX_SEG )
		|| ( num_frames > MAX_FRAME ) || ( num_frames == 0 ) )
	{
#ifdef DEBUG
		printf ( "rbc_advance error: seg %d frames %d\n",
			seg_no, num_frames );
#endif 
		return BAD_PARAMETER;
	}

	cmd_register->advance = ( seg_no << SEG_SHIFT ) + num_frames;

#ifdef EMULATOR
	if ( emulator_emulating )
		emulate_advance();
#endif

	status_word = cmd_register->status
		& (COMMAND_ABORT | HARDWARE_ERROR | ADVANCE_MASK);

	if ( ! status_word )
		return RBC_SUCCESS; /* success */

	else if ( status_word & COMMAND_ABORT )
		return ABORTED_COMMAND;

	else if ( status_word & ADVANCE_MASK )
	{
#ifdef DEBUG
		printf ( "rbc_advance frame OV: status %d\n", status_word);
#endif
		return FRAME_OVERFLOW;
	}

	else /* if ( status_word & HARDWARE_ERROR ) */
	{
#ifdef DEBUG
		printf ( "rbc_advance h/w error: status %d\n", status_word);
#endif
		return HARD_ERROR;
	}

	/* NOT REACHED */
}

rbc_mark ( seg_no, num_frames )
unsigned int seg_no, num_frames;
{
	unsigned long status_word;

	if ( ( seg_no > MAX_SEG )
		|| ( num_frames > MAX_FRAME ) || ( num_frames == 0 ) )
	{
#ifdef DEBUG
		printf ( "rbc_mark error: seg %d frames %d\n",
			seg_no, num_frames );
#endif 
		return BAD_PARAMETER;
	}

	cmd_register->mark = ( seg_no << SEG_SHIFT ) + num_frames;


#ifdef EMULATOR
/*
	printf ( "mark: seg %d num_frames %d\n", seg_no, num_frames );
*/
	if ( emulator_emulating )
		emulate_mark();
#endif

	status_word = cmd_register->status
		& (COMMAND_ABORT | HARDWARE_ERROR | MARK_MASK);

	if ( ! status_word )
		return RBC_SUCCESS; /* success */

	else if ( status_word & MARK_MASK )
	{
#ifdef DEBUG
		printf ( "rbc_mark frame OV: status %d\n", status_word);
#endif
		return MARK_OVERFLOW;
	}

	else if ( status_word & COMMAND_ABORT )
		return ABORTED_COMMAND;

	else /* if ( status_word & HARDWARE_ERROR ) */
	{
#ifdef DEBUG
		printf ( "rbc_mark h/w error: status %d\n", status_word);
#endif
		return HARD_ERROR;
	}

	/* NOT REACHED */
}

/*********/

rbc_init_start ()
{
	unsigned long mem_size, status;
#ifdef EMULATOR
	unsigned long emulator_size, cmd_address, state_address;
#ifdef TWOS
	extern char * m_allocate();
#endif
	int i;

	/* some time out conditon yet to be coded */
	emulator_emulating = 1; /* TRUE */

	if ( emulator_emulating )
	{
		emulator_size = RBC_NUM_KB * 1024 /* for state mem */
			+ 8 * 1024 /* control */ + 1023; /* round off */

#ifdef TWOS
		cmd_address = (unsigned long) m_allocate ( emulator_size );
#else
		cmd_address = (unsigned long) malloc ( emulator_size );
#endif
	
		if ( ! cmd_address )
		{
			printf ( "malloc failed in rbc_init\n" );
			return RBC_FATAL_ERROR;
		}
	
		/* round up to KB boundry */
		cmd_address = (cmd_address + 1023) & ~1023;
	
		state_address = cmd_address + 8 * 1024;
		kilo_memory = (kilo_mem *) state_address;
	}
#endif
	/* we need to test for the rollback chip's existence */
	/* and return RBC_FATAL_ERROR if it does not exist */
	/* The test will be some sort of time out on a memory read */

	/* set up hardware addresses */
	cmd_register = (RBC_register *) (CMD_ADDRESS + REG_OFFSET);
	seg_def_table =
		(unsigned long *) (CMD_ADDRESS + SEG_DEF_OFFSET);
	seg_frame_counters = 
		(unsigned long *) (CMD_ADDRESS + SEG_FRAME_COUNT_OFFSET);
	seg_encode_table = 
		(unsigned long *) (CMD_ADDRESS + SEG_ENCODE_OFFSET);

	/* initialize tables --- zero's seg_def and seg_encode tables */
	/* clear all written bit info */ /* reset all frame counters */
	cmd_register->system_commands =
		SET_RUN | CLEAR_TABLES | CLEAR_WB_ALL | RESET_MFC_ALL;

	status = cmd_register->status;

	/* Only the outside world can clear this table */
	for ( i = 0; i < NUM_SEGS; i++ )
		seg_encode_table[i] = 0;

#ifdef EMULATOR
	if ( emulator_emulating )
	{
		/* determine memory size */
		rbc_num_KB = RBC_NUM_KB;

		init_queues ();
	}
	else
	{
#endif
		/* determine memory size */
		mem_size = (status & MEM_CODE_MASK) >> MEM_CODE_SHIFT;

		if ( mem_size < 7 )
		{
			rbc_num_KB = ( 16 <<  mem_size ); /* In kilobytes */
		}
		else
		{
#ifdef DEBUG
			printf ( "rbc_init_start: illegal memory size\n" );
#endif
			return RBC_FATAL_ERROR;
		}
#ifdef EMULATOR
	}
#endif

	rbc_memory_start = STATE_ADDRESS;
	rbc_memory_end = 1024 * rbc_num_KB + STATE_ADDRESS;

	if ( status & (COMMAND_ABORT | HARDWARE_ERROR) )
	{
#ifdef DEBUG
		printf ( "rbc_init_start: chip refused initialization\n" );
#endif
		return RBC_FATAL_ERROR;
	}
	else
		return RBC_SUCCESS;
}

rbc_init_done()
{
	while ( cmd_register->status &
		(CLRING_WB_ALL | CLRING_MFC_ALL | CLRING_WB_TABLES) )
	{
		/* wait */
		/* maybe sleep here if we are talking over the switch 
			on the butterfly */
	}

	return RBC_SUCCESS;
}

rbc_ready( seg_no )
unsigned int seg_no;
{
	if ( seg_no > MAX_SEG )
	{
#ifdef DEBUG
		printf ( "rbc_ready seg_no %d\n", seg_no );
#endif
		return BAD_PARAMETER;
	}

	cmd_register->system_commands = SET_RUN | SET_SID_STATUS;

	cmd_register->status = seg_no << SEG_SHIFT;

	while ( cmd_register->status &
		(CLRING_WB_SEG |TAG_N_PROGRESS |CLRING_MFC_ALL ) ) 
/*?? should we check for advance in progress etc?? */
	{
		/* wait */
		/* maybe sleep here if we are talking over the switch 
			on the butterfly */
	}

	cmd_register->system_commands = SET_RUN; /* clear SID */

	return RBC_SUCCESS;
}

/*********/
/* allocation */
/* begin_addr is first_kilo, end_addr is last_kilo */
/* thus begin_addr <= (valid_add >> 10) - offset <= end_addr */

rbc_allocate ( seg_no, begin_addr, end_addr )
unsigned int seg_no;
unsigned long begin_addr, end_addr; /* addresses in K relative to rbc */
{
	int i;

	if ( (seg_no > MAX_SEG) || (end_addr < begin_addr)
		|| (rbc_num_KB <= end_addr) )
	{
#ifdef DEBUG
		printf ( "rbc_allocate: seg %d; begin %d; end %d\n",
			seg_no, begin_addr, end_addr );
#endif
		return BAD_PARAMETER;
	}

	seg_def_table[seg_no] = (end_addr << HIGH_SHIFT) + begin_addr;

	for ( i = begin_addr; i <= end_addr; i++ )
	{
		seg_encode_table[i] = seg_no;
	}

	return RBC_SUCCESS;
}

rbc_dealloc_start ( seg_no, begin_addr, end_addr )
unsigned int seg_no;
unsigned long begin_addr, end_addr;
{
	unsigned long status;

	if ( (seg_no > MAX_SEG) || (end_addr < begin_addr)
		|| (rbc_num_KB <= end_addr) )
	{
#ifdef DEBUG
		printf ( "rbc_dealloc_start: seg %d; begin %d; end %d\n",
			seg_no, begin_addr, end_addr );
#endif
		return BAD_PARAMETER;
	}

	cmd_register->reset_mfc = seg_no << SEG_SHIFT;
	status = cmd_register->status;

	if ( status & (HARDWARE_ERROR | COMMAND_ABORT) )
	{
#ifdef DEBUG
		printf ( "rbc_dealloc failed to clear counters\n" );
#endif
		return RBC_FATAL_ERROR;
	}

	cmd_register->clear_wb = seg_no << SEG_SHIFT;
	status = cmd_register->status;

	if ( status & (HARDWARE_ERROR | COMMAND_ABORT) )
	{
#ifdef DEBUG
		printf ( "rbc_dealloc failed clear written bits\n" );
#endif
		return RBC_FATAL_ERROR;
	}

	return RBC_SUCCESS;
}

rbc_dealloc_done ( seg_no, begin_addr, end_addr, wait )
unsigned int seg_no;
unsigned long begin_addr, end_addr;
int wait;
{
	int i, not_done;

	if ( (seg_no > MAX_SEG) || (end_addr < begin_addr)
		|| (rbc_num_KB <= end_addr) )
	{
#ifdef DEBUG
		printf ( "rbc_dealloc_done: seg %d; begin %d; end %d\n",
			seg_no, begin_addr, end_addr );
#endif
		return BAD_PARAMETER;
	}

	cmd_register->system_commands = SET_RUN | SET_SID_STATUS;
	cmd_register->status = seg_no << SEG_SHIFT;

	not_done = cmd_register->status & CLRING_WB_SEG;

	if ( wait )
		while ( (not_done = cmd_register->status & CLRING_WB_SEG) )
		{
			/* perhaps sleep here */
		}

	cmd_register->system_commands = SET_RUN; /* clear SID_STATUS */

	if ( not_done )
		return ABORTED_COMMAND;

#ifdef EMULATOR
	if ( emulator_emulating )
		emulate_dealloc ( seg_no, begin_addr, end_addr );
#endif

	seg_def_table[seg_no] = 0;

	for ( i = begin_addr; i <= end_addr; i++ )
	{
		seg_encode_table[i] = 0;
	}

	return RBC_SUCCESS;
}

/************/

rbc_read_memory ( frame_num, memory_loc, destination, num_of_chars )
unsigned int frame_num;
unsigned long memory_loc;
char * destination;
int num_of_chars;
{
	int i;

	if ( ((frame_num > MAX_FRAME) && (frame_num != ARCH_FRAME)) )
	{
#ifdef DEBUG
		/* there is no such frame */
		printf ( "frame %d in rbc_read_memory\n", frame_num );
#endif
		return BAD_PARAMETER;
	}

#ifdef EMULATOR
	if ( emulator_emulating )
	{
		printf ( "rbc_read_memory not emulated\n" );

		return RBC_FATAL_ERROR;
	}
#else
	cmd_register->system_commands = SET_TEST;

	cmd_register->set_prefix = frame_num;

	for ( i = 0; i < num_of_chars; i++ )
	{
		if ( memory_loc >= rbc_memory_end ) break;
		*destination++ = *(char *)memory_loc++;
	}
	cmd_register->system_commands = SET_RUN;
	return i;
#endif
}

rbc_write_memory ( frame_num, memory_loc, source, num_of_chars )
unsigned int frame_num;
unsigned long memory_loc;
char * source;
int num_of_chars;
{
	int i;

	if ( ((frame_num > MAX_FRAME) && (frame_num != ARCH_FRAME)) )
	{
#ifdef DEBUG
		/* there is no such frame */
		printf ( "frame %d in rbc_write_memory\n", frame_num );
#endif
		return BAD_PARAMETER;
	}

#ifdef EMULATOR
	if ( emulator_emulating )
	{
		printf ( "rbc_read_memory not emulated\n" );

		return RBC_FATAL_ERROR;
	}
#else
	cmd_register->system_commands = SET_TEST;

	cmd_register->set_prefix = frame_num;

	for ( i = 0; i < num_of_chars; i++ )
	{
		if ( memory_loc >= rbc_memory_end ) break;
		*(char *)memory_loc++ = *source++;
	}
	cmd_register->system_commands = SET_RUN;
	return i;
#endif
}

/*************/

rbc_get_def ( seg_no, start_addr, end_addr )
unsigned int seg_no;
unsigned long *start_addr, *end_addr;
{
	if ( seg_no > MAX_SEG )
	{
#ifdef DEBUG
		printf ( "rbc_get_def: seg_no = %d\n", seg_no );
#endif
		return BAD_PARAMETER;
	}

	*start_addr = seg_def_table[seg_no] & LOW_MASK;

	*end_addr = (seg_def_table[seg_no] & HIGH_MASK) >> HIGH_SHIFT;

	return RBC_SUCCESS;
}

