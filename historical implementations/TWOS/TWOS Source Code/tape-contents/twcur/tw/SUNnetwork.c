/*      Copyright (C) 1989, 1991, California Institute of Technology.
		U. S. Government Sponsorship under NASA Contract NAS7-918
		is acknowledged.        */

/*
 * $Log:	SUNnetwork.c,v $
 * Revision 1.4  91/07/17  15:06:54  judy
 * New copyright notice.
 * 
 * Revision 1.3  91/07/09  13:14:23  steve
 * 1. Signal Driven Sockets
 * 2. robust socket connections
 * 3. treats CP like tw_num_nodes
 * 
 * Revision 1.2  91/06/03  12:23:31  configtw
 * Tab conversion.
 * 
 * Revision 1.1  90/08/07  15:37:50  configtw
 * Initial revision
 * 
*/
char network_id [] = "@(#)SUNnetwork.c  1.11\t6/2/89\t12:42:45\tTIMEWARP";


/*

Purpose:

		The routines in this module are responsible for setting
		up low-level connections between nodes.  These routines
		should operate well below the level of typical systems
		programmer interest.  Some of these routines are executed
		at the start of a run, to set up IPC channels between
		nodes.  One is responsible for closing channels at the
		end of the run.

Functions:

		exit_handler() - close channels to other nodes
				Parameters - int node_num
				Return - always returns 0

		broken_pipe() - handle a broken pipe by dying
				Parameters - none
				Return - Does not return

		init_node(node_num) - Set up all of a node's channels
				Parameters - int nuode_num
				Return - Always returns 0

		read_config_file() - Read a file of node names into the node table
				Parameters - none
				Return - 0, if it returns, but it calls exit() if it fails

		bind_socket(port) - bind a socket to a port, and initialize it
				Parameters - int port
				Return - On success, the file descriptor for the socket;
						calls exit() on failure

		connect_socket(port) - connect to a socket set up by another node
				Parameters - int port
				Return -  On success, the file descriptor for the socket;
						calls exit() on failure

Implementation:

		exit_handler() simply loops, shutting down its end of all
		sockets connecting to other nodes.  broken_pipe() is even
		simpler.  It just prints an error message and exits.  (The
		exit will result in a trap to the special exit routine.)

		init_node() calls read_config_file() to initialize the
		local host table.  It calls connect_socket to set up a
		couple of IO channels local to this node.  Then it iteratively 
		calls bind_socket() to set up socket communications links to 
		all other nodes being used.  init_node() then tells the 
		system to call the routine exit_handler() if the system exits.
		Finally, init_node() arranges that broken pipe signals will
		trap to broken_pipe().

		read_config_file() opens the file "NODES", then reads the
		contents into the host table.  If the open fails, the system
		will die.

		bind_socket(port) makes a system call to socket(), to tell
		the system that the socket it wants to set up has Internet
		addressing, and is of the type "STREAM".  Set a couple of
		options on the socket, so that the system can reuse local
		addresses, and so that sockets will be closed quickly.
		This socket is really only used for setting up the socket
		actually wanted.  Zero out a temporary socket data structure, 
		and set its port field to the desired port.  Bind the file 
		descriptor returned by the earlier socket() call to this 
		structure.  listen() on this socket, to see when the other 
		end will be set up.  Once it is, call accept() to get the 
		file descriptor over which data will really come.  Close
		the socket that was used for temporary setup, and call
		fcntl() to set the appropriate options on the real socket.
		Return this new socket.  If something goes wrong in this
		routine, the system will exit(), rather than proceeding.

		connect_socket() first sleeps for one second, presumably to
		give the node creating the socket time to do so.  Then it
		gets the hostname for the desired port from the host table.
		Having gotten the name, it then retrieves an entire data
		structure for the host.  Next, a call to socket() returns
		an IPC file descriptor.  Zero out a temporary socket
		structure, and copy some information about the host and
		port into it.  Then try to connect() the port.  If the
		attempt fails, sleep one second, and try again.  Keep
		sleeping and trying until the connection comes through.
		Use fcntl() to set the socket options properly, then return
		the socket descriptor.  Except in the case of failure to
		connect, any error in this routine causes the system to 
		exit.

*/

#include <stdio.h>
#include "twcommon.h"
#include "twsys.h"
#include "machdep.h"

static char host[MAX_NODES + 1][20];

#define HOST_LEN 32
char sunname[HOST_LEN];

int tw_num_nodes;

int msg_port = 2048;
int ctl_port = 3072;

int msg_ichan[MAX_NODES + 1];
int msg_ochan[MAX_NODES + 1];

int ctl_ichan;
int ctl_ochan;

exit_handler ( node_num )

	int node_num;
{
	int i;
/*
	printf ( "exit_handler: node_num = %d\n", node_num );
*/
	for ( i = 0; i < tw_num_nodes; i++ )
	{
		if ( i != node_num )
		{
			shutdown ( msg_ichan[i], 2 );
			shutdown ( msg_ochan[i], 2 );
		}
	}
}

broken_pipe ()
{
	printf ( "broken pipe\n" );
	exit ();
}

init_node ( node_num )

	int node_num;
{
	int i;

	read_config_file ();

	ctl_ochan = connect_socket ( ctl_port + node_num );
	ctl_ichan = connect_socket ( ctl_port + node_num + 16 );

	for ( i = 0; i < tw_num_nodes; i++ )
	{
		if ( i > node_num )
		{
			msg_ichan[i] = bind_socket ( msg_port + i*32 + node_num );
			msg_ochan[i] = bind_socket ( msg_port + i*32 + node_num + 16 );
		}
		else
		if ( i < node_num )
		{
			msg_ochan[i] = connect_socket ( msg_port + node_num*32 + i );
			msg_ichan[i] = connect_socket ( msg_port + node_num*32 + i + 16 );
		}
	}

	msg_ochan[tw_num_nodes] = ctl_ochan;
	msg_ichan[tw_num_nodes] = ctl_ichan;

	on_exit ( exit_handler, node_num );
	signal ( SIGPIPE, broken_pipe );
}

read_config_file ()
{
	int node;
	FILE * fp;

	fp = fopen ( "NODES", "r" );

	if ( fp == NULL )
	{
		printf ( "NODES file not found\n" );
		exit ();
	}

	while ( fscanf ( fp, "%d", &node ) == 1 )
	{
		fscanf ( fp, " %s", host[node] );
	}

	tw_num_nodes = node;

	fclose ( fp );

	gethostname ( sunname, HOST_LEN );
}

int bind_socket ( port )

	int port;
{
	int s, r, snew;
	struct sockaddr_in sin;
	struct sockaddr_in from;
	int fromlen;
/*
	printf ( "bind_socket: host = %s, port = %x\n", sunname, port );
*/
	s = socket ( AF_INET, SOCK_STREAM, 0 );

	if ( s == -1 )
	{
		perror ( "socket" );
		exit ();
	}

	if ( setsockopt ( s, SOL_SOCKET, SO_REUSEADDR, 0, NULL ) == -1 )
	{
		perror ( "setsockopt REUSEADDR" );
		exit ();
	}

	if ( setsockopt ( s, SOL_SOCKET, SO_DONTLINGER, 0, NULL ) == -1 )
	{
		perror ( "setsockopt DONTLINGER" );
		exit ();
	}

	bzero ( &sin, sizeof(sin) );

	sin.sin_port = port;

	r = bind ( s, &sin, sizeof(sin) );

	if ( r == -1 )
	{
		perror ( "bind" );
		exit ();
	}

	r = listen ( s, 1);

	if ( r == -1 )
	{
		perror ( "listen" );
		exit ();
	}

	fromlen = sizeof(from);

	snew = accept ( s, &from, &fromlen );

	if ( snew == -1 )
	{
		perror ( "accept" );
		exit ();
	}

	close ( s );

	fcntl ( snew, F_SETOWN, getpid() );

	fcntl ( snew, F_SETFL, FASYNC+FNDELAY );

	return ( snew );
}

int connect_socket ( port )

	int port;
{
	int s, r;
	struct sockaddr_in sin;
	struct hostent *hp;
	char * hostname;
	int count = 0;


	if ( ( port & 0xff00 ) == ctl_port )
		hostname = host[tw_num_nodes];
	else
		hostname = host[port&0x0f];

	hp = gethostbyname ( hostname );

	if ( hp == 0 )
	{
		printf ( "Host %s not found\n", hostname );
		exit ();
	}
/*
	printf ( "connect_socket: host = %s, port = %x\n", hostname, port );
*/
	/* retry_connect: */
	for ( ;; )
	{
		s = socket ( AF_INET, SOCK_STREAM, 0 );

		if ( s == -1 )
		{
			perror ( "socket" );
			exit ();
		}

		bzero ( &sin, sizeof(sin) );
		bcopy ( hp->h_addr, &sin.sin_addr, hp->h_length );
		sin.sin_family = hp->h_addrtype;
		sin.sin_port = port;

		r = connect ( s, &sin, sizeof(sin) );

		if ( r != -1 )
			break;
		if ( ++count == 100 )
		{
			printf ( "connect_socket stuck: host = %s, port = %x\n",
				hostname, port );
			count = 0;
		}
		close ( s );
		sleep ( 1 );
	}

	fcntl ( s, F_SETOWN, getpid() );

	fcntl ( s, F_SETFL, FASYNC+FNDELAY );

	return ( s );
}
