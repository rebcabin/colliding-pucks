/*
 * $Log:	rbc_debug.c,v $
 * Revision 1.1  91/03/26  10:32:00  pls
 * Initial revision
 * 
*/

/*
 * Copyright (C) 1991, Integrated Parallel Technology, All Rights Reserved.
 * U. S. Government Sponsorship under SBIR Contract NAS7-1102 is acknowledged.
 */

#include "rbc_public.h"
#include "rbc_private.h"

#define DEBUG

unsigned long rbc_status()
{
	return cmd_register->status;
}

unsigned long rbc_seg_status( seg_no )
unsigned int seg_no;
{
	unsigned long status;

	cmd_register->system_commands = SET_RUN | SET_SID_STATUS;

	/* no way to return bad seg number */
	cmd_register->status = (seg_no & MAX_SEG) << SEG_SHIFT;

	status = cmd_register->status;

	cmd_register->system_commands = SET_RUN; /* clear SID */

	return status;
}

#if 0
/* in drivers now */
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
		(CLRING_WB_SEG |ADV_N_PROGRESS |TAG_N_PROGRESS |RB_N_PROGRESS) )
/*?? should we check for advance in progress etc?? */
	{
		/* wait */
		/* maybe sleep here if we are talking over the switch 
			on the butterfly */
	}

	cmd_register->system_commands = SET_RUN; /* clear SID */

	return RBC_SUCCESS;
}
#endif

#if 0
/* currrently in drivers */
rbc_get_def ( seg_no, start_addr, end_addr )
unsigned int seg_no;
unsigned long *start_addr, *end_addr;
{
	if ( seg_no > MAX_SEG )
	{
#ifdef DEBUG
		printf ( "rbc_get_def: seg_no %d\n", seg_no );
#endif
		return BAD_PARAMETER;
	}

	*start_addr = seg_def_table[seg_no] & LOW_MASK;

	*end_addr = (seg_def_table[seg_no] & HIGH_MASK) >> HIGH_SHIFT;

	return RBC_SUCCESS;
}
#endif

rbc_put_def ( seg_no, start_addr, end_addr )
unsigned int seg_no;
unsigned long start_addr, end_addr;
{
	if ( (seg_no > MAX_SEG) || (end_addr < start_addr) 
		|| (rbc_num_KB <= end_addr) )
	{
#ifdef DEBUG
		printf ( "rbc_put_def seg %d start %d end %d\n",
			seg_no, start_addr, end_addr );
#endif
		return BAD_PARAMETER;
	}

	seg_def_table[seg_no] = ((end_addr & LOW_MASK) >> HIGH_SHIFT ) +
		(start_addr & LOW_MASK);

	return RBC_SUCCESS;
}

unsigned int rbc_get_encode ( address )
unsigned long address;
{
	if ( address >= rbc_num_KB )
	{
#ifdef DEBUG
		printf ( "rbc_get_encode: address %d\n", address );
#endif
		return BAD_PARAMETER;
	}

	return seg_encode_table[address] & ENCODE_MASK;
}

unsigned int rbc_put_encode ( address, seg_no )
unsigned long address;
unsigned int seg_no;
{
	if ( (seg_no > MAX_SEG) || (address >= rbc_num_KB) )
	{
#ifdef DEBUG
		printf ( "rbc_put_encode: seg_no %d address %d\n",
			seg_no, address );
#endif
		return BAD_PARAMETER;
	}

	seg_encode_table[address] = seg_no;

	return RBC_SUCCESS;
}

/*************/

rbc_get_omf ( seg_no )
unsigned int seg_no;
{
	if ( seg_no > MAX_SEG )
	{
#ifdef DEBUG
		printf ( "rbc_get_omf: seg_no %d\n", seg_no );
#endif
		return BAD_PARAMETER;
	}

	return (seg_frame_counters[seg_no] & OFC_MASK) >> OFC_SHIFT;
}

rbc_put_omf ( seg_no, new_frame )
unsigned int seg_no, new_frame;
{
	unsigned long temp;

	if ( (seg_no > MAX_SEG) || (new_frame > MAX_FRAME) )
	{
#ifdef DEBUG
		printf ( "rbc_put_omf: seg_no %d new_frame %d\n",
			seg_no, new_frame );
#endif
		return BAD_PARAMETER;
	}

	temp = seg_frame_counters[seg_no] & CFC_MASK;
	seg_frame_counters[seg_no] = temp + (new_frame << OFC_SHIFT);

	return RBC_SUCCESS;
}


rbc_get_cmf ( seg_no )
unsigned int seg_no;
{
	if ( seg_no > MAX_SEG )
	{
#ifdef DEBUG
		printf ( "rbc_get_cmf: seg_no %d\n", seg_no );
#endif
		return BAD_PARAMETER;
	}

	return seg_frame_counters[seg_no] & CFC_MASK;
}

rbc_put_cmf ( seg_no, new_frame )
unsigned int seg_no, new_frame;
{
	unsigned long temp;

	if ( (seg_no > MAX_SEG) || (new_frame > MAX_FRAME) )
	{
#ifdef DEBUG
		printf ( "rbc_put_cmf: seg_no %d new_frame %d\n",
			seg_no, new_frame );
#endif
		return BAD_PARAMETER;
	}

	temp = seg_frame_counters[seg_no] & OFC_MASK;
	seg_frame_counters[seg_no] = temp + new_frame;

	return RBC_SUCCESS;
}

rbc_get_wb ( address, wb_low, wb_high )
unsigned long address, *wb_low, *wb_high;
{
	unsigned long i = address - rbc_memory_start;

	if ( (address < rbc_memory_start) || (i >= rbc_memory_end) )
	{
#ifdef DEBUG
		printf ( "get_wb address out of range 0x%x\n", address );
#endif
		return BAD_PARAMETER;
	}

	cmd_register->set_wb_address = i;
	*wb_low = cmd_register->read_wb;

	cmd_register->set_wb_address = i | WB_MSB;
	*wb_high = cmd_register->read_wb;

	return RBC_SUCCESS;
}

/*************/


/* debug */

/* print anything and everything */

#include <stdio.h>
static FILE * fp = stderr;


/* general format
	fprintf ( fp, "error message value\n", pars );
*/


void print_seg_encode_table ()
{
	int i;

	fprintf ( fp, "Encoder Table Dump\n" );

	for ( i = 0; i < rbc_num_KB; i += 8 )
		fprintf ( fp,
			"%4dK:  %3d  %3d  %3d  %3d    %3d  %3d  %3d  %3d\n",
			i, 
			seg_encode_table[i] & ENCODE_MASK,
			seg_encode_table[i+1] & ENCODE_MASK,
			seg_encode_table[i+2] & ENCODE_MASK,
			seg_encode_table[i+3] & ENCODE_MASK,
			seg_encode_table[i+4] & ENCODE_MASK,
			seg_encode_table[i+5] & ENCODE_MASK,
			seg_encode_table[i+6] & ENCODE_MASK,
			seg_encode_table[i+7] & ENCODE_MASK );
}

void print_seg_encode ( kilobyte )
unsigned int kilobyte;
{
	if ( kilobyte > rbc_num_KB )
	{
		fprintf ( fp, "bad kilobyte %d\n", kilobyte );
		return;
	}
	fprintf ( fp, "%dK is in segment %d\n", kilobyte,
		seg_encode_table[kilobyte] & ENCODE_MASK );
}

void print_seg_def_table ()
{
	int i;

	fprintf ( fp, "Definition Table Dump\n" );

	for ( i = 0; i < NUM_SEGS; i += 4 )
		fprintf ( fp,
		"%3d:%4d-%4dK  %3d:%4d-%4dK %3d:%4d-%4dK %3d:%4d-%4dK\n",
			i, seg_def_table[i] & LOW_MASK,
			(seg_def_table[i] & HIGH_MASK) >> HIGH_SHIFT,
			i+1, seg_def_table[i+1] & LOW_MASK,
			(seg_def_table[i+1] & HIGH_MASK) >> HIGH_SHIFT,
			i+2, seg_def_table[i+2] & LOW_MASK,
			(seg_def_table[i+2] & HIGH_MASK) >> HIGH_SHIFT,
			i+3, seg_def_table[i+3] & LOW_MASK,
			(seg_def_table[i+3] & HIGH_MASK) >> HIGH_SHIFT );
}

void print_seg_def ( seg_no )
unsigned int seg_no;
{
	if ( seg_no > MAX_SEG )
	{
		fprintf ( fp, "bad seg_no %d\n", seg_no );
		return;
	}

	fprintf ( fp, "seg %d starts %d and ends %d\n", seg_no,
		seg_def_table[seg_no] & LOW_MASK,
		(seg_def_table[seg_no] & HIGH_MASK) >> HIGH_SHIFT );
}

void print_wb_seg ( seg_no )
unsigned int seg_no;
{
	unsigned long i;
	unsigned long seg_start, seg_end;
	unsigned long lsb, msb;

	if ( seg_no > MAX_SEG )
	{
		fprintf ( fp, "print_wb_seg_no given seg %d\n", seg_no );
		return;
	}

	fprintf ( fp, "Dumping Written Bit Memory for seg %d\n", seg_no );

	seg_start = (seg_def_table[seg_no] & LOW_MASK) << 10;
	seg_end = (seg_def_table[seg_no] & HIGH_MASK) >> HIGH_SHIFT;
	seg_end++;
	seg_end <<= 10;

	for ( i = seg_start; i < seg_end; i += 64 ) /* every 64 bits??? */
	{
		cmd_register->set_wb_address = i;
		lsb = cmd_register->read_wb;

		cmd_register->set_wb_address = i | WB_MSB;
		msb = cmd_register->read_wb;

		fprintf ( fp, "%6x %8x %8x\n", i, lsb, msb );
	}
}

void print_wb ( address )
unsigned long address;
{
	unsigned long lsb, msb;
	unsigned long i = address - rbc_memory_start;

	cmd_register->set_wb_address = i;
	lsb = cmd_register->read_wb;

	cmd_register->set_wb_address = i | WB_MSB;
	msb = cmd_register->read_wb;

	fprintf ( fp, "address 0x%x = relative 0x%x: lsb = 0x%x  msb = 0x%x\n",
		address, i, lsb, msb );
}

void print_frame_counter_table ()
{
	int i;

	fprintf ( fp, "Counter Table Dump\n" );

	for ( i = 0; i < NUM_SEGS; i += 4 )
		fprintf ( fp,
"%3d:%2d O %2d C %3d:%2d O %2d C   %3d:%2d O %2d C %3d:%2d O %2d C\n",
			i, (seg_frame_counters[i] & OFC_MASK) >> OFC_SHIFT,
			seg_frame_counters[i] & CFC_MASK,
			i + 1,
			(seg_frame_counters[i + 1] & OFC_MASK) >> OFC_SHIFT,
			seg_frame_counters[i + 1] & CFC_MASK,
			i + 2,
			(seg_frame_counters[i + 2] & OFC_MASK) >> OFC_SHIFT,
			seg_frame_counters[i + 2] & CFC_MASK,
			i + 3,
			(seg_frame_counters[i + 3] & OFC_MASK) >> OFC_SHIFT,
			seg_frame_counters[i + 3] & CFC_MASK );
}

void print_frame_counter ( seg_no )
unsigned int seg_no;
{
	if ( seg_no > MAX_SEG )
	{
		fprintf ( fp, "bad seg_no %d\n", seg_no );
		return;
	}

	fprintf ( fp, "seg %d -- OFC %d  CFC %d\n", seg_no,
		(seg_frame_counters[seg_no] & OFC_MASK) >> OFC_SHIFT,
		seg_frame_counters[seg_no] & CFC_MASK );
}

void status_in_english ( status )
unsigned long status;
{
	if ( status & COMMAND_ABORT )
		fprintf ( fp, "	command aborted\n" );
	if ( status & HARDWARE_ERROR )
		fprintf ( fp, "	hardward error\n" );
	if ( status & ADVANCE_MASK )
		fprintf ( fp, "	advance overflow\n" );
	if ( status & ROLLBACK_MASK )
		fprintf ( fp, "	rollback overflow\n" );
	if ( status & MARK_MASK	 )
		fprintf ( fp, "	mark overflow\n" );
	if ( status & TAG_N_PROGRESS )
		fprintf ( fp, "	tag update in progress\n" );
	if ( status & RB_N_PROGRESS )
		fprintf ( fp, "	rollback in progress\n" );
	if ( status & ADV_N_PROGRESS )
		fprintf ( fp, "	advance in progress\n" );
	if ( status & CLRING_WB_ALL )
		fprintf ( fp, "	clearing all written bits\n" );
	if ( status & CLRING_MFC_ALL )
		fprintf ( fp, "	clearing all frame counters\n" );
	if ( status & CLRING_WB_SEG )
		fprintf ( fp, "	clearing segments written bits\n" );
	if ( status & CLRING_WB_TABLES )
		fprintf ( fp, "	clearing written bit tables\n" );
	fprintf ( fp, "end status in english\n" );
}

void print_status ()
{
	fprintf ( fp, "rbc status: 0x%x\n", cmd_register->status );
	status_in_english ( cmd_register->status );
}

void print_seg_status ( seg_no )
unsigned int seg_no;
{
	if ( seg_no > MAX_SEG )
	{
		fprintf ( fp, "bad seg_no %d\n", seg_no );
		return;
	}
	cmd_register->system_commands = SET_RUN | SET_SID_STATUS;

	cmd_register->status = seg_no << SEG_SHIFT;

	fprintf ( fp, "rbc seg %d status 0x%x\n",
		seg_no, cmd_register->status );

	status_in_english ( cmd_register->status );

	cmd_register->system_commands = SET_RUN; /*clear SID*/
}

set_print_file ( filename )
char * filename;
{
	FILE * temp;

	temp = fopen ( filename, "w" );

	if ( ! temp )
	{
		fprintf ( fp, "Unable to open %s\n", filename );
		return -1;
	}

	fprintf ( fp, "output transfered to %s\n", filename );

	fp = temp; 
	return 0;
}
