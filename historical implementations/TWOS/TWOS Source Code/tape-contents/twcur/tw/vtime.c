/*      Copyright (C) 1989, 1991, California Institute of Technology.
		U. S. Government Sponsorship under NASA Contract NAS7-918
		is acknowledged.        */

/*
 * $Log:	vtime.c,v $
 * Revision 1.3  91/07/17  15:14:09  judy
 * New copyright notice.
 * 
 * Revision 1.2  91/06/03  12:27:32  configtw
 * Tab conversion.
 * 
 * Revision 1.1  90/08/07  15:41:43  configtw
 * Initial revision
 * 
*/
char vtime_id [] = "@(#)vtime.c 1.5\t9/27/89\t15:54:23\tTIMEWARP";


#include "twcommon.h"

VTime newVTime ( aSimTime, aSequence1, aSequence2 )

	STime aSimTime;
	Ulong aSequence1;
	Ulong aSequence2;
{
	VTime aVTime;

	aVTime.simtime = aSimTime;
	aVTime.sequence1 = aSequence1;
	aVTime.sequence2 = aSequence2;

	return ( aVTime );
}


VTime SetSimTime ( aVTime, aSimTime )

	VTime aVTime;
	STime aSimTime;
{
	aVTime.simtime = aSimTime;
	return ( aVTime );
}

VTime SetSequence1 ( aVTime, aSequence1 )

	VTime aVTime;
	Ulong aSequence1;
{
	aVTime.sequence1 = aSequence1;
	return ( aVTime );
}

VTime SetSequence2 ( aVTime, aSequence2 )

	VTime aVTime;
	Ulong aSequence2;
{
	aVTime.sequence2 = aSequence2;
	return ( aVTime );
}

VTime sscanVTime ( aString )

	char * aString;
{
	VTime aVTime;

	sscanf ( aString, "%lf, %ld, %ld",
		&aVTime.simtime,
		&aVTime.sequence1,
		&aVTime.sequence2 );

	return ( aVTime );
}

sprintVTime ( aString, aVTime )

	char * aString;
	VTime aVTime;
{
	sprintf ( aString, "%f %d %d",
		aVTime.simtime,
		aVTime.sequence1,
		aVTime.sequence2 );
}

#ifndef FAST_VTIME_MACROS


gtSTime ( a, b )

STime a,b;
{
   if ( a > b ) 
		return TRUE; 
   else
		return FALSE;

}

ltSTime ( a, b )

STime a,b;
{
   if ( a < b ) 
		return TRUE;
   else
		return FALSE;
}

geSTime ( a, b )

STime a,b;
{
   if ( a >= b ) 
		return TRUE; 
   else 
		return FALSE;

}

leSTime ( a, b )

STime a,b;
{
   if ( a <= b ) 
		return TRUE; 
   else
		return FALSE ;

}

eqSTime ( a, b )

STime a,b;
{
   if ( a == b ) 
		return TRUE; 
   else
		return FALSE;

}

neSTime ( a, b )

STime a,b;
{
   if ( a != b ) 
		return TRUE; 
   else
		return FALSE;

}

#endif

