/*
 *
 *	PUCKS -- version 2.0 -- 10 Aug 1989
 *
 *		Philip Hontalas
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
    List sectors[MAX_SECTORS];
    Circle Last_Puck_State, Puck_State;
    Point worldmin, worldmax;	/* min &  max table coordinates */
    Name my_name;
    VTime now1;
    int my_number;
    int action_id;
    int sector_width, sector_height;
    int Initialized;

#ifdef THERMAL_LOG
    VTime Last_Log_Time;
    int Thermal_Record_Count;
    Thermal_Record Thermal_Log[MAX_THERMAL_RECORDS];
#endif  THERMAL_LOG

    /* Termination Statistics	 */

    int Total_Puck_Collisions;
    int Total_Puck_Collisions_Cancelled;
    int Total_Cushion_Collisions;
    int Total_Cushion_Collisions_Cancelled;
    int Total_Enter_Sectors;
    int Total_Enter_Sectors_Cancelled;
    int Total_Depart_Sectors;
    int Total_Depart_Sectors_Cancelled;

} State;

int puck_init ();
int puck_event ();
int puck_term ();

ObjectType puckType =
{
    "puck",
    puck_init,
    puck_event,
    0,
    0,
    0,
    sizeof (State),
    0,
    0
};

puck_init ()
{
    register int i;

    State *ps = (State *) myState;

    ps->action_id = 0;

    for (i = 0; i < MAX_PLANS; i++)
	ps->plans[i].available = TRUE;

    for (i = 0; i < MAX_SECTORS; i++)
	ps->sectors[i].available = TRUE;

    ps->Initialized = FALSE;
    
    myName ( ps->my_name );

    ps->my_number =
	( ps->my_name[4] - '0' ) * 1000 +
	( ps->my_name[5] - '0' ) * 100 +
	( ps->my_name[6] - '0' ) * 10 +
	( ps->my_name[7] - '0' ) * 1;

#ifdef THERMAL_LOG
    ps->Last_Log_Time = newVTime ( -30.0, 0, 0);	/* PJH  Arbitrary */
    ps->Thermal_Record_Count = 0;
#endif THERMAL_LOG

}

puck_event ()
{
    State *ps = (State *) myState;
    int totMsgs;
    register int i, msg_i;
    char cannedMsgs[MAX_NUMBER_OF_MESSAGES];




    int tst_i;

    totMsgs = numMsgs;
    ps->now1 = now;
    
    canTheMsgs ( cannedMsgs, ps, totMsgs );
/*
 *	Now we process all of the un-cancelled puck messages
 */

			 
    for (msg_i = 0; msg_i < totMsgs; msg_i++)
    {

	if ( cannedMsgs[msg_i] )
	    continue;

	switch (msgSelector (msg_i))
	{
	case CONFIG_MSG:
	case PUCK_START:
	    puckStart ( msg_i, ps );
	    break;

	case NEW_TRAJECTORY:
	    newTrajectory ( msg_i, ps );
	    break;

	case COLLIDE_CUSHION:
	    collideCush ( msg_i, ps );
	    ps->Total_Cushion_Collisions++;
	    break;

	case COLLIDE_PUCK:
	    collidePuck ( msg_i, ps );
	    ps->Total_Puck_Collisions++;
	    break;

	case ENTER_SECTOR:
	    enterSector ( msg_i, ps );
	    ps->Total_Enter_Sectors++;
	    break;

	case DEPART_SECTOR:
	    departSector ( msg_i, ps );
	    ps->Total_Depart_Sectors++;
	    break;

	case UPDATE_SELF:
#ifdef AUTO_UPDATE
	    do_it_update ();
#endif AUTO_UPDATE
	    break;

	default:
/*???PJH
	    tw_printf ("%s Received an UNKNOWN msg type\n", ps->my_name );
	    Dump_Pool_Msg ( msg_i );
*/
	    break;

	}
    }
}
/*** end event ***/

/************************************************************************/
/*	The first thing we do during each event is read all of the	*/
/*	messages for a given Virtual Time and process the CANCEL-	*/
/*	-_ACTION messages first.					*/
/************************************************************************/

static
canTheMsgs ( cannedMsgs, ps, totMsgs )
char * cannedMsgs;
State *ps;
int totMsgs;
{
    register int i, j;
    int select;

    unActMsg *mp;
    actionMsg *mp2;
  
    if ( totMsgs > MAX_NUMBER_OF_MESSAGES )
    {
	userError ( "exceeded MAX_NUMBER_OF_MESSAGES" );
    }

    for (i = 0; i < totMsgs; i++)
    {
	cannedMsgs[i] = FALSE;
    }

    for (i = 0; i < totMsgs; i++)
    {
	if (  cannedMsgs[i] || ( (msgSelector (i) != CANCEL_ACTION) ) )
	{
	    continue;
	}

	mp = (unActMsg *) msgText (i);
	cannedMsgs[i] = TRUE;

	for (j = 0; j < totMsgs; j++)
	{
	    if ( cannedMsgs[j] )
	        continue;

	    switch ( select = msgSelector ( j ) )
	    {
	    case NEW_TRAJECTORY:
	    case CANCEL_ACTION:
	    case UPDATE_SELF:
	    case PUCK_START:
	    case CONFIG_MSG:
		/* above are info msgs -- can't be cancelled */
		continue;
	    default:
		break;
	    }

	    mp2 = (actionMsg *) msgText (j);

	    if ( ( mp->action_id != mp2->action_id )
		|| ( strcmp ( mp->with_whom, mp2->with_whom ) != 0 ) )
	    {
		continue;
	    }

	    switch ( select )
	    {
	    case COLLIDE_PUCK:
		ps->Total_Puck_Collisions_Cancelled++;
		break;
	    case COLLIDE_CUSHION:
		ps->Total_Cushion_Collisions_Cancelled++;
		break;
	    case ENTER_SECTOR:
		ps->Total_Enter_Sectors_Cancelled++;
		break;
	    case DEPART_SECTOR:
		ps->Total_Depart_Sectors_Cancelled++;
		break;
	    default:
		userError ( "unknown msg type to cancel" );
		break;
	    }

	    cannedMsgs[j] = TRUE;
	    break;
	}
    }
}


/************************************************************************/
/*	This behavior establishes the puck's initial state in the	*/
/*	simulation. The message is sent by the initial event message	*/
/*	in the configuration file.					*/
/*									*/
/*  The Initialization fields are defined as follows:			*/
/*									*/
/*  Puck radius,  Puck mass,  Puck pos. x, Puck pos. y, Puck vel.x,	*/
/*  Puck vel. y,  Sector puck is in, Minimum x of table, Minimum y	*/
/*  of table, Maximum x of table, Maximum y of table.			*/
/************************************************************************/
 puckStart ( msg_i, ps )
 int msg_i;
 State *ps;
 {
    infoMsg pv;
    register int i;
    char *p;
    Name initial_sector;

    p = (char *) msgText ( msg_i );
    
    while ( *p < '0' || '9' < *p )
    	p++;

    i = sscanf ( p, "%lf %lf %lf %lf %lf %lf %s %d %d %lf %lf %lf %lf",
	    &ps->Puck_State.r,
	    &ps->Puck_State.m,
	    &ps->Puck_State.p.x,
	    &ps->Puck_State.p.y,
	    &ps->Puck_State.v.x,
	    &ps->Puck_State.v.y,
	    initial_sector,
	    &ps->sector_width,
	    &ps->sector_height,
	    &ps->worldmin.x,
	    &ps->worldmin.y,
	    &ps->worldmax.x,
	    &ps->worldmax.y
	);

    if ( i != 13 )
    	userError ( "config msg snafu in puck" );

/************************************************************************/
/*   First  we need to check the  start  position to make sure that it	*/
/*   is within the bounds of the table.					*/
/************************************************************************/
    errorCheck ( ps->Puck_State.p, "puck start" );

    ps->Initialized = TRUE;
    ps->Last_Puck_State.p = ps->Puck_State.p;
    ps->Last_Puck_State.v = ps->Puck_State.v;
    ps->Puck_State.t = ps->now1.simtime;

    listSector ( initial_sector );

    clear (&pv, sizeof (pv));
    strcpy (pv.puck_name, ps->my_name);
    pv.Puck_State = ps->Puck_State;
    tell ( initial_sector, IncSequence2 ( 1 ),
    	INITIAL_VELOCITY, sizeof ( pv ), &pv );

#ifdef  DISPLAY
    display_puck ();
#endif	DISPLAY

#ifdef AUTO_UPDATE
    do_it_update ();
#endif AUTO_UPDATE
}

VTime nextCollTime ( collide_time, puck_name, ps )
double collide_time;
Name puck_name;
State *ps;
{
    VTime answer;
    int collide_number;

    collide_number =
	( puck_name[4] - '0' ) * 1000 +
	( puck_name[5] - '0' ) * 100 +
	( puck_name[6] - '0' ) * 10 +
	( puck_name[7] - '0' ) * 1;


    collide_number = ps->my_number * 100000 + collide_number * 10 + 10000;

    if ( collide_time > ps->now1.simtime )
    {
	answer = newVTime ( collide_time, 0, collide_number );
    }
    else
    {
	answer = newVTime ( ps->now1.simtime,
	    ps->now1.sequence1 + 1, collide_number );
    }

    return answer;
}

/************************************************************************/
/*	Puck objects receive NEW_TRAJECTORY messages from the		*/
/*	sectors they reside in. Puck objects also send NEW-		*/
/*	-_TRAJECTORY to the sectors they reside in.			*/
/*	When a puck object gets one of these messages it indicates that	*/
/*	a puck is taking a new trajectory. The receiving puck needs to	*/
/*	check if this new puck course will effect its own course through*/
/*	a collision.							*/
/************************************************************************/
newTrajectory ( msg_i, ps )
int msg_i;
State *ps;
{
    actionMsg cMe, cIt;
    infoMsg pv, *mp;
    Collision Collide;
    Circle Temp_Puck_State;
    register int i;

    mp = (infoMsg *) msgText ( msg_i );

    if ( !ps->Initialized )
    {
	userError ( "uninitialized puck with new traj msg" );
    }
/************************************************************************/
/*	First, the puck needs to update its position for the time of	*/
/*	this action.						*/
/*									*/
/************************************************************************/
    enter_critical_section ();
    Temp_Puck_State.p = whereCircCtr ( &ps->Puck_State, mp->Puck_State.t );
    leave_critical_section ();


/************************************************************************/
/*   Now we need to check that this new position is within the bounds	*/
/*   of the table.							*/
/************************************************************************/

    if ( checkbounds ( Temp_Puck_State.p) == FALSE )
    {
/*PJH Test..
 	userError ( "new traj out of bounds" );
	printf ( "new traj out of bounds %s at %f\n",
		  ps->my_name, mp->Puck_State.t );
*/

 
        return;

/*???PJH Need to explore why we get these occassionally.
 *	userError ( "new traj out of bounds" );
 *
 *	reverse this puck ( back to the board? )
 *
 * START FOOLING with PHYSICS
	ps->Puck_State.t = mp->Puck_State.t;

	enter_critical_section ();
	ps->Puck_State.v = vectorReverse (ps->Puck_State.v);
	leave_critical_section ();

	clear (&pv, sizeof (pv));
	strcpy (pv.puck_name, ps->my_name);
	pv.Puck_State = ps->Puck_State;

	for (i = 0; i < MAX_SECTORS; i++)
	{
	    if (ps->sectors[i].available == FALSE)
	    {
	    	tell ( ps->sectors[i].element, IncSequence2 ( 1 ),
		    CHANGE_VELOCITY, sizeof ( pv ), &pv );
	    }
	}

	cancel_all_plans ();
	return;		*/

    }
/*
 * END FOOLING WITH PHYSICS
 */

    ps->Puck_State.p = Temp_Puck_State.p;
    ps->Puck_State.t = mp->Puck_State.t;

/************************************************************************/
/*   If this puck has any previously scheduled plans with the puck	*/
/*   involved in in this message they will be cancelled since that	*/
/*   puck now has a new trajectory.					*/
/************************************************************************/
    cancelPlans ( mp->puck_name );

/************************************************************************/
/*   Now the puck should check if a collision between itself and	*/
/*   the involved puck may occur.				 	*/
/************************************************************************/
    clear ( &cMe, sizeof ( cMe ) );
    cMe.Puck_State = ps->Puck_State;

    enter_critical_section ();
    Collide = circlesAfterCircColl_SE ( &cMe.Puck_State, &mp->Puck_State );
    leave_critical_section ();


    if ( Collide.yes && ( Puck_In_Sector_List ( cMe.Puck_State ) == TRUE )
	&& ( checkbounds ( cMe.Puck_State.p ) == TRUE)
	&& ( Collide.at > ps->now1.simtime * TIME_FUDGE ) )
    {

	cMe.action_time = nextCollTime (Collide.at, mp->puck_name, ps );

	/* for events at near now */
	Collide.at = cMe.action_time.simtime;
	mp->Puck_State.t = Collide.at;

	if ( cMe.action_time.simtime < CUT_OFF  )
	{

/************************************************************************/
/*   If there will be a collision, the receiving puck first sends a	*/
/*   message to itself for the scheduled collision time. This 		*/
/*   collision message contains the position, velocity and absolute	*/
/*   time that the puck will have immediately after the collision.	*/
/************************************************************************/
	    clear ( &cIt, sizeof ( cIt ) );
	    strcpy ( cMe.with_whom, mp->puck_name );
	    strcpy ( cIt.with_whom, ps->my_name );
	    cIt.action_id = cMe.action_id = ps->action_id++;
	    cIt.Puck_State = mp->Puck_State;
	    cMe.Puck_State.t = Collide.at;
	    cIt.action_time = cMe.action_time;
	    
	    tell ( ps->my_name, cMe.action_time,
	    	COLLIDE_PUCK, sizeof ( cMe ), &cMe );
	    tell ( mp->puck_name, cIt.action_time,
	    	COLLIDE_PUCK, sizeof ( cIt ), &cIt );

	    addPlan ( mp->puck_name, cIt.action_time,
	    	cIt.action_id );
	}
    }
}


/************************************************************************/
/*   The table's cushion objects (cushion.c) send these COLLIDE-	*/
/*   CUSHION messages to pucks when they determine from NEW-		*/
/*   TRAJECTORY sent by pucks that a collision is to occur. This	*/
/*   message gives the puck's new position, velocity, and absolute time */
/*   immediately after the puck's collision with the cushion.		*/
/************************************************************************/

collideCush ( msg_i, ps )
int msg_i;
State *ps;
{
    infoMsg pv;
    actionMsg *mp, *mp2;
    register int i, j;
    Collision Collide;
 
    mp = ( actionMsg * ) msgText ( msg_i );

    if (!ps->Initialized)
	userError ( "unintialized puck with collide cushion msg" );

    errorCheck ( mp->Puck_State.p, "collide cush out of bounds" );

/************************************************************************/
/*   Send CHANGE_VELOCITY messages to all of the sectors with this puck	*/
/*   is currently in for a puck position update.			*/
/************************************************************************/

    clear (&pv, sizeof (pv));
    strcpy (pv.puck_name, ps->my_name);
    pv.Puck_State = ps->Puck_State = mp->Puck_State;

    for (i = 0; i < MAX_SECTORS; i++)
    {
	if (ps->sectors[i].available == FALSE)
	{
	    tell ( ps->sectors[i].element, IncSequence2 ( 1 ),
	    	CHANGE_VELOCITY, sizeof ( pv ), &pv );
	}
    }
    cancel_all_plans ();

#ifdef DISPLAY
    erase_puck ();
    display_puck ();
    display_collision_msg ( mp->with_whom);
#endif DISPLAY
}


/************************************************************************/
/*	This informs a puck that another puck in the same sector it is	*/
/*	in has calculated a collision. The behavior is the composite to	*/
/*	the collision section of the NEW_TRAJECTORY behavior    */
/* 	which we saw above.						*/
/*	This message will give the receiving puck its new position,	*/
/*	velocity and absolute time immediately after the collision.	*/
/*									*/
/************************************************************************/

collidePuck ( msg_i, ps )
int msg_i;
State *ps;
{
    int i, j;
    infoMsg pv;
    actionMsg *mp, *mp2;
    Collision Collide;

    if ( !ps->Initialized )
	userError ( "unintialized puck with collide puck msg" );

    mp = (actionMsg *) msgText ( msg_i );
/************************************************************************/
/*   Now we need to check that this new position is within the bounds	*/
/*   of the table AND that the puck we are colliding with is within	*/
/*   at least one of the sectors we are in.				*/
/************************************************************************/
    if ((checkbounds ( mp->Puck_State.p) == FALSE) ||
	(Puck_In_Sector_List ( mp->Puck_State) == FALSE)
	)
    {
	/* this happens */
	return;
	userError ( "snafu in collide puck" );
    }

/************************************************************************/
/*   Send CHANGE_VELOCITY messages to all of the sectors with this puck	*/
/*   is currently in for a puck position update.			*/
/************************************************************************/

    clear (&pv, sizeof (pv));
    strcpy (pv.puck_name, ps->my_name);
    ps->Puck_State = pv.Puck_State = mp->Puck_State;

    for (i = 0; i < MAX_SECTORS; i++)
    {
	if (ps->sectors[i].available == FALSE)
	{
	    tell ( ps->sectors[i].element, IncSequence2 ( 1 ),
	    	CHANGE_VELOCITY, sizeof ( pv ), &pv );
	}
    }

    cancel_all_plans ();

#ifdef DISPLAY
    erase_puck ();
    display_puck ();
    display_collision_msg ( mp->with_whom);
#endif DISPLAY
}

/************************************************************************/
/*	This message informs a puck it is now entering a new sector.  	*/
/*	This message type is sent by the  table's sector objects	*/
/*      ( sector_beh ) which calculate puck entry and departure times	*/
/*	to and from sectors. 						*/
/*      All the puck object need do in this behavior is add		*/
/*	the new sector name to its list of sectors which it is in.	*/
/************************************************************************/

enterSector ( msg_i, ps )
int msg_i;
State *ps;
{
    actionMsg *mp;
    int i;
    
    mp = (actionMsg *) msgText ( msg_i );

    if (!ps->Initialized)
	userError ( "uninitialized puck with enter sector msg" );

    errorCheck ( mp->Puck_State.p, "enter sector out of bounds" );

/************************************************************************/
/*   		Update to our new ENTER_SECTOR  position		*/
/************************************************************************/
    ps->Puck_State = mp->Puck_State;

    /* add sector to the set of occupied sectors */
    listSector ( mp->with_whom );


#ifdef DISPLAY
    erase_puck ();
    display_puck ();
#endif DISPLAY
}


/************************************************************************/
/*	This message informs a puck it is now departing a sector.  	*/
/*	This message type is sent by the  table's sector objects	*/
/*      ( sector_beh ) which calculates puck entry and departure times	*/
/*	to and from sectors. 						*/
/*      All the puck object need do in this behavior is delete		*/
/*	the sector name from its list of sectors which it is in.	*/
/************************************************************************/
departSector ( msg_i, ps )
int msg_i;
State *ps;
{
    actionMsg *mp = (actionMsg *) msgText ( msg_i );

    if (!ps->Initialized)
	userError ( "unintialized puck with depart sector msg" );

    errorCheck ( mp->Puck_State.p , "depart sector" );

    ps->Puck_State = mp->Puck_State;

    unlistSector ( mp->with_whom);

#ifdef DISPLAY
    erase_puck ();
    display_puck ();
#endif
}


static 
display_collision_msg ( whom)

Name whom;

{
    State *ps = (State *) myState;

#ifndef IRIS

    tw_printf ( "pad:printstring %f %f  %s collides w %s at %lf \n",
	     ((ps->worldmax.x / 2) - 90.0), (ps->worldmax.y + 12),
	     ps->my_name, whom, ps->now1.simtime
	);

#else

    tw_printf ( "COLLISION: %s %f %f %f %f %f\n",
	     ps->my_name, ps->Puck_State.t,
	     ps->Puck_State.p.x, ps->Puck_State.p.y,
	     ps->Puck_State.v.x, ps->Puck_State.v.y
	);

#endif

}

/*???PJH Display and erase functions may also be replaced by circles func.. */

static 
erase_puck ()
{
    State *ps = (State *) myState;
    
#ifndef IRIS
    Point q;

#ifdef DISPLAY
    tw_printf
	(
	 "pad:erasecircle16 %f %f %f\n",
	 ps->Last_Puck_State.p.x,
	 ps->Last_Puck_State.p.y,
	 ps->Puck_State.r
	);
/*
    tw_printf ( "pad:erasestring %f %f %s\n",
	    ps->Last_Puck_State.p.x,
	    ps->Last_Puck_State.p.y,
	    &ps->my_name[4]
 	);
*/
    q = otherEndPoint
	(
	 ps->Last_Puck_State.p,
	 ps->Last_Puck_State.v
	);

    tw_printf ( "pad:un_arrow %f %f %f %f\n",
	 ps->Last_Puck_State.p.x,
	 ps->Last_Puck_State.p.y,
	 q.x,
	 q.y
	);
#endif DISPLAY
    ps->Last_Puck_State.p = ps->Puck_State.p;
    ps->Last_Puck_State.v = ps->Puck_State.v;
#endif
}

static 
display_puck ()

{
#ifndef IRIS
    register int i;
    Point q;

#ifdef DISPLAY
    State *ps = (State *) myState;

    tw_printf ( "pad:circle16 %f %f %f\n",
	 ps->Puck_State.p.x, ps->Puck_State.p.y, ps->Puck_State.r);

    q = otherEndPoint
	(
	 ps->Puck_State.p,
	 ps->Puck_State.v
	);
    tw_printf ( "pad:arrow %f %f %f %f\n",
	 ps->Puck_State.p.x, ps->Puck_State.p.y, q.x, q.y);
/*
    tw_printf ( "pad:printstring %f %f %s\n",
	   ps->Puck_State.p.x, ps->Puck_State.p.y, &my_name[4]);
*/

#endif DISPLAY

#endif

}

static 
addPlan ( with_whom, for_when, id_number )
Name with_whom;
VTime for_when;
int id_number;
{
    State *ps = (State *) myState;
    register int i;

    for ( i = 0; ; i++ )
    {
	if (i == MAX_PLANS)
	{
	    userError ( "Too many plans" );
	}
	else if (ps->plans[i].available == TRUE)
	{
	    ps->plans[i].available = FALSE;
	    strcpy (ps->plans[i].with_whom, with_whom );
	    ps->plans[i].action_id = id_number;
	    ps->plans[i].action_time = for_when;
	    print_plan ( &ps->plans[i]);
	    return;
	}
    }
}

static 
cancel_all_plans ()
{
    State *ps = (State *) myState;
    unActMsg unMe, unIt;
    register int i;

    /* for all plans */
    for ( i = 0; i < MAX_PLANS; i++ )
    {
	if ( ps->plans[i].available == FALSE )
	{
	    ps->plans[i].available = TRUE;

	    if ( gtVTime ( ps->plans[i].action_time, ps->now1 )
		&& ps->now1.simtime < CUT_OFF )
	    {
	    	clear ( &unMe, sizeof ( unMe ) );
		clear ( &unIt, sizeof ( unIt ) );
		unIt.action_id = ps->plans[i].action_id;
		unMe.action_id = ps->plans[i].action_id;
		myName ( unIt.with_whom );
		strcpy ( unMe.with_whom, ps->plans[i].with_whom );
		tell ( ps->my_name, ps->plans[i].action_time,
		     CANCEL_ACTION, sizeof ( unMe ), &unMe );
		tell ( ps->plans[i].with_whom, ps->plans[i].action_time,
		     CANCEL_ACTION, sizeof ( unIt ), &unIt );
	    }
	}
    }
}

static 
cancelPlans ( s )
Name s;
{
    State *ps = (State *) myState;
    PlanType pt;
    register int i;
    unActMsg unMe, unIt;

    for (i = 0; i < MAX_PLANS; i++)
    {
	if (ps->plans[i].available == FALSE &&
	    strcmp (ps->plans[i].with_whom, s) == 0)
	{

	    ps->plans[i].available = TRUE;

	    if (gtVTime (ps->plans[i].action_time, ps->now1 )
		&& ps->now1.simtime < CUT_OFF)
	    {
	    	clear ( &unMe, sizeof ( unMe ) );
		clear ( &unIt, sizeof ( unIt ) );
		unIt.action_id = ps->plans[i].action_id;
		unMe.action_id = ps->plans[i].action_id;
		myName ( unIt.with_whom );
		strcpy ( unMe.with_whom, ps->plans[i].with_whom );
		tell ( ps->my_name, ps->plans[i].action_time,
		     CANCEL_ACTION, sizeof ( unMe ), &unMe );
		tell ( ps->plans[i].with_whom, ps->plans[i].action_time,
		     CANCEL_ACTION, sizeof ( unIt ), &unIt );
	    }
	}
    }
}

static 
listSector ( s )
Name s;
{
    State *ps = (State *) myState;

    register int i;

    for ( i = 0; i < MAX_SECTORS; i++ )
    {
	if ( ps->sectors[i].available == FALSE &&
	    strcmp ( ps->sectors[i].element, s ) == 0 )
	{
	    return;
	}
    }

    for ( i = 0; i < MAX_SECTORS; i++ )
    {
	if ( ps->sectors[i].available == TRUE )
	{
	    ps->sectors[i].available = FALSE;
	    strcpy (ps->sectors[i].element, s);
	    return;
	}
    }

    userError ( "sector list overflow" );
}

static 
unlistSector ( s )
Name s;
{
    register int i;
    State *ps = (State *) myState;

    for (i = 0; i < MAX_SECTORS; i++)
    {

	if ( ps->sectors[i].available == FALSE &&
	    strcmp (ps->sectors[i].element, s) == 0 )
	{
	    ps->sectors[i].available = TRUE;
	    break;
	}

    }

}

static 
print_plan ( p)
PlanType * p;
{

#ifdef SHOWPLANS
    tw_printf ("PUCK:  Printing plan \n");
    tw_printf ("\tmy_name is %s\n", ps->my_name);
    tw_printf ("\tcollide with_whom = %s\n", p->with_whom);
    tw_printf ("\taction_id = %d\n", p->action_id);
    tw_printf ("\taction_time = %d\n", p->action_time);
    tw_printf ("\tavailable = %s\n", p->available ? "TRUE" : "FALSE");
#endif

}

#ifdef AUTO_UPDATE
static 
do_it_update ()

{
    State *ps = (State *) myState;
    VTime update_time;
    extern sim_debug ();
    Point newpoint;
    int change_pos;

    update_time = IncSimTime ((double) PUCK_UPDATE_INCREMENT);


#ifdef DISPLAY

    enter_critical_section ();
    newpoint = whereCircCtr ( &ps->Puck_State, ps->now1.simtime );
    leave_critical_section ();

/****************************************************************/
/* If the update will take the puck out-of-bounds, we do not 	*/
/* want to update, but we do want to send another update 	*/
/* message for later 						*/
/****************************************************************/

    if (checkbounds ( newpoint) == TRUE)
    {

	if (newpoint.x != ps->Puck_State.p.x ||
	    newpoint.y != ps->Puck_State.p.y )
	{
	    ps->Puck_State.p = newpoint;
	    change_pos = TRUE;
	}
	else
	{
	    change_pos = FALSE;
	}

	ps->Puck_State.t = ps->now1.simtime;

	if (change_pos)
	{
	    erase_puck ();
	}

	display_puck ();

    }



    if (update_time.simtime < CUT_OFF)
    {
    	tell ( ps->my_name, update_time, UPDATE_SELF, 7, "UpDate" );
    }
#endif DISPLAY

#ifdef  THERMAL_LOG

    if (neVTime (ps->Last_Log_Time, ps->now1))
    {
	if (ps->Thermal_Record_Count < MAX_THERMAL_RECORDS)
	{
	    ps->Thermal_Log[ps->Thermal_Record_Count].time =
	    ps->now1.simtime;
	    ps->Thermal_Log[ps->Thermal_Record_Count].mass =
	    ps->Ball_State.m;
	    ps->Thermal_Log[ps->Thermal_Record_Count].velocity =
	    ps->Ball_State.v;
	    ps->Thermal_Record_Count++;
	}
	else
	{
	    tw_printf ("ERROR %s Thermal_Log Overflow %d \n",
		    ps->my_name, ps->Thermal_Record_Count);
	}
	if (update_time.simtime < CUT_OFF)
	{
    	    tell ( ps->my_name, update_time, UPDATE_SELF, 7, "UpDate" );
	}
	ps->Last_Log_Time = ps->now1;
    }
#endif  THERMAL_LOG
}

#endif AUTO_UPDATE


/****************************************************************/
/* This routine checks if a puck has gone off the table for 	*/
/* any particular action. The CHEAT_CONST is a kludge 	*/
/* that is required because of various floating point garbage.	*/
/****************************************************************/


static 
checkbounds ( p )
Point p;
{
    State *ps = (State *) myState;

    if (
	(p.x < ((ps->worldmin.x + ps->Puck_State.r)
		- CHEAT_CONST))
	|| (p.x > ((ps->worldmax.x - ps->Puck_State.r)
		   + CHEAT_CONST))
	|| (p.y < ((ps->worldmin.y + ps->Puck_State.r)
		   - CHEAT_CONST))
	|| (p.y > ((ps->worldmax.y - ps->Puck_State.r)
		   + CHEAT_CONST))
	)

    {
	return FALSE;
    }
    else
    {
	return TRUE;
    }
}

static
errorCheck ( p, string )
Point p;
char * string;
{
    if (checkbounds ( p ) == FALSE)
    {
	tw_printf ( "errorCheck O_O_B  x = %f y = %f \n", p.x, p.y);
	userError ( string );
    }
}


static 
Puck_In_Sector_List ( Puck_State )
Circle Puck_State;
{
    State *ps = (State *) myState;

    char x_buf[3];
    char y_buf[3];
    int i;

    int x_pos, y_pos;


    for (i = 0; i < MAX_SECTORS; i++)
    {
	if (ps->sectors[i].available == FALSE)
	{

	    y_buf[0] = ps->sectors[i].element[7];
	    y_buf[1] = ps->sectors[i].element[8];
	    y_buf[2] = '\0';
	    x_buf[0] = ps->sectors[i].element[10];
	    x_buf[1] = ps->sectors[i].element[11];
	    x_buf[2] = '\0';

	    sscanf (x_buf, "%d", &x_pos);
	    sscanf (y_buf, "%d", &y_pos);

	    if (((Puck_State.p.x + Puck_State.r) >
		 (x_pos * ps->sector_width)) &&
		((Puck_State.p.x - Puck_State.r) <
		 ((x_pos + 1) * ps->sector_width)) &&
		((Puck_State.p.y + Puck_State.r) >
		 (y_pos * ps->sector_height)) &&
		((Puck_State.p.y - Puck_State.r) <
		 ((y_pos + 1) * ps->sector_height))
		)
	    {
		return TRUE;
	    }
	}
    }
    return FALSE;

}

/****************************************************************/
/* For Time Warp version 110 the printfs in this section need 	*/
/* to be commented out when doing timings.			*/
/****************************************************************/

puck_term ()
{
    State *ps = (State *) myState;

#ifdef THERMAL_LOG

    for (i = 0; i < ps->Thermal_Record_Count; i++)
    {
	tw_printf ("%s  %lf  %f %f %f \n",
		ps->my_name,
		ps->Thermal_Log[i].time.simtime,
		ps->Thermal_Log[i].mass,
		ps->Thermal_Log[i].velocity.x,
		ps->Thermal_Log[i].velocity.y
	    );
    }
#else


    tw_printf ("<TERM_STATS> %s %d %d %d %d %d %d %d %d\n", ps->my_name,
	    ps->Total_Puck_Collisions,
	    ps->Total_Puck_Collisions_Cancelled,
	    ps->Total_Cushion_Collisions,
	    ps->Total_Cushion_Collisions_Cancelled,
	    ps->Total_Enter_Sectors,
	    ps->Total_Enter_Sectors_Cancelled,
	    ps->Total_Depart_Sectors,
	    ps->Total_Depart_Sectors_Cancelled

	);
#endif
}
