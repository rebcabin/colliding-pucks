/* The functions in this file marked Splay Routine are not copyrighted by
 *  California Institute of Technology. See copyright notice below for
 *  the remainder of this file.
 *
 *  Splay routine code: 
 *  This code was written by:
 *  Douglas W. Jones with assistance from Srinivas R. Sataluri
 *
 *  Translated to C by:
 *    David Brower, daveb@rtech.uucp
 *
 *  Modified  by:
 *    J. Wedel  9/20/88
 *
 */

/* "Copyright (C) 1989, California Institute of Technology. 
     U. S. Government Sponsorship under NASA Contract 
   NAS7-918 is acknowledged." */



/*************************************************************************
 *
 *  twqueues.c
 *
 *  Modified  splay routine slightly and integrated with Time Warp simulator
 *  by J. Wedel  9/20/88
 *
 *  Made it work by fixing spdeq JJW 12/27/88
 *  added pr_delete_messages to this file. It is now q specific 5/16/89
 *  Fixed dequeueing error at time 'now' 7/3/89
 ***************************************************************************/

static char  twqueueid[] = "@(#)twqueue.c	1.1\t4/18/88";

#ifdef BF_MACH          /* Surely this repetitive nonsense   */
#include <stdio.h>      /* can be fix by a properly worked   */
#endif                  /* Makefile?                         */
#ifdef BF_PLUS
#include <stdio.h>
#endif

#ifdef SUN
#include <stdio.h>
typedef int     time_t;
#endif SUN

#ifdef MARK3
#include "stdio.h"
#endif MARK3

#include "twcommon.h"
#include "machdep.h"
#include "tws.h"
#include "twsd.h"
#include "twqueues.h"

extern void error();
extern void sim_debug();
extern char* sim_malloc();

extern int mesgdefsize;     /* packet length */

/* global variables for this module */

int last_q;
SPTREE  *pq;
mesg_block	*save_blk;	/* static storage for get_msg_pointer */
mesg_block	*save_blk2;	/* see find_next_event */
VTime hq_this_vtime;	/* set ONLY by find_next_event */
/* emq_first_ptr;   defined in twsd, points to start of free list */
/* emq_current_ptr;   defined in twsd, pointer to current event */
/* emq_next_ptr; defined in twsd,  not used   */
/* emq_prior_ptr; defined in twsd, not used  */
/* emq_endmsg_ptr; ptr to end of the free list. used in pr_delete_messages */


/****************************************************************
 *
 * Splay Routine
 * spinit() -- initialize an empty splay tree
 *
 ********************************************************************/
FUNCTION SPTREE *spinit()
{
    SPTREE * q;

    q = (SPTREE *) sim_malloc( sizeof( *q ) );
    if (q == 0) 
	{
	printf("can't allocate a queue\n");
	exit(1);
	}

    q->deqs = 0;
    q->deqloops = 0;
    q->enqs = 0;
    q->enqcmps = 0;
    q->splays = 0;
    q->splayloops = 0;
    q->root = NULL;
    return( q );
}

/***********************************************************************
 *
 * Splay Routine
 * spempty() -- is an event-set represented as a splay tree empty?
 *************************************************************************/

FUNCTION int spempty( q )

SPTREE *q;

{
    return( q == NULL || q->root == NULL );
}


/**********************************************************************
 *
 * Splay Routine
 *  spenq() -- insert item in a tree.
 *
 *  put n in q after all other nodes with the same key; when this is
 *  done, n will be the root of the splay tree representing q, all nodes
 *  in q with keys less than or equal to that of n will be in the
 *  left subtree, all with greater keys will be in the right subtree;
 *  the tree is split into these subtrees from the top down, with rotations
 *  performed along the way to shorten the left branch of the right subtree
 *  and the right branch of the left subtree
 *************************************************************************/
FUNCTION mesg_block *spenq( n, q )

register mesg_block * n;
register SPTREE * q;

{
    register mesg_block * left;	/* the rightmost node in the left tree */
    register mesg_block * right;	/* the leftmost node in the right tree */
    register mesg_block * nextbl;	/* the root of the unsplit part */
    register mesg_block * temp;


    q->enqs++;
    n->uplink = NULL;
    nextbl = q->root;
    q->root = n;
    if( nextbl == NULL )	/* trivial enq */
    {
        n->prior = NULL;
        n->next = NULL;
    }
    else		/* difficult enq */
    {
        left = n;
        right = n;

        /* n's left and right children will hold the right and left
	   splayed trees resulting from splitting on n->key;
	   note that the children will be reversed! */

	q->enqcmps++;
        if ( q_find( nextbl, n ) > 0 )
	    goto two;

    one:	/* assert nextbl->key <= key */

	do	/* walk to the right in the left tree */
	{
            temp = nextbl->next;
            if( temp == NULL )
	    {
                left->next = nextbl;
                nextbl->uplink = left;
                right->prior = NULL;
                goto done;	/* job done, entire tree split */
            }

	    q->enqcmps++;
            if( q_find( temp, n ) > 0 )
	    {
                left->next = nextbl;
                nextbl->uplink = left;
                left = nextbl;
                nextbl = temp;
                goto two;	/* change sides */
            }

            nextbl->next = temp->prior;
            if( temp->prior != NULL )
	    	temp->prior->uplink = nextbl;
            left->next = temp;
            temp->uplink = left;
            temp->prior = nextbl;
            nextbl->uplink = temp;
            left = temp;
            nextbl = temp->next;
            if( nextbl == NULL )
	    {
                right->prior = NULL;
                goto done;	/* job done, entire tree split */
            }

	    q->enqcmps++;

	} while( q_find( nextbl, n ) <= 0 );	/* change sides */

    two:	/* assert nextbl->key > key */

	do	/* walk to the left in the right tree */
	{
            temp = nextbl->prior;
            if( temp == NULL )
	    {
                right->prior = nextbl;
                nextbl->uplink = right;
                left->next = NULL;
                goto done;	/* job done, entire tree split */
            }

	    q->enqcmps++;
            if( q_find( temp, n ) <= 0 )
	    {
                right->prior = nextbl;
                nextbl->uplink = right;
                right = nextbl;
                nextbl = temp;
                goto one;	/* change sides */
            }
            nextbl->prior = temp->next;
            if( temp->next != NULL )
	    	temp->next->uplink = nextbl;
            right->prior = temp;
            temp->uplink = right;
            temp->next = nextbl;
            nextbl->uplink = temp;
            right = temp;
            nextbl = temp->prior;
            if( nextbl == NULL )
	    {
                left->next = NULL;
                goto done;	/* job done, entire tree split */
            }

	    q->enqcmps++;

	} while( q_find( nextbl, n ) > 0 );	/* change sides */

        goto one;

    done:	/* split is done, branches of n need reversal */

        temp = n->prior;
        n->prior = n->next;
        n->next = temp;
    }

    return( n );

} /* spenq */


/*----------------
 *
 * Splay Routine
 *  spdeq() -- return and remove head node from a tree.
 *
 *  remove and return the head node from the node set; this deletes
 *  (and returns) the leftmost node from q, replacing it with its right
 *  subtree (if there is one); on the way to the leftmost node, rotations
 *  are performed to shorten the left branch of the tree
 *  modified to work on a tree, not a subtree by  JJW
 */
FUNCTION mesg_block *spdeq( q )

	 SPTREE *q;


{
    register mesg_block * n;
    register mesg_block * deq;		/* one to return */
    register mesg_block * nextbl;       /* the next thing to deal with */
    register mesg_block * left;      	/* the left child of next */
    register mesg_block * farleft;		/* the left child of left */
    register mesg_block * farfarleft;	/* the left child of farleft */

    n = q->root;
    q->deqs++;
    if( n == NULL )
    {
        deq = NULL;
    }
    else
    {
	/* if left is null this is the deq block but also we have
	to reset the root.  JJW */
        nextbl = n;
        left = nextbl->prior;
        if( left == NULL )
	{
            deq = nextbl;
            n = nextbl->next;
            if( n != NULL )
		{
		n->uplink = NULL;
		q->root = n;
		}
	    else
		q->root = NULL;
        }
	else for(;;)
	{
            /* next is not it, left is not NULL, might be it */
	    q->deqloops++;
            farleft = left->prior;
            if( farleft == NULL )
	    {
                deq = left;
                nextbl->prior = left->next;
                if( left->next != NULL )
		    left->next->uplink = nextbl;
		break;
            }

            /* next, left are not it, farleft is not NULL, might be it */
            farfarleft = farleft->prior;
            if( farfarleft == NULL )
	    {
                deq = farleft;
                left->prior = farleft->next;
                if( farleft->next != NULL )
		    farleft->next->uplink = left;
		break;
            }

            /* next, left, farleft are not it, rotate */
            nextbl->prior = farleft;
            farleft->uplink = nextbl;
            left->prior = farleft->next;
            if( farleft->next != NULL )
		farleft->next->uplink = left;
            farleft->next = left;
            left->uplink = farleft;
            nextbl = farleft;
            left = farfarleft;
	}
    }

    return( deq );

} /* spdeq */


/*----------------
 *
 * Splay Routine
 *  splay() -- reorganize the tree.
 *
 *  the tree is reorganized so that n is the root of the
 *  splay tree representing q; results are unpredictable if n is not
 *  in q to start with; q is split from n up to the old root, with all
 *  nodes to the left of n ending up in the left subtree, and all nodes
 *  to the right of n ending up in the right subtree; the left branch of
 *  the right subtree and the right branch of the left subtree are
 *  shortened in the process
 *
 *  this code assumes that n is not NULL and is in q; it can sometimes
 *  detect n not in q and complain
 */

FUNCTION void splay( n, q )

register mesg_block * n;
SPTREE * q;

{
    register mesg_block * up;	/* points to the node being dealt with */
    register mesg_block * prev;	/* a descendent of up, already dealt with */
    register mesg_block * upup;	/* the parent of up */
    register mesg_block * upupup;	/* the grandparent of up */
    register mesg_block * left;	/* the top of left subtree being built */
    register mesg_block * right;	/* the top of right subtree being built */

    left = n->prior;
    right = n->next;
    prev = n;
    up = prev->uplink;

    q->splays++;

    while( up != NULL )
    {
	q->splayloops++;

        /* walk up the tree towards the root, splaying all to the left of
	   n into the left subtree, all to right into the right subtree */

        upup = up->uplink;
        if( up->prior == prev )	/* up is to the right of n */
	{
            if( upup != NULL && upup->prior == up )  /* rotate */
	    {
                upupup = upup->uplink;
                upup->prior = up->next;
                if( upup->prior != NULL )
		    upup->prior->uplink = upup;
                up->next = upup;
                upup->uplink = up;
                if( upupup == NULL )
		    q->root = up;
		else if( upupup->prior == upup )
		    upupup->prior = up;
		else
		    upupup->next = up;
                up->uplink = upupup;
                upup = upupup;
            }
            up->prior = right;
            if( right != NULL )
		right->uplink = up;
            right = up;

        }
	else				/* up is to the left of n */
	{
            if( upup != NULL && upup->next == up )	/* rotate */
	    {
                upupup = upup->uplink;
                upup->next = up->prior;
                if( upup->next != NULL )
		    upup->next->uplink = upup;
                up->prior = upup;
                upup->uplink = up;
                if( upupup == NULL )
		    q->root = up;
		else if( upupup->next == upup )
		    upupup->next = up;
		else
		    upupup->prior = up;
                up->uplink = upupup;
                upup = upupup;
            }
            up->next = left;
            if( left != NULL )
		left->uplink = up;
            left = up;
        }
        prev = up;
        up = upup;
    }

# ifdef DEBUG
    if( q->root != prev )
    {
/*	fprintf(stderr, " *** bug in splay: n not in q *** " ); */
	abort();
    }
# endif

    n->prior = left;
    n->next = right;
    if( left != NULL )
	left->uplink = n;
    if( right != NULL )
	right->uplink = n;
    q->root = n;
    n->uplink = NULL;

} /* splay */

/*----------------
 *
 * Splay Routine
 * spfnext() -- fast return next higher item in the tree, or NULL.
 *
 *	return the successor of n in q, represented as a splay tree.
 *	This is a fast (on average) version that does not splay.
 */
FUNCTION mesg_block *spfnext( n )

register mesg_block * n;

{
    register mesg_block * nextbl;
    register mesg_block * x;

    /* a long version, avoids splaying for fast average,
     * poor amortized bound
     */

    if( n == NULL )
        return( n );

    x = n->next;
    if( x != NULL )
    {
        while( x->prior != NULL )
	    x = x->prior;
        nextbl = x;
    }
    else	/* x == NULL */
    {
        x = n->uplink;
        nextbl = NULL;
        while( x != NULL )
	{
            if( x->prior == n )
	    {
                nextbl = x;
                x = NULL;
            }
	    else
	    {
                n = x;
                x = n->uplink;
            }
        }
    }

    return( nextbl );

} /* spfnext */


/*----------------
 *
 * Splay Routine
 * spfhead() --	return the "lowest" element in the tree.
 *
 *	returns a reference to the head event in the event-set q.
 *	avoids splaying but just searches for and returns a pointer to
 *	the bottom of the left branch.
 */
FUNCTION mesg_block *spfhead( q )

register SPTREE * q;

{
    register mesg_block * x;

    if( NULL != ( x = q->root ) )
	while( x->prior != NULL )
	    x = x->prior;

    return( x );

} /* spfhead */

/* End of the Splay Routines. Copyright by California Institute of
   Technology from here to end of this file */

/****************************************************************************
*
*   set up the initial heap with first message
*   the message is located at mb_ptr
*   called by cs_send_mesg in config.c
****************************************************************************/ 
FUNCTION eq_initqueue()
  {

  fmsg_flag = FALSE;
  pq = spinit();
  mb_ptr->uplink = NULL;
  mb_ptr->next = NULL;
  mb_ptr->prior = NULL;
  pq->root = mb_ptr;
  last_q = 1;
  }


/***************************************************
*       
* cm_queue_event_message   - insert()
* called by evtmesg
* the message, as usual, is at mb_ptr
*****************************************************/
FUNCTION cm_queue_event_message()

   {
   if (fmsg_flag == TRUE) 
	eq_initqueue();
   else  {
	last_q++;
	if (last_q > test_var2) test_var2 = last_q; /* record max heap */
	spenq(mb_ptr,pq);		/* determines where to put it */
	}
   return(0);
   }

/*************************************************************************
*   find_next_event
*   -  get the next thing on the queue. It should have a new time or process
*   -  name  because setup messages got all items with same time/process.
*   -  does not dequeue the  message block.
*         
*   -  Dynamic create, destroy  implemented  9/6/88 
*
************************************************************************/
FUNCTION find_next_event()
{ 

   register mesg_block *el2;

   if (last_q == 0 || spempty(pq) ) return(FAILURE);

   el2 = spfhead(pq);  /* don't dequeue it now see setup */
   if (strcmp(el2->rname, "+obcreate") == 0)
	   emq_current_ptr = el2;
	else if (strcmp(el2->rname, "}obdestroy") == 0)
	   emq_current_ptr = el2;

	else if (pr_find_next_process
		(el2->rname) == FAILURE)
		   {
		   dm_one_message(el2, 0);
		   sim_debug("in sim_debug");
		   return(FAILURE);
		   }
		else 
		  { emq_current_ptr = el2;
		    hq_this_vtime = el2->rlvt;
		  }
        return(SUCCESS);
}

/*************************************************************************
* pr_setup_messages called by main_process.
* routine find_next_event has previously set up first thing found in queue 
* and some global pointers and variables.
* Now set up all the messages for this process at this time in mesq_ptr[]
* This dequeues the message blocks.
*   added flag argument to speed up check for obcreate obdestroy
*   msgs by setting the argument to 1.  (0 = normal msgs)
* 
************************************************************************/
FUNCTION pr_setup_messages(flag)
    int flag;
{
    mesg_block   *next_item, *deq_item, *this_item;

    mesg_lmt = 1;
    mesg_ind = -1;
    mesg_ptr[0] = emq_current_ptr;
    next_item= spfnext(emq_current_ptr);
    deq_item = spdeq(pq);
    if (deq_item != emq_current_ptr)
	printf("dryrot, setup for current pointer: deq=%d  this=%d\n",
		deq_item, emq_current_ptr);
    last_q--;
    if (flag == 1) return(SUCCESS);
    while(last_q > 0  &&  !spempty(pq) )
      {  
        if( next_item != NULL  && 
	    eqVTime(next_item->rlvt, hq_this_vtime)  &&
	    strcmp(emq_current_ptr->rname, next_item->rname) == 0)
	    {
	      if (mesg_lmt < MAXMSGS )
		mesg_ptr[mesg_lmt] = next_item;
	      else
	        { error("mesg ptr array overflow"); return (FAILURE);
	        }
	      this_item = next_item;
	      next_item= spfnext(this_item);
	      deq_item = spdeq(pq);
	      if (deq_item != this_item)
		{printf("dryrot, setup: deq=%d  this=%d\n",
			 deq_item, this_item);
		}
	      mesg_lmt++;
	      last_q--;
	    }
	else break;
      }
	 	   	
    obj_bod[gl_bod_ind].obj_msglimit = mesg_lmt;	
    obj_bod[gl_bod_ind].obj_cemsgs  += mesg_lmt;	
    return (SUCCESS);
}

/************************************************************************
*
*	pr_delete_messages - reclaim all message blocks that were
*			     received by the current object at the
*			     current virtual time
*
*	called by - main_process, pr_create_object, pr_destroy_object
*
*	- mark each message as deleted and enqueue all message sub-blocks
*	-    blocks (allows reallocation of all message sub-blocks)
*
*       - this version adds them to the free list at the end so the 
*	- debug print of DM del will have a better chance to see old
*	- blocks.
*       - The get_mesg_blocks routines gets the blocks from the front
*	- of the list.
*
*************************************************************************/

void FUNCTION pr_delete_messages()
{
    int             i;

    for (i = 0; i < mesg_lmt; i++)
    {
	mb_ptr_2 = mesg_ptr[i];
	mb_ptr_2->status = TW_DELETED;
	mb_ptr_2->next = NULL;
	if (emq_endmsg_ptr == NULL)
	    {
	     mb_ptr_2->prior = NULL;
	     emq_endmsg_ptr = mb_ptr_2;
	     emq_first_ptr = mb_ptr_2;
	    }
	else
	   {
	    mb_ptr_2->prior = emq_endmsg_ptr;
	    emq_endmsg_ptr->next = mb_ptr_2;
	    emq_endmsg_ptr = mb_ptr_2;
	   }
    }

}



/***************************************************************************
*
*  get_msg_pointer() 
*
*  gets the message pointer to the next message on the queue. Used by
*  the message display routines in twhelp.c
*  this splay version uses global variable save_blk.
*
****************************************************************************/
FUNCTION  mesg_block  *get_msg_pointer(indx,qindx)
int indx;
int qindx;
{
    mesg_block	*spelt;
    mesg_block     *mptr;

mptr = 0;
if ( indx == 1)
    	mptr = emq_first_ptr;
else if (indx == 2)
	{
	save_blk = spfhead(pq);
	if (save_blk != NULL)
	       mptr = save_blk;
	else   mptr = 0;
	}
else if ( indx == 4 )
	mptr = emq_current_ptr;
else if (indx == 5)
	{
	save_blk = spfnext(save_blk);
	if (save_blk != NULL)
	       mptr = save_blk;
	else   mptr = 0;
	}
else
     fprintf(stderr,"system- invalid index to get_msg_pointer (%d)\n",indx);
return (mptr);


}

/**********************************************************************
* q_find
* locate place in q to insert item
* return 1 to stop or 0
*************************************************************************/
FUNCTION q_find(nextbl, this)
register mesg_block *this;
register mesg_block *nextbl;
{
register int  txtlen, cmpb;


if (ltVTime(this->rlvt, nextbl->rlvt)) return(1); 
if (eqVTime(this->rlvt, nextbl->rlvt))
   {
   if (strcmp( this->rname, nextbl->rname) < 0) return(1);
   if (strcmp( this->rname, nextbl->rname) == 0)
      {
      if (this->select < nextbl->select) return(1);
      if (this->select == nextbl->select)
	{
	if (this->mtlen < nextbl->mtlen) 
		txtlen =  this->mtlen;
	else  txtlen =  nextbl->mtlen;
	if (txtlen > mesgdefsize) txtlen = mesgdefsize;
	cmpb =  bytcmp(this->text, nextbl->text, txtlen);
	if (cmpb < 0 ) return(1); 
	if (cmpb == 0) 
           if (this->mtlen < nextbl->mtlen) return(1);
	}
      }
   }
return(0);
}

/****************************************************************
*
* Function to display some queue statistics
* returns values of some variables incremented in the splay routines
*
*******************************************************************/
#ifdef fuji

/* This really returns a pointer to a character string  */

static char qstatistics[300];

FUNCTION qustats()
{
   if (pq == NULL)
      strcpy(qstatistics,"no queue");
   else
   sprintf(qstatistics,
"      deqs: %d\n  deqloops: %d\n      enqs: %d\n   enqcmps: %d\n    splays: %d\nsplayloops: %d\n",
	pq->deqs,pq->deqloops,pq->enqs,pq->enqcmps,pq->splays,
	pq->splayloops);
   return( (int)qstatistics);
}
#endif

/* end of twqueues.c file */



