/*
 * divmsg.h - Fred Wieland, Jet Propulsion Laboratory, January 1988
 *
 * This file contains data structure definitions for input messages
 * to the division object.
 */

#define INITIALIZE_DIV	6
#define				INIT_DIV				61

#define MOVE				7
#define				START_MOVE		 	71
#define				CONTINUE_MOVE	 	72
#define				HALT				 	73
#define				CHANGE_LOCATION 	74

#define FIGHT				8
#define				INITIATE_BATTLE 	81
#define				ASSESS_COMBAT	 	82
#define				SUFFER_LOSSES	 	83



#define INTEL			9
#define				PERCEIVE			91	

#define COMMUNICATE 	11	
#define				OP_ORDER				111 /* received from corps */
#define				SPOT_REPORT			112 /* sent to corps       */

#define PLANNING 		12	
#define				EVALUATE_SITUATION	121


/*PJH added for query removal	*/
#define QUERY_FOR_UNIT_PARAMS	13
#define				SEND_UNIT_PARAMS  131	
#define				UNIT_PARAMS			132	

#define GRAPHICS		14	
#define				REDRAW_SELF			141

#define MAX_NUM_LEGS 4

/* enum modes { Rigid, Rear, Not_moving }; moved to ctls88.h */
enum Defense { Hasty, Deliberate };

typedef struct Init_div
	{
	Int code;
	Name_object div_name;
	enum Postures posture;
	Vector objective;
	Vector init_pos;
	enum Sides side;
	Name_object corps_superior;
	int sector_width;
	int pradius;
	enum Roles role;
	} Init_div;

/*
 * In the Start_move and Continue_move data structures, there is a
 * variable called 'route_list.'  This variable holds a list of
 * (x, y) coordinates which the division will move through.  Each
 * element in this list is called a 'leg.'  The array is implemented
 * as a stack: the last element in the array is the first leg of the
 * route, and the first element in the array is the last leg of the 
 * route.  The route list is not destroyed as the division moves;
 * rather, the variable num_legs_in_route is decremented so that it
 * always points to the next leg.
 */
typedef struct Move_order
	{
	Int code;
	int movement_key;
	Particle route_list[MAX_NUM_LEGS];
	int num_legs_in_route;
	enum modes move_mode;  /* RIGID or REAR */
	} Move_order;

/*
 * The continue_move and start_move messages are identical; they
 * are both represented by the move_order structure above.
 *	typedef struct Continue_move
		{
		Int code;
		int movement_key;
		Particle route_list[MAX_NUM_LEGS];
		int num_legs_in_route;
		enum modes move_mode; 
		} Continue_move;
*/

typedef struct halt_move
	{
	Int code;
	} Halt;

typedef struct Change_location
	{
	Int code;
	Name_object whereto;
	} Change_location;

typedef struct Perception
	{
	Int code;
	Name_object UnitName;
	Particle where;
	Simulation_time time;
	} Perception;

/* 
 * The following message structures are for the fight 
 * process.
 */

typedef struct Initiate_battle
	{
	Int code;
	Name_object UnitName;
	int enemy_sighted_flag;
	} Initiate_battle;

typedef struct Unit_parameters  /*PJH changed from query to evtmsg  */
	{
	Int code;
	Name_object UnitName;
	Line north_bound;
	Line south_bound;
   Particle flot_position;
	Particle cp;  /* centerpoint pos and vel for enemy */
	double force_strength;
	double direct_fire_range;
	enum Postures posture;
	enum Sides side;
	int assignment;
	int engaged_flag;
	int enemy_sighted_flag;  /*???PJH	*/
	int sector_i;		/* This is for the interaction with corps */
	int enemy_j;
	int index;
	} Unit_parameters;


typedef struct Send_Unit_parms  /*PJH added for query removal	*/
	{
	Int code;
	Name_object  UnitName;	/* requestor	*/
	int sector_i;	/* This is for the interaction with corps */
	int enemy_j;
	int index;
	}Send_Unit_parms;


enum Msgsource {Init, Suffer};

typedef struct Combat_assess
	{
	Int code;
	Name_object EnemyName;
	double pct_sys_to_alloc;
	enum Msgsource source_is;  /* INIT or SUFFER */
	int combat_key;
	} Combat_assess;

typedef struct Combat_systems
	{
	enum Sysnames name;
	double oper;  /* number currently operational */
	double mtoe;  /* maximum number unit can have */
	double strength;
	} Combat_systems;

/*
 * We have two types of systems at each division: cs, which is combat
 * service, and css, which is combat service support.  The cs systems
 * are the only ones which can attrit other systems, so in the suffer
 * message, the firing unit sends information about its cs systems 
 * only.   The css systems suffer attrition by direct fire from enemy
 * cs systems, but do not cause any attrition themselves.
 */
typedef struct Suffer_attrition
	{
	Int code;
	Name_object UnitName; /* name of sender of Suffer_attrition msg */
	Combat_systems cs[MAX_CS]; 
	Line north_boundary, south_boundary;
	double flot_xloc, rear_position;
	Particle p;
	enum Postures posture;
	double prev_strength_ratio;
	} Suffer_attrition;

typedef struct Op_order
	{
	Int code;
	enum Postures posture;
	double objective;
	Particle movement_path[MAX_NUM_LEGS];
	} Op_order;

	
