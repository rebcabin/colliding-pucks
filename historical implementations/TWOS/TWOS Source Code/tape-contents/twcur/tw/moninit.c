/*      Copyright (C) 1989, 1991, California Institute of Technology.
		U. S. Government Sponsorship under NASA Contract NAS7-918
		is acknowledged.        */

/*
 * $Log:	moninit.c,v $
 * Revision 1.3  91/07/17  15:10:26  judy
 * New copyright notice.
 * 
 * Revision 1.2  91/06/03  12:25:25  configtw
 * Tab conversion.
 * 
 * Revision 1.1  90/08/07  15:40:19  configtw
 * Initial revision
 * 
*/
char moninit_id [] = "@(#)moninit.c     1.23\t6/2/89\t12:44:37\tTIMEWARP";


#ifndef TRANSPUTER

/*

Purpose:

		moninit.c contains code that initializes the monitoring function
		of Time Warp, primarily by reading several files into arrays,
		thereby giving the monitor knowledge of where in its load modules
		various functions are located, and the number and kind of parameters
		accepted by those functions.

Functions:

		moninit() - initializes the monitor
				Parameters - none
				Returns - Always returns zero

		load_funcs() - fill the mon_funcs[] table
				Parameters - none
				Return -  Returns zero on success, calls tw_exit() on failure

		load_nt() - fill the mon_array[] table
				Parameters - none
				Return - Returns zero on success, calls tw_exit() on failure

		sort(array,x,num) - perform a bubble sort on an array
				Parameters - long array[][2], int x, int num
				Return - Always returns zero

		load_levels() - load function monitor levels into the mon_func[] table
				Parameters - none
				Return - Returns zero on success, calls tw_exit() on failure

		set_level(name,level) - set an individual level entry in the mon_func[]
								table
				Parameters - char *name, int level
				Return - Always returns zero

		str(word) - copy a string into a string holding area
				Parameters - char *word
				Return - a pointer to the string in the holding area,
								if successful; tw_exit() is called on failure

		getfunc(func) - read in a function's parameters
				Parameters - FUNC *func
				Return - func, if successful; zero for one kind of failure; 
						tw_exit() is called for another kind of failure

		getword(word) - read in one word from a function description file
				Parameters - char *word
				Return - Returns zero on success, calls tw_exit() on failure

		gettype(word) - read in a function's data type
				Parameters - char *word
				Return - an index into the types[] table, if successful;
						calls tw_exit() on failure

		typestr(num) - find a type in the types[] table
				Parameters - int num
				Return - a pointer to the type string, if found; a
						pointer to an error string, if not found

Implementation:
				
		The business of routines in this module is largely reading formatted
		files and transferring their contents to monitor arrays, for use
		when the monitor is needed.  If moninit() is called at all, filling
		these arrays is regarded as vital system business, and failure
		results in a tw_exit from Time Warp.

		moninit() calls several functions that actually read in tables, then
		it turns the monitor on.

		load_funcs() iteratively calls getfunc(), filling up the mon_funcs[]
		table.  As a side effect, the types[] table will be filled, as the
		first call to getfunc() will cause a call to gettype(), and the first
		call to gettype() will read in the types table.  Also, getfunc()
		will fill the mon_strs[] table with the names of the functions.

		load_nt() reads in a file that contains the starting addresses of
		all functions in the system.  (This file is produced at load time
		by the UNIX nm utility.)  The string names of the functions are
		compared against the strings in the mon_funcs[] table.  A match is
		used as an index into the mon_array[] table, which will hold the
		beginning point for functions.  This table will be used by the
		monitor when it is trying to figure out which function called
		mad_monitor().  (See monitor.c for details.)  After the mon_array[]
		table has been filled, sort() will be used to put it in order.

		sort() is a bubble sort used exclusively to sort the mon_array[],
		though it is sufficiently general to be used for other purposes.

		load_levels() is called by moninit().  It reads a file describing
		the diagnostic levels for the various Time Warp functions handled
		by the monitor.  Higher level numbers imply more diagnostic action
		taken by the monitor when called for that function.  For each function,
		load_levels() calls set_level() to put the level information in
		the mon_funcs[] table.

		set_level() gets pointers from the mon_func[] table to examine 
		function names in the mon_strs[] table.  Once a match is found
		to the string given in set_level()'s parameter, the appropriate
		entry in the mon_func[] table has been found, and its level field
		is set to the level provided in this function's other parameter.

		str() puts strings into the mon_strs[] table.  It does not try
		to prevent duplications.  The strings actually being put in the
		table are the names of functions.  The mon_func[] table entries
		hold a pointer into the mon_strs[] table, rather than keeping
		the name in the mon_func[] table entry itself.  Presumably, this
		method is used because function names vary greatly in length,
		and less space is wasted by storing them all in a single, packed
		table.

		getfunc() reads in information about a function for storage in
		the mon_func[] table.  Reading in the information is moderately
		complicated, as the file has a somewhat complicated format, to
		allow simple, human-readable expression of the information it
		contains.  The function's name, type, and list of arguments is
		read in.  The most complex part of the routine is the reading
		in of the argument list.  This part is best understood if one
		realizes that what is being read is a typical C function 
		definition argument list.

		getword() is a utility used by getfunc() to get one token at
		a time.  It skips over white space, and recognizes certain
		characters as token delimiters, such as ",", ";", "*", "(".
		and ")".  Everything up to, but not including, the delimiter is
		returned.  The delimiter itself will be returned as a separate
		word on the next call to getword().

		gettype() reads in the data types file, if it hasn't been read in 
		already.

*/

#include <stdio.h>
#include "twcommon.h"
#include "twsys.h"
#include "func.h"

FUNC * getfunc ();

#define MAX_FUNCS 400

FUNC mon_func[MAX_FUNCS];
int num_mon_funcs = 0;
long mon_array[MAX_FUNCS][2];

#define STR_SIZE 8192

char mon_strs[STR_SIZE];

int mon_str_len;

static char * monstrp = mon_strs;

extern int tw_node_num;

moninit ()
{
#ifndef SUN
	if ( tw_node_num == 0 )
#endif
		printf ( "Monitor Initialization beginning...\n" );
	load_funcs ();
	load_nt ();
	load_levels ();
	monon ();
#ifndef SUN
	if ( tw_node_num == 0 )
#endif
		printf ( "Monitor Initialization complete\n" );
}

load_funcs ()
{
/*
	char *n;
*/

	num_mon_funcs = 1;

	while ( getfunc ( &mon_func[num_mon_funcs] ) )
	{
/*
		for ( n = &mon_strs[mon_func[num_mon_funcs].name]; *n != 0; n++ )
			*n = toupper ( *n );
*/
/*
		printf ( "%2d %s\n", num_mon_funcs,
						&mon_strs[mon_func[num_mon_funcs].name] );
*/
		num_mon_funcs++;
		if ( num_mon_funcs >= MAX_FUNCS )
		{
			printf ( "Too Many Functions\n" );
			tw_exit ();
		}
	}

#ifndef SUN
	if ( tw_node_num == 0 )
#endif
		printf ("%d functions loaded...\n", num_mon_funcs) ;

	mon_func[0] = mon_func[num_mon_funcs];      /* UNKNOWN */
}

load_nt ()
{
	FILE * fp;
	char module[60], *name;
	long beg, end = 0;
	int n;

	fp = fopen ( "names", "r" );

	if ( fp == NULL )
	{
		printf ( "names file not found\n" );
		tw_exit (0);
	}

	for ( ;; )
	{
		n = fscanf ( fp, "%s %lx", module, &beg );
/*
		printf ( "map: n = %d module = %s beg = %x\n", n, module, beg );
*/
		if ( n != 2 )
			break;

		for ( name = module; *name != 0; name++ )
			if ( *name == ':' )
			{
				name++;
				break;
			}
		if ( *name == 0 )
			name = module;

		for ( n = 0; n < num_mon_funcs; n++ )
			if ( strcmp ( name, &mon_strs[mon_func[n].name] ) == 0 )
				break;

		if ( n < num_mon_funcs )
		{
			mon_func[n].beg = beg;
			mon_func[n].end = end;
		}
	}

	fclose ( fp );

	for ( n = 0; n < num_mon_funcs; n++ )
	{
		mon_array[n][0] = mon_func[n].beg;
		mon_array[n][1] = n;
	}

	sort ( mon_array, 0, num_mon_funcs );
/*
	mon_array[n][0] = mon_func[n-1].end;
*/
	mon_array[n][0] = 0x7fffffff;
	mon_array[n][1] = n;
/*
	printf ( "\n" );
	for ( n = 0; n < num_mon_funcs; n++ )
		printf ( "%2d %lx %ld %s\n", n, mon_array[n][0], mon_array[n][1],
				&mon_strs[mon_func[mon_array[n][1]].name] );
*/
}

static sort (array, x, num)

	long array[][2];
	int x, num;
{
	int i, j;
	long k;

	for (i=0; i<num-1; i++)
	{
		if (array[i][x] > array[i+1][x])
		{
			for (j=i; j>=0; j--)
			{
				if (array[j][x] > array[j+1][x])
				{
					k = array[j][x];
					array[j][x] = array[j+1][x];
					array[j+1][x] = k;
					k = array[j][1-x];
					array[j][1-x] = array[j+1][1-x];
					array[j+1][1-x] = k;
				}
			}
		}
	}
}

static set_level ( name, level )

	char *name;
	int level;
{
/*
	char *s;
*/
	int n;
/*
	for ( s = name; *s != 0; s++ )
		*s = toupper ( *s );
*/
	for ( n = 0; n < num_mon_funcs; n++ )
		if ( strcmp ( name, &mon_strs[mon_func[n].name] ) == 0 )
			break;

	if ( n < num_mon_funcs )
	{
		mon_func[n].level = (char) level;
	}
	else
		printf ( "Function %s Not Found\n", name );
}

load_levels ()
{
	FILE * fp;
	char name[60];
	int level;

	fp = fopen ( "levels", "r" );

	if ( fp == NULL )
	{
		printf ( "levels file not found\n");
		tw_exit (0);
	}

	while ( fscanf ( fp, "%s %d", name, &level ) == 2 )
	{
		set_level ( name, level );
	}

	fclose ( fp );
}

int str ( word )

	char * word;
{
	char * w = word;

	int offset = monstrp - mon_strs;

	while ( *monstrp++ = *w++ )
		;

	if ( monstrp > mon_strs + STR_SIZE )
	{
		printf ( "String Area Full\n" );
		tw_exit ();
	}

	return ( offset );
}


FUNC * getfunc ( func )

	FUNC * func;
{
	char word[60];
	char type[60];
	char xtra[60];

	int i, j, k, n;

	clear ( func, sizeof (FUNC) );

	getword ( type );

	if ( *type == ';' )
		getword ( type );

	if ( *type == 0 )
	{
		func->name = str ( "UNKNOWN" );
		mon_str_len = str  ( "" );
		return ( 0 );
	}

	if ( strcmp ( type, "static" ) == 0 )
		getword ( type );

	if ( strcmp ( type, "struct" ) == 0 )
		getword ( type );

	for ( ;; )
	{
		getword ( word );

		if ( *word == '*' )
			func->fstars++;
		else
			break;
	}

	if ( *word == '(' )
	{
		func->name = str ( type );
	}
	else
	{
		func->name = str ( word );
		func->ftype = gettype ( type );
		getword ( word );       /* better be '(' */
	}

	for ( n = 0; ; )            /* get arg list */
	{
		getword ( word );

		if ( *word == ')' )
			break;

		if ( *word != ',' )
		{
			if ( n >= 10 )
			{
				printf ( "Too Many Args: %s\n", &mon_strs[func->name] );
				tw_exit ();
			}
			func->args[n++] = str ( word );
		}
	}

	for ( i = 0; i < n; i++ )   /* get arg defs */
	{
		getword ( word );

		if ( *word == ';' )
			getword ( word );

		if ( strcmp ( word, "register" ) == 0 )
			getword ( word );

		if ( *word != ',' )
			strcpy ( type, word );

		for ( k = 0; ; k++ )
		{
			getword ( word );

			if ( *word != '*' )
				break;
		}

		if ( *word == '(' )
		{
			getword ( word );   /* should be *    */
			getword ( word );   /* should be Func */
			getword ( xtra );   /* should be )    */
			getword ( xtra );   /* should be (    */
			getword ( xtra );   /* should be )    */
			strcpy ( type, "(*Func)()" );
		}

		for ( j = 0; j < n; j++ )
			if ( strcmp ( word, &mon_strs[func->args[j]] ) == 0 )
				break;

		func->types[j] = gettype ( type );
		func->stars[j] = k;
	}

	return ( func );
}

#define BUFSIZE 100

getword ( word )

	char *word;
{
	static char buff[BUFSIZE];
	static char *bp = buff;
	static FILE * fp = 0;
	char * stat;
	char *w = word;
	char c;

	if ( fp == NULL )
	{
		fp = fopen ( "str", "r" );

		if ( fp == NULL )
		{
			printf ( "str file not found\n" );
			tw_exit (0);
		}
	}

start:

	if ( *bp == 0 )
	{
		stat = fgets ( bp = buff, BUFSIZE-1, fp );

		if ( stat == 0 )
		{
			fclose ( fp );
			goto end;
		}
	}

	while ( *bp == ' ' || *bp == '\t' || *bp == '\n' )
		bp++;

	if ( *bp == 0 )
		goto start;

	if ( *bp == '/' && *(bp+1) == '*' )
	{
		while ( *bp != '*' || *(bp+1) != '/' )
		{
			if ( *bp == 0 )
			{
				stat = fgets ( bp = buff, BUFSIZE-1, fp );

				if ( stat == 0 )
				{
					printf ("Unterminated Comment\n");
					tw_exit ();
				}
			}
			else
				bp++;
		}
		bp += 2;
		goto start;
	}

	while ( *bp != 0 && *bp != ' ' && *bp != '\t' && *bp != '\n' )
	{
		c = *w++ = *bp++;

		if ( c == '*' || c == '(' || c == ')' || c == ',' || c == ';' )
			break;

		c = *bp;

		if ( c == '*' || c == '(' || c == ')' || c == ',' || c == ';' )
			break;
	}

end:
	*w = 0;
}

static char types[50][30];

int gettype ( word )

	char *word;
{
	static FILE * fp = 0;
	int i, n, rc;
	char type[30];

	if ( fp == NULL )
	{
		fp = fopen ( "datatypes", "r" );

		if ( fp == NULL )
		{
			printf ( "datatypes file not found\n" );
			tw_exit (0);
		}

		while ( ( rc = fscanf ( fp, "%d %s", &n, type ) ) == 2 )
		{
			strcpy ( types[n], type );
		}

		fclose ( fp );
	}

	for ( i = 1; types[i][0] != 0; i++ )
		if ( strcmp ( word, types[i] ) == 0 )
			break;

	return ( i );
}

#endif  /* TRANSPUTER */
