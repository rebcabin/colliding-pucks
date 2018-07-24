/*
 * corps.c - Fred Wieland, Jet Propulsion Laboratory
 *
 * Code for the corps object in ctls-88. In order to understand this code,
 * it is necessary to understand the concepts of 'role' and 'sector.'  
 * Each division under a corp's command may be in one of three roles:
 * 'reinforcing', 'Up', or 'Reserve.'  Reserve units are not activel
 * participating in the battle.  'Up' units are on the front and either
 * fighting or ready to fight.  Reinforcing units are aiding 'Up' units.
 * Each corps has three sectors to manage, called Sector1, Secto2, and
 * Sector3.  Each corps also has a reserve sector, called Reserve.  
 * Information about these sectors can be found in the Sector_info data
 * structure. In the state, a variable 'si' of dimension 4 contains 
 * the instantiation of this data structure.  The zeroth element in this
 * array contains the Reserve sector information; elements 1, 2, and 3
 * contain information about Sector1-3 respectively.  The Corps geometry
 * looks like:

 --------------------------------------------------------
|                                  |                    |
|                                  |  Division Sector 1 |  Enemy Divisions
|                                  |                    |
|                                    ----------------------    ^
|         Corps HQ                   |                    |    |
|         and Reserve                | Division Sector 2  |    | 8 km
|           Units                    |                    |    |
|                                    ----------------------    v
|                                  |                    |   
|                                  | Division Sector 3  |  
|                                  |                    |
|--------------------------------------------------------
                                                        ^ 
	                                                     |=Division Flot
	
                                   <-------30 km -------> 
* Note that the division sectors are 8 km NS and 30 km EW.  Also note that
* it is possible for the flots of the various sectors to not line up; i.e.
* the flot for sector 2 is ahead of the flots for sectors 1 and 3 in this
* case. (FLOT = Front Line Of Troops)
*
* Corps intelligence basically involves receiving messages from subordinate
* divisions about the status of ongoing battles.  Corps command and control
* is involved with two functions: (1) determine the corps 'phase line' and
* (2) determining which subordinate divisions go into which sectors. The
* 'phase line' is a north-south line to which the corps will advance.  The
* corps will decide what the phase line is (in future models, it will be
* determined by theatre and communicated to corps) and tell the divisions
* to move there. 
*/

#ifdef BF_MACH
#include <math.h>
#include "pjhmath.h"

#else
#include <math.h>
#endif

#ifdef MARK3
#undef HUGE
#define HUGE  1.0e99
#endif


#include "twcommon.h"
#include "motion.h"
#include "stb88.h"
#include "formtab.h"
#include "corps.h"
#include "corpsconst.h"
#include "corpsdefs.h"
#include "corpsmsg.h"
#include "divmsg.h"
#include "divconst.h"
#include "gridmsg.h"
#include "array.h"
#include "ctlslib.h"


#define tprintf if(0==1)printf
#define gprintf if(s->f.draw_graphics==TRUE)printf
#define plan_p  if(0==1)printf /* plan: printed output */
/* #define PLANS */
/* #define TEST  */

/*************************************************************************
 *
 *                      S T A T E 
 *
 *************************************************************************/
typedef struct state
	{
	CEnvironment e;
	Params p;
	Formation s[4]; /* indexed by Reserve, Sector1...Sector3 (enum Sectors) */
	Movement m;
	Divisions div[MAX_DIV_UNITS_IN_CORPS];
	Enemies en[MAX_DIV_UNITS_IN_CORPS];
	Reinforcements rf[REINFORCING_LIST_SIZE];
	Reserves rs;
	Sector_info si[4]; /* indexed by Reserve, Sector1...Sector3 (enum Sectors)*/
	Miscellaneous misc;
	CFlags f;
	} state;

/*
 * If the data structures Input_message and Output_message are
 * typedefed as structs rather than as unions, the system will not
 * work.  The receiving object will decode the message incorrectly.
 */
/*************************************************************************
 *
 *              I N P U T    M E S S A G E S 
 *
 *************************************************************************/
typedef union Input_message
	{
	Int behavior_selector;
	Init_corps ic;
	Spot_report sr;
        Send_Unit_parms sup;
        Unit_parameters up;
	} Input_message;

/*************************************************************************
 *
 *             O U T P U T    M E S S A G E S 
 *
 *************************************************************************/
typedef union Output_message
	{
	Int behavior_selector;
	Init_div id;
	Add_unit au;
	Op_order oo;
	Evaluate_objectives eo;
	Move_order sm;
        Send_Unit_parms sup;
        Unit_parameters up;
	} Output_message;


/************************************************************************** 
 *
 *         O B J E C T    T Y P E    D E F I N I T I O N
 *
 **************************************************************************
 These definitions are necessary for the Time Warp Operating System, and
 are not needed for the STB88 algorithms.
 */
ObjectType corpsType = { "corps", i_CORPS, e_CORPS, t_CORPS, 0,0,sizeof(state),0,0};

char *create_div_name();
double determine_combat_power();
double determine_phase_line();
double force_for_sector_posture();
double most_advanced_div_flot();
double least_advanced_up_div_flot();
double most_advanced_flot_in_sector();

/*************************************************************************
 *
 *                    I N I T    S E C T I O N
 *
 *************************************************************************/
init()
	{
	int i, j;
	state *ps = (state *)myState;

	ps->e.myself[0] = '\0';
	for (i = 0; i < MAX_UNITS_IN_SECTOR; i++)
		{
		ps->rs.reserve_index[i] = -1;
		}
  /*
	* Initialize sector data structure
	*/
	for (i = 0; i < MAX_UNITS_IN_SECTOR; i++)
		{
		for (j = 0; j < 4; j++)
			{
			ps->si[j].enemy_index[i] = -1;
			ps->si[j].friendly_index[i] = -1;
			}
		}
	ps->p.attack_sector = (enum Sectors)0;
	ps->p.attack_sector_t = NEGINF;
	ps->f.draw_graphics = FALSE;
 
	}

/*************************************************************************
 *
 *               E V E N T    S E C T I O N 
 *
 *************************************************************************/
event()
	{
	state *s = (state *)myState;

	c_pre_process(s);
	main_event_loop(s);
	c_post_process(s);
	}

/*************************************************************************
 *
 *           P R E - P R O C E S S    F U N C T I O N
 *
 *************************************************************************/
/*static*/ int c_pre_process(s)
state *s;
	{
	s->e.number = numMsgs;
	s->misc.next_avail_divnum = 1; /* must be before call to initialize */
	if (s->e.myself[0] == '\0') 
		{
		/*strcpy(s->e.myself, myName);*/ myName(s->e.myself);
		c_initialize(s);
		}
	s->f.evaluate_objectives = FALSE;
	}

/*************************************************************************
 *
 *              I N I T I A L I Z E    B E H A V I O R
 *
 *************************************************************************/
/*static*/ int c_initialize(s)
state *s;
	{
	int i;

	if (strncmp(s->e.myself, "red", 3) == 0)
		{
		s->p.side = Red;
		s->p.corps_num = atoi(&(s->e.myself[9]));
		s->p.posture = Attack;
		}
	else
		{
		s->p.side = Blue;
		s->p.corps_num = atoi(&(s->e.myself[10]));
		s->p.posture = Defend;
		}
	i = Find(s->e.myself, &(corps_table[0]), sizeof(struct Corpstab),
				TOTAL_CORPS, String);
	s->p.formation = corps_table[i].formation_num;
	s->m.p.pos = corps_table[i].location;
	s->m.front_offset = NOMINAL_CORPS_FRONT_OFFSET;
	s->p.sectors_in_corps = 3;
	fill_formation_array(s);
	s->p.units_in_corps = count_corps_units(s);
  /*
	* Set phase line to be 60 km in front of corps 
	*/
	s->p.phase_line = s->m.p.pos.x + (double)c_x_multiplier(s) * 60;
	init_sectors(s);
	init_sector_locations(s);
	setup_reserve_units(s);
	setup_sector_units(s);
	send_objective_msg(s);
	c_toggle_graphics(s);
	}

/*static*/ int c_toggle_graphics(s)
state *s;
	{
#	ifdef TURN_ON_GRAPHICS
	Output_message o;

   clear(&o, sizeof(o));
	o.behavior_selector = GRAPH_ON;
	tell(s->e.myself, now+SIM_TIME_SCALE, 0, sizeof(o.behavior_selector), &o);
#	endif
	}

/*
 * 'send objective msg' sends a 'manage corps objectives' message
 * to self at a later time.
 */
/*static*/ int send_objective_msg(s)
state *s;
	{
	Simulation_time later;
	Output_message o;

#	ifdef TEST
  /*
	* For testing purposes, consider just the first corps object.
	*/
	clear(&o, sizeof(o));
   if (strcmp(s->e.myself, "red_corps9") != 0 and
		 strcmp(s->e.myself, "blue_corps9") != 0) return;
#	endif

  /*
	* If we're in the model initialization stage (now < 0), then
	* we need to send a corps intel message for time 0 so that the
	* sector data is set up appropriately, and an evaluate objectives
	* message for time 1, so that the corps can start its planning
	* cycle.
	*/
	if (now < 0) 
		{
		o.behavior_selector = CORPS_INTEL;
		tell(s->e.myself, 0L, 0, sizeof(o.behavior_selector), &o);
		later = ( SIM_TIME_SCALE );
		}
	else
		{
		later = ( 720 + s->p.corps_num * 10 ) * SIM_TIME_SCALE ;
		}
	o.behavior_selector = EVALUATE_OBJECTIVES;
	tell(s->e.myself, later, 0, sizeof(o.behavior_selector), &o);
	}

/*static*/ int fill_formation_array(s)
state *s;
	{
	int i, j;
	enum Sectors sector;	
	int snum;

	for (i = 0, j = 0; i < MAX_FORMTAB; i++)
		{
		if (form_table[i].side == s->p.side and
			 form_table[i].formation == s->p.formation)
			{
	 	  /*
			* The sector table in the state is indexed by enum Sector.
			*/
			sector = form_table[i].sector;	
			snum = (int)sector;  /* find index */
			s->s[snum].sector = (enum Sectors) snum;
			s->s[snum].units_in_sector = form_table[i].units_in_sector;
			s->s[snum].sector_width = form_table[i].sector_width;
			}
		}
	}

/*static*/ int setup_sector_units(s)
state *s;
	{
	int i, num_units, next_divnum, j;
	Name_object div_name;
	Output_message o;
	char divname[20], distributor[15];

   clear(&o, sizeof(o));
  /*
	* Figure out name of distributor object (one for each corps object)
	*/
	sprintf(distributor, "distrib%d", s->p.corps_num);
  /*
	* For all non-reserve sectors,
	*/
	for (i = 1; i <= s->p.sectors_in_corps; i++)
		{
		num_units = count_sector_units(s, (enum Sectors) i);
		next_divnum = next_available_divnum(s);
	  /*
		* for all divisions in that sector,
		*/
		for (j = next_divnum; j < next_divnum + num_units; 
			  j = next_available_divnum(s))
			{
	     /*
			* Calculate the division's name and add it to the list,
			* then tell the division its sector and role.
			*/
			strcpy(div_name, create_div_name(s, j, divname));
			add_div_unit_to_array(s, div_name, (enum Sectors)i, Up);
			add_unit_to_sector(s, div_name, (enum Sectors)i);
			format_init_div(s, div_name, Up, &o);
			tell(
				div_name, 
				(-90 * SIM_TIME_SCALE), 
				0,
				sizeof(o.id), 
				&o
			   );
			format_add_unit(s, div_name, o.id.init_pos, &o);
			tell(
				distributor, 
				(-90 * SIM_TIME_SCALE ), 
				0, 
				sizeof(o.au), 
				&o
			    );
			incr_next_available_divnum(s);
			}
		}
	}

/*static*/ int setup_reserve_units(s)
state *s;
	{
	int i, next_divnum, num_units;
	Output_message o;
	Name_object div_name;
	char divname[20], distributor[15];

   clear(&o, sizeof(o));
  /*
	* Figure out name of distributor object (one for each corps object)
	*/
	sprintf(distributor, "distrib%d", s->p.corps_num);
	s->rs.num_units_in_reserve = 0;
  /*
	* First, figure out how many divisions are in reserve for this
 	* formation.
	*/
	num_units = count_sector_units(s, Reserve);
  /*
	* Next, place these units on the reserve list and inform them
	* that they are on reserve. Then inform the distributor object
	* of the division's whereabouts so that the division/grid mapping
	* may be made.
	*/
	next_divnum = next_available_divnum(s);
	for (i = next_divnum; i < num_units + next_divnum; 
		  i = next_available_divnum(s))
		{
		strcpy(div_name, create_div_name(s, i, divname));
		add_div_unit_to_array(s, div_name, Reserve, Reserved);
		add_unit_to_reserve(s, div_name);
		add_unit_to_sector(s, div_name, Reserve);
		format_init_div(s, div_name, Reserved, &o);
		tell(
			div_name, 
			(-90 * SIM_TIME_SCALE ), 
			0, 
			sizeof(o.id), 
			&o
		    );
		format_add_unit(s, div_name, o.id.init_pos, &o);
		tell(
			distributor, 
			(-90 * SIM_TIME_SCALE), 
			0, 
			sizeof(o.au), 
			&o
		    );
		incr_next_available_divnum(s);
		}
	}

/*static*/ int format_init_div(s, div_name, role, o)
state *s;
Name_object div_name;
enum Roles role;
Output_message *o;
	{
	int i;

   (*o).id.code = INIT_DIV;
	strcpy((*o).id.div_name, div_name);
	(*o).id.posture = s->p.posture;
	corps_loc(s, &o->id.objective);  /* DEBUG THIS LINE!!!!! */ 
	(*o).id.side = s->p.side;
	strcpy((*o).id.corps_superior, s->e.myself);
	(*o).id.sector_width = sector_width(s, Reserve);
	(*o).id.role = role;
	i = Find(div_name, &(s->div[0]), sizeof(struct Divisions),
				MAX_DIV_UNITS_IN_CORPS, String);
	(*o).id.init_pos = s->div[i].p.pos;
	(*o).id.pradius = DIVISION_PRADIUS;
	}

/*static*/ int format_add_unit(s, div_name, where, o)
state *s;
Name_object div_name;
Vector where;
Output_message *o;
	{
   (*o).au.where.pos = where;
	null_vector(&o->au.where.vel);
	(*o).au.code = ADD_UNIT;
	strcpy((*o).au.UnitName, div_name);
	strcpy((*o).au.GridLoc, "\0");
	(*o).au.pradius = DIVISION_PRADIUS;
	}

/*static*/ int next_available_divnum(s)
state *s;
	{
	return (s->misc.next_avail_divnum);
	}

/*static*/ int incr_next_available_divnum(s)
state *s;
	{
	s->misc.next_avail_divnum++;
	}

/*static*/ int which_side(name, side)
Name_object name;
enum Sides *side;
	{
	if (strncmp(name, "red", 3) == 0) *side = Red;
	else *side = Blue;
	}

/*static*/ int opp_side(s)
state *s;
	{
	if (s->p.side == Red) return (int)Blue;
	else return (int)Red;
	}

	
char *create_div_name(s, div_num, retval)
state *s;
int div_num;
char retval[12];
	{
	char tmp[9];

	if (s->p.side == Red)
		strcpy(tmp, "red_div");
	else
		strcpy(tmp, "blue_div");
	sprintf(retval, "%s%d/%d", tmp, div_num, s->p.corps_num);
	return retval;
	}

/*static*/ int main_event_loop(s)
state *s;
	{
	int i;
	Int bread, error, which_process;
	Input_message m;
/*
	Name_object who;
	Simulation_time time;
*/

	for (i = 0; i < s->e.number; i++)
		{
		if (msgSelector(i) == 99) continue;
		m = * (Input_message *) msgText(i);
/*
		strcpy(who, msgSender(i));
		time = msgSendTime(i);
		if (strcmp(who, "twscon") == 0) continue;
*/
		which_process = (Int) (m.behavior_selector/10);	
		switch(which_process)
			{

			case CINTEL:
				corps_intel(s, &m);
				break;

			case COMMAND_AND_CONTROL:
				c2_process(s, &m);
				break;

			case COMMUNICATIONS:
				communications_process(s, &m);
				break;

			case QUERY_FOR_UNIT_PARAMS:
				C_unit_params_process (s, &m );
				break;

			case DEBUG:
				debug_process(s, &m);
				break;
			default:
				{
				char errstring[40];
				sprintf(errstring, "Message code %d not recognized\n", 
					m.behavior_selector);
				corps_error(s, errstring);
				}
			}
		}
	}

/*static*/ int c2_process(s, m)
state *s;
Input_message *m;
	{
	switch(m->behavior_selector)
		{
		case EVALUATE_OBJECTIVES:
			evaluate_corps_objectives(s, m);
			break;
		default:
			{
			char errstring[40];
			sprintf(errstring, "C2 behavior code %d not recognized\n",
				m->behavior_selector);
			corps_error(s, errstring);
			}
		}
	}			

/*static*/ int communications_process(s, m)
state *s;
Input_message *m;
	{
	int i, j;

	i = Find(m->sr.div, &(s->div[0]), sizeof(struct Divisions), 
		MAX_DIV_UNITS_IN_CORPS, String);
  /*
	* IF (the division is not found) AND (the posture is destroyed)
	* 		do nothing; return (since the division is destroyed, we might
	*		not find it.)
	* ELSE IF (the division is not found) AND (the posture is NOT destroyed)
	*		increment the late division sitreps statistic.  In this case,
	*		the sitrep indicating the division was destroyed arrived earlier
	*		than the sitrep before that one.  This can happen if, for instance,
	*		the communication lines are jammed. See the routine calc
	*		communication delay in div.c.
	*/
	if (i == -1 and m->sr.posture == Destroyed) return;
	else if (i == -1 and m->sr.posture != Destroyed)
		{
		s->misc.late_div_sitreps++; 
		return;
		}
	s->div[i].flot = m->sr.flot;
	s->div[i].current_strength = m->sr.current_str;
	s->div[i].full_up_strength = m->sr.full_up_str;
	s->div[i].p = m->sr.p;
	if (fabs(s->div[i].p.vel.x) < 1.0e-10) s->div[i].p.vel.x = 0.0;
	if (fabs(s->div[i].p.vel.y) < 1.0e-10) s->div[i].p.vel.x = 0.0;
	s->div[i].last_report = now;
	s->div[i].posture = m->sr.posture;
	s->div[i].is_engaged = m->sr.in_battle;
  /*
	* Update enemies
 	*/
	for (j = 0; j < MAX_NUM_BATTLES; j++)
		{
		if (strcmp(m->sr.enemies[j], "\0") != 0)
			{
			add_enemy_name(s, m->sr.enemies[j]);
			add_unit_to_sector(s, m->sr.enemies[j], s->div[i].sector);
			}
		}
	if (s->div[i].posture == Destroyed)
		cleanup_destroyed_div(s, i);
	}

/*static*/ int cleanup_destroyed_div(s, dividx)
state *s;
int dividx;
	{
  /*
	* The following two functions must be called in this order.
	*/
	remove_unit_from_sector(s, s->div[dividx].name, s->div[dividx].sector);
	del_div_unit_from_array(s, s->div[dividx].name);
	}

/*static*/ int	 C_unit_params_process (s, m )
state *s;
Input_message *m;
	{
	switch(m->behavior_selector)
		{
		case UNIT_PARAMS:
			read_unit_params (s, m );
			break;
		default :
			printf ("%s Received Unknown message type in QUERY_UNIT_PARAM\n",
							s->e.myself );
			break;
		}
	}


int	read_unit_params (s, m )
state *s;
Input_message *m;

  {
   int i,j,index;

/*PJH Need to have a lookup function which returns the index value of
	the enemy for a particular sector...	*/
/*PJH
                  printf ("%s got params from %s at %ld size = %d \n",
                           s->e.myself,
                           m->up.UnitName,
                           now,
                           sizeof (m.up)
                	);
	*/

		  /*
			* . . . update sector parameters 
			*/
	s->en[m->up.index].enemy_strength =
		 m->up.force_strength;
 /*
  * NOTE: If flot is outside of this sector,then you should
  * delete this enemy from the sector data structure!!	
		*/
	s->en[m->up.index].enemy_flot = m->up.flot_position;
	if (
		fabs(s->si[m->up.sector_i].most_advanced_flot - 
			     s->en[m->up.index].enemy_flot.pos.x) 
				< DIRECT_ENGAGE_RANGE
	   )
	  {
		s->en[m->up.index].enemy_up = TRUE;
				s->si[m->up.sector_i].up_enemy_strength += 
					s->en[m->up.index].enemy_strength;
	  }
	else s->en[m->up.index].enemy_up = FALSE;
  /*
	* For each sector in the corps,
	*/
	for (i = 0; i < 4; i++)
		{
	  /*
		* For each friendly unit in sector. . .
		*/
		for (j = 0; j < s->si[i].num_friendly_units; j++)
			{
			index = s->si[i].friendly_index[j];
			if (index < 0 or index > MAX_DIV_UNITS_IN_CORPS) 
				continue;
	     /*
			* . . . update strength parameters in sector
			*/
			if (s->div[index].role == Reinforcing and 
				 s->div[index].is_engaged == TRUE or 
				 s->div[index].move_mode == Not_moving)
				{
				s->div[index].role = Up;
				s->si[i].reinforcing_div_strength -= 
					s->div[index].current_strength;
				s->si[i].up_div_strength +=
					 s->div[index].current_strength;
				}
			else if (s->div[index].role == Up)
				{
				s->si[i].up_div_strength += 
					s->div[index].current_strength;
				}
			else if (s->div[index].role == Reinforcing)
				{
				s->si[i].reinforcing_div_strength += 
					s->div[index].current_strength;
				}
			} /* end for each friendly unit in sector  */
		} /* end for each sector in corps */

	}
	
/*static*/ int debug_process(s, m)
state *s;
Input_message *m;
	{
	switch(m->behavior_selector)
		{
		case GRAPH_ON:
			s->f.draw_graphics = TRUE;
			break;
		case GRAPH_OFF:
			s->f.draw_graphics = FALSE;
			break;
		}
	}

/*
 * The following routines deal with Geometry and Sector Management   
 */
/*static*/ int c_x_multiplier(s)  /* part of geometry ADT */
state *s;
	{
	if (s->p.side == Blue) return 1;
	else return -1;
	}

/*static*/ int count_corps_units(s)
state *s;
	{
	int i, j;

	for (i = 0; i <= s->p.sectors_in_corps; i++)
		{
		j += count_sector_units(s, (enum Sectors)i);
		}
	return j;
	}

/*static*/ int count_sector_units(s, sector)
state *s;
enum Sectors sector;
	{
	return s->s[(int)sector].units_in_sector;
	}

/*static*/ int sector_width(s, sector)
state *s;
enum Sectors sector;
	{
	return (s->s[(int)sector].sector_width);
	}

/*static*/ int corps_width(s)  /* sum up widths of sectors */
state *s;
	{
	int total_width = 0;

	total_width = /*sector_width(s, Reserve)+*/ sector_width(s, Sector1) +
					  sector_width(s, Sector2) + sector_width(s, Sector3);
	return total_width;
	}

/*static*/ int init_sectors(s)
state *s;
	{
	int i, j;

	for (i = 0; i <= s->p.sectors_in_corps; i++)
		{
		s->si[i].num_enemy_units = 0;
		s->si[i].num_friendly_units = 0;
		for (i = 0; i < MAX_UNITS_IN_SECTOR; i++)
			{
			for (j = 0; j < 4; j++)
				{
				s->si[j].enemy_index[i] = -1;
				s->si[j].friendly_index[i] = -1;
				}
			}
		}
	}

/*static*/ int init_sector_locations(s)
state *s;
	{
	int i, offset;
	Vector corpsloc;
	double div_xloc, sector_north_bound, sector_half_width;

   corps_loc(s, &corpsloc);
	s->si[(int)Reserve].up_div_loc = corpsloc;
	offset = (corps_front_offset(s) + div_rear_offset(s)) * c_x_multiplier(s);
	div_xloc = corpsloc.x + offset;
	sector_north_bound = corpsloc.y + corps_width(s)/2;
  /*
	* The following loop assumes that enum Sectors Reserve = 0, so the
 	* loop starts with 1.
	*/
	for (i = 1; i <= s->p.sectors_in_corps; i++)
		{
		sector_half_width = 	sector_width(s, (enum Sectors)i) / 2;
		s->si[i].up_div_loc.x = div_xloc;
		s->si[i].up_div_loc.y = sector_north_bound - sector_half_width;
		sector_north_bound = s->si[i].up_div_loc.y - sector_half_width;
		}
	}
	
/*
 * The following routines deal with movement and location
 */
/*static*/ int corps_loc(s, where)
state *s;
Vector *where;
	{
	*where = s->m.p.pos;
	}

/*
 * The corps_front_offset should return a POSITIVE number; if the
 * side is RED, then the calling function can multiply the result
 * by c_x_multiplier(s) to get the correct x coordinate. The front offset
 * is the distance between the corps centerpoint (s->m.p) and the front
 * of the corps (which should be equal to the division's rear).
 */
/*static*/ int corps_front_offset(s)
state *s;
	{
	return (s->m.front_offset);
	}

/*static*/ int div_rear_offset(s)
state *s;
	{
	return NOMINAL_DIV_REAR_OFFSET;
	}

/*static*/ int corps_intel(s, m)
state *s;
Input_message *m;
	{ 
	        setup_sector_data(s);
	}

/*static*/ int setup_sector_data(s)
state *s;
	{
	int i, j, index;
	Int bread, error;
	Output_message o;

	clear(&o, sizeof(o));
  /*
	* For each sector in the corps,
	*/
	for (i = 0; i < 4; i++)
		{
		s->si[i].sector_posture = Defend; 
		s->si[i].up_enemy_strength = 0.0;
		s->si[i].up_div_strength = 0.0;
		s->si[i].reinforcing_div_strength = 0.0;
		s->si[i].most_advanced_flot = 
			most_advanced_flot_in_sector(s, (enum Sectors)i);

	  /*
		* For each enemy unit in sector,
		*/
		for (j = 0; j < s->si[i].num_enemy_units; j++)
			{
			index = s->si[i].enemy_index[j];
		
			if (index < 0 or index > MAX_DIV_UNITS_IN_CORPS)
			    continue;
		  /*
			* . . . query for its unit parameters
			*/
        	        o.behavior_selector = SEND_UNIT_PARAMS;
                        strcpy(o.sup.UnitName, s->e.myself);
			o.sup.sector_i = i;
			o.sup.enemy_j = j;
			o.sup.index = index;
/*PJH
        printf ("%s asking %s for params at %ld size = %d \n",
                 o.sup.UnitName,
              	s->en[index].enemy_name,
              	now,
             	sizeof (o)
           		);
*/


			tell(
					s->en[index].enemy_name, 
					now+QUERY_TIME_INC, 
					0, 
					sizeof(o.sup),
					&o
				 );

			}   /* end of each enemy unit in sector, */

		} /* end for each sector in corps */
	}





/*static*/ double most_advanced_flot_in_sector(s, sector)
state *s;
enum Sectors sector;
	{
	int i, index, sindex;
	double max_flot;
	struct Sector_info sd;
	Particle prev_pos, curr_pos;

   max_flot = (double)(s->p.side == Blue ? -1.0 : HUGE); /* defined in math.h */
   sindex = (int)sector;
	sd = s->si[sindex];
	for (i = 0; i < MAX_UNITS_IN_SECTOR; i++)
		{
		index = sd.friendly_index[i];
		if (index < 0 or index > MAX_DIV_UNITS_IN_CORPS) continue;
	  /*
		* Figure out where flot is now by assuming that it's moving at
		* the same velocity as the centerpoint.
		*/
		prev_pos.pos.x = s->div[index].flot;
		prev_pos.pos.y = s->div[index].p.pos.y;
		prev_pos.vel = s->div[index].p.vel;
		whereis (
						prev_pos,
						(now - s->div[index].last_report)/SIM_TIME_SCALE, 
						&curr_pos
					);
		if (s->p.side == Blue and curr_pos.pos.x > max_flot)
			max_flot = s->div[index].flot;
		else if (s->p.side == Red and curr_pos.pos.x < max_flot)
			max_flot = s->div[index].flot;
		}
	sd.most_advanced_flot = max_flot;
	return max_flot;
	}

/*static*/ int evaluate_corps_objectives(s, m)
state *s;
Input_message *m;
	{
	double combat_power;
	int i, j;

   setup_sector_data(s);
	combat_power = determine_combat_power(s);
	s->p.phase_line = determine_phase_line(s, combat_power);
	implement_corps_strategy(s);
#	ifdef PLANS
	output_unit_status(s);
#	endif
	}

/*static*/ int output_unit_status(s)
state *s;
	{
	int i, j, attack_p = 0, defend_p = 0, withdraw_p = 0, destroy_p = 0;

	plan_p("\nplan:%ld %s %lf", now, s->e.myself, s->p.phase_line);
  /* 
	* for each sector. . .
	*/
	for (i = 0; i < 4; i++)
		{
		if (i == 0) plan_p("\nplan:Reserve Sector: ");
		else plan_p("\nplan:Sector %d: ", i);
	  /* 
		* for each unit in sector. . .
		*/
		for (j = 0; j < MAX_UNITS_IN_SECTOR; j++)
			if (s->si[i].friendly_index[j] != -1)
				{
				int idx;
			
			  /* 
				* print out the list of friendly units in that sector
				*/
				idx = s->si[i].friendly_index[j];
				plan_p("%d ", idx);
				}
		}
  /*
	* Count up posture bins
	*/
	for (i = 0; i < MAX_DIV_UNITS_IN_CORPS; i++)
		{
		if (s->div[i].name[0] == '\0') continue;
		switch ((int)s->div[i].posture)
			{
			case Attack:
				attack_p ++;
				break;
			case Defend:
				defend_p ++;
				break;
			case Withdraw:
				withdraw_p ++;
				break;
			case Destroyed:
				destroy_p ++;
				break;
			}
		}
	plan_p("\nposture: Attack %d Defend %d Withdraw %d Destroyed %d\n",
			attack_p, defend_p, withdraw_p, destroy_p);
	} 

/*static*/ int implement_corps_strategy(s)
state *s;
	{
	int attack_sector;

	switch((int)s->p.posture)
		{
		case Attack:
			attack_sector = corps_attack_strategy(s);
			break;
		case Defend:
			attack_sector = corps_defend_strategy(s);
			break;
		}
	reinforce_defending_sectors(s, attack_sector);
	send_oporders(s);
	s->f.evaluate_objectives = TRUE;
	}
/* 
 * Send oporders to 'Up' divisions 
 */
/*static*/ int send_oporders(s)
state *s;
	{
	int i, j, msg_delay, index;
	struct Sector_info sd;
	Output_message o;

	clear(&o, sizeof(o));
  /*
	* For each sector in corps. . .
	*/
	for (i = 1; i < 4; i++)
		{
		sd = s->si[i];
	  /*
		* For each friendly division in sector. . .
		*/
		for (j = 0; j < sd.num_friendly_units; j++)
			{
		   index = sd.friendly_index[j];
			msg_delay = determine_oporder_delay(s, index); 
			/* the following should be an error */ 
			if (index < 0 or index > MAX_DIV_UNITS_IN_CORPS) continue;
		  /*
			* If the division's role is 'Up', send it an oporder.
			*/
			if (s->div[index].role == Up)
				{
				o.behavior_selector = OP_ORDER;
				o.oo.posture = sd.sector_posture;
				o.oo.objective = s->p.phase_line;
				tell(
						s->div[index].name, 
						now+msg_delay, 
						0, 
						sizeof(o.oo), 
						&o
					 );
				}
			} /* end for each division */
		} /* end for each sector in corps */
	}

/*
 * The following algorithm ensures that oporder delays are different
 * for different divisions within the corps.
 */
/*static*/ int determine_oporder_delay(s, seed)
state *s;
int seed;
	{
	int delay;

	if (now/2 == (int)(now/2))
		{
		delay = (int)(seed/2) + 1;
		}
	else
		{
		delay = seed + 1 ;
		}
	if (delay <= 0) delay = 1;
	return ( delay * SIM_TIME_SCALE );
	}

/*static*/ int reinforce_defending_sectors(s, attack_sector)
state *s;
int attack_sector;
	{
	int i, max_additional_units;
	struct Sector_info sd;
	double required_strength;

	for (i = 1; i < 4; i++)
		{
	  /*
		* Skip the attack sector; it was reinforced in the 'corps attack
		* strategy' routine.
		*/
		if (i == attack_sector) continue;
		sd = s->si[i];
		required_strength = sd.up_enemy_strength * SECTOR_DEFEND_FR_THRESHOLD -
			(sd.up_div_strength + sd.reinforcing_div_strength);
		max_additional_units = MAX_UNITS_IN_SECTOR - sd.num_friendly_units;
		plan_p("corpsratio: %s @ %ld sector %d required strength %.2lf\n",
			s->e.myself, now, i, required_strength);
 	  /*
		* Reinforce only if you need to.
		*/
		if (required_strength > 0.0)
			{
			reinforce_sector(s, (enum Sectors)i, required_strength,
				max_additional_units, (enum Sectors)attack_sector);	
			}
		}
	}

/*static*/ int corps_attack_strategy(s)
state *s;
	{
	int i, attack_sector, max_additional_units;
	int unit_not_available, sector_holder;
	double strength_ratio, strength_ratio2, required_strength;
	struct Sector_info *sd;
	
  /*
	* IF (we have not chosen an attack sector yet) OR
	*    (the attack sector's flot is within 5 km of the corps phase line)
	* THEN
	*	  choose a new attack sector; update state variable
	*/
	attack_sector = (int)s->p.attack_sector;
	if ((int)s->p.attack_sector == 0 or 
		 (s->p.phase_line - s->si[attack_sector].most_advanced_flot) *
		 c_x_multiplier(s) < 5.0)
		{
		attack_sector = select_attack_sector(s);
		if (attack_sector < 0 or attack_sector > 3)
			{
			char errstr[160];
	
			sprintf(errstr, 
					"Attack sector=%d out of range (func corps_attack_strategy)",
					attack_sector
					 );
			corps_error(s, errstr);
			return;
			}
		s->p.attack_sector = (enum Sectors) attack_sector;
		s->p.attack_sector_t = now;
		}
  /*
	* Determine which units will attack in sector
	*/
	sd = &(s->si[attack_sector]);
	if (sd->up_enemy_strength == 0.0)
		{
		strength_ratio = HUGE;
		strength_ratio2 = HUGE;
		}
	else
		{
		strength_ratio = sd->up_div_strength / sd->up_enemy_strength;
		strength_ratio2 = (sd->reinforcing_div_strength + sd->up_div_strength)
								 / sd->up_enemy_strength;
		}
		
	plan_p("corpsratio: %s @ %ld sector %d ratio1 %.2lf ratio2 %.2lf\n",
		s->e.myself, now, (int)attack_sector, strength_ratio, 
		strength_ratio2);
	if (sd->up_enemy_strength > 0.0 and strength_ratio >= MIN_ATTACK_FR)
		{
		sd->sector_posture = Attack;
		}
	else if (strength_ratio2 >= MIN_ATTACK_FR)
		{
		sd->sector_posture = Defend;
		}
  /*
	* Attempt to get reinforcements for attack_sector
	*/
	else
		{
		if (sd->up_enemy_strength <= 0.0) sd->sector_posture = Attack;
		else sd->sector_posture = Defend;
		required_strength = sd->up_enemy_strength * MIN_ATTACK_FR -
				(sd->up_div_strength + sd->reinforcing_div_strength);
		required_strength = dmax(1.0, required_strength);
		max_additional_units = MAX_UNITS_IN_SECTOR - sd->num_friendly_units;
		reinforce_sector(s, (enum Sectors)attack_sector, required_strength,
				max_additional_units, (enum Sectors)attack_sector);
		}
	return attack_sector;
	}

/*static*/ int reinforce_sector(s, sector, required_strength, max_additional_units,
									 attack_sector)
state *s;
enum Sectors sector;
double required_strength;
int max_additional_units;
enum Sectors attack_sector;
	{
	int unit_available, index, division_index;
	
	clear(&(s->rf[0]), sizeof(struct Reinforcements)*REINFORCING_LIST_SIZE);
	unit_available = TRUE;
	while (max_additional_units > 0 and unit_available == TRUE and
			 required_strength > 0.0)
		{
		unit_available = FALSE;
	  /*
		* First, attempt to get a unit from another sector
		*/
		specify_reinforcing_unit(s,sector, &required_strength, &unit_available,
					attack_sector);
		if (unit_available == TRUE)
			max_additional_units--;
	  /*
		* Next, attempt to get a unit from reserve
		*/
		if (required_strength > 0.0 and any_reserve_unit_exists(s) and
			 max_additional_units > 0)
			{
			index = find_strongest_reserve_unit(s);
			division_index = s->rs.reserve_index[index];
			remove_unit_from_reserve(s, index);
			add_unit_to_reinforcing_list(s, division_index, sector);
			required_strength -= s->div[division_index].current_strength;
			}
		} /* end while */
  /*
	* Commit the forces only if the required strength is met.
	*/
	if (required_strength <= 0.0)
	  /*
		* commit... scans the reinforcement list and removes each unit
		* from the old sector and puts it in its new sector. It also sends
		* messages out to the division objects telling them where they go.
		*/
		commit_units_to_reinforce_sector(s, sector);
	else
	  /*
		* abort... scans the reinforcements list and puts each unit back
		* in its original place. POSSIBLE BUG: If a unit has not yet been
		* deleted from its old position, then why do we have to put it
		* back?
		*/
		abort_reinforcements(s);
	}

/*
 * 'specify reinforcing unit' adds a unit to the reinforcing array in the
 * state. The units in the array may be used to reinforce a sector, or
 * they may be returned to their original sector.
 */
/*static*/ int
specify_reinforcing_unit(s,sector_to_reinforce,required_strength,
								 unit_available, attack_sector)
state *s;
enum Sectors sector_to_reinforce;
double *required_strength;
int *unit_available;
enum Sectors attack_sector;
	{
	int i, div_index, chosen_unit_idx;
	double min_sector_force, chosen_unit_str;

   *unit_available = FALSE;
	for (i = 1; i < 4 and *required_strength > 0.0; i++)
		{
		if (i == (int) sector_to_reinforce) continue;
		if (i == (int) attack_sector) continue;
		min_sector_force = force_for_sector_posture(s,(enum Sectors) i );
		search_for_reinforcements(s, (enum Sectors) i, *required_strength, 
						min_sector_force, &chosen_unit_idx, &chosen_unit_str);
		if (chosen_unit_idx != -1)
			{
			*unit_available = TRUE;
			div_index = s->si[i].friendly_index[chosen_unit_idx];
			add_unit_to_reinforcing_list(s, div_index, (enum Sectors) i);
			*required_strength -= s->div[div_index].current_strength;
			}
		}
	}

/*static*/ double force_for_sector_posture(s, sector)
state *s;
enum Sectors sector;	
	{
	int sindex;
	double required_force;

	sindex = (int)sector;
	if (s->si[sindex].sector_posture == Attack)
		required_force = SECTOR_ATTACK_FR_THRESHOLD;
	else
		required_force = SECTOR_DEFEND_FR_THRESHOLD;
	if (s->si[sindex].up_enemy_strength > 1.0)
		{
		required_force *= s->si[sindex].up_enemy_strength;
		}
	return required_force;
	}

/*
 * 'search for reinforcements' finds a unit that can be taken away from the
 * sector without compromising the sector's capability to maintain a
 * minimum required force.
 */
/*static*/ int search_for_reinforcements(s,sector,req_str, min_force, cindex, cstr)
state *s;
enum Sectors sector;
double req_str, min_force;
int *cindex; /* index of unit in s->si[sector].friendly_index[*cindex] */
double *cstr;
	{
	enum Roles acceptable_role, chosen_unit_role;
	double delta_strength, temp_up_sector_str;
	struct Sector_info sd;
	struct Divisions div;
	int i;

	sd = s->si[(int)sector];
	temp_up_sector_str = sd.up_div_strength;
	*cindex = -1;
	*cstr = 0.0;
	chosen_unit_role = Reinforcing;
  /*
	* If the sector's strength is greater than the minimum force
	* required, then we can send a unit into the reinforcing list.
	*/
	if (temp_up_sector_str > min_force)
		{
	  /*
		* First, attempt to find a unit whose role is 'Reinforcing.'
		* Note that we cannot use any unit which has been previously
		* placed in the reinforcing list.
		*/
		for (i = 0; i < sd.num_friendly_units and *cstr < req_str; i++)
			{
			if (sd.friendly_index[i] < 0 or 
				 sd.friendly_index[i] > MAX_DIV_UNITS_IN_CORPS) continue;
			div = s->div[sd.friendly_index[i]];
		  /*
			* If the division is already in the reinforcing list, then
			* go on to the next one.
			*/
			if (Find(div.name, &(s->rf[0]), sizeof(struct Reinforcements),
					REINFORCING_LIST_SIZE, String) != -1) continue;
			if (div.role != Up and div.current_strength > *cstr)
				{
				*cstr = div.current_strength;
				*cindex = i;
				chosen_unit_role = div.role;
				}
			} /* end for each friendly unit */
	  /*
		* Next, attempt to find some 'Up' units.
		*/
		for (i = 0; i < sd.num_friendly_units and *cstr < req_str; i++)
			{
		  /*
			* If index out of range, then continue.
			*/
			if (sd.friendly_index[i] < 0 or 
				 sd.friendly_index[i] > MAX_DIV_UNITS_IN_CORPS) continue;
			div = s->div[sd.friendly_index[i]];
		  /*
			* If the division is already in the reinforcing list, then
			* go on to the next one.
			*/
			if (Find(div.name, &(s->rf[0]), sizeof(struct Reinforcements),
					REINFORCING_LIST_SIZE, String) != -1) continue;
			if (div.role == Up and div.current_strength > *cstr)
				{
			  /*
				* Consider giving up this unit.
				*/
				delta_strength = 0.0;
				if (chosen_unit_role == Up) delta_strength -= *cstr;
				delta_strength += div.current_strength;
			  /*
				* If up force maintained, then we can give it up.
				*/
				if (temp_up_sector_str - delta_strength > min_force)
					{
					temp_up_sector_str -= delta_strength;
					chosen_unit_role = Up;
					*cstr = div.current_strength;
					*cindex = i;
					}
				} /* end if div.role == Up ...*/
			} /* end for each friendly unit */
		} /* end if temp_up_str > min_force */
	} /* end function */

/*static*/ int corps_defend_strategy(s)
state *s;
	{
	int attack_sector;

	attack_sector = -1;
	return attack_sector;
	}

/*static*/ int select_attack_sector(s)
state *s;
	{
	int i, attack_sector;
	double weakest_enemy_sector_strength = HUGE; /* math.h */

  /*
	* Select sector in which weakest enemy force exists. Exclude sector
	* if units have reached corps phase line in that sector.
	*/
	for (i = 1; i < 4; i++)
		{
		if (s->si[i].most_advanced_flot != s->p.phase_line and
			 s->si[i].up_enemy_strength < weakest_enemy_sector_strength)
			{
			attack_sector = i;
			weakest_enemy_sector_strength = s->si[i].up_enemy_strength;
			}
		}
	return attack_sector;
	}

/*static*/ double determine_phase_line(s, combat_power)
state *s;
double combat_power;
	{
	double phase_line;

	if (combat_power >= CORPS_CPR_ATT_THRESH)
		{
		s->p.posture = Attack;
		phase_line = least_advanced_up_div_flot(s) +
						 ((double)ONE_DAYS_DIV_TRAVEL_DISTANCE * 
						  (double)c_x_multiplier(s));
		}
	else 
		{
		s->p.posture = Defend;
		phase_line = most_advanced_div_flot(s);
		}
	if (s->p.side == Red)
		phase_line = dmax(phase_line, (double)WEST_BOUND);
	else
		phase_line = dmin(phase_line, (double)EAST_BOUND);
	return phase_line;
	}

/*static*/ double most_advanced_div_flot(s)
state *s;
	{
	int i;
	double flot;
	Particle prev_pos, curr_pos;

   if (s->p.side == Red) flot = EAST_BOUND;
	else flot = WEST_BOUND;

	for (i = 0; i < MAX_DIV_UNITS_IN_CORPS; i++)
		{
		if (s->div[i].role == Up)
			{
		  /*
			* Figure out where flot is now.
			*/
			prev_pos.pos.x = s->div[i].flot;
			prev_pos.pos.y = s->div[i].p.pos.y;
			prev_pos.vel = s->div[i].p.vel;
			whereis(
						prev_pos,
						( now - s->div[i].last_report )/SIM_TIME_SCALE,
		 				&curr_pos
					 );
			if (s->p.side == Red) flot = dmin(flot, curr_pos.pos.x);
			else flot = dmax(flot, curr_pos.pos.x);
			}
		}
	return flot;
	}

/*static*/ double least_advanced_up_div_flot(s)
state *s;
	{
	int i;
	double flot;
	Particle prev_pos, curr_pos;

   flot = -1.0;
	for (i = 0; i < MAX_DIV_UNITS_IN_CORPS; i++)
		{
		if (s->div[i].role == Up)
			{
			if (flot == -1.0) flot = s->div[i].flot;
		  /*
			* Figure out where flot is now.
			*/
			prev_pos.pos.x = s->div[i].flot;
			prev_pos.pos.y = s->div[i].p.pos.y;
			prev_pos.vel = s->div[i].p.vel;
			whereis(
						prev_pos, 
						( now - s->div[i].last_report )/SIM_TIME_SCALE,
					   &curr_pos
					  );
			if (s->p.side == Red) flot = dmax(flot, curr_pos.pos.x);
			else flot = dmin(flot, curr_pos.pos.x);
			}
		}
	if (flot == -1.0) return 0.0;
	else return flot;
	}

/*static*/ double determine_combat_power(s)
state *s;
	{
	int i;
	double total_div_strength = 0.0, total_div_full_up_str = 0.0;
	double corps_combat_power;

	for (i = 0; i < MAX_DIV_UNITS_IN_CORPS; i++)
		{
		if (s->div[i].posture != Destroyed)
			{
			total_div_strength += s->div[i].current_strength;
			total_div_full_up_str += s->div[i].full_up_strength;
			}
		}
	if (total_div_full_up_str != 0.0)
		corps_combat_power = total_div_strength / total_div_full_up_str;
	else
		corps_combat_power = 0.0;
	return corps_combat_power;
	}

/*static*/ int c_post_process(s)
state *s;
	{
	send_messages(s);
	c_write_pad_commands(s);
	}

/*static*/ int send_messages(s)
state *s;
	{
	Output_message o;
	Simulation_time When;

   clear(&o, sizeof(o));
	if (s->f.evaluate_objectives == TRUE and now < CUTOFF_TIME)
		{
		o.behavior_selector = EVALUATE_OBJECTIVES;
		When = now +
			 ( ( CORPS_PLANNING_PERIOD + s->p.corps_num ) *
			 SIM_TIME_SCALE );
		tell(s->e.myself, When, 0, sizeof(o.eo), &o);
		}
	}

/*static*/ int c_write_pad_commands(s)
state *s;
	{
  /*
	* First, check to see if graphics toggle is on.
 	*/
	if (s->f.draw_graphics == FALSE) return;
  /*
	* Next, draw the corps boundaries only if the old boundaries have
	* changed.
	*/
   if (s->f.wrote_corps_boundaries == FALSE)
		{
	  /*
		* If there were old boundaries (i.e. it's not at the beginning of
		* the game), then erase them.
		*/
		if (s->misc.corps_top.x != 0.0 or s->misc.corps_top.y != 0.0)
			{
			erase_old_boundaries(s);
			}
		draw_new_boundaries(s);
		s->f.wrote_corps_boundaries = TRUE;
		}
	}

/*static*/ int erase_old_boundaries(s)
state *s;
	{
	char padstr[80];
/*PJH
	Terasestring(padstr, s->misc.corps_top.x, s->misc.corps_top.y, "XXX");
	gprintf("pad:%s", padstr);
	Terasestring(padstr, s->misc.corps_bottom.x, s->misc.corps_bottom.y, "XXX");
	gprintf("pad:%s", padstr);
	*/

	}
	
/*
 * 'draw new boundaries' draws the new corps boundaries.
 */
/*static*/ int draw_new_boundaries(s)
state *s;
	{
	int height;
	char padstr[80];

	height = corps_width(s);
	s->misc.corps_top.y = s->m.p.pos.y + (height / 2) - 12;
	s->misc.corps_bottom.y = s->m.p.pos.y - (height / 2);
	s->misc.corps_top.x = s->m.p.pos.x-5;
	s->misc.corps_bottom.x = s->misc.corps_top.x;
/*PJH
	Tprintstring(padstr, s->misc.corps_top.x, s->misc.corps_top.y, "XXX");
	gprintf("pad:%s", padstr);
	Tprintstring(padstr, s->misc.corps_bottom.x, s->misc.corps_bottom.y, "XXX");
	gprintf("pad:%s", padstr);
	*/

	}

query()
	{}

/*static*/ int corps_error(s, string)
state *s;
char *string;	
	{
	char error[240], temp[80];

	sprintf(error, "CTLS runtime debugger has detected a coding error.\n");
	sprintf(temp, "Object %s time %ld\n", s->e.myself, now);
	strcat(error, temp);
	strcat(error, string);
	tell("stdout", now, 0, strlen(error)+1, error);
	}

/*************************************************************************
 *
 *                    A R R A Y    M A N A G E M E N T
 *
 *************************************************************************

 This section contains functions that manage arrays: addition, deletion,
 and updating.
 */
/*************************************************************************
 *
 *                      R E S E R V E    A R R A Y
 *
 *************************************************************************/

/*static*/ int add_unit_to_reserve(s, name)
state *s;
Name_object name;  /* name of unit you're adding */
	{
	int i, j;

  /*
	* First, find index of unit in the division list.
	*/
	i = Find(name, &(s->div[0]), sizeof(struct Divisions), 
				MAX_DIV_UNITS_IN_CORPS, String);
	if (i == -1)
		{
		char errstring[40];
		sprintf(errstring, "Cannot find %s in Divisions list\n", name);
		corps_error(s, errstring);
		return;
		}
  /*
	* Next, find a blank spot in the reserve list.
	*/
	for (j = 0; j < MAX_UNITS_IN_SECTOR; j++)
		{
		if (s->rs.reserve_index[j] == -1) 
			{
		  /*
			* Insert it.
			*/
			s->rs.reserve_index[j] = i;
			break;
			}
		}
  /*
	* Finally, determine the number of units now in reserve.
	*/
	s->rs.num_units_in_reserve = count_reserves(s);
	}

/*static*/ int remove_unit_from_reserve(s, index)
state *s;
int index;
	{
	int i;

	if (index >= 0)
		{
		s->rs.reserve_index[index] = -1;
		}
	s->rs.num_units_in_reserve = count_reserves(s);
	}

/*static*/ int count_reserves(s)
state *s;
	{
	int i, count;

	for (i = 0, count = 0; i < MAX_UNITS_IN_SECTOR; i++)
		{
		if (s->rs.reserve_index[i] != -1) count++;
		}
	return count;
	}

/*
 * The following function returns the index of the reserve array which
 * holds the strongest reserve division.
 */
/*static*/ int find_strongest_reserve_unit(s)
state *s;
	{
	int i, divindex, max_strength_index;
	double strength;

	for (i = 0, strength = 0.0, max_strength_index = -1; 
		  i < MAX_UNITS_IN_SECTOR; i++)
		{
		divindex = s->rs.reserve_index[i];
		if (s->div[divindex].current_strength > strength)
			{
			strength = s->div[divindex].current_strength;
			max_strength_index = i; /* NOT divindex!! */
			}
		}
	return max_strength_index;
	}

/*static*/ int any_reserve_unit_exists(s)
state *s;
	{
	if (s->rs.num_units_in_reserve > 0) return TRUE;
	else return FALSE;
	}

/*************************************************************************
 *
 *              R E I N F O R C E M E N T S    A R R A Y
 *
 *************************************************************************/

/*static*/ int clear_reinforcing_list(s)
state *s;
	{ 
	clear(&(s->rf[0]), sizeof(struct Reinforcements) * REINFORCING_LIST_SIZE);
	}

/*static*/ int add_unit_to_reinforcing_list(s, div_index, dest_sector)
state *s;
int div_index;
enum Sectors dest_sector;
	{
	int i, index;

	index = find_blank(&(s->rf[0]), sizeof(struct Reinforcements),
							REINFORCING_LIST_SIZE, String);
	if (index == -1)
		{
		char errstr[80];

		sprintf(errstr, "No more room in reinforcing list; size = %d",
			REINFORCING_LIST_SIZE);
		corps_error(s, errstr);
		return;
		}
	strcpy(s->rf[index].div_name, s->div[div_index].name);
	s->rf[index].originating_sector = s->div[div_index].sector;
	s->rf[index].destination_sector = dest_sector;
	}

/*static*/ int commit_units_to_reinforce_sector(s, sector)
state *s;
enum Sectors sector;
	{
	int i, divindex, msg_delay, sindex;
	struct Divisions *D;
	Output_message o;
	Particle currp;  /* route_list[MAX_NUM_LEGS]; */
	double x_objective, orig_sector_flot;

	clear(&o, sizeof(o));
   sindex = (int)sector;
	for (i = 0; i < REINFORCING_LIST_SIZE; i++)
		{
		if (s->rf[i].div_name[0] == '\0') continue;
		divindex = Find(s->rf[i].div_name, &(s->div[0]), sizeof(struct Divisions),
							 MAX_DIV_UNITS_IN_CORPS, String);
		D = &(s->div[divindex]);
		D->role = Reinforcing;
		add_unit_to_sector(s, D->name, sector);
		remove_unit_from_sector(s, s->rf[i].div_name,s->rf[i].originating_sector);
		whereis(
					s->div[divindex].p, 
					(now - s->div[divindex].last_report )/SIM_TIME_SCALE, 
					&currp
				 );	
	  /*
		* Set x objective to either the most advanced flot in the sector
		* the unit will move into, or the current corps objective point if
		* there are no other divisions in the new sector.
		*/
		if (s->si[sindex].most_advanced_flot == -1.0 or 
			 s->si[sindex].most_advanced_flot == HUGE)
			{
			x_objective = s->p.phase_line;
			}
		else x_objective = s->si[sindex].most_advanced_flot;
	  /*
		* At this point, we are committing a unit to go someplace else. In
		* a better design, the command should go down to the unit and the
		* unit should decide how to move to the objective point and should
		* schedule its own start move.  However, I've decided to circumvent
		* that normal method in this case.
		*/	
		o.behavior_selector = START_MOVE;
	  /*
		* The following if statement prevents a unit in reserve from 
		* moving backwards (disengaging) when assigned to a sector.
		*/
		if (s->rf[i].originating_sector == Reserve)
			orig_sector_flot = HUGE * (double)c_x_multiplier(s);
		else orig_sector_flot = 
			most_advanced_flot_in_sector(s, s->rf[i].originating_sector);
 	  /*
		* IF (you are currently one day's travel distance behind your new
		*     objective)  AND
		*    (you are currently one day's travel distance behind the front
		*     in your current sector)
		* THEN
		* 	  move forward to your new objective
		* ELSE
		*    move back to disengage, then move forward.
		*/
		if ((x_objective - currp.pos.x) * c_x_multiplier(s) > 
				ONE_DAYS_DIV_TRAVEL_DISTANCE  and
			 (orig_sector_flot - currp.pos.x) * c_x_multiplier(s) >
				ONE_DAYS_DIV_TRAVEL_DISTANCE)
			{
			route_to_front_from_behind(s, divindex, currp, x_objective, &o);
			}
		else
			{
			route_to_front(s, divindex, currp, x_objective, &o);
			}
		o.sm.move_mode = Rigid;
		null_vector(&(o.sm.route_list[0].vel));
		msg_delay = determine_oporder_delay(s, divindex);
		tell(s->div[divindex].name, now+msg_delay, 0, sizeof(o.sm), &o);
		}
	clear_reinforcing_list(s);
	}

/*
 * The following function (route_to_front) figure out the route
 * taken by a unit in combat in one sector when it moves to another
 * sector.  Possible bugs include the following:
 * (1) The unit in combat must disengage from the enemy before moving.
 * (2) After arriving at the new sector, the unit in combat must
 *	    reengage the new enemy.
 * These two conditions may be violated due to the way the grid 
 * informs the units of velocity changes and the way combat rules are
 * enforced. 
 */

/*static*/ int route_to_front(s, index, currp, x_objective, o)
state *s;
int index;
Particle currp;
double x_objective;
Output_message *o;
	{
	int sindex;
	double first_x_pos;

   if (s->p.side == Red)
		{
		first_x_pos = dmax(currp.pos.x + (double)ONE_DAYS_DIV_TRAVEL_DISTANCE, 
								x_objective + (double)ONE_DAYS_DIV_TRAVEL_DISTANCE);
		}
	else /* side is blue */
		{
		first_x_pos = dmin(currp.pos.x - (double)ONE_DAYS_DIV_TRAVEL_DISTANCE, 
							   x_objective - (double)ONE_DAYS_DIV_TRAVEL_DISTANCE);
		}
	sindex = (int)s->div[index].sector;
	o->sm.num_legs_in_route = 3;

	o->sm.route_list[2].pos.x = first_x_pos;
	o->sm.route_list[2].pos.y = currp.pos.y;
	o->sm.route_list[2].vel.x = 0.0;
	o->sm.route_list[2].vel.y = 0.0;

	o->sm.route_list[1].pos.x = o->sm.route_list[2].pos.x;
	o->sm.route_list[1].pos.y = s->si[sindex].up_div_loc.y;
	o->sm.route_list[1].vel.x = 0.0;
	o->sm.route_list[1].vel.y = 0.0;

	o->sm.route_list[0].pos.x = x_objective;
	o->sm.route_list[0].pos.y = s->si[sindex].up_div_loc.y;
	o->sm.route_list[0].vel.x = 0.0;
	o->sm.route_list[0].vel.y = 0.0;
	}

/*static*/ int route_to_front_from_behind(s, index, currp, x_objective, o)
state *s;
int index;
Particle currp;
double x_objective;
Output_message *o;
	{
	int sindex;

	sindex = (int)s->div[index].sector;
	o->sm.num_legs_in_route = 2;

	o->sm.route_list[1].pos.x = currp.pos.x;
	o->sm.route_list[1].pos.y = s->si[sindex].up_div_loc.y;
	o->sm.route_list[1].vel.x = 0.0;
	o->sm.route_list[1].vel.y = 0.0;

	o->sm.route_list[0].pos.x = x_objective;
	o->sm.route_list[0].pos.y = s->si[sindex].up_div_loc.y;
	o->sm.route_list[0].vel.x = 0.0;
	o->sm.route_list[0].vel.y = 0.0;
	}

/*static*/ int abort_reinforcements(s)
state *s;
	{
	int i;

  /*
	* IN DEBUGGER CHECK if the unit has been removed from its original
	* sector.  If not, then why add it again?
	*/
	for (i = 0; i < REINFORCING_LIST_SIZE; i++)
		{
		if (s->rf[i].div_name[0] == '\0') continue;
		add_unit_to_sector(s, s->rf[i].div_name, s->rf[i].originating_sector);
		}
	clear_reinforcing_list(s);
	}

/*************************************************************************
 *
 *                       E N E M Y    A R R A Y
 *
 *************************************************************************/
/*
 * The following function adds the name of an enemy to the enemy list
 * in the corp's state.  Only the name of the enemy is entered; information
 * about the strength, flot location, etc. about the enemy is not updated
 * by this routine; rather, the corps must query the enemy to find out 
 * this information.
 */
/*static*/ int add_enemy_name(s, enemy_name)
state *s;
Name_object enemy_name;
	{
	int j, k;

  /*
	* Check to see if the enemy is already known by this corps
	*/
	k = Find(enemy_name, &(s->en[0]), sizeof(struct Enemies),
			MAX_DIV_UNITS_IN_CORPS, String);
  /*
	* If the enemy is not already known, ...
	*/
	if (k == -1)
		{
	  /*
		* Find a blank spot in the enemy database
		*/
		k = find_blank(&(s->en[0]), sizeof(struct Enemies), 
			MAX_DIV_UNITS_IN_CORPS, String);
	  /*
		* If no blank spot is found, issue an out of memory error
		* message
		*/
		if (k == -1)
			{
			corps_error("add_enemy_name: no more room in struct Enemies");
			}      
	  /*
		* else, if space is found, insert the enemy.
		*/
		else strcpy(s->en[k].enemy_name, enemy_name);
		}
	}

/*************************************************************************
 *
 *                      S E C T O R    A R R A Y
 *
 *************************************************************************/
/*
 * Given the name of a division (either Red or Blue, it does not matter),
 * update the sector data structure.
 */
/*static*/ int add_unit_to_sector(s, div_name, sector)
state *s;
Name_object div_name;
enum Sectors sector;
	{
	int i, j, sindex, key = -1;
	enum Sides side;

   sindex = (int)sector;
	which_side(div_name, &side);
  /*
	* The sector data structure maintains two arrays of units inside of
	* it.  One array is for the friendly forces, another array is for the
	* enemy forces.  Each array is an array of integers; each integer is the
	* index into either the Enemies array or the Divisions array.          
	* To update the array, we do the following:
	* Find name of division in appropriate list (friendly or enemy list).
	* Then, stuff the index of the division in the above list into the
	* sector data structure.  
	*/
	if ( (int)side == opp_side(s))
		{
	  /*
		* Find division in enemy array
		*/
		i = Find(div_name, &(s->en[0]), sizeof(struct Enemies),               
				MAX_DIV_UNITS_IN_CORPS, String);
     /*
		* Check to see if unit is already there.
		*/
		j = Find(&i, &(s->si[sindex].enemy_index[0]), sizeof(int),
				MAX_UNITS_IN_SECTOR, integer);
	  /*	
		* If not. . .
		*/
		if (j == -1)
			{
	     /*
			* find a blank location, which is defined as having key = -1.
			*/
			j = Find(&key, &(s->si[sindex].enemy_index[0]), sizeof(int),
					MAX_UNITS_IN_SECTOR, integer);
	     /*
			* If no blank location is found, then print an error.
			*/
			if (j == -1)
				{	
				char errstr[160];

				sprintf("No more space in s->si.enemies array; size is %d\nTrying to add %s",
						MAX_UNITS_IN_SECTOR, div_name);
				corps_error(s, errstr);
				return;
				}
		  /*
			* Else, if a blank location is found, insert it.
			*/
			else
				{
			  /*
				* Fill the blank spot just found.
				*/
				s->si[sindex].enemy_index[j] = i;
				s->si[sindex].num_enemy_units++;
				s->si[sindex].up_enemy_strength += s->en[i].enemy_strength;
				}
			} /* end insert code */
		} /* end if side == opp_side() */
	else
		{
	  /*
		* Find division in friendly array
		*/
		i = Find(div_name, &(s->div[0]), sizeof(struct Divisions),
				MAX_DIV_UNITS_IN_CORPS, String);
     /*
		* Check to see if the division is already in the sector array.
		*/
		j = Find(&i, &(s->si[sindex].friendly_index[0]), sizeof(int),
				MAX_UNITS_IN_SECTOR, integer);
		if (j == -1)
			{
	     /*
			* find a blank location, which is defined as having a key = -1.
			*/
			j = Find(&key, &(s->si[sindex].friendly_index[0]), sizeof(int),
					MAX_UNITS_IN_SECTOR, integer);
	     /*
			* If no blank location is found, then print an error.
			*/
			if (j == -1)
				{	
				corps_error(s, "No more space in s->si[] array.\n");
				return;
				}
		  /*
			* Else, if a blank location is found, insert it.
			*/
			else
				{
			  /*
				* Fill the blank spot just found.
				*/
				s->si[sindex].friendly_index[j] = i;
				s->si[sindex].num_friendly_units++;
			  /*
				* Update the Division data structure 
				*/
				if (s->div[i].role == Up)
					s->si[sindex].up_div_strength += s->div[i].current_strength;
				else
					s->si[sindex].reinforcing_div_strength += 
									s->div[i].current_strength;
				s->div[i].sector = sector;
				}
			} /* end insert code */
		} /* end else (if division is friendly) */
	}

/*static*/ int remove_unit_from_sector(s, div_name, sector) 
state *s;
Name_object div_name;
enum Sectors sector;
	{
	int i, j, sindex;
	enum Sides side;

	sindex = (int)sector;
	which_side(div_name, &side);
	if ( (int)side == opp_side(s))
		{
	  /*
		* Find the division in the enemy array.
		*/
		i = Find(div_name, &(s->en[0]), sizeof(struct Enemies), 
				MAX_DIV_UNITS_IN_CORPS, String);
		if (i == -1) /* bad error */
			{
			char errstr[80];
			sprintf(errstr, "Unit %s not found in enemy division array\n",
				div_name);
			corps_error(s, errstr);
			return;
			}
     /*
		* Find the division in the sector's enemy_index array.
		*/
		j = Find(&i, &(s->si[sindex].enemy_index[0]), sizeof(int),
				MAX_UNITS_IN_SECTOR, integer);
		if (j == -1)  /* unit is not there */ return;
		s->si[sindex].enemy_index[j] = -1;
		s->si[sindex].num_enemy_units --;
		if (s->si[sindex].num_enemy_units < 0) 
			s->si[sindex].num_enemy_units = 0;
	  /*
		* We will assume all enemies are 'Up', or, conversely, that the
		* 'up_enemy_strength' field in the sector array is the total of
		* all enemy strength.
		*/
		s->si[sindex].up_enemy_strength -= s->en[i].enemy_strength;
		if (s->si[sindex].up_enemy_strength < 0.0)
			s->si[sindex].up_enemy_strength = 0.0;
		}
	else
		{
	  /*
		* Find the division in the friendly division array.
		*/
		i = Find(div_name, &(s->div[0]), sizeof(struct Divisions),
				MAX_DIV_UNITS_IN_CORPS, String);
		if (i == -1) /* bad error */
			{
			char errstr[80];
			sprintf(errstr, "Unit %s not found in division array\n",
				div_name);
			corps_error(s, errstr);
			return;
			}
     /*
		* Find the division in the sector's friendly_index array.
		*/
		j = Find(&i, &(s->si[sindex].friendly_index[0]), sizeof(int),
				MAX_UNITS_IN_SECTOR, integer);
		if (j == -1)  /* unit is not there */ return;
		s->si[sindex].friendly_index[j] = -1;
		s->si[sindex].num_friendly_units --;
		if (s->si[sindex].num_friendly_units < 0)
			s->si[sindex].num_friendly_units = 0;
		if (s->div[i].role == Up)
			{
			s->si[sindex].up_div_strength -= s->div[i].current_strength;
			if (s->si[sindex].up_div_strength < 0.0)
				s->si[sindex].up_div_strength = 0.0;
			}
		else
			{
			s->si[sindex].reinforcing_div_strength -= s->div[i].current_strength;
			if (s->si[sindex].reinforcing_div_strength < 0.0)
				s->si[sindex].reinforcing_div_strength = 0.0;
			}
		}
	}
			
		

/*************************************************************************
 *
 *                    D I V I S I O N    A R R A Y
 *
 *************************************************************************/
/*
 * ARRAY MANAGEMENT: The following routines manage the s->div[] array
 * in the state.
 */
/*static*/ int add_div_unit_to_array(s, div_name, sector, role)
state *s;
Name_object div_name;
enum Sectors sector;
enum Roles role;
	{
	int i;

	i = find_blank(&(s->div[0]), sizeof(struct Divisions), 
			MAX_DIV_UNITS_IN_CORPS, String);
	if (i == -1)
		{
		corps_error(s, "no more room in s->div[] array.");
		return;
		}
	strcpy(s->div[i].name, div_name);
	s->div[i].role = role;
	s->div[i].sector = sector;
	s->div[i].p.pos = s->si[(int)sector].up_div_loc;
	}

/*static*/ int del_div_unit_from_array(s, div_name)
state *s;
Name_object div_name;
	{
	int i; 

	i = clearit(div_name, &(s->div[0]), sizeof(struct Divisions),
			MAX_DIV_UNITS_IN_CORPS, String);
	if (i == -1)
		{
		char errstr[60];
		sprintf(errstr, "division %s not found in div array\n", div_name);
		corps_error(s, errstr);
		}
	}



/*************************************************************************
 *
 *                  D E B U G    R O U T I N E S 
 *
 *************************************************************************/
/*
 * The following routines are useful when combined with dbxtool.
 */

/*static*/ int dump_divisions(s)
state *s;
	{
	int i;

	for (i = 0; i < MAX_DIV_UNITS_IN_CORPS; i++)
		{
		if (s->div[i].name[0] == '\0') continue;
		printf("div[%d]: name %s str %.2lf flot %.2lf xpos %.2lf ypos %.2lf\n",
				i, s->div[i].name, s->div[i].current_strength, s->div[i].flot,
				s->div[i].p.pos.x, s->div[i].p.pos.y);
		}
	}

/*static*/ int dump_enemies(s)
state *s;
	{
	int i;

	for (i = 0; i < MAX_DIV_UNITS_IN_CORPS; i++)
		{
		if (s->en[i].enemy_name[0] == '\0') continue;
		printf("s->en[%d]: %s str %.2lf flot: x %.2lf y %.2lf\n",
			i, s->en[i].enemy_name, s->en[i].enemy_strength,
			s->en[i].enemy_flot.pos.x, s->en[i].enemy_flot.pos.y);
		}
	}

/*static*/ int dump_sectors(s)
state *s;
	{
	int i;

	for (i = 0; i < 4; i++)
		{
		printf("sector %d (string %s)\n", s->si[i].sector, s->si[i].sector);
		printf("   num enemies %d  num friends %d\n",
			s->si[i].num_enemy_units, s->si[i].num_friendly_units);
		}
	}


