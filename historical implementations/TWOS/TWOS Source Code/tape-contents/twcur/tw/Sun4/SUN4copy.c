/*      Copyright (C) 1989, 1991, California Institute of Technology.
		U. S. Government Sponsorship under NASA Contract NAS7-918
		is acknowledged.        */

/*
 * $Log:	SUN4copy.c,v $
 * Revision 1.3  91/07/17  16:15:46  judy
 * New copyright notice.
 * 
 * Revision 1.2  91/07/10  10:46:27  steve
 * Use bcopy system call.
 * 
 * Revision 1.1  90/08/07  13:36:05  configtw
 * Initial revision
 * 
*/

entcpy ( dest, src, numbytes )

	char *dest, *src;
	int numbytes;
{
	bcopy ( src, dest, numbytes );
#if 0
	while ( numbytes-- )
	{
		*dest++ = *src++;
	}
#endif
}
