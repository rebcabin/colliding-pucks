/*
 *
 *	PUCKS -- version 2.0 -- 10 Aug 1989
 *
 *		Philip Hontales
 *		Steven Bellenot
 *		Brian Beckman
 *		Kathy Sturdevant
 *
 *
 *	Pucks now uses all fields of extended virtual time
 *	and (attempts) handles multiple collusions correctly.
 *	All macro's have been removed and identifiers have been
 *	simplified.
 */

#include "twcommon.h"
#include "pucktypes.h"

typedef struct StateStruct
{

    PlanType plans[MAX_PLANS];
    Point p, p1;
    double elasticity;
    int action_id;
    LineSegment line;
    Point worldmin, worldmax;
    int Initialized;
    Name my_name;
    VTime now1;
    int my_number;

    /* Termination Statistics	 */

    int Total_Collisions_Scheduled;
    int Total_Collisions_Cancelled;

} State;


int cushion_init ();
int cushion_event ();
int cushion_term ();

ObjectType cushionType =
{
    "cushion",
    cushion_init,
    cushion_event,
    0,
    0,
    0,
    sizeof (State),
    0,
    0
};

cushion_init ()
{
    register int i;

    State *ps = (State *) myState;

    ps->elasticity = 1;
    ps->action_id = 0;

    for (i = 0; i < MAX_PLANS; i++)
	ps->plans[i].available = TRUE;

    myName ( ps->my_name );

    ps->my_number =
	( ps->my_name[8] - '0' ) * 1000 +
	( ps->my_name[10] - '0' ) * 100 +
	( ps->my_name[11] - '0' ) * 10 + 10;

    ps->Initialized = FALSE;
}

cushion_event ()
{
    register int msg_i;
    int totMsgs;

    State *ps = (State *) myState;

    totMsgs = numMsgs;
    ps->now1 = now;

    for ( msg_i = 0; msg_i < totMsgs; msg_i++ )
    {

	switch (msgSelector (msg_i))
	{

	case CONFIG_MSG:
	case CUSHION_START:
	    cushionStart ( msg_i, ps );
	    break;

	case NEW_TRAJECTORY:
	    newPuckDirection ( msg_i, ps );
	    break;

	case CANCEL_ACTION:
	    userError ( "cushion with a cancel msg" );
	    break;

	case UPDATE_SELF:
#ifdef AUTO_UPDATE
	    display_cushion ( ps );
#endif AUTO_UPDATE
	    break;

	default:
	    break;

	}	/*** end switch on selector ***/
    }		/*** end main message loop ***/
}		/*** end event ***/

/************************************************************************/
/*  This is the cushion's initialization message which is sent by the	*/
/*  configuration file . With this information the cushion establishes	*/
/*  its state that defines it area of control.				*/
/*  									*/
/*  The field here are as follows:					*/
/*									*/
/*  Cushion start pos. x, Cushion start pos. y,  Cushion end pos. x,	*/
/*  Cushion end pos. y, Table min. x, Table min. y Table max. x, 	*/
/*  Table max. y.							*/
/************************************************************************/

cushionStart ( msg_i, ps )
int msg_i;
State *ps;
{
    int i;
    char *p;
    
    p = (char *) msgText ( msg_i );
    
    while ( *p < '0' || *p > '9' )
    	p++;

    i = sscanf ( p, "%lf %lf %lf %lf %lf %lf %lf %lf",
	 &ps->p.x, &ps->p.y, &ps->p1.x, &ps->p1.y,
	 &ps->worldmin.x, &ps->worldmin.y,
	 &ps->worldmax.x, &ps->worldmax.y );

    if ( i != 8 )
    	userError ( "cushion snafu in config data" );

    ps->Initialized = TRUE;

    enter_critical_section ();
    ps->line = constructLineSegment ( ps->p,
		    constructVectFromPts (ps->p, ps->p1) );
    leave_critical_section ();

#ifdef AUTO_UPDATE
    display_cushion ( ps );
#endif AUTO_UPDATE
}

newPuckDirection ( msg_i, ps )
int msg_i;
State *ps;
{
    infoMsg *mp;
    actionMsg cm;
    Collision Collide;

    mp = (infoMsg *) msgText ( msg_i );

    if ( !ps->Initialized )
	userError ( "uninitialized cushion with new traj msg" );

    /* Cancel any planned collisions with puck X */
    cancelPlansWith ( mp->puck_name, ps);

    clear (&cm, sizeof (cm));
    cm.Puck_State = mp->Puck_State;

    enter_critical_section ();
    Collide = circAfterLineSegColl_SE ( &cm.Puck_State, ps->line);
    leave_critical_section ();

    if (Collide.yes && checkbounds ( cm.Puck_State, ps )
    	&& !cushion_endpoint ( cm.Puck_State, ps ) 
	&& Collide.at > ps->now1.simtime * TIME_FUDGE )
    {
	if ( Collide.at > ps->now1.simtime )
	{
	    cm.action_time = newVTime (Collide.at, 0, ps->my_number);
	}
	else
	{
	    cm.action_time = newVTime ( ps->now1.simtime,
		ps->now1.sequence1 + 1, ps->my_number );
	    /* current time so nothing goes backward in time */
	    cm.Puck_State.t = cm.action_time.simtime;
	}

	if ( cm.action_time.simtime < CUT_OFF )
	{
	    strcpy (cm.with_whom, ps->my_name);
	    cm.action_id = ps->action_id++;
	    tell ( mp->puck_name, cm.action_time,
	        COLLIDE_CUSHION, sizeof ( cm ), &cm );

	    logCollision ( mp->puck_name, cm.action_time,
	        cm.action_id, ps );
	    ps->Total_Collisions_Scheduled++;
#ifdef DISPLAY
	    display_cushion ( ps );
#endif DISPLAY
	}
    }
}


static 
display_cushion ( ps )
State *ps;
{
#ifdef DISPLAY
    tw_printf ( "pad:vector %f %f %f %f\n",
	 ps->p.x, ps->p.y, ps->p1.x, ps->p1.y);
#endif DISPLAY
}

static 
logCollision ( with_whom, for_when, id_number, ps )
Name with_whom;
VTime for_when;
int id_number;
State * ps;
{
    register int i;

    for ( i = 0; ; i++ )
    {
	if ( i >= MAX_PLANS )
	{
	    userError ( "Too many plans" );
	}
	else if ( ps->plans[i].available == TRUE )
	{
	    ps->plans[i].available = FALSE;
	    strcpy ( ps->plans[i].with_whom, with_whom );
	    ps->plans[i].action_id = id_number;
	    ps->plans[i].action_time = for_when;
	    break;
	}
    }
}

static 
cancelPlansWith ( s, ps )
Name s;
State *ps;
{
    register int i;
    unActMsg Killit;


    for (i = 0; i < MAX_PLANS; i++)
    {
	if (ps->plans[i].available == FALSE &&
	    strcmp (ps->plans[i].with_whom, s) == 0)
	{
	    ps->plans[i].available = TRUE;
	    if ( gtVTime ( ps->plans[i].action_time, ps->now1 ) &&
		ps->plans[i].action_time.simtime < CUT_OFF )
	    {
		clear ( &Killit, sizeof ( Killit ) );
		myName ( Killit.with_whom );
		Killit.action_id = ps->plans[i].action_id;
		tell ( ps->plans[i].with_whom, ps->plans[i].action_time,
		    CANCEL_ACTION, sizeof ( Killit ), &Killit );
		ps->Total_Collisions_Cancelled++;
	    }
	}
    }
}


/****************************************************************/
/* This routine checks to see if the puck we are planning a	*/
/* collision with has leaked off of the table. The CHEAR_CONST	*/
/* is a necessary kludge because of floating point garbage in	*/
/* the tail of the C data types.				*/
/****************************************************************/

static 
checkbounds ( Puck_State, ps )
Circle Puck_State;
State *ps;
{

    if ((Puck_State.p.x < ((ps->worldmin.x + Puck_State.r)
			   - CHEAT_CONST))
	|| (Puck_State.p.x > ((ps->worldmax.x - Puck_State.r)
			      + CHEAT_CONST))
	|| (Puck_State.p.y < ((ps->worldmin.y + Puck_State.r)
			      - CHEAT_CONST))
	|| (Puck_State.p.y > ((ps->worldmax.y - Puck_State.r)
			      + CHEAT_CONST))
	)
    {
	return FALSE;
    }
    else
	return TRUE;

}

/****************************************************************/
/* This routine checks to see if the puck collision with the	*/
/* cushion that we are planning here is on the endpoint of the	*/
/* cushion. We return TRUE if we are at a cushion endpoint and	*/
/* FALSE if we are not. This is required so if a collision	*/
/* on a cushion boundary occurrs, we don't get multiple collide */
/* messages.							*/
/*								*/
/****************************************************************/

/*???PJH This is a real kludge right now but it works...	*/
/*	 Need to make a bit more elegant when there is time.	*/


int 
cushion_endpoint ( Puck_State, ps )
Circle Puck_State;
State *ps;
{
    if (ps->line.l.x == 0.0)
    {
	if (Puck_State.p.y >= (ps->line.e.y + ps->line.l.y))
	{
	    return TRUE;
	}
    }
    else if (ps->line.l.y == 0.0)
    {
	if (Puck_State.p.x >= (ps->line.e.x + ps->line.l.x))
	{
	    return TRUE;
	}
    }
    return FALSE;
}

cushion_term ()
{
    State *ps = (State *) myState;

    tw_printf ("<TERM_STATS> %s %d %d \n", ps->my_name,
	    ps->Total_Collisions_Scheduled, ps->Total_Collisions_Cancelled );
}
