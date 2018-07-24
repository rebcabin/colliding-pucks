/*
 * divdefs.h - Fred Wieland, Jet Propulsion Laboratory
 *
 * This file contains the data structure definitions for the ADT's
 * contained in the state of the division object.
 */

/* following moved to ctls88.h */
/* #define DIRECT_ENGAGE_RANGE 5  /* in km */


/*
 * The Environment ADT contains information about the current event.
 * Relevant information includes the current simulation time, number
 * of events schedules, object name, etc.
 */

typedef struct Environment
	{
  /*
	* stuff to manage each event 
	*/
	Name_object myself;
	Int number;
  /*
	* Unit parameters
	*/
	enum Sides side;
	enum Postures posture;
	enum Roles role;
	enum Defense defend_submode; /* Hasty or Deliberate */
	Name_object superior_corps;
	Simulation_time time_in_hasty_defense;
	double current_strength;
	double full_up_str;
	double current_combat_power;
	Simulation_time time_last_spot_rpt_sent;
	} Environment;

/*
 * The Move ADT contains information about position, mode of movement,
 * and geometry of the division.  The geometry is relatively involved.
 *
 *                      25 km                     5km
 *        <--------------------------------> <------------->          
 *        __________________________________________________    
 *        |                                                |  ^
 *        |                                                |  | FLOT:
 *        |                                CP              |  | 8, 16, 24,
 *        |                                                |  | or 32 km
 *        |________________________________________________|  v North-South
 *
 * The division contains a center point (CP) which the grid object keeps
 * track of.  It is modelled as a rectangle, with a FLOT (forward line
 * of troops) for divisions that are on the front.  The FLOT is 5 km
 * in front of the CP, and the rear is 25 km behind the CP.  In battle,
 * the flot can move without the CP moving. The north-south dimension of
 * the division can be 8, 16, 24, or 32 km, depending on the division's
 * mission, status, and role.
 */

typedef struct Move
	{
  /*
	* The sign of the flot_offset means something different for Red and     
	* Blue. If the flot_offset is positive, then the Red flot is behind
	* the Red center point; for Blue, it would mean that the blue flot is
	* in front of the Blue centerpoint.  A similar argument holds for a
	* negative flot_offset.
	*/
	double flot_offset;  /* this is always POSITIVE, for both RED and BLUE */
	double incr_flot_move_dist_1;  /* > 0 for BLUE, < 0 for RED */
	double incr_flot_move_dist_2;  /* >0 for BLUE, < 0 for RED */
	enum modes move_mode;  /* NOT_MOVING, RIGID, or REAR */
	Simulation_time move_start_time;
	Line  north_boundary; 
	Line south_boundary;  
	int num_subsectors_in_flot;
	int movement_key;
	Name_object owning_grid;
	Vector rear;  /* two components are rear.x and rear.y */
	Particle p;  /* where the centerpoint of the unit is located */
	Simulation_time last_p; /* last time p was calculated */
	Particle objective_point;
	int pradius;
	} Move;

/*    
 * The Battle ADT contains all information relative to ongoing engagements.
 */

typedef struct Battle_data
	{
	Name_object EnemyName;
	double my_prev_flot_offs;
	double my_prev_strength_ratio;
	int my_battle_key;
	Simulation_time battle_began;
	} Battle_data;

typedef struct Battle
	{
	Battle_data bd[MAX_NUM_BATTLES];
	Simulation_time last_suffer_losses; /* used in send_spot_report */
	} Battle;

typedef struct CombatADT
	{
	Combat_systems cs[MAX_CS], css[MAX_CSS];
	} CombatADT;

/* 
 * The intel ADT contains all information regarding intelligence.
 */

#define MAX_OTHER_UNITS 15
typedef struct Other_unit_db
	{
   Name_object UnitName;
	Particle where;
	Simulation_time time_of_sight;
	int other_unit_sighted_flag;
	Simulation_time time_to_engage;
	} Other_unit_db;

typedef struct intel 
	{
	Other_unit_db intel[MAX_OTHER_UNITS];
	} intel;

/*
 * The Graphics ADT contains information about the
 * latest graphical output produced by the object.
 * This allows the object to "erase" the last graphic 
 * before producing a new one.
 */
typedef struct Graphics
	{
	Vector n1, n2, s1, s2, f1, f2;
	} Graphics;

/*
 * The Flags ADT contains all relevant flags for the event. Flags
 * are holding places for variables that must go into messages in the
 * post_process routine.
 */

typedef struct Flags
	{
	int start_move;
	int continue_move;
	Simulation_time move_time;  /* receive time for move order message */
  /*
	* The following are for the move_order message.
	*/
	Particle route_list[MAX_NUM_LEGS]; /* defined in divmsg.h */
   int num_legs_in_route;	
	int trace;  /* ON if you want a trace of the event */
	int changed_velocity;
	int send_ca;
	int strength_changed;
	int posture_changed;
	int no_activity;
	int graphics;
	int redraw_flag;
  /*
	* The following are for the assess_combat message. num_battles
	* is set to the number of battles for which you want to send an
	* assess_combat message during this event; the array which_battle
	* contains a pointer (index) to the information necessary to format
 	* the assess_combat message. The pointer is pointing to an array
	* element in ou in the intel structure.
	*/
	int num_battles;
	int which_battle[MAX_NUM_BATTLES];
  /*
	* The following is for the initialize battle message.
 	*/
	int initialize_battle; 
	Name_object EnemyName;
	Simulation_time When; /* to send the init battle msg */
	} Flags;
