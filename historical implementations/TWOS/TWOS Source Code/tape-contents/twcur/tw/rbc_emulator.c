/*
 * $Log:	rbc_emulator.c,v $
 * Revision 1.5  91/07/22  13:03:58  steve
 * Wrapped _pprintf with ifdef TWOS.
 * 
 * Revision 1.4  91/06/05  14:08:51  configtw
 * Add Log header for RCS.
 * 
 */

/*
 * Copyright (C) 1991, Integrated Parallel Technology, All Rights Reserved.
 * U. S. Government Sponsorship under SBIR Contract NAS7-1102 is acknowledged.
 */

/* rbc_emulator.c */

#ifdef BF_MACH
#define TWOS
#endif
#ifdef SUN
#define TWOS
#endif
#ifdef Sun4
#define PRE_ALLOC_KILO_COUNT	1500
#else
#define PRE_ALLOC_KILO_COUNT	1000
#endif

#include "rbc_emulator.h"

#include "rbc_public.h"
#include "rbc_private.h"

#define DEBUG

/* kilo_mem kilo_memory[RBC_NUM_KB] but malloced */
kilo_mem * kilo_memory;

typedef struct kilo_block
{
	struct kilo_block * prev, * next;
	int size;
	char storage[1024];
}
kilo_block;

typedef struct
{
	kilo_block * prev, * next;
	int size;			/* also counts num elem's in q */
}
q_head;

q_head * kilo_queue[RBC_NUM_KB];
q_head * free_kilo_list;

/* storage for the above */
q_head	q_storage[RBC_NUM_KB];
q_head	free_q_storage;

init_queues ()
{
	int i;

	for ( i = 0; i < RBC_NUM_KB; i++ )
	{
		kilo_queue[i] = (q_head *) &(q_storage[i]);
		kilo_queue[i]->next = kilo_queue[i]->prev =
			(kilo_block *) kilo_queue[i];
		kilo_queue[i]->size = 0;
	}
	
	free_kilo_list = (q_head *) &free_q_storage;
	free_kilo_list->next = free_kilo_list->prev =
			(kilo_block *) free_kilo_list;
	free_kilo_list->size = 0;

#ifdef TWOS
	/* prime the free list with a bunch of `kilos' */
	{
		kilo_block * p;
		extern char * m_allocate();

		for ( i = RBC_NUM_KB; i < PRE_ALLOC_KILO_COUNT; i++ )
		{
			p = (kilo_block *) m_allocate ( sizeof (kilo_block) );
			if ( p )
			{
				p->size = 1024;
				p->next = p->prev = p;
				free_block ( p );
			}
		}
/*
		printf ( "pre-allocated %d kilo_blocks\n",
			PRE_ALLOC_KILO_COUNT);
*/
	}
#endif
}

free_block ( p )
kilo_block * p;
{
	kilo_block * q;
	
	if ( (!p) || (p->size != 1024) 
		/* || (p != p->next) || (p != p->prev) */ )
	{
		printf ( "free_block free-ing something strange\n" );
		printf ( "p is %x, size is %d\n", p, p->size );
	}
	else
	{
		q = free_kilo_list->next;
		q->prev = p;
		p->next = q;
		free_kilo_list->next = p;
	}
}

int blocks_out = PRE_ALLOC_KILO_COUNT;

kilo_block * new_block ()
{
	kilo_block * p, *q;
	
	p = free_kilo_list->next;
	if ( p->size == 1024 ) /* i.e. p != free_kilo_list */
	{
		q = p->next;
		q->prev = p->prev;
		free_kilo_list->next = q;
		p->next = p->prev = p;
	}
	else
	{
		if ( blocks_out % 100 == 0 )
#ifdef TWOS
		   _pprintf ( "Malloc-ing RBC memory %d blocks\n", blocks_out );
#else
		   printf ( "Malloc-ing RBC memory %d blocks\n", blocks_out );
#endif
		p = (kilo_block *) malloc ( sizeof (kilo_block) );
		if ( p )
		{
			blocks_out++;
			p->size = 1024;
			p->next = p->prev = p;
		}
		/* return null -- rbc_mark will handle it */
	}
	
	return p;
}

#ifndef TWOS
kilo_copy ( dest, src )
char * dest, * src;
{
	int i = 0;
	
	while ( ++i < 1024 )
		*++dest = *++src;
}
#endif

emulate_rollback ()
{
	unsigned int seg_no, num_frames;
	unsigned long begin, end;
	int i, j;
	kilo_block * p, * q;
	
	seg_no = cmd_register->rollback >> SEG_SHIFT;
	num_frames = cmd_register->rollback  &  MAX_FRAME;

	rbc_get_def ( seg_no, &begin, &end );

	if ( num_frames > kilo_queue[begin]->size )
	{
		cmd_register->status = ROLLBACK_MASK;
		return;
	}
	
	for ( i = begin; i <= end; i++ )
	{
		if ( num_frames > kilo_queue[i]->size )
		{
			cmd_register->status = HARDWARE_ERROR;
			return;
		}

		for ( j = 1, p = kilo_queue[i]->prev, q = p->prev;
			; j++, p = q, q = q->prev )
		{
			free_block ( p );
			if ( j >= num_frames )
				break;
		}
#ifdef TWOS
		entcpy ( kilo_memory[i], p->storage, 1024 );
#else
		kilo_copy ( kilo_memory[i], p->storage );
#endif
		
		kilo_queue[i]->prev = q;
		q->next = (kilo_block *) kilo_queue[i];
		
		kilo_queue[i]->size -= num_frames;
	}
		
	cmd_register->status = 0;
	cmd_register->rollback = 0;

	rbc_put_cmf ( seg_no,
	    (rbc_get_cmf ( seg_no ) + NUM_FRAMES - num_frames) % NUM_FRAMES );
}

emulate_advance ()
{
	unsigned int seg_no, num_frames;
	unsigned long begin, end;
	int i, j;
	kilo_block * p, * q;
	
	seg_no = cmd_register->advance >> SEG_SHIFT;
	num_frames = cmd_register->advance  &  MAX_FRAME;

	rbc_get_def ( seg_no, &begin, &end );

	if ( num_frames > kilo_queue[begin]->size )
	{
		cmd_register->status = ADVANCE_MASK;
		return;
	}
	
	for ( i = begin; i <= end; i++ )
	{
		if ( num_frames > kilo_queue[i]->size )
		{
			cmd_register->status = HARDWARE_ERROR;
			return;
		}

		for ( j = 1, p = kilo_queue[i]->next, q = p->next;
			; j++, p = q, q = q->next )
		{
			free_block ( p );
			if ( j >= num_frames )
				break;
		}
		
		kilo_queue[i]->next = q;
		q->prev = (kilo_block *) kilo_queue[i];
		
		kilo_queue[i]->size -= num_frames;
	}
		
	cmd_register->status = 0;
	cmd_register->advance = 0;

	rbc_put_omf ( seg_no,
		(rbc_get_omf ( seg_no ) + num_frames) % NUM_FRAMES );
}

emulate_mark ()
{
	unsigned int seg_no, num_frames;
	unsigned long begin, end;
	int i, j;
	kilo_block * p, * q;
	
	seg_no = (cmd_register->mark) >> SEG_SHIFT;
	num_frames = cmd_register->mark  &  MAX_FRAME;

	rbc_get_def ( seg_no, &begin, &end );
/*
printf ( "emulate_mark seg %d begin %x num_frames %d qued %d\n",
		seg_no, begin, num_frames, kilo_queue[begin]->size );
*/

	if ( num_frames > MAX_FRAME - kilo_queue[begin]->size )
	{
		cmd_register->status = MARK_MASK;
		return;
	}
	
	for ( i = begin; i <= end; i++ )
	{
		if ( num_frames > MAX_FRAME - kilo_queue[i]->size )
		{
			cmd_register->status = HARDWARE_ERROR;
			return;
		}

		for ( j = 0, q = kilo_queue[i]->prev;
			j < num_frames; j++, q = p )
		{
			p = new_block ();
			if ( !p )
			{
				cmd_register->status = HARDWARE_ERROR;
				return;
			}
#ifdef TWOS
			entcpy ( p->storage, kilo_memory[i], 1024 );
#else
			kilo_copy ( p->storage, kilo_memory[i] );
#endif
			q->next = p;
			p->prev = q;
		}
		
		kilo_queue[i]->prev = p;
		p->next = (kilo_block *) kilo_queue[i];
		
		kilo_queue[i]->size += num_frames;
	}
		
	cmd_register->status = 0;
	cmd_register->mark = 0;

	rbc_put_cmf ( seg_no,
		(rbc_get_cmf ( seg_no ) + num_frames) % NUM_FRAMES );
}

emulate_dealloc ( seg_no, begin_addr, end_addr )
unsigned int seg_no;
unsigned long begin_addr, end_addr;
{
	int i, j;
	kilo_block * p, * q;

	for ( i = begin_addr; i <= end_addr; i++ )
	{
		for ( j = 0, p = kilo_queue[i]->next, q = p->next;
			; j++, p = q, q = q->next )
		{
			if ( j >= kilo_queue[i]->size )
				break;
			free_block ( p );
		}
		
		kilo_queue[i]->next = kilo_queue[i]->prev =
			(kilo_block *) kilo_queue[i];
		kilo_queue[i]->size = 0;
	}

	rbc_put_omf ( seg_no, 0 );
	rbc_put_cmf ( seg_no, 0 );
}


/*
 * debugging functions
 */

emulator_check_frame_count ( seg_no, count )
unsigned int seg_no;
int count;
{
	unsigned long begin, end;

	if ( rbc_get_def ( seg_no, &begin, &end ) )
	{
		printf ( "error in emulator_check/rbc_get_def\n" );
		return -1;
	}

	if ( count != kilo_queue[begin]->size + 1 )
	{
		printf ( "state count off %d vs %d seg_no %d\n", count,
			kilo_queue[begin]->size, seg_no );
		return -1;
	}

	end = rbc_get_cmf ( seg_no ) + NUM_FRAMES - rbc_get_omf (seg_no ) + 1;
	end %= NUM_FRAMES;
	if ( end == 0 )
		end = NUM_FRAMES;
	if ( count != end )
	{
		printf ( "counters off %d vs %d\n", count, end );
		printf ( "Current is %d Oldest is %d\n", rbc_get_cmf ( seg_no ),
			rbc_get_omf ( seg_no ) );
		return -1;
	}
	return 0;
}

print_q_sizes()
{
	int i;

	for ( i = 0; i < rbc_num_KB; i += 8 )
		printf ( "queue %3d sizes: %2d %2d %2d %2d   %2d %2d %2d %2d\n",
			i, kilo_queue[i]->size, kilo_queue[i + 1]->size,
			kilo_queue[i + 2]->size, kilo_queue[i + 3]->size,
			kilo_queue[i + 4]->size, kilo_queue[i + 5]->size,
			kilo_queue[i + 6]->size, kilo_queue[i + 7]->size );
}
