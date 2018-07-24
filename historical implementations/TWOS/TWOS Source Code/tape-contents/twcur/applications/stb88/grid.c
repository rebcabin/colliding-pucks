/*
 * grid.c : Fred Wieland, Jet Propulsion Laboratory, October 1987
 *			This program codes the grid object, which is essentially a
 *			database of positions and is used to detect combat and provide
 * 		intel.
 * CREATED	26 October 1987
 * MODIFIED continually
 */

#ifdef BF_MACH
#include <stdio.h>
#include <math.h>
#include "pjhmath.h"

#ifndef HUGE
#define HUGE MAXFLOAT
#endif

#else
#endif

#include "twcommon.h"
#include "motion.h"
#include "grid.h"
#include "stb88.h"
#include "gridmsg.h"
#include "divmsg.h" 
#include "array.h"

#include <math.h>

#define tprintf if(s->traceon==1)printf  /* trace printf */
#define rprintf if(s->reporton==1)printf  /* report printf */
#define gprintf if(s->graphon==1)printf  /* graphics (pad) printf */


/*************************************************************************
 *
 *            M E S S A G E    D A T A    S T R U C T U R E S  
 *
 *************************************************************************/ 
typedef union Input_Message
	{
	Selector select;
 	Initialize  in;
  	Add_unit add;
  	Del_unit del;
  	Evaluate eval;
  	Change_velocity change;
  	} Input_message;

typedef union Output_Message
	{
   Selector select;
	Perception p;
   Add_unit add;
	Del_unit del;
	Evaluate eval;
   Change_location change;
	} Output_message;

/*************************************************************************
 *
 *     M I S C E L L A N E O U S    D A T A    S T R U C T U R E S
 *
 *************************************************************************/ 
typedef struct ExitInfo /* for routine */
	{
	Name_object Grid;
	Simulation_time addTime, delTime;
	int num_simultaneous;
	} ExitInfo;

typedef struct CollInfo  /* for collision() routine */
	{
	int indx;
	Simulation_time time, time2;
	int num_simultaneous;
	} CollInfo;

typedef struct History
	{
	int whichEvent;
	Simulation_time When;
	} History;

/*
 * the following typedef assumes the longest name will be 13 chars of
 * the form blue_divxx/yy, where xx is the division number and yy is the
 * corps number.
 */

#define OBJLEN 15
typedef char Obj_name[OBJLEN];  /* to save space; need not 20 chars */

typedef struct PerHist
	{
	int index; /* this is an index into the s->Units[] array in the state */
	int ismarked;
	} PerHist;

/*
 * The following constant determines how many eval messages the object
 * keeps track of. If the number is less than the number of collisions
 * likely in the grid cell, then duplicate eval messages will be sent.
 * The program will still work correctly with duplicate eval messages,
 * but it works more efficiently if the duplicates are removed.
 */
#define MAX_EVAL_ARRAY 25  
#define MAX_HIST_ARRAY 10
#define MAX_PERCEIVE 25
#define UNITS_PER_GRID 20  /* also change in msgsize.c */

/*************************************************************************
 *
 *                           S T A T E 
 *
 *************************************************************************/ 
typedef struct state
	{
   Int number;  	/* number messages scheduled for now */
   Obj_name myself; 	/* name of myself */ 
	int side;
	struct Units
		{
		Name_object UnitName;
		Particle where;		/* consists of where.pos and where.vel */
		Simulation_time time;/* time at which where.vel updated */
		Obj_name GridLocs[4];	/* grid locations aware of this unit*/
		int pradius; 			/* perception radius */
		PerHist perceive[MAX_PERCEIVE]; 
		History past[MAX_HIST_ARRAY];
		int changed_v;			/* TRUE if unit changed its velocity this event */
		Vector circle;   /* coordinates of last circle drawn */
		int should_delete; /* default=FALSE; TRUE if del_unit is run */
		} Units[UNITS_PER_GRID];
	struct Neighbors
		{
		Obj_name name;
		Line boundary;  /* lines forming boundary of neighbor */
		} Neighbors[4]; /* indexed by NORTH, SOUTH, EAST, and WEST */
   Vector corner[4];  /* indexed by SW or NE for SouthWest or NorthEast */
	int gridx, gridy;  /* x and y grid coordinates */
	Simulation_time scheduled_eval[MAX_EVAL_ARRAY]; 
	char blanks[30];
  /*
   * Flags. All values are either TRUE or FALSE.
   */
	int first_run;  
	int graphon;  /* if TRUE, then pad graphics turned on */
	int reporton; /* if TRUE, then report output on */
	int traceon;  /* if TRUE, then trace output on */
	int been_inited; /* TRUE if the initialize behavior has fired. */
	int enabled_grids; /* TRUE if you want grid lines on pad output */
	int enabled_objs;  /* TRUE if you want to see pradius of tactical 
								 objects on pad output. Default is FALSE */
	} state;

/************************************************************************** 
 *
 *         O B J E C T    T Y P E    D E F I N I T I O N
 *
 **************************************************************************
 These definitions are necessary for the Time Warp Operating System, and
 are not needed for the STB88 algorithms.
 */
ObjectType gridType = {"grid", i_GRID, e_GRID, t_GRID, 0, 0, sizeof(state),0,0};

/*************************************************************************
 *
 *           F U N C T I O N    D E C L A R A T I O N S
 *
 *************************************************************************/ 
Simulation_time next_eval();
Simulation_time search_hist();

/*************************************************************************
 *
 *                      I N I T    S E C T I O N
 *
 *************************************************************************/ 
init()
   {
	int i, j;
	state *s = (state *)myState ;

   /* printf("sizeof grid state = %d\n", sizeof(state)); */

   clear(s->myself, OBJLEN);
	s->first_run = TRUE;
	strcpy(s->blanks, "                             ");
	/*strcpy(s->myself, myName);*/ myName(s->myself);
	s->traceon = 0;
	s->reporton = 0;
	s->graphon = FALSE;
	s->enabled_grids = FALSE;  /* for PAD graphics */
	s->enabled_objs = FALSE; /* pad graphics */
	s->been_inited = FALSE;
	for (i = 0; i < UNITS_PER_GRID; i++)
		for (j = 0; j < MAX_PERCEIVE; j++)
			s->Units[i].perceive[j].index = -1;

   }

/************************************************************************
 *                                                                      *
 *                   M A I N    E V E N T    L O O P                    *
 *                                                                      *
 ************************************************************************/
event()
   {
   int i;
	Int bread, error;
   Input_message m;
	Simulation_time time;
	Name_object fromwhom;
	state *s = (state *) myState;

   pre_process(s);

   if (now > CUTOFF_TIME) return;
   for (i = 0; i < s->number; i++)
		{
		m = * (Input_message *) msgText(i);
		strcpy(fromwhom, msgSender(i));
		time = msgSendTime(i);
      switch (m.select.code)
			{
			case INITIALIZE:
				tprintf("\n%s@%ld : INITIALIZE\n", s->myself, now);
				rprintf("\n%-20s", "INITIALIZE");
				initialize(s);
				break;
			case ADD_UNIT:
				tprintf("\n%s@%ld : ADD_UNIT\n", s->myself, now);
				rprintf("\n%-20s", "ADD_UNIT");
				add_unit(s, &m, fromwhom);
				break;
			case DELETE_UNIT:
				tprintf("\n%s@%ld : DELETE_UNIT\n", s->myself, now);
				rprintf("\n%-20s", "DELETE_UNIT");
				del_unit(s, &m, time);
				break;
			case EVALUATE:
				tprintf("\n%s@%ld : EVALUATE\n", s->myself, now);
				rprintf("\n%-20s", "EVALUATE");
				eval(s, &m);
				break;
			case CHANGE_VELOCITY:
				tprintf("\n%s@%ld : CHANGE_VELOCITY\n", s->myself, now);
				rprintf("\n%-20s", "CHANGE_VELOCITY");
				change_v(s, &m, fromwhom);
				break;
			case TRACE_ON:
				s->traceon = 1;
				break;
			case TRACE_OFF:
				s->traceon = 0;
				break;
			case GRAPH_ON:
				s->graphon = 1;
				break;
			case GRAPH_OFF:
				s->graphon = 0;
				break;
			case REPORT_ON:
				s->reporton = 1;
				break;
			case REPORT_OFF:
				s->reporton = 0;
				break;
			default:
				{
#if 0
				char errstring[80];

				sprintf(errstring, 
					"Illegal behavior selector = %d in object %s at time %ld\n",
					m.select.code, s->myself, now);
				grid_error(s, errstring);
#endif
				break;
				}
			}
		}
	post_process(s);
	}                       

/************************************************************************
 *                                                                      *
 *                       P R E    P R O C E S S                         *
 *                                                                      *
 ************************************************************************/
/*static*/ int pre_process(s)
state *s;
   {
   int i, length, nblanks;
	char report[100], temp[50];

	s->number = numMsgs;
   if (s->myself[0] == '\0') /* then this is first time object has ran */
      {
		/*strcpy(s->myself, myName);*/ myName(s->myself);
    	if (s->myself[0] == 'r' or s->myself[0] == 'R')
			{
			s->side = RED;
			}
		else
			{
			s->side = BLUE;
			}
		}
	for (i = 0; i < UNITS_PER_GRID; i++)
		{
		s->Units[i].changed_v = FALSE;
		}
	if (s->been_inited == FALSE) initialize(s);
#  if 0
	sprintf(report, "Object %s", s->myself);
   length = strlen(report);
	sprintf(report, "\n\nObject %-20s\tSimulation Time %ld\n", 
		s->myself, now);
	for (i = 0; i < length; i++)
		strcat(report, "-");
	nblanks = (29-length > 0 ? 29-length : 0) ;
	sprintf(temp, "%s\t--------------------", &(s->blanks[29-nblanks]));
	strcat(report, temp);
	rprintf("%s", report);
#  endif
	}

/************************************************************************
 *                                                                      *
 *                     P O S T    P R O C E S S                         *
 *                                                                      *
 ************************************************************************/

/*static*/ int post_process(s)
state *s;
	{
	look_for_deletions(s);
	solve_collisions(s);
	write_pad_commands(s);
	}

/*
 * The solve_collisions routine looks for the following two types of events
 * at each object in the grid:
 * (1) it looks for collisions with other objects;
 * (2) it looks for objects exiting the grid.
 * 
 * If it finds events of either type, then it sends an evaluate message
 * to ITSELF or a perception or change_location message to the unit. Which
 * it sends depends on how far in the future the collision or exit will
 * occur.  If the collision or exit occurs in the immediate future (i.e. 1
 * time step in the future) then the message goes to the unit. If the event
 * is farther in the future (i.e. two or more time stes away) then the
 * message goes to the grid object (myself).  We do the latter because the
 * unit might change velocity or be destroyed before the collision or 
 * exit occurs, and we want to avoid sending cancellation message. See FPW
 * if this explanation is fuzzy.
 */

/*static*/ int solve_collisions(s)
state *s;
   {
   int i, result, status;

   tprintf("\n\n*****%s @ %ld : Post process*****\n", s->myself, now);
	rprintf("\npost process:%.7s", s->blanks);
  /*
	* (1) Look for collisions with other objects:
	*
   * Loop over all units.  If unit A collides with B, then the collision 
   * will be detected twice; once when the loop counter is set to unit A
   * and once when it is set to unit B.  
   */
   for (i = 0; i < UNITS_PER_GRID and s->Units[i].UnitName[0] != '\0'; i++ ) 
  		{
		rprintf("unit %s: \n%.20s", 
			s->Units[i].UnitName, s->blanks);
		if (s->traceon == TRUE)
			{
			tprintf("UnitName[%d] = %s\n", i, s->Units[i].UnitName);
			tprintf("Pos = (%lf, %lf) vel = (%lf, %lf) @t = %ld\n", 
				s->Units[i].where.pos.x,
				s->Units[i].where.pos.y, s->Units[i].where.vel.x,
				s->Units[i].where.vel.y, s->Units[i].time);
			if (s->Units[i].time < now)
				{
				Particle p1;
				
				whereis(
					s->Units[i].where,
				   ( now-s->Units[i].time )/SIM_TIME_SCALE,
					&p1
				);
				tprintf("Pos = (%lf, %lf) vel =(%lf, %lf) right now\n",
					p1.pos.x, p1.pos.y, p1.vel.x, p1.vel.y);
				}
			}
      unmark_perceive(s, i); /* initialize flags in 'perceive' array */
     /*
      * Check for the following conditions:
		* (1) collision of object i with other objects (other_object_collision)
		* (2) collision of object i with cell boundary (cell_boundary_collision)
		* (3) center of object i leaving the cell (center_exit).
      */

		status = other_object_collision(s, i);
		if (status == FAILURE)
			{
			rprintf("No collisions with other objects\n%.20s", s->blanks);
			}
		else
			{
			rprintf("Collisions detected\n%.20s", s->blanks);
			}
		if (cell_boundary_collision(s, i) == FAILURE)
			{
			rprintf("No collisions with cell boundaries detected\n%.20s",
				s->blanks);
		  /*
			* The following routine is not needed for STB88-2, because
			* all objects stay within the same grid cell.
			*/
			/* center_grid_exit(s, i); */
			}
		else
			{
			rprintf("Collisions with cell boundaries found\n%.20s", s->blanks);
			}
     /*
		* Check to see if there's anyone within the unit's perception radius, 
		* in which case we must send a 'perceive' message. Also check if
		* anyone who is in the perception radius has changed its velocity.
		*/
		check_perception_radius(s, i);
	  /*
		* Check to see if we can delete anything from the perception ADT.
		*/
		check_perceive(s, i);
		}
	}

/*static*/ int other_object_collision(s, i)
state *s;
int i;  /* index of object you're considering */
	{
	Output_message o;
	CollInfo info[5];
	int j, ih, index;
	Simulation_time time;
	
   collide(s, i, &(info[0]));
	for (j = 0; 
		  info[0].num_simultaneous > 0 and j < info[0].num_simultaneous;
		  j++)
		{
		index = info[j].indx;
		rprintf("Collision of %s with %s at %ld, exit at %ld\n%.20s", 
			s->Units[i].UnitName, s->Units[index].UnitName, info[j].time, 
			info[j].time2, s->blanks);
     /*
      * If (unit i is colliding now or at the next time step, and
		*     we have not already sent it a perception message), 
		*     then send unit 'i' a PERCEIVE message.
      */
		if ((info[j].time == now + SIM_TIME_SCALE  or info[j].time == now) and 
			 search_perceive(s, i, s->Units[index].UnitName) == -1) 
			{
 			o.select.code = PERCEIVE;
			strcpy(o.p.UnitName, s->Units[index].UnitName);
			whereis(	s->Units[index].where,
					   ( SIM_TIME_SCALE + now - s->Units[index].time)/SIM_TIME_SCALE,
						&o.p.where
					  );
			o.p.time = now+SIM_TIME_SCALE;
			tell(s->Units[i].UnitName, now+SIM_TIME_SCALE, 0, sizeof(o.p), &o);
			/* add_hist(s, PERCEIVE, i); */
			add_perceive(s, i, s->Units[index].UnitName);
			rprintf("sent %s to %s at %4ld about unit %s\n%.20s", "PERCEIVE", 
				s->Units[i].UnitName, now+SIM_TIME_SCALE, s->Units[index].UnitName,
				s->blanks);
			}
     /*
      * If unit i is colliding at a time less than or equal to now,
		* don't do anything! The PERCEIVE message has already been sent,
		* and you sure do not want to send an EVALUATE message to yourself
		* for an earlier time!
      */
      else if (info[j].time <= now + SIM_TIME_SCALE )
			{
			tprintf("Collision occuring <= now; no action taken.\n");
			}
     /*
      * If we get here, unit i is colliding 2 or more units of time in
      * the future.  Send a message to myself one minute BEFORE the
      * collision is to take place, in order to see if the collision
      * really does take place (i.e. velocities have not changed). 
  		*/
		else
			{
			if (search_eval(s, info[j].time-SIM_TIME_SCALE) != SUCCESS)
				{
				add_eval(s, info[j].time-SIM_TIME_SCALE);
				o.select.code = EVALUATE;
				tell(
						s->myself, 
						info[j].time-SIM_TIME_SCALE,
						0, 
						sizeof(o.select), 
						&o
					 );
			   rprintf("sent %s to %s at %4ld\n%.20s", "EVALUATE", s->myself, 
					info[j].time-SIM_TIME_SCALE, s->blanks);
				}
		  /*
			* info[j].time2 is the exit time of the collision. We will
			* schedule an evaluate behavior only if none has already been
			* scheduled between now and time2.
			*/
			if (next_eval(s) > info[j].time2-SIM_TIME_SCALE)
				{
				add_eval(s, info[j].time2-SIM_TIME_SCALE);
				o.select.code = EVALUATE;
				tell(
						s->myself, 
						info[j].time2-SIM_TIME_SCALE, 
						0, 
						sizeof(o.select), 
						&o
					 );
			   rprintf("sent %s to %s at %4ld\n%.20s", "EVALUATE", s->myself, 
					info[j].time2-SIM_TIME_SCALE, s->blanks);
				}
			}
		}
	if (info[0].num_simultaneous == 0)
		{
		tprintf("No collisions detected for object %s\n", s->Units[i].UnitName);
		return FAILURE;
		}
	else return SUCCESS;
	}

/*static*/ int cell_boundary_collision(s, i)
state *s;
int i;
   {
	Output_message o;
	ExitInfo info[3];
	int j, result;
	Simulation_time time;
	
  /*
   * (2) Look for objects exiting the grid.
   */
	grid_exit(s, i, &(info[0]));
	j = 0;
	result = FAILURE;
	for (j = 0; j < info[0].num_simultaneous; j++) 
		{
	  /*
		* If the addTime is now + SIM_TIME_SCALE, then we will 'commit' the movement,
		* which means we must:  
		* (1) check to see if the unit will move off the board.  If so,
		*	   we must stop the unit.
		* (2) send an add_unit message to the new grid cell.
		* (3) send a del_unit message to myself when the unit is more than
		*		'pradius' distance units away (note: this might be handled
		* 		in post_process without an explicit message).
		* (4) determine when the center of the unit will move out, and send
		*		an EVALUATE message at that time.
		*
		* Screwy logic: The EVALUATE to check for the delete unit message
		* is sent by the transfer_unit() function, which is called below.
		*/
		if (info[j].addTime == now + SIM_TIME_SCALE )
			{
			result = SUCCESS;
			if (info[j].Grid[0] == '\0') /* unit leaving gameboard */
				{
				halt_unit(s, i);
				}
			else /* unit entering another grid */
				{
				transfer_unit(s, i, info[j].Grid);
				tprintf("...sending ADD_UNIT to %s at %ld\n", 
					info[j].Grid, now+SIM_TIME_SCALE);
				}
			}
	  /*
		* if the exit time is now, we must do an extra check to see if
		* we've already sent an ADD_UNIT message. Better software engineering
		* would keep us from these two mostly-redundant if statements, the
		* if (info... == now + SIM_TIME_SCALE) and if (info... == now).
		*/
		else if (info[j].addTime == now)
			{
		  /*
			* . . . search the history array to see if you've already
			* sent an ADD_UNIT.  If not, then send an ADD_UNIT message.
			* This condition can arise if a unit begins right next to
			* a border and then moves out.
			*/
			result = SUCCESS;
			if ((time = search_hist(s, ADD_UNIT, i)) == 0  or
				  time < now+SIM_TIME_SCALE )
				{
				if (info[j].Grid[0] == '\0') halt_unit(s, i);
				else /* unit entering another grid */
					{
					transfer_unit(s, i, info[j].Grid);
					tprintf("...sending ADD_UNIT to %s at %ld\n", 
						info[j].Grid, now+SIM_TIME_SCALE);
					}
				}
			}
	  /*
		* If the addTime is less than or equal to NOW, then we will ignore
		* this event; it is an anomaly.
		*/
		else if (info[j].addTime == now or info[j].addTime < now)
			{
		   tprintf("Exit lies in past; no action taken now\n");
			result = FAILURE;
			continue;	
			}
	  /*
		* If the exit time is more than one unit in the future, we
		* will not schedule it yet; instead, we will schedule an 'evaluate'
		* later to see if the exit will really happen.
		*/
		else
			{	
			result = SUCCESS;
			tprintf("Unit %s leaving for grid |%s| at %ld\n", 
				s->Units[i].UnitName,
			  	info[j].Grid, info[j].addTime);
		  /*
			* Send an EVALUATE message if you have not already done
			* so.
			*/
			if (search_eval(s, info[j].addTime - SIM_TIME_SCALE) != SUCCESS)
				{
				tprintf("EVALUATE message sent to myself for %ld\n", 
					info[j].addTime - SIM_TIME_SCALE);
				add_eval(s, info[j].addTime - SIM_TIME_SCALE);
				o.select.code = EVALUATE;
				tell(
						s->myself, 
						info[j].addTime-SIM_TIME_SCALE, 
						0, 
						sizeof(o.select), 
						&o
					 );
				rprintf("sent %s to %s at %4ld\n%.20s", "EVALUATE", s->myself,
					info[j].addTime-SIM_TIME_SCALE, s->blanks);
				}
			else
				{
				rprintf("already sent EVALUATE to %s at %ld\n%.20s",
					s->myself, info[j].addTime-SIM_TIME_SCALE, s->blanks);
				}
			}
		}
	if (info[0].num_simultaneous == 0)
		{
		tprintf("Unit %s will not exit this grid cell\n",
			s->Units[i].UnitName);
		}
	return result;
	}

/*static*/ int halt_unit(s, i)
state *s;
int i;  /* unit index */
	{
	Output_message o;
	int ih;

  /*
	* Only send a halt if the unit's velocity is nonzero
	*/
	if (magnitude(s->Units[i].where.vel) == 0.0) return;
	tprintf("Unit %s leaving gameboard\n", s->Units[i].UnitName);
	tprintf("Sending HALT to unit %s at %ld\n", s->Units[i].UnitName,
		now+SIM_TIME_SCALE);
	o.select.code = HALT;
	tell(
			s->Units[i].UnitName, 
			now+SIM_TIME_SCALE, 
			0, 
			sizeof(o.select), 
			&o
		 );
	/* add_hist(s, HALT, i); */
	rprintf("sent %s to %s at %4ld\n%.20s", "HALT", 
		s->Units[i].UnitName,now+SIM_TIME_SCALE, s->blanks);
	}

/*static*/ int transfer_unit(s, i, whichGrid)
state *s;
int i;
Name_object whichGrid;
	{
	Output_message o;
	Simulation_time time;
	int ih;

  /*
	* send ADD_UNIT message to new grid cell for now + SIM_TIME_SCALE 
	*/
	add_unit_msg(s, i, &o);
	tell(
			whichGrid, 
			now+SIM_TIME_SCALE, 
			0, 
			sizeof(o.add), 
			&o);
	add_gridloc(s, i, whichGrid);
	rprintf("sent %s to %s at %4ld, unit %s\n%.20s", "ADD_UNIT", 
		whichGrid, now+SIM_TIME_SCALE, o.add.UnitName, s->blanks);
  /*
	* Predict when the center will leave the grid cell. Send
	* an EVALUATE message to myself at that time; the EVALUATE
	* should fire the center_exit routine below.
	*/
	center_exit_time(s, i, &time);
	if (time > 0  && search_eval(s, now + time - SIM_TIME_SCALE) != SUCCESS )
		{
		tprintf("sending EVALUATE message to check for center exit at %ld\n",
			now + time);
		add_eval(s, now + time - SIM_TIME_SCALE);
		o.select.code = EVALUATE;
		tell(s->myself, now + time - SIM_TIME_SCALE, 0, sizeof(o.select), &o);
		rprintf("sent %s to %s at %4ld\n%.20s", "EVALUATE", 
			s->myself, now+time, s->blanks);
		}
	else if (time <= 0)
		{
		if (magnitude(s->Units[i].where.vel) != 0.0)
			{
#if 0
			char error[160];
			sprintf(error, "routine transfer_unit speed is nonzero; no center exit!\ncenter exit time=%ld",
				time);
			grid_error(s, error);
#endif
			}
		tprintf("Already sent EVALUATE for %s's center exit at time %ld\n",
			s->Units[i].UnitName, now+time-SIM_TIME_SCALE);
		}
	}

/*static*/ int add_unit_msg(s, i, o)
state *s;
int i;
Output_message *o;
	{
	(*o).select.code = ADD_UNIT;
	strcpy((*o).add.UnitName, s->Units[i].UnitName);
  /*
	* Since the new grid cell will get the ADD_UNIT message one
	* minute in the future, we must set (*o).add.where to the particle's
	* position one minute from now.
	*/
	whereis(
				s->Units[i].where, 
				(SIM_TIME_SCALE + now - s->Units[i].time)/SIM_TIME_SCALE, 
				&o->add.where
			  );
	(*o).add.pradius = s->Units[i].pradius;
	tprintf("transfer_unit: PRADIUS %d = %d\n", (*o).add.pradius, 
		s->Units[i].pradius);
	add_hist(s, ADD_UNIT, i);
	}

/*
 * center_exit_time calculates the time at which the center of the
 * unit will pierce one of the grid cell's boundaries. It callse
 * the utility function fast_inexact_line_collision to calculate the
 * collision of a point (circle with zero radius) with a line. That
 * function returns a floating point number for the collision time,
 * which must be rounded up to the nearest integer, because the function
 * which calls center_exit_time subtracts 1 from the result and schedules
 * a message then.  Thus, if a unit is going to exit at time 5.5, we
 * round the time to 6.0, return it to the calling function, which then
 * schedules an evaluate for 5.0 units from now.
 */
/*static*/ int center_exit_time(s, i, time)
state *s;
int i;
Simulation_time *time;
	{
	Particle p;
	Line l;
	Answer collision;
	int j, b;

 	*time = POSINF;
	whereis(
				s->Units[i].where,
				( now - s->Units[i].time)/SIM_TIME_SCALE, 
				&p
			 );
	for (j = 0, b = -1; j < 4; j++)
		{
		l = s->Neighbors[j].boundary;
		fast_inexact_line_collision(p, l, 0, &collision);
		if (collision.yesno == YES and
			 (Simulation_time)collision.time < ( *time /SIM_TIME_SCALE ) and
			 (Simulation_time)(ceil(collision.time)) > 0 )
			{
			*time =  (Simulation_time)( (ceil(collision.time)) * SIM_TIME_SCALE);
			b = j;
			}
		}
  /*
	* if (there is no collision) set time = NEGINF to signal that to
	* calling routine (transfer_unit).
	*/
	if (*time == POSINF) *time = NEGINF;
	tprintf("Center exit time is %ld, with bound %d\n", 
		now + *time, b);
	}

/*static*/ int center_grid_exit(s, i)
state *s;
int i;  /* unit index */
	{
	Particle p, p1;
	Name_object NewGrid;
	Output_message o;
	Simulation_time time, When;
	int ih;

   tprintf("center_exit starting: s->Units[%d].GridLoc = %s\n", 
		i, s->Units[i].GridLocs[0]);
  /*
   * Particle may actually cross boundary line at some time between
   * now and now+SIM_TIME_SCALE.  Thus we will check exit conditions for particle
   * position at now and now+SIM_TIME_SCALE.
	*/
	whereis(
				s->Units[i].where, 
				(now - s->Units[i].time)/SIM_TIME_SCALE, 
				&p
			  );
	whereis(
				s->Units[i].where, 
				( SIM_TIME_SCALE + now - s->Units[i].time)/SIM_TIME_SCALE, 
				&p1
			 );
	tprintf("center_exit: p.pos.x=%lf, p.pos.y=%lf\n", p.pos.x, p.pos.y);
	if (
      /* check position at now: */
 	    p.pos.x >= s->corner[SE].x or p.pos.x <= s->corner[SW].x or
		 p.pos.y >= s->corner[NE].y or p.pos.y <= s->corner[SE].y 	or
      /* check position at now + SIM_TIME_SCALE: */
 	    p1.pos.x >= s->corner[SE].x or p1.pos.x <= s->corner[SW].x or
		 p1.pos.y >= s->corner[NE].y or p1.pos.y <= s->corner[SE].y  	
		)
		{
		which_grid(s, NewGrid, p);
     /*
		* If NewGrid is NULL, then we are at the edge of the gameboard
		* and we must send a halt unit message to the object.
		*/
		if (NewGrid[0] == '\0')
			{
			halt_unit(s, i);
			return;
			}
		rprintf("Center exiting at time %d to grid %s\n%.20s", now+SIM_TIME_SCALE,
			NewGrid, s->blanks);
	  /*
		* If the following test is TRUE, then we have already sent a
		* CHANGE_LOCATION message to the new grid.
		*/
		if ((When = search_hist(s, CHANGE_LOCATION, i)) == now+SIM_TIME_SCALE)
			{
			tprintf("Already sent a change location msg to %s\n",
				s->Units[i].UnitName);
			return;
			}
	  /*
		* If you have not already sent an add unit message, then sent
		* one!  Note: you might not have sent one if the unit's pradius
		* extends to a neighboring grid, and the unit has just changed
		* its velocity from 0 to nonzero.
		*/
		if (search_hist(s, ADD_UNIT, i) == 0L) 
			{
			add_unit_msg(s, i, &o);
			tell(NewGrid, now + SIM_TIME_SCALE, 0, sizeof(o.add), &o);
			}
	  /*
	   * send CHANGE_LOCATION message to unit for now +SIM_TIME_SCALE 
	   */
	   tprintf("...sending CHANGE_LOCATION to %s at %ld; new loc = %s\n",
			s->Units[i].UnitName, now+SIM_TIME_SCALE, NewGrid);
		o.select.code = CHANGE_LOCATION;
		strcpy(o.change.whereto, NewGrid);
		tell(s->Units[i].UnitName, now+SIM_TIME_SCALE, 0, sizeof(o.change), &o);
		add_hist(s, CHANGE_LOCATION, i);
		rprintf("sent %s to %s at %4ld, new loc = %s\n%.20s", 
			"CHANGE_LOCATION", s->Units[i].UnitName, now+SIM_TIME_SCALE,
			NewGrid, s->blanks);
		clear(&o, sizeof(o));
	  /*
		* Determine when trailing edge (tail) of unit will leave the
		* Grid, and schedule a delete unit for that time. 
		* If we get a change_velocity message between now and the
		* tail exit time, then we will forward the message to all the
		* grid objects listed in the GridLocs ADT for this unit in the
		* state.
		*/
		tail_exit_time(s, i, &time);
		if (time > 0 and time < POSINF)
			{
			tprintf("...sending DELETE to myself at %ld\n", now + time);
		   o.select.code = DELETE_UNIT;
			strcpy(o.del.UnitName, s->Units[i].UnitName);
			tell(s->myself, now + time, 0, sizeof(o.del), &o);
			/* add_hist(s, DELETE_UNIT, i); */
			rprintf("sent %s to %s at %4ld, unit %s\n%.20s", 
				"DELETE_UNIT", s->myself, 
				now+time, o.del.UnitName, s->blanks);
			clear(&o, sizeof(o));
			}
		}
	else
		{
		rprintf("No center exit detected.\n%.20s", s->blanks);
		tprintf("Center exit failed. Position dump:\n");
		tprintf(" At now, pos = (%.3lf, %.3lf), vel = (%.3lf, %.3lf)\n",
			p.pos.x, p.pos.y, p.vel.x, p.vel.y);
		tprintf(" At now+SIM_TIME_SCALE, pos=(%.3lf, %.3lf), vel = (%.3lf, %.3lf)\n",
			p1.pos.x, p1.pos.y, p1.vel.x, p1.vel.y);
		tprintf("Last saved pos = (%.3lf, %.3lf) at time %ld\n",
			s->Units[i].where.pos.x, s->Units[i].where.pos.y, s->Units[i].time);
		}
	}

/*static*/ int tail_exit_time(s, i, time)
state *s;
int i;
Simulation_time *time;
	{
	Particle p;
	Line l;
	Answer collision;
	int j;

 	*time = POSINF;
	whereis(
				s->Units[i].where, 
				( now - s->Units[i].time )/SIM_TIME_SCALE , 
				&p
			  );
	for (j = 0; j < 4; j++)
		{
		l = s->Neighbors[j].boundary;
		fast_inexact_line_collision(p, l, s->Units[i].pradius, &collision);
		if (collision.yesno == YES)
			{
			if (  collision.time < 0 and
				   collision.time2 < 0
				)
				{
					 continue;
				}
			if (  collision.time2 > 0 and 
					collision.time2 < ( *time / SIM_TIME_SCALE)
				)
				{
				*time = ( collision.time2 *SIM_TIME_SCALE );
				}
			}
		}
	tprintf("Tail exit time is %ld\n", *time);
	}

/* 
 * The following function determines which grid the particle is now in.
 */
/*static*/ int which_grid(s, NewGrid, p)
state *s;
Name_object NewGrid;
Particle p;
	{
	Particle p1;
	Name_object GridName;

	whereis(p, SIM_TIME_SCALE, &p1);
	if (p1.pos.x > s->corner[NE].x and p1.pos.y > s->corner[NE].y)
		{
		diagonal_neighbor(s, GridName, NE);
		strcpy(NewGrid, GridName);
		}
	else if (p1.pos.x >= s->corner[SE].x and p1.pos.y < s->corner[NE].y and
		p1.pos.y > s->corner[SE].y)
		strcpy(NewGrid, s->Neighbors[EAST].name);
	else if (p1.pos.x > s->corner[SE].x and p1.pos.y < s->corner[SE].y)
		{
		diagonal_neighbor(s, GridName, SE);
		strcpy(NewGrid, GridName);
		}
	else if (p1.pos.y <= s->corner[SW].y and p1.pos.x > s->corner[SW].x and
		p1.pos.x < s->corner[SE].x)
		strcpy(NewGrid, s->Neighbors[SOUTH].name);
	else if (p1.pos.x < s->corner[SW].x and p1.pos.y < s->corner[SW].y)
		{
		diagonal_neighbor(s, GridName, SW);
		strcpy(NewGrid, GridName);
		}
	else if (p1.pos.x <= s->corner[NW].x and p1.pos.y > s->corner[SW].y and
		p1.pos.y < s->corner[NW].y)
		strcpy(NewGrid, s->Neighbors[WEST].name);
	else if (p1.pos.x < s->corner[NW].x and p1.pos.y > s->corner[NW].y)
		{
		diagonal_neighbor(s, GridName, NW);
		strcpy(NewGrid, GridName);
		}
	else if (p1.pos.x > s->corner[NW].x and p1.pos.x < s->corner[NE].x and
		p1.pos.y >= s->corner[NW].y)
		strcpy(NewGrid, s->Neighbors[NORTH].name);
	else if (p1.pos.x == s->corner[NW].x and p1.pos.y == s->corner[NW].y)
		{
		if (p1.vel.x == 0 and p1.vel.y > 0)
			strcpy(NewGrid, s->Neighbors[NORTH].name);
		else if (p1.vel.x < 0 and p1.vel.y == 0)
			strcpy(NewGrid, s->Neighbors[WEST].name);
		else if (p1.vel.x < 0 and p1.vel.y > 0)
			{
			diagonal_neighbor(s, GridName, NW);
			strcpy(NewGrid, GridName);	
			}
		}
	else if (p1.pos.x == s->corner[SW].x and p1.pos.y == s->corner[SW].y) 
		{
		if (p1.vel.x == 0 and p1.vel.y < 0)
			strcpy(NewGrid, s->Neighbors[SOUTH].name);
		else if (p1.vel.x < 0 and p1.vel.y == 0)
			strcpy(NewGrid, s->Neighbors[WEST].name);
		else if (p1.vel.x < 0 and p1.vel.y < 0)
			{
			diagonal_neighbor(s, GridName, SW);
			strcpy(NewGrid, GridName);	
			}
		}
	else if (p1.pos.x == s->corner[NE].x and p1.pos.y == s->corner[NE].y)
		{
		if (p1.vel.x > 0 and p1.vel.y == 0)
			strcpy(NewGrid, s->Neighbors[EAST].name);
		else if (p1.vel.x == 0 and p1.vel.y > 0)
			strcpy(NewGrid, s->Neighbors[NORTH].name);
		else if (p1.vel.x > 0 and p1.vel.y > 0)
			{
			diagonal_neighbor(s, GridName, NE);
			strcpy(NewGrid, GridName);
			}
		}
	else if (p1.pos.x == s->corner[SE].x and p1.pos.y == s->corner[SE].y)
		{
		if (p1.vel.x > 0 and p1.vel.y == 0)
			strcpy(NewGrid, s->Neighbors[EAST].name);
		else if (p1.vel.x == 0 and p1.vel.y < 0)
			strcpy(NewGrid, s->Neighbors[SOUTH].name);
		else if (p1.vel.x > 0 and p1.vel.y < 0)
			{
			diagonal_neighbor(s, GridName, SE);
			strcpy(NewGrid, GridName);
			}
		}	
	else
		{
		char error[80];

		grid_error(s, "routine which_grid hit exception condition.");
#		if 0
		printf("CTLS runtime debugger has detected a coding error.\n");
		printf("Object %s routine which_grid hit exception condition.\n",
			s->myself);
		printf("pos=(%lf, %lf)\n     NE corn=(%lf,%lf), SE corn=(%lf,%lf)\n",
			p1.pos.x, p1.pos.y, s->corner[NE].x, s->corner[NE].y,
			s->corner[SE].x, s->corner[SE].y);
		printf("     NW corn=(%lf,%lf), SW corn=(%lf, %lf)\n",
			s->corner[NW].x, s->corner[NW].y, s->corner[SW].x,
			s->corner[SW].y);
		printf("Fatal error. Will exit routine with garbage return value.\n");
#		endif
		}
	tprintf("unit will exit to grid %s\n", NewGrid);
	}

/*
 * 'collide' does the following things:
 * Its input is s (a pointer to the state) and index1, which is the 
 * index in the 'Units' array in the state of the unit for which
 * you want to calculate a collision. The 'collide' function loops
 * through all the remaining units in the 'Units' array and returns
 * SUCCESS if there is a collision, FAILURE otherwise.  If SUCCESS
 * is returned, then 'collisionTime' is set to be the time of the
 * collision, 'otherObject' is the name of the other object which
 * you will collide with, and 'index2' is the index element of
 * otherObject in the 'Units' array in the state. If there are
 * multiple collisions, 'collide' will return only the earliest
 * collision it finds.  If there are collisions with multiple objects
 * at 'collisionTime,' then only one of the colliding objects will
 * be returned (this condition is ill-defined at present).
 */
/*static*/ int collide(s, index1, info)
state *s;
int index1;
CollInfo info[];
	{
	Particle p1, p2;
   Answer collision;
   int i, result, counter = 0;
	int t_of_collision, t_of_collision2;

	info[0].time = POSINF;
	info[0].num_simultaneous = 0;
   whereis(
				s->Units[index1].where,
				( now - s->Units[index1].time )/SIM_TIME_SCALE, 
				&p1
			 );

   for (i = 0; i < UNITS_PER_GRID and s->Units[i].UnitName[0] != '\0';
		  i++)
		{
	  /*
 	   * Skip unit at index1.
		*/
		if (i == index1) continue;
		whereis(
					s->Units[i].where, 
					( now - s->Units[i].time )/SIM_TIME_SCALE, 
					&p2
				);
	  /*
		* Calculate time two units are 'pradius' distance units apart.
		*/
		inexact_particle_collision(p1, p2, s->Units[index1].pradius, &collision);
		if (collision.yesno == YES) 
			{
			tprintf("collision.yesno = YES, collision.time=%ld\n", collision.time);
			t_of_collision = ( (int)(ceil(collision.time)) * SIM_TIME_SCALE);
			t_of_collision2 =( (int)(ceil(collision.time2)) * SIM_TIME_SCALE);
		  /*
			* Note that with the following if-test, it is possible to return
			* a time of zero, meaning that the two units are colliding NOW.
			* The function which calls this routine must be capable of handling
 			* a zero collision time.
			*/
			if ((Simulation_time) t_of_collision + now < info[0].time and 
			    (Simulation_time) t_of_collision >= 0)
				{	
				counter = 0;
				info[0].num_simultaneous = 1;
				info[0].time = (Simulation_time) t_of_collision + now;
				info[0].time2 = (Simulation_time) t_of_collision2 + now;
				info[0].indx = i;
				clear(&(info[1]), sizeof(info[1]));
				}
			if (t_of_collision == info[0].time)
				{
				counter++;
				info[0].num_simultaneous++;
				info[counter].num_simultaneous = counter+1;
				info[counter].time = (Simulation_time) t_of_collision;
				info[counter].time2 = (Simulation_time) t_of_collision2 + now;
				info[counter].indx = i;
				}
			}
		else
			{
			tprintf("collision.yesno = NO\n");
			}
		}
	}

/*static*/ int grid_exit(s, index1, info)
state *s;
int index1;  /* index number of object you wish to look at */
ExitInfo info[];
	{
	int i, counter = 0, bound1, bound2, icorn;
	Answer collision;
 	Particle p, pcorn;
	Line l;

   whereis(
				s->Units[index1].where, 
				( now - s->Units[index1].time )/SIM_TIME_SCALE, 
				&p
			 );
   info[0].addTime = POSINF;
	info[0].num_simultaneous = 0;
  /*
	* Check only those boundaries lying in the direction the particle
 	* is moving.
	*/
	if (p.vel.x > 0) bound1 = EAST;
	else if (p.vel.x < 0) bound1 = WEST;
	else /* check to see if unit's on the edge */
		{
		if (p.pos.x == s->Neighbors[WEST].boundary.e1.x)
			bound1 = WEST;
		else if (p.pos.x == s->Neighbors[EAST].boundary.e1.x)
			bound1 = EAST;
		else bound1 = -1;
		}

	if (p.vel.y > 0) bound2 = NORTH;
	else if (p.vel.y < 0) bound2 = SOUTH;
	else /* check to see if unit's on the edge */
		{
		if (p.pos.y == s->Neighbors[NORTH].boundary.e1.y)
			bound2 = NORTH;
		else if (p.pos.y == s->Neighbors[SOUTH].boundary.e1.y)
			bound2 = SOUTH;
		else bound2 = -1;
		}
  /*
	* Loop through appropriate neighbors, and determine when and if there is a
	* collision.
	*/
   for (i = 0, counter = 0; i < 4; i++) /* loop through all neighbors */
		{
		if (i != bound1 and i != bound2) continue;
		l = s->Neighbors[i].boundary;
		fast_inexact_line_collision(p, l, s->Units[index1].pradius, &collision);
		if (collision.yesno == YES && collision.time > 0)
			{
			tprintf("raw returned time=%lf, time2=%lf\n", collision.time,
				collision.time2);
			info[counter].addTime = 
					((Simulation_time)collision.time * SIM_TIME_SCALE )+ now;
			info[counter].delTime = 
					((Simulation_time)collision.time2 * SIM_TIME_SCALE ) + now;
			info[0].num_simultaneous++; 
			strcpy(info[counter].Grid, s->Neighbors[i].name);
			counter++;
			}
		}
  /*
	* Check corner collision; if so, then diagonal grid cell must be
 	* added to 'info' structure.
	*/
	icorn = -1;
	if (bound1 == EAST and bound2 == NORTH) icorn = NE;
	else if (bound1 == EAST and bound2 == SOUTH) icorn = SE;
	else if (bound1 == WEST and bound2 == NORTH) icorn = NW;
	else if (bound1 == WEST and bound2 == SOUTH) icorn = SW;
	if (icorn != -1) 
		{
		pcorn.pos = s->corner[icorn];
		pcorn.vel.x = 0;
		pcorn.vel.y = 0;
		inexact_particle_collision(p,pcorn,s->Units[index1].pradius, &collision);
		tprintf("Collision with corner (%lf, %lf): collision.yesno = %d\n",
			pcorn.pos.x, pcorn.pos.y, collision.yesno);
		}
	if (icorn != -1 and collision.yesno == YES)
		{
		info[counter].addTime =
					 ((Simulation_time)collision.time * SIM_TIME_SCALE) + now;
		info[counter].delTime = 
					 ((Simulation_time)collision.time2 * SIM_TIME_SCALE)+ now;
		info[0].num_simultaneous++; 
		diagonal_neighbor(s, info[counter].Grid, icorn);
		counter++;
		}
  /*
	* Debugging and tracing stuff.
   */	
	if (info[0].addTime < POSINF)
		{
		tprintf("%s @%ld : unit %s will exit at time %ld to object %s\n",
			s->myself, now, s->Units[index1].UnitName, info[0].addTime,
			info[0].Grid);
		return SUCCESS;
		}
	else 
		{
		tprintf("%s @%ld : unit %s will not exit\n", s->myself, now,
			s->Units[index1].UnitName);
		return FAILURE;
		}
	}

/*static*/ int diagonal_neighbor(s, who, index)
state *s;
char *who;
int index;
	{
  /*
	* This routine determines the diagonal neighbors of the
	* current grid cell. Note that if the number of grid cells
	* along the x direction is given by NUM_EW_GRIDS, then they
	* are numbered G0,* through GN,* where N = NUM_EW_GRIDS-1.  
	* Thus we have a >= relation for each out-of-bounds test.
	*/
	switch (index)
		{
		case NE:
			if (s->gridx+1 >= NUM_EW_GRIDS or s->gridy+1 >= NUM_NS_GRIDS)
				sprintf(who, "\0");
			else sprintf(who, "G%d,%d", s->gridx+1, s->gridy+1);
			break;
		case SE:
			if (s->gridx+1 >= NUM_EW_GRIDS or s->gridy-1 < 0)
				sprintf(who, "\0");
			else sprintf(who, "G%d,%d", s->gridx+1, s->gridy-1);
			break;
		case NW:
			if (s->gridx-1 < 0 or s->gridy+1 >= NUM_NS_GRIDS)
				sprintf(who, "\0");
			else sprintf(who, "G%d,%d", s->gridx-1, s->gridy+1);
			break;
		case SW:
			if (s->gridx-1 < 0 or s->gridy-1 < 0) 
				sprintf(who, "\0");
			else sprintf(who, "G%d,%d", s->gridx-1, s->gridy-1);
			break;
		}
	}

/*
 * The 'eval' data structure is stored in the array s->scheduled_eval[]
 * in the state.  This data structure is a list of (future) times for
 * which EVALUATE events are scheduled.  Since the grid object schedules
 * EVALUATE events for itself, it can know with certainty at what future
 * times such events are scheduled.  Whenever the grid wants to scheduled
 * an eval event, it first searches the data structure (with search_eval)
 * to see if an event for that time is already scheduled; if so, it dooes
 * not reschedule the event. If the search fails, it schedules the event
 * and adds the time to the list (with add_eval). When an eval event
 * fires, it deletes the time from the list (with del_eval).
 */

/*static*/ int search_eval(s, time)
state *s;
Simulation_time time;
	{
	int i;

	for (i = 0; i < MAX_EVAL_ARRAY; i++)
		{
		if (s->scheduled_eval[i] == time) return SUCCESS;
		}
	return FAILURE;
	}

/*static*/ Simulation_time next_eval(s)
state *s;
	{
	int i;
	Simulation_time next_time;

   next_time = POSINF;
	for (i = 0; i < MAX_EVAL_ARRAY; i++)
		{
		if (s->scheduled_eval[i] > now and
			 s->scheduled_eval[i] < next_time) 
			 next_time = s->scheduled_eval[i];
		}
	return next_time;
	}

/*static*/ int add_eval(s, time)
state *s;
Simulation_time time;
	{
	int i;

	for (i = 0; i < MAX_EVAL_ARRAY; i++) 
		{
		if (s->scheduled_eval[i] == 0) 
			{
			s->scheduled_eval[i] = time;
			return SUCCESS;
			}
		}
	return FAILURE;
	}

/*static*/ int del_eval(s, time)
state *s;
Simulation_time time;
	{
	int i, j;
	
	for (i = 0; i < MAX_EVAL_ARRAY; i++)
		{
		if (s->scheduled_eval[i] <= now and s->scheduled_eval[i] != 0)
			{
			for (j = i; j < MAX_EVAL_ARRAY - 1; j++)
				{
				s->scheduled_eval[j] = s->scheduled_eval[j+1];
				}
			s->scheduled_eval[MAX_EVAL_ARRAY-1] = 0;
			}
		}
	return SUCCESS;
	}


	
/*
 * The following routines manipulate the 'history' array in the
 * unit array in the state.  The first routine finds the first blank
 * position in this array, and the second routine returns the time
 * of a given event.  If the event is not in the history array, then
 * the returned time is zero.
 */

int find_hist(s, unitIndex)
state *s;
int unitIndex;
	{
	int i;
	char error[80];

	for (i = 0; i < MAX_HIST_ARRAY; i++)
		{
		if (s->Units[unitIndex].past[i].whichEvent == 0)
			return i;
		else if (s->Units[unitIndex].past[i].When < now)
			return i;
		}
#if 0
	sprintf(error, "History struct for unit %s is too small (size is %d)",
		s->Units[unitIndex].UnitName, MAX_HIST_ARRAY);
	grid_error(s, error);
#endif 
	return 0;
	}

/*static*/ int add_hist(s, whichEvent, unitIndex)
state *s;
int whichEvent, unitIndex;
	{
	int i;
	
	i = find_hist(s, unitIndex);
	s->Units[unitIndex].past[i].When = now;
	s->Units[unitIndex].past[i].whichEvent = whichEvent;
	return i;
	}


Simulation_time search_hist(s, whichEvent, unitIndex)
state *s;
int whichEvent;
int unitIndex;
	{ 
	int i;
	Simulation_time result = 0L;

	for (i = 0; i < MAX_HIST_ARRAY; i++)
		{
		if (s->Units[unitIndex].past[i].whichEvent == whichEvent)
			result = s->Units[unitIndex].past[i].When;
		}
	return result;
	}

/*static*/ int write_pad_commands(s)
state *s;
	{
	char padstr[80];
	Vector l1, l2;
	Particle p;
	Vector old_circle;
	int i;
	
	if (s->enabled_grids == TRUE)
		{
		/* gprintf("\n"); */
		if (s->first_run == TRUE)
			{
			l1 = s->Neighbors[NORTH].boundary.e1;
			l2 = s->Neighbors[NORTH].boundary.e2;
			Tvector(padstr,  s->corner[SW].x/SCALE_FACTOR, 
				s->corner[SW].y/SCALE_FACTOR, s->corner[NW].x/SCALE_FACTOR, 
				s->corner[NW].y/SCALE_FACTOR);
			gprintf("pad %ld :%s",now, padstr);
			l1 = s->Neighbors[EAST].boundary.e1;
			l2 = s->Neighbors[EAST].boundary.e2;
			Tvector(padstr, s->corner[NW].x/SCALE_FACTOR, 
				s->corner[NW].y/SCALE_FACTOR, s->corner[NE].x/SCALE_FACTOR, 
				s->corner[NE].y/SCALE_FACTOR);
			gprintf("pad %ld :%s", now, padstr);
			l1 = s->Neighbors[SOUTH].boundary.e1;
			l2 = s->Neighbors[SOUTH].boundary.e2;
			Tvector(padstr, s->corner[NE].x/SCALE_FACTOR, 
				s->corner[NE].y/SCALE_FACTOR, s->corner[SE].x/SCALE_FACTOR, 
				s->corner[SE].y/SCALE_FACTOR);
			gprintf("pad %ld :%s", now, padstr);
			l1 = s->Neighbors[WEST].boundary.e1;
			l2 = s->Neighbors[WEST].boundary.e2;
			Tvector(padstr, s->corner[SE].x/SCALE_FACTOR, 
				s->corner[SE].y/SCALE_FACTOR, s->corner[SW].x/SCALE_FACTOR, 
				s->corner[SW].y/SCALE_FACTOR);
			gprintf("pad %ld :%s",now, padstr);
			s->first_run = FALSE;
			}
		}
	if (s->enabled_objs == TRUE)
		{
		for (i = 0; i < UNITS_PER_GRID and s->Units[i].UnitName[0] != '\0'; i++)
			{
			tprintf("Pad commands for unit %s\n", s->Units[i].UnitName);
			whereis(
						s->Units[i].where, 
						(now - s->Units[i].time)/SIM_TIME_SCALE, 
						&p
					 );
			old_circle = s->Units[i].circle;
			if (old_circle.x == p.pos.x and old_circle.y == p.pos.y) continue;
			if (old_circle.x != 0. and old_circle.y != 0.)
				{
				Terasecircle16(padstr, old_circle.x/SCALE_FACTOR,
					old_circle.y/SCALE_FACTOR,
					(double)s->Units[i].pradius/SCALE_FACTOR);
				gprintf("pad %ld :%s", now, padstr);
				}
			Tcircle16(padstr, p.pos.x/SCALE_FACTOR, 
				p.pos.y/SCALE_FACTOR, (double)s->Units[i].pradius/SCALE_FACTOR);
			gprintf("pad %ld :%s", now, padstr);
			Tbig_dot(padstr, p.pos.x/SCALE_FACTOR, p.pos.y/SCALE_FACTOR);
			/* gprintf("pad %ld :%s", now, padstr); */
		  /*
			* Save circle coordinates for erasure next time
			*/
			s->Units[i].circle.x = p.pos.x;
			s->Units[i].circle.y = p.pos.y;
			}
		}
	}
			
/*************************************************************************
 *                                                                       *
 *                           A D D    U N I T                            *
 *                                                                       *
 *************************************************************************/

/*   
 * Receives an add_unit message and updates the appropriate data structures.
 * NOTE: this routine assumes that the 'position' field in the add_unit message
 * is set up for time now, that is, the receive time of the add unit
 * message at the grid cell.  
 */
/*static*/ int add_unit(s, m, m_sender)
state *s;
Input_message *m;
Name_object m_sender;
	{
	int i, j;

  /*
	* If (the unit we're adding is already in the s->Units array in the
	*		state) 
	* Then
	*		(1) Delete the sender of the add unit message from the GridLocs
	*		    array in s->Units[xx].GridLoc, where xx=number returned from
	*			 Find. NOTE: This handles the exception condition where an
	*			 object sends an add_unit message to another object, and then
	*			 the unit changes velocity and the second object deletes
	*			 the unit from its state.
	*		(2) return, since there is no more work to do here.
	* else, find a blank position in the array and add the unit info.
	*/
   if ((i = Find(m->add.UnitName, &(s->Units[0]), sizeof(struct Units),
			UNITS_PER_GRID, String)) != -1) 
		{
		for (j = 0; j < 4; j++)
			{
			if (strcmp(s->Units[i].GridLocs[j], m_sender) == 0)
				strcpy(s->Units[i].GridLocs[j], "\0");
			}
		return;
		}
   i = find_blank(&(s->Units[0]), sizeof(struct Units), UNITS_PER_GRID,
			String);
	if (i == -1)
		{
#if 0
		char error[80];

		sprintf(error, "array s->Units[] dimension %d has overflowed\n",
			UNITS_PER_GRID);
		grid_error(s, error);
#endif
		return;	
		}

	strcpy(s->Units[i].UnitName, m->add.UnitName);
	s->Units[i].where = m->add.where;
	s->Units[i].time = now;
	tprintf("add_unit: PRADIUS %d = %d\n", s->Units[i].pradius, m->add.pradius);
	s->Units[i].pradius = m->add.pradius;
  /*
	* GridLoc in the Units array is an ADT keeping track of all the
	* grid objects which this grid object has sent an ADD_UNIT message to.
   * Thus it is a list of all the grid objects we must keep updated whenever
 	* a change_velocity message is received. Since we are starting anew,
	* we have not yet sent out any add_unit messages, thus the gridloc
	* array should be null. We will place our own name as the first element
	* of this list.
	*/
	strcpy(s->Units[i].GridLocs[0], s->myself);
  /*
	* Clear out all data structures
	*/
	for (j = 1; j < 4; j++)
		{
		strcpy(s->Units[i].GridLocs[j], "\0");
		}
	for (j = 0; j < MAX_PERCEIVE; j++)
		{
		s->Units[i].perceive[j].index = -1;
		}
	for (j = 0; j < MAX_HIST_ARRAY; j++)
		{
		clear(&(s->Units[i].past[j]), sizeof(History));
		}
	null_vector(&s->Units[i].circle);
	s->Units[i].changed_v = FALSE;
	s->Units[i].should_delete = FALSE;
	rprintf("unit %s", s->Units[i].UnitName);
	}

/*
 * find_unit searches the Units array in the state, and returns an
 * index to the unit or a -1 if the unit is not found.
 */
/*static*/ int find_unit(s, name)
state *s;
char *name;
	{
	int i, lowest;
	Simulation_time mintime;

   i = Find(name, &(s->Units[0]), sizeof(struct Units), 
			UNITS_PER_GRID, String);
	return i;
	}

/*
 * The following function is called from add_unit and checks to see
 * if the newly added unit is within one perception radius (pradius)
 * of any other unit in the grid.  If so, it informs the new unit of
 * the other unit's existence (via a PERCEIVE message).  It will also
 * inform other units of the new units existence if the new unit is
 * within another unit's pradius.  Note that the pradius for units A
 * and B might be different, so if A can perceive B, B might not be
 * able to perceive A.  This function correctly handles the exception
 * case where two units are initially placed within 'pradius' of each
 * other, but with velocity vectors that are parallel. In this case,
 * the 'collide()' routine will not schedule a perception message, 
 * because the two objects never collide.
 */
/*static*/ int check_perception_radius(s, indx)
state *s;
int indx;
	{
	int i, d, ih;
	Particle p1, p2;
	Output_message o;


	for (i = 0; i < UNITS_PER_GRID and s->Units[i].UnitName[0] != '\0';
			i++)
		{
		if (i == indx) continue;
		whereis(
					s->Units[i].where, 
					(now - s->Units[i].time)/SIM_TIME_SCALE, 
					&p1
				 );
		whereis(
					s->Units[indx].where, 
					(now - s->Units[indx].time)/SIM_TIME_SCALE, 
					&p2
				 );
		d = (int) distance(p1, p2);
		tprintf("pair %s-%s: distance %d\n", s->Units[i].UnitName,
			s->Units[indx].UnitName, d);
	  /*
		* If (unit 'i' is within the perception radius of unit 'indx') AND
		*    (we have not already sent unit 'indx' an intelligence message
		*	   about unit 'i' OR unit 'i' has changed its velocity)
		* Then:    
		* 		1. Send unit 'indx' a perception (intel) message.
		* 		2. Add the name of unit 'i' to the perception list in the
		*			Units array in the state.
		*/
		if (d < s->Units[indx].pradius and
			 (search_perceive(s, indx, s->Units[i].UnitName) == -1 or
			  s->Units[i].changed_v == TRUE))
			{
			o.select.code = PERCEIVE;
			strcpy(o.p.UnitName, s->Units[i].UnitName);
			whereis(
				s->Units[i].where, 
				( SIM_TIME_SCALE + now - s->Units[i].time)/SIM_TIME_SCALE, 
				&o.p.where
			);
			o.p.time = now+SIM_TIME_SCALE;
			tell(s->Units[indx].UnitName, now+SIM_TIME_SCALE, 0, sizeof(o.p), &o);
			add_perceive(s, indx, s->Units[i].UnitName);
			rprintf("sent %s to %s at %4ld about unit %s\n%.20s", "PERCEIVE", 
				s->Units[indx].UnitName, now+SIM_TIME_SCALE, s->Units[i].UnitName, 
				s->blanks);
			tprintf("Sent PERCEIVE to %s at %ld about %s, dist=%d, pradius=%d\n", 
				s->Units[indx].UnitName, now+SIM_TIME_SCALE, s->Units[i].UnitName, d,
				s->Units[indx].pradius);
			}
	  /*
		* If unit 'indx' can perceive unit 'i', but we have already sent
		* it a perception message, make note of it if trace print out 
		* (tprintf) is turned on.
		*/
		else if (d < s->Units[indx].pradius and
			 		search_perceive(s, indx, s->Units[i].UnitName) != -1)
			{
			tprintf("Unit %s is within %s's pradius, but a PERCEIVE has already been sent\n%.20s",
				s->Units[i].UnitName, s->Units[indx].UnitName, s->blanks);
			}
		}
	}

/*************************************************************************
 *                                                                       *
 *                         D E L    U N I T                              *
 *                                                                       *
 *************************************************************************/
/*static*/ int del_unit(s, m, stime)
state *s;
Input_message *m;
Simulation_time stime; /* send time of message */
	{
   int i;

   rprintf("unit %s", m->del.UnitName);
   if ((i = find_unit(s, m->del.UnitName)) == -1)
		{
#if 0
		char error[80];

		sprintf(error, "del_unit: Unit %s not found in s->Units[] table.\n");
		grid_error(s, error);
#endif
		return;
		}
  /*
	* Mark the unit for deletion now, and actually do the deletion
	* in the post_process routine.  This is necessary due to message
 	* ordering.  There was a case where a change velocity and a delete
 	* unit message, both for the same unit, was received at the same
	* virtual time.  If the delete unit goes first, then the change
	* velocity cannot find the unit and an error occurs.
	*/
	s->Units[i].should_delete = TRUE;
	}

/* 
 * The following function is called in the post process routine.
 */
/*static*/ int look_for_deletions(s)
state *s;
	{
	int i, j;
	Vector old_circle;
	char padstr[80];

	for (i = 0; i < UNITS_PER_GRID && s->Units[i].UnitName[0] != '\0'; i++)
		{
		if (s->Units[i].should_delete == TRUE)
			{
			old_circle = s->Units[i].circle;
			Terasecircle16(padstr, old_circle.x/SCALE_FACTOR,
				old_circle.y/SCALE_FACTOR,
				(double)s->Units[i].pradius/SCALE_FACTOR);
			gprintf("\npad %ld :%s", now, padstr);
			rprintf("\n%.20s", s->blanks);
			strcpy(s->Units[i].UnitName, "\0\0\0");
			/* arr_delete(s->Units[i].UnitName, &(s->Units[0]), sizeof(struct Units),
					UNITS_PER_GRID, String); */
			}
		}
	}

/*************************************************************************
 *                                                                       *
 *                           E V A L U A T E                             *
 *                                                                       *
 *************************************************************************/
/*static*/ int eval(s, m)
state *s;
Input_message *m;
	{
	del_eval(s, now);
	}

/*************************************************************************
 *                                                                       *
 *                    C H A N G E   V E L O C I T Y                      * 
 *                                                                       *
 *************************************************************************/
/*
 * Assumption here is that the change velocity message was sent one
 * moment ago.
 */
/*static*/ int change_v(s, m, fromwhom)
state *s;
Input_message *m;
Name_object fromwhom;
	{
   int i, j;

	i = find_unit(s, m->change.UnitName);
	if (strcmp(s->Units[i].UnitName, m->change.UnitName) != 0)
		{
#if 0
		char error[80];

		sprintf(error, "object %s not found in units array in state.",
			m->change.UnitName);
		grid_error(s, error);
#endif
		return;
		}
	s->Units[i].where = m->change.p;
	s->Units[i].pradius = m->change.pradius;
	s->Units[i].time = now-SIM_TIME_SCALE; /* technically, should be sendtime() */
	s->Units[i].changed_v = TRUE;
  /*
	* After updating the state, we must now check to see if we have to
	* forward this message to any other grid units.       
	* if (there's a nonempty string in GridLoc element j AND
	*     that nonempty string is not equal to my own name AND
	*     the change velocity message did not come from a Grid object
	* Then forward the message to the string in the jth element of the
	*     GridLocs table.
	*/
	for (j = 1; j < 4; j++)
		{
		if (strcmp(s->Units[i].GridLocs[j], "\0") != 0 and
			 strcmp(s->Units[i].GridLocs[j], s->myself) != 0 and
			 fromwhom[0] != 'G' )
			{
			tell(s->Units[i].GridLocs[j], now+SIM_TIME_SCALE, 0, sizeof(m->change), m);
			tprintf("Forwarded Change_velocity from unit %s to grid cell %s\n",
				s->Units[i].UnitName, s->Units[i].GridLocs[j]);
			}
		}
  /*
	* NOTE: the new velocity is sent to units who can perceive this
	* unit in the routine check_perceive.
	*/
	tprintf("Changed vector: (%lf, %lf, %lf, %lf) @time %ld\n",
		s->Units[i].where.pos.x, s->Units[i].where.pos.y,
		s->Units[i].where.vel.x, s->Units[i].where.vel.y,
		s->Units[i].time);
	rprintf("\tunit %s, pos = (%.2lf, %.2lf), vel = (%.2lf, %.2lf)", 
		m->change.UnitName, s->Units[i].where.pos.x, s->Units[i].where.pos.y,
		s->Units[i].where.vel.x, s->Units[i].where.vel.y);
	}

/*************************************************************************
 *                                                                       *
 *                          I N I T I A L I Z E                          *
 *                                                                       *
 *************************************************************************/
/*static*/ int initialize(s)
state *s;
	{
   int y_delta, x_delta, x_gridnum, y_gridnum, sindex, x, y;
	Vector null_vector;
	Line null_line;
	char result[40];

   tprintf("Initializing grid %s. . .\n", s->myself);
   s->been_inited = TRUE;
 	null_vector.x = 0.;
	null_vector.y = 0.;
	null_line.e1 = null_vector;
	null_line.e2 = null_vector;

   x_gridnum = atoi(&s->myself[1]);
   if (s->myself[2] == ',') sindex = 3;
   else if (s->myself[3] == ',') sindex = 4;
	else printf("Fatal error: Grid object name %s has name in wrong fomat\n",
		s->myself);
   y_gridnum = atoi(&s->myself[sindex]);
   x_delta = WEST_BOUND + x_gridnum * EW_SEPARATION;
   y_delta = SOUTH_BOUND + y_gridnum * NS_SEPARATION;
   s->corner[SW].x = (double) x_delta;
	s->corner[SW].y = (double) y_delta;
	s->corner[NW].x = (double) x_delta;
	s->corner[NW].y = (double) (y_delta + NS_SEPARATION);
	s->corner[NE].x = (double) (x_delta + EW_SEPARATION);
	s->corner[NE].y = (double) (y_delta + NS_SEPARATION);
  	s->corner[SE].x = (double) (x_delta + EW_SEPARATION);
	s->corner[SE].y = (double) y_delta;
	tprintf("Corners:\n\tSW=(%lf, %lf), NW= (%lf, %lf)\n",
		s->corner[SW].x, s->corner[SW].y, s->corner[NW].x, s->corner[NW].y);
	tprintf("\tNE = (%lf, %lf), SE = (%lf, %lf)\n",
		s->corner[NE].x, s->corner[NE].y, s->corner[SE].x, s->corner[SE].y);
  /*
   * Determine neighbor to west, and western boundary.
   */
   if ((x = x_gridnum - 1) >= 0)
		{
	   sprintf(s->Neighbors[WEST].name, "G%d,%d", x, y_gridnum);
		}
	else strcpy(s->Neighbors[WEST].name, "\0");	
	s->Neighbors[WEST].boundary.e1 = s->corner[SW];
	s->Neighbors[WEST].boundary.e2 = s->corner[NW];
  /*
	* Determine neighbor to the North
	*/
	if ((y = y_gridnum + 1) * NS_SEPARATION < NORTH_BOUND)
		{
		sprintf(s->Neighbors[NORTH].name, "G%d,%d", x_gridnum, y);
		}
	else strcpy(s->Neighbors[NORTH].name, "\0");
	s->Neighbors[NORTH].boundary.e1 = s->corner[NW];
	s->Neighbors[NORTH].boundary.e2 = s->corner[NE];
  /*
	* Determine neighbor to the South
	*/
	if ((y = y_gridnum - 1) >= 0)
		{
		sprintf(s->Neighbors[SOUTH].name, "G%d,%d", x_gridnum, y);
		}
	else strcpy(s->Neighbors[SOUTH].name, "\0");
	s->Neighbors[SOUTH].boundary.e1 = s->corner[SW];
	s->Neighbors[SOUTH].boundary.e2 = s->corner[SE];
  /*
	* Determine neighbor to the East
	*/
	if ((x = x_gridnum + 1) * EW_SEPARATION < EAST_BOUND)
		{
		sprintf(s->Neighbors[EAST].name, "G%d,%d",x, y_gridnum);	
		}
	else strcpy(s->Neighbors[EAST].name, "\0");
	s->Neighbors[EAST].boundary.e1 = s->corner[SE];
	s->Neighbors[EAST].boundary.e2 = s->corner[NE];

	s->gridx = x_gridnum;	
	s->gridy = y_gridnum;

	tprintf("Name dump:\nNorth %s South %s East %s West %s\n",
		s->Neighbors[NORTH].name, s->Neighbors[SOUTH].name,
		s->Neighbors[EAST].name, s->Neighbors[WEST].name);
#	if 0
	tprintf("Boundary dump:\nNorth %s", 
		printline(s->Neighbors[NORTH].boundary,result));
	tprintf(" South %s\n",
		printline(s->Neighbors[SOUTH].boundary,result));
	tprintf("East %s",
		printline(s->Neighbors[EAST].boundary,result));
	tprintf("West %s\n",
		printline(s->Neighbors[WEST].boundary,result));
#	endif
	toggle_graphics(s);
   }

/*static*/ int toggle_graphics(s)
state *s;
	{
#	ifdef TURN_ON_GRAPHICS
	Output_message o;

	o.select.code = GRAPH_ON;
	tell(s->myself, now+SIM_TIME_SCALE, 0, sizeof(o.select.code), &o);
#	endif
	}

query()
{}

/*
 *						P E R C E I V E    A D T
 *
 * The following routines manipulate the 'perceive' array in the
 * unit array in the state.
 */

int find_perceive(s, unitIndex, unitIndex_tofind)
state *s;
int unitIndex, unitIndex_tofind;
	{
	int i;
	char error[80];

	for (i = 0; i < MAX_PERCEIVE; i++)
		{
		if (s->Units[unitIndex].perceive[i].index == unitIndex_tofind) return i;
		else if (s->Units[unitIndex].perceive[i].index == -1) return i;
		}
#if 0
	sprintf(error, "Array s->units[%d].perceive is not big enough (size = %d)",
		unitIndex, MAX_PERCEIVE);
	grid_error(s, error);
#endif 
	return 0;
	}

/*static*/ int add_perceive(s, unitIndex, who)
state *s;
int unitIndex;
Name_object who;
	{
	int i, j;

  /*
   * Find index of unit you are adding to the perception list
	*/
   i = Find(who, &(s->Units[0]), sizeof(struct Units), UNITS_PER_GRID,
			String);
  /*
	* Find a location in the perception list to add the new unit.
	*/
	j = find_perceive(s, unitIndex, i);
  /*
	* Add the unit to the perception list. Possible bug: what happens if
	* the unit is deleted (via del_unit) and the index stored in the
	* perception list now refers to an invalid member of the s->Units[] array?
	*/
	s->Units[unitIndex].perceive[j].index = i;
	s->Units[unitIndex].perceive[j].ismarked = TRUE;
	}

/*
 * Warning! The following function has a side-effect: it sets the
 * 'ismarked' field to TRUE if the unit is found in the perception
 * array.
 */
/*static*/ int search_perceive(s, unitIndex, who)
state *s;
int unitIndex;
Name_object who;
	{
	int index_tofind, i;

   index_tofind = Find(who, &(s->Units[0]), sizeof(struct Units),
			UNITS_PER_GRID, String);

	for (i = 0; i < MAX_PERCEIVE; i++)
		{
		if (s->Units[unitIndex].perceive[i].index == index_tofind)
			{
			s->Units[unitIndex].perceive[i].ismarked = TRUE;
			return i;
			}
		}
	return -1;
	}

int del_perceive(s, unitIndex, i)
state *s;
int unitIndex;
int i;  /* index of entry to delete */
	{
	int j;

	for (j = i; j < MAX_PERCEIVE-1; j++)
		{
		s->Units[unitIndex].perceive[j] = 
			s->Units[unitIndex].perceive[j+1];
		}
	clear(&(s->Units[unitIndex].perceive[MAX_PERCEIVE-1]),     
		sizeof(PerHist));
	s->Units[unitIndex].perceive[MAX_PERCEIVE-1].index = -1;
	}

/*
 * The 'marked' field in the perceive structure allows us to determine
 * at the end of the event which objects hitherto perceived are now
 * out of the range of perception. The 'marked' field initially starts
 * as FALSE, and is changed to TRUE when the check_perception_radius
 * function decides that the unit is still in the radius of perception.
 * At the end of the event, all perceive-array entries with a FALSE
 * marked bit are then assumed out-of-range, and an UNINTELLIGENCE 
 * message is sent.
 */

/*static*/ int unmark_perceive(s, unitIndex) 
state *s;
int unitIndex;
	{
	int i;

	for (i = 0; i < MAX_PERCEIVE; i++)
		{
		s->Units[unitIndex].perceive[i].ismarked = FALSE;
		}
	}

/*
 * The following functions marches through the perception list for
 * unit 'unitIndex.'  If there is an entry in the perception list
 * for that unit that is not marked, then we must delete the entry.
 */
/*static*/ int check_perceive(s, unitIndex)
state *s;
int unitIndex;
	{
	int i, index;
	Output_message o;

	for (i = 0; i < MAX_PERCEIVE; i++)
		{
		index = s->Units[unitIndex].perceive[i].index;
		if (index != -1 and
			 s->Units[unitIndex].perceive[i].ismarked == FALSE)
			{
			del_perceive(s, unitIndex, i);
			}
		}
	}

/*
 * The Gridloc ADT is in the state.  It keeps track of all of the
 * grid cells that are aware of a unit's location. find_gridloc returns
 * the index in the array at which 'name' is found, or if 'name' is not
 * found it returns the first blank element in the array.
 */

/*static*/ int find_gridloc(s, i, name)
state *s;
int i;	
Obj_name name;
	{
	int j;
	char error[80];

	for (j = 0; j < 4; j++)
		{
		if (strcmp(s->Units[i].GridLocs[j], "\0") == 0 or
			 strcmp(s->Units[i].GridLocs[j], name) == 0 )
			return j;
		}
  /*
	* gridloc should NEVER have more than 4 elements.
	*/
	grid_error(s, "array gridloc, with 4 elements, has overflowed.");
	return 3;
	}

/*static*/ int add_gridloc(s, i, gridname)
state *s;
int i;	
Obj_name gridname;
	{
	int j;

	j = find_gridloc(s, i, gridname);
	strcpy(s->Units[i].GridLocs[j], gridname);
	}

/*static*/ int search_gridloc(s, i, forwho)
state *s;
int i;
Name_object forwho;
	{
	int j;

	for (j = 0; j < 4; j++)
		{
		if (strcmp(s->Units[i].GridLocs[j], forwho) == 0) return SUCCESS;
		}
	return FAILURE;
	}

int grid_error(s, string)
state *s;
char *string;
	{
	char temp[80], errstr[240];

	sprintf(errstr, "CTLS runtime debugger has detected a coding error.\n");
	sprintf(temp, "Object %s time %ld\n", s->myself, now);
	strcat(errstr, temp);  /* ??? May not work well on Bfly */
	strcat(errstr, string); /* Ditto */
	tell("stdout", now, 0, strlen(errstr)+1, errstr);
	}
