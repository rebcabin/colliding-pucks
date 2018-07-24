/****************************************************************************
 ******************* P I N G . C ********************************************
 ****************************************************************************
 
 This is a ping game. FPW and LVW 2/86 and JJW 9/87 
 - with fileio in ?/88

 REWRITTEN(?) JJW 5/17/88 THIS IS PING AND PONG. - there is only 1 object
 type,  use config file to specify appropriate object 

 REWRITTEN PLR 4/4/89 to remove almost everything. */

#ifdef SUN3
#include <stdio.h>
#endif


#include "twcommon.h"

#define CUTOFF 2500.

typedef struct {
	  int simname;
        } State;


i_PING()
{
	State *ps;
	Name simnamest;
	
        ps = (State *) myState;
	myName(simnamest);

	if (strcmp(simnamest,"ping") == 0)
	    ps->simname = 1;    /* PING = 1 */
	else  
	{
	    ps->simname = 0;  /* PONG = 0 */
	}

}


e_PING()
{
    State *ps;

    ps = (State *) myState;

/* ping uses CUTOFF to stop, pong doesn't stop as long as it gets a message 
	from ping.  */

    if ( now.simtime < CUTOFF ){

     	if (ps->simname ) {

		tell("pong", IncSimTime ( 1.0 ) , 0, 0, NULL);
	}
     	if (!ps->simname) {

		tell("ping", IncSimTime ( 1.0 ) , 0, 0, NULL);
     	}
    }


}

ObjectType playerType = { "player", i_PING, e_PING,  0, 0, 0,  sizeof(State),0,0 };
