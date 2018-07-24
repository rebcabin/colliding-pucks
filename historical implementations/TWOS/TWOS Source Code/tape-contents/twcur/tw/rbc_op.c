/*
 * $Log:	rbc_op.c,v $
 * Revision 1.3  91/06/03  12:26:23  configtw
 * Tab conversion.
 * 
 * Revision 1.2  91/03/28  09:58:01  configtw
 * Add RBC change for Steve.
 * 
 * Revision 1.1  91/03/26  10:34:26  pls
 * Initial revision
 *
*/

/*
 * Copyright (C) 1991, Integrated Parallel Technology, All Rights Reserved.
 * U. S. Government Sponsorship under SBIR Contract NAS7-1102 is acknowledged.
 */

/* rbc_op.c */

#include "twcommon.h"
#include "twsys.h"
#include "rbc_public.h"
/*
#define PARANOID
*/
int rbc_present;

rollback_op ( ocb, num_frames, rb_time )
Ocb * ocb;
int num_frames;
VTime rb_time;
{
	int rv;
	int i;
	FootCB * p, * q;

	/* repeat until 0 return or error */
	while ( rv = rbc_rollback ( ocb->first_seg, num_frames ) )
		if ( rv != ABORTED_COMMAND )
		{
			_pprintf ( "Error in rollback_op rv is %d\n", rv );
			tester();
		}

	/* rbc_rollback successful do book keeping */
	ocb->frames_used -= num_frames;

	/* loops immediately exit if there are no dynamic state pieces */

	/* pass one update rollback chip */
	for ( p = ocb->fcb; p; p = p->next )
	{
		for ( i = p->first_used_segment; i < SEG_PER_CB; i++ )
		{
			if ( p->seg[i].status == SEG_FREE )
			{
				continue;
			}

			/* breaks if ok */
			while ( rv = rbc_rollback ( p->seg[i].seg_no, num_frames ) )
				if ( rv != ABORTED_COMMAND )
				{
					_pprintf (
						"rollback_op rv = %d for %s p = %x, i = %d, seg = %d\n",
						rv, ocb->name, p, i, p->seg[i].seg_no );
					tester();
				}

			if ( (p->seg[i].status & SEG_FUTURE_FREE) &&
				leVTime ( rb_time, p->seg[i].last_used ) )
			{
				p->seg[i].status &= ~SEG_FUTURE_FREE;
			}
		}
	}

	/* pass two free rolled-over segments */
	for ( p = ocb->fcb; p; p = q )
	{
		int rolled_over_checking_done = 0;

		q = p->next; /* in case p is destroyed */

		for ( i = p->first_used_segment; i < SEG_PER_CB; )
		{
			if ( p->seg[i].status == SEG_FREE )
			{
				i++;
			}
			else if ( leVTime ( rb_time,  p->seg[i].first_alloc ) )
			{
/*
_pprintf ( "rollback freeing seg %d\n", p->seg[i].seg_no );
*/
				p->seg[i].status = SEG_FREE;
				free_segment ( p->seg[i].seg_no );
				i++;
			}
			else /* the rest can't be rolled over */
			{
				rolled_over_checking_done = 1;
				p->first_used_segment = i;
				break;
			}
		}

		if ( SEG_PER_CB == i ) /* all of p is empty */
		{
/*
check_empty_footCB ( p );
*/
			l_destroy ( p );
			ocb->fcb = q;
		}

		if ( rolled_over_checking_done )
			break;
	}
#ifdef PARANOID
	checkOcbRBC ( ocb, "rollback" );
#endif
	return SUCCESS;
}

advance_op ( ocb, num_frames, rb_time )
Ocb * ocb;
int num_frames;
VTime rb_time; /* should always be gvt ? */
{
	int rv;
	int i;
	FootCB * p, * q;

	/* repeat until 0 return or error */
	while ( rv = rbc_advance ( ocb->first_seg, num_frames ) )
		if ( rv != ABORTED_COMMAND )
		{
			_pprintf ( "Error in advance_op rv is %d\n", rv );
			tester();
			return;
		}


	/* rbc_advance successful do book keeping */
	ocb->frames_used -= num_frames;


	/* loops immediately exit if there are no dynamic state pieces */

	/* pass one update rollback chip */
	for ( p = ocb->fcb; p; p = p->next )
	{
		for ( i = p->first_used_segment; i < SEG_PER_CB; i++ )
		{
			if ( p->seg[i].status == SEG_FREE )
			{
				continue;
			}

			/* breaks if ok */
			while ( rv = rbc_advance ( p->seg[i].seg_no, num_frames ) )
				if ( rv != ABORTED_COMMAND )
				{
					_pprintf (
						"advance_op rv = %d for %s p = %x, i = %d, seg = %d\n",
						rv, ocb->name, p, i, p->seg[i].seg_no );
					tester();
				}

			if ( (p->seg[i].status & SEG_FUTURE_FREE) &&
				gtVTime ( rb_time, p->seg[i].last_used ) )
			{
/*
_pprintf ( "advance freeing seg %d\n", p->seg[i].seg_no );
*/
				p->seg[i].status = SEG_FREE;
				free_segment ( p->seg[i].seg_no );
			}
		}

		/* make p->first_used_segment correct,(used in grab-collect) */
		for ( i = p->first_used_segment; ; i++ )
		{
			if ( ( SEG_PER_CB == i ) || ( p->seg[i].status != SEG_FREE ) )
			{
				p->first_used_segment = i;
				break;
			}
		}
	}

	/* delete empty foot cb's at front */
	while ( (p = ocb->fcb) && (p->first_used_segment == SEG_PER_CB) )
	{
		ocb->fcb = p->next;
		l_destroy ( p );
	}

	/* delete empty foot cb's past the front */
	for ( p = ocb->fcb; p && (q = p->next); )
	{
		if ( q->first_used_segment == SEG_PER_CB )
		{
			p->next = q->next;
			l_destroy ( q );
		}
		else
		{
			p = q;
		}
	}
#ifdef PARANOID
	checkOcbRBC ( ocb, "advance" );
#endif
	return SUCCESS;
}

mark_op ( ocb, num_frames, rb_time )
Ocb * ocb;
int num_frames; /* should always be one ? */
VTime rb_time; /* doesn't matter ? */
{
	int rv;
	int i;
	FootCB * p, * q;

	/* repeat until 0 return or error */
	while ( rv = rbc_mark ( ocb->first_seg, num_frames ) )
	{
		if ( rv = MARK_OVERFLOW )
			return FAILURE;

		if ( rv != ABORTED_COMMAND )
		{
			_pprintf ( "Error in mark_op rv is %d\n", rv );
			tester();
		}
	}

	/* rbc_mark successful do book keeping */
	ocb->frames_used += num_frames;
	ocb->last_chance = NULL;

	/* for loop immediately exits if there are no dynamic state pieces */
	for ( p = ocb->fcb; p; p = p->next )
	{
		/* update rollback chip */
		for ( i = p->first_used_segment; i < SEG_PER_CB; i++ )
		{
			if ( p->seg[i].status == SEG_FREE )
			{
				continue;
			}

			/* breaks if ok */
			while ( rv = rbc_mark ( p->seg[i].seg_no, num_frames ) )
				if ( rv != ABORTED_COMMAND )
				{
					_pprintf (
						"mark_op rv = %d for %s p = %x, i = %d, seg = %d\n",
						rv, ocb->name, p, i, p->seg[i].seg_no );
					tester();
				}

		}
	}
#ifdef PARANOID
	checkOcbRBC ( ocb, "mark" );
/*
	print_ocb_rbc ( ocb );
*/
#endif


	return SUCCESS;
}

/*--------------------------------------------------------------------------*
   Clean Up
*--------------------------------------------------------------------------*/
term_op ( ocb )
Ocb *ocb;
{
	int rv;
	int wait = TRUE;
	unsigned long begin, end;

	/* this function should delete all the other segments (if any) */
	rbc_get_def ( ocb->first_seg, &begin, &end );

	ocb->frames_used--;

	if ( (rv = rbc_dealloc_start ( ocb->first_seg, begin, end ) ) ||
		(rv = rbc_dealloc_done ( ocb->first_seg, begin, end, wait ) ) )
	{
		_pprintf ( "term_op returned %d\n", rv );
		tester();
	}
	ocb->first_seg = -1;
	ocb->footer = NULL;
	ocb->fcb = NULL;
	/* ocb->frames_used = 0;*/
	ocb->uses_rbc = FALSE;
}

/*--------------------------------------------------------------------------*
   Debugging Functions
*--------------------------------------------------------------------------*/
/* this function is used only for debugging */
rbc_check_frame_count ( ocb, count )
Ocb *ocb;
int count;
{
	if ( ocb->runstat == ITS_STDOUT )
		return 0;

	if ( ocb->sb ) /* one more state not in the queue */
		count++;

	if ( count != ocb->frames_used )
	{
		_pprintf ( "Echeck: ocb %s count = %d, frames = %d\n",
			ocb->name, count, ocb->frames_used );
		tester();
	}

	if ( ocb->frames_used  &&
		emulator_check_frame_count ( ocb->first_seg, count ) )
	{
		_pprintf ( "That was object: %s\n", ocb->name );
		tester();
		return -1;
	}
	return 0;
}

check_footCB ( p, ocb, string )
FootCB *p;
Ocb * ocb;
char * string;
{
	int i;
	int count = ocb->frames_used;
	int status;
	State * s = fststate_macro (ocb);
	VTime garbTime;

	garbTime = s->sndtim;

	for ( i = 0; i < SEG_PER_CB; i++ )
	{
		if ( status = p->seg[i].status )
		{
			if ( count && emulator_check_frame_count(p->seg[i].seg_no, count) )
			{
				_pprintf ( "check_fcb: counters: object: %s @ %s\n",
					ocb->name, string );
				tester();
			}
			if ( ltVTime ( ocb->svt, p->seg[i].first_alloc ) )
			{
				_pprintf ( "check_fcb: rollback: object: %s @ %s\n",
					ocb->name, string );
				tester();
			}
			if ( (status & SEG_FUTURE_FREE) &&
				ltVTime ( p->seg[i].last_used, garbTime ) )
			{
				_pprintf ( "check_fcb: advance: object: %s @ %s\n",
					ocb->name, string );
				tester();
			}
		}
	}
}

check_empty_footCB ( p )
FootCB *p;
{
	int i;

	for ( i = 0; i < SEG_PER_CB; i++ )
	{
		if ( p->seg[i].status )
		{
			_pprintf ( "FCB is not free 0x%x->seg[%d]\n", p, i );
			tester();
		}
	}
}

checkOcbRBC ( ocb, string )
Ocb *ocb;
char * string;
{
	State *s;
	int count;
	FootCB *p;

	count = 0;
	for (s = fststate_macro (ocb); s; s = nxtstate_macro (s))
	{
		count++;
	}

	rbc_check_frame_count ( ocb, count );

	for ( p = ocb->fcb; p; p = p->next )
		check_footCB ( p, ocb, string );
}

print_footCB ( p )
FootCB *p;
{
	int i;

	for ( i = p->first_used_segment; i < SEG_PER_CB; i++ )
	{
		printf ( "%d: seg: %d status %d firstVT %lf lastVT %lf\n",
			i, p->seg[i].seg_no, p->seg[i].status,
			p->seg[i].first_alloc.simtime,
			p->seg[i].last_used.simtime );
		print_frame_counter ( p->seg[i].seg_no );
	}
}

print_ocb_rbc ( ocb )
Ocb * ocb;
{
	FootCB *p;

	printf ( "rbc dump for %s 0x%x\n", ocb->name, ocb );
	if ( ! ocb->uses_rbc )
	{
		printf ( "ocb doesn't use rbc\n" );
		return;
	}
	printf ( "1st seg %d, frames_used %d, footer 0x%x\n",
		ocb->first_seg, ocb->frames_used, ocb->footer );
	print_frame_counter ( ocb->first_seg );
	for ( p = ocb->fcb; p ; p = p->next )
		print_footCB ( p );
}
