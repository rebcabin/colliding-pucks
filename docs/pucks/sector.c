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


/************************************************************************/
/* sectors can be surrounded by 4 neighbors - all cushions or 		*/
/* all sector boundaries or a combination of both. In addition, sectors */
/* must keep a list of the pucks which reside in them.                  */
/************************************************************************/

typedef struct StateStruct
{
    PlanType plans[MAX_PLANS];
    double x0, y0;		/* this is the corner that the sector is
				 * drawn from */
    double height, width;
    double elasticity;
    int max_i, max_j;
    int action_id;
    List pucks[MAX_LIST_SIZE];
    Name cushions[MAX_CUSHIONS];
    BoundInfo boundary[MAX_BOUNDARIES];
    Point boundary_pt[MAX_BOUNDARIES];
    int boundary_count;
    int cushion_count;
    int Initialized;
    Name my_name;
    VTime now1;


    int Total_Enter_Sectors;
    int Total_Depart_Sectors;
    int Total_Enter_Sectors_Cancelled;
    int Total_Depart_Sectors_Cancelled;
    int Total_Velocity_Changes;
    int Total_New_Trajectories;

} State;

int sector_init ();
int sector_event ();
int sector_term ();

ObjectType sectorType =
{
    "sector",
    sector_init,
    sector_event,
    0,
    0,
    0,
    sizeof (State),
    0,
    0
};

sector_init ()
{
    register int i;

    State *ps = (State *) myState;

    ps->boundary_count = 0;
    ps->cushion_count = 0;
    ps->elasticity = 1;

    for (i = 0; i < MAX_PLANS; i++)
	ps->plans[i].available = TRUE;

    for (i = 0; i < MAX_LIST_SIZE; i++)
	ps->pucks[i].available = TRUE;

    myName ( ps->my_name );

    ps->Initialized = FALSE;
}

sector_event ()
{
    State *ps = (State *) myState;

    int totMsgs;
    register int msg_i;
    char cannedMsgs[MAX_NUMBER_OF_MESSAGES];

    totMsgs = numMsgs;
    ps->now1 = now;

    canTheMsgs ( cannedMsgs, ps, totMsgs );

    for (msg_i = 0; msg_i < totMsgs; msg_i++)
    {
	if ( cannedMsgs[msg_i] )
	    continue;

	switch (msgSelector (msg_i))
	{
	case CONFIG_MSG:
	case SECTOR_START:
	    sectorStart ( msg_i, ps );
	    break;

	case INITIAL_VELOCITY:
	    initVelocity ( msg_i, ps );
	    break;

	case CHANGE_VELOCITY:
	    changeVelocity ( msg_i, ps );
	    ps->Total_Velocity_Changes++;
	    break;

	case NEW_TRAJECTORY:
	    newPuck ( msg_i, ps );
	    ps->Total_New_Trajectories++;
	    break;

	case ENTER_SECTOR:
	    arriveSector ( msg_i, ps );
	    ps->Total_Enter_Sectors++;
	    break;

	case DEPART_SECTOR:
	    leaveSector ( msg_i, ps );
	    ps->Total_Depart_Sectors++;
	    break;

#ifdef AUTO_UPDATE
	case UPDATE_SELF:
	    display_sector ( ps );
	    break;
#endif AUTO_UPDATE

	default:
	    tw_printf ("%s RECEIVED UNKNOW MSGTYPE %d\n",
		ps->my_name, msgSelector ( msg_i ) );
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
char *cannedMsgs;
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
	    case INITIAL_VELOCITY:
	    case CHANGE_VELOCITY:
	    case CANCEL_ACTION:
	    case UPDATE_SELF:
	    case SECTOR_START:
	    case CONFIG_MSG:
		/* the above are info msgs -- they can't be cancelled */
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


sectorStart ( msg_i, ps )
int msg_i;
State *ps;
{
    char *p;
    int i;

    p = (char *) msgText ( msg_i );
    
    while ( *p < '0' || *p > '9' )
	p++;

    i = sscanf ( p, "%d %d %lf %lf %lf %lf",
	    &ps->max_i, &ps->max_j,
	    &ps->x0, &ps->y0,
	    &ps->height, &ps->width );

    if ( i != 6 )
    	userError ( "sector didn't get enuff parameters" );

    calculate_boundaries ( ps );
    ps->Initialized = TRUE;

#ifdef AUTO_UPDATE
    tell ( ps->my_name, IncSimTime ( 1.0 ), UPDATE_SELF, 0, 0);
#endif AUTO_UPDATE
}

initVelocity ( msg_i, ps )
int msg_i;
State *ps;
{
    infoMsg *mp;

    mp = (infoMsg *) msgText ( msg_i );

    if (!ps->Initialized)
	userError ( "uninitialized sector with init velocity msg" );

    listPuck ( mp->puck_name, ps );
    informWorld ( mp->puck_name, mp->Puck_State, ps );
    schedule_departure ( mp, ps );
}

/************************************************************************/
/* 	CHANGE_VELOCITY messages are sent by pucks just after they 	*/
/*	have collided with either another puck or a cushion. These	*/
/*	messages are sent to the residing sector which cancels all 	*/
/* 	of the previous plans which it may have had with that puck	*/
/*	and establishes the pucks new trajectory.			*/
/************************************************************************/

changeVelocity ( msg_i, ps )
int msg_i;
State *ps;
{
    infoMsg *mp;

    if (!ps->Initialized)
	userError ( "unitialized sector with change velocity msg" );

    mp = (infoMsg *) msgText ( msg_i );
/************************************************************************/
/*	Cancel this sector's previous plans with this puck		*/
/************************************************************************/
    deletePlansWith ( mp->puck_name, ps );

    if ( !puck_in_list ( mp->puck_name, ps ) )
    {
	return;
    }

    informWorld ( mp->puck_name, mp->Puck_State, ps );
    schedule_departure ( mp, ps );

}

newPuck ( msg_i, ps )
int msg_i;
State *ps;
{
    int i;
    int puck_in = FALSE;
    int in_sector = TRUE;
    Collision Xing, Last_Xing;
    Circle Puck_State, Last_Puck_State;
    actionMsg eds, eds2;
    infoMsg *mp;

    if (!ps->Initialized)
	userError ( "uninitialized sector with new traj msg" );

    mp = (infoMsg *) msgText ( msg_i );
/************************************************************************/
/* 	Go through this sector's puck list and see if the puck for 	*/
/*	this message is already in here.				*/
/************************************************************************/
    for (i = 0; i < MAX_LIST_SIZE; i++)
    {
	if ( ps->pucks[i].available == FALSE &&
		strcmp (ps->pucks[i].element, mp->puck_name) == 0 )
	{
	    puck_in = TRUE;
	    break;
	}
    }

/************************************************************************/
/*	Here we are trying to determine if the puck for the Examine	*/
/*	New Trajectory message is going to enter this sector. This	*/
/*	is done by calling the circles routine circAfterLineSegColl_SE()*/
/*	for each of the sides of the sector. If an intersection occurs  */
/*	with any of the sector boundaries, the intersection with the 	*/
/*	lowest Absolute Time is choosen for the ENTER_SECTOR		*/
/*	transaction.							*/
/************************************************************************/

    if (puck_in == FALSE) /* done at change velocity time if it's in */
    {

/************************************************************************/
/* 	Cancel any previous plans we had for this puck if it was not 	*/
/*			   in this sector.				*/
/************************************************************************/

	deletePlansWith ( mp->puck_name, ps );

/************************************************************************/
/* 	Check each of the sector's boundaries for the puck's intersect 	*/
/*	and pick the earliest intersection for the time the puck enters	*/
/*	this sector.							*/
/************************************************************************/

	Last_Xing.yes = Xing.yes = FALSE;
	Last_Xing.at = Xing.at = 0.0;

	for (i = 0; i < ps->boundary_count; i++)
	{
	    Puck_State = mp->Puck_State;

	    enter_critical_section ();
	    Xing = circLineSegmentIntersect_SE
		( &Puck_State, ps->boundary[i].line );
	    leave_critical_section ();

	    if (Xing.yes && checkbounds ( Puck_State, ps )
	    	&& (Xing.at > ps->now1.simtime) )
	    {

		if (!Last_Xing.yes || Xing.at < Last_Xing.at)
		{
		    Last_Xing.yes = Xing.yes;
		    Last_Xing.at = Xing.at;
		    Last_Puck_State = Puck_State;
		}
	    }

	    Puck_State = mp->Puck_State;

	    enter_critical_section ();
	    Xing = circPointIntersect_SE ( &Puck_State, ps->boundary_pt[i] );
	    leave_critical_section ();

	    if (Xing.yes && checkbounds ( Puck_State, ps )
	    	&& (Xing.at > ps->now1.simtime * TIME_FUDGE ) )
	    {

		if (!Last_Xing.yes || Xing.at < Last_Xing.at )
		{
		    Last_Xing.yes = Xing.yes;
		    Last_Xing.at = Xing.at;
		    Last_Puck_State = Puck_State;
		}
	    }

	}
	if (!Last_Xing.yes)
	    return;

	clear (&eds, sizeof (eds));
	clear (&eds2, sizeof (eds2));

	if ( Last_Xing.at > ps->now1.simtime )
	{
	    eds.action_time = newVTime ( Last_Xing.at, 0, 0 );
	}
	else
	{
	    eds.action_time = IncSequence2 ( 1 );
	    Last_Puck_State = Puck_State; /* sent current info */
	}

	eds2.action_time = eds.action_time;

/************************************************************************/
/*	If there was an intersect we now have the earliest one and we 	*/
/*	can schedule an ENTER_SECTOR event. This event message is sent	*/
/*	to the puck involved and to this sector.			*/
/************************************************************************/
	if ( eds.action_time.simtime < CUT_OFF )
	{
	    strcpy (eds.with_whom, ps->my_name);
	    strcpy ( eds2.with_whom,  mp->puck_name );
	    eds2.action_id = eds.action_id =
		ps->action_id++;
	    eds2.Puck_State = eds.Puck_State = Last_Puck_State;

	    tell ( mp->puck_name, eds.action_time,
	    	ENTER_SECTOR, sizeof ( eds ), &eds );
	    tell ( ps->my_name, eds2.action_time,
	    	ENTER_SECTOR, sizeof ( eds2 ), &eds2 );

	    addToPlans ( mp->puck_name, eds.action_time, eds.action_id,
	    	ps );
	}
    }
}


/************************************************************************/
/* 	ENTER_SECTOR  messages are sent by sectors to the pucks involved*/
/*	and themselves. As the name implies, this message announces when*/
/*	and where a puck will enter the receiving sector. When a sector */
/*	gets an ENTER_SECTOR message it must send an NEW_TRAJ- 	*/
/* 	ECTORY message to all of its neighbors and all of the puck that	*/
/*	are currently residing in this sector. Then it must schedule the*/
/*	departure time and position of the puck and send the appropriate*/
/*	DEPART_SECTOR messages.						*/
/************************************************************************/

arriveSector ( msg_i, ps )
int msg_i;
State *ps;
{
    actionMsg *mp;
    infoMsg pv;
    int i;

    if (!ps->Initialized)
	userError ( "uninitialized sector with enter sect msg" );

    mp = (actionMsg *) msgText ( msg_i );

    listPuck ( mp->with_whom, ps );
    informWorld ( mp->with_whom, mp->Puck_State, ps );
    deletePlansWith ( mp->with_whom, ps );
    strcpy ( pv.puck_name, mp->with_whom );
    pv.Puck_State = mp->Puck_State;
    schedule_departure ( &pv, ps );		   
}

/************************************************************************/
/* 	DEPART_SECTOR messages are sent by sectors to the puck involved */
/*	and themselves. This message informs a sector when an where a	*/
/*      puck is to depart its region. When a sector gets a DEPART_SECTOR*/
/*	message it simply cancels all of the future plans it may have	*/
/*	had for this puck and removes it from its list of pucks.	*/
/************************************************************************/

leaveSector ( msg_i, ps )
int msg_i;
State *ps;
{
    actionMsg *mp;
    
    mp = ( actionMsg *) msgText ( msg_i );

    if (!ps->Initialized)
	userError ( "uninitialized sector with depart sect msg" );

    unlistPuck ( mp->with_whom, ps );
    deletePlansWith ( mp->with_whom, ps);
}


informWorld ( puckName, puckState, ps )
Name puckName;
Circle puckState;
State *ps;
{
    infoMsg pv;
    int i;
    VTime now2;

    if (ps->now1.simtime > CUT_OFF)
    	return;

    now2 = IncSequence2 ( 1 );

    clear (&pv, sizeof (pv));
    strcpy (pv.puck_name, puckName);
    pv.Puck_State = puckState;

    for (i = 0; i < ps->boundary_count; i++)
    {
    	tell ( ps->boundary[i].neighbor, now2,
	    NEW_TRAJECTORY, sizeof ( pv ), &pv );
    }

    for (i = 0; i < MAX_LIST_SIZE; i++)
    {
	if ( ps->pucks[i].available == FALSE &&
	    strcmp ( ps->pucks[i].element, puckName ) != 0 )
	{
	    tell ( ps->pucks[i].element, now2,
		NEW_TRAJECTORY, sizeof ( pv ), &pv );
	}
    }

    for (i = 0; i < ps->cushion_count; i++)
    {
    	tell ( ps->cushions[i], now2,
	    NEW_TRAJECTORY, sizeof ( pv ), &pv );
    }
}


/************************************************************************/
/*	Calculate_boundaries () fills in two important areas of this	*/
/*	sector's state. 						*/
/*	First, it determines  the object names for all of the neighbors	*/
/*	for this sector, be they other sectors or table cushions.   	*/
/*									*/
/*	Then it creates the four line segments which make up the  	*/
/*	sector's boundaries and stores them in their state.		*/
/*									*/
/************************************************************************/

calculate_boundaries ( ps )
State *ps;
{
    int i, j;

    int ii, jj;
    char buf[3];
    Name east, west, north, south;
    Point point, point1;

/************************************************************************/
/* 	Sectors are named by the convention sector_ii_jj. With ii &	*/
/*	jj representing the relative position of the sector. ii is the	*/
/* 	sector position relative to the x axis and jj is the sector 	*/
/*	position relative to the y axis.				*/
/*									*/
/*	Cushion follow the naming format cushion_x_pp. Where x is the	*/
/*	side of the table for the cushion i.e. west = 0, north = 1,	*/
/*	east = 2 and south = 3. pp is the nth member of a particular	*/
/*	side.								*/
/************************************************************************/


    buf[0] = ps->my_name[10];
    buf[1] = ps->my_name[11];
    buf[2] = '\0';
    sscanf (buf, "%d", &jj);

    buf[0] = ps->my_name[7];
    buf[1] = ps->my_name[8];
    buf[2] = '\0';

    sscanf (buf, "%d", &ii);

/************************************************************************/
/* ***Note*** The check of ii & jj for <9 is a kludge needed for MarkIII*/
/* hypercube portability since the Counterpoint compiler has a sprintf  */
/* bug which causes %2.2 to produce a leading blank instead of a leading*/
/* 0.									*/
/************************************************************************/

    i = j = 0;

/************************************************************************/
/*	Generate the West Boundary and neighbor || cushion		*/
/************************************************************************/

    if (jj == 0)		/* no west neighbor except cushion_0 */
    {
	sprintf (west, "cushion_0_%2.2d", ii);

	if (ii < 9)		/* This kludge is due to a Counterpoint	 */
	{			/* compiler bug in sprintf()		 */
	    west[10] = '0';
	}
	else
	{
	    west[10] = (ii / 10) + '0';
	    west[11] = (ii % 10) + '0';
	}

	strcpy (ps->cushions[j], west);
	j++;			/* counter for cushion_count */

    }

    else
    {
	sprintf (west, "sector_%2.2d_%2.2d", ii, jj - 1);

	if (ii < 9)		/* This kludge is due to a Counterpoint	 */
	{			/* compiler bug in sprintf()		 */
	    west[7] = '0';
	}
	else
	{
	    west[7] = (ii / 10) + '0';
	    west[8] = (ii % 10) + '0';
	}

	if (jj < 9)
	{
	    west[10] = '0';
	}
	else
	{
	    west[10] = ((jj - 1) / 10) + '0';
	    west[11] = ((jj - 1) % 10) + '0';
	}

	strcpy (ps->boundary[i].neighbor, west);

	point.x = (jj * ps->width);
	point.y = (ii * ps->height);
	point1.x = point.x;
	point1.y = (point.y + ps->height);

	enter_critical_section ();
	ps->boundary[i].line =
	    constructLineSegment ( point, constructVectFromPts(point, point1) );
	leave_critical_section ();

	ps->boundary[i].direction = WEST;
	ps->boundary_pt[i] = point;
	i++;			/* counter for boundary_count (ie. sector
				 * neighbors ) */
    }

/************************************************************************/
/*	Generate the East Boundary and neighbor || cushion		*/
/************************************************************************/

    if (jj == (ps->max_j - 1))	/* no east neighbor except cushion_2 */
    {
	sprintf (east, "cushion_2_%2.2d", ii);

	if (ii < 9)		/* This kludge is due to a Counterpoint	 */
	{			/* compiler bug in sprintf()		 */
	    east[10] = '0';
	}
	else
	{
	    east[10] = (ii / 10) + '0';
	    east[11] = (ii % 10) + '0';
	}

	strcpy (ps->cushions[j], east);
	j++;
    }

    else
    {
	sprintf (east, "sector_%2.2d_%2.2d", ii, jj + 1);

	if (ii < 9)		/* This kludge is necessary because of a bug
				 * in	 */
	{			/* the Counterpoint compiler in sprintf()	 */
	    east[7] = '0';
	}
	else
	{
	    east[7] = (ii / 10) + '0';
	    east[8] = (ii % 10) + '0';
	}

	if (jj < 9)
	{
	    east[10] = '0';
	}
	else
	{
	    east[10] = ((jj + 1) / 10) + '0';
	    east[11] = ((jj + 1) % 10) + '0';
	}



	strcpy (ps->boundary[i].neighbor, east);

	point.x = ((jj + 1) * ps->width);
	point.y = (ii * ps->height);
	point1.x = point.x;
	point1.y = (point.y + ps->height);

	enter_critical_section ();
	ps->boundary[i].line =
	    constructLineSegment (point, constructVectFromPts (point, point1));
	leave_critical_section ();

	ps->boundary[i].direction = EAST;
	ps->boundary_pt[i].x = point.x;
	ps->boundary_pt[i].y = point1.y;
	i++;
    }

/************************************************************************/
/*	Generate the North Boundary and neighbor || cushion		*/
/************************************************************************/

    if (ii == 0)		/* no north neighbor except cushion */
    {
	sprintf (north, "cushion_1_%2.2d", jj);

	if (jj < 9)		/* This kludge is due to a Counterpoint	 */
	{			/* compiler bug in sprintf()		 */
	    north[10] = '0';
	}
	else
	{
	    north[10] = (jj / 10) + '0';
	    north[11] = (jj % 10) + '0';
	}

	strcpy (ps->cushions[j], north);
	j++;
    }

    else
    {
	sprintf (north, "sector_%2.2d_%2.2d", ii - 1, jj);

	if (ii < 9)		/* This kludge is necessary because of a  */
	{			/* Counterpoint compiler bug in sprintf() */
	    north[7] = '0';
	}
	else
	{
	    north[7] = ((ii - 1) / 10) + '0';
	    north[8] = ((ii - 1) % 10) + '0';
	}

	if (jj < 9)
	{
	    north[10] = '0';
	}
	else
	{
	    north[10] = (jj / 10) + '0';
	    north[11] = (jj % 10) + '0';
	}

	strcpy (ps->boundary[i].neighbor, north);

	point.x = (jj * ps->width);
	point.y = (ii * ps->height);
	point1.x = (point.x + ps->width);
	point1.y = point.y;

	enter_critical_section ();
	ps->boundary[i].line =
	    constructLineSegment ( point, constructVectFromPts (point, point1));
	leave_critical_section ();

	ps->boundary[i].direction = NORTH;
	ps->boundary_pt[i].x = point1.x;
	ps->boundary_pt[i].y = point.y;
	i++;
    }

/************************************************************************/
/*	Generate the South Boundary and neighbor || cushion		*/
/************************************************************************/

    if (ii == (ps->max_i - 1))
	/* no south neighbor except cushion */
    {
	sprintf (south, "cushion_3_%2.2d", jj);

	if (jj < 9)		/* This kludge is due to a Counterpoint	 */
	{			/* compiler bug in sprintf()		 */
	    south[10] = '0';
	}
	else
	{
	    south[10] = (jj / 10) + '0';
	    south[11] = (jj % 10) + '0';
	}

	strcpy (ps->cushions[j], south);
	j++;
    }

    else
    {
	sprintf (south, "sector_%2.2d_%2.2d", ii + 1, jj);

	if (ii < 9)
	{
	    south[7] = '0';
	}
	else
	{
	    south[7] = ((ii + 1) / 10) + '0';
	    south[8] = ((ii + 1) % 10) + '0';
	}

	if (jj < 9)
	{
	    south[10] = '0';
	}
	else
	{
	    south[10] = (jj / 10) + '0';
	    south[11] = (jj % 10) + '0';
	}

	strcpy (ps->boundary[i].neighbor, south);

	point.x = (jj * ps->width);
	point.y = ((ii + 1) * ps->height);
	point1.x = (point.x + ps->width);
	point1.y = point.y;

	enter_critical_section ();
	ps->boundary[i].line =
	    constructLineSegment ( point, constructVectFromPts (point, point1));
	leave_critical_section ();

	ps->boundary[i].direction = SOUTH;
	ps->boundary_pt[i] = point;
	i++;
    }

    ps->boundary_count = i;
    ps->cushion_count = j;

}

#ifdef AUTO_UPDATE

/************************************************************************/
/*  This function transmits a graphics message out to the stdout	*/
/*  object of Time Warp. This graphics message is designed to interact	*/
/*  with the pad graphics interpreter which runs on the SUN.		*/
/*  After this message is sent the next display sector procedure is	*/
/*  scheduled in the future according to DRAW_INCREMENT.		*/
/************************************************************************/

int 
display_sector ( ps )
State *ps;
{
    VTime display_time;
    int i;
    Point q;

#ifdef DISPLAY

    for (i = 0; i < ps->boundary_count; i++)
    {
	q = otherEndPoint
	    (
	     ps->boundary[i].line.e,
	     ps->boundary[i].line.l
	    );

	tw_printf ( "pad:vector %f %f %f %f\n", ps->boundary[i].line.e.x,
	     ps->boundary[i].line.e.y, q.x, q.y);
    }

#endif DISPLAY

    if ( ps->now1.simtime < CUT_OFF )
    {
	display_time = IncSimTime ((double) SECTOR_UPDATE_INCREMENT);
	tell ( ps->my_name, display_time, UPDATE_SELF, 9, "doUpdate" );
    }

}

#endif AUTO_UPDATE

static 
addToPlans ( with_whom, for_when, id_number, ps )
Name with_whom;
VTime for_when;
int id_number;
State *ps;
{
    register int i;

    for (i = 0; ; i++)
    {
	if (i >= MAX_PLANS)
	{
	    userError ( "Too many plans");
	}
	else if (ps->plans[i].available == TRUE)
	{
	    ps->plans[i].available = FALSE;
	    strcpy (ps->plans[i].with_whom, with_whom);
	    ps->plans[i].action_id = id_number;
	    ps->plans[i].action_time = for_when;
	    return;
	}
    }
}

int 
listPuck ( s, ps )
Name s;
State *ps;
{
    register int i;


    for ( i = 0; ; i++ )
    {
	if ( i >= MAX_LIST_SIZE )
	{
	    userError ( "Too many pucks" );
	}
	else if ( ps->pucks[i].available == TRUE )
	{
	    ps->pucks[i].available = FALSE;
	    strcpy ( ps->pucks[i].element, s );
	    break;
	}
    }
}

int 
unlistPuck ( s, ps )
Name s;
State *ps;
{
    register int i;


    for ( i = 0; i < MAX_LIST_SIZE; i++ )
    {
	if ( ps->pucks[i].available == FALSE &&
	    strcmp ( ps->pucks[i].element, s ) == 0 )
	{
	    ps->pucks[i].available = TRUE;
	}
    }
}

int 
deletePlansWith ( s, ps )
Name s;
State *ps;
{
    unActMsg Mine, His;
    register int i;

    for (i = 0; i < MAX_PLANS; i++)
    {

	if (ps->plans[i].available == FALSE &&
	    strcmp (ps->plans[i].with_whom, s) == 0
	    )
	{

	    ps->plans[i].available = TRUE;
	    if ( gtVTime ( ps->plans[i].action_time, ps->now1 )
		&& ps->now1.simtime < CUT_OFF )
	    {
		clear ( &Mine, sizeof ( Mine ) );
		clear ( &His, sizeof ( His ) );
		strcpy ( Mine.with_whom, ps->plans[i].with_whom );
		myName ( His.with_whom );
		His.action_id = Mine.action_id =
		    ps->plans[i].action_id;
		tell ( ps->my_name, ps->plans[i].action_time,
		    CANCEL_ACTION, sizeof ( Mine ), &Mine );
		tell ( Mine.with_whom, ps->plans[i].action_time,
		    CANCEL_ACTION, sizeof ( His ), &His );
	    }
	}
    }
}


/************************************************************************/
/*  This procedure determines when a puck which is within this sector.  */
/*  Will depart this sector. This is done by calls to the circles 	*/
/*  package routine circLineSegDepart_SE.			 	*/
/*  This routine projects a temporary parallel line segment beyond the  */
/*  borders of the sector which is 1 puck diameter away from the actual */
/*  sector boundary. By doing this the sector departure can be 		*/
/*  determined by the intersect time of these extended Line Segments.	*/
/*									*/
/*  When the earliest departure time and position is determined, the	*/
/*  appropriate DEPART_SECTOR message will be sent to the puck that is  */
/*  involved.								*/
/*									*/
/************************************************************************/


int 
schedule_departure ( pv, ps )
infoMsg *pv;
State *ps;
{
    actionMsg eds, eds2;
    Collision Xing, Last_Xing;
    Circle Puck_State, Last_Puck_State;
    int i;
    int Displacement_Direction;

    Last_Xing.yes = Xing.yes = FALSE;
    Last_Xing.at = Xing.at = 0.0;

    for ( i = 0; i < ps->boundary_count; i++ )
    {
	if (ps->boundary[i].direction == EAST ||
	    ps->boundary[i].direction == NORTH)
	{
	    Displacement_Direction = NEGATIVE;
	}
	else
	{
	    Displacement_Direction = POSITIVE;
	}
	Puck_State = pv->Puck_State;

	enter_critical_section ();
	Xing = circLineSegDepart_SE
	    ( &Puck_State, ps->boundary[i].line, Displacement_Direction );
	leave_critical_section ();

	if ( Xing.yes && checkbounds ( Puck_State, ps ) )
	{
	    if ( !Last_Xing.yes || (Xing.at < Last_Xing.at) )
	    {
		Last_Xing.yes = Xing.yes;
		Last_Xing.at = Xing.at;
		Last_Puck_State = Puck_State;
	    }
	}
    }
    if ( !Last_Xing.yes )
    {
	return 0;
    }
    clear ( &eds, sizeof ( eds ) );
    clear ( &eds2, sizeof ( eds2 ) );

    if ( Last_Xing.at > ps->now1.simtime )
    {
	eds.action_time = newVTime ( Last_Xing.at, 0, 0 );
    }
    else
    {
	eds.action_time = IncSequence2 ( 1 );
	Last_Puck_State = Puck_State; /* sent current info */
    }

    eds2.action_time = eds.action_time;

/************************************************************************/
/*	If there was an intersect we now have the earliest one and we 	*/
/*	can schedule an ENTER_SECTOR event. This event message is sent	*/
/*	to the puck involved and to this sector.			*/
/************************************************************************/
    if ( eds.action_time.simtime < CUT_OFF )
    {
	myName ( eds.with_whom );
	strcpy ( eds2.with_whom, pv->puck_name );
	eds2.action_id = eds.action_id = ps->action_id++;
	eds2.Puck_State = eds.Puck_State = Last_Puck_State;
	tell ( pv->puck_name, eds.action_time,
	    DEPART_SECTOR, sizeof ( eds ), &eds );
	tell ( ps->my_name, eds2.action_time,
	    DEPART_SECTOR, sizeof ( eds2 ), &eds2 );
	addToPlans ( pv->puck_name, eds.action_time, 
	    eds.action_id, ps );
    }
}

static 
print_plan ( p )
PlanType * p;
{
    State *ps = (State *) myState;

    tw_printf ( "%s: Printing plan\n", ps->my_name);
    tw_printf ( "\tinteract with_whom = %s\n", p->with_whom);
    tw_printf ( "\taction_id = %d\n", p->action_id);
    tw_printf ( "\taction_time = %d\n", p->action_time);
    tw_printf ( "\tavailable = %s\n", p->available ? "TRUE" : "FALSE");
}

static 
print_boundaries ( ps )
State *ps;
{
    int i;

    tw_printf ("%s: Printing boundaries\n", ps->my_name);
    for (i = 0; i < ps->boundary_count; i++)
    {
	tw_printf ("\tneighbor = %s\n", ps->boundary[i].neighbor);
	tw_printf ("\tx = %f\n", ps->boundary[i].line.e.x);
	tw_printf ("\ty = %f\n", ps->boundary[i].line.e.y);
	tw_printf ("\tnx = %f\n", ps->boundary[i].line.l.x);
	tw_printf ("\tny = %f\n", ps->boundary[i].line.l.y);
    }
}

static 
print_cushions ( ps )
State *ps;
{
    int i;

    tw_printf ("%s: Printing cushions\n", ps->my_name);
    for (i = 0; i < ps->cushion_count; i++)
    {
	tw_printf ("\tcushion boundary = %s\n", ps->cushions[i]);
    }
}

static 
checkbounds ( Puck_State, ps )
Circle Puck_State;
State *ps;
{
    if ( (Puck_State.p.x < (Puck_State.r - CHEAT_CONST)) ||
	(Puck_State.p.x >
	 ( ((ps->max_j * ps->width) - Puck_State.r) + CHEAT_CONST)) ||
	(Puck_State.p.y < (Puck_State.r - CHEAT_CONST)) ||
	(Puck_State.p.y >
	 ( ((ps->max_i * ps->height) - Puck_State.r) + CHEAT_CONST))
	)
    {
	return FALSE;
    }
    else
    {
	return TRUE;
    }
}

int 
puck_in_list ( s, ps )
Name s;
State *ps;
{
    int i;

/************************************************************************/
/*	Find this puck in the sector's list of pucks			*/
/************************************************************************/
    for (i = 0; i < MAX_LIST_SIZE; i++)
    {
	if (strcmp ( ps->pucks[i].element, s ) == 0)
	{
	    return TRUE;
	}
    }
    return FALSE;

}

sector_term ()
{
    State *ps = (State *) myState;

    tw_printf ("<TERM_STATS> %s %d %d %d %d %d %d\n", ps->my_name,
	    ps->Total_Enter_Sectors,
	    ps->Total_Depart_Sectors,
	    ps->Total_Enter_Sectors_Cancelled,
	    ps->Total_Depart_Sectors_Cancelled,
	    ps->Total_Velocity_Changes,
	    ps->Total_New_Trajectories );

}
