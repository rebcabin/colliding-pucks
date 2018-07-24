/*      Copyright (C) 1989, 1991, California Institute of Technology.
		U. S. Government Sponsorship under NASA Contract NAS7-918
		is acknowledged.        */

/*
 * $Log:	SUN4clear.c,v $
 * Revision 1.3  91/07/17  16:15:34  judy
 * New copyright notice.
 * 
 * Revision 1.2  91/07/10  10:45:44  steve
 * Use bzero system call.
 * 
 * Revision 1.1  90/08/07  13:36:02  configtw
 * Initial revision
 * 
*/

clear ( addr, numbytes )

	register char *addr;
	register int numbytes;
{
	bzero ( addr, numbytes );
#if 0
	while ( numbytes-- )
	{
		*addr++ = 0;
	}
#endif
}
