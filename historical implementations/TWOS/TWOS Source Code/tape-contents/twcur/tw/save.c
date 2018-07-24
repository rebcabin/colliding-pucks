/*      Copyright (C) 1989, 1991, California Institute of Technology.
		U. S. Government Sponsorship under NASA Contract NAS7-918
		is acknowledged.        */

/*
 * $Log:	save.c,v $
 * Revision 1.4  91/11/01  10:06:24  pls
 * 1.  Change ifdef's, version id.
 * 2.  Set up check_stack_and_state() for use (SCR 200).
 * 
 * Revision 1.3  91/07/17  15:12:02  judy
 * New copyright notice.
 * 
 * Revision 1.2  91/06/03  12:26:33  configtw
 * Tab conversion.
 * 
 * Revision 1.1  90/08/07  15:40:52  configtw
 * Initial revision
 * 
*/
char save_id [] = "@(#)save.c   $Revision: 1.4 $\t$Date: 91/11/01 10:06:24 $\tTIMEWARP";


#include "twcommon.h"
#include "twsys.h"

FUNCTION save_state ( ocb )

	Ocb * ocb;
{
  Debug

#if PARANOID
	check_stack_and_state ( ocb );
#endif

	if ( ocb->stk )
	{
		l_destroy ( ocb->stk );
		ocb->stk = NULL;
	}

	l_insert ( ocb->cs, ocb->sb );

	ocb->cs = ocb->sb;

	ocb->sb = NULL;

	ocb->stats.nssave++;
}

FUNCTION void check_stack_and_state ( ocb )

	Ocb * ocb;
{
	if ( strcmp ( ocb->stk, "stack limit" ) != 0 )
	{
		_pprintf ( "%s stack overflow at time %f\n", ocb->name, ocb->svt.simtime );
		tester ();
	}

	if ( strcmp ( ((Byte *)(ocb->sb+1)) + ocb->pvz_len, "state limit" ) )
	{
		_pprintf ( "%s state overflow at time %f\n", ocb->name, ocb->svt.simtime );
		tester ();
	}
}
