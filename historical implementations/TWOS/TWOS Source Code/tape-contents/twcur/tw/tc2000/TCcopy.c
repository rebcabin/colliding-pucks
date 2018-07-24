/*	Copyright (C) 1989, 1991, California Institute of Technology.
		U. S. Government Sponsorship under NASA Contract NAS7-918
		is acknowledged.	*/

/*
 * $Log:	TCcopy.c,v $
 * Revision 1.2  91/07/17  16:22:29  judy
 * New copyright notice.
 * 
 * Revision 1.1  90/12/12  11:05:39  configtw
 * Initial revision
 * 
*/
/* @(#)TCcopy.c	1.1 10/1/90 */

entcpy ( dest, src, numbytes )

    char *dest, *src;
    int numbytes;
{
    while ( numbytes-- )
    {
	*dest++ = *src++;
    }
}
