/*      Copyright (C) 1989, 1991, California Institute of Technology.
		U. S. Government Sponsorship under NASA Contract NAS7-918
		is acknowledged.        */

/*
 * $Log:	SUN4comp.c,v $
 * Revision 1.3  91/07/17  16:15:41  judy
 * New copyright notice.
 * 
 * Revision 1.2  91/07/10  10:46:12  steve
 * Use bcmp system call.
 * 
 * Revision 1.1  90/08/07  13:36:04  configtw
 * Initial revision
 * 
*/

int bytcmp ( a, b, l )

	register unsigned char *a, *b;
	register int l;
{
	return bcmp ( b, a, l );
#if 0
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
#endif
}
