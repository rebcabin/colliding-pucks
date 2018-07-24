/*      Copyright (C) 1989, 1991, California Institute of Technology.
		U. S. Government Sponsorship under NASA Contract NAS7-918
		is acknowledged.        */

/*
 * $Log:	oldtw.c,v $
 * Revision 1.3  91/07/17  15:11:43  judy
 * New copyright notice.
 * 
 * Revision 1.2  91/06/03  12:26:06  configtw
 * Tab conversion.
 * 
 * Revision 1.1  90/08/07  15:40:46  configtw
 * Initial revision
 * 
*/

#include "twcommon.h"
#include "twsys.h"

FUNCTION tellI ( rcver, ircvtim, selector, txtlen, message )

	Name * rcver;
	int ircvtim;
	Long selector;
	int txtlen;
	char * message;
{
	VTime rcvtim;

	rcvtim = newVTime ( (STime) ircvtim, 0, 0 );

	tell ( rcver, rcvtim, selector, txtlen, message );
}

FUNCTION newObjI ( rcver, ircvtim, objtype )

	Name *rcver;
	int ircvtim;
	Name *objtype;
{
	VTime rcvtim;

	rcvtim = newVTime ( (STime) ircvtim, 0, 0 );

	newObj ( rcver, rcvtim, objtype );
}

FUNCTION delObjI ( rcver, ircvtim )

	Name *rcver;
	int ircvtim;
{
	VTime rcvtim;

	rcvtim = newVTime ( (STime) ircvtim, 0, 0 );

	delObj ( rcver, rcvtim );
}

int obj_nowI ()
{
	return ( (int) xqting_ocb->svt.simtime );
}

int msgSendTimeI ( n )

	int n;
{
	return ( (int) GetSimTime ( msgSendTime ( n ) ) );
}
