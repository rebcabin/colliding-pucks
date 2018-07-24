/*
 * ctls88_1: Fred Wieland Jet Propulsion Laboratory October 1987
 *			 contains constants used throughout the ctls code
 *
 * CREATED:		November, 1987
 * MODIFIED:	
 */

/* GRAPHICS PARAMETERS:
   1. Indicate whether you want graphics on or off by appropriate commenting
      of the following definitions:
*/

/*#define TURN_ON_GRAPHICS   /* Leave in if you want PAD graphics */
#define TURN_OFF_GRAPHICS  /* Leave in if you don't want PAD graphics */

/* #define TURN_ON_STATISTICS  /* red and blue stats objects */

#define QUERY_TIME_INC	1    /* Used for De-queried version	*/
#define SIM_TIME_SCALE	10   /* Used for De-queried version	*/

#define CUTOFF_TIME  (50000 * SIM_TIME_SCALE )
		  /* maximum virtual time for self-propelled objs */

/**************!!!!!!!!!!!!!!
 * If you change the following line to a short int, then all of the
 * calls to the Find and Find_blank function in xcorps.c must have their
 * last argument changed to a Shortint.
 **************!!!!!!!!!!!!!!*/
/* typedef int int; in my own copy of twcommon.h */

/*
   The following 6 constants are used to define the size of the gameboard. 
   Standard cartesian coordinates are used, with (0,0) at the SW part of
   the battlefield. These constants are input parameters to the game, and
   ONLY THESE 6 need be changed to change the size of the playing field.
*/

/*  
	INVARIANT: The NW_SEPARATION must divide into the NORTH_BOUND evenly.
 	           Similarly, the EW_SEPARATION must divide into the EAST_BOUND
				  evenly.
*/
#if 1
#define SOUTH_BOUND  	0			/* Playing field extent is from 				*/
#define NORTH_BOUND  	2120		/* coordinates (SOUTH_BOUND, WEST_BOUND)	*/
#define WEST_BOUND  		0			/* to (NORTH_BOUND, EAST_BOUND).				*/
#define EAST_BOUND		400		/*														*/

#define NS_SEPARATION 106  /* in the same units as the BOUNDs above */
#define EW_SEPARATION 400
#endif
#if 0
#define SOUTH_BOUND  	0			/* Playing field extent is from 				*/
#define NORTH_BOUND  	1600		/* coordinates (SOUTH_BOUND, WEST_BOUND)	*/
#define WEST_BOUND  		0			/* to (NORTH_BOUND, EAST_BOUND).				*/
#define EAST_BOUND		2000		/*														*/

#define NS_SEPARATION 400  /* in the same units as the BOUNDs above */
#define EW_SEPARATION 500
#endif

/*
   The following constants are used to define the size of a grid cell.
   The minimum size is 20 km; the maximum size is the size of the
   battlefield.
 */

#define NS_BOARD_SIZE  (NORTH_BOUND-SOUTH_BOUND)
#define EW_BOARD_SIZE  (EAST_BOUND-WEST_BOUND)

#define NUM_NS_GRIDS  (NS_BOARD_SIZE/NS_SEPARATION)
#define NUM_EW_GRIDS  (EW_BOARD_SIZE/EW_SEPARATION)
#define NUM_Y_GRIDS	NUM_NS_GRIDS
#define NUM_X_GRIDS	NUM_EW_GRIDS
#define GRID_AREA		(NS_SEPARATION*EW_SEPARATION)
#define TOTAL_GRIDS (int)(NUM_NS_GRIDS*NUM_EW_GRIDS)

/* parameters for units */

#define NUM_RED_DIVS  180  /* 90 */
#define NUM_BLUE_DIVS 126  /* 63 */
#define MAX_DIVS 180 /* used to size an array in stat.c */
#define NUM_RED_CORPS 18
#define NUM_BLUE_CORPS 18

#define TOTAL_REDS (NUM_RED_DIVS+NUM_RED_CORPS)
#define TOTAL_BLUES (NUM_BLUE_DIVS+NUM_BLUE_CORPS)
#define TOTAL_UNITS (TOTAL_REDS+TOTAL_BLUES)
#define TOTAL_DIVS (NUM_RED_DIVS+NUM_BLUE_DIVS)
#define TOTAL_CORPS (NUM_RED_CORPS+NUM_BLUE_CORPS)

/* #define UNITS_PER_GRID ((int)(1.1*TOTAL_UNITS)/TOTAL_GRIDS) */
/* #define UNITS_PER_GRID 18 moved to grid.c */

/* The following constants are used for the fight process. */

#define RED_ATTACK_MULTIPLIER 1.3
#define BLUE_ATTACK_MULTIPLIER 1.3

#define RED_DEFEND_MULTIPLIER 0.9
#define BLUE_DEFEND_MULTIPLIER 0.9

#define ATTACK_DEFEND_THRESH  0.8
#define ATTACK_POSTURE_THRESH 0.8
#define DEFEND_POSTURE_THRESH 0.6
#define WITHDRAW_POSTURE_THRESH 0.4

#define DIRECT_ENGAGE_RANGE 15  /* in km */
/*
 * Maximum number of battles a particular division can be in
 * simultaneously.
 */
#define MAX_NUM_BATTLES 15 /* used here and in divmsg.h */

/* scale factor for graphics display */

#define SCALE_FACTOR 2

/* miscellaneous constants */

#define NORTH 	0  /* these constants can be used as array indices to */
#define SOUTH 	1  /* figure direction.											*/
#define EAST 	2
#define WEST 	3

#ifdef NE
#undef NE
#endif

#define SW	0  	/* more directional constants to use as array indices */
#define NE	1
#define NW  2
#define SE  3

#define RED 0
#define BLUE 1

/* Other model parameters */

#define LCAP 60  /* lanchester attrition coefficient */

/* message format used by every object for message identification */

typedef struct Selector
	{
	Int code;
	} Selector;

enum Postures {Attack, Defend, Withdraw, Destroyed};
enum Roles {Reserved, Reinforcing, Up};
enum Sectors {Reserve, Sector1, Sector2, Sector3};
enum Sides {Red, Blue};
enum Compass {North, South, East, West, Sw, Ne, Nw, Se};
enum modes { Rigid, Rear, Not_moving };

/* Data for division data structures, shared by corps. Used in file
   sysdata.h             */

enum Sysnames {M60A1, T_62, Util_truck, Five_ton_truck, Fred0, Fred1,
					Fred2, Fred3, Fred4, Fred5, Fred6, Fred7, Troop_carrier,
					APC };

/*
 * The following six parameters are linked to the file sysdata.h.
 */
#define MAX_CS   10  /* Combat Systems; they can fire weapons */
#define MAX_CSS  4  /* Combat Service Support; cannot fire weapons */

#define MAX_BLUE_CS 5
#define MAX_RED_CS 5

#define MAX_BLUE_CSS 2
#define MAX_RED_CSS 2


/* behaviors common to all objects */

#define DEBUG 		10
#define GRAPH_ON  101
#define GRAPH_OFF 102
#define TRACE_ON	103
#define TRACE_OFF	104

typedef Vtime Simulation_time;

#ifdef POSINF
#undef POSINF
#define POSINF 0x7FFFFFFFL  /* positive infinity for simulation time */
#endif



#ifdef TRUE
#undef TRUE
#endif
#ifdef FALSE
#undef FALSE
#endif

#define TRUE 1
#define FALSE 0

/* C style definitions */

#define and &&
#define or ||
#define not !

