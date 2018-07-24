/*      Copyright (C) 1989, 1991, California Institute of Technology.
		U. S. Government Sponsorship under NASA Contract NAS7-918
		is acknowledged.        */


/*
 * $Log:	SUN_Hg.c,v $
 * Revision 1.6  91/11/01  09:13:27  pls
 * 1.  Change ifdef's to if's.
 * 2.  Don't ACK IH msgs (Steve's changes).
 * 
 * Revision 1.5  91/08/14  08:38:06  steve
 * Replaced two writes in send_msg with one writev for increased speed.
 * 
 * Revision 1.4  91/07/22  13:10:26  steve
 * Fixed bug: The header could be sent, but the message body failed to be
 * sent. This was a failure to send, corrected to a partial send.
 * 
 * Revision 1.3  91/07/17  15:06:38  judy
 * New copyright notice.
 * 
 * Revision 1.2  91/07/11  09:00:55  steve
 * Rewritten to prepend a LowLevelMsgH to each message. Thus zero length
 * messages and data messages now work.
 * 
 * Revision 1.1  91/07/09  12:46:52  steve
 * Initial revision
 * 
 */

#include <stdio.h>
#include "twcommon.h"
#include "twsys.h"
#include "tester.h"
#include "machdep.h"
#include <sys/uio.h>

extern int last_node;
extern int maybe_socket_io;
static int last_node;

#if JOURNAL_FILE
static FILE *fp;
static char filename[35];
#endif

/* partially send stuff */
int partial_send;
int resnd_node;
char * resnd_msg;
int resnd_len;
int resnd_head_len;
char * resnd_head_msg;
LowLevelMsgH resnd_head;
char resnd_storage[ MAXPKTL + sizeof ( Msgh ) ];

/* stuff for the select call */
#if SELECT
static fd_set all_socks;
static struct timeval timeout;
static int maxfd;
int select_on;

init_select()
{
	int i;

	FD_ZERO( &all_socks );

	for ( i = 0; i <= tw_num_nodes; i++ )
	{
		if ( i == tw_node_num )
			continue;

		FD_SET( msg_ichan[i], &all_socks );

		if ( msg_ichan[i] > maxfd )
			maxfd = msg_ichan[i];
	}

	maxfd++;
}
#endif

/* 
 *	note that these routines will not work in the standalone version
 *
 */

get_msg ( msg_structure_ptr )
MSG_STRUCT * msg_structure_ptr;
{
	register LowLevelMsgH * ll_msg = (LowLevelMsgH *) (msg_structure_ptr->buf);
	register char * msgbuf = (char *) ll_msg;

	register int cc, n, node, mlen;
#if SELECT
	fd_set readfds;
#endif


#if JOURNAL_FILE
	if ( !fp )
	{
		sprintf ( filename, "communications%d", tw_node_num );
		fp = fopen ( filename, "w" );
	}
#endif

	maybe_socket_io = FALSE;

#if SELECT
	readfds = all_socks;

	n = select ( maxfd, &readfds, NULL, NULL, &timeout );

	if ( n <= 0 )
		return;
/*
	printf ( "Select returned %d and 0x%x\n", n, readfds );
*/
#endif

	for ( node = last_node + 1; ; node++ )
	{
		if ( node > tw_num_nodes )
			node = 0;

		if ( node == tw_node_num )
			goto check_for_exit;

#if SELECT
		if ( ! FD_ISSET ( msg_ichan[node], &readfds ) )
			goto check_for_exit;
#endif
read_ack:
		cc = read ( msg_ichan[node], msgbuf, sizeof(LowLevelMsgH) );

		if ( cc > 0 )
		{
			while ( cc <  sizeof(LowLevelMsgH) )
			{
				while ((n = read(msg_ichan[node], msgbuf + cc, 
						sizeof(LowLevelMsgH) - cc ) ) == -1 )
					if ( partial_send )
						resend_msg();
				cc += n;
			}

			mlen = msg_structure_ptr->mlen = ll_msg->length;
			msg_structure_ptr->dest = ll_msg->to_node;
			msg_structure_ptr->source = ll_msg->from_node;
			msg_structure_ptr->type = ll_msg->type;

			cc = 0;

#if JOURNAL_FILE
			fprintf ( fp, "get_msg header src %d, dest %d, type %d, len %d\n",
				ll_msg->from_node, ll_msg->to_node, ll_msg->type, ll_msg->length				);
#endif

			while ( cc < mlen )
			{
				while ( ( n = read(msg_ichan[node], msgbuf + cc,
						mlen - cc ) ) == -1 )
					if ( partial_send )
						resend_msg();
				cc += n;
			}

#if JOURNAL_FILE
			dumpit ( msgbuf, mlen);
#endif

			if ( ll_msg->type == ACK_MSG )   /* ACK */
			{
				rcvack ( ll_msg, node );
				goto read_ack;
			}

#if CHECKSUM
			if ( checksum ( msgbuf ) != ((Msgh *)msgbuf)->checksum )
			{
				_pprintf ( "Checksum Error\n" );
				showmsg ( msgbuf );
				tester ();
			}
#endif
			if ( (node != tw_node_num) && (node != CP)
				&&  !(((Msgh *)msgbuf)->flags & (SYSMSG|MOVING))
				&& (strcmp ( ((Msgh *)msgbuf)->rcver, "$IH" ) != 0) )
			{
				sndack ( msgbuf, node );
			}

			if ( (node == CP) &&
				( msg_structure_ptr->type == TESTER_COMMAND ) )
			{
				extern char buff[], * bp;
				extern int host_input_waiting;

				msgbuf[mlen] = '\0';

				strcpy ( buff, msgbuf );

				bp = buff;

				host_input_waiting = TRUE;
				maybe_socket_io = TRUE;

				return FALSE;
			}

			last_node = node;

			/* maybe more messages */
			maybe_socket_io = TRUE;

			return TRUE;
		}

check_for_exit:
		if ( node == last_node )
		{
			return FALSE;
		}
	}
}

get_msg_w ( msg_structure_ptr )
MSG_STRUCT    *msg_structure_ptr;
{
	for (;;)
	{
		if ( get_msg ( msg_structure_ptr ) )
			return TRUE;

		sigblock ( sigmask ( SIGIO ) );

		if ( maybe_socket_io == 0 )
			sigpause(0);

		sigsetmask(0);
	}
}

resend_msg()
{
	int ret;

	if ( resnd_head_len )
	{
		ret = write ( msg_ochan[resnd_node], resnd_head_msg, resnd_head_len );

		if ( ret == resnd_head_len )
		{
			resnd_head_len = 0;
			/* continue below */
		}
		else if ( ret > 0 )
		{
			resnd_head_msg += ret;
			resnd_head_len -= ret;
			return FALSE;
		}
		else
			return FALSE;
	}

	ret = write ( msg_ochan[resnd_node], resnd_msg, resnd_len );

	if ( ret == resnd_len )
	{
		partial_send = FALSE;
		return TRUE;
	}
	else if ( ret > 0 )
	{
		resnd_msg += ret;
		resnd_len -= ret;
		return FALSE;
	}
	else
		return FALSE;
}


send_msg ( msg_structure_ptr )
MSG_STRUCT  *msg_structure_ptr;
{
	register LowLevelMsgH * ll_msg = (LowLevelMsgH *) (msg_structure_ptr->buf);
	register char * msgbuf = (char *) ll_msg;

	register int ret, dest, len;
	LowLevelMsgH head;
	struct iovec iov[2];

	extern int mlog, node_cputime;

	if ( partial_send && ! resend_msg() )
		return -1;

#if JOURNAL_FILE
	if ( !fp )
	{
		sprintf ( filename, "communications%d", tw_node_num );
		fp = fopen ( filename, "w" );
	}
#endif

	dest = head.to_node = msg_structure_ptr->dest;
	head.from_node = msg_structure_ptr->source;
	head.type = msg_structure_ptr->type;

	iov[1].iov_len = len = head.length = msg_structure_ptr->mlen;

	iov[0].iov_base = (char *)&head;
	iov[0].iov_len = sizeof ( LowLevelMsgH );
	iov[1].iov_base = (char *)msg_structure_ptr->buf;

#if PARANOID
	if ( ( msg_structure_ptr->mlen != ll_msg->length ) ||
		( msg_structure_ptr->dest != ll_msg->to_node ) ||
		( msg_structure_ptr->source != ll_msg->from_node ) ||
		( msg_structure_ptr->type != ll_msg->type ) )

		if ( ll_msg->to_node != IH_NODE )
			printf ( "parameter missmatch in send_msg trying to send to %d\n",
				ll_msg->to_node );
#endif
		
#if JOURNAL_FILE
	fprintf ( fp, "send_msg header src %d, dest %d, type %d, len %d\n",
		ll_msg->from_node, ll_msg->to_node, ll_msg->type, ll_msg->length );
	dumpit ( msgbuf, ll_msg->length);
#endif

/*
	if ( msg_structure_ptr->type == ACK_MSG )
	{
		printf ( "send_msg: given ACK\n" );
		tester();
	}
*/
	if ( msg_structure_ptr->dest == ALL )
	{
		printf ( "SUN send_msg doesn't b'cst\n" );
		tester();
	}

	if ( msg_structure_ptr->dest == tw_node_num )
	{
		printf ( "send_msg doesn't send to self add: 0x%x\n", ll_msg );
		tester();
	}

#if MICROTIME
	if ( mlog && (msg_structure_ptr->type != ACK_MSG) && ( dest != CP ) )
	{
		MicroTime ();
		((Msgh *)msgbuf)->msgtimef = node_cputime;
	}
#endif

	ret = writev ( msg_ochan[dest], iov, 2 );

	if ( ret == -1 )
	{
		/* failure */
	}
	else if ( ret < sizeof(LowLevelMsgH) ) /* partial send */
	{
		resnd_head_len = sizeof(LowLevelMsgH) - ret;
		resnd_head = head;
		resnd_head_msg = (char *) &resnd_head;
		resnd_head_msg += ret;

		partial_send = TRUE;
		bcopy(msgbuf, resnd_storage, len );
		resnd_msg = &resnd_storage[0];
		resnd_node = dest;
		resnd_len = len;

		/* claim to succeed to calling routines */
		ret = len;
	}
	else /* at least the head was sent */
	{
		ret -= sizeof(LowLevelMsgH);

		if ( ret != len ) /* partial send */
		{
			resnd_head_len = 0;
			partial_send = TRUE;
			bcopy(msgbuf, resnd_storage, len );
			resnd_msg = &resnd_storage[ret];
			resnd_node = dest;
			resnd_len = len - ret;

			/* claim to suceed to calling routines */
			ret = len;
		}
	}

#if JOURNAL_FILE
	fprintf ( fp, "send_msg returning %d\n\n", ret );
#endif

	return ( ret ); /* -1 indicates failure */
}

send_msg_w ( msg_structure_ptr )
MSG_STRUCT  *msg_structure_ptr;
{
	register int ret;
	register int count = 0;

	while ( -1 == (ret = send_msg ( msg_structure_ptr ) ) )
	{
		if ( count++ >= 1000 )
		{
			count = 0;
			_pprintf ( "Node %d stuck writing to node %d\n", tw_node_num,
				msg_structure_ptr->dest );
		}
	}

	return ( ret ); /* -1 indicates failure */
}

show_ll_msgh ( sb )
LowLevelMsgH * sb;
{
  printf ("length =%d to_node =%d from_node =%d type =%d\n",
			sb->length, sb->from_node, sb->to_node, sb->type );
}

extern char buff[], * bp;
extern int host_input_waiting;

get_tester_input ()
{
	register char * msgbuf = buff;
	register LowLevelMsgH * ll_msg = (LowLevelMsgH *) msgbuf;

	register int cc, n, node, mlen;

#if JOURNAL_FILE
	if ( !fp )
	{
		sprintf ( filename, "communications%d", tw_node_num );
		fp = fopen ( filename, "w" );
	}
#endif

	node = CP;

	for (;;)
	{
		maybe_socket_io = FALSE;

		cc = read ( msg_ichan[node], msgbuf, sizeof(LowLevelMsgH) );

		if ( cc > 0 )
		{
			while ( cc <  sizeof(LowLevelMsgH) )
			{
				while ((n = read(msg_ichan[node], msgbuf + cc, 
					sizeof(LowLevelMsgH) - cc ) ) == -1 )
					;
				cc += n;
			}

#if JOURNAL_FILE
			fprintf ( fp,
				"get_test_input header src %d, dest %d, type %d, len %d\n",
				ll_msg->from_node, ll_msg->to_node,
				ll_msg->type, ll_msg->length );
#endif

			if ( ll_msg->type != TESTER_COMMAND )
			{
				printf ("get_tester_input got wrong type %d\n", ll_msg->type );

				show_ll_msgh ( ll_msg );
			}

			mlen = ll_msg->length;

			if ( mlen >= 512 )
			{
				printf ( "Tester input too long! About to overwrite buff\n" );
			}

			cc = 0;

			while ( cc < mlen )
			{
				while ( ( n = read(msg_ichan[node], msgbuf + cc,
					mlen - cc ) ) == -1 )
					;
				cc += n;
			}

			msgbuf[mlen] = '\0';
			bp = buff;

			host_input_waiting = TRUE;
			maybe_socket_io = TRUE;

#if JOURNAL_FILE
			dumpit ( msgbuf, mlen);
#endif

			return TRUE;
		}

		sigblock ( sigmask ( SIGIO ) );

		if ( maybe_socket_io == 0 )
			sigpause(0);

		sigsetmask(0);
	}
}

string_to_cp_w ( string )
char * string;
{
	MSG_STRUCT m;
	register int ret;
	register int count = 0;


	m.mlen = strlen ( string );
	m.dest = CP;
	m.source = tw_node_num;
	m.type = TESTER_COMMAND;
	m.buf = (int *)string;

	while ( -1 == (ret = send_msg ( &m ) ) )
	{
		if ( count++ >= 1000 )
		{
			count = 0;
			printf ( "Node %d Stuck writing string to CP\n", tw_node_num );
		}
	}

	return ( ret ); /* -1 indicates failure */
}

#if JOURNAL_FILE
dumpit (u, n)
	char           *u;
	int             n;
{
	char            s[80];
	char            t[10],
					c;
	int             i,
					j;

	s[78] = '\n';
	s[79] = '\0';

#define clrs    {int i; for(i=0; i<78; i++) s[i] = ' ';}
#define toupper(c)	(( c >= 'a' && c <= 'z' )? c - ' ': c)

	for (i = 0; i < n;)
	{
		clrs

		sprintf (t, "%3.3x:", i % 0x1000);

		s[0] = toupper (t[0]);
		s[1] = toupper (t[1]);
		s[2] = toupper (t[2]);
		s[3] = t[3];

		for (j = 0; j < 16 && i < n; j++, i++)
		{
			sprintf (t, "%2x ", 0377 & (c = 0377 & u[i]));
			s[5 + 3 * j] = toupper (t[0]);
			s[5 + 3 * j + 1] = toupper (t[1]);
			s[5 + 52 + j] = c < 32 || c > 127 ? '.' : c;
		}
		fprintf (fp, "%s", s);
	}
		fprintf (fp, "\n" );
		fflush (fp);
}
#endif
