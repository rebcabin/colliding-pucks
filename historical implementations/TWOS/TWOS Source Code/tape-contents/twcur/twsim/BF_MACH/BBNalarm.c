/*      Copyright (C) 1989, 1991, California Institute of Technology.
		U. S. Government Sponsorship under NASA Contract NAS7-918
		is acknowledged.	*/

#include <stdio.h>

#define MTIMER_EVENT    7

/******************************************************************************/
/*									      */
/*			S I G N A L   S I G M A L A R M			      */

#define ONE_MSEC ((int)(1000. / 62.5))

static int (*malarm_routine) ();

static EH malarm_eh;

malarm ( msecs ) 

    int msecs;
{
    if ( malarm_eh == 0 )
	malarm_eh = Make_Event ( 0,0,0,0 );

    Set_Timer ( malarm_eh, rtc + ONE_MSEC * msecs, MTIMER_EVENT );
}

butterfly_msigalarm ( routine )

    int (*routine) ();
{
    malarm_routine = routine;
}

/******************************************************************************/
/*									      */
/*			C H E C K   F O R   E V E N T S			      */

check_for_events ()
{
    EH eh;
    int edata;

    eh = Receive_Event ();

    if ( eh )
    {
	edata = Event_Data ( eh );

	Reset_Event ( eh );

	switch ( edata & 0xffff )
	{
	    case MTIMER_EVENT:

		if ( malarm_routine )
		{
		    (*malarm_routine) ();
		}
		break;

	    default:

		printf ( "received unknown event %d\n", edata );
	}

	return ( edata );
    }

    return ( 0 );
}

/******************************************************************************/
