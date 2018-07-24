/*      Copyright (C) 1989, 1991, California Institute of Technology.
		U. S. Government Sponsorship under NASA Contract NAS7-918
		is acknowledged.        */

/*
 * $Log:	twfabs.c,v $
 * Revision 1.2  91/07/17  16:15:56  judy
 * New copyright notice.
 * 
 * Revision 1.1  90/08/07  13:36:09  configtw
 * Initial revision
 * 
*/

double fabs ( x )

double x;

{
   if ( x < 0.0 )
	x = ( 0.0 - x );
   return x;
}

