/*	Copyright (C) 1989, 1991, California Institute of Technology.
		U.S. Government Sponsorship under NASA Contract NAS7-918
		is acknowledged.       */

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
