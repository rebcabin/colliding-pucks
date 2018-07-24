/*
 * $Log:	rbc_malloc.c,v $
 * Revision 1.3  91/06/03  12:26:19  configtw
 * Tab conversion.
 * 
 * Revision 1.2  91/03/28  09:55:55  configtw
 * Add RBC change for Steve.
 * 
 * Revision 1.1  91/03/26  10:34:15  pls
 * Initial revision
 *
*/

/*
 * Copyright (C) 1991, Integrated Parallel Technology, All Rights Reserved.
 * U. S. Government Sponsorship under SBIR Contract NAS7-1102 is acknowledged.
 */

#include "rbc_public.h"
#include "twcommon.h"
#include "twsys.h"

/*--------------------------------------------------------------------------*
   Memory Block Header -- assumes we are living in a 32 bit world
*--------------------------------------------------------------------------*/
typedef union uName
{
	struct
	{
		union uName * next; /* next free block-zero when allocated */
		union uName * prev; /* prev free block-zero when allocated */
		int size;           /* for list data when allocated */
		int MemSize;        /* size of this free block in MemBlockHdr's */
	} s;

	double alignment_help;  /* so (double *)rbc_malloc works on sparc */

} MemBlockHdr;

/*--------------------------------------------------------------------------*
   Main Segment Header -- for the first segment allocated for an object
*--------------------------------------------------------------------------*/
typedef struct
{
		MemBlockHdr  * last_alloc;
		MemBlockHdr  * last_release;
		Ocb * ocb;              /* two more fields to make it the same size */
		int     footer_size;    /* as a MemBlockHdr */
} MainSegHdr;

/*--------------------------------------------------------------------------*
   Malloc Segment Header -- for the other segments allocated for an object
*--------------------------------------------------------------------------*/
typedef struct
{
		Ocb * ocb;
		SegmentCB * scb;
		int footCB_offset;
		int magic_size;
/* exactly twice MemBlockHdr */
		MemBlockHdr * next; /* next free block, zero when allocated */
		MemBlockHdr * prev; /* prev free block, zero when allocated */
		int size;           /* for list data when allocated */
		int MemSize;        /* size of this free block in MemBlockHdr's */
#define MALLOC_HEADER -1
} MallocSegHdr;


#define UNITSIZE sizeof(MemBlockHdr)
#define NULL 0

/* size to create a new segment is SCALE times the current request */
#define SCALE 10
#define KILO_SIZE 1024          /* Number of bytes in a kilobyte */

/*--------------------------------------------------------------------------*
   Kilobyte Controls -- Status of all Kilobytes in RBC
*--------------------------------------------------------------------------*/
#define MAX_KILO 1024

short kilo_status[MAX_KILO];
#define KILO_ALLOCATED 0
#define NOT_ENUFF_KILOS -1

int last_kilo;

int kilo_table_initialized;

/*--------------------------------------------------------------------------*
   Segment Controls -- Status of all Segments in RBC
*--------------------------------------------------------------------------*/
char seg_status[NUM_SEGS];
#define STATUS_FREE 0
#define STATUS_RETIRING 1
#define STATUS_BUSY 2

#define NO_MORE_SEGMENTS -1

int last_seg = 1; /* skip zero for debugging */
int last_reclaim = 1; /* skip zero for debugging */

/*--------------------------------------------------------------------------*
   Kilobyte Control Functions
*--------------------------------------------------------------------------*/
kilo_alloc ( size_in_K )
int size_in_K;
{
	int i, j;

	/* one time initialization */
	if ( ! kilo_table_initialized )
	{
		kilo_table_initialized = TRUE;
		for ( i = 0, j = rbc_num_KB; i < rbc_num_KB; i++, j-- )
			kilo_status[i] = j;
		for ( ; i < MAX_KILO; i++ )
			kilo_status[i] = KILO_ALLOCATED;
	}

	for ( i = last_kilo; ; )
	{
		while ( kilo_status[i] == KILO_ALLOCATED )
		{
			i++;
			i = (i >= rbc_num_KB) ? 0 : i;
			if ( i == last_kilo ) /* wrap around */
			{
				if ( reclaim_kilos() ) /* zero on success, so failure case */
				{
					_pprintf ( "out_of_kilos\n" );
					return NOT_ENUFF_KILOS;
				}
			}
		}

		if ( size_in_K <= kilo_status[i] ) /* success */
		{
			last_kilo = i;
			for ( j = 0; j < size_in_K; j++ )
				kilo_status[last_kilo++] = KILO_ALLOCATED;
			last_kilo = (last_kilo >= rbc_num_KB) ? 0 : last_kilo;
			return i;
		}
		else
		{
			i += kilo_status[i];
			i = (i >= rbc_num_KB) ? 0 : i;
		}
	}
}

kilo_dealloc ( start, end )
unsigned long start, end;
{
	int i, j;

	i = end;
	j = i + 1;
	j = ( j >= rbc_num_KB ) ? 1 : kilo_status[j] + 1;

	for ( ; i >= start; )
	{
		kilo_status[i--] = j++;
	}

	for ( i--; i >= 0; )
	{
		if ( kilo_status[i] ) /* un-alloc-ed */
		{
			kilo_status[i--] = ++j;
		}
		else
			break;
	}

	for ( i = last_kilo - 1; i >= 0; )
	{
		if ( kilo_status[i] )
		{
			last_kilo = i--;
		}
		else
		{
			break;
		}
	}
	return RBC_SUCCESS;
}

reclaim_kilos ()
{
	int i, j;
	unsigned long start_addr, end_addr;

	i = last_reclaim;
	for ( j = 0; j < MAX_SEG; j++ )
	{
		if ( seg_status[i] == STATUS_RETIRING )
		{
			if ( rbc_get_def ( i, &start_addr, &end_addr ) ||
				rbc_dealloc_done ( i, start_addr, end_addr, TRUE ) ||
				kilo_dealloc ( start_addr, end_addr ) )
			{
				_pprintf ("Strange error in reclaim_kilos\n" );
				tester();
			}
			seg_status[i] = STATUS_FREE;
			last_reclaim = i;
			return RBC_SUCCESS;
		}
		i++;
		if ( i == NUM_SEGS ) i = 1; /* skip zero for debugging */
	}

	_pprintf ( "rbc out of segments too reclaim\n" );
	return NOT_ENUFF_KILOS; /* no more segment too reclaim */
}

/*--------------------------------------------------------------------------*
   Segment Control Functions
*--------------------------------------------------------------------------*/
new_segment ()
{
	int i = last_seg, j;
	unsigned long start_addr, end_addr;

	for ( j = 0; j < MAX_SEG; j++ )
	{
		if ( seg_status[i] == STATUS_FREE )
		{
			seg_status[i] = STATUS_BUSY;
			last_seg = i;
			return i;
		}
		i++;
		if ( i == NUM_SEGS ) i = 1; /* skip zero for debugging */
	}

	i = last_seg;
	for ( j = 0; j < MAX_SEG; j++ )
	{
		if ( seg_status[i] == STATUS_RETIRING )
		{
			if ( rbc_get_def ( i, &start_addr, &end_addr ) ||
				rbc_dealloc_done ( i, start_addr, end_addr, TRUE ) ||
				kilo_dealloc ( start_addr, end_addr ) )
			{
				_pprintf ("Strange error in new_segment\n" );
				tester();
			}
			seg_status[i] = STATUS_BUSY;
			last_seg = i;
			return i;
		}
		i++;
		if ( i == NUM_SEGS ) i = 1; /* skip zero for debugging */
	}

	_pprintf ( "rbc out of segments\n" );
	return NO_MORE_SEGMENTS; /* no more segment numbers */
}

free_segment ( seg_no )
int seg_no;
{
	int rv;
	unsigned long start_addr, end_addr;
	MallocSegHdr * top;
/*
_pprintf ( "freeing seg %d\n", seg_no );
*/
	if ( seg_status[seg_no] != STATUS_BUSY )
	{
		_pprintf ( "free_segment freeing non-busy seg %d\n", seg_no );
		tester ();
	}

	if ( rbc_get_def ( seg_no, &start_addr, &end_addr ) ||
		rbc_dealloc_start ( seg_no, start_addr, end_addr ) )
	{
		_pprintf ( "error in free_segment\n" );
		tester ();
	}

	seg_status[seg_no] = STATUS_RETIRING;
}

/*--------------------------------------------------------------------------*
   Main Rollback Chip Malloc and Free
*--------------------------------------------------------------------------*/
char * rbc_malloc ( ocb, n )
Ocb * ocb;
int n; /* in bytes */
{
	MemBlockHdr *p, *q, *r, *last_alloc, *last_release, *old_prev;
	int nunits;
	int old_size;
/*
printf ( "rbc_malloc: %s, %d bytes\n", ocb->name, n );
*/
	last_alloc = ( (MainSegHdr *) (ocb->footer) - 1 )->last_alloc;
	last_release = ( (MainSegHdr *) (ocb->footer) - 1 )->last_release;

	q = last_alloc;
	if (q == NULL)
	{                           /* create the free list first time through */
		printf ( "error in rbc_malloc\n" );
		tester ();
	}

	/* Round request up to multiple of a header... */
	nunits =   (n + UNITSIZE - 1) / UNITSIZE;
	/* plus room for one header */
	nunits += 1;

	/* Hunt through the free list for memory, allocating from MI as needed */
	/* q = last_alloc already, start with the block in the free chain after */
	/* the block at q */

	/* First time through, q points to base and so does q->s.next */
	/* q is last_alloc */

	for (p = q->s.next;; q = p, p = p->s.next)
	{
		if (p->s.MemSize >= nunits) /* Found first fit */
		{
			r = p;
			old_size = p->s.MemSize;
			old_prev = r->s.prev;
			if (p->s.MemSize == nunits) /* Exact fit */
			{           
				q->s.next = p->s.next;
				p->s.next->s.prev = q;

				if ( last_release == p )
						( (MainSegHdr *) (ocb->footer) - 1 )->last_release = q;
			}
			else /* Room left over */
			{   
				p->s.MemSize -= nunits;
				p += p->s.MemSize;
				p->s.MemSize = nunits;
			}

			p->s.next = p->s.prev = 0; /* allocated block */
			p->s.size = n; /* for list stuff */

			/* next time, start searching after here */
			( (MainSegHdr *) (ocb->footer) - 1 )->last_alloc = q;

			return  (char *)  (p + 1);  /* beyond header */
		}

		/* wrapped around end of free list */
		/* First time through, we get to here */
		if (p == last_alloc)
		{
			if ( !moreKilos ( ocb, nunits ) )
			{
				return NULL;
			}
		}
	}
}

moreKilos ( ocb, num_units )
Ocb * ocb;
int num_units;  /* in MemBlockHdr's */
{
	MemBlockHdr *hp;
	MallocSegHdr * top;
	int seg_no;
	int kilos_needed;
	int start_kilo;
	int rv;
	FootCB *new_fcb = 0;

	/* second chance stuff */
	top = (MallocSegHdr *)(ocb->last_chance);
/*
	if ( top )
	{
		_pprintf ( "kilos needed non-Null top: %d need %d top status %d\n",
			top->magic_size, num_units, top->scb->status );
		_pprintf ( "vtimes %lf seg vs %lf ocb\n", top->scb->last_used.simtime,
			ocb->svt.simtime );
	}
*/
	if ( top && (top->magic_size > num_units ) &&
		/* debugging checks */
		eqVTime ( top->scb->last_used, ocb->svt ) &&
		(top->scb->status & SEG_FUTURE_FREE)  )
	{
		top->scb->status &= ~SEG_FUTURE_FREE;
		ocb->last_chance = NULL;

		num_units = 2 + top->magic_size;
		goto link_top;
	}

	num_units += 2; /* overhead for MallocSegHdr */
	num_units *= UNITSIZE; /* in Bytes */
	num_units *= SCALE; /* get SCALE times the request in Bytes */

	kilos_needed = ( num_units <= KILO_SIZE )?
		1 : (num_units + KILO_SIZE - 1) / KILO_SIZE;

	num_units = kilos_needed * KILO_SIZE / UNITSIZE; /* in MemBlockHdr's */

	if ( ocb->fcb && ocb->fcb->first_used_segment )
	{
		ocb->fcb->first_used_segment--;
	}
	else if ( new_fcb = (FootCB *) l_create ( sizeof(FootCB) )  )
	{
		/* debugging only? */
		clear ( new_fcb, sizeof(FootCB) );

		new_fcb->next = ocb->fcb;
		ocb->fcb = new_fcb;
		ocb->fcb->first_used_segment = SEG_PER_CB - 1;
	}
	else /* memory allocation failed */
	{
		_pprintf ( "moreKilos: could not alloc footCB\n" );
		return NULL;
	}

	if ( (seg_no = new_segment() ) == NO_MORE_SEGMENTS )
	{
		if ( new_fcb ) /* non-zero if alloc'ed above */
		{
			ocb->fcb = new_fcb->next;
			l_destroy ( new_fcb );
		}
		_pprintf ( "moreKilos: out of segments\n" );
		return NULL;
	}

	if ( ( start_kilo = kilo_alloc ( kilos_needed ) ) == NOT_ENUFF_KILOS )
	{
		seg_status[seg_no] = STATUS_FREE;
		if ( new_fcb ) /* non-zero if alloc'ed above */
		{
			ocb->fcb = new_fcb->next;
			l_destroy ( new_fcb );
		}
/*
		_pprintf ( "moreKilos: out of memory\n" );
*/
		return NULL;
	}

	if ( rv = rbc_allocate(seg_no, start_kilo, start_kilo + kilos_needed - 1) )
	{
		_pprintf ( "rbc_allocate returned %d in moreKilos\n", rv  );
		tester();
	}

	if ( rv = rbc_mark ( seg_no, ocb->frames_used - 1 ) )
	{
		_pprintf ( "mark strangeness in moreKilo's returned %d\n", rv );
		tester();
	}

	top = (MallocSegHdr *) rbc_kilo2real ( start_kilo );

	top->ocb = ocb;
	top->footCB_offset = ocb->fcb->first_used_segment;
	top->scb = &(ocb->fcb->seg[top->footCB_offset]);

	top->scb->status = SEG_ALLOC | SEG_PAST_FREE;
	top->scb->first_alloc = ocb->svt;
	top->scb->seg_no = seg_no;
	top->scb->rbc_start_KB = start_kilo;
	top->scb->rbc_end_KB = start_kilo + kilos_needed - 1;

link_top:
	top->next = top->prev  = 0;
	top->size = top->MemSize = MALLOC_HEADER;
	/* which is -1 so it will not be combined in the heap */

	/* call free with pointer below the header */
	rbc_free ( ocb, top + 1 );

	hp = (MemBlockHdr *) ( top + 1 );

	/* insert hp into the free list by hand */
	hp->s.next = top->next;
	top->next = hp;
	hp->s.prev = hp - 1; /* in the middle of top */
	top->magic_size = hp->s.MemSize = num_units - 2;

/*
	printf ( "%d more kilos of memory allocated to %s\n",
		kilos_needed, ocb->name );
	print_ocb_rbc ( ocb );
*/
	return 1; /* yes we found more kilo's */
}

rbc_free ( ocb, p )
Ocb * ocb;
MemBlockHdr *p; /* address of memory to be deallocated */
{
	MemBlockHdr *q, *r, *after_q;
	MainSegHdr *f = (MainSegHdr *)(ocb->footer) - 1;
/*
	printf ( "free: %s\n", ocb->name );
*/
	/*
	 * Hunt through the free list for the proper place to put the block,
	 * maintaining the list sorted by address value 
	 */

	r = (MemBlockHdr *) p - 1;  /* point to block header */

	if ( r->s.next || r->s.prev )
	{
		_pprintf ( "rbc_free: Munged block header\n" );
	}

	/* Move q until r is between q and the block after q */
	q = f->last_release;
	if ( r > q )
	{
		for ( ;; q = q->s.next)
		{
			if (r > q && r < q->s.next) /* found a spot between q and q_next */
			{
				break;
			}

			if (q >= q->s.next && (r > q || r < q->s.next))
			{
				/* q is at an end of the address - sorted list */
				/* and r is outside */
				break;
			}
		}
	}
	else
	{
		for ( ;; q = q->s.prev)
		{
			if (r > q && r < q->s.next) /* found a spot between q and q_next */
			{
				break;
			}

			if (q >= q->s.next && (r > q || r < q->s.next))
			{
				/* q is at an end of the address - sorted list */
				/* and r is outside */
				break;
			}
		}
	}

	after_q = q->s.next;

	/*
	 * Connect the block with its neighbors or combine (meld) them when
	 * adjacent 
	 */

	/* fit block to its lower neighbor */
	if (r + r->s.MemSize == after_q)
	{
		r->s.MemSize += after_q->s.MemSize;
		r->s.next = after_q->s.next;
		after_q->s.next->s.prev = r;
		if ( ( (MainSegHdr *)(ocb->footer) - 1 )->last_alloc == after_q )
			( (MainSegHdr *)(ocb->footer) - 1 )->last_alloc = q;
	}
	else
	{
		r->s.next = after_q;
		after_q->s.prev = r;
	}

	/* fit block to its upper neighbor */
	if (q + q->s.MemSize == r)
	{
		q->s.MemSize += r->s.MemSize;
		q->s.next = r->s.next;
		r->s.next->s.prev = q;
	}
	else
	{
		q->s.next = r;
		r->s.prev = q;

		q = r;
	}

	f->last_release = q;

	if ( ! ocb->keep_all_segs ) /* can release dynamic segments */
	{
		MallocSegHdr * p = (MallocSegHdr *) q - 1;

		if ( ( q->s.prev->s.MemSize == MALLOC_HEADER )
			&& ( q->s.prev ==  q - 1 )
			&& ( q->s.MemSize == p->magic_size ) )
		{
			p->scb->last_used = ocb->svt;
			p->scb->status |= SEG_FUTURE_FREE;

			if ( ( f->last_alloc == q ) || ( f->last_alloc == q - 1 ) )
				f->last_alloc = q->s.next;

			f->last_release = q->s.next;

			/* need to unlink it now! */
			p->prev->s.next = q->s.next;
			q->s.next->s.prev = p->prev;

			/* save segment for a second chance */
			if ( (ocb->last_chance == NULL) || ((p->magic_size) >
				(((MallocSegHdr *) ocb->last_chance)->magic_size)))
			{
				ocb->last_chance = (Byte *) p;
			}

			/* complete loop for debugging */
			p->prev = q;
			q->s.next = q - 1;
/*
			_pprintf ( "future free segment %d\n", p->scb->seg_no );
*/
		}
	}
}

/*--------------------------------------------------------------------------*
   The Initial Rollback Chip State
*--------------------------------------------------------------------------*/
init_op ( ocb, size_in_bytes )
Ocb *ocb;
unsigned long size_in_bytes;
{
	int rv, seg_no;
	MainSegHdr *top;
	MemBlockHdr *second, *last;

	int begin_kilo;
	int size_in_nunits = (size_in_bytes + UNITSIZE - 1)/UNITSIZE;
	int size_in_k = (size_in_bytes + 3 * UNITSIZE + 1023) >> 10;

	if ( (seg_no = new_segment() ) == NO_MORE_SEGMENTS )
	{
		_pprintf ( "no more segments\n" );
		tester();
	}

	if ( ( begin_kilo = kilo_alloc ( size_in_k ) ) == NOT_ENUFF_KILOS )
	{
		_pprintf ( "not_enuff_kilo's\n" );
		tester();
	}

	if ( rv = rbc_allocate ( seg_no, begin_kilo, begin_kilo +size_in_k - 1 ) )
	{
		_pprintf ( "rbc_allocate returned %d\n", rv );
		tester();
	}

	top = (MainSegHdr *) rbc_kilo2real ( begin_kilo );
	second = (MemBlockHdr *) top;
	second++;                   /* second points to footer now */
	ocb->footer = (void *) second;
	second += size_in_nunits;   /* second points above footer */
	last = second + 1;          /* move last beyond footer */

	second->s.next = second->s.prev = last;
	second->s.size = 0;
	second->s.MemSize = 0;      /* second will never be alloc'ed */

	last->s.next = last->s.prev = second;
	last->s.size = 0;
	last->s.MemSize = (1024 / UNITSIZE) * size_in_k - size_in_nunits - 2;

	top->last_release = top->last_alloc = last;
	top->ocb = ocb;
	top->footer_size = size_in_bytes;

	ocb->first_seg = seg_no;
	ocb->fcb = NULL;
	ocb->frames_used = 1;
	ocb->uses_rbc = TRUE;
}

/*--------------------------------------------------------------------------*
   Debugging Functions
*--------------------------------------------------------------------------*/
print_ocb_mem_list ( ocb )
Ocb * ocb;
{
	MainSegHdr * top = (MainSegHdr *) (ocb->footer) -1;
	MemBlockHdr *p;

	_pprintf ( "Dumping Free List for %s\n", ocb->name );
	_pprintf ( "last alloc 0x%x last release 0x%x\n", top->last_alloc,
		top->last_release );
	 for ( p = top->last_release;; )
	 {
		_pprintf ( "\tAdd: 0x%x Prev: 0x%x Next: 0x%x Size: %d\n",
			p, p->s.prev, p->s.next, p->s.MemSize );
		p = p->s.next;
		if ( p == top->last_release )
			break;
	 }
	 _pprintf ( "Done\n" );
}

print_kilo_status()
{
	int i;

	for ( i = 0; i < rbc_num_KB; i += 8 )
	{
		printf ( "kilo status %4d: %4d %4d %4d %4d  %4d %4d %4d %4d\n",
			i, kilo_status[i], kilo_status[i + 1],
			kilo_status[i + 2], kilo_status[i + 3],
			kilo_status[i + 4], kilo_status[i + 5],
			kilo_status[i + 6], kilo_status[i + 7] );
	}
}

print_malloc_seg_status()
{
	int i;

	for ( i = 0; i < NUM_SEGS; i += 8 )
	{
		printf ( "rbc_malloc seg status %3d: %d %d %d %d  %d %d %d %d\n",
			i, seg_status[i], seg_status[i + 1],
			seg_status[i + 2], seg_status[i + 3],
			seg_status[i + 4], seg_status[i + 5],
			seg_status[i + 6], seg_status[i + 7] );
	}
}

