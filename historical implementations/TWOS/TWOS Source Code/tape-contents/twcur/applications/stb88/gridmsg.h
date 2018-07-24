/*
 * gridmsg.h - Fred Wieland, Jet Propulsion Laboratory, November 1987
 *
 * Data structures for the grid object in Ctls88_1.
 *
 * CREATED:		November, 1987
 * MODIFIED:
 */

#define INITIALIZE 		150
#define ADD_UNIT		160	
#define DELETE_UNIT		170	
#define EVALUATE		180	
#define CHANGE_VELOCITY		190	
#define REPORT_ON		200	
#define REPORT_OFF		210	

typedef struct Initialize
	{
	Int code;
	int grid_num;  /* number of grid unit you are initializing */
  	} Initialize;

typedef struct Add_unit
	{
	Int code;
	Name_object UnitName;   /* name of unit we're adding */
	Particle where;			/* components are where.pos and where.vel */
	Name_object GridLoc;    /* if NULL, then this is initial placement */
	int pradius;				/* perception radius */
	} Add_unit;

typedef struct Del_unit
	{
	Int code;	
	Name_object UnitName;	/* name of object we're deleting */
	} Del_unit;

typedef struct Eval_move
	{
	Int code;
	Name_object UnitName;
	Simulation_time whensent;
	} Evaluate;

typedef struct Change_velocity
	{
	Int code;
	Name_object UnitName;
	Particle p;
	int pradius;
	} Change_velocity;
