/*	Copyright (C) 1989, 1991, California Institute of Technology.
		U. S. Government Sponsorship under NASA Contract NAS7-918
		is acknowledged.	*/

/*
 * $Log:	TCclear.c,v $
 * Revision 1.2  91/07/17  16:22:16  judy
 * New copyright notice.
 * 
 * Revision 1.1  90/12/12  11:05:30  configtw
 * Initial revision
 * 
*/
/* @(#)TCclear.c	1.1 10/1/90 */

clear ( addr, numbytes )

    register char *addr;
    register int numbytes;
{
    while ( numbytes-- )
    {
	*addr++ = 0;
    }
}
