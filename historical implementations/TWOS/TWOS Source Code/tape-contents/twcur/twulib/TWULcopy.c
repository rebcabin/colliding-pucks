/*  	Copyright (C) 1989, 1991, California Institute of Technology.
		U.S. Government Sponsorship under NASA Contract NAS7-918
		is acknowledged.        */

/*
 * $Log:$
 *
*/

TWULfastcpy ( dest, src, numbytes )

    char *dest, *src;
    int numbytes;
{
    while ( numbytes-- )
    {
   *dest++ = *src++;
    }
}
