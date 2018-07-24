/*      Copyright (C) 1989, 1991, California Institute of Technology.
		U. S. Government Sponsorship under NASA Contract NAS7-918
		is acknowledged.        */

/*
 * $Log:	cmp.c,v $
 * Revision 1.4  91/12/27  09:01:15  pls
 * 1.  Add TIMING related code.
 * 2.  Change imcmp comparisons for better performance.
 * 
 * Revision 1.3  91/07/17  15:07:21  judy
 * New copyright notice.
 * 
 * Revision 1.2  91/06/03  12:23:43  configtw
 * Tab conversion.
 * 
 * Revision 1.1  90/08/07  15:38:00  configtw
 * Initial revision
 * 
*/
char cmp_id [] = "@(#)cmp.c     1.18\t6/2/89\t12:43:16\tTIMEWARP";


/*

Purpose:

		cmp.c contains several comparison routines.  Typically, these
		routines compare two items, returning one of three values -
		0 if they are the same, positive if the first is greater than
		the second, and negative if the second is greater than the
		first.

Functions:

		namecmp(name1,name2) - compare two names
				Parameters - char *name1, char *name2
				Returns - 0 if equal, positive value if name1 is greater, 
						negative otherwise

		uidcmp(a,b) - compare two uids
				Parameters - Uid a, Uid b
				Returns - 0 if equal, positive value if a is greater, 
						negative otherwise

		imcmp(a,b) - compare two input messages
				Parameters - Msgh *a, Msgh *b
				Returns - 0 if equal, positive value if a is greater, 
						negative otherwise

		omcmp(a,b) - compare two output messages
				Parameters - Msgh *a, Msgh *b
				Returns - 0 if equal, positive value if a is greater, 
						negative otherwise

		statecmp(a,b) - compare two state's sndtim's
				Parameters - State *a, State *b
				Returns - 0 if equal, positive value if a is greater, 
						negative otherwise

Implementation:

		Many of these routines feature very simple comparisons of a
		single pair of values.  The more substantial ones compare more
		values, but have little other additional complexity.  Typically,
		these routines end with a subtraction of one value from another,
		thereby giving the three-way results desired.  The magnitude 
		produced is unimportant - only the sign matters.

		namecmp() does a character by character comparison of the two
		names.  ocbcmp() subtracts one object's svt from the other's,
		returning the difference.  uidcmp() first checks to see if
		the Uids in question are on the same node.  If not, their 
		num fields are examined. 

		imcmp() compares two input messages by comparing several of their
		fields.  Once one comparison fails, no others are attempted.
		The values compared include rcvtim, mtype, sndtim, snder,
		txtlen, the messages' texts, and gid.

		omcmp() first checks both messages' sndtims.  If they are the same,
		uidcmp() compares their uids.

		statecmp() subtracts one sndtim from another, returning the result.
*/

#include "twcommon.h"
#include "twsys.h"

#if TIMING
#define IQCMP_TIMING_MODE 15
#endif

FUNCTION int namecmp ( name1, name2 )

	register char *name1, *name2;
{
	while ( *name1 && *name1 == *name2 )
	{
		name1++; name2++;
	}

	return ( *name1 - *name2 ); /* 0 if match */
}

FUNCTION int uidcmp ( a, b )

	Uid a, b;
{
	register int comp;

	if ( comp = ( a.node - b.node ) )

		return comp;

	return ( a.num - b.num );
}


FUNCTION int imcmp ( a, b )

	register Msgh *a, *b;
{
	register int comp;
	register int txtlen;

	if ( gtVTime ( a->rcvtim, b->rcvtim ) )

		return 1;       /* if a's time > b's time */

	if ( ltVTime ( a->rcvtim, b->rcvtim ) )

		return -1;      /* if a's time < b's time */

	if ( comp = ( a->mtype - b->mtype ) )

		return comp;    /* if time's are same, go by msg type */

	if ( comp = ( a->selector - b->selector ) )

		return comp;    /* otherwise use selector */

	if ( a->txtlen <= b->txtlen )
		txtlen = a->txtlen;
	else
		txtlen = b->txtlen;

#if 0
	start_timing(IQCMP_TIMING_MODE);
	comp = bytcmp ( (Byte *)(a+1), (Byte *)(b+1), txtlen );
	stop_timing();
	if (comp)
#else
	if ( comp = bytcmp ( (Byte *)(a+1), (Byte *)(b+1), txtlen ) )
#endif

		return comp ;   /* else use message content */

	if ( comp = ( a->txtlen - b->txtlen ) )

		return comp;    /* else use message length */

	comp = uidcmp ( a->gid, b->gid ) ;

	return comp ;       /* otherwise node or Uid number */
}


/****************************************************************************
********************************   OMCMP   **********************************
*****************************************************************************

	Function comparing two output msgs, a and b, in the sense of a - b
	for priority ordering in the output queue and annihilation.  Will return
	0 iff messages have same gid's.
	*/

FUNCTION int omcmp ( a, b )

	register Msgh *a, *b;
{
	register int comp;

	if ( ltVTime ( a->sndtim, b->sndtim ) )

		return -1;      /* if a's send time < b's */

	if ( gtVTime ( a->sndtim, b->sndtim ) )

		return 1;       /* if a's send time > b's */

	comp = uidcmp ( a->gid, b->gid );

	return comp;        /* else use node & UID info */
}

FUNCTION int statecmp ( a, b )

	register State *a, *b;
{
	if ( gtVTime ( a->sndtim, b->sndtim ) )

		return 1;

	if ( ltVTime ( a->sndtim, b->sndtim ) )

		return -1;

	return  ( 0 );
}
