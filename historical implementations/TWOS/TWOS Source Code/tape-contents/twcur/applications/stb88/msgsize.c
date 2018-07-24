#define SUN3
#include "twcommon.h"
#include "motion.h"
#include "stb88.h"
#include "divmsg.h"
#include "gridmsg.h"
#include "corpsmsg.h"
#include "divdefs.h"
#include "corpsconst.h"
#include "corpsdefs.h"
#include "divconst.h"

/*     
 * Division state
 */
typedef struct Divstate
	{
	Move m;  
	Battle b;
	intel i;
	Flags f;
	Environment e;
	CombatADT combat;
	Graphics g;
	} Divstate;

typedef struct Corpsstate1
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
	} Corpsstate;

#define OBJLEN 8
typedef char Obj_name[OBJLEN];  /* to save space; need not 20 chars */
#define MAX_EVAL_ARRAY 10  
#define MAX_HIST_ARRAY 5
#define MAX_PERCEIVE 20
typedef struct PerHist
	{
	int index; /* this is an index into the s->Units[] array in the state */
	int ismarked;
	} PerHist;
typedef struct History
	{
	int whichEvent;
	Simulation_time when;
	} History;

#define UNITS_PER_GRID  21   /* from grid.c */

typedef struct Gridstate1
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
	int message[20];
	} Gridstate;

main()
	{
  /*
	* Grid messages
	*/
	printf(" Initialize\t%d\n", sizeof(Initialize));
	printf(" Add_unit\t%d\n", sizeof(Add_unit));
	printf(" Del_unit\t%d\n", sizeof(Del_unit));
	printf(" Eval_move\t%d\n", sizeof(Evaluate));
	printf(" Change_velocity\t%d\n", sizeof(Change_velocity));
  /*
	* Corps messages
	*/
	printf(" Init_corps\t%d\n", sizeof(struct Init_corps));
	printf(" Evaluate_objectives\t%d\n", 
				sizeof(struct Evaluate_objectives));	
	printf(" Spot_report\t%d\n", sizeof(struct Spot_report));
  /*
	* Division messages
	*/
	printf(" Init_div\t%d\n", sizeof(struct Init_div));
	printf(" Move_order\t%d\n", sizeof(struct Move_order));
	printf(" halt_move\t%d\n", sizeof(struct halt_move));
	printf(" Change_location\t%d\n", sizeof(struct Change_location));
	printf(" Perception\t%d\n", sizeof(struct Perception));
	printf(" Initiate_battle\t%d\n", sizeof(struct Initiate_battle));
	printf(" Unit_parameters\t%d\n", sizeof(struct Unit_parameters));
	printf(" Combat_assess\t%d\n", sizeof(struct Combat_assess));
	printf(" Suffer_attrition\t%d\n", sizeof(struct Suffer_attrition));
	printf(" Op_order\t%d\n", sizeof(struct Op_order));
  /*
	* States
	*/
	printf(" Division state\t%d\n", sizeof(Divstate));
	printf(" Corps state\t%d\n", sizeof(Corpsstate));
	printf(" Grid state\t%d\n", sizeof(Gridstate));
	}
