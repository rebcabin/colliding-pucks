/*      Copyright (C) 1989, 1991, California Institute of Technology.
		U. S. Government Sponsorship under NASA Contract NAS7-918
		is acknowledged.        */

/*
 * $Log:	BF_MACH_Hg.c,v $
 * Revision 1.9  91/11/01  13:35:44  pls
 * 1.  Fix bug 11, "Attempt to set timer in the past".
 * 2.  Fix system message deadlock, bug 17.
 * 
 * Revision 1.8  91/07/17  15:51:48  judy
 * New copyright notice.
 * 
 * Revision 1.7  91/06/03  12:16:20  configtw
 * Tab conversion.
 * 
 * Revision 1.6  91/04/01  15:33:13  reiher
 * Added the weLooped variable to tell when some node went to sleep.  It's
 * printed out into the XL_STATS file.
 * 
 * Revision 1.5  90/12/10  11:09:21  configtw
 * change block_copy() call to bcopy() for TC2000
 * 
 * Revision 1.4  90/11/27  10:16:46  csupport
 * prevent IH messages from affecting GVT calculation (plr)
 * 
 * Revision 1.3  90/08/27  10:33:16  configtw
 * Add some debugging printouts
 * 
 * Revision 1.2  90/08/16  11:10:19  steve
 * Major Revision
 * 1. removed the Uniform System
 * 2. changed the allocation so only `number_of_buffers' are allocated
 *      on each node.
 * 3. changed the touch routines for TC2000 alignment
 * 4. added special case code to snd_msg to help prevent a single node
 *      failure (or delay) from locking up the system as fast.
 * 5. added delay functions
 * 6. added which_nodes()
 * 7. added lock and unlock
 * 
 * Revision 1.1  90/08/06  14:15:29  configtw
 * Initial revision
 * 
*/

/******************************************************************************/
/*                                                                            */
/*              BUTTERFLY MESSAGE SEND AND RECEIVE ROUTINES                   */

#include <stdio.h>
#include <mach.h>
#include <sys/vm_mapmem.h>
#include <sys/cluster.h>
#include "twcommon.h"
#include "twsys.h"
#include "BBN.h"



extern int tw_node_num;
extern int tw_num_nodes;

#define BUFF_SIZE 824   /*PJH-TC2000    Alignment Problems??    */
#define RECV_QSIZE 256 
#define RECV_PQSIZE 256 
#define EVENT_QSIZE 128 
#define Q_SIZE ( (sizeof(FIFO_QUEUE)+7)&~7 )

int number_of_buffers = DEFAULT_NUM_BUFFS;


/* Global data structures that everyone will get a copy of */
char * shared_area[MAX_NODES];          /* Start of the shared area */
unsigned int shared_area_size;          /* Size of the shared area */

FIFO_QUEUE *event_queue[MAX_NODES];     /* Event Queue  */
FIFO_QUEUE *recv_queue[MAX_NODES];      /* Message Queue */
FIFO_QUEUE *recv_pqueue[MAX_NODES];     /* Prioirity Message Queue */
char *buffer_pool[MAX_NODES];
MESSAGE * send_buff[MAX_NUM_BUFFS];

Uint alarm_time;
Uint malarm_time;
Uint dlm_alarm_time;

#define EXHAUST_THRESHOLD       10000
int buf_exhaust_count = 0;


/*****************************************************************************/
InitQueue ( newq, qsize, datap )
FIFO_QUEUE * newq;
int qsize;
QUEUE_ITEM * datap;
{
	newq->data = datap;
	newq->size = qsize;
	newq->in = 0;
	newq->out = 0;
	newq->lock = 0;
}

int weLooped = 0;

/******************************************************************************/
/*                                                                            */
/*                      H O S T   I N I T I A L I Z A T I O N                 */


#define START_SHARE 0x40000000

butterfly_host_init ()
{

	int i;
	int node_num;
	char * vAddress, *askAddress;
	char temp, *p, *q;
	kern_return_t ret_error;

	alarm_time = 0;
	tw_node_num = tw_num_nodes;

	shared_area_size = 3 * Q_SIZE + number_of_buffers * BUFF_SIZE
		+ sizeof(QUEUE_ITEM) * (RECV_QSIZE + RECV_PQSIZE + EVENT_QSIZE);

	shared_area_size =
		((shared_area_size + vm_page_size - 1)/vm_page_size) * vm_page_size;

	/* PJH-TC2000 Need to check this for port   */
	/* man pages imply that these pages must be initialized for atomic
		operations on the TC2000--when we port to it */

	for ( node_num = 0; node_num <= tw_num_nodes; node_num++ )
	{
		askAddress = vAddress =
			(char *) (START_SHARE + node_num * shared_area_size);

		if ( 0 != (ret_error = vm_mapmem ( task_self(), &vAddress,
			shared_area_size, VM_MAPMEM_ALLOCATE, 0, 0, node_num ) ) )
			 /* VM_MAPMEM_WIRE | */
		{
			printf ("vm_mapmem failed error code %d\n", ret_error );
			exit (1);
		}

		if ( askAddress != vAddress )
			printf ( "vm_mapmem returned different address\n" );

		if ( 0 != (ret_error = vm_inherit( task_self(), vAddress,
			shared_area_size, VM_INHERIT_SHARE ) ) )
		{
			printf ( "vm_inherit returned %d\n", ret_error );

		}

/*
 * We need a recv_queue and buffer pool on each node.
 * Allocate room for each in the shared data area.
 */
		shared_area[node_num] = vAddress;

		event_queue[node_num] = (FIFO_QUEUE *) vAddress;
		vAddress = (char *) ((int)vAddress + Q_SIZE);
		InitQueue ( event_queue[node_num], EVENT_QSIZE, vAddress );
		vAddress = (char *) ((int)vAddress + EVENT_QSIZE * sizeof(QUEUE_ITEM) );

		recv_pqueue[node_num] = (FIFO_QUEUE *) vAddress;
		vAddress = (char *) ((int)vAddress + Q_SIZE);
		InitQueue ( recv_pqueue[node_num], RECV_PQSIZE, vAddress );
		vAddress = (char *) ((int)vAddress + RECV_PQSIZE * sizeof(QUEUE_ITEM) );

		recv_queue[node_num] = (FIFO_QUEUE *) vAddress;
		vAddress = (char *) ((int)vAddress + Q_SIZE);
		InitQueue ( recv_queue[node_num], RECV_QSIZE, vAddress );
		vAddress = (char *) ((int)vAddress + RECV_QSIZE * sizeof(QUEUE_ITEM) );

		buffer_pool[node_num] = vAddress;
	}


/* Compute my own set of ptrs into my send buffer space (all of the
 * child processes will have to do this also)
 */

	for ( i = 0; i < number_of_buffers; i++ )
	{
		send_buff[i] = 
				(MESSAGE *) (buffer_pool[CP] + (BUFF_SIZE * i));
	}


/* This may help prevent spurious page faults  */
	for ( i = 0; i <= tw_num_nodes; i++ )
	{
		p = shared_area[i];
		q = (char *)((unsigned long) p + shared_area_size);

		for ( ; p < q; p = (char *)((unsigned long) p + vm_page_size) )
		{
			temp = *p;
			*p = temp;
		}
	}

}

/* Here we are waiting for everyone else to initialize their */
/* Event Queues and message buffers.                         */


butterfly_host_wait_for_nodes ()
{
	int i,event_type;
	i = tw_num_nodes;

	while (i !=0)
	{
	  event_type = check_for_events ();
	  if ( event_type == NODE_READY_EVENT )
	  {
		 i--;
	  }
	}
	/* start_nodes */ 

	broadcast_event ( START_EVENT );

}


/******************************************************************************/
/*                                                                            */
/*                      N O D E   I N I T I A L I Z A T I O N                 */

butterfly_node_init ( )

{
	int i;
	char *p, *q;
	char temp;
	int event_type;
	extern int rtc_sync;

	alarm_time = 0;

	butterflytime_init ( rtc_sync );  


/* Create my own set of pointers into my own send buffer space */

	for (i = 0; i < number_of_buffers; i++)
	{ 
		send_buff[i] = (MESSAGE *) 
						(buffer_pool[tw_node_num] + (BUFF_SIZE * i));
	}

/* go touch everyone's queues and buffer pools (including myself) */
	for ( i = 0; i <= tw_num_nodes; i++ )
	{
		p = shared_area[i];
		q = (char *)((unsigned long) p + shared_area_size);

		for ( ; p < q; p = (char *)((unsigned long) p + vm_page_size) )
		{
			temp = *p;
			*p = temp;
		}
	}


/* tell the CP we're ready  */

	post_event_w (event_queue[CP], NODE_READY_EVENT);

/* Now wait for the START_EVENT */

	event_type = 0;


	for ( ;; )
	{

		event_type = check_for_events ();

		if ( event_type == START_EVENT )
			break;

		wait_a_millisecond();
	}

}

/******************************************************************************/
/*                                                                            */
/*                      N O D E   T E R M I N A T I O N                       */

butterfly_node_term ()
{
   post_event_w ( event_queue[CP], END_EVENT);
}

VTime butterfly_min ()
{
	register int i;
	register Msgh * msg;
	VTime min;

	min = posinfPlus1;

	for ( i = 0; i < number_of_buffers; i++ )
	{
		if ( send_buff[i]->use_count == 0 ) continue;
		if ( send_buff[i]->dest == CP ) continue;
		msg = (Msgh *) send_buff[i]->data;
		if ( issys_macro ( msg ) ) continue;

/*  The following line is necessary to omit IH messages from nodes other
		than node 0 from GVT computation.  The line above that tests
		destination against CP might not be necessary with the following
		line in place, but I've left it in for good luck.  PLR */

		if ( strcmp ( msg->rcver, "$IH") == 0 ) continue;
		if ( ! ( msg->flags & MOVING ) )
		{
			if ( gtVTime ( min, msg->sndtim ) )
				min = msg->sndtim;
		}
		if ( ltVTime ( min, gvt ) )
		{
			twerror("butterfly_min: setting min %f to before gvt %f\n",
				min.simtime, gvt.simtime);
			showmsg ( msg );
			tester();
		}
	}

	return ( min );
}

showbuffstruct ( sb )
MESSAGE * sb;
{
  _pprintf ("count =%d len =%d src =%d dest =%d type =%d ptr =%lx\n",
			sb->use_count,
			sb->length,
			sb->src,
			sb->dest,
			sb->type,
			sb->data
		   );
}

showHgMstruct ( Hg_s )
MSG_STRUCT * Hg_s;
{
  _pprintf ("buf %lx blen =%d mlen =%d dest =%d  src =%d type =%d \n",
			Hg_s->buf,
			Hg_s->blen,
			Hg_s->mlen,
			Hg_s->dest,
			Hg_s->source,
			Hg_s->type
		   );
}

showsendbuffs ()
{
	int i;
	Msgh * msg;

	_pprintf("Send buffers\n");
	for ( i = 0; i < number_of_buffers; i++ )
	{
		if ( send_buff[i]->use_count == 0 ) continue;
		if ( send_buff[i]->dest == CP) continue;
		msg = (Msgh *) send_buff[i]->data;
		printf("Destination %d\n",send_buff[i]->dest);
		showmsg ( msg );
	}
}

/******************************************************************************/
/*                                                                            */
/*                      S E N D   M E S S A G E                               */

send_msg ( msg_structure_ptr )

	MSG_STRUCT * msg_structure_ptr;
{
	char * msgbuf;
	int ret, i, node, first, last;
	int start;
	int msg_pri;
	int dest0;
	int sameNode;
	extern int not_dumping_stats;


	extern int max_acks;
	extern int mlog, node_cputime;

	msg_pri = 0;
	msgbuf = NULL;

	if ( msg_structure_ptr->dest != CP
	&&   issys_macro ( (Msgh *) (msg_structure_ptr->buf) ) )
		msg_pri = 1;

	if ( msg_pri || isanti_macro ( (Msgh *) (msg_structure_ptr->buf) ) )
	{
		for ( i = max_acks; i < number_of_buffers; i++ )
		{
			if ( send_buff[i]->use_count == 0 )
			{
				msgbuf = send_buff[i]->data;
				break;
			}
		}
	}
	else
	{
		for ( i = 0; i < max_acks; i++ )
		{
			if ( send_buff[i]->use_count == 0 )
			{
				msgbuf = send_buff[i]->data;
				break;
			}
		}

/*
 * The code below is design to help with one node delays.
 * If all the message buffers < max_acks are to the same
 * node, and the message we are attempting to send is to
 * a different node, and buffer number `number_of_buffers - 1'
 * is free, then we will use this last message buffer to
 * send this message. 
 */

		if ( (msgbuf == NULL) &&
				((dest0 = send_buff[0]->dest) != msg_structure_ptr->dest) &&
				(send_buff[number_of_buffers - 1]->use_count == 0) )
		{
			sameNode = TRUE;

			for ( i = 1; sameNode && (i < max_acks); i++ )
			{
				/* sameNode is TRUE if we got here */
				sameNode = ( dest0 == send_buff[i]->dest );
			}

			i = number_of_buffers - 1;

			if ( sameNode )
			{
				msgbuf = send_buff[i]->data;
			}
		}
	}

	if ( msgbuf == NULL )
	{
		if ( (++buf_exhaust_count > EXHAUST_THRESHOLD ) && not_dumping_stats )
		{
			_pprintf (
				"%d loops in send_msg without change: dest:%d msg_pri is %d\n",
				buf_exhaust_count, msg_structure_ptr->dest, msg_pri );
			weLooped++;

			for ( i = 0; i < max_acks; i++ )
				showbuffstruct ( send_buff[i] );

		   buf_exhaust_count =0;
		} 

		return ( -1 );  /* no buffers */
	}
	else
	{
		buf_exhaust_count =0;
	} 


	if ( msg_structure_ptr->mlen > 0 )
	{
#ifdef TC2000
		bcopy ( msg_structure_ptr->buf,
#else
		block_copy ( msg_structure_ptr->buf,
#endif
					 msgbuf, 
					 msg_structure_ptr->mlen 
				   );

	}
	else
	{   
		_pprintf ( "send_msg: mlen %d msg %x\n",
			msg_structure_ptr->mlen, msg_structure_ptr );
		tester ();
	}

	send_buff[i]->use_count = 1;
	send_buff[i]->length = msg_structure_ptr->mlen;
	send_buff[i]->dest = msg_structure_ptr->dest;
	send_buff[i]->src = msg_structure_ptr->source;
	send_buff[i]->type = msg_structure_ptr->type;


	if ( msg_structure_ptr->dest == CP )
	{
		first = last = CP;
	}
	else
	if ( msg_structure_ptr->dest == ALL )
	{
		first = 0; last = (tw_num_nodes -1);
		send_buff[i]->use_count = last - first;
	}
	else
	{
		first = last = (msg_structure_ptr->dest);
	}

	for ( node = first; node <= last; node++ )
	{
		if ( node == tw_node_num )
			continue;

		if ( mlog && ( msg_structure_ptr->dest != CP ) )
		{
			butterflytime ();
			((Msgh *)msgbuf)->msgtimef = node_cputime;
		}

		for ( ret = FALSE; ret == FALSE; )
		{
			if ( msg_pri ) 
			{
				ret = EnqueueQueue ( recv_pqueue[node],
						i + ( tw_node_num << 16 ) );
			}
			else   
			{
				ret = EnqueueQueue ( recv_queue[node],
						i + ( tw_node_num << 16 ) );

			}

/* If we are not successfull in getting a DualQ, waiting for a bit.     */
/* Otherwise, we just increase switch traffic.                          */

			if ( ! ret )
			{   
#if 0
				if ( msg_pri == 0 )
#endif
				{
					send_buff[i]->use_count = 0;
					return -1;
				}

				wait_a_millisecond();
			} 
		}
	}

   return ( msg_structure_ptr->mlen );
}

/******************************************************************************/
/*                                                                            */
/*              S E N D   M E S S A G E   A N D   W A I T                     */

send_msg_w ( msg_structure_ptr )

	MSG_STRUCT  *msg_structure_ptr;

{
	char * msgbuf;
	int ret, i, node, first, last;
	int start;


	for ( msgbuf = 0; msgbuf == 0; )
	{
		for ( i = 0; i < number_of_buffers; i++ )
			if ( send_buff[i]->use_count == 0 )
				break;

		if ( i < number_of_buffers )
			msgbuf = send_buff[i]->data;
	}
	if ( msgbuf == NULL)
	{
		if (++buf_exhaust_count > EXHAUST_THRESHOLD )
		{
			_pprintf ( "%d loops in send_msg_w without change\n",
				buf_exhaust_count );

		   buf_exhaust_count =0;
		}

		return (-1);
	}
	else
	{
		buf_exhaust_count =0;
	}           

	if ( msg_structure_ptr->mlen > 0 )
	{
#ifdef TC2000
		bcopy ( msg_structure_ptr->buf, msgbuf, msg_structure_ptr->mlen );
#else
		block_copy ( msg_structure_ptr->buf, msgbuf, msg_structure_ptr->mlen );
#endif
	}


	send_buff[i]->use_count = 1;
	send_buff[i]->length = msg_structure_ptr->mlen;
	send_buff[i]->dest = msg_structure_ptr->dest;
	send_buff[i]->src = msg_structure_ptr->source;
	send_buff[i]->type = msg_structure_ptr->type;

	if ( msg_structure_ptr->dest == CP )
	{
		first = last = CP;
	}
	else
	if ( msg_structure_ptr->dest == ALL )
	{
		first = 0; last = tw_num_nodes - 1;
		send_buff[i]->use_count = last - first;
	}
	else
		first = last = msg_structure_ptr->dest;

	for ( node = first; node <= last; node++ )
	{
		if ( node == tw_node_num )
			continue;

		for ( ret = FALSE; ret == FALSE; )
		{

			ret = EnqueueQueue ( recv_queue[node],
				i + ( tw_node_num << 16 ) );

/* If we are not successfull in getting a DualQ, waiting for a bit.     */
/* Otherwise, we just increase switch traffic.                          */

			if ( ! ret )
				wait_a_millisecond();
		}
	}

   return ( TRUE );
}

/******************************************************************************/
/*                                                                            */
/*                      R E C E I V E   M E S S A G E                         */

get_msg ( msg_structure_ptr )

  MSG_STRUCT    *msg_structure_ptr;

{
	int ret, buffno, datum;
	int  node;

	MESSAGE * baddr;

	ret = FALSE;
	if ( CheckQueue (recv_pqueue[tw_node_num]) ) 
	 {
		ret = DequeueQueue ( recv_pqueue[tw_node_num], &datum );
	 }
	if ( ret == FALSE && CheckQueue (recv_queue[tw_node_num]) ) 
	 {
		ret = DequeueQueue ( recv_queue[tw_node_num], &datum );
	 }
	if ( ret == FALSE )
	 {
		 return ( FALSE );
	 }
	node = datum >> 16;
	buffno = datum & 0xffff;
	baddr = (MESSAGE *)(buffer_pool[node] + ( BUFF_SIZE * buffno ));

/*PJH DEBUG
	_pprintf ("get_msg()datum = %x no = %x node =%x bp =%lx ptr=%lx\n",
				datum,buffno,node,buffer_pool[node], baddr );
*/


#ifdef PARANOID  /*This check is obsolete */
#if 0
	if ( baddr->src != node )
	{
		_pprintf ( "get_msg: baddr %x node %d buffno %d source %d\n",
			baddr, node, buffno, baddr->src );
		tester ();
	}

	if ( baddr->length < 0 || baddr->length > msgdefsize )
	{
		_pprintf ( "get_msg: baddr %x node %d buffno %d source %d length %d\n",
			baddr, node, buffno, baddr->src, baddr->length );
		tester ();
	}
#endif
#endif


	msg_structure_ptr->mlen = baddr->length;
	msg_structure_ptr->dest = baddr->dest;
	msg_structure_ptr->source = baddr->src;
	msg_structure_ptr->type = baddr->type;



	if ( baddr->length > 0 )
#ifdef TC2000
		bcopy ( baddr->data, msg_structure_ptr->buf, baddr->length );
#else
		block_copy ( baddr->data, msg_structure_ptr->buf, baddr->length );
#endif

	atomadd ( &baddr->use_count, -1 );

	return ( TRUE );
}

/******************************************************************************/
/*                                                                            */
/*              R E C E I V E   M E S S A G E    W A I T                      */

get_msg_w ( msg_structure_ptr )

  MSG_STRUCT    *msg_structure_ptr;

{
	int  buffno, datum;
	int  node;

	MESSAGE * baddr;

	while (  DequeueQueue ( recv_pqueue[tw_node_num], &datum ) == FALSE 
	&&       DequeueQueue ( recv_queue[tw_node_num], &datum ) == FALSE )
		; 

	node = datum >> 16;
	buffno = datum & 0xffff;
	baddr = (MESSAGE *)(buffer_pool[node] + ( BUFF_SIZE * buffno ));

	msg_structure_ptr->mlen = baddr->length;
	msg_structure_ptr->dest = baddr->dest;
	msg_structure_ptr->source = baddr->src;
	msg_structure_ptr->type = baddr->type;

	if ( baddr->length > 0 )
	 {
#ifdef TC2000
		bcopy ( baddr->data, msg_structure_ptr->buf, baddr->length );
#else
		block_copy ( baddr->data, msg_structure_ptr->buf, baddr->length );
#endif
	 }
	atomadd ( &baddr->use_count, -1 );

	return ( TRUE );
}

/******************************************************************************/
/*                                                                            */
/*                      S E N D   C O M M A N D                               */

butterfly_send_command ( msg, dest )

	char * msg;
	int dest;
{
	int i, node, first, last;
	int length, type = 0;
	char * msgbuf;

	for ( msgbuf = 0; msgbuf == 0; )
	{
		for ( i = 0; i < number_of_buffers; i++ )
			if ( send_buff[i]->use_count == 0 )
				break;

		if ( i < number_of_buffers )
		{
			msgbuf = send_buff[i]->data;
		}
	}

	length = strlen ( msg ) + 1;

#ifdef TC2000
	bcopy ( msg, msgbuf, length );
#else
	block_copy ( msg, msgbuf, length );
#endif

	if ( dest == ALL )
	{
		first = 0; last = tw_num_nodes - 1;
	}
	else
		first = last = dest;

	send_buff[i]->use_count = last - first + 1;
	send_buff[i]->length = length;
	send_buff[i]->type = type;

	for ( node = first; node <= last; node++ )
	{
		post_event_w ( event_queue[node], COMMAND_EVENT + ( i << 16 ) );
	}
}


/*****************************************************************************/
broadcast_event (datum)
	int datum;
{
	int first, last, node;

	first = 0; last = tw_num_nodes - 1;

/*PJH */ 
	/* note that this version will post to ALL children, regardless of who
	 * sent it -- if coming from CP, this is probably the right thing.  if
	 * one child wants to interrupt all others, it should probably NOT
	 * enqueue an event on its own node.  how about the following ?
	 */

	for (node = first; node <= last; node++)
	{
		if (node != tw_node_num)
		 {
			post_event_w (event_queue[node], datum);
		 }
	}

}


/******************************************************************************/
/*                                                                            */
/*              P O S T   E V E N T   A N D   W A I T                         */

post_event_w ( equeue, datum )

	FIFO_QUEUE *equeue;
	int datum;
{

int status = FALSE;

	while (!status)
	{
		status = EnqueueQueue (equeue, datum);
	}
	if ( tw_node_num == CP )
	{
			check_for_events ();
	}
}

/******************************************************************************/
/*                                                                            */
/*                      R E C E I V E   C O M M A N D                         */

butterfly_recv_command ( msg )

	char * msg;
{
	int ret, length, type, buffno;
	MESSAGE * baddr;

	for ( ;; )
	{
		ret = check_for_events ();

		if ( ( ret  & 0xffff ) == COMMAND_EVENT )
			break;
	}

	buffno = ret >> 16;

	baddr = buffer_pool[CP] + ( BUFF_SIZE * buffno );

	length = baddr->length;
	type = baddr->type;

#ifdef TC2000
	bcopy ( baddr->data, msg, length );
#else
	block_copy ( baddr->data, msg, length );
#endif

	atomadd ( &baddr->use_count, -1 );
}

/******************************************************************************/
/*                                                                            */
/*                      G E T   P R O M P T                                   */

butterfly_get_prompt ( prompt, dest )

	char * prompt;
	int dest;
{
	int i, node, first, last;
	char * msgbuf;

	for ( msgbuf = 0; msgbuf == 0; )
	{
		for ( i = 0; i < number_of_buffers; i++ )
			if ( send_buff[i]->use_count == 0 )
				break;

		if ( i < number_of_buffers )
		{
			msgbuf = send_buff[i]->data;
		}
	}

	if ( dest == ALL )
	{
		first = 0; last = tw_num_nodes - 1;
	}
	else
		first = last = dest;

	send_buff[i]->use_count = last - first + 1;

	for ( node = first; node <= last; node++ )
	{
		post_event_w ( event_queue[node], PROMPT_EVENT + ( i << 16 ) );
	}

	while ( send_buff[i]->use_count > 0 )
	{
		wait_a_millisecond();
	}

	strcpy ( prompt, send_buff[i]->data );
}

/******************************************************************************/
/*                                                                            */
/*                      P R O M P T   F O R   C O M M A N D                   */

butterfly_prompt ( prompt )

	char * prompt;
{
	int ret, buffno;
	MESSAGE * baddr;

	for ( ;; )
	{
		ret = check_for_events ();

		if ( ( ret  & 0xffff ) == PROMPT_EVENT )
			break;
	}

	buffno = ret >> 16;

	baddr = buffer_pool[CP] + ( BUFF_SIZE * buffno );

#ifdef TC2000
	bcopy ( prompt, baddr->data, strlen(prompt)+1 );
#else
	block_copy ( prompt, baddr->data, strlen(prompt)+1 );
#endif

	atomadd ( &baddr->use_count, -1 );
}

Set_Timer (rtc_time, EVT_TYPE )
Uint rtc_time;
int EVT_TYPE;
{
	Uint	nowTime;
	
	nowTime = getrtc();
	if ((rtc_time < nowTime) && ((nowTime - rtc_time) < 0xffff))
	 {
		_pprintf ("Set_Timer: Attempt to set timer in the past\n");
		_pprintf ("Resetting ALL alarms!!\n");

		alarm_time = 0;
		malarm_time =0;
		dlm_alarm_time =0;
	 }
	else
	 {

		switch (EVT_TYPE)
		{
		case MTIMER_EVENT:
		   malarm_time = rtc_time;
		   break;
		case TIMER_EVENT:
		   alarm_time = rtc_time;
		   break;
#ifdef DLM

		case DLM_EVENT:
		   dlm_alarm_time = rtc_time;
		   break;
#endif

		}
	 }
}


/******************************************************************************/
/*                                                                            */
/*                      S I G N A L   S I G M A L A R M                       */

#define ONE_MSEC ((int)(1000. / 62.5))

static int (*malarm_routine) ();


malarm ( msecs )

	int msecs;
{

	Set_Timer ( getrtc() + ONE_MSEC * msecs, MTIMER_EVENT );
}

butterfly_msigalarm ( routine )

	int (*routine) ();
{
	malarm_routine = routine;
}

/******************************************************************************/
/*                                                                            */
/*                      S I G N A L   S I G A L A R M                         */

#define ONE_SECOND ((int)(1000000. / 62.5))

static int (*alarm_routine) ();


alarm ( seconds )

	int seconds;
{

	Set_Timer ( getrtc() + ONE_SECOND * seconds, TIMER_EVENT );
}

butterfly_sigalarm ( routine )

	int (*routine) ();
{
	alarm_routine = routine;
}

#ifdef DLM

static int ( *dlmAlarm_routine ) ();


dlmAlarm ( seconds )

	int seconds;
{

	Set_Timer ( getrtc() + ONE_SECOND * seconds, DLM_EVENT );
}

butterfly_dlmAlarm ( routine )

	int (*routine) ();
{
	dlmAlarm_routine = routine;
}

#endif DLM

/******************************************************************************/
/*                                                                            */
/*                      S I G N A L   S I G I N T                             */

static int (*interrupt_routine) ();

butterfly_sigint ( routine )

	int (*routine) ();
{
	interrupt_routine = routine;
}


/*****************************************************************************/
check_alarm ()
{
	Uint		nowTime;
	
	nowTime = getrtc();
	
/*PJH I wonder if this will work OK with multiple event posts? */

	if ((malarm_time != 0) && 
		(((nowTime > malarm_time) && ((nowTime - malarm_time) < 0x3fffffff)) ||
		((nowTime < malarm_time) && ((malarm_time - nowTime) > 0x3fffffff))))
	{
/* post the MTIMER event and reset the timer */ 
		post_event_w(event_queue[tw_node_num], MTIMER_EVENT);
		malarm_time = 0;
	}

#ifdef DLM

	if ((dlm_alarm_time != 0) &&
		(((nowTime > dlm_alarm_time) && ((nowTime - dlm_alarm_time) < 
			0x3fffffff)) ||
		((nowTime < dlm_alarm_time) && ((dlm_alarm_time - nowTime) >
			0x3fffffff))))
	{
/* post the DLM_EVENT event and reset the timer */
		post_event_w(event_queue[tw_node_num], DLM_EVENT);
		dlm_alarm_time = 0;
	}
#endif

	if ((alarm_time != 0) &&
		(((nowTime > alarm_time) && ((nowTime - alarm_time) < 0x3fffffff)) ||
		((nowTime < alarm_time) && ((alarm_time - nowTime) > 0x3fffffff))))
	{
/* post the TIMER_EVENT event and reset the timer */
		post_event_w(event_queue[tw_node_num], TIMER_EVENT);
		alarm_time = 0;
	}
}    




/******************************************************************************/
/*                                                                            */
/*                      C H E C K   F O R   E V E N T S                       */

check_for_events ()
{
	int status;
	int edata;

	status = 0;

	if ( CheckQueue ( event_queue [tw_node_num] ) )
		status = DequeueQueue(event_queue[tw_node_num], &edata);

	if ( status )
	{

		switch ( edata & 0xffff )
		{
			case MTIMER_EVENT:

				if ( malarm_routine )
				{
					(*malarm_routine) ();
				}
				break;

			case TIMER_EVENT:

				if ( alarm_routine )
				{
					(*alarm_routine) ();
				}
				break;
#ifdef DLM
			case DLM_EVENT:

				if ( dlmAlarm_routine )
				{
					( *dlmAlarm_routine ) ();
				}
				break;
#endif DLM

			case INTERRUPT_EVENT:

				if ( interrupt_routine )
				{
					(*interrupt_routine) ();
				}
				break;

			case COMMAND_EVENT:

				break;

			case PROMPT_EVENT:

				break;

			case END_EVENT:

				if ( tw_node_num != CP )
				{
				   exit (0);
				}

				break;

			case NODE_READY_EVENT:

				break;

			case START_EVENT:

				break;

			default:

				printf ( "%d received unknown event %d\n", 
						tw_node_num,edata );
		}

		return ( edata );
	}

	return ( 0 );
}

/******************************************************************************/
/*                                                                            */
/*                      I N T E R R U P T   N O D E S                         */

interrupt_nodes ()
{
	int i;

  if ( tw_node_num == CP )
  {
	for ( i = 0; i < tw_num_nodes; i++ )
		post_event_w ( event_queue[i], INTERRUPT_EVENT );
  }
  else
	post_event_w ( event_queue[CP], INTERRUPT_EVENT );
}

/******************************************************************************/


int EnqueueQueue (queue, item)

	FIFO_QUEUE *queue;
	QUEUE_ITEM item;
{
	register int in, qsize;     /* cached copy for speed */
								/* OK since manipulated under lock */


	lock ( &(queue->lock) );


	in = queue->in;
	qsize = queue->size;

	if ( queue->out == ( (in == qsize - 1)? 0 : in + 1 ) )
	{

#ifdef ENQUEUE_TRACE
		printf ("Enqueue failed: queue 0x%x is full\n", queue);
#endif


		unlock (&(queue->lock));


		return (FALSE);
	}
/*
_pprintf("EQ item is %x queue is %x\n", item, queue );
*/
	queue->data[in++] = item;

/* Write back the new in position */

	queue->in = (in == qsize)? 0 : in;


	unlock (&(queue->lock));


	return (TRUE);
}


int DequeueQueue (queue, item_ptr)

	FIFO_QUEUE *queue;
	QUEUE_ITEM *item_ptr;

{
	register int out;           /* cached copy for speed */
								/* OK since manipulated under lock */

/*
 * sfb: there should be a separate read lock (dequeue) and
 * write lock (enqueue) for each queue, since reads and writes
 * can safely happen in parallel. However, since each node is the only
 * reader of his queue we don't need a read lock.
 *
 */

	out = queue->out;

	if (out == queue->in)
	{
#ifdef DEQUEUE_TRACE
		printf ("Dequeue failed: queue 0x%x is empty\n", queue);
#endif


		return (FALSE);
	}

	*item_ptr = queue->data[out++];

/* Write back the new in position */

	queue->out = (out == queue->size)? 0 : out;

/*
_pprintf( "DQ item is %x queue is %x\n", *item_ptr, queue );
*/

	return (TRUE);
}

int CheckQueue (queue)

	FIFO_QUEUE *queue;

{
	int ret, out;                       

	out = queue->out;
	ret = ((out == queue->in) ? FALSE : TRUE);
	return (ret);

}


/******************************************************************************/

/* debug int max_spins;*/

lock ( location )
short * location;
{
	/* debug int spin = 0; */

	for ( ;; )
	{
		if ( 0 == atomior ( location, 1 ) )
		{
			/* max_spins = (spin > max_spins)? spin : max_spins; */
			return;
		}
		{
			register int k = 40;

			while ( k-- )
			{
				/* spin for ~ 40 microseconds on GP1000*/
			}
		}
		/* spin++; */
	}
}

unlock ( location )
short * location;
{
	atomand ( location, 0 );
}

wait_a_millisecond()
{
	register int k = 1300;

	while ( k-- )
	{
		/* spin -- 1300 = about a millisecond on GP1000*/
	}
}

which_nodes ( cluster_logical, mach_logical, physical )
int * cluster_logical;
int * mach_logical;
int * physical;
{
	cluster_id_t not_used = 0;
	int data_count;
	struct home_node_data answer;

	cluster_ctl ( not_used, GET_HOME_NODE, &answer, &data_count);

	*cluster_logical = answer.home_pnn;
	*mach_logical = answer.system_pnn;
	*physical = answer.physical_pnn;
}
