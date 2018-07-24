/*
 * $Log:	SUNreadmap.c,v $
 * Revision 1.2  91/07/17  15:59:26  judy
 * New copyright notice.
 * 
 * Revision 1.1  90/08/07  11:12:56  configtw
 * Initial revision
 * 
*/
char readmap_id [] = "@(#)SUNreadmap.c	1.4\t6/2/89\t12:42:48";

/*	Copyright (C) 1989, 1991, California Institute of Technology.
		U. S. Government Sponsorship under NASA Contract NAS7-918
		is acknowledged.	*/

#include <stdio.h>
main ()
{
    FILE * ifp, * ofp;
    int address;
    char type[2];
    char name[30];

    ifp = fopen ( "tester.map", "r" );

    if ( ifp == 0 )
    {
	printf ( "tester.map not found\n" );
	exit ();
    }

    ofp = fopen ( "names", "w" );

    while ( fscanf ( ifp, " %x %s %s", &address, type, name ) == 3 )
    {
	if ( type[0] == 'T' )
	    fprintf ( ofp, "%-30s %x\n", &name[1], address );
    }
}
