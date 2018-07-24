/*      Copyright (C) 1989, 1991, California Institute of Technology.
		U. S. Government Sponsorship under NASA Contract NAS7-918
		is acknowledged.        */

/*
 * $Log:	command.c,v $
 * Revision 1.6  91/07/17  15:07:35  judy
 * New copyright notice.
 * 
 * Revision 1.5  91/07/09  13:23:03  steve
 * changed SUN version to use get_tester_input. Replaced 2 by SIGNAL_THE_CUBE
 * and 0 by READ_THE_CONSOLE
 * 
 * Revision 1.4  91/06/03  12:23:51  configtw
 * Tab conversion.
 * 
 * Revision 1.3  91/03/26  09:21:04  pls
 * Allow both * and time parameters in a tester command (used by DELFILE).
 * 
 * Revision 1.2  90/12/10  10:39:55  configtw
 * use .simtime field as necessary
 * 
 * Revision 1.1  90/08/07  15:38:04  configtw
 * Initial revision
 * 
*/
char command_id [] = "@(#)command.c     1.52\t9/26/89\t15:38:16\tTIMEWARP";


/*

Purpose:

		command.c contains the code related to the tester command interpreter.
		This code includes modules for reading in lines and extracting
		meaningful tokens from them.

Functions:

		command(prompt) - print a prompt and get a command from the console
				Parameters - char * prompt
				Return - Always returns zero

		set_prompt(string) - set the manual mode prompt
				Parameters - char * string
				Return - Always returns zero

		get_word(word) - read a significant word off the console input
				Parameters - char * word
				Return - Returns zero or exits

		init_command(config_file) - prepare to read the config file
				Parameters - char * config_file
				Return - Always returns zero

		get_line(buff) - read a line of input from the console keyboard
				Parameters -  char * buff
				Return - Always returns zero

		brdcst_command(buff, time) - broadcast a message to all nodes
				Parameters - char * buff, VTime time
				Return - Always returns zero

		new_line() - put a zero at the end of a string
				Parameters - none
				Return - Always returns zero

		error(string) - print an error message
				Parameters - char * string
				Return - Always returns zero

Implementation:

		command() prints the prompt given to it as a parameter, and
		then calls get_word() to read in a command.  It finds the 
		function corresponding to the command in the func_defs[] table,
		gets the needed parameter types from the ard_defs[] table, prompts
		for each, calls get_word() to read in the user's value for each,
		and converts the string read in into the proper form.  It then
		calls the requested function with the provided arguments.

		set_prompt() sets the value of the prompt to its parameter.

		get_word() calls get_line(), if necessary, and then extracts a 
		single word from the line.  (If get_line()'s last invocation left
		some stuff unscanned, then the requested word may already have
		been read into a buffer, and we need only extract it.)  Comments
		are skipped, as is white space.  If necessary, do more get_line()s
		to skip past stuff that isn't significant.

		init_command() opens the config file.

		get_line() prints a prompt.  (If we're on a Mark3, we may call
		dep() first.) On the Mark3, get_line() calls gets() until something 
		is gotten in buff[].  If the first character in buff[] is numeric, 
		convert the alpha representation of the number in buff[] into an int.  
		This number is assumed to be a node number.  If it is out of the
		correct range of node numbers for this run, print an error message
		and prompt again.  After the number is converted, zero the positions
		in buff[] that held it, and look at the remaining string.  Change
		leading '*'s to blanks.  If this node isn't the cmd_node, prompt
		again.  Otherwise, call indep().  If the string is "go", and we're
		on node 0, call send_message() to send a notification to the CP. 
		If we're not on a Mark3, buff[] is filled rather differently,
		with fgets() calls and read()s on ctl_ichan.  However buff[] got
		filled, if there is an '@' in the first position of buff[], treat
		this as an attempt to open a file for read, can call fopen().  If
		the first character was '*', call brdcst_command() for the rest of
		the contents of buff[].  If the contents of buff[] was "nostdout",
		or "maxacks", or "monoff", call brdcst_command() on it.  Otherwise,
		return the line.

		brdcst_command() calls twmsg() to make a COMMAND message, and calls
		brdcst() to send it out.  As long as the broadcasting flag is non-
		null, call send_from_q() to keep sending out copies of the message.

		new_line() zeros bp.

		error() prints a message, followed by a new line.
*/

#include <stdio.h>
#include "twcommon.h"
#include "twsys.h"
#include "tester.h"
#include "machdep.h"

FILE * HOST_fopen ();
char * HOST_fgets ();

#ifdef TRANSPUTER
#include "kmsgh.h"
#endif

char prompt[32];
char buff[512];
char *bp = buff;
FILE *fp_tab[10];
FILE **fp = fp_tab;
extern FUNC_DEF func_defs[];
extern ARG_DEF arg_defs[];
extern SYM_DEF sym_defs[];
extern char standalone;
static int file_echo;

extern int host_input_waiting;

int command ( prompt )  /* returns 1 for ok, 0 for end of file */

	char * prompt;
{
	int f, a, **l, s;
	char tw_function[30];
	char argument[300];
	char *w;

	set_prompt ( prompt );      /* set global "prompt" to parm "prompt" */

get_func:

		/* put a word from the input into tw_function */
		if ( get_word ( tw_function ) == 0 )
			return ( 0 );       /* end of file */

		for ( w = tw_function; *w != 0; w++ )
			*w = toupper ( *w );        /* capitalize the word */

		for ( f = 0; func_defs[f].func_name != 0; f++ )
			if ( strcmp ( tw_function, func_defs[f].func_name ) == 0 )
				break;  /* match with the function list */

		if ( func_defs[f].func_name == 0 )
		{  /* if no match */
			error ( "UNKNOWN FUNCTION" );
			goto get_func;
		}

		if ( tw_node_num == 0 && func_defs[f].bcast )
		{  /* command is to be broadcast */
			brdcst_command ( buff, gvt );
		}

		for ( l = func_defs[f].arg_list; *l != 0; l++ )
		{  /* go through argument list for this command */
			for ( a = 0; arg_defs[a].arg_name != 0; a++ )
				if ( *l == arg_defs[a].arg_address )
					break;      /* find the match in the arg list */

			set_prompt ( arg_defs[a].arg_name );        /* get the arg name */

get_arg:

			if ( get_word ( argument ) == 0 )   /* get next argument */
				return ( 0 );                   /* ran out of arguments */

			switch ( arg_defs[a].arg_type )
			{  /* expecting the following argument type */
				case NAME:

					if ( strlen ( argument ) >= 30 )
					{  /* max of 29 chars for object name */
						error ( "OBJECT NAME TOO LONG" );
						goto get_arg;
					}

					strcpy ( *l, argument );    /* store the argument */
					break;

				case STIME:

					/* store the argument */
					if ( sscanf ( argument, "%lf", *l ) == 0 )
					{
						error ( "INVALID VIRTUAL TIME" );
						goto get_arg;
					}
					break;

				case STRING:

					strcpy ( *l, argument );    /* store the argument */
					break;

				case INTEGER:

					/* store the argument */
					if ( sscanf ( argument, "%d", *l ) == 0 )
					{
						error ( "NOT AN INTEGER" );
						goto get_arg;
					}
					break;

				case SYMBOL:

					for ( w = argument; *w != 0; w++ )
						*w = toupper ( *w );

					/* symbolic argument:  find it in the list */
					for ( s = 0; sym_defs[s].symbol != 0; s++ )
						if ( strcmp ( argument, sym_defs[s].symbol ) == 0 )
							break;

					if ( sym_defs[s].symbol == 0 )
					{
						error ( "UNKNOWN SYMBOL" );
						goto get_arg;
					}

					/* store the argument */
					**l = sym_defs[s].value;
					break;

				case HEX:

#ifndef TRANSPUTER 
					/* store the argument */
					if ( sscanf ( argument, "%lx", *l ) == 0 )
					{
						error ( "NOT HEX ENOUGH" );
						goto get_arg;
					}
					break;

#else   /* TRANSPUTER */

					if ( sscanf ( argument, "%lx", *l ) == 0 )
					{
						error ( "NOT HEX ENOUGH" );
						goto get_arg;
					}

					*l |= 0x80000000;

					break;
#endif
				case REAL:

					/* store the argument */
					if ( sscanf ( argument, "%lf", *l ) == 0 )
					{
						error ( "NOT REAL ENOUGH" );
						goto get_arg;
					}
					break;
			}  /* switch ...arg_type */
		}  /* for ( l = func_defs[f].arg_list */

		l = func_defs[f].arg_list;

		/* now execute the associated routine, passing 6 arguments */
		( * func_defs[f].routine )
				( *l, *(l+1), *(l+2), *(l+3), *(l+4), *(l+5) );

#ifdef TRANSPUTER

		/*
		We don't want to ACK calls to command as a result
		of the TW "COMMAND" msg type.
		*/

		if ( tw_node_num != 0  && host_input_waiting )
		{
			xsend ( buff, /* len */ 0, /* node */ 0, CMD_ACK_TYPE );
		}

#endif

	return ( 1 );
}

set_prompt ( string )

/* store "string" in the global "prompt" */

	char * string;
{
	sprintf ( prompt, "%s: ", string );
}

int in_a_comment;

int get_word ( word )   /* returns 1 for ok, 0 for end of file */

	char *word;
{
	int stat;
	char *w;
	char c;
	w = word;

start:

	if ( *bp == 0 )
	{  /* get a new line (and handle command msgs, broadcasts, @, etc.) */
		stat = get_line ( bp = buff );

		if ( stat == 0 )
			goto end;   /* done */

		if ( *bp == '#' )
		{  /* comment line */
			*bp = 0;
			goto start;
		}
	}

	while ( *bp == ' ' || *bp == '\t' || *bp == '\n' )
		bp++;   /* skip over white space */

	if ( *bp == 0 )
		goto start;     /* line is blank or already processed */

	if ( *bp == '/' && *(bp+1) == '*' )
	{  /* start of comment found */
		in_a_comment = TRUE;

		while ( *bp != '*' || *(bp+1) != '/' )
		{  /* loop til end of comment */
			if ( *bp == 0 )
			{
				stat = get_line ( bp = buff );

				if ( stat == 0 )
				{
					_pprintf ("Unterminated Comment\n");
					goto end;
				}
			}
			else
				bp++;
		}  /* comment has ended */
		in_a_comment = FALSE;
		bp += 2;
		goto start;
	}

	if ( *bp == '\"' )
	{  /* double quote found */
		bp++;

		while ( *bp != 0 && *bp != '\"' )
		{  /* accumulate word within double quote */
			*w++ = *bp++;
		}
		if ( *bp == '\"' )
			bp++;
	}
	else
	while ( *bp != 0 && *bp != ' ' && *bp != '\t' && *bp != '\n' )
	{
		c = *bp++;

		if ( c == '*' || c == '(' || c == ')' || c == ',' || c == ';' )
			continue;   /* skip these if first characters */

		*w++ = c;

		c = *bp;

		if ( c == '*' || c == '(' || c == ')' || c == ',' || c == ';' )
			break;      /* word terminators */
	}

	if ( w == word )
		goto start;     /* nothing found yet */

end:
	*w = 0;     /* terminate the word */

	return ( stat );
}  /* get_word */

init_command ( config_file )

	char * config_file;
{
	*(++fp) = HOST_fopen ( config_file, "r" );  /* open the config file */

	if ( *fp == NULL )
	{   /* error in opening file */
		_pprintf ( "%s File Not Found\n", config_file );
		fp--;
#ifdef SUN
		host_input_waiting = 1;
#endif
#ifdef MARK3
		send_message ( 0, 0, CP, SIGNAL_THE_CUBE );/* tell CP to signal nodes */
#endif
#ifdef BBN
		interrupt_nodes ();
#endif
	}
	else
	{   /* file opened successfully */
		_pprintf ( "Reading %s\n", config_file );

		host_input_waiting = 1;

		while ( host_input_waiting && command ( "CONFIG" ) )
			;   /* read in and execute config file commands */

		if ( host_input_waiting )
		{
			host_input_waiting = 0;

			gvtinit (); /* set up gvt interrupts */
#ifdef DLM
			loadinit ();
#endif DLM
		}
	}
}

enable_file_echo ()
{
	file_echo = TRUE;
}

disable_file_echo ()
{
	file_echo = FALSE;
}

close_command_file ()
{
	while ( *fp != NULL )
	{
		HOST_fclose ( *fp );

		*fp-- = NULL;
	}
}

int get_line ( buff )   /* returns 1 for ok, 0 for end of file */

	char *      buff;
{
#ifdef TRANSPUTER               /* for xrecv */
	int len;
	int rtn_len, rtn_src, rtn_type;
#endif

	int         i;
	int stat = 1;
	char * stat2;

#ifdef MARK3
	static int cmd_node;
#endif

	if ( rm_msg != NULL && rm_msg->mtype == COMMAND )
	{
		if ( gtSTime ( rm_msg->rcvtim.simtime, gvt.simtime ) )
		{
			Msgh * tw_msg = (Msgh *) l_create ( msgdefsize );
			if ( tw_msg )
			{
				entcpy ( tw_msg, rm_msg, sizeof(Msgh) + rm_msg->txtlen );
				enq_command ( tw_msg );
				buff[0] = 0;
				stat = 0;
			}
			else
			_pprintf ( "couldn't allocate memory for a command message\n" );
		}
		else
			strcpy ( buff, rm_msg + 1 );

		acceptmsg ( NULL );
	}

#ifdef TRANSPUTER

	else if ( tw_node_num != 0 )
	{
		while 
		( 
			(
			  rtn_len = 
			  xrecv ( buff, 512, /* src node */ 0, CMD_TYPE, &rtn_src, & rtn_type )
			) 
			== -1
		)
		{;}
	}
#endif

	else
	for ( ;; )
	{
		if ( *fp == NULL )
		{  /* get from std input */

#ifdef MARK3
			dep ();
prompt_again:
			printf ( "%d--%s", cmd_node, prompt );
#endif

#ifdef SUN
			_pprintf ( "%s", prompt );
#endif


#ifdef  BBN /*PJH Not sure about this one for BF_MACH... */

			butterfly_prompt ( prompt );
			butterfly_recv_command ( buff );
			stat    =1;
#endif


#ifdef TRANSPUTER
prompt_again:
			_pprintf ( "%s", prompt );
#endif

get_again:

#ifdef TRANSPUTER

/*
The machine dependent pieces of code below input info by some variant
of gets and put it into a buff.  Once in the buff, it is processed in
the same way as the input that is read in from a file.  This is done
in the non-machine dependent part of get_line below.

The mk3 code below handles the input of node numbers and also detects
the illegal entry of the command "go" to some node other than the
command node.  The cmd_node is responsible for telling the CP to signal
all the nodes (which will ultimately set the variable host_input_waiting)
to tell them to go.
*/


		/*
		Note that the user will have to type
		3 go
		2 go
		1 go
		go
		*/

			if ( tw_node_num == 0 )
			{
				kgets ( buff );

				printf ( "\n" );

				if ( buff[0] >= '0' && buff[0] <= '9' )
				{
					int node = atoi ( buff );
					if ( node >= tw_num_nodes )
					{
						SEMGET
						printf ( "Invalid Node #\n" );
						SEMFREE
						goto prompt_again;
					}

					buff[0] = ' ';
					if ( buff[1] >= '0' && buff[1] <= '9' )
						buff[1] = ' ';

					len = strlen ( buff ) + 1;

					xsend ( buff, len, node, CMD_TYPE );

					/* make the buff NULL so it isn't processed locally */

					buff[0] = NULL;

					/* wait for acknowledgement of command */

					while 
					( 
						( rtn_len = 
						  xrecv ( buff, 512, node, CMD_ACK_TYPE, &rtn_src, & rtn_type )
						) == -1
					)
					{;}
				}
			}

#endif  /* Transputer */

#ifdef MARK3
			stat2 = gets ( buff );

			if ( stat2 == NULL )
				goto get_again;

			printf ( "\n" );

			if ( buff[0] >= '0' && buff[0] <= '9' ) 
			{ 
				int node = atoi ( buff );
				if ( node >= tw_num_nodes )
				{
					printf ( "Invalid Node #\n" );
					goto prompt_again;
				}
				cmd_node = node;

				buff[0] = ' ';
				if ( buff[1] >= '0' && buff[1] <= '9' )
				{
					buff[1] = ' ';
					if ( buff[2] >= '0' && buff[2] <= '9' )
						buff[2] = ' ';
				}
			}

			if ( buff[0] == '*' && tw_node_num < tw_num_nodes ) /* all nodes */
				buff[0] = ' ';
			else
			if ( cmd_node != (tw_node_num + node_offset)
			&&   strncmp ( buff, "go", 2 ) != 0 )
				goto prompt_again;

			indep ();

			if ( strncmp ( buff, "go", 2 ) == 0 )
			{
				if ( (tw_node_num + node_offset) == 0 )
				{
					send_message ( 0, 0, CP, READ_THE_CONSOLE ); /* tell CP to take console */
				}
			}
#endif
#ifdef SUN
			if ( standalone )
			{
				stat2 = fgets ( buff, 80, stdin );

				if ( stat2 == NULL )
					tw_exit ( 0 );

				buff[strlen(buff)-1] = 0; /* kill \n */
			}
			else
			{
				get_tester_input (); /* waits for input */
			}
#endif
		}
		else    /* *fp <> NULL */
		{
			stat2 = HOST_fgets ( buff, 500, *fp );      /* read next line */

			if ( stat2 == NULL )
			{   /* error or EOF */
				HOST_fclose ( *fp );

				*fp-- = NULL;
				_pprintf ( "End of File\n" );

				if ( *fp == NULL )
				{
					buff[0] = 0;
					return ( 0 );
				}
				continue;
			}
			else
			{
				buff[strlen(buff)-1] = 0; /* kill \n */

				if ( file_echo )
					_pprintf ( "%s\n", buff );  /* echo it */
			}
		}       /* if (*fp <> NULL) ... else */

		if ( in_a_comment )
			break;

		if ( *buff == '@' )
		{       /* read another file */

			*(++fp) = HOST_fopen ( buff+1, "r" );       /* open new file */

			if ( *fp == NULL )
			{
				_pprintf ( "%s File Not Found\n", buff+1 );
				fp--;
			}
			_pprintf ( "Reading %s\n", buff+1 );        /* tell the world */
			continue;
		}
		else
		if ( buff[0] >= '0' && buff[0] <= '9' )
			{
			/* get node # */
			int node = atoi ( buff ) % tw_num_nodes;

			for ( i = 0; buff[i] >= '0' && buff[i] <= '9'; i++ )
				buff[i] = ' ';  /* blank out first number */

			while ( buff[i] > 0 && buff[i] <= ' ' )
				i++;    /* skip over control characters & spaces */

			if ( buff[i] >= '0' && buff[i] <= '9' )
				{
				/* get time value */
				int time = atoi ( buff + i );

				for ( ; buff[i] >= '0' && buff[i] <= '9'; i++ )
					buff[i] = ' ';      /* blank out second number */

				/* send this line out as a command */
				send_command ( buff, node, newVTime ( (STime)time, 0, 0 ) );
				buff[0] = 0;    /* flag that this line is processed */
				}
			else
				if ( node != tw_node_num )
					{  /* no time value--use gvt */
					send_command ( buff, node, gvt );
					buff[0] = 0;        /* flag that this line is processed */
					}
			}   /* if node # found */
		else
			if ( buff[0] == '*' && buff[1] != '/' )
				{  /* it's a broadcast command */
				buff[0] = ' ';

				i = 1;
				while ( buff[i] > 0 && buff[i] <= ' ' )
					i++;        /* skip over control characters & spaces */

				if ( buff[i] >= '0' && buff[i] <= '9' )
					{
					/* get time value */
					int time = atoi ( buff + i );

					for ( ; buff[i] >= '0' && buff[i] <= '9'; i++ )
						buff[i] = ' ';  /* blank out second number */

					/* send this line out as a broadcast command */
					brdcst_command ( buff, newVTime ( (STime)time, 0, 0 ) );
					
					/* also send to this node (actually call enq_command) */
					send_command(buff,tw_node_num,newVTime((STime)time,0,0));
					buff[0] = 0;        /* flag that this line is processed */
					}
				else
					brdcst_command ( buff, gvt );
				}

		break;  /* break out of loop when we have a line */
	}  /* for(;;) */

	return ( stat );
}  /* get_line */

FUNCTION brdcst_command ( buff, time )

	char *      buff;
	VTime       time;
{
	Msgh * tw_msg;
	int len;

	len = strlen ( buff ) + 1;

	/* create the message ??? need to test for NULL */
	tw_msg = make_message ( (Byte) COMMAND,
				"TW",     /* snder */
				gvt,      /* sndtim */
				"TW",     /* rcver */
				time,      /* rcvtim */
				len,    /* txtlen */
				(Byte *) buff
				);

	tw_msg->flags |= SYSMSG ;

	brdcst ( tw_msg, sizeof ( Msgh ) + len ) ;

	while ( brdcst_msg != NULL )
		send_from_q ();

	acceptmsg ( NULL ); /* ignore the incoming message */
}

FUNCTION send_command ( buff, node, time )

	char * buff;
	int node;
	VTime time;
{
	Msgh * tw_msg;
	int len;

	len = strlen ( buff ) + 1;

	/* create the message ??? need to test for NULL */
	tw_msg = make_message ( (Byte) COMMAND,
				"TW",   /* snder */
				gvt,    /* sndtim */
				"TW",   /* rcver */
				time,   /* rcvtim */
				len,    /* txtlen */
				(Byte *) buff
				);

	tw_msg->flags |= SYSMSG ;

	if ( node == tw_node_num )
	{  /* going to this node */
		enq_command ( tw_msg ); /* put into command_queue */
	}
	else
	{  /* send off-node */
		sndmsg ( tw_msg, sizeof ( Msgh ) + len, node ) ;

		send_from_q (); /* ??? already done by sndmsg */
	}
}

new_line ()
{
	*bp = 0;
}

static error ( string )

	char * string;
{
	_pprintf ( "%s\n", string );

	new_line;
}


