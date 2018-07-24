/*	Copyright (C) 1989, 1991, California Institute of Technology.
		U.S. Government Sponsorship under NASA Contract NAS7-918
		is acknowledged.       */	

#include <stdio.h>
#include <fcntl.h>
#include "mplot.h"

extern COBJ cobj[MAX_ITEMS];

extern int first_nobjs, nobjs;
extern int nodes, shift_up, cno;

double delta_real_time;
double max_delta = 0, min_delta = 500000;

read_flow_data ( flowname )

	char * flowname;
{
	FILE * fp;
	int i, n, node;
	char object[20];
	char filename[50];

	strcpy ( filename, flowname );
	strcat ( filename, "mname" );

	fp = fopen ( filename, "r" );

	if ( fp == 0 )
	{
		printf ( "File %s does not exits\n", filename );
		exit (0);
	}

	first_nobjs = nobjs;

	nodes--;

	while ( ( n = fscanf ( fp, "%s %d", object, &node ) ) == 2 )
	{
		i = nobjs;

		if ( i == MAX_OBJS )
		{
			fprintf ( stderr, "exceeded %d objects\n", i );
			exit (1);
		}

		strcpy ( obj[i].name, object );
		obj[i].node = node;
		nobjs++;

		if ( node > nodes )
			nodes = node;
	}

	nodes++;

	printf ( "%d nodes\n", nodes );

	fclose ( fp ) ;

	strcpy ( filename, flowname );
	strcat ( filename, "mmlog" );

	getin ( filename );
}

double tics[32];

getin ( filename )

	char * filename;
{
	int n, fd;
	int old_cno = cno;
	int m, first_good;
	int snder, rcver, min_msg;
/*
	FILE * fp;

	fp = fopen ( "tics", "r" );

	for ( n = 0; n < 32; n++ )
	{
		if ( fp )
		{
			fscanf ( fp, " %d--avg tics/sec %F", &m, &tics[n] );
		}
		else
		{
			tics[n] = 500000.0;
		}

		fprintf ( stderr, "%d--avg tics/sec %f\n", m, tics[n] );

	}

	if ( fp )
	{
		fclose ( fp );
	}
*/
	fd = open ( filename, O_RDONLY, 0 );

	while ( ( n = read ( fd, &cobj[cno], sizeof(cobj[0]) * BLOCKSIZE ) ) > 0 )
	{
		cno += ( n / sizeof(cobj[0]) );

		if ( cno >= MAX_ITEMS )
		{
			fprintf ( stderr, "exceeded %d items\n", MAX_ITEMS );
			break;
		}
	}

	close ( fd );

#if 0
	for ( n = old_cno; n < cno; n++)
	{
		if ( cobj[n].sndtim > INIT_VTIME && cobj[n].rcvtim > INIT_VTIME )
				break;
	}

	first_good = n;

	for ( n = old_cno, m = first_good; m < cno; n++, m++ )
	{
		cobj[n] = cobj[m];

		cobj[n].sndtim += shift_up;
		cobj[n].rcvtim += shift_up;
		cobj[n].snder += first_nobjs;
		cobj[n].rcver += first_nobjs;
/*
		snder = cobj[n].snder;
		rcver = cobj[n].rcver;

		delta_real_time = cobj[n].hgtimet / tics[obj[rcver].node]
						- cobj[n].hgtimef / tics[obj[snder].node];

		if ( delta_real_time > max_delta )
			max_delta = delta_real_time;

		if ( delta_real_time < min_delta )
		{
			min_delta = delta_real_time;
			min_msg = n;
		}
*/
	}

	cno = n;
/*
	snder = cobj[min_msg].snder;
	rcver = cobj[min_msg].rcver;

	fprintf ( stderr, "%f max -- min %f -- min snd %d -- min rcv %d\n",
		max_delta, min_delta, obj[snder].node, obj[rcver].node );
*/
#endif
	fprintf ( stderr, "%d items\n", cno );
/*
	for ( n = 0; n < cno; n++ )
	{
		snder = cobj[n].snder;
		rcver = cobj[n].rcver;

		printf ( "%5d %-16s %.2f %-16s %8.2f\n", n,
			obj[snder].name, cobj[n].sndtim, obj[rcver].name, cobj[n].rcvtim );
	}
*/
}
