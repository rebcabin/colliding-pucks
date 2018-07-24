

#ifndef FUNCTION
#define FUNCTION
#endif

/*-------------------------------------------------------
   Shifts to get physical node numbers out of physical addresses
   NMASK is node mask for the tc2000
   NSHIFT is the node to memory shift and is different for the tc2000 and gp1000
   BFLY2 is the tc2000 and BFLY1 is the gp1000
   data from D. Rich and from BBN
   note that the below defaults to BFLY1 unless BFLY2 is defined
*--------------------------------------------------------*/

#ifdef BFLY2
#define NMASK 0x1f800000
#define NSHIFT 23
#define node_of_addr(addr) ((((unsigned)(addr)) & NMASK) >> NSHIFT)
#else
#define NSHIFT 24
#define node_of_addr(addr) ((( unsigned)(addr)) >> NSHIFT)
#endif

/*-------------------------------------------------------*/

#if defined(BF_MACH) && !defined(NOVMMAP)
/* 278 pages exactly what is needed for pucks */
#define MEMSIZE 296 * 8 * 1024

#include <mach.h>
#include <sys/vm_mapmem.h>
#include <stdio.h>

static vm_address_t vAddress;
static char *memstart, *memend, *curmem;
int MyNode;

extern void sim_debug();

/*--------------------------------------------------------------------------*
   Memory Block Header -- assumes we are living in a 32 bit world
*--------------------------------------------------------------------------*/
typedef union uName
{
    struct
    {
        union uName * next; /* next free block-zero when allocated */
	union uName * prev; /* prev free block-zero when allocated */
        int size;           /* size of this free block in MemBlockHdr's */
	int debug_check;    /* simple overwrite check- MAGICD when allocated */
    } s;

    double alignment_help;  /* so (double *)sim_malloc works on sparc */

} MemBlockHdr;

#define UNITSIZE sizeof(MemBlockHdr)
#define MAGICD  1598971726
#define NULL 0

MemBlockHdr base;		/* empty list to get started */
MemBlockHdr * last_alloc;	/* last block allocated */
MemBlockHdr * last_release;	/* last block released */

FUNCTION char * sim_malloc ( n )
int n; /* in bytes */
{
    MemBlockHdr *p, *q;
    int nunits;

    q = last_alloc;
    if (q == NULL)
    {				/* create the free list first time through */
	fprintf (stderr, "This isn't happening\n" );
	base.s.next = base.s.prev = last_alloc = last_release = q = &base;
	base.s.size = 0;
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
	if (p->s.size >= nunits) /* Found first fit */
	{
	    if (p->s.size == nunits) /* Exact fit */
	    {		
		q->s.next = p->s.next;
		p->s.next->s.prev = q;

		if ( last_release == p )
		    last_release = q;
	    }
	    else /* Room left over */
	    {	
		p->s.size -= nunits;
		p += p->s.size;
		p->s.size = nunits;
	    }

	    p->s.debug_check = MAGICD;
	    p->s.next = p->s.prev = 0; /* allocated block */

	    last_alloc = q;	/* next time, start searching after here */
	    return  (char *)  (p + 1);	/* beyond header */
	}

	/* wrapped around end of free list */
	/* First time through, we get to here */
	if (p == last_alloc)
	{
	    if ( !morePages (nunits) )
	    {
		return NULL;
	    }
	}
    }
}

FUNCTION morePages (num_units)
int num_units;	/* in MemBlockHdr's */
{
    MemBlockHdr *hp;
    int pages_needed;
    vm_address_t addr = 0;
    int cluster_node_num = 0;

    num_units *= UNITSIZE; /* in Bytes */

    pages_needed = ( num_units <= vm_page_size )?
	1 : (num_units + vm_page_size - 1) / vm_page_size;

    num_units = pages_needed * vm_page_size / UNITSIZE; /* in MemBlockHdr's */

    if ( 0 != vm_mapmem ( task_self(), &addr, pages_needed * vm_page_size,
	 VM_MAPMEM_ALLOCATE  | VM_MAPMEM_ANYWHERE, 
		0, 0, cluster_node_num ) )
    {
	fprintf (stderr,"morePages: out of memory\n" );
	sim_debug("in sim-debug");
	return 0;
    }

    hp = (MemBlockHdr *) addr;
    hp->s.size = num_units;
    hp->s.next = hp->s.prev = 0;
    hp->s.debug_check = MAGICD;
    sim_free (hp + 1);

    fprintf (stderr, "%d more pages of memory allocated\n", pages_needed );
    return 1;
}

FUNCTION sim_free (p)
MemBlockHdr *p;	/* address of memory to be deallocated */
{
    MemBlockHdr *q, *r, *after_q;

    /*
     * Hunt through the free list for the proper place to put the block,
     * maintaining the list sorted by address value 
     */

    r = (MemBlockHdr *) p - 1;	/* point to block header */

    if (  r->s.next ||  r->s.prev || (r->s.debug_check != MAGICD) )
    {
	fprintf (stderr, "sim_free: Munged block header\n" );
	fprintf (stderr," next: %d, prev: %d, dbg: %d\n",r->s.next,r->s.prev, r->s.debug_check);
	sim_debug("in sim-debug");
    }

    /* Move q until r is between q and the block after q */
    q = last_release;
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
    if (r + r->s.size == after_q)
    {
	r->s.size += after_q->s.size;
	r->s.next = after_q->s.next;
	after_q->s.next->s.prev = r;
	if ( last_alloc == after_q )
	    last_alloc = q;
    }
    else
    {
	r->s.next = after_q;
	after_q->s.prev = r;
    }

    /* fit block to its upper neighbor */
    if (q + q->s.size == r)
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

    last_release = q;
}


FUNCTION init_sim_mem ()
{
    kern_return_t ret_error;
    int cluster_node_num = 0;
    char *malloc();
    MemBlockHdr *base;

    vAddress = 0;
/*    MyNode = getphysaddr ( vAddress )  >> 24; */
    MyNode = node_of_addr(getphysaddr ( vAddress ));

/*
    fprintf (stderr, "node %x vm_mapmeming %d pages of unwired virtual memory\n",
	MyNode, MEMSIZE / vm_page_size );
*/
    if ( 0 != (ret_error = vm_mapmem ( task_self(), &vAddress, MEMSIZE,
/*	 VM_MAPMEM_WIRE | Wires the memory down, but must be in wheel group */
	 VM_MAPMEM_ALLOCATE  | VM_MAPMEM_ANYWHERE, 
		0, 0, cluster_node_num ) ) )
    {
	fprintf (stderr,"vm_mapmem failed error code %d\n", ret_error );
	sim_debug("in sim-debug");
	exit (1);
    }
#if 0
    vAddress = (vm_address_t) malloc ( MEMSIZE );
    if ( ! vAddress )
	fprintf (stderr, "malloc failed\n" );
#endif

    memstart = curmem = (char *) vAddress;
    memend = memstart + MEMSIZE;

    touch_everything();
    node_map();

    base = (MemBlockHdr *) vAddress;
    base->s.next = base->s.prev = last_alloc = last_release = base;
    base->s.size = MEMSIZE / UNITSIZE;
}

extern char etext, end;

FUNCTION touch_everything ()
{
    char temp;
    char *p;

    /* read the text area */
    for ( p = (char *) 0; p < &etext;
        p = (char *)((unsigned long)  p + vm_page_size) )
    {
        temp = *p;
    }

    /* note that p >= &etext so it points to the data area now */

    /* read and write back data area */
    for ( ; p < &end; p = (char *)((unsigned long)p + vm_page_size) )
    {
        temp = *p;
        *p = temp;
    }

    /* read and write back Time Warp's heap */
    for ( p = memstart; p < memend;
	p = (char *)((unsigned long)p + vm_page_size) )
    {
        temp = *p;
        *p = temp;
    }

}

FUNCTION node_map()
{
    char *p;
    int t_on, t_off, d_on, d_off, m_on, m_off;

    t_on = t_off = d_on = d_off = m_on = m_off = 0;

    for ( p = (char *) 0; p <= &etext; p += vm_page_size )
/*	(getphysaddr ( p )>>24) == MyNode ? t_on++ : t_off++; */
	(node_of_addr(getphysaddr ( p ))) == MyNode ? t_on++ : t_off++;

    for ( p = &etext + vm_page_size - 1; p <= &end; p += vm_page_size )
/*	(getphysaddr ( p )>>24) == MyNode ? d_on++ : d_off++; */
	(node_of_addr(getphysaddr ( p ))) == MyNode ? d_on++ : d_off++;

    for ( p = memstart; p < memend; p += vm_page_size )
/*	(getphysaddr ( p )>>24) == MyNode ? m_on++ : m_off++; */
	(node_of_addr(getphysaddr ( p ))) == MyNode ? m_on++ : m_off++;

    if ( t_off || d_off || m_off )
    {
	fprintf (stderr, "Program is using OFF node memory!!!\n" );
	fprintf (stderr, "text: %don %doff data: %don %doff malloc: %don %doff\n",
	    t_on, t_off, d_on, d_off, m_on, m_off );
    }
}
#else BF_MACH
char *malloc();

FUNCTION char * sim_malloc ( size )
unsigned int size;
{
    return malloc ( size );
}

FUNCTION sim_free ( pointer )
char * pointer;
{
    free ( pointer );
}
#endif

