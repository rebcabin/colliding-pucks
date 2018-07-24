/*	Copyright (C) 1989, 1991, California Institute of Technology.
		U. S. Government Sponsorship under NASA Contract NAS7-918
		is acknowledged.	*/

clear ( addr, numbytes )

    register char *addr;
    register int numbytes;
{
    while ( numbytes-- )
    {
	*addr++ = 0;
    }
}
