/*      Copyright (C) 1989, 1991, California Institute of Technology.
		U. S. Government Sponsorship under NASA Contract NAS7-918
		is acknowledged.        */

/*
 * $Log:	mem.c,v $
 * Revision 1.7  91/12/27  09:08:13  pls
 * Add support for variable length address table (SCR 214).
 * 
 * Revision 1.6  91/11/01  09:49:02  pls
 * 1.  Change ifdef's, version id.
 * 2.  Add Derek's changes to mark memory.
 * 
 * Revision 1.5  91/07/17  15:09:58  judy
 * New copyright notice.
 * 
 * Revision 1.4  91/07/09  17:46:08  steve
 * Support for memdraw tool added.
 * 
 * Revision 1.3  91/06/03  12:24:54  configtw
 * Tab conversion.
 * 
 * Revision 1.2  90/11/27  09:45:07  csupport
 * 1.  change conflict with ANSII standard //
 * 2.  change some printf's to _pprintf's
 * 
 * Revision 1.1  90/08/07  15:40:11  configtw
 * Initial revision
 * 
*/
char mem_id [] = "@(#)mem.c     $Revision: 1.7 $\t$Date: 91/12/27 09:08:13 $\tTIMEWARP";


/*

Purpose:

		mem.c contains the basic memory allocation and deallocation
		routines, as well as some routines for determining the current
		status of the memory.  These routines are used by the Time
		Warp operating system to provide space for user purposes, and
		to support the operating system itself.  Therefore, any changes
		in these routines will affect the system uniformly.  Both
		the user and the kernel may compete for memory, so interrupts
		(if any exist) should be disabled when in the memory handling
		routines, to ensure mutual exclusion.

		The basic method used to handle memory in the current implementation
		is to maintain a linked list of unallocated memory segments.
		Requests for memory are satisfied from this list.  Only very
		simple methods are used to prevent fragmentation: released blocks of 
		memory that are physically contiguous to blocks of memory already
		in the linked list are combined, but no other attempt at memory
		compaction is made.

Functions:

		print_m_alloc_stats() - print memory allocation statistics
				Parameters - none
				Return - Always returns 0

		m_allocate(n) - allocate a piece of memory of size n
				Parameters - int n
				Return - a pointer to the allocated memory, or a 
						NULL pointer, if none available

		memanal() - print memory analysis stats into a file
				Parameters - none
				Return - Always returns 0

		mem_stats() - print memory analysis stats to the console
				Parameters - none
				Return - Always returns 0

		m_memory(num_units) - low level memory allocation routine
				Parameters - int num_units
				Return -  a pointer to the allocated memory, or a 
						NULL pointer, if none available

		print_m_release_stats() - print stats relating to releasing memory
				Parameters - none
				Return - Always returns 0

		m_release(p) - return a piece of memory to the allocater
				Parameters - Mem_hdr *p
				Return - Always returns 0

		percent_free() - find the percentage of memory currently free
				Parameters - none
				Return - percentage of free memory

		percent_used() - find the percentage of memory in use
				Parameters - none
				Return - percentage of memory in use

		bytes_free() - find the number of unused bytes of memory
				Parameters - none
				Return - number of free bytes

		m_available() - determine how much memory is in the free list
				Parameters - none
				Return - number of free bytes

		m_contiguous() - find the largest contiguous free block of memory
				Parameters - none
				Return - size of the largest free block

		m_next(arg) - return the size of the next free block of memory
				Parameters - Int arg
				Return - size of next free block

		set_memsize(memsize) - restrict the node's memory to a particular size
				Parameters - int memsize
				Return - return zero or tw_exit

		mavail(tblocks,cblocks,unused) - initialize some memory variables
				Parameters - Uint * tblocks, Uint * cblocks, Uint * unused
				Return - Always returns zero

		tw_getmem(addr,cblocks) - initialize the free memory pointer
				Parameters - Byte ** addr, Uint cblocks
				Return - SUCCESS or crash

Implementation:

		Some of the routines in mem.c are very simple, and some are
		quite complex.  The routines that print out or calculate statistics,
		such as print_m_alloc_stats() and percent_free(), are almost trivial.
		On the other hand, routines like m_allocate() and m_memory() have
		complexities both in their internal code and in their interactions.

		One detail that adds complexity to several routines, in the
		case of the Mark3 Hypercube version of the system, is that we
		have to divide the physical memory into two separate parts.
		Mercury has an unfortunate habit of writing into a particular
		place in memory that is not really its space.  When this write
		will be done is not completely predictable from within Time
		Warp, so we cannot save before the write and restore afterwards.
		Therefore, the memory allocation routines skip over the small
		area of memory affected.

		To deal with the simplest routines first, print_m_alloc_stats()
		performs a division to determine average search length.  (Unless
		no searches have occurred yet, in which case it sets the length
		to 0.)  The search in question is the search through the memory
		free list for suitably sized blocks of memory for allocation.
		Then it prints some statistics, including the one it just figured.

		memanal() opens a file called "memdump" for writing.  It runs
		through each element in the memory list, counting the number
		allocated and the number free, and keeping track of the size of
		memory allocated, and the largest block found.  The size of the
		free memory, and its largest block, is also maintained.  When all
		blocks have been examined, some statistics are written to the file,
		and the file is closed.

		mem_stats() works like memanal(), but prints to the console,
		rather than to a file.  Also, it calculates a fragmentation
		index, which is the largest free block divided by the total 
		free memory, the quantity multiplied by 100.

		print_m_release_stats() finds and prints the average number of 
		searches necessary to find where to put a released block of memory.
		This average is determinable from statistics kept by other
		routines.

		percent_free() uses two variables kept by the allocation and
		deallocation routines to determine what percentage of the
		memory is currently free.  percent_used() calls percent_free(),
		and subtracts the result from 100 to get its answer.  bytes_free()
		subtracts the number of bytes in use (from a variable kept by
		the allocation and deallocation routines) from the number of bytes
		initially available.

		m_available() runs through the free list, accumulating the various
		sizes of free blocks into a single total.

		m_contiguous() runs through the free list, looking for the largest
		single block in the list.  (If the routine is called before any
		allocations have been made, it instead calls mavail() to find
		out how much memory the system has, since, before allocations,
		it's all in one contiguous chunk.  mavail() deals with the
		special Mark3 case, in which a small section of memory cannot
		be used.)

		m_next() is a routine meant to be called iteratively.  On its
		first call, it initializes a search through the free list.  On
		all subsequent calls, it examines the next element in the free
		list, returning its size.

		The remaining routines are more substantial.  To understand 
		them, it helps to understand the general method used to
		manage the memory.  The system maintains a doubly linked list 
		of all free memory segments.  The system also keeps a pointer
		to the last block it allocated( really, to the unallocated block 
		before that block), and to the last block it released.  When a block 
		of memory must be allocated, the search starts at the block before
		the last block allocated, and continues until either a block
		large enough has been found, or until all blocks have been
		examined and rejected.  The allocation strategy used is first-fit,
		so once a block large enough has been found, that block is allocated.
		Deallocated blocks are linked back into the list by order of
		their addresses.  If a deallocated block is contiguous with a 
		block already in the list, those two blocks are merged.

		m_allocate() is the routine Time Warp uses when it wants memory.
		(Currently, it is only called from the routines in list.c that
		create new list elements, implying that all memory allocated
		is for list elements.)  The routine starts by setting a search
		pointer to the free element before the last element that was
		allocated.  (Seaches start from where the last search ended, 
		more or less.)  The first time the routine is called, this
		pointer is NULL, of course.  In this case, the search pointer
		is set to the dummy element at the base of the list.  Next,
		the allocation requested is turned into some integral number
		of allocation blocks.  Memory is allocated only in multiples of
		UNITSIZE bytes.  (UNITSIZE is the size of a memory list header.)
		One is added to the number of blocks requested by the user, so
		that the allocation will include a memory header.  The sizes
		kept in the memory free list are in UNITSIZE denominations, not
		in bytes.

		Now m_allocate() starts running through the free list.  For
		each element, the size of that element (kept in the memory
		header) is checked.  If an element of exactly the number of units
		requested is found, then the important work is done.  If the
		element is too small, move on to the next element.  If the
		element is too big, memory will be allocated from this element,
		but the unused portion must be linked into the free list.
		To minimize the amount of linking and unlinking, the memory is
		allocated from the end of the element that is too big, so all
		that need be changed in that free list element is its size.
		A search pointer is set to the proper offset within the element,
		and this new memory element's header gets a size equal to that
		calculated for the allocation.  Its pointer fields are zeroed,
		to indicate that it is allocated.

		If the entire free list was searched without finding a block of
		memory large enough, then m_memory() is called.  In principle,
		m_memory() could do memory compaction in such cases.  In the
		current implementation, however, calling m_memory() is only
		effective the first time, when the call results in setting up
		the free list, in the first place.  All subsequent times,
		m_memory() will fail to find more memory.

		If memory statistics are being gathered, then whether or not memory 
		was found, increment a counter that keeps track of how many times 
		the routine was called.  For each element in the free list that 
		was examined, a counter called "search" was incremented.  Check 
		to see if this was the longest search through the free list so far, 
		and keep track of it, if it was.  

		If memory was found, multiply the number of memory blocks allocated 
		by UNITSIZE, and add the result into bytes_used.  Return a pointer 
		to the memory.  This pointer points just beyond the memory header.

		m_memory() is, in this version of the system, the memory free
		list initialization routine.  It's called at other times, but
		those calls have a net effect of returning failure.  If memory
		compaction were being used in the system, m_memory() is the
		place it would go.

		The first thing m_memory() does is call mavail().  mavail() is
		a routine in Tester.c that fills in some information about the
		most contiguous blocks, and the total number of blocks, that
		the machine interface can provide.  Unless this is the first
		call to mavail(), the current answer is always that no blocks
		can be provided.  On the first call, it provides as many blocks
		as the memory (as defined in two compile-time parameters) holds.
		(No provision is made here for dealing with the Mark3's special
		problem.)  One item returned by mavail() is the number of blocks
		in the system.  If the global variable holding the number of
		bytes initially available is still 0, set it by multiplying
		the number of blocks by the block size.  (As best as I can tell,
		this global variable is never initialized to zero, so, if this
		calculation is ever performed, it's only through luck.  The
		variable should be set to zero somewhere in start.c.) 

		Another item provided by mavail() was the largest number of
		contiguous blocks available from the machine interface.  (At
		system start time, this is set to the same number as the total
		number of blocks available.  Given the Mark3's little problem,
		it shouldn't be.)  Find out how many contiguous allocation-sized 
		units are available.  If it's less than the number requested in
		m_memory()'s parameter, set the return value to indicate failure.
		If it's large enough, call tw_getmem() to get it all.  If this
		call succeeds, start setting it up.

		It is here that m_memory() reveals that it is only meant to
		do real work at initialization time.  Two global variables that
		indicate the beginning and the end of memory are set to values
		dependent on the new machine interface allocation.  Also, if
		running on the Mark3, and if the allocation includes the
		bad area, chop up the allocation into two segments, one before
		the problem aread, one after it, and call m_release() on both.
		m_release() will effectively put them into the memory free list.

		If not running on the Mark3 (or, in the future, when the Mark3
		problem has been fixed), set up one huge free element, and
		call m_release() to put it in the free list.  m_release() has
		the side effect of decreasing the bytes_used field by the amount
		released.  This field shouldn't be decreased, in this case, because
		memory is being added for the first time, rather than being returned
		from use, so increase the bytes_used field by the amount of
		memory sent to m_release().  At this point, the code again reveals
		that it was only meant to be executed once per run.  The pointers
		used in searching are set to the start of the newly allocated memory.
		If this routine were being used to get a supplemental allocation,
		perhaps as a result of memory compaction, this line of code could
		have the effect of setting up an alternate free list, and could
		cast the main one adrift, possibly causing the run to lose
		substantial amounts of memory.  (If the memory compaction routine
		completely reworked the existing free list, and returned a 
		pointer to it, then this problem would not arise, since the old
		free list would have been destroyed to make the new one.)

		After all is done, either a pointer to the memory, or NULL, is
		returned.

		m_release() must start by finding where to put the released block
		in the free list.  That list is ordered by pointer, a physical
		ordering.  Starting at the last place a released block was placed,
		look at each element in the free list. Examine its pointer, 
		comparing it to the address of the memory header of the block to
		be released.  The search may go either forward or backward, 
		depending on its starting point.  

		Once the place has been found, if we are keeping statistics,
		keep track of information about how much searching we did.

		Now check to see if the released block is physically contiguous
		with either the block above it, the block below it, or both.
		If it is contiguous with another block, combine it with that
		block into a single block.  This combination requires changing
		pointers in the list and changing size fields.  The exact pointers
		to be changed, and their new values, depend on whether the merge
		is with the preceding or following block.  Finally, set the
		last-released-block pointer to the newly freed block, and decrease
		the number of blocks used by the size of that block.

		set_memsize() multiplies its parameter by 1024, to convert from
		kbytes to bytes.  It allocates an amount of memory equal to that
		actually available less that desired, and sets that memory aside.

		mavail() initializes the free memory list.  tw_getmem() calculates
		and returns a pointer to the free memory list.


*/

#include <stdio.h>  
#include "twcommon.h"
#include "twsys.h"
#include "tester.h"
#include "machdep.h"

FILE * HOST_fopen ();

#define UNITSIZE sizeof(Mem_hdr)

Mem_hdr         base;                   /* empty list to get started */
Mem_hdr        *allocp_a = NULL;        /* last block allocated */
Mem_hdr        *allocp_r = NULL;        /* last block released */


#if M_ALLOCATE_STATS

int m_allocate_calls;
int m_allocate_searches;
int m_allocate_longest_search;



print_m_allocate_stats ()
{
	int m_allocate_average_search = 0;

	if ( m_allocate_calls > 0 )
		m_allocate_average_search = m_allocate_searches / m_allocate_calls;

	printf ( "\n" );
	printf ( "M_allocate: Number of Calls = %d\n", m_allocate_calls );
	printf ( "M_allocate: Number of Loops = %d\n", m_allocate_searches );
	printf ( "M_allocate: Longest Search = %d\n", m_allocate_longest_search );
	printf ( "M_allocate: Average Search = %d\n", m_allocate_average_search );
}

#endif

#define BLOCK_SIZE      2048

extern char * mem;
extern int mem_size;

FUNCTION Byte  *m_allocate (n)
	int            n;           /* in bytes */
{
	register  Mem_hdr        *p;
	register  Mem_hdr        *q;
	int            nunits;
	Byte           *retval;

#if M_ALLOCATE_STATS

	register int search = 0;

#endif

	q = allocp_a;
	if (q == NULL)
	{                           /* create the free list first time through */
		base.s.next = base.s.prev = allocp_a = allocp_r = q = &base;
		base.s.size = 0;
	}

	/* Round request up to multiple of a header... */

	nunits =   (n + UNITSIZE - 1) /
		/*-------------------------*/
				   UNITSIZE;

	nunits += 1;                /* plus room for one header */

	/* Hunt through the free list for memory, allocating from MI as needed */
	/* q = allocp already, start with the block in the free chain AFTER */
	/* the block at q */

	/* First time through, q points to base and so does q->s.next */

	/* q is allocp */

	for (p = q->s.next; /* NO TEST */ ; q = p, p = p->s.next)
	{

#if M_ALLOCATE_STATS

		search++;

#endif

		if (p->s.size >= nunits)
		{                       /* FOUND FIRST FIT */
			if (p->s.size == nunits)
			{                   /* EXACT FIT */

				q->s.next = p->s.next;
				p->s.next->s.prev = q;

				if ( allocp_r == p )
					allocp_r = q;
			}

			else
			{                   /* ROOM LEFT OVER */
				p->s.size -= nunits;
				p += p->s.size;
				p->s.size = nunits;
			}

			allocp_a = q;       /* NEXT TIME, START SEARCHING AFTER HERE */
			retval =  (Byte *)  (p + 1);        /* BEYOND HEADER */

			p->s.next = p->s.prev = 0;  /* flag allocated block */

			break;

		}

		/* First time through, we get to here */

		if (p == allocp_a)
		{
			/* wrapped around end of free list */

			p = m_memory (nunits);
			if (p == NULL)
			{
				retval = NULL;
				break;
			}
		}
	}

#if M_ALLOCATE_STATS

	m_allocate_calls++;
	m_allocate_searches += search;
	if ( search > m_allocate_longest_search )
		m_allocate_longest_search = search;

#endif

	if (retval != NULL)
	{
		bytes_used += nunits * UNITSIZE /* includes the header */ ;
	}

	return retval;
}

char * membeg;
char * memend;

/* zero is free, non-zero is allocated, == 1 is unknown */
#define FREE_MEM	0x040000000
#define LOST_MEM	0x01
#define ALLOCATED	0x01
#define POOL_MEM	0x02
/* Object Stuff */
#define OBJECT_MEM	0x04
#define OCB_MEM		0x08
#define FILETYPE_AREA	0x080000000
	/* State stuff */
#define	STATE_CUR	0x010
#define STATE_Q		0x020
#define STATE_ADD	0x040
#define STATE_DYM	0x0800
#define	STATE_SENT	0x01000
#define STATE_ERR	0x02000
#define STACK_MEM	0x04000
	/* Message stuff */
#define MESSAGE_MEM	0x08000
#define INPUT_MSG	0x010000
#define OUTPUT_MSG	0x020000
		/* Current Message copies */
#define MESS_VEC	0x040000
#define NOW_EMSGS	0x080000
		/* signs */
#define POS_MSG		0x0100000
#define NEG_MSG		0x0200000
#define SYS_MSG		0x0400000
	/* Queue HEADER */
#define QUEUE_HDR	0x0800000
/* TW overhead */
#define TW_OVER		0x01000000
#define CACHE_MEM	0x02000000
#define HOME_LIST	0x04000000
/* COMMUNICATIONS */
#define IN_TRANS	0x08000000
#define ONNODE		0x010000000
#define OFFNODE		0x020000000

#define MEM_HEADER(n)	( ( (Mem_hdr *) ( LIST_HEADER(n) ) ) - 1 )
#define obj_id pad

markstate (state, oid)
	State *state;
	int oid;
{
	register Mem_hdr  *p;
	register List_hdr *q;
	int i;

	if ( state->address_table != NULL )
	{
		q = ( (List_hdr *) state->address_table ) - 1;
		p = ( (Mem_hdr *) q ) - 1;
		q->obj_id = oid;
		p->s.type = STATE_ADD;
		for ( i = 0; i < l_size(state->address_table) / sizeof(Address); i++ )
			if ( state->address_table[i] != NULL &&
				state->address_table[i] != DEFERRED )
			{
				q = ( (List_hdr *) state->address_table[i] ) - 1;
				p = ( (Mem_hdr *) q ) - 1;
				q->obj_id = oid;
				p->s.type = STATE_DYM;
			}
	}
	if ( state->serror != NULL )
	{
		q = ( (List_hdr *) state->serror ) - 1;
		p = ( (Mem_hdr *) q ) - 1;
		q->obj_id = oid;
		p->s.type = STATE_ERR;
	}
}

#define ismoving_macro(M)	BITTEST( (M)->flags,MOVING )
#define COMING 0
#define GOING 1
markmessage ( message, direction )
	Msgh *message;
	int direction;
{
	Mem_hdr  *p = MEM_HEADER (message);

	if ( isposi_macro(message) )
		p->s.type |= POS_MSG;
	if ( isanti_macro(message) )
		p->s.type |= NEG_MSG;
	if ( issys_macro(message) )
		p->s.type |= SYS_MSG;
	if ( ismoving_macro(message) )
		p->s.type |= IN_TRANS;
	if ( direction == GOING )
	{
		if ( FindInSchedQueue(message->rcver, message->rcvtim)
			 == NULL )
			p->s.type |= OFFNODE;
		else
			p->s.type |= ONNODE;
	}
	else
	{
		if ( FindInSchedQueue(message->snder, message->sndtim)
			 == NULL )
			p->s.type |= OFFNODE;
		else
			p->s.type |= ONNODE;
	}
}

markOcb ( fp, ocb, mask )
	FILE *fp;
	Ocb * ocb;
	long mask;
{
	register Mem_hdr  *p;
	register List_hdr *q;
	register Msgh *r;
	int i;

	/* Mark the object control block itself */
	q = (List_hdr *) ocb;
	p = MEM_HEADER (q);
	p->s.type = OBJECT_MEM | OCB_MEM | mask;
	(q-1)->obj_id = ocb->oid;

	/* Mark the saved state queue header */
	q = (List_hdr *) (ocb->sqh);
	p = MEM_HEADER (q);
	p->s.type = OBJECT_MEM | QUEUE_HDR | mask;
	(q-1)->obj_id = ocb->oid;

	/* Cycle through the object's saved states */
	for ( q = (List_hdr *) fststate_macro(ocb); q;
		q = (List_hdr *) nxtstate_macro(q) )
	{
		p = MEM_HEADER (q);
		p->s.type = OBJECT_MEM | STATE_Q | mask;
		(q-1)->obj_id = ocb->oid;
		markstate( (State *) q, ocb->oid);
	}

	/* Mark the r state */
	q = (List_hdr *) ocb->rstate;
	if ( q != NULL)
	{
		p = MEM_HEADER (q);
		p->s.type = OBJECT_MEM | IN_TRANS | mask;
		(q-1)->obj_id = ocb->oid;
		markstate( (State *) q, ocb->oid);
	}

	/* Mark message vector stuff */
	q = (List_hdr *) ocb->msgv;
	if ( q != NULL)
	{
		p = MEM_HEADER (q);
		p->s.type = MESS_VEC | mask;
		(q-1)->obj_id = ocb->oid;
		for ( i = 0; i < ocb->ecount; i++ )
			if ( ocb->msgv[i] != NULL )
			{
				q = (List_hdr *) ocb->msgv[i];
				(q-1)->obj_id = ocb->oid;
				p = MEM_HEADER (q);
				p->s.type = NOW_EMSGS | MESSAGE_MEM | mask;
			}
	}

	/* Mark STATE_SENT */
	q = (List_hdr *) ocb->last_sent;
	if ( q && ocb->out_of_sq )
	{
		p = MEM_HEADER (q);
		p->s.type = STATE_SENT | mask;
		(q-1)->obj_id = ocb->oid;
		markstate( (State *) q, ocb->oid);
	}

	/* Mark the temporary stack */
	q = (List_hdr *) ocb->stk;
	if ( q != NULL)
	{
		p = MEM_HEADER (q);
		p->s.type = STACK_MEM | mask;
		(q-1)->obj_id = ocb->oid;
	}

	/* Mark the temporary state buffer */
	q = (List_hdr *) ocb->sb;
	if ( q != NULL)
	{
		p = MEM_HEADER (q);
		p->s.type = STATE_CUR | mask;
		(q-1)->obj_id = ocb->oid;
		markstate( (State *) q, ocb->oid);
	}

	/* Mark the input queue header */
	q = (List_hdr *) (ocb->iqh);
	p = MEM_HEADER (q);
	p->s.type = OBJECT_MEM | QUEUE_HDR | MESSAGE_MEM | mask;
	(q-1)->obj_id = ocb->oid;

		/* Cycle through the object's input messages */
	for ( q = (List_hdr *) fstimsg_macro(ocb); q;
		q = (List_hdr *) nxtimsg_macro(q) )
	{
		p = MEM_HEADER (q);
		p->s.type = OBJECT_MEM | INPUT_MSG | MESSAGE_MEM | mask;
		(q-1)->obj_id = ocb->oid;
		r = (Msgh *) q;
		markmessage( r, COMING );
	}

	/* Mark the output queue header */
	q = (List_hdr *) (ocb->oqh);
	p = MEM_HEADER (q);
	p->s.type = OBJECT_MEM | QUEUE_HDR | MESSAGE_MEM | mask;
	(q-1)->obj_id = ocb->oid;

		/* Cycle through the object's output messages */
	for ( q = (List_hdr *) fstomsg_macro(ocb); q;
		q = (List_hdr *) nxtomsg_macro(q) )
	{
		p = MEM_HEADER (q);
		p->s.type = OBJECT_MEM | OUTPUT_MSG | MESSAGE_MEM | mask;
		(q-1)->obj_id = ocb->oid;
		r = (Msgh *) q;
		markmessage( r, GOING );
	}

	/* Write out the id# & names associated with this Ocb */
	HOST_fprintf ( fp, "%d\t%s\n", ocb->oid, ocb->name );
}


markmem ( fp )
	FILE *fp;
/*
   markmem has two functions:  1) to mark memory according to its use by 
	filling in the "type" field in the memory header block (Mem_hdr.type), 
	and 2) to write information about object memory use to "memdump".

   markmem first writes a list of object id numbers and their names
	followed by a marker that is a dummy id number of -1.
   Then it writes lines that consist of two numbers:  1) the (hexadecimal) 
	address of a memory block, and 2) the object id number of the object
	that is associated with that memory block.  This is followed by
	another marker, which is an address of 0.  The rest of the writing is
	done in memdump().
*/ 
{
	register Mem_hdr  *p;
	register List_hdr *q;
	register Msgh *r;
	Ocb *ocb;
	extern List_hdr * free_pool, * msg_free_pool;
	int i;

#define CACHE_SIZE 63
extern Cache_entry *CacheFreePool, *objloc_cache[];
extern struct HomeList_str *HomeListHeader[];
extern int homeListSize;
extern Msgh *rmq, *command_queue;
extern Pending_entry * PendingListHeader;

	for ( p = (Mem_hdr *) membeg; p < (Mem_hdr *)memend; p += p->s.size )
	{
		((List_hdr *) (p + 1))->obj_id = -1;
		if ( p->s.next != 0 )
			p->s.type = FREE_MEM;
		else
			p->s.type = LOST_MEM;
	}

	p = MEM_HEADER (free_pool);
	p->s.type = TW_OVER | QUEUE_HDR;

	for ( q = l_next_macro(free_pool); q != free_pool; q = l_next_macro (q) )
	{
		p = MEM_HEADER (q);
		p->s.type = POOL_MEM;
	}

	p = MEM_HEADER (msg_free_pool);
	p->s.type = TW_OVER | QUEUE_HDR;

	for ( q = l_next_macro(msg_free_pool); q != msg_free_pool;
		q = l_next_macro (q) )
	{
		p = MEM_HEADER (q);
		p->s.type = POOL_MEM;
	}
	p = MEM_HEADER (emergbuf);
	p->s.type = POOL_MEM | TW_OVER;
	p = MEM_HEADER (report_buf);
	p->s.type = POOL_MEM | TW_OVER;
	p = MEM_HEADER (brdcst_buf);
	p->s.type = POOL_MEM | TW_OVER;
	p = MEM_HEADER (rm_buf);
	p->s.type = POOL_MEM | TW_OVER;
	if ( rm_msg != NULL )
	{
		p = MEM_HEADER (rm_msg);
		p->s.type = POOL_MEM | TW_OVER;
	}
	
	p = MEM_HEADER (_prqhd);
	p->s.type = TW_OVER | QUEUE_HDR;

	for ( ocb = fstocb_macro; ocb; ocb = nxtocb_macro(ocb) )
		/* Cycle through all objects, marking their memory */
		markOcb( fp, ocb, NULL );

		/* Mark the send queue */
	p = MEM_HEADER (pmq);
	p->s.type = QUEUE_HDR | MESSAGE_MEM;
		/* Mark pmq and rmq messages */
	for ( q = (List_hdr *) l_next_macro (pmq);
			!l_ishead_macro(q);
			q = l_next_macro(q) )
	{
		markmessage ( (Msgh *) q, GOING );
		p = MEM_HEADER (q);
		p->s.type = MESSAGE_MEM | IN_TRANS;
	}

		/* Mark the receive queue */
	p = MEM_HEADER (rmq);
	p->s.type = QUEUE_HDR | MESSAGE_MEM;
	for ( q = (List_hdr *) l_next_macro (rmq);
			!l_ishead_macro(q);
			q = l_next_macro(q) )
	{
		markmessage ( (Msgh *) q, COMING );
		p = MEM_HEADER (q);
		p->s.type = MESSAGE_MEM | IN_TRANS;
	}

		/* Mark the Pending List */
	p = MEM_HEADER (PendingListHeader);
	p->s.type = QUEUE_HDR | TW_OVER;
	for ( q = (List_hdr *) l_next_macro (PendingListHeader);
			!l_ishead_macro(q);
			q = l_next_macro(q) )
	{
		p = MEM_HEADER (q);
		p->s.type = TW_OVER | IN_TRANS;
	}

		/* Mark the Command Queue */
	p = MEM_HEADER (command_queue);
	p->s.type = QUEUE_HDR | TW_OVER;
	for ( q = (List_hdr *) l_next_macro (command_queue);
			!l_ishead_macro(q);
			q = l_next_macro(q) )
	{
		p = MEM_HEADER (q);
		p->s.type = IN_TRANS;
	}

		/* Mark home location cache memory */
	for (i = 0; i < CACHE_SIZE; i++)
	{
		p = MEM_HEADER (objloc_cache[i]);
		p->s.type = CACHE_MEM | QUEUE_HDR | TW_OVER;
		
		for ( q = (List_hdr *) l_next_macro(objloc_cache[i]);
				!l_ishead_macro(q); 
				q = l_next_macro(q) )
		{
			p = MEM_HEADER (q);
			p->s.type = CACHE_MEM | TW_OVER;
		}
	}

		/* Mark free cache pool memory */

	p = MEM_HEADER (CacheFreePool);
	p->s.type = QUEUE_HDR | POOL_MEM;
	for ( q = (List_hdr *) l_next_macro (CacheFreePool);
			!l_ishead_macro(q);
			q = l_next_macro(q) )
	{
		p = MEM_HEADER (q);
		p->s.type = POOL_MEM;
	}

		/* Mark HomeList memory */
	for (i = 0; i < homeListSize; i++)
	{
		p = MEM_HEADER (HomeListHeader[i]);
		p->s.type = HOME_LIST | QUEUE_HDR | TW_OVER;

		for (q = (List_hdr *) l_next_macro(HomeListHeader[i]);
				!l_ishead_macro(q);
				q = l_next_macro(q) )
		{
			p = MEM_HEADER (q);
			p->s.type = HOME_LIST | TW_OVER;
		}
	}

		/* Mark dead Ocb q memory */
	p = MEM_HEADER (DeadOcbList);
	p->s.type = OCB_MEM | QUEUE_HDR;
	for ( q = (List_hdr *) l_next_macro (DeadOcbList);
			!l_ishead_macro(q);
			q = l_next_macro(q) )
	{
		p = MEM_HEADER (q);
		p->s.type = OCB_MEM;
	}

		/* Mark Send Ocb q memory */
	p = MEM_HEADER (sendOcbQ);
	p->s.type = OCB_MEM | QUEUE_HDR;
	for ( q = (List_hdr *) l_next_macro (sendOcbQ);
			!l_ishead_macro(q);
			q = l_next_macro(q) )
		markOcb( fp, (Ocb *) q, IN_TRANS );

		/* Mark Send State q memory */
	p = MEM_HEADER (sendStateQ);
	p->s.type = STATE_Q | QUEUE_HDR;
	for ( q = (List_hdr *) l_next_macro (sendStateQ);
			!l_ishead_macro(q);
			q = l_next_macro(q) )
	{
		p = MEM_HEADER (q);
		p->s.type = TW_OVER | IN_TRANS;
		p = MEM_HEADER ( ((State_Migr_Hdr *) q)->state );
		p->s.type = STATE_Q | STATE_SENT | IN_TRANS;
		( ( (List_hdr *) ( ((State_Migr_Hdr *) q)->state ) ) - 1)->obj_id =
			( (Ocb *) ((State_Migr_Hdr *) q)->state->ocb )->oid;
		markstate ( ((State_Migr_Hdr *) q)->state,
			( (Ocb *) ((State_Migr_Hdr *) q)->state->ocb )->oid );
	}

		/* Mark the type table type areas */
	for ( i = 0; i < MAXNTYP; i++ )
	{
		if ( type_table[i].typeArea != NULL )
		{
			p = ( (Mem_hdr *) type_table[i].typeArea ) - 1;
			p->s.type = FILETYPE_AREA;
		}
	}

		/* Mark the file areas */
	for ( i = 0; i < MAX_TW_FILES; i++ )
	{
		if ( tw_file[i].area != NULL )
		{
			p = ( (Mem_hdr *) tw_file[i].area ) - 1;
			p->s.type = FILETYPE_AREA;
		}
	}

		/* Mark the end of the Ocb id# & name list */
	HOST_fprintf ( fp, "-1\tMartha\n" );
}

memanal ()
{
	register  Mem_hdr        *p;
	char filename[30];

	int a_largest = 0, a_number = 0, a_total = 0;
	int f_largest = 0, f_number = 0, f_total = 0;
	int allocated;
	FILE *fp;

	sprintf ( filename, "%dmemdump", miparm.me );

	fp = HOST_fopen ( filename, "w" );

	markmem( fp );

	for ( p = (Mem_hdr *) membeg; p < (Mem_hdr *)memend; p += p->s.size )
	{
		if ( p->s.next == 0 )           /* it is allocated */
		{
			allocated = TRUE;

			a_number++;

			a_total += p->s.size;

			if ( p->s.size > a_largest )
				a_largest = p->s.size;
		}
		else
		{
			allocated = FALSE;

			f_number++;

			f_total += p->s.size;

			if ( p->s.size > f_largest )
				f_largest = p->s.size;
		}

		HOST_fprintf ( fp, "%d %8x\t%8x\t%8d\t%8d\n",
			p->s.type, p, p->s.size * UNITSIZE, p->s.size * UNITSIZE, ((List_hdr *) (p + 1))->obj_id );
	}

	_pprintf ( "\nbytes_used = %d\n", bytes_used );

	_pprintf ( "number used is %d, largest is %d bytes, total is %d\n",
			a_number, a_largest * UNITSIZE, a_total * UNITSIZE );

	_pprintf ( "number free is %d, largest is %d bytes, total is %d\n",
			f_number, f_largest * UNITSIZE, f_total * UNITSIZE );

	HOST_fprintf ( fp, "\nbytes_used = %d\n", bytes_used );

	HOST_fprintf 
				( fp, 
				  "number used is %d, largest is %d bytes, total is %d\n",
				  a_number, a_largest * UNITSIZE, a_total * UNITSIZE 
				);

	HOST_fprintf ( fp, 
				   "number free is %d, largest is %d bytes, total is %d\n",
				   f_number, f_largest * UNITSIZE, f_total * UNITSIZE 
				 );

	HOST_fclose ( fp );
}

mem_stats ()
{
	register  Mem_hdr        *p;

	int a_largest = 0;
	int f_largest = 0, f_number = 0, f_total = 0;
	double fragmentation_index;

	for ( p = (Mem_hdr *) membeg; p < (Mem_hdr *) memend; p += p->s.size)
	{
		if ( p->s.next == 0 )           /* it is allocated */
		{
			if ( p->s.size > a_largest )
				a_largest = p->s.size;
		}
		else
		{
			f_number++;

			f_total += p->s.size;

			if ( p->s.size > f_largest )
				f_largest = p->s.size;
		}
	}

	f_largest *= UNITSIZE;
	f_total *= UNITSIZE;

	if ( f_total == 0 )
		fragmentation_index = 0.0;
	else
		fragmentation_index = 100. * f_largest / f_total;

	_pprintf
 ( "num free blocks = %d, largest = %d, total = %d, frag index = %.2f\n",
		f_number, f_largest, f_total, fragmentation_index );
}

FUNCTION Mem_hdr *m_memory (num_units)
	int            num_units;   /* in Mem_hdr's */
{
	register Mem_hdr *hp;

	Uint            cblocks,
					tblocks;
	Ulong           avail_units;
	char           *addr;
	Mem_hdr        *retval;
	Int             status;     /* for memory checking routine */

	mavail (&tblocks, &cblocks);

	if (initial_freemem_bytes == 0)
	{
		initial_freemem_bytes = tblocks * BLOCK_SIZE;
	}

	/* cblocks = largest contiguous area */

	avail_units = (cblocks * BLOCK_SIZE) / UNITSIZE;

	if (avail_units < num_units)
	{                           /* couldn't satisfy request */
		retval = NULL;
	}

	else
	{


		status = tw_getmem (&addr);     /* GET IT ALL */
		if (status == SUCCESS)
		{
			membeg = addr;
			memend = membeg + initial_freemem_bytes;

			{
				hp =  (Mem_hdr *) addr;
				hp->s.size = avail_units;
				m_release (hp + 1);
			}
			bytes_used += avail_units * UNITSIZE;       /* patch value */
			/* because m_release will subtract from bytes_used */

			/* m_release sets allocp... */
			retval = allocp_a = allocp_r;
		}

		else
		{
			retval = NULL;
		}

	}

	return retval;
}


#if M_RELEASE_STATS

int m_release_calls;
int m_release_searches;
int m_release_longest_search;

print_m_release_stats ()
{
	int m_release_average_search = 0;

	if ( m_release_calls > 0 )
		m_release_average_search = m_release_searches / m_release_calls;

	printf ( "\n" );
	printf ( "M_release: Number of Calls = %d\n", m_release_calls );
	printf ( "M_release: Number of Loops = %d\n", m_release_searches );
	printf ( "M_release: Longest Search = %d\n", m_release_longest_search );
	printf ( "M_release: Average Search = %d\n", m_release_average_search );
	printf ( "\n" );
}

#endif

FUNCTION        m_release (p)
	Mem_hdr           *p;       /* address of memory to be deallocated */
{
	register Mem_hdr        *q;
	register Mem_hdr        *r;
	register Mem_hdr        *after_q;
	register Ulong           r_size;
	register Ulong           q_size;

#if M_RELEASE_STATS

	register int searches = 0;

#endif

	/*
	 * Hunt through the free list for the proper place to put the block,
	 * maintaining the list sorted by address value 
	 */

	r = (Mem_hdr *) p - 1;      /* point to block header */

	r_size =  r->s.size;        /* Units */

	/* Move q until r is between q and the block after q */

	q = allocp_r;

	if ( r > q )
	{
		for ( ;; /* NO LOOP TEST HERE */ q = q->s.next)
		{

#if M_RELEASE_STATS

			searches++;
#endif

			if (r > q && r < q->s.next)
			{
				/* found a spot between q and q_next */
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
		for ( ;; /* NO LOOP TEST HERE */ q = q->s.prev)
		{

#if M_RELEASE_STATS

			searches++;
#endif

			if (r > q && r < q->s.next)
			{
				/* found a spot between q and q_next */
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

#if M_RELEASE_STATS

	m_release_calls++;
	m_release_searches += searches;
	if ( searches > m_release_longest_search )
		m_release_longest_search = searches;

#endif

	q_size =  q->s.size;        /* Units */

	after_q = q->s.next;

	/*
	 * Connect the block with its neighbors or combine (meld) them when
	 * adjacent 
	 */

	/* fit block to its lower neighbor */
	if (r + r_size == after_q)
	{
		r->s.size += after_q->s.size;
		r->s.next = after_q->s.next;
		after_q->s.next->s.prev = r;
		if ( allocp_a == after_q )
			allocp_a = q;
	}

	else
	{
		r->s.next = after_q;
		after_q->s.prev = r;
	}

	/* fit block to its upper neighbor */

	if (q + q_size == r)
	{
		q->s.size += r->s.size;
		q->s.next = r->s.next;
		r->s.next->s.prev = q;
	}

	else
	{
		q->s.next = r;
		r->s.prev = q;
	}

	allocp_r = q;
	bytes_used -= (r_size * UNITSIZE);
}



FUNCTION int   percent_free ()
{
	return (int) (

	   (100 * (initial_freemem_bytes - bytes_used)) /
	/*--------------------------------------------------*/
				   initial_freemem_bytes

	);
}

FUNCTION int   percent_used ()
{
	return (int) (100 - percent_free ());

}

FUNCTION long  bytes_free ()
{
	return initial_freemem_bytes - bytes_used;
}


#if 0                           
/*---------------------------------------------------------------*/
/* THIS ROUTINE IS EXPENSIVE */

/* returns total number of free bytes in pool */

FUNCTION int   m_available ()
{
	int            nunits,
					tblocks,
					cblocks;
	Long           *paddr;
	Mem_hdr        *q;

	if ((q = allocp) == NULL)
	{
		base.s.next = allocp = q = &base;
		base.s.size = 0;
	}

	/* Add up the number of memory units available in the free list */
	nunits = 0;
	do
	{
		nunits += q->s.size;
		q = q->s.next;

	} while (q != allocp);

	/* Find the space available from the MI */
	mavail (&tblocks, &cblocks);
	if (initial_freemem_bytes == 0)
	{
		initial_freemem_bytes = tblocks * BLOCK_SIZE;
	}

	return (nunits * UNITSIZE) + (tblocks * BLOCK_SIZE);
}

#endif                          
/*---------------------------------------------------------------*/



/* returns size in bytes of largest contiguous region in free pool */

FUNCTION int   m_contiguous ()
{
	int            max,
					nbytes,
					tblocks,
					cblocks;
	Mem_hdr        *q;
	register int   r;

	/* Initialize for the first time */

	if ((q = allocp_a) == NULL)
	{                           /* create the free list first time */
		base.s.next = allocp_a = q = &base;
		base.s.size = 0;
	}

	/* Find the largest contiguous space available from the MI */

	mavail (&tblocks, &cblocks);
	if (initial_freemem_bytes == 0)
	{
		initial_freemem_bytes = tblocks * BLOCK_SIZE;
	}
	max = cblocks * BLOCK_SIZE;

	/* Find the largest contiguous space from the MI or the free list */
	do
	{
		nbytes = q->s.size * UNITSIZE;
		if (nbytes > max)
			max = nbytes;
		q = q->s.next;
	} while (q != allocp_a);

	/* Return the proper value, taking into account the header size */

	if (max < UNITSIZE)
	{
		r = 0;
	}

	else
	{
		r = max - UNITSIZE;
	}

	return r;
}



set_memsize ( memsize )

	int * memsize;
{
	int desired_memsize;
	int disposal_memsize;
	Byte * p;

	desired_memsize = *memsize * 1024;

	if ( desired_memsize > mem_size )
	{
		_pprintf ( "Desired Memory Size %d is greater than maximum %d\n",
			desired_memsize, mem_size );
		tw_exit (0);
	}

	disposal_memsize = mem_size - desired_memsize;

	p =  m_allocate ( disposal_memsize );

	if ( p == NULL )
	{
		_pprintf ( "Couldn't Allocate %d  out of %d Bytes\n",
			disposal_memsize, mem_size );
		tw_exit (0);
	}
}

mavail ( tblocks, cblocks)

	Uint *tblocks, *cblocks;
{
	static int one_time = 0;

	if ( one_time == 0 )
	{
		*tblocks = *cblocks = mem_size / BLOCK_SIZE;
		one_time = 1;
	}
	else
	{
		*tblocks = *cblocks = 0;
	}
}

tw_getmem ( addr)

	Byte ** addr;
{
	static int one_time = 0;

	if ( one_time == 0 )
	{
		*addr =  (Byte *) (((int) mem + 3) & 0xFFFFFFFC);
		one_time = 1;
		return ( SUCCESS );
	}
	else
	{
		_pprintf ( "GETMEM called twice\n" );
		crash ();
		return ( FAILURE );
	}
}

