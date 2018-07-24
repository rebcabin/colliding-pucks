/*      Copyright (C) 1989, 1991, California Institute of Technology.
		U. S. Government Sponsorship under NASA Contract NAS7-918
		is acknowledged.        */

/*
 * $Log:	list.c,v $
 * Revision 1.7  91/12/27  08:42:32  reiher
 * added code to support a free pool of truncated states
 * 
 * Revision 1.6  91/11/01  09:29:03  pls
 * 1. Change ifdef's, version id.
 * 2. Better management of message pool (SCR 197).
 * 
 * Revision 1.5  91/07/17  15:09:01  judy
 * New copyright notice.
 * 
 * Revision 1.4  91/06/07  13:43:20  configtw
 * Allocate initial chunk for msg_free_pool.
 * 
 * Revision 1.3  91/06/03  12:24:28  configtw
 * Tab conversion.
 * 
 * Revision 1.2  91/05/31  13:23:13  pls
 * Modify PARANOID code to check for emergbuf case.
 * 
 * Revision 1.1  90/08/07  15:39:53  configtw
 * Initial revision
 * 
*/
char list_id [] = "@(#)list.c   $Revision: 1.7 $\t$Date: 91/12/27 08:42:32 $\tTIMEWARP";


/*

Purpose:

		list.c contains all code for handling lists of all types used
		by Time Warp.  Such lists include every object's input and
		output message queues, and the list of states saved by Time
		Warp for each process.  From the point of view of the Time
		Warp systems programmer, these lists should be simple, linear
		lists, but he should be able to manipulate the lists easily.
		By calling appropriate routines, the systems programmer should 
		be able to create lists, add elements to lists, delete elements 
		from lists, and traverse lists.  He should not have to worry 
		about details of the data structures used to store the lists.

		The implication of the above is that the routines contained
		in this module should be completely self-contained list
		handling mechanisms.  All interactions with the list should
		go through functions in this module.

		A second implication is that the actual data structure 
		supporting the lists can be changed merely by changing
		the code in this module.  (Plus some code in .h files.)
		The current implementation of lists is as circular,
		doubly linked lists.  That implementation could be changed
		to a tree, or a heap, or even to an actual contiguous list,
		merely by changing code here.  In this sense, this module
		is less a part of Time Warp itself than it is a lower layer
		of software supporting Time Warp.

		The list management functions for adding and deleting
		list elements allow the systems programmer to put elements
		anywhere in a list.  This ability must be a feature of
		any implementation of the list management facilities.

		The elements of a list need not be of constant size.  The
		systems programmer may ask the list processing primitive
		functions to add any sized element to any list.
		Checking the correctness of doing so is the systems
		programmer's responsibility, not the list management
		functions'.  

		The list management code should take care of any 
		necessary allocation or deallocation of memory.

Functions:

		l_create(size) - create a list element of the requested
				size.  Do not, as yet, put it in any list.  
				Paramters - int size
				Return - pointer to an empty space for the element
				if successful, pointer to NULL if failure.

		l_destroy(element) - destroy the named element, freeing
				its space for other uses.
				Parameters - Byte *element
				Return - SUCCESS usually, possibly FAILURE if
				compiled with the PARANOID option, and something
				goes wrong.

		l_hcreate() - create a new list header.  In effect, initialize
				a new list.
				Parameters - none
				Return - a pointer to the header, or to NULL, if the call
				fails.

		l_hdestroy(header) - destroy the named list header, freeing its
				space for other uses.  In essence, destroy the 
				associated list.  Only empty lists can be destroyed.
				Parameters -  Byte *header
				Return -  SUCCESS or FAILURE

		l_insert(position, element) - insert the element into the list
				position pointed to by "position".
				Parameters - Byte *position, *element
				Return - SUCCESS or FAILURE

		l_ishead(element) - check to see if the named element is a 
				list header.
				Parameters - Byte *element
				Return - 0 or ~0

		l_maxelt() - call the memory manager to find out how large an
				element could be created, given the current state of
				memory.
				Parameters - none
				Return - # of contiguous bytes available to hold data

		l_next(element) - find the element that follows the named
				element in its list.
				Parameters - Byte *element
				Return - pointer to the next element (Byte *), or to
						NULL if the call failed

		l_prev(element) - find the element that preceeds the named
				element in its list.
				Parameters - Byte *element
				Return - pointer to the previous element (Byte *), or to
						NULL if the call failed

		l_remove(element) - remove the named element from its list.
				Do not, at this point, free its storage, however.
				Parameters - Byte *element
				Return - pointer to the removed element (Byte *), or to
						NULL if the call failed

		Note:  "size" is an integer, all other parameters listed
				above are pointers to the appropriate objects.

Implementation:

		The particular interface in place, as stated earlier, is
		a form of linked list.  Every list in the system has
		a special header element.  This header element is not
		available for system programmer inspection directly (or
		shouldn't be).  It is used only by the code in list.c,
		as a starting point for lists.  The process of creating
		a new list consists largely of creating its header, and
		appropriately initializing it.  Destruction of a list
		would be completed when the list header was released.

		This diagram illustrates a typical element in one of
		the linked lists managed by this code.

									          ^       ^
									          |       |
									    +-----|-------|--+
									    | next. | prev.  |
		Area used for list       -----> |----------------|
		   management only              |     size       |
									    +----------------+
									    | data area      |
									    |                |
		Area visible to the rest -----> |                |
		of Time Warp                    |                |
									    |                |
									    +----------------+

		An unfortunate choice in the terminology used in the
		current implementation has allowed potential confusion
		about the meaning of the term "header" in regard to
		lists.  The term has two meanings.  One type of header
		is that discussed above, a special header node for each
		list.  The other is a block of list control information
		kept for every list element.  (Included in this block
		are items like the forward and back links.)  Perhaps
		consistently referring to the first type as "list
		headers", and the second as "list element headers",
		or "element headers", might be clearer.

		The following diagram illustrates the difference between
		the two types of headers.


								+----------- list element headers
								|                      /
								|                     /
							   \ /                   /
								.                   /
			   +------+     +------+     +------+  /
			   |    --|---->|    --|---->|      |\/_
			   |      |<----|--    |<----|--    |
			   +------+     +------+     +------+
				   .        |      |     |      |
				  /|\       |      |     |      |
				   |        |      |     |      |
				   |        +------+     +------+
				   |            .           .
				   |           / \         / \
				   |            |           |
				List Header     List elements

		Note that the list header for this list consists only of
		a list element header.  (List header do not contain any
		data.)

		Should the underlying implementation of lists be changed,
		be sure to update the comments in this section of file.c.
		Most of the earlier comments should only change if there 
		is a change in the interface to the list management
		code.

*/


#include "twcommon.h"
#include "twsys.h"


/* Here are the things you can test about pointers to list elements and */
/* things you can do with them                                          */
/* definition: user pointer: pointer to contiguous area available for   */
/* use by the user.  header: hidden above the users pointer, not        */
/* available for users (hidden at lower addresses)                      */


#define HEADER(u)       u-1     /* header given user pointer */
#define USERPTR(h)      h+1     /* user pointer given header */

#define ISSING(l)       ((l)->next==(l) && (l)->prev==(l))
#define ISHEAD(l)       ((l)->size==0)
#define ISVALID(l)      ((l)!=NULL)
#define ISRELEASED(l)   ((l)->prev == NULL && (l)->size == 0)
#define RELEASE(l)      ((l)->prev = NULL,(l)->size = 0)  /* secret code */
#define MAKESING(l)     ((l)->next=(l),(l)->prev=(l))   /* point to self */
#define MAKEHEAD(l)     ((l)->size=0)   /* indicate queue header */
#define SETSIZE(l,s)    ((l)->size=(s))
#define SETNEXT(l,n)    ((l)->next=(n))
#define SETPREV(l,p)    ((l)->prev=(p))


/****************************************************************************
********************************   L_CREATE   *******************************
*****************************************************************************

	This function creates a new list item of the requested size
	as a singleton. It returns a pointer to the new item, or
	NULL if it is unable to create the item. Any memory which is
	required is allocated. 
	*/

#define INITMSGBUFFS 500
#define MAXFREEDFLT 200
#define INITTSTATEBUFFS 1000
#define MAXFREETSTATES 200

List_hdr * msg_free_pool = 0;
int msg_free_pool_size = 0;
int init_free_msg_buffs = INITMSGBUFFS;
int maxFreeMsgs = MAXFREEDFLT;
int max_msg_buffers_alloc = INITMSGBUFFS;
int msg_buffers_alloc = INITMSGBUFFS;
int msg_buffers_released = 0;
int num_msg_buffs_released_from_pool= 0;

List_hdr * truncStateFreePool = 0;
int truncStateFreePoolSize = 0;
int initTruncStateBuffs = INITTSTATEBUFFS;
int maxFreeTruncStates = MAXFREETSTATES;
int truncStateAlloc = INITTSTATEBUFFS;
extern long critEnabled;
List_hdr * firstTruncStateBuff, * lastTruncStateBuff;

/*  hwm here stands for "high water mark".  This variable controls how
	many msg buffers will be kept in the free list even if a memory request
	wants to deallocate some of them. */

int free_msg_buffs_hwm = 500;

List_hdr * free_pool = 0;
int free_pool_size = 0;

List_hdr *first_msg_buffer, *last_msg_buffer;
int free_pool_total_size;

FUNCTION List_hdr *l_create (reqsize)
	int            reqsize;     /* size of the list item to be created */
{
	register List_hdr *buffer;
	register List_hdr *head;
    register List_hdr *msg;

    int         i;

	if ( reqsize == msgdefsize )
	{
		register List_hdr * p;

		if ( msg_free_pool == 0 )
            {   /* set up list header and grab inital allocation */
            msg_free_pool = l_hcreate ();
            for (i = 1; i <= init_free_msg_buffs; i++)
                {
                msg = (List_hdr *) m_allocate ( reqsize + sizeof (List_hdr));

				if ( i == 1 )
				{
					first_msg_buffer = msg;
				}	
				else
				if ( i == init_free_msg_buffs )
				{
					last_msg_buffer = msg;
				}
					
                if (msg != NULL)
                    {
                    head = msg;
                    MAKESING (head);
                    SETSIZE (head, reqsize);
                    msg = USERPTR (head);
                    l_destroy(msg);     /* put in msg pool */
                    }
                }       /* for i... */

			/*  Check to see if all allocated buffers have addresses
				between the first and last allocated.  For any not in that
				range, print a message. */

#if PARANOID
			if ( tw_node_num == 0 )
			{
				
				Msgh * temp;
				for ( temp = l_next_macro ( msg_free_pool);
					  temp != msg_free_pool;
					  temp = l_next_macro ( temp ))
				{
					if ( temp < last_msg_buffer || 
						 temp > (first_msg_buffer + sizeof ( Msgh )))
						_pprintf("initial buffer %x outside of range %x to %x\n",							temp, last_msg_buffer, first_msg_buffer );
				}


			}
#endif PARANOID
	
            }   /* if (msg_free_pool == 0) */

		p = l_next_macro ( msg_free_pool );
		if ( p != msg_free_pool )
		{
			l_remove (p);
			msg_free_pool_size--;
			return (p);
		}
	}
	else
	if ( reqsize == truncStateSize  && critEnabled == TRUE )
	{
		register List_hdr * p;
		truncState * tState;

		if ( truncStateFreePool == 0 )
            {   /* set up list header and grab inital allocation */
            	truncStateFreePool = l_hcreate ();
            	for (i = 1; i <= initTruncStateBuffs; i++)
                {
                    tState = (List_hdr *) m_allocate ( reqsize + sizeof (List_hdr));

					if ( i == 1 )
					{
						firstTruncStateBuff = tState;
					}	
					else
					if ( i == initTruncStateBuffs )
					{
						lastTruncStateBuff = tState;
					}
					
                	if (tState != NULL)
                    {
                    head = tState;
                    MAKESING (head);
                    SETSIZE (head, reqsize);
                    tState = USERPTR (head);
                    l_destroy(tState);     /* put in msg pool */
                    }
                }       /* for i... */

            }   /* if (truncStateFreePool == 0) */

		p = l_next_macro ( truncStateFreePool );
		if ( p != truncStateFreePool )
		{
			l_remove (p);
			truncStateFreePoolSize--;
			return (p);
		}
	}
	else

	if ( reqsize == objstksize )
	{
		register List_hdr * p;

		if ( free_pool == 0 )
			free_pool = l_hcreate ();

		p = l_next_macro ( free_pool );
		if ( p != free_pool )
		{
			l_remove (p);
			free_pool_size--;
			return (p);
		}
	}

	buffer = (List_hdr *) m_allocate ( reqsize + sizeof (List_hdr));

	if (buffer != NULL)
	{
		head = buffer;
		MAKESING (head);
		SETSIZE (head, reqsize);
		buffer = USERPTR (head);

		/*  If it's a message sized request and we haven't satisfied it from
			the free pool of msg buffers, then effectively we are about to add
			a buffer to the free pool.  Also, check to see if we have hit a
			new maximum on buffers in use. */

		if ( reqsize == msgdefsize )
		{
			msg_buffers_alloc++ ;

			if ( msg_buffers_alloc > max_msg_buffers_alloc )
				max_msg_buffers_alloc = msg_buffers_alloc ;
		
		}
	}

	return (buffer);
}


/****************************************************************************
********************************   L_DESTROY   ******************************
*****************************************************************************

	This function deallocates a list element which is not
	currently part of a list other than the one which contains
	only itself (that is, it must be a singleton). It returns
	SUCCESS if all went well, or FAILURE in the event of an
	error. 
	*/

FUNCTION l_destroy (p)
	List_hdr *p;        /* pointer to list item to be deallocated */
{
	register List_hdr       *head;
	register Msgh			*msg;

	head = HEADER (p);  /* convert to header pointer */

	if ( head->size == msgdefsize )
		{   /* the size indicates it's a message */
		msg = p;
		if ( (msg_free_pool_size >= maxFreeMsgs) && 
			(msg < last_msg_buffer || msg > first_msg_buffer) )
			{	/* release back to main memory heap */
			m_release ((Mem_hdr *) head);
			msg_buffers_alloc--;
			num_msg_buffs_released_from_pool++;
			}
		else
			{	/* release back to msg_free_pool */
			l_insert ( msg_free_pool, p );  /* put p in pool */
			msg_free_pool_size++;
			}
		return;
		}
	else
	if ( head->size == truncStateSize && critEnabled == TRUE )
		{   /* the size indicates it's a truncated state */
			truncState *tState;
		tState = p;
		if ( (truncStateFreePoolSize >= maxFreeTruncStates) && 
			(tState < lastTruncStateBuff || tState > firstTruncStateBuff) )
			{	/* release back to main memory heap */
			m_release ((Mem_hdr *) head);
			}
		else
			{	/* release back to msg_free_pool */
			l_insert ( msg_free_pool, p );  /* put p in pool */
			truncStateFreePoolSize++;
			}
		return;
		}
	else
	if ( head->size == objstksize )
		{   /* it's an object */
		l_insert ( free_pool, p );      /* put p in this pool */
		free_pool_size++;
		return;
		}

#if PARANOID

	if (ISVALID (p)
		&& ISSING (head)
		&& !ISHEAD (head)
		)
	{

		RELEASE (head);         /* mark the element in case the user tries to
								 * insert this destroyed block later -- used
								 * to detect pointers to released memory */
#endif

/* if none of the above, just release it */
		m_release ((Mem_hdr *) head);
		return;

#if PARANOID
	}

	else
	{
		twerror ( "l_destroy F invalid list element %x", p );
		tester ();
	}
#endif
}


/****************************************************************************
********************************   L_HCREATE   ******************************
*****************************************************************************

	This function creates a new list header. It returns a
	pointer to the new header, or NULL if it is unable to create
	the header. Any memory which is required is allocated. 
	 */

FUNCTION List_hdr  *l_hcreate ()
{
	List_hdr       *buffer;
	List_hdr       *head;


	buffer = (List_hdr *) m_allocate ( sizeof (List_hdr));

	if (buffer != NULL)
	{
		head =  buffer;
		MAKESING (head);        /* points to itself when empty */
		MAKEHEAD (head);        /* make size 0 */

		return USERPTR (head);  /* point to just past the q header */
	}

	else
	{
		return NULL;
	}

}


/****************************************************************************
********************************   L_HDESTROY   *****************************
*****************************************************************************

	This function deallocates a list header which does not currently have
	any members other than itself. It verifies that the input is truly
	a list header, and not an ordinary list element. It returns
	SUCCESS if all went well, or FAILURE in the event of an error. 
	*/

FUNCTION l_hdestroy (p)
	List_hdr *p;        /* pointer to list item to be deallocated */
{
	List_hdr       *head;


	head = HEADER (p);
	if (ISVALID (p)             /* not NULL */
		&& ISSING (head)        /* and a singleton */
		&& ISHEAD (head)        /* with size 0 */
		)
	{
		RELEASE (head);         /* mark the memory in case user tries to
								 * insert on this destroyed list later; used
								 * to detect pointers to released memory */

		m_release ((Mem_hdr *) head);

		return SUCCESS;
	}

	else
	{
		return FAILURE;
	}

}


/****************************************************************************
********************************   L_INSERT   *******************************
*****************************************************************************

	This function inserts an element into a list AFTER the indicated
	position. It returns SUCCESS if the operation succeeded, FAILURE
	otherwise. 
	*/

FUNCTION l_insert (pos, p)
	List_hdr *pos;      /* pointer to a member already in a list */
	List_hdr *p;        /* pointer to an element to be inserted after
								 * pos */
{
	register List_hdr       *insert,
				   *afterme,
				   *beforme;

#if PARANOID
if (!ISVALID (p) || !ISVALID (pos))
	{
		_pprintf ( "l_insert got a NULL\n" );
		tester();
	}
#endif


	insert = HEADER (p);
	afterme = HEADER (pos);
	beforme = afterme->next;

#if PARANOID

	if ((ISSING (insert) || (p == emergbuf))
		&& !ISHEAD (insert)
		&& !ISRELEASED (afterme)
		&& !ISRELEASED (insert)
		&& ((!ISSING (afterme)) || ISHEAD (afterme))
		)
#endif

	{
		SETNEXT (afterme, insert);      /* point afterme to insert */
		SETPREV (beforme, insert);      /* point beforeme back to insert */
		SETNEXT (insert, beforme);      /* set up insert's pointers */
		SETPREV (insert, afterme);

		return;
	}

#if  PARANOID
  else                  /* error checking failed */
	{
		_pprintf ( "l_insert failure: pos = %x, p = %x\n", pos, p );
		if ( !ISSING (insert) )
			_pprintf ( "!ISSING (insert) %x %x %d\n",
				insert->prev, insert->next, insert->size  );
		if ( ISHEAD (insert) )
			_pprintf ( "ISHEAD (insert) %x %x %d\n",
				insert->prev, insert->next, insert->size );
		if ( ISRELEASED (afterme) )
			_pprintf ( "ISRELEASED (afterme) %x %x %d\n",
				afterme->prev, afterme->next, afterme->size );
		if ( ISRELEASED (insert) )
			_pprintf ( "ISRELEASED (insert) %x %x %d\n",
				insert->prev, insert->next, insert->size );
		if ( ISSING (afterme) )
			_pprintf ( "ISSING (afterme) %x %x %d\n",
				afterme->prev, afterme->next, afterme->size );
		tester ();
		return;
	}
#endif


}


/****************************************************************************
********************************   L_ISHEAD   *******************************
*****************************************************************************

	This function is used to differentiate a list header from a list member.
	It returns TRUE if its input is a pointer to a list header, and FALSE
	otherwise. 
	*/

FUNCTION l_ishead (p)
	List_hdr       *p;
{
	List_hdr       *head;


	head = HEADER (p);

	return (

			ISVALID (p)         /* queue mgr depends on this check */
			&& ISHEAD (head)
			&& !ISRELEASED (head)

		);

	/*
	 * order of conditions is important for speed since this routine is a hot
	 * spot 
	 */
}


/****************************************************************************
********************************   L_MAXELT   *******************************
*****************************************************************************

	This function returns as its value the largest list element which
	could be created successfully if a call to l_create() or l_hcreate()
	were to be issued. The result is zero if there is not enough room
	for anything. */

FUNCTION int   l_maxelt ()
{
	int            contig;

	if ((contig = m_contiguous ()) < sizeof (List_hdr))
	{
		return 0;
	}

	else
	{
		return contig - sizeof (List_hdr);
	}

}


/****************************************************************************
********************************   L_NEXT   *********************************
*****************************************************************************

	This function returns a pointer to the list member which follows
	the input member. NULL is returned if the input is not part of a
	valid list. 
	*/

FUNCTION List_hdr  *l_next (p)
	List_hdr       *p;          /* pointer to list item */
{
	List_hdr       *head;


	head = HEADER (p);

	if (ISVALID (p))
	{
		return USERPTR (head->next);
	}

	else
	{
		return NULL;
	}

}


/****************************************************************************
********************************   L_PREV   *********************************
*****************************************************************************

	This function returns a pointer to the list member which precedes
	the input member. NULL is returned if the input is not part of a
	valid list. 
	*/

FUNCTION List_hdr  *l_prev (p)
	List_hdr       *p;          /* pointer to list member */
{
	List_hdr       *head;


	head = HEADER (p);

	if (ISVALID (p))
	{
		return USERPTR (head->prev);
	}

	else
	{
		return NULL;
	}

}


/****************************************************************************
********************************   L_REMOVE   *******************************
*****************************************************************************

	This function removes an element from a list, closing the list in
	behind it, and returning the pointer to the removed element as its
	value. A list header may not be removed from its list. 
	*/

FUNCTION List_hdr  *l_remove (p)
	List_hdr       *p;          /* pointer to the member to be removed */
{
	register List_hdr       *a,
				   *b,
				   *c;          /* assume list begins as a <-> b <-> c */

	/* and we're trying to l_remove b */


	b = HEADER (p);
	a = b->prev;
	c = b->next;

#if PARANOID

	if (ISVALID (p)
		&& !ISHEAD (b)          /* not allowed to remove the head */
		&& !ISSING (b)          /* not allowed to l_remove something twice */
		&& ISVALID (a) && ISVALID (c)
		&& !ISRELEASED (a) && !ISRELEASED (c)
		)
#endif

	{

		SETNEXT (a, c);         /* automatically makes a singleton out of the */
		SETPREV (c, a);         /* list head element if it is the only        */
		MAKESING (b);           /* element left in the list after removal     */

		return p;
	}
#if PARANOID
 else
	{
		return NULL;
	}
#endif

}

#if PARANOID
verify_free_msg_pool()
{

	Msgh * msg, *next;

	msg = msg_free_pool;
	if ( l_prev_macro ( l_next_macro ( msg ) ) != msg  ||
		 l_next_macro ( l_prev_macro ( msg ) ) != msg )
	{
		_pprintf (" msg pool corrupted near header %x\n",msg_free_pool);
		tester();
	}
	for ( msg = l_next_macro ( msg_free_pool ) ; msg != msg_free_pool;
				msg = l_next_macro ( msg ) )
	{
		if ( l_prev_macro ( l_next_macro ( msg ) ) != msg  ||
		 	l_next_macro ( l_prev_macro ( msg ) ) != msg )
		{
			_pprintf("msg pool corrupted near %x\n",msg );
			tester();
		}
		
	}
}
#endif
