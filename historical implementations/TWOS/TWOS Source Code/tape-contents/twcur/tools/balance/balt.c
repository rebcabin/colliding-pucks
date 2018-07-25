/*	Copyright (C) 1989, 1991, California Institute of Technology.
        	U. S. Government Sponsorship under NASA Contract NAS7-918 
                is acknowledged.	*/

/*	Balance Config File	*/
/*      link with sortarrayd.c  */

#define MAXOBJ 2500
#define MAXNODES 128
#include <stdio.h>

struct
{
    char name[20];
    double time;
    int node;
}
    object[MAXOBJ];

int	idxArray[MAXOBJ];
double	valArray[MAXOBJ];

char *progname;

double total;
double goal;
int nobjs;
int nodes;
int verbose;

struct
{
    double total;
}
    node[MAXNODES];

main ( argc, argv )

    int argc;
    char *argv[];
{
    double time;
    int i, n;
    char name[20];
    char buff[200];
    int got_one, low_node;
    double low_total;
    FILE *fpi, *fpo, *fp;
    char *s;

    verbose = 0;

    progname = argv[0];

    if ( argc > 1 )
       if (strcmp("-v", argv[1]) == 0)
       {
          verbose = 1;
          argc--;
          argv++;
       }

    if ( argc != 5 )
    {
	fprintf ( stderr, "usage: %s [-v] trace #nodes cfgin cfgout\n", 
			  progname );
	fprintf ( stderr, "(the trace file is generated by using 'reduce')\n");
	exit (0);
    }

    fp = fopen ( argv[1], "r" );

    if ( fp == 0 )
    {
	fprintf ( stderr, "%s: File %s not found\n", progname, argv[1] );
	exit (0);
    }

    nodes = atoi ( argv[2] );

    if ( nodes < 0 || nodes > MAXNODES )
    {
	fprintf ( stderr, "%s: nodes %d must be 1 to %d\n", progname, nodes , MAXNODES);
	exit (0);
    }

    if (verbose)
       printf ( "Object Names and Times\n\n" );

    total = 0.0;

    for ( i = 0; fgets ( buff, 200, fp ); )
    {
	n = sscanf ( buff, "%s %lf", name, &time);

	if ( n != 2 )
	    continue;

	if (verbose)
           printf ( "%3d) %-20s %lf\n", i, name, time );

	strcpy ( object[i].name, name );

	object[i].time = time;

	total += time;

	idxArray[i] = i;
	valArray[i] = time;

	i++;
    }

    fclose ( fp );

    nobjs = i;

    if (verbose)
    {
       printf ( "\nObject to Node Assignments\n\n" );
       printf ( "Total = %f Nodes = %d Goal = %f\n\n", total, nodes, goal );
    }


    goal = total / nodes;

    for ( n = nobjs - 1; n >= 0; n-- )
    {
	if ( valArray[n] == 0 )
	    continue;

	low_total = total;

	for ( i = 0; i < nodes; i++ )
	{
	    if ( node[i].total < low_total )
	    {
		low_total = node[i].total;
		low_node  = i;
	    }
	}

	node[low_node].total += valArray[n];
	object[idxArray[n]].node = low_node;

	if (verbose)
           printf ( "%2d %-20s %d %lf\n",
		    low_node,
		    object[idxArray[n]].name,
                    valArray[n],
		    node[low_node].total );

	valArray[n] = 0;
    }

    if (verbose)
    {
       printf ( "\nNode Totals\n\n" );
       for ( i = 0; i < nodes; i++ )
	   printf ( "%2d %f\n", i, node[i].total );
    }

    for ( i = 0; i < nobjs; i++ )
    {
	idxArray[i] = i;
	valArray[i] = object[i].node;
    }

    sortarrayd ( valArray, idxArray, nobjs );

    if (verbose)
    {
       printf ( "\nObjects Sorted by Node\n\n" );

       for ( i = 0; i < nobjs; i++ )
       {
	   n = idxArray[i];

	   printf ( "%3d %2d %-20s %lf\n",
                    i,
		    object[n].node,
		    object[n].name,
		    object[n].time );
       }
    }

    fpi = fopen ( argv[3], "r" );

    if ( fpi == 0 )
    {
	printf ( "%s file not found\n", argv[3] );
	exit (0);
    }

    fpo = fopen ( argv[4], "w" );

    while ( fgets ( buff, 200, fpi ) )
    {
	if ( strncmp ( buff, "obcreate", 8 ) == 0 )
	{
	    for ( s = buff + strlen ( buff ) - 1; s > buff; s-- )
	    {
		if ( *s >= '0' && *s <= '9' )
		    break;
	    }
	    for ( ; s > buff; s-- )
	    {
		if ( *s < '0' || *s > '9' )
		    break;
	    }
	    sscanf ( buff, "%*s %s", name );

	    if ( name[0] == '\"' )
	    {
		name[strlen(name)-1] = 0;
		strcpy ( name, name + 1 );
	    }

	    for ( i = 0; i < nobjs; i++ )
	    {
		if ( strcmp ( name, object[i].name ) == 0 )
		    break;
	    }

	    if ( i == nobjs )
	    {
		fprintf ( stderr, "%s: object %s not in %s\n", 
 			           progname, name, argv[1] );
	    }
	    else
	    {
		sprintf ( s + 1, "%2d\n", object[i].node );
	    }
	}

	fputs ( buff, fpo );
    }

    fclose ( fpi );
    fclose ( fpo );
}