/*      Copyright (C) 1989, 1991, California Institute of Technology.
		U. S. Government Sponsorship under NASA Contract NAS7-918
		is acknowledged.        */

/*
 * $Log:	SUNhost.c,v $
 * Revision 1.5  91/07/17  15:06:47  judy
 * New copyright notice.
 * 
 * Revision 1.4  91/07/11  08:59:27  steve
 * fixed multiple prompt bug. Support zero length messages.
 * 
 * Revision 1.3  91/07/10  08:42:04  steve
 * commented out debugging printfs
 * 
 * Revision 1.2  91/07/09  15:15:12  steve
 * 1. Signal driven I/O more robust.
 * 2. Uses mercury like message passing.
 * 3. Support for `log-ing' code
 * 4. Sun4 support
 * 
 * Revision 1.1  90/08/07  11:12:55  configtw
 * Initial revision
 * 
*/
char host_id [] = "@(#)host.c   1.1\t7/7/87\t17:00:46";


#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include "twcommon.h"
#include "twsys.h"
#include "machdep.h"

extern int tw_num_nodes;

extern int msg_port;
extern int ctl_port;

static int fd = 0;
static int ichan[16] = 0;
static int ochan[16] = 0;
static int ctrlc = 0;

int maybe_socket_io;


MicroTime(){}
sndack(){}
rcvack(){}
tester(){}
_pprintf ( ){}

int mlog;
int node_cputime;
int tw_node_num;
int host_input_waiting;
char * bp, *buff;

keyint ()
{
	ctrlc = 1;
}

ioint ()
{
	maybe_socket_io = 1;
}

termint ()
{
	exit ();
}

main ()
{
	char buff[256];
	MSG_STRUCT m;
	int commander ();
	int exit_handler ();
	int i, cc;
	FILE * monout;
	int mask = sigmask ( SIGINT ) | sigmask ( SIGIO );

	monout = fopen ( "MONOUT", "w" );

	m.buf = buff;

	read_config_file ();

	on_exit ( exit_handler, 0 );

	for ( i = 0; i < tw_num_nodes; i++ )
	{
		msg_ichan[i] = ichan[i] = bind_socket ( ctl_port + i );
		msg_ochan[i] = ochan[i] = bind_socket ( ctl_port + i + 16 );
	}

	signal ( SIGIO, ioint );
	signal ( SIGINT, keyint );
	signal ( SIGTERM, termint );

	printf ( "\nready\n\n" );
	fcntl ( fd, F_SETFL, FASYNC+FNDELAY );

	tw_node_num = tw_num_nodes;

	for ( ;; )
	{
		sigblock ( mask );

		if ( maybe_socket_io == 0 )
			sigpause ( 0 );

		sigsetmask ( 0 );

		if ( ctrlc )
		{
			commander ( "*\n" );
			ctrlc = 0;
		}

		if ( maybe_socket_io == 0 )
			continue;

		maybe_socket_io = 0;

		cc = read ( fd, buff, 80 );

		if ( cc > 1 )
		{
/*
			printf ( "CP got %s", buff );
*/
			buff[cc-1] = 0;

			commander ( buff );
		}

		if ( get_msg ( &m ) )
		{
			if ( m.type == TESTER_COMMAND )
			{
				buff[m.mlen] = '\0';
				printf ( "%d--%s", m.source, buff );
				fflush ( stdout );
			}
			else
				handle_cp_msg ( m );
		}
	}
}

commander ( command )

	char * command;
{
	static int node = 0;
	int i, n;

	LowLevelMsgH m;

	m.from_node = CP;
	m.type = TESTER_COMMAND;

	if ( *command == '\n' )
		return;

	if ( strncmp ( command, "stop", 4 )== 0
	||   strncmp ( command, "quit", 4 ) == 0 )
		exit ();

	if ( *command == '*' )
	{
		command++;

		n = strlen ( command );

		for ( i = 0; i < tw_num_nodes; i++ )
		{
			m.to_node = i;
			m.length = n;
			write ( ochan[i], &m, sizeof(m) );
			write ( ochan[i], command, n );
/*
			printf ( "sent node %d: <%s>\n", i, command );
*/
		}

		return;
	}

	i = *command - '0';

	if ( i >= 0 && i < tw_num_nodes )
	{
		node = i;
		m.to_node = i;
		command++;
	}

	if ( *command == '\n' )
		return;

	n = strlen ( command );

	m.length = n;
	write ( ochan[node], &m, sizeof(m) );
	write ( ochan[node], command, n );
/*
	printf ( "sent node %d: <%s>\n", node, command );
*/
}

static exit_handler ()
{
	int i;
	LowLevelMsgH m;

	m.from_node = CP;
	m.type = TESTER_COMMAND;
	m.length = 5;

	fcntl ( 0, F_SETFL, 0 );

	for ( i = 0; i < tw_num_nodes; i++ )
	{
		m.to_node = i;
		write ( ochan[i], &m, sizeof(LowLevelMsgH) );
		write ( ochan[i], "stop\0", 5 );
	}

	for ( i = 0; i < tw_num_nodes; i++ )
	{
		shutdown ( ichan[i], 2 );
		shutdown ( ochan[i], 2 );
	}
}
