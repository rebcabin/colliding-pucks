/*
 * div.c - Fred Wieland, Jet Propulsion Laboratory, January 1988
 *
 * This file contains the source code for the division objects in
 * CTLS-88.  The division objects are the main combat objects in
 * the wargame simulation, and they interface chiefly with other
 * division objects, the grid object, and their superior corps
 * commander object.  They encompass a rectangular region that is
 * about 20 x 30 kilometers.  Those divisions on the front have
 * a 'flot', or 'front line of troops,' which moves as the battle
 * progresses.  The division also has a center point, which is what
 * the grid keeps track of.  The division moves east-west when in
 * battle, but in any direction when not engaged in combat.
 */

#include <stdio.h>
#ifdef BF_MACH
#include <math.h>
#include "pjhmath.h"
#endif

#ifdef BF_PLUS
#include "/usr/butterfly/chrys/Chrys40/include/math.h"
#include "pjhmath.h"

#ifndef HUGE
#define HUGE MAXFLOAT
#endif

#else
#ifndef BF_MACH
#include <math.h>
#endif
#endif

#ifdef CRAY
#undef HUGE
#define HUGE	999.0
#endif

#ifdef MARK3
#undef HUGE
#define HUGE 1.0e99
#endif
 

#include "twcommon.h" /* the TW system include file */
#include "motion.h"   /* motion ADT */
#include "div.h"		 /* definitions of init, event, and query */
#include "stb88.h"   /* constants and typedefs common to all objects */
#include "sysdata.h"  /* initial data for each cs and css system */
#include "divconst.h" /* constants (#define's) for division */
#include "divmsg.h"   /* message data structure definitions */
#include "divdefs.h" /* must follow divmsg.h */
#include "gridmsg.h" /* grid message data structure defs */
#include "statmsg.h" /* stat object input message definitions */
#include "corpsmsg.h" /* corps object input message defs */
#include "FWLdata.h" /* lanchester coefficient data */
#include "ctlslib.h" /* defines dmax, dmin functions */


#define gprintf if(s->f.graphics==1)printf
#define flot_p if(0==1)printf  /* divflot: output */
#define tmacro if(now==2918&&strcmp(s->e.myself,"blue_div1/9")==0)printf

/************************************************************************** 
 *
 *         S T A T E    S T R U C T U R E    D E F I N I T I O N S    
 *
 **************************************************************************
 These definitions may be found in divdefs.h      
 */

typedef struct state
	{
	Move m;  
	Battle b;
	intel i;
	Flags f;
	Environment e;
	CombatADT combat;
	Graphics g;
	int	Atomic_Operation;
	} state;

/*************************************************************************
 *
 *       M E S S A G E    S T R U C T U R E    D E F I N I T I O N S
 *
 *************************************************************************/
/*
 * Structures found in the input and output message lists are defined
 * in one of the *msg.h files (i.e. divmsg.h, corpsmsg.h, etc.)
 */
typedef union Input_message
	{
	Int behavior_selector;
	Init_div id;
	Move_order mo;
	Halt hm;
	Change_location cl;
	Initiate_battle ib;
	Suffer_attrition sa;
	Combat_assess ca;
	Perception p;
	Op_order oo;
        Send_Unit_parms sup;
        Unit_parameters up;
	} Input_message;

typedef union Output_message
	{      
	Int behavior_selector;
	Move_order mo;
	Change_velocity cv;
	Combat_assess ca;
	Initiate_battle ib;
	Suffer_attrition sa;
	Update_stats us;
	Spot_report sr;
        Send_Unit_parms sup;
        Unit_parameters up;
	Del_unit du;
	} Output_message;


/************************************************************************** 
 *
 *         O B J E C T    T Y P E    D E F I N I T I O N
 *
 **************************************************************************
 These definitions are necessary for the Time Warp Operating System, and
 are not needed for the STB88 algorithms.
 */

ObjectType divType = { "div", i_DIV, e_DIV,  t_DIV, 0, 0,  sizeof(state),0,0 };

/*************************************************************************
 *
 *     F U N C T I O N    D E C L A R A T I O N S                      
 *
 *************************************************************************/
/*
 * Declarations of functions returning nonintegers only.
 */
double calc_current_strength();
double calc_current_combat_power_ratio();
double calc_full_up_strength();
double flot_xloc();
double FWL();
double attack_multiplier();
double defend_multiplier();
double q_flot_xloc();
enum Sides d_opp_side();
double x_multiplier();

#define tprintf if(s->f.trace==1)printf

/************************************************************************
 *
 *                I N I T    S E C T I O N
 *
 ************************************************************************/

init()
	{	
	state *ps = (state *)myState;

	ps->f.trace = 0;
	ps->e.myself[0] = '\0';
	ps->m.flot_offset = NOMINAL_FLOT_OFFSET;
	ps->m.move_mode = Not_moving;
	ps->b.last_suffer_losses = -10000L;  /* don't set to NEGINF */
  /*
	* Set the initial velocity to HUGE (defined in math.h). This
	* forces the user to explicitly initialize the velocities of all
	* units, lest one speed off the board.
	*/
	ps->m.p.vel.x = HUGE; 
	ps->m.p.vel.y = HUGE;
	ps->Atomic_Operation = FALSE;
	ps->f.graphics = FALSE;

	}

/************************************************************************
 *
 *                     E V E N T    S E C T I O N
 *
 ************************************************************************/
event()
	{
	state *s = (state *)myState;

	d_pre_process(s);
  /*
	* You could put fancy logic here to see what is happening if the
	* cutoff time is reached.  For instance, if you have a suffer_losses
	* message, then you are in the middle of a battle.
	*/
	if (now > CUTOFF_TIME) return;
	d_main_event_loop(s);
	d_post_process(s);
	}

/*************************************************************************
 *
 *                P R E    P R O C E S S    F U N C T I O N 
 *
 **************************************************************************/
/* static */ int d_pre_process(s)
state *s;
	{
	int i;

	s->f.start_move = FALSE;
	s->f.continue_move = FALSE;
	s->f.changed_velocity = FALSE;
	s->f.send_ca = FALSE;
	s->f.initialize_battle = FALSE;
	s->f.no_activity = FALSE;
	s->f.redraw_flag = FALSE;
	s->e.number = numMsgs;
	if (s->e.myself[0] == '\0') 
		{
		/*strcpy(s->e.myself, myName);*/ myName(s->e.myself);
		if (strncmp(s->e.myself, "red", 3) == 0) s->e.side = Red;
		else s->e.side = Blue;
		initialize_combat_systems(s);
		calc_current_strength(s);
		send_stat_emsg(s);
		start_red_movement(s);
		}
  /*
	* Find division's current position and boundaries.
	*/
	whereis(s->m.p, ( now - s->m.last_p)/SIM_TIME_SCALE, &s->m.p);
	find_rear(s, &s->m.rear);
	find_north_bound(s, &s->m.north_boundary);
	find_south_bound(s, &s->m.south_boundary);

	s->m.last_p = now;
	s->f.num_legs_in_route = 0;
	s->f.num_battles = 0;
	s->f.posture_changed = FALSE;
	for (i = 0; i < MAX_NUM_LEGS; i++)
		clear(&(s->f.route_list[i]), sizeof(s->f.route_list[i]));
	tprintf("\n\n***%s @%ld: %d messages to process\n", s->e.myself,
		now, s->e.number);
	}

/*
 * Kludge function. Should be deleted.
 */
/* static */ int start_red_movement(s)
state *s;
	{
	Output_message o;

	clear(&o, sizeof(o));
	if (s->e.side == Blue) return;
	o.behavior_selector = START_MOVE;
	o.mo.num_legs_in_route = 1;
	o.mo.move_mode = Rigid;
	o.mo.route_list[0].pos.x = s->m.p.pos.x - 6.0;
	o.mo.route_list[0].pos.y = s->m.p.pos.y;
	o.mo.route_list[0].vel.x = -1.;
	o.mo.route_list[0].vel.y = 0.;
	/* Aevtemsg(s->e.myself, 40, sizeof(o.mo), &o); */
	}

/*
 * Read data from include file combatsys.h and put it in state.
 * The user can change the data in combatsys.h from run to run
 * in order to vary the outcome of the simulation.
 */
/* static */ int initialize_combat_systems(s)
state *s;
	{
	int i, j, max_combat_sys, max_combat_support;

   max_combat_sys = get_max_cs(s, s->e.side);
	max_combat_support = get_max_css(s, s->e.side);
  /*
	* First, initialize combat support array (s->combat.cs)
	*/
	for (i = 0, j = 0; i < MAX_CS; i++)
		{
		if (combatsys[i].side == s->e.side)
			{
			s->combat.cs[j].name = combatsys[i].name;
			s->combat.cs[j].oper = (double)combatsys[i].operational;
			s->combat.cs[j].mtoe = (double)combatsys[i].mtoe;
			s->combat.cs[j].strength = combatsys[i].strength;
			if (++j > max_combat_sys) break;
			}
		}
  /*	
	* Next, initialize combat service support (s->combat.css)
	*/
	for (i = 0, j = 0; i < MAX_CSS; i++)
		{
		if (combatsupp[i].side == s->e.side)
			{
			s->combat.css[j].name = combatsupp[i].name;
			s->combat.css[j].oper = combatsupp[i].operational;
			s->combat.css[j].mtoe = combatsupp[i].mtoe;
			s->combat.css[j].strength = combatsupp[i].strength;
			if (++j > max_combat_support) break;
			}
		}
  /*
	* Finally, calculate full up strength and store in state
	*/
	s->e.full_up_str = calc_full_up_strength(s);
	s->e.current_combat_power = calc_current_combat_power_ratio(s);
	}

/* static */ int get_max_cs(s, side)
state *s;
enum Sides side;
	{
	if (side == Red) return MAX_RED_CS;
	else return MAX_BLUE_CS;
	}

/* static */ int get_max_css(s, side)
state *s;
enum Sides side;
	{
	if (side == Red) return MAX_RED_CSS;
	else return MAX_BLUE_CSS;
	}

/* static */ int send_stat_emsg(s)
state *s;
	{
#	ifdef TURN_ON_STATISTICS
	Output_message o;

	clear(&o, sizeof(o));
	format_us(s, &o);
	if (s->e.side == Red)
		tell("red_stats", now+SIM_TIME_SCALE, 0, sizeof(o.us), &o);
	else
		tell("blue_stats", now+SIM_TIME_SCALE, 0, sizeof(o.us), &o);
#	endif
	}

/**************************************************************************
 *
 *                      M A I N    E V E N T    L O O P
 *
 **************************************************************************/
/* static */ int d_main_event_loop(s)
state *s;
	{
	int i;
	Int bread, error, which_process;
	Input_message m;
	Name_object name;
	Simulation_time time;

	for (i = 0; i < s->e.number; i++)
		{
		m = * (Input_message *) msgText(i);
		which_process = (Int)(m.behavior_selector / 10);
		switch (which_process)
			{
			case INITIALIZE_DIV:
				stb_init_process(s, &m);
				break;
			case MOVE:
				move_process(s, &m);
				break;
			case FIGHT:
				fight_process(s, &m);
				break;
			case INTEL:
				intel_process(s, &m);
				break;
			case COMMUNICATE:
				communication_process(s, &m);	
				break;
			case PLANNING:
				evaluate_process(s, &m);
				break;

			case QUERY_FOR_UNIT_PARAMS :
				unit_params_process (s, &m );
				break;

			case GRAPHICS:
				graphics_process(s);
				break;
			case DEBUG:
				d_debug_process(s, &m);
				break;
			default:
				{
				Name_object who;
				Simulation_time whensent;
				printf(
				   "Error in object %s at time %ld : input message not recognized\n",
					s->e.myself, now );
				printf("behavior_selector = %d, which_process = %d\n",
					m.behavior_selector, which_process);
				strcpy(who, msgSender(i));
				whensent = msgSendTime(i);
				printf("sender %s sendtime %ld\n", who, whensent);
				break;
				}
			}
		}
	}

/* static */ int graphics_process(s)
state *s;
	{
	Output_message o;

#	ifdef TURN_ON_GRAPHICS
  /*
	* Force redrawing the unit's position.
	*/
	clear(&o, sizeof(o));
	s->f.redraw_flag = TRUE; 
	o.behavior_selector = REDRAW_SELF;
	tell(
		s->e.myself,
		now + ((12 * 60)* SIM_TIME_SCALE), 
		0, 
		sizeof(o.behavior_selector), 
		&o
	    );
#	endif
	}

/* static */ int stb_init_process(s, m)
state *s;
Input_message *m;
	{
	switch(m->behavior_selector)
		{
		case INIT_DIV:
			init_div(s, m);
			break;
		default:
			printf("Object %s at %ld: behavior_selector %d not recognized\n",
					s->e.myself, now, m->behavior_selector);
		}
	}

/* static */ int init_div(s, m)
state *s;
Input_message *m;
	{
	s->e.posture = m->id.posture;
	s->f.posture_changed = TRUE; /* triggers spot report to corps */
	strcpy(s->e.superior_corps, m->id.corps_superior);
	s->e.role = m->id.role;
	s->m.objective_point.pos = m->id.objective;
	s->m.p.pos = m->id.init_pos;
	null_vector(&s->m.p.vel);
	s->e.defend_submode = Deliberate;
	s->e.time_in_hasty_defense = 0;
	s->m.pradius = m->id.pradius;
	fix_geometry(s);  /* make NSEW bounds of div conform to model */
	d_toggle_graphics(s);
	}

/* static */ int d_toggle_graphics(s)
state *s;
	{
#	ifdef TURN_ON_GRAPHICS
	Output_message o;

	clear(&o, sizeof(o));
	o.behavior_selector = GRAPH_ON;
	tell(
		s->e.myself, 
		now+SIM_TIME_SCALE , 
		0,
		sizeof(o.behavior_selector), 
		&o
	    );
	o.behavior_selector = REDRAW_SELF;
	tell(
		s->e.myself, 
		SIM_TIME_SCALE, 
		0, 
		sizeof(o.behavior_selector), 
		&o
		);
#	endif
	}

/*
 * The detailed picture of a division's geometry can be found in
 * divdefs.h.  For this program, you can always assume that the
 * Blue flot will be to the East of the division and the Red flot
 * will always be to the West of the division. 
 */
/* static */ int fix_geometry(s)
state *s;
	{
	find_rear(s, &s->m.rear);
	s->m.flot_offset = NOMINAL_FLOT_OFFSET;
	flot_p("divflot:%s at %ld flot at (%.2lf, %.2lf)\n", s->e.myself, now, 
			flot_xloc(s), s->m.p.pos.y);
	s->m.num_subsectors_in_flot = 4;
	find_north_bound(s, &s->m.north_boundary);
	find_south_bound(s, &s->m.south_boundary);
	}

/*
 * The routines find_north_bound and find_south_bound return the current
 * north and south boundary lines for this division.  The routines 
 * depend on the following state variables:
 *    	s->m.p : 	the division center point (absolute coordinates)
 *			s->m.rear:	the division rear point (absolute coordinates)
 *			s->m.num_subsectors_in_flot:	the number of subsectors the
 *							division encompasses (normally, this = 4).
 */
/* static */ int find_north_bound(s, north_bound)
state *s;
Line *north_bound;
	{
	if (s->e.side == Blue)
		{
		(*north_bound).e1.x = s->m.rear.x;
		(*north_bound).e1.y = s->m.p.pos.y + div_NS_length(s) / 2;
		(*north_bound).e2.x = s->m.p.pos.x + s->m.flot_offset;
		if ((*north_bound).e2.x != flot_xloc(s))
			{
			fprintf(stderr, "object %s time %ld bad north bound\n",
				s->e.myself, now);
			fprintf(stderr, "north bound x = %.2lf, flot x = %.2lf\n",
				(*north_bound).e2.x, flot_xloc(s));
			}
		(*north_bound).e2.y = s->m.p.pos.y + div_NS_length(s) / 2;
		}
	else /* side is Red */
		{
		(*north_bound).e1.x = s->m.p.pos.x - s->m.flot_offset;
		if ((*north_bound).e1.x != flot_xloc(s))
			{
			fprintf(stderr, "object %s time %ld bad north bound\n",
				s->e.myself, now);
			fprintf(stderr, "north bound x = %.2lf, flot x = %.2lf\n",
				(*north_bound).e1.x, flot_xloc(s));
			}
		(*north_bound).e1.y = s->m.p.pos.y + div_NS_length(s) / 2;
		(*north_bound).e2.x = s->m.rear.x;
		(*north_bound).e2.y = s->m.p.pos.y + div_NS_length(s) / 2;
		}
	}

/* static */ int find_south_bound(s, south_bound) 
state *s;
Line *south_bound;
	{
   if (s->e.side == Blue)
		{
		(*south_bound).e1.x = s->m.rear.x;
		(*south_bound).e1.y = s->m.p.pos.y - div_NS_length(s) / 2;
		(*south_bound).e2.x = s->m.p.pos.x + s->m.flot_offset;
		(*south_bound).e2.y = s->m.p.pos.y - div_NS_length(s) / 2;
		}
	else /* side is Red */
		{
		(*south_bound).e1.x = s->m.p.pos.x - s->m.flot_offset;
		(*south_bound).e1.y = s->m.p.pos.y - div_NS_length(s) / 2;
		(*south_bound).e2.x = s->m.rear.x;
		(*south_bound).e2.y = s->m.p.pos.y - div_NS_length(s) / 2;
		}
	}

/* static */ int div_NS_length(s)
state *s;
	{
	return (s->m.num_subsectors_in_flot * 8);
	}

/* static */ int find_rear(s, rear)
state *s;
Vector *rear;
	{
	int rear_multiplier;

	if (s->e.side == Blue) rear_multiplier = -1;
	else rear_multiplier = 1;
	(*rear).x = s->m.p.pos.x + (REAR_OFFSET * rear_multiplier);
	(*rear).y = s->m.p.pos.y;
	}

/* static */ int move_process(s, m)
state *s;
Input_message *m;
	{
	if (s->e.posture == Destroyed) return;

	switch(m->behavior_selector)
		{
		case START_MOVE:
			tprintf("START_MOVE:\n ");
			start_move(s, m);
			break;
		case CONTINUE_MOVE:
			tprintf("CONTINUE_MOVE:\n ");
			continue_move(s, m);
			break;
		case HALT:
			tprintf("HALT:\n ");
			halt_move(s, m);	
			break;
		case CHANGE_LOCATION:
			tprintf("CHANGE_LOCATION:\n ");
			change_location(s, m);
			break;
		}
	}

/* static */ int start_move(s, m)
state *s;
Input_message *m;
	{
	if (move_isvalid(s, m) == FALSE) return;
	setup_next_leg(s, m);
	s->f.start_move = TRUE;  /* activates d_post_process( */
	}

/* static */ int setup_next_leg(s, m)
state *s;
Input_message *m;
	{
	int i, index, time1;
	double Dx, Dy, speed, x_dist, y_dist, angle;
	double x_vel, y_vel;
	Simulation_time time_to_next_route_change;
	int random_num, seed;

  /*
   * This function sets up the parameters for a continue_move
	* command. If some other function has already set up the
 	* parameters, then we must abort (return) from this routine.
	* Another function which influences movement is move flot.
	*/
   if (s->f.num_legs_in_route != 0) return;
  /*
	* The message start_move or continue_move has an array of points
  	* to move through.  These points are scanned in reverse order, so
   * that if there are i points, the first point to move to is in the
   * ith minus one (i - 1) index in the array, and the last point to
   * move through is in the zeroth element of the array.
	*/
	index = m->mo.num_legs_in_route - 1;
	if (index < 0) return; /* ??? should never be true */
  /*
	* get new coordinates
	*/
	Dx = m->mo.route_list[index].pos.x;
	Dy = m->mo.route_list[index].pos.y;
	tprintf("\t...continuing to x=%.2lf, y=%.2lf\n", Dx, Dy);
  /*
	* get new speed (it's an input parameter from message)
	*/
	speed = magnitude(m->mo.route_list[index].vel);
	if (speed == 0.0) 
		{
		speed = MAX_FORWARD_SPEED/1440.0;
		seed = 
			(int)floor(
				    ( now /SIM_TIME_SCALE) * s->e.current_strength) + 1;
		random_num = my_random(11, &seed) - 5;
		speed *= ((random_num/100.0) + 1.0);
		}
  /*
	* figure out how far to travel, in x and y directions
	*/
	x_dist = Dx - s->m.p.pos.x;
	y_dist = Dy - s->m.p.pos.y;
  /*
	* figure out the direction (angle) of travel. 
	*/
	if(x_dist == 0.)
		{	
		if (y_dist > 0.) angle = M_PI_2; /* defined in /usr/include/math.h */
		else angle = -M_PI_2;
		}
	else 
		{
		angle = atan(y_dist/x_dist);
		}
  /*
	* Correct for proper aliasing of tangent function.  If the x distance
	* is negative, we must add PI to the angle so that we get a negative
	* x_vel when taking the cosine of the angle.
	*/
	if (x_dist < 0.) angle += M_PI;
  /*
	* figure out x and y components of velocity
	*/ 
	x_vel = cos(angle) * speed; 
	y_vel = sin(angle) * speed;
  /*
	* figure out when you will arrive at destination
	* The test for 1.0e-5 below should read if (x_vel != 0.0). However,
	* a strange compiler bug causes x_vel to be nonzero when angle = pi/2.
	* Thus we test for x not being around zero. The compiler bug occurs
 	* only if you compile with the -g swith (debugger switch) on.
	*/
	if (x_vel > 1.0e-5 || x_vel < -1.0e-5)
		{
		time1 =  fround(fabs(x_dist/x_vel));
/*PJH	
		printf ("%s time1 = %d\n", s->e.myself, time1 );
		printf ("speed= %f x_dist= %f x_vel= %f \n",
                 speed,
                 x_dist,
                 x_vel
             	);
	*/

		if (time1 == 0)
			 time_to_next_route_change = (Simulation_time)1L;
		else 
			 time_to_next_route_change = time1;
		}
	else
		{
		time1 =  fround(fabs(y_dist/y_vel)) ;
		if (time1 == 0)
			 time_to_next_route_change = (Simulation_time)1L;
		else 
		    time_to_next_route_change = (Simulation_time)time1;
		x_vel = 0.0;
		}

   time_to_next_route_change  *= SIM_TIME_SCALE;

/*PJH	

	printf ("%s time_to_next_route_change = %d\n", s->e.myself,
				time_to_next_route_change 
		);
	*/
 
	if (y_vel < 1.0e-5 and y_vel > -1.0e-5)
		 y_vel = 0.0; 
	s->m.move_start_time = now + SIM_TIME_SCALE;
  /*
	* IF (the new velocity differs from the old velocity)
	* THEN 
	*		- flag the velocity as changed
	*		- recalculate the time of any initiate battle messages you might
	*		  have sent.
	*/
	if (x_vel != s->m.p.vel.x or y_vel != s->m.p.vel.y)
		{
		  s->m.p.vel.x = x_vel;
		s->m.p.vel.y = y_vel;
		s->f.changed_velocity = TRUE;
	  /*
		* If we have sent any initiate battle messages for our future, then
		* we have to recalculate the time at which the battle will occur.
		*/
		for (i = 0; i < MAX_OTHER_UNITS; i++)
			{
			if (s->i.intel[i].UnitName[0] == '\0') continue;
			else if (s->i.intel[i].time_to_engage > now)
				{ 
				calc_engagement_time(s, i);
				}
			}
		}
	s->m.move_mode = Rear;
  /*
	* Store parameters of message in state, so that d_post_process( can
	* assemble a continue_move message.
	*/
   for (i = 0; i < MAX_NUM_LEGS; i++)
		{
		s->f.route_list[i] = m->mo.route_list[i];
		}
	s->f.num_legs_in_route = m->mo.num_legs_in_route - 1;
	s->f.move_time = now + time_to_next_route_change;
  /*
	* Update state variables in move data structure (s->m)
	*/
	s->m.objective_point.pos.x = Dx;
	s->m.objective_point.pos.y = Dy;
	find_rear(s, &s->m.rear);
	if (s->f.move_time <= now)
		{
		printf("Error in object %s at time %ld\n",
			s->e.myself, now);
		printf("time_to_next_route_change is zero or negative.\n");
		printf("Parameter dump follows: msg index = %d, num_legs = %d\n", 
			index, m->mo.num_legs_in_route);
		printf("from msg: target x=%lf, y=%lf, speed x=%lf, y=%lf\n",
			Dx, Dy, m->mo.route_list[index].vel.x, m->mo.route_list[index].vel.y);
		printf("x_dist = %lf, y_dist = %lf\n", x_dist, y_dist);
		printf("angle = %lf, speed = %lf\n", angle, speed);
		printf("x_vel=%lf, y_vel=%lf\n", x_vel, y_vel);
		printf("time_to_next_route_change = %ld\n", time_to_next_route_change);	
		}   
	}

/*
 * Here we check to see if we're about to run into an enemy. If so,
 * we cannot move forward.
 */
/* static */ int move_isvalid(s, m)
state *s;
Input_message *m;
	{
	int i;
	Other_unit_db ou;
	Particle p;

	for (i = 0; i < MAX_OTHER_UNITS; i++)
		{
		ou = s->i.intel[i];
		if (ou.UnitName[0] == '\0') continue;
		if (ou.where.pos.x == 0.0 and ou.where.pos.y == 0.0) continue;
		whereis(ou.where, (now - ou.time_of_sight)/SIM_TIME_SCALE, &p);
	  /*
		* IF
		*    (your NS orientation overlaps with the enemy) AND
		*    (your move order sends you over enemy lines)
		* THEN 
		*		the move is invalid.
		*/
		if (fabs(s->m.p.pos.y - ou.where.pos.y) < div_NS_length(s) and
			(m->mo.route_list[0].pos.x - ou.where.pos.x) * x_multiplier(s) >= 0.0)
			{
			tprintf("\tmove to x=%.2lf invalid; over enemy lines\n", 
				m->mo.route_list[0].pos.x);
		 	return FALSE;
			}
		}
	return TRUE;
	}

/* static */ int halt_move(s, m)
state *s;
Input_message *m;
	{
	null_vector(&s->m.p.vel);
	s->f.changed_velocity = TRUE;
	}

/* static */ int continue_move(s, m)
state *s;
Input_message *m;
	{
	Particle p1;
	
	if (m->mo.movement_key != s->m.movement_key) return;
	p1 = s->m.p;
	if (m->mo.move_mode == Rear) /* not sure what this does */
		{
		/* should we adjust the flot offset here? Larry's original
		 * design did, but I'm not sure it's the right thing to do.
		 */
		}
	if (m->mo.num_legs_in_route <= 0)
		{
		tprintf("\t...movement halted; no more legs\n");
		stop_move(s);
		}
	else
	  /*
		* Set the post process flag continue_move TRUE only if there
 		* is another leg in the route.
		*/
		{
		setup_next_leg(s, m);
		s->f.continue_move = TRUE;
		}
	}

/* static */ int stop_move(s)
state *s;
	{
	Output_message o;

	clear(&o, sizeof(o));
	s->m.movement_key++;
	null_vector(&s->m.p.vel);
	s->f.changed_velocity = TRUE;
	s->m.move_mode = Not_moving;
	s->f.no_activity = TRUE;
	}
	
/* static */ int change_location(s, m)
state *s;
Input_message *m;
	{
	strcpy(s->m.owning_grid, m->cl.whereto);
	tprintf("changed location to %s\n", m->cl.whereto);
	}

/* static */ int move_flot(s, psr, posture, enemy_posture, prev_flot)
double psr;
enum Postures posture, enemy_posture;
double prev_flot;
state *s;
	{
	double flot_move, calc_new_flot(), new_flot_offset();
	double tmp_flot, new_offset;

	tprintf("\told flot x = %.2lf", flot_xloc(s));
  /*
   * First, calculate movement due to previous attrition.
	*/
	flot_move = calc_new_flot(s,psr,enemy_posture);
  /*
	* Calculate new flot offset, consider some other battle may
	* have caused move to curr_flot
	*/
	tmp_flot = new_flot_offset(s,flot_move,prev_flot);
	new_offset = fabs(tmp_flot - s->m.p.pos.x);
  /*
	* Check Blue and Red movement constraints, and send move orders.
	* Movement constraints:
	* (1) cannot go past current objective point
	* (2) cannot go too far in front of rear
	* (3) cannot be retreated past rear
	*/
	if (s->e.side == Blue)
		blue_move_constraints(s,tmp_flot,&new_offset);
	else
		red_move_constraints(s,tmp_flot,&new_offset);
  /*
	* Set flot offset in the state.
	*/
	s->m.flot_offset = new_offset;
	flot_p("divflot:%s at %ld flot at (%.2lf, %.2lf)\n", s->e.myself, now, 
			flot_xloc(s), s->m.p.pos.y);
	tprintf(" new flot x = %.2lf\n", flot_xloc(s));
	}
	
/*
 * We should generate a move order only if the new offset is > maximum
 * flot advance, or if it is < the maximum flot retreat. The center
 * point may be ahead or behind the flot.
 */
/* static */ int red_move_constraints(s,tmp_flot,new_offset)
state *s;
double tmp_flot, *new_offset;
	{
	if (s->e.posture == Attack and 
		 s->m.objective_point.pos.x <= s->m.p.pos.x)
		{
		if (tmp_flot < s->m.objective_point.pos.x)
			*new_offset = s->m.p.pos.x - s->m.objective_point.pos.x;
		if (*new_offset > MAX_FLOT_ADVANCE)
			{
			*new_offset = MAX_FLOT_ADVANCE;
			put_flot_move_in_state(s,*new_offset);
			}
		}
	else /* we must withdraw */
		{
		red_fall_back(s,*new_offset);
		}
	}
	
/* static */ int blue_move_constraints(s,tmp_flot,new_offset)
state *s;
double tmp_flot, *new_offset;
	{
	if (s->e.posture == Attack and 
		 s->m.objective_point.pos.x >= s->m.p.pos.x)
		{
		if (tmp_flot > s->m.objective_point.pos.x)
			*new_offset = s->m.objective_point.pos.x - s->m.p.pos.x;
		if (*new_offset > MAX_FLOT_ADVANCE)
			{
			*new_offset = MAX_FLOT_ADVANCE;
			put_flot_move_in_state(s,*new_offset);
			}
		}
	else
		{
		blue_fall_back(s, *new_offset);
		}
	}

/* static */ int blue_fall_back(s, new_offset) 
state *s;
double new_offset;
	{

	if (new_offset + s->m.p.pos.x  <=  s->m.rear.x)
		{
		s->e.posture = Destroyed;
		emsg(s, "Posture changed to destroy because flot < rear\n");
		if (s->m.move_mode != Not_moving)
			stop_move(s);
		s->f.posture_changed = TRUE; /* triggers spot report to corps */
		}
  /*
	* The following condition corresponds to the case where new_offset
	* is behind the centerpoint (i.e. new_offset < s->m.p.pos.x). If
	* the new offset is negative, and it is greater than MAX_FLOT_RETREAT
	* in absolute value, then we must move.  If the new_offset is positive,
	* then we do nothing.
	*/
	else if (-new_offset > MAX_FLOT_RETREAT)
		{
		put_flot_move_in_state(s,new_offset);
		}
	}

/* static */ int red_fall_back(s, new_offset) 
state *s;
double new_offset;
	{

	if (new_offset + s->m.p.pos.x >=  s->m.rear.x)
		{
		s->e.posture = Destroyed;
		emsg(s, "Posture changed to destroy because flot < rear\n");
		if (s->m.move_mode != Not_moving)
			stop_move(s);
		s->f.posture_changed = TRUE; /* triggers spot report to corps */
		}
  /*
	* The following condition corresponds to the case where new_offset
	* is ahead-of the centerpoint (i.e. new_offset > s->m.p.pos.x). If
	* the new offset is a positive number, then it means we are going
	* backwards.
	*/
	else if (new_offset > MAX_FLOT_RETREAT)
		{
		put_flot_move_in_state(s,new_offset);
		}
	}
				
/* static */ int put_flot_move_in_state(s,new_offset)
state *s;
double new_offset;
	{
	int random_num, seed;

	s->f.num_legs_in_route = 1;
	if (s->e.side == Blue)
		{
		s->f.route_list[0].pos.x = 
				s->m.p.pos.x + new_offset - (double)NOMINAL_FLOT_OFFSET;
		}
	else
		{
		s->f.route_list[0].pos.x = 
				s->m.p.pos.x - new_offset + (double) NOMINAL_FLOT_OFFSET;
		}
	s->f.route_list[0].pos.y = s->m.p.pos.y;
	s->f.route_list[0].vel.x = (double)MAX_FORWARD_SPEED/1440.0;
	seed = (int)floor((now/SIM_TIME_SCALE) * s->e.current_strength);
	random_num = my_random(11, &seed);
	s->f.route_list[0].vel.x = 
		s->f.route_list[0].vel.x * ((random_num/100.0) + 1.0);
  /*
	* Get sign of x velocity correct
	*/
	if (s->f.route_list[0].pos.x < s->m.p.pos.x)
		s->f.route_list[0].vel.x *= -1.0;
	s->f.route_list[0].vel.y = 0.0;
	s->f.start_move = TRUE;
	s->f.move_time = now+SIM_TIME_SCALE;
	}

/* static */ double calc_new_flot(s, psr, enemy_posture)
state *s;
double psr;
enum Postures enemy_posture;
	{
	double flot_move, inv_psr;

	flot_move = 0.0;
	if (s->e.posture == Attack)
		{
		if (psr > PSR_THRESH_2)
			flot_move = INCR_FLOT_MOVE_DIST_2;
		else if (psr >= PSR_THRESH_1)
			flot_move = INCR_FLOT_MOVE_DIST_1;
		if (enemy_posture == Attack and flot_move == 0.0)
			{
			if (psr == 0.0) inv_psr = HUGE;
			else inv_psr = 1 / psr;
			if (inv_psr >= PSR_THRESH_2)
				flot_move = -INCR_FLOT_MOVE_DIST_2;
			else if (inv_psr >= PSR_THRESH_1)
				flot_move = -INCR_FLOT_MOVE_DIST_1;	
 			}
		}
	else /* posture is Defend or Destroyed */
		{
		if (enemy_posture == Attack)
			{
			if (psr >= PSR_THRESH_2)
				flot_move = -INCR_FLOT_MOVE_DIST_2;
			else if (psr >= PSR_THRESH_1)
				flot_move = -INCR_FLOT_MOVE_DIST_1;
			}
		}
	return flot_move;
	}	
					
/* static */ double new_flot_offset(s,flot_move,prev_flot)
state *s;
double flot_move, prev_flot;
	{
	double tmp_flot, curr_flot;
	
   curr_flot = flot_xloc(s);
	if (flot_move <= 0.0)
		{
		tmp_flot = dmin(curr_flot, prev_flot + flot_move);
		}
	else
		{
		tmp_flot = dmax(curr_flot, prev_flot + flot_move);
		}
	return tmp_flot;
	}

/*
 * The flot location depends on whether the speed of the unit is
 * in the positive or negative x-direction.
 */
/* static */ double flot_xloc(s)
state *s;
	{
	double retval;

	if (s->e.side == Red)
		{
		retval = s->m.p.pos.x - s->m.flot_offset; 
		return retval;
		}
	else 
		{	
		retval = s->m.p.pos.x + s->m.flot_offset;
		return retval;
		}
	}

double common_boundary_ratio(s, index)
state *s;
int index; /* into intel array for enemy unit */
	{
	return ((double)1.0); /* stubbed for now */
	}

/* 
 * The following function returns either TRUE or FALSE depending on
 * whether or not the north_bound or south_bound is shared with this
 * division.
 */
int bounds_are_shared_with(s, north_bound, south_bound)
state *s;
Line north_bound, south_bound;
	{
	if ( line_ge(s->m.north_boundary, north_bound) and
		  line_gt(north_bound, s->m.south_boundary) )
		return TRUE;
	else if ( line_gt(s->m.north_boundary, south_bound) and
		  line_ge(south_bound, s->m.south_boundary) )
		return TRUE;
	else if (line_ge(north_bound, s->m.north_boundary) and
		 line_gt(s->m.north_boundary, south_bound) )
		return TRUE;
	else if (line_gt(north_bound, s->m.south_boundary) and
		 line_ge(s->m.south_boundary, south_bound) )
		return TRUE;
	else return FALSE;
	}



/* static */ int intel_process(s, m)
state *s;
Input_message *m;
	{
	int i;
	enum Sides side;
	Output_message o;
	Simulation_time time;

	clear(&o, sizeof(o));
   tprintf("INTEL_PROCESS: \n");
	tprintf("\tadding unit %s\n", m->p.UnitName);
	if (strncmp(m->p.UnitName, "red", 3) == 0) side = Red;
	else side = Blue;
  /*
	* Don't store intelligence about friendly forces.
	*/
	if (s->e.side == side) return;
  /*
	* Add intelligence to database
	*/
	i = add_intel_from_emsg(s, m);
  /*
	* Calculate parameters for engagement time
	*/
  /*
	* 'calc engagement time' calculates the time at which the friendly
	* and enemy forces will engage, if at all.  The actual initiate
	* battle message is sent from the post process routine. 
	*/

	calc_engagement_time(s, i);
	}

/* static */ int fight_process(s, m) 
state *s;
Input_message *m;
	{
	switch(m->behavior_selector)
		{
		case INITIATE_BATTLE:
			tprintf("INITIATE_BATTLE:\n ");
			initiate_battle(s, m);
			break;
		case ASSESS_COMBAT:
			tprintf("ASSESS_COMBAT:\n ");
			assess_combat(s, m);
			break;
		case SUFFER_LOSSES:
			tprintf("SUFFER_LOSSES:\n ");
			suffer_losses(s, m);
			break;

		}
	}

/*
 * The communications process handles information sent to/from the
 * division's superior corps object.  Currently, two such messages are
 * modelled: an operations order (oporder) from corps telling division
 * what to do, and a spot report from division to corps informing corps
 * of the division's status.
 */
/* static */ int communication_process(s, m)
state *s;
Input_message *m;
	{
	switch(m->behavior_selector)
		{
		case OP_ORDER:
			tprintf("CARRY_OUT_CORPS_COMMANDS:\n ");
		   carry_out_corps_commands(s, m);
			break;
		default:
			printf("object %s time %ld: communications process behavior %d\n",
				s->e.myself, now, m->behavior_selector);
			printf("not recognized.\n");
		}
	}

/* static */ int carry_out_corps_commands(s, m)
state *s;
Input_message *m;
	{
  /*
	* If the objective has already been set, then ignore the rest of
	* this behavior.
	*/
	if (s->m.objective_point.pos.x == m->oo.objective)
		{
		tprintf("\tobjective already set; no action taken.\n");
		return;
		}
	s->m.objective_point.pos.x = m->oo.objective;
	s->m.objective_point.pos.y = s->m.p.pos.y;  
	tprintf("\tnew objective point (%.2lf, %.2lf)\n", 
			s->m.objective_point.pos.x, s->m.objective_point.pos.y);
  /*
	* IF (we're currently not engaged) AND (we're currently not moving)
	* THEN (move to the new objective point)
	*/
   if (  (now - s->b.last_suffer_losses > 
			( LCAP * SIM_TIME_SCALE ) + SIM_TIME_SCALE ) and
		  	(magnitude(s->m.p.vel) < 1.0e-5) 
		)
		{
		s->e.posture = m->oo.posture;  /* ??? Modified by strength??? */
		tprintf("\tmoving to that new objective immediately.\n");
		move_to_objective(s);
		}
	}

/* static */ int move_to_objective(s)
state *s;
	{
	double dist_to_objective;
	Output_message o;
	int random_num, seed;

	clear(&o, sizeof(o));
	dist_to_objective = s->m.objective_point.pos.x - flot_xloc(s);
	if (s->e.side == Red) dist_to_objective = -dist_to_objective;
	if (dist_to_objective > 0.0)
		{
		o.behavior_selector = START_MOVE;
		o.mo.movement_key = ++s->m.movement_key;
		o.mo.route_list[0].pos.x = s->m.objective_point.pos.x;
		o.mo.route_list[0].pos.y = s->m.p.pos.y;
		o.mo.route_list[0].vel.x = 30.0/1440.0; /*30 km in 1 day (1440 minutes)*/
		seed = (int)floor( (now/SIM_TIME_SCALE) * s->e.current_strength) + 1;
		random_num = my_random(11, &seed) - 5;
		o.mo.route_list[0].vel.x = 
			o.mo.route_list[0].vel.x * ((random_num/100.0) + 1.0);
		o.mo.route_list[0].vel.y = 0.;
		o.mo.num_legs_in_route = 1;
		o.mo.move_mode = Rigid;
		tell(s->e.myself, now+SIM_TIME_SCALE, 0, sizeof(o.mo), &o);
		}
	else
		{
		tprintf("\tforce already at objective; distance = %.2lf\n", 
			dist_to_objective);
		}
	/* else you might want to send a spot report here, telling corps you
	 * have reached your objective.
	 */
	}

/* static */ int evaluate_process(s, m)
state *s;
Input_message *m;
	{
	tprintf("PLANNING: \n");
  /*
	* IF (you're currently not engaged) AND (you're currently not moving)
	* THEN (move toward next objective point)
	*/
	if (  now - s->b.last_suffer_losses > 
			( LCAP * SIM_TIME_SCALE) + SIM_TIME_SCALE and
		    magnitude(s->m.p.vel) < 1.0e-10
		) 
		{
		tprintf("\tmoving toward objective.\n");
		move_to_objective(s);
		}
	else
		{
		tprintf("\tnot moving toward objective; in battle or already moving.\n");
		}
	}

/* static */ int initiate_battle(s, m)
state *s;
Input_message *m;
	{
	int l, k, n, numb, flag;
	Int bread, error;
	Output_message o;
	double flot_x, closing_vel, dist;
	Particle flotpos;

	clear(&o, sizeof(o));
  /*
	* Debug
	*/
   tprintf("\twith enemy %s\n", m->ib.UnitName);
  /*
	* Find enemy in intel database
	*/

	l = lookup_intel_data(s, m->ib.UnitName);
	if (l == -1)
	 {
 		return; 
	 }
	if (m->ib.enemy_sighted_flag != s->i.intel[l].other_unit_sighted_flag)
	 {
/*PJH	
		printf ("%s at %d something happened since message gen\n",
					s->e.myself,now );	*/

		
		return;  /* something has changed since message was generated */
	 }	

  /*
	* Find battle in Battle database
	*/
	k = lookup_battle_entry(s, m->ib.UnitName);
	if (k == -1)
		{
		emsg(s, "no more room in battle entry table.\n");
		printf("table is %d entries long\n", MAX_NUM_BATTLES);
		return;
		}
  /*
	* If element k is nonblank, then there is an entry already for this 
	* battle and we should abort the initiate_battle behavior.
	*/
	if (s->b.bd[k].EnemyName[0] != '\0') 
	 {
		return;
	 }

  /*
	* Ask for unit parameters
	*/
	o.behavior_selector = SEND_UNIT_PARAMS;
	strcpy(o.sup.UnitName, s->e.myself);


	tell(m->ib.UnitName, now+QUERY_TIME_INC, 0, sizeof(o.sup), &o );

	s->Atomic_Operation = TRUE; 


    }

/*static */  int	unit_params_process (s, m )
state *s;
Input_message *m;
 {
	switch(m->behavior_selector)
	{
		case SEND_UNIT_PARAMS: 
			send_unit_params(s, m); 
			break;

		case UNIT_PARAMS:
			get_unit_params(s, m);
			break;
	}
 }

/* static */ int send_unit_params (s, m)
state *s;
Input_message *m;
	{
	Output_message o;
	int error;
	Particle temp_p;

	clear ( &o.up, sizeof(o.up) );

	whereis(
				s->m.p,
			   (simtime_roundup (now - s->m.last_p))/SIM_TIME_SCALE , 
				&temp_p
			 );
	strcpy(o.up.UnitName, s->e.myself);
	find_north_bound(s, &o.up.north_bound);
	find_south_bound(s, &o.up.south_bound);
	o.up.flot_position.pos.x = q_flot_xloc(s, temp_p.pos.x);
	o.up.flot_position.pos.x = temp_p.pos.x; 
	o.up.flot_position.pos.y = temp_p.pos.y;
	o.up.cp = temp_p;
	o.up.force_strength = calc_current_strength(s); /* side effect */
	o.up.direct_fire_range = DIRECT_ENGAGE_RANGE;
	o.up.posture = s->e.posture;
	o.up.side = s->e.side;
	o.up.assignment = 0; /* not currently used */
	o.up.engaged_flag = 0; /* should check for engagement */
	o.up.sector_i = m->sup.sector_i;	/* Only for corps	*/
	o.up.enemy_j = m->sup.enemy_j;
	o.up.index = m->sup.index;
	o.behavior_selector = UNIT_PARAMS;

	tell(m->sup.UnitName,  now+QUERY_TIME_INC, 0, sizeof(o.up), &o );

	}



  /*
	* Update intelligence data, especially the velocity data.
	*/

/* static */ int get_unit_params(s, m)
state *s;
Input_message *m;
	{
	int l, k, n, numb, flag;
	Int bread, error;
	double flot_x, closing_vel, dist;
	Particle flotpos;

  /*
	* Debug
	*/

	l = lookup_intel_data(s, m->up.UnitName);
	if (l == -1)
	  {
		tprintf ("%s Cannot find intel data for %s\n",
		        s->e.myself,
		 	m->up.UnitName
			);
 		return; 
	   }
  /*
	* Find battle in Battle database
	*/
	k = lookup_battle_entry(s, m->up.UnitName);
	if (k == -1)
		{
		emsg(s, "no more room in battle entry table.\n");
		printf("table is %d entries long\n", MAX_NUM_BATTLES);
		return;
		}

	if (m->up.posture == Destroyed)
   { 
/*PJH   */ 
      printf ( "%s Destroyed at %d\n", m->up.UnitName,now);
		delete_intel_data(s, m->up.UnitName);
	}
	s->i.intel[l].where = m->up.cp;
	s->i.intel[l].time_of_sight = simtime_roundup(now);
  /*
	* Don't need; see comment below.
	*
	* flot_x = flot_xloc(s);
	* flotpos.pos.x = flot_x;
	* flotpos.pos.y = s->m.p.pos.y;
	*/
  /*
	* Determine if the enemy unit shares boundaries with you, and the
	* distance between you and the enemy unit.
	*/
	flag = bounds_are_shared_with(s, m->up.north_bound, m->up.south_bound);
	/* dist = distance(flotpos, m->up.flot_position); */
  /*
	* Design change: We used to calculate engagement rules based on the
	* distance between two flots.  However, this requires that battling
	* units be aware of each other's flot--an expensive operation. The   
	* software is not working correctly as it stands, so I changed it to
	* the distance between two centerpoints, and I adjusted the engagement
	* distance constant appropriately. NOTE: the original design did not
	* take into account recalculation of the engagement time when the
	* velocity of a unit changes.  It is at that point that the flot must
	* be known precisely.  
	*/
	dist = distance(s->m.p, m->up.cp);
  /*
	* Determine if battle conditions exist.
	*/
/*PJH   
        printf("%s IB at %d flag= %d dist= %f\n",
                s->e.myself, now, flag, dist ); 	*/

 


	if (flag == TRUE and floor(dist) <= DIRECT_ENGAGE_RANGE)
		{

		create_battle_entry(s, m->up.UnitName, k);
	  /*
		* Signal to d_post_process( to send a combat assess message; store
		* parameters of message in state.
		*/
		numb = s->f.num_battles++;
		s->f.which_battle[numb] = k;
		s->f.send_ca = TRUE;
		if (s->m.move_mode != Not_moving)
			 stop_move(s);

		s->b.last_suffer_losses =
				 simtime_roundup (now ); /* flag to signal battle begun */
		}
  /*
	* If battle conditions do not currently exist, analyze whether they
	* might exist sometime in the future.  If so, then send an initiate
	* battle message for that future time.
	*/
	else if (flag == TRUE and dist > DIRECT_ENGAGE_RANGE)
		{
		calc_engagement_time(s, l);
		}
	else
		{
/*PJH */
      printf ("%s at %d initiate_battle: FLOPS overlap error.\n",
               s->e.myself,now );	
/*		emsg(s, "behavior initiate_battle: FLOPS overlap error.\n");	*/

		}
	s->Atomic_Operation = FALSE;
	}


/*static */ Simulation_time simtime_roundup ( t )
Simulation_time t;

   {
     int r;
      r = t % SIM_TIME_SCALE ;

      if ( r != 0 )
      {  
         return ( t + ( SIM_TIME_SCALE - r ) );
      }  
      else
      {   
         return t;
      }  
   }


/* static */ int calc_engagement_time(s, enemy_idx)
state *s;
int enemy_idx;
	{
	double closing_vel, dist;
	Particle enemy_p, moving_p, still_p;
	Output_message o;
	int sign;

	clear(&o, sizeof(o));


	whereis( s->i.intel[enemy_idx].where, 
		 		simtime_roundup (now - s->i.intel[enemy_idx].time_of_sight)
					/SIM_TIME_SCALE ,
		 		&enemy_p
			 );
  /*
	* Figure out closing velocity. The closing velocity will have a positive
	* sign if the two units are closing in on each other, and a negative
	* sign otherwise.
	* IF (the two units have velocities with the same sign (positive or
	*		negative)
	* THEN closing velocity = difference between you and the enemy's velocity
	* ELSE IF (one of the units has a zero velocity)
	* THEN 
	*		IF (still particle's x location > moving particle's x location)
	*		THEN
	*			closing vel = moving particle's vel * sign of moving part. vel
	*		ELSE IF (still part. x location < moving particle's x loc)
	*		THEN
	*			closing vel = moving particle's vel * opposite sign of m.p.vel
	*		ELSE (two units have same x location)
	*			closing vel = 0
	* ELSE (two units have velocities with opposite signs)
	*		 closing velocity = (your velocity + enemy's velocity) * sign,
	* 		 where sign = the sign of the velocity of the westmost unit.
	*/
	if ( (s->m.p.vel.x < 0.0 and enemy_p.vel.x < 0.0) or
		  (s->m.p.vel.x > 0.0 and enemy_p.vel.x > 0.0))
		{
		closing_vel = fabs(s->m.p.vel.x) - fabs(enemy_p.vel.x);
		}
	else if (s->m.p.vel.x == 0.0 or enemy_p.vel.x == 0.0)
		{  
		if (s->m.p.vel.x != 0.0) 
			{
			moving_p = s->m.p;
			still_p = enemy_p;
			}
		else if (enemy_p.vel.x != 0.0) 
			{
			moving_p = enemy_p;
			still_p = enemy_p;
			}
		else
			{
			null_vector(&moving_p.pos);
			null_vector(&still_p.pos);
			}
		if (still_p.pos.x > moving_p.pos.x)
			{
			closing_vel = fabs(moving_p.vel.x) * my_sign(moving_p.vel.x);		
			}
		else if (still_p.pos.x < moving_p.pos.x)
			{
			closing_vel = fabs(moving_p.vel.x) * opp_my_sign(moving_p.vel.x);
			}
		else
			{
			closing_vel = 0.0;
			}
		}
	else
		{
		sign = (s->m.p.pos.x < enemy_p.pos.x ? my_sign(s->m.p.pos.x) : 
				  my_sign(enemy_p.pos.x));	
		closing_vel = (fabs(s->m.p.vel.x) + fabs(enemy_p.vel.x)) * sign;
		}
  /*
	* Design change: battle will be based on separation of cp's rather
	* than flots.  See extensive comment in initiate battle behavior.
	*/
	dist = distance(s->m.p, enemy_p);
	if (closing_vel > 0.0)
		{ 
	  /*
		* If (enemy is behind you) then ignore battle (s->f.When = +inf)
		* else if (enemy is within direct fire range) schedule battle for
		*            now +SIM_TIME_SCALE 
		* else schedule battle for time when enemy will be exactly one
		*      direct fire range away.
		*/
		if (dist < 0.0) s->f.When = POSINF;
		else if (dist < DIRECT_ENGAGE_RANGE)
		{
			 s->f.When = simtime_roundup (now) + SIM_TIME_SCALE;
		}
		else
		{
			 s->f.When = simtime_roundup(now) + 
				(((dist - DIRECT_ENGAGE_RANGE) / closing_vel)*SIM_TIME_SCALE);
		}
		if (s->f.When == simtime_roundup (now) )
		{
			 s->f.When+=SIM_TIME_SCALE;
		}
	  /*
		* IF (the current engagement time) < (a previously calculated 
		*     engagement time)     
		* THEN commit the current engagement
		* ELSE don't do anything new.
		*
		* This situation arises when an engagement time is calculated, and
		* between the calculation and the time of the engagement the unit
		* accelerates.  The new engagement time, then, will be earlier than
		* the old engagement time.  If the unit decelerates, then when the
		* old engagement time is reached, a new engagement time will be
		* calculated sometime in the future. If the new engagement time is
		* EQUAL to the previously calculated engagement time, then that 
		* means there will be an initiate battle message NOW. This message
		* might not yet have been processed in the main event loop.
		*/
		if (s->f.When < s->i.intel[enemy_idx].time_to_engage)
			{
			s->i.intel[enemy_idx].time_to_engage = s->f.When;
			s->i.intel[enemy_idx].other_unit_sighted_flag++;
			format_ib(s, enemy_idx, &o);
			tell(s->e.myself, s->f.When, 0, sizeof(o.ib), &o);
			/* s->f.initialize_battle = TRUE; */
			/* strcpy(s->f.EnemyName, s->i.intel[enemy_idx].UnitName); */
			}
/*PJH			printf("%s battles with enemy %s at time %ld\n", 
			s->e.myself, s->i.intel[enemy_idx].UnitName, 
			s->f.When);	*/

		}
	}

/* static */ int format_ib(s, enemy_idx, o)
state *s;
int enemy_idx;
Output_message *o;
	{
	int l;

	(*o).behavior_selector = INITIATE_BATTLE;
	strcpy((*o).ib.UnitName, s->i.intel[enemy_idx].UnitName);
	l = lookup_intel_data(s, s->i.intel[enemy_idx].UnitName);
	(*o).ib.enemy_sighted_flag = s->i.intel[l].other_unit_sighted_flag;
	}

int create_battle_entry(s, EnemyName, index)
state *s;
Name_object EnemyName;
int index;  /* 'index' is place in battle array to put the new entry */
	{
	strcpy(s->b.bd[index].EnemyName, EnemyName);
	s->b.bd[index].my_prev_flot_offs = s->m.flot_offset;
	s->b.bd[index].my_prev_strength_ratio = 1.0;
	s->b.bd[index].my_battle_key = 0;
	s->b.bd[index].battle_began = now;
	}
	
int lookup_battle_entry(s, name)
state *s;
Name_object name;
	{
	int i;

	for (i = 0; i < MAX_NUM_BATTLES; i++)
		{
		if (strcmp(s->b.bd[i].EnemyName, name) == 0) return i;
		else if (s->b.bd[i].EnemyName[0] == '\0') return i;
		}
	return -1;
	}

/*
 * NOTE: The program will be in ERROR if you get an initiate_battle
 * and a delete_battle at the same time, and the initiate comes before
 * the delete.  In this case, the pointer stored in s->f.which_battle
 * may point to the incorrect entry in the battle array.  The correct
 * fix is to do the delete_battle_entry AFTER the send_messages routine
 * is called in d_post_process(. As of 3/26/88, I have not done this yet
 * (because this bug has not yet happened).
 */
int delete_battle_entry(s, who)
state *s;
Name_object who;
	{
	int i, j;

   j = lookup_battle_entry(s, who);
   if (j != -1)
		{
		for (i = j; i < MAX_NUM_BATTLES-1; i++)
			{
			s->b.bd[i] = s->b.bd[i+1];
			}
		clear(&(s->b.bd[MAX_NUM_BATTLES-1]), sizeof(Battle_data));
		return 0;
		}
	else return -1;
	}

/*
 * ADT to manage intel database in state.
 */
/* static */ int add_intel_from_emsg(s, m)
state *s;
Input_message *m;
	{
	int i;

	if ((i = lookup_intel_data(s, m->p.UnitName)) == -1)
		{
		i = find_blank_intel(s);
		}
   strcpy(s->i.intel[i].UnitName, m->p.UnitName);
	s->i.intel[i].where = m->p.where;
	s->i.intel[i].time_of_sight = simtime_roundup(now);
	s->i.intel[i].time_to_engage = POSINF;
	return i; /* return index record is stuffed in */
	}


/* static */ int find_blank_intel(s)
state *s;
	{
	int i;

	for (i = 0; i < MAX_OTHER_UNITS; i++)
		{
		if (s->i.intel[i].UnitName[0] == '\0') return i;
		}
	emsg(s, "find_blank_intel: no more spaces left; returning maximum space.\n");
	printf("Unpredictable results may now happen.\n");
	}

/* static */ int lookup_intel_data(s, name)
state *s;
Name_object name;
	{
	int i;

	for (i = 0; i < MAX_OTHER_UNITS; i++)
		{
		if (strcmp(s->i.intel[i].UnitName, name) == 0) return i;
		}
	return -1;
	}

/* static */ int delete_intel_data(s, name)
state *s;
Name_object name;
	{
	int i, whichone;

   whichone = lookup_intel_data(s, name);
	if (whichone >= 0)
		{
		for (i = whichone; i < MAX_OTHER_UNITS - 1; i++)
			{
			s->i.intel[i] = s->i.intel[i+1];
			}
		clear(&(s->i.intel[MAX_OTHER_UNITS-1]), sizeof(Other_unit_db));
		}
	else
		{
		return -1;
		}
	}
	
/*
 * Structure of a battle:
 * (1) A battle begins with an 'initiate battle' behavior, which 
 *     determines if the two units are within fighting distance and, if
 *     so, it schedules an assess combat message for self.
 * (2) The battle then iterates over these next two behaviors until one
 *     of the fighting units is destroyed:
 *     (a) Assess Combat, which formats a suffer message for the enemy,
 *         containing the number and type of weapons systems directed
 *         at the enemy. The suffer message is sent only if the enemy
 *         is not in destroyed posture.
 *     (b) Suffer Losses, which calculates your own casulaties, and
 *         schedules an assess combat for self (thus continuing the 
 *         loop).
 * (3) When one side reaches destroyed posture, then the loop above
 *     is broken at (2a). What happens is the destroyed force sends
 *     one last suffer losses message to the surviving force, and
 *		 simultaneously the surviving force is sending its suffer losses
 *		 message to the destroyed force.  When the destroyed force 
 *		 receives the suffer losses message, it ignores it.  When the
 *		 surviving force receives the suffer losses message, the
 *		 surviving force decides what to do next, which usually will
 *		 be to advance to the next objective point established by the
 *		 corps.
 */

/* static */ int assess_combat(s, m)
state *s;
Input_message *m;
	{
	Output_message o;
	int l, i, loop_max;

	clear(&o, sizeof(o));
   o.behavior_selector = SUFFER_LOSSES;
	strcpy(o.sa.UnitName, s->e.myself);
	o.sa.posture = s->e.posture;
	l = lookup_battle_entry(s, m->ca.EnemyName);
	if (l == -1)
		{
		char error[80];

		error_emsg(s);
		sprintf(error, "beh assess_combat: unit %s not found in battle array\n",
			m->ca.EnemyName);
		tell("stdout", now, 0, strlen(error)+1, error);
		return;
		}
	if (m->ca.combat_key != s->b.bd[l].my_battle_key) return;
	o.sa.prev_strength_ratio = s->b.bd[l].my_prev_strength_ratio;
  /*
	* Fill the outgoing message structure with information about
	* the combat systems you will send to the other side.
	*/
	loop_max = get_max_cs(s, s->e.side);
	for (i = 0; i < loop_max; i++)
		{
		o.sa.cs[i] = s->combat.cs[i];
		}
  /*
	* Modify strength of each combat system by the amount of strength
	* allocated to this battle.
	*/
	for (i = 0; i < loop_max; i++)
		{
		o.sa.cs[i].strength *= m->ca.pct_sys_to_alloc;
		}
	find_north_bound(s, &o.sa.north_boundary);
	find_south_bound(s, &o.sa.south_boundary);
	o.sa.flot_xloc = flot_xloc(s);
	o.sa.rear_position = s->m.rear.x;
	o.sa.p = s->m.p;
	tell (
				m->ca.EnemyName, 
				simtime_roundup ( now) +SIM_TIME_SCALE, 
				0, 
				sizeof(o.sa), 
				&o
			); 

	trace(s, "SUFFER_LOSSES", m->ca.EnemyName, now+SIM_TIME_SCALE);
	}

int suffer_losses(s, m)
state *s;
Input_message *m;
	{
	Output_message o;
	int i, j, flag;
	double psr, dist;
	
	clear(&o, sizeof(o));
	if (s->e.posture == Destroyed) return;
  /*
	* Check to see if the enemy your fighting is in your intelligence
	* database.
	*/
	j = lookup_intel_data(s, m->sa.UnitName);
  /*
	* If not, we've got a severe error.
	*/
	if (j == -1)
		{
		char error[80];

		sprintf(error, "Object %s time %ld behavior suffer_losses\n",
			s->e.myself, now); 
		tell("stdout", now, 0, strlen(error)+1, error);
		sprintf(error, "enemy intel data not found; enemy name %s\n",
			m->sa.UnitName);
		tell("stdout", now, 0, strlen(error)+1, error);
		return;
		}
	else /* update position information */
		{
		s->i.intel[j].where = m->sa.p;
		s->i.intel[j].time_of_sight = now;
		}
  /*
	* If enemy is destroyed, then 
	*	(1) get rid of its position and velocity information in the intel
	*		 database
	*	(2) try to move forward to the next objective point.
	*/
	if (m->sa.posture == Destroyed)
		{
		null_vector(&(s->i.intel[j].where.pos));
		null_vector(&(s->i.intel[j].where.vel));
		delete_intel_data(s, m->sa.UnitName);
		delete_battle_entry(s, m->sa.UnitName);
		s->f.no_activity = TRUE; /* sends an evaluate msg in post process */
		/* move_to_objective(s); */
		return;
		}
  /*
	* Check to see that battle conditions exist. Enemy must share a
	* common boundary and be within range, and you must not already be
	* destroyed.
	*/
   flag = bounds_are_shared_with(s,m->sa.north_boundary,m->sa.south_boundary);
	dist = m->sa.flot_xloc - flot_xloc(s);
  /*
	* If enemy out of range then skip the remainder of this behavior.
	*/
	if (fabs(dist) > DIRECT_ENGAGE_RANGE) return;
#	if 0 /* this next block of code interferes with divisions withdrawing */
  	if (flag == TRUE and fabs(dist) <= DIRECT_ENGAGE_RANGE)
		{
		if ((s->e.posture == Attack || s->e.posture == Defend) &&
			  s->m.move_mode != Not_moving)
			{
			stop_move(s);
			}
		}
#	endif
	i = lookup_battle_entry(s, m->sa.UnitName);
  /*
	* If a battle entry does not exist, it may be because the enemy unit
	* has detected us before we detect him due to our smaller perception
	* radius.
	*/
	if (s->b.bd[i].EnemyName[0] == '\0')
		{
		create_battle_entry(s, m->sa.UnitName, i);
		}
	s->b.bd[i].my_battle_key++;
  /*
	* Calculate losses due to enemy firepower.
	* Then format and send a combat assess message.
	*/
	if (s->b.bd[i].my_prev_strength_ratio == 0.0)
		psr = HUGE;
	else 
		psr = m->sa.prev_strength_ratio / s->b.bd[i].my_prev_strength_ratio;
	move_flot(s, psr, s->e.posture, m->sa.posture, flot_xloc(s));
	calculate_casualties(s, m, i);
	s->b.last_suffer_losses = simtime_roundup (now);
  /*
	* Continue this battle only if enemy is still alive.
	*/
	if (m->sa.posture != Destroyed && s->e.posture != Destroyed)
		{
		o.behavior_selector = ASSESS_COMBAT;
		strcpy(o.ca.EnemyName, m->sa.UnitName);
		o.ca.source_is = Suffer;
		o.ca.pct_sys_to_alloc = common_boundary_ratio(s, j);
		o.ca.combat_key = s->b.bd[i].my_battle_key;

		tell(
				s->e.myself, 
			   simtime_roundup(now) 
				+  ( LCAP * SIM_TIME_SCALE) - SIM_TIME_SCALE , 
				0, sizeof(o.ca), 
				&o
			  );
		}
	}

/*
 * calculate_casualties is responsible for attrition calculations.
 * It calls lower level routines (toe_losses_suffered, calc_current_
 * combat_power_ratio, etc.) to actually do the attrition calcs, and
 * using the new strengths it sets our posture appropriately.
 */
/* static */ int calculate_casualties(s, m, index)
state *s;
Input_message *m;
int index; /* into battle array data: s->b.bd[index]... */
	{
	double current_strength, rcp;
	
	tprintf("\told strength = %.2lf", calc_current_strength(s));
	if (s->e.posture == Destroyed) return;
	if (m->sa.posture == Destroyed)
		{
		delete_intel_data(s, m->sa.UnitName);
		delete_battle_entry(s, m->sa.UnitName);
		}
	toe_losses_suffered(s, &(m->sa.cs[0]));
	if (s->e.posture == Attack and m->sa.posture == Attack)
		{
		if (s->b.bd[index].my_prev_strength_ratio/m->sa.prev_strength_ratio
			 < ATTACK_DEFEND_THRESH)
			{
			s->e.posture = Defend;
			s->e.defend_submode = Hasty;
			s->e.time_in_hasty_defense = now;
			s->f.posture_changed = TRUE; /* triggers spot report to corps */
			}
		}
	rcp = calc_current_combat_power_ratio(s);
	current_strength = calc_current_strength(s);
	s->b.bd[index].my_prev_strength_ratio = 
			(s->e.full_up_str == 0.0 ? HUGE :
			current_strength / s->e.full_up_str);
	s->b.bd[index].my_prev_flot_offs = s->m.flot_offset;
	if (s->e.posture == Attack and rcp < ATTACK_POSTURE_THRESH)
		{
		s->e.posture = Defend;
		s->e.defend_submode = Hasty;
		s->e.time_in_hasty_defense = now;
		s->f.posture_changed = TRUE; /* triggers spot report to corps */
		}
	else if (s->e.posture == Defend and rcp < DEFEND_POSTURE_THRESH)
		{
		s->e.posture = Withdraw;
		s->f.posture_changed = TRUE; /* triggers spot report to corps */
		}
	else if (s->e.posture == Withdraw and rcp < WITHDRAW_POSTURE_THRESH)
		{
		destroy_unit(s); /* sets posture to Destroyed and does cleanup */
		s->f.posture_changed = TRUE; /* triggers spot report to corps */
		}
	tprintf(" new strength = %.2lf\n", current_strength);
	}

/*
 * The following routine destroys the unit by zeroing out its position
 * and boundaries.  This routine needs to be executed so that the
 * pad graphics in 'write pad commands' will cause the unit to become
 * invisible.
 */

/* static */ int destroy_unit(s)
state *s;
	{
	Output_message o;

	clear(&o, sizeof(o));
	s->e.posture = Destroyed;
	null_vector(&s->m.north_boundary.e1);
	null_vector(&s->m.north_boundary.e2);
	null_vector(&s->m.south_boundary.e1);
	null_vector(&s->m.south_boundary.e2);
	null_vector(&s->m.p.pos);
	s->m.flot_offset = 0.0;
	flot_p("divflot:%s at %ld flot at (%.2lf, %.2lf)\n", s->e.myself, now, 
			flot_xloc(s), s->m.p.pos.y);
	o.behavior_selector = DELETE_UNIT;
	strcpy(o.du.UnitName, s->e.myself);

printf ("Destroy_Unit %s sending to %s sndtime == %d recvtime == %d\n",
		   s->e.myself, s->m.owning_grid , now, now + SIM_TIME_SCALE );

	tell(s->m.owning_grid, now+SIM_TIME_SCALE, 0, sizeof(o.du), &o);
	}

/* static */ double calc_current_strength(s)
state *s;
	{
	int i, loop_max;
	double strength = 0.0;

   loop_max = get_max_cs(s, s->e.side);
	for (i = 0; i < loop_max; i++)
		{
		strength += (s->combat.cs[i].oper * s->combat.cs[i].strength);
		}
	s->e.current_strength = strength;
	return strength;
	}

/* static */ double calc_full_up_strength(s)
state *s;
	{
	int i, loop_max;
	double strength = 0.0;

	loop_max = get_max_cs(s, s->e.side);
	for (i = 0; i < loop_max; i++)
		{
		strength += (s->combat.cs[i].mtoe * s->combat.cs[i].strength);
		}
	s->e.full_up_str = strength;
	return strength;
	}

/* static */ double calc_current_combat_power_ratio(s)
state *s;
	{
	int strength;
	double cpr;

	strength = s->e.current_strength;
	if (s->e.full_up_str == 0.0) cpr = HUGE;
	else cpr = strength / s->e.full_up_str;
	s->e.current_combat_power = cpr;
	return cpr;
	}

/* static */ int toe_losses_suffered(s, enemy_csys)
state *s;
Combat_systems enemy_csys[]; /* systems enemy is firing at you */
	{
  /*
	* Calculate attrition of combat systems (cs) as well as combat
	* service support (css).
	*/
	calc_attrition(s, &(s->combat.cs[0]), get_max_cs(s, s->e.side), 
			  &(enemy_csys[0]));
	calc_attrition(s, &(s->combat.css[0]), get_max_css(s, s->e.side), 
			  &(enemy_csys[0]));
	}

/* static */ int calc_attrition(s, sysarray, maximum, enemy_csys)
state *s;
Combat_systems sysarray[];
int maximum;  /* maximum value in sysarray */
Combat_systems enemy_csys[]; /* enemy is firing combat systems (CS) at you */
	{
	int i, j, max_enemy_cs;
	double losses, posture_multiplier, coef;
	
   max_enemy_cs = get_max_cs(s, d_opp_side(s));
  /*
	* Figure out posture multiplier
	*/
	if (s->e.posture == Attack) 
		posture_multiplier = attack_multiplier(s);
	else if (s->e.posture == Defend and s->e.defend_submode == Deliberate)
		posture_multiplier = defend_multiplier(s);
	else posture_multiplier = 1.0;
  /*
   * For each element in sysarray (the victims),
	*/
	for (i = 0; i < maximum; i++)
		{
		if (sysarray[i].oper <= 0.) continue;
	  /*
		* For each element in enemy shooter array,
		*/
		for (j = 0; j < max_enemy_cs; j++)
			{
		  /*
			* Calculate losses of the ith victim by the jth shooter
			*/
			coef = FWL(s, (int)enemy_csys[j].name, (int)sysarray[i].name);
			losses = coef * enemy_csys[j].strength * posture_multiplier * 
							 (double)LCAP;
		  /*
			* The loss of the ith target cannot exceed the number of         
			* operational elements you had to begin with.
			*/
			losses = dmin(losses, (double)sysarray[i].oper);	
			sysarray[i].oper -= losses;
			sysarray[i].mtoe += losses;
			}
		}
	s->f.strength_changed = TRUE;
	}

/* static */ double FWL(s, shooter, target)
state *s;
int shooter, target;
	{
	int i;
	double retval;

	for (i = 0; i < MAXFWL; i++)
		{
		if ((int)FWL_coef[i].shooter == shooter and 
			 (int)FWL_coef[i].target == target)
			{
			retval = FWL_coef[i].coef;
			return retval;
			}
		}
	printf("Coefficient not found: shooter = %d, target = %d, MAXFWL=%d\n",
		shooter, target, MAXFWL);
	return 0.0;
	}

/* static */ enum Sides d_opp_side(s)
state *s;
	{
	enum Sides retval;
	
	if (s->e.side == Red) retval = Blue;
	else if (s->e.side == Blue) retval = Red;
	return retval;
	}

/* static */ double attack_multiplier(s)
state *s;
	{
	if (s->e.side == Blue) return (double)BLUE_ATTACK_MULTIPLIER;
	else return (double)RED_ATTACK_MULTIPLIER;
	}

/* static */ double defend_multiplier(s)
state *s;
	{
	if (s->e.side == Blue) return ((double)BLUE_DEFEND_MULTIPLIER);
	else return ((double)RED_DEFEND_MULTIPLIER);
	}

/* static */ int d_debug_process(s, m)
state *s;
Input_message *m;
	{
	switch(m->behavior_selector)
		{
		case TRACE_ON:
			s->f.trace = 1;
			tprintf("%s @%ld: %d messages received\n", s->e.myself, now,
					s->e.number);
			break;
		case TRACE_OFF:	
			s->f.trace = 0;
			break;
		case GRAPH_ON:
			s->f.graphics = 1;
			break;
		case GRAPH_OFF:
			s->f.graphics = 0;
			break;
		}
	}

/* static */ int d_post_process(s)
state *s;
	{
	if ( ! s->Atomic_Operation )
	 {
	     d_send_messages(s);
	     d_write_pad_commands(s);
	 }
	}

/* static */ int d_send_messages(s)
state *s;
	{
	Output_message o;
	int i, spot_report_sent;

	clear(&o, sizeof(o));
   tprintf("\nPost Process:\n");
   clear(&o, sizeof(o));
	if (s->f.start_move == TRUE || s->f.continue_move == TRUE)
		{
	  /*
		* Send continue move message to myself.
		*/
		format_cm(s, &o);
		tell(s->e.myself, s->f.move_time, 0, sizeof(o.mo), &o);
		trace(s, "CONTINUE_MOVE", s->e.myself, s->f.move_time);
		clear(&o, sizeof(o));
		}
	if (s->f.changed_velocity == TRUE)
	 	{
	  /*  
		* Send change velocity message to grid object.
		*/
		format_cv(s, &o);
		tell(s->m.owning_grid, now+SIM_TIME_SCALE, 0, sizeof(o.cv), &o);
		trace(s, "CHANGE_VELOCITY", s->m.owning_grid, now+SIM_TIME_SCALE);
		clear(&o, sizeof(o));
		}
	if (s->f.send_ca == TRUE)
		{
	  /*
		* Send combat assess to myself at now + CAP, for each battle
		* that is happening concurrently. For CTLS 88-1, there should
		* only be one concurrent battle at a time (MAX_NUM_BATTLES = 1).
		*/
		for (i = 0; i < s->f.num_battles and i < MAX_NUM_BATTLES; i++)
			{
			format_ca(s, i, &o);
			tell(
					s->e.myself, 
					now + ( LCAP * SIM_TIME_SCALE ) - SIM_TIME_SCALE, 
					0, 
					sizeof(o.ca), 
					&o
				 ); 
			trace(s, "COMBAT_ASSESS", 
								s->e.myself, 
								now+( ( LCAP * SIM_TIME_SCALE) -SIM_TIME_SCALE)
					);
			clear(&o, sizeof(o));
			}
		}
	if (s->f.no_activity == TRUE) 
		{
	  /*
	 	* Send an evaluate message so that the division can determine
		* what to do next. NOTE: it is tempting to call the evaluate
		* function here directly, but if you do so the state gets messed
		* up and the game doesn't work.
		*/
		o.behavior_selector = EVALUATE_SITUATION;
		tell(s->e.myself, now+SIM_TIME_SCALE, 0, sizeof(o.behavior_selector), &o);
		trace(s, "EVALUTE_SITUATION", s->e.myself, now+SIM_TIME_SCALE);
		clear(&o, sizeof(o));
		}
  /*
	* CAUTION! The following test to initialize the battle may be
	* redundant.  The initialize battle message may now be sent from
	* the calc engagement time routine.
	*/
	if (s->f.initialize_battle == TRUE)
		{
	  /*
		* Send initialize battle message to myself later.
		*/
		format_ib(s, &o);
		tell(s->e.myself, s->f.When, 0, sizeof(o.ib), &o);
		trace(s, "INITIATE_BATTLE", s->e.myself, s->f.When);
		clear(&o, sizeof(o));
		}
	if (s->f.strength_changed == TRUE)
		{
	  /*
		* Send an update stats message to appropriate stats object.
		*/
		send_stat_emsg(s);
		trace(s, "UPDATE_STATS", "stats_object", now+SIM_TIME_SCALE);
		clear(&o, sizeof(o));
		}	
  /*
	* IF [ 
	*		(my posture has changed)  OR 
   *		(my velocity has changed) OR
	*     (my last spot report sent to corps was more than 120 minutes ago)  
	*	  ]  AND
	*		(the simulation is not yet over)
	* THEN send a spot report.
	*/
	if ( (s->f.posture_changed == TRUE or s->f.changed_velocity == TRUE or
			now - s->e.time_last_spot_rpt_sent > ( 120 * SIM_TIME_SCALE) ) and
			now < CUTOFF_TIME)
		{
		send_spot_report(s);
		}
	}

/* static */ int format_cm(s, o)
state *s;
Output_message *o;
	{
	int i;

	(*o).behavior_selector = CONTINUE_MOVE;
	(*o).mo.movement_key = (++s->m.movement_key);
	for(i = 0; i < MAX_NUM_LEGS; i++)
		{
		(*o).mo.route_list[i] = s->f.route_list[i];
		(*o).mo.num_legs_in_route = s->f.num_legs_in_route;
		}
	(*o).mo.move_mode = s->m.move_mode;
	}

/* static */ int format_cv(s, o)
state *s;
Output_message *o;
	{
	(*o).behavior_selector = CHANGE_VELOCITY;
	strcpy((*o).cv.UnitName, s->e.myself);
	(*o).cv.p = s->m.p;
	(*o).cv.pradius = s->m.pradius;
	}

/* static */ int format_ca(s, i, o)
state *s;
int i;
Output_message *o;
	{
	int index;

	(*o).behavior_selector = ASSESS_COMBAT;
	index = s->f.which_battle[i];
	strcpy((*o).ca.EnemyName, s->b.bd[index].EnemyName);
	(*o).ca.pct_sys_to_alloc = common_boundary_ratio(s, i);
	(*o).ca.source_is = Init;
	(*o).ca.combat_key = 0; /* used to lock future behaviors */
	}


/* static */ int format_us(s, o)
state *s;
Output_message *o;
	{
	(*o).behavior_selector = UPDATE_STATS;
	strcpy((*o).us.name, s->e.myself);
	(*o).us.side = s->e.side;
	(*o).us.pct_strength = s->e.current_strength/s->e.full_up_str;
	strcpy((*o).us.gridloc, s->m.owning_grid);
   (*o).us.posture = s->e.posture;
   (*o).us.sector = Sector1; /* not implemented yet */



  /*
	* Determine whether or not you are in battle by the time interval
	* since the last suffer losses message was received.
 	*/
	if (
			now - s->b.last_suffer_losses < 
			( ( LCAP * SIM_TIME_SCALE) + SIM_TIME_SCALE) 
		)
		 (*o).us.is_engaged = TRUE;
	else      
		 (*o).us.is_engaged = FALSE;	
	}

/*
 * The following function is called by the post process routine under
 * one of the following two conditions:
 * (1) More than 120 minutes has elapsed since the last spot report
 *     was sent;
 * (2) The division's posture has changed.
 */
/* static */ int send_spot_report(s)
state *s;
	{
	int i, commo_delay;
	Output_message o;
	Simulation_time rtime;

  /*
	* Format message 
	*/
	o.behavior_selector = DIV_SPOT_REPORT;
	strcpy(o.sr.div, s->e.myself);
	o.sr.flot = flot_xloc(s);
	o.sr.current_str = calc_current_strength(s);
	o.sr.full_up_str = calc_full_up_strength(s);
	o.sr.p = s->m.p;
	o.sr.posture = s->e.posture;
  /*
	* Determine whether or not you are in battle by the time interval
	* since the last suffer losses message was received.
 	*/
	if (
			now - s->b.last_suffer_losses < 
			( ( LCAP * SIM_TIME_SCALE ) + SIM_TIME_SCALE)
		)
		o.sr.in_battle = TRUE;
	else      
		o.sr.in_battle = FALSE;	
  /*
	* Tell corps who your enemies are.
	*/
	for (i = 0; i < MAX_NUM_BATTLES; i++)
		{
		strcpy(o.sr.enemies[i], s->b.bd[i].EnemyName);
		} 
  /*
	* Determine receive time of spot report
	*/
	commo_delay = calc_communication_delay(s);
	rtime = now + commo_delay;
	tell(s->e.superior_corps, rtime, 0, sizeof(o.sr), &o);
	s->e.time_last_spot_rpt_sent = now;
	}

/* static */ int calc_communication_delay(s)
state *s;
	{
	int delay;

  /*
	* For now, the communications delay is a function of simulation time.
	* This models the phenomena that as the war ages, the lines of    
	* communications become less precise and slower. Specifically, we
	* are degrading communications at the rate of 1%.
	*/
	delay = 1 + ( ( now/SIM_TIME_SCALE) % 100 );
	if (delay <= 0) delay = 1;
	return ( delay * SIM_TIME_SCALE );
	}

/* static */ int d_write_pad_commands(s)
state *s;
	{
	if (s->f.graphics == FALSE) return;
	draw_position(s);
	/* print_strength(s); */
	}

/* static */ int draw_position(s)
state *s;
	{
	char padbuff[80];
	double flot_x;
	Vector n1, n2, s1, s2, f1, f2;
	Vector nd1, nd2, sd1, sd2, fd1, fd2;
	Vector gf1, gf2, gs2;

  /*
	* Calculate north boundaries
	*/
	n1.x = s->m.north_boundary.e1.x/SCALE_FACTOR;
	n1.y = s->m.north_boundary.e1.y/SCALE_FACTOR;
	n2.x = s->m.north_boundary.e2.x/SCALE_FACTOR;
	n2.y = s->m.north_boundary.e2.y/SCALE_FACTOR;
	difference(n1, s->g.n1, &nd1);
	difference(n2, s->g.n2, &nd2);
  /*
	* Calculate south boundaries
	*/
	s1.x = s->m.south_boundary.e1.x/SCALE_FACTOR;
	s1.y = s->m.south_boundary.e1.y/SCALE_FACTOR;
	s2.x = s->m.south_boundary.e2.x/SCALE_FACTOR;
	s2.y = s->m.south_boundary.e2.y/SCALE_FACTOR;
	difference(s1, s->g.s1, &sd1);
	difference(s2, s->g.s2, &sd2);
  /*
	* Calculate flot
	*/
	flot_x = flot_xloc(s);
	f1.x = flot_x/SCALE_FACTOR;
	f1.y = s->m.north_boundary.e1.y/SCALE_FACTOR;
	f2.x = flot_x/SCALE_FACTOR;
	f2.y = s->m.south_boundary.e1.y/SCALE_FACTOR;
	difference(f1, s->g.f1, &fd1);
	difference(f2, s->g.f2, &fd2);
  /*
	* Draw position only if it has changed by one scale factor since
	* the last drawing, or if the redraw flag is on.
	*/
	if (magnitude(nd1) > (double)SCALE_FACTOR or
		 magnitude(nd2) > (double)SCALE_FACTOR or
		 magnitude(sd1) > (double)SCALE_FACTOR or
		 magnitude(sd2) > (double)SCALE_FACTOR or
		 magnitude(fd1) > (double)SCALE_FACTOR or
		 magnitude(fd2) > (double)SCALE_FACTOR or
		 s->f.redraw_flag == TRUE)
		{
     /*
		* Erase old boundaries
		*/
		Tcoord(s->g.f1, &gf1);
		Tcoord(s->g.f2, &gf2);
		if (s->e.side == Red)
			{
			if (s->g.s1.x > s->g.s2.x) Tcoord(s->g.s1, &gs2);
			else Tcoord(s->g.s2, &gs2);
			gprintf("pad %ld :@erase_reddiv %d %d %d %d %d\n",
				now, (int)gf1.x, (int)gf1.y,(int)gf2.x,(int)gf2.y,(int)gs2.x);
			}
		else
			{
			if (s->g.s1.x < s->g.s2.x) Tcoord(s->g.s1, &gs2);
			else Tcoord(s->g.s2, &gs2);
			gprintf("pad %ld :@erase_bluediv %d %d %d %d %d\n",
				now, (int)gf1.x, (int)gf1.y,(int)gf2.x,(int)gf2.y,(int)gs2.x);
			}
		if (s->e.posture == Destroyed) return;
	  /*
		* Draw new boundaries
		*/
		Tcoord(f1, &gf1);
		Tcoord(f2, &gf2);
		if (s->e.side == Red)
			{
			if (s1.x > s2.x) Tcoord(s1, &gs2);
			else Tcoord(s2, &gs2);
			gprintf("pad %ld :@reddiv %d %d %d %d %d\n",
				now, (int)gf1.x, (int)gf1.y, (int)gf2.x,(int)gf2.y,(int)gs2.x);
			}
		else
			{
			if (s1.x < s2.x) Tcoord(s1, &gs2);
			else Tcoord(s2, &gs2);
			gprintf("pad %ld :@bluediv %d %d %d %d %d\n",
				now, (int)gf1.x, (int)gf1.y, (int)gf2.x,(int)gf2.y,(int)gs2.x);
			}
		s->g.n1 = n1;
		s->g.n2 = n2;
		s->g.s2 = s2;
		s->g.s1 = s1;
		s->g.f1 = f1;
		s->g.f2 = f2;
		s->f.redraw_flag = FALSE;
#		if 0
		Terasevector(padbuff, s->g.n1.x, s->g.n1.y, s->g.n2.x, s->g.n2.y);
		gprintf("pad %ld :%s", now, padbuff);
		Tvector(padbuff, n1.x, n1.y, n2.x, n2.y);
		gprintf("pad %ld :%s", now, padbuff);
		if (s->e.side == Red)
			{
			Terasevector(padbuff,s->g.n1.x, s->g.n1.y-1, s->g.n2.x, 
					s->g.n2.y-1);
			gprintf("pad %ld :%s", now, padbuff);
			Tvector(padbuff, n1.x, n1.y-1, n2.x, n2.y-1);
			gprintf("pad %ld :%s", now, padbuff);
			}
     /*
		* South boundary
		*/
		Terasevector(padbuff, s->g.s1.x, s->g.s1.y, s->g.s2.x, s->g.s2.y);
		gprintf("pad %ld :%s", now, padbuff);
		Tvector(padbuff, s1.x, s1.y, s2.x, s2.y);
		gprintf("pad %ld :%s", now, padbuff);
		if (s->e.side == Red)
			{
			Terasevector(padbuff,s->g.s1.x, s->g.s1.y+1, s->g.s2.x, 
					s->g.s2.y+1);
			gprintf("pad %ld :%s", now, padbuff);
			Tvector(padbuff, s1.x, s1.y+1, s2.x, s2.y+1);
			gprintf("pad %ld :%s", now, padbuff);
			}
	  /*
		* Flot
		*/
		printf("flot:%s %ld old %.2lf new %.2lf\n", s->e.myself, now,
				s->g.f1.x, f1.x);
		Terasevector(padbuff, s->g.f1.x, s->g.f1.y, s->g.f2.x, s->g.f2.y);
		gprintf("pad %ld :%s", now, padbuff);
		Tvector(padbuff, f1.x, f1.y, f2.x, f2.y);
		gprintf("pad %ld :%s", now, padbuff);
	  /*
		* Make Red flot look different
		*/
		if (s->e.side == Red)
			{
			Terasevector(padbuff,s->g.f1.x+1, s->g.f1.y, s->g.f2.x+1, 
					s->g.f2.y);
			gprintf("pad %ld :%s", now, padbuff);
			Tvector(padbuff, f1.x+1, f1.y, f2.x+1, f2.y);
			gprintf("pad %ld :%s", now, padbuff);
			Terasevector(padbuff,s->g.f1.x+2, s->g.f1.y, s->g.f2.x+2, 
					s->g.f2.y);
			gprintf("pad %ld :%s", now, padbuff);
			Tvector(padbuff, f1.x+2, f1.y, f2.x+2, f2.y);
			gprintf("pad %ld :%s", now, padbuff);
			}
#		endif
		}
	}

/* static */ int print_strength(s)
state *s;
	{
	char buffer[80];
	double strength, pct_strength;
	int where;

   strength = calc_current_strength(s);
	if (s->e.full_up_str == 0.0) pct_strength = HUGE;
	else pct_strength = 100 * strength / s->e.full_up_str;  
	sprintf(buffer, "%s time %ld : strength %.2lf at %.2lf%%\n",
		s->e.myself, now, strength, pct_strength);
	if (s->e.side == Red) where = 10;
	else where = 40;
	gprintf("pad %ld :printstring 410 %d %s\n", now, where, buffer);
	}

	
/*
 * The following are utility routines, mainly for tracing and debugging.
 */

trace(s, what, towhom, When)
state *s;
char what[20];
Name_object towhom;
Simulation_time When;
	{
	if (s->f.trace == 1)
		{
		printf("\n%s message sent at %ld to %s for %ld\n", what,now, towhom, When);
		}
	}

/* static */ int error_emsg(s)
state *s;
	{
	char format[80], error[160];

	sprintf(error, "CTLS runtime debugger has detected an error condition.\n");
	sprintf(format, "Object %s time %ld\n", s->e.myself, now);
	strcat(error, format);  /* ??? May fail on Bfly */
	tell("stdout", now, 0, strlen(error)+1, error);
	}

/* static */ int emsg(s, string)
state *s;
char *string;
	{
	char error[80];

	error_emsg(s);
	sprintf(error, "%s\n", string);
	tell("stdout", now, 0, strlen(error)+1, error);
	}

/* static */ int fround(f)
double f;
	{
	f += 0.5;
	return (int)f;
	}



/* static */ double x_multiplier(s)
state *s;
	{
	if (s->e.side == Red) return -1.0;
	else return 1.0;
	}

/* static */ int my_sign(x)
double x;
	{
	if (x < 0.0) return -1;
	else return 1;
	}

/* static */ int opp_my_sign(x)
double x;
	{
	if (x < 0.0) return 1;
	else return -1;
	}

query (s) {}



/*
 * The following function calculates the flot offset from the query
 * section.
 */
/* static */ double q_flot_xloc(s, xpos)
state *s;
double xpos;
	{
	double retval;

	if (s->e.side == Red)
		{
		retval = xpos - s->m.flot_offset;
		return retval;
		}
	else 
		{
		retval = xpos + s->m.flot_offset;
		return retval;
		}
	}

