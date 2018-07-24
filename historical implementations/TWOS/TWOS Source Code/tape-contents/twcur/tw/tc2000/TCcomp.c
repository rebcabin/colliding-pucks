/*	Copyright (C) 1989, 1991, California Institute of Technology.
		U. S. Government Sponsorship under NASA Contract NAS7-918
		is acknowledged.	*/

/*
 * $Log:	TCcomp.c,v $
 * Revision 1.2  91/07/17  16:22:25  judy
 * New copyright notice.
 * 
 * Revision 1.1  90/12/12  11:05:35  configtw
 * Initial revision
 * 
/*
/* @(#)TCcomp.c	1.1 10/1/90 */

int bytcmp ( a, b, l )

    register unsigned char *a, *b;
    register int l;
{
    while ( l-- )
    {
	if ( *a < *b )
	    return ( -1 );
	else
	if ( *a > *b )
	    return ( 1 );

	a++;  b++;
    }

    return ( 0 );
}
