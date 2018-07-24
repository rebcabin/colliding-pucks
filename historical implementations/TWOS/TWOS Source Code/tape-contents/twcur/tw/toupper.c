/*      Copyright (C) 1989, 1991, California Institute of Technology.
		U. S. Government Sponsorship under NASA Contract NAS7-918
		is acknowledged.        */

/*
 * $Log:	toupper.c,v $
 * Revision 1.3  91/07/17  15:13:38  judy
 * New copyright notice.
 * 
 * Revision 1.2  91/06/03  12:27:16  configtw
 * Tab conversion.
 * 
 * Revision 1.1  90/08/07  15:41:29  configtw
 * Initial revision
 * 
*/
char toupper_id [] = "@(#)toupper.c     1.5\t6/2/89\t12:46:26\tTIMEWARP";


/*

Purpose:

		toupper.c converts lower case alphabetic characters to the
		corresponding upper case characters.

Functions:

		toupper(c) - convert a character from lower to upper case
						Parameters - char c
						Return - the upper case character corresponding
								to c, if c was a lower case character; 
								the unaltered character, otherwise

Implementation:

		Test that c is one of the lower case characters.  If it is,
		convert it to upper case by subtracting off the ' ' character.
		Return either the upper case character, or the unaltered character,
		if toupper() was called with an improper argument.

*/


char toupper ( c )

	char c;
{
	if ( c >= 'a' && c <= 'z' )
		c -= ' ';

	return c;
}

char tolower ( c )

	char c;
{
	if ( c >= 'A' && c <= 'Z' )
		c += ' ';

	return c;
}
