/****************************************************************/
/*  pucktypes.h					07-25-89	*/
/*  								*/
/* Constants and Data Structures for the poolpucks program	*/
/*								*/
/****************************************************************/
#ifdef SIMULATOR
#define _pprintf printf
#endif

#define CONFIG_MSG		0
#define PUCK_START		1
#define CUSHION_START		2
#define SECTOR_START		3
#define INITIAL_VELOCITY	4
#define CANCEL_ACTION		5
#define COLLIDE_PUCK		6
#define COLLIDE_CUSHION		7
#define ENTER_SECTOR		8
#define DEPART_SECTOR		9
#define CHANGE_VELOCITY		10
#define NEW_TRAJECTORY		11
#define UPDATE_SELF		12


#include "circles.h"

/*???PJH the following 3 constants should be changed to 	*/
/*	 variables set in the configuration file.		*/
/*
#define AUTO_UPDATE	1 
#define DISPLAY	1 		*/

/* #define IRIS		1 */
/* #define THERMAL_LOG	1 */

#define TIME_FUDGE 0.999999999
#define CHEAT_CONST 0.001

#define PUCK_UPDATE_INCREMENT  1L
#define SECTOR_UPDATE_INCREMENT  2L
#define CUSHION_UPDATE_INCREMENT 10L
#define CUT_OFF 400

#define MAX_PLANS 90
#define MAX_NUMBER_OF_MESSAGES 300
#define MAX_SECTORS 16
#define MAX_THERMAL_RECORDS 910
#define MAX_CUSHIONS 4
#define MAX_BOUNDARIES 4
#define MAX_LIST_SIZE 100


/*	Direction definitions for determining neighbors 	*/
/*	and borders.						*/

#define WEST	0
#define NORTH	1
#define EAST	2
#define SOUTH	3

#define POSITIVE	1
#define NEGATIVE	-1


/* This structure is used for messages to update a puck's	*/
/* movement along the table for non-collision messages		*/


typedef struct infoMsgStruct
{
    Name puck_name;
    Circle Puck_State;
}
  infoMsg;

/*  This structure is used for Enter and Depart Sector msgs	*/
/* This structure is used for collision  messages (either with	*/
/* pucks or cushions ). 					*/

typedef struct actionMsgStruct
{
    Name with_whom;
    int action_id;
    VTime action_time;
    Circle Puck_State;
}
  actionMsg;

typedef struct unActMsgStruct
{
    Name with_whom;
    int action_id;
}
  unActMsg;

/* Pucks in this simulation must keep track of their planned	*/
/* actions with other objects.				*/

typedef struct PlanTypeStruct
{
    Name with_whom;
    int action_id;
    VTime action_time;
    int available;
}
 PlanType;

/*  Sectors must keep track of which pucks are inside and	*/
/*  pucks must keep track of which sectors they are in.		*/

typedef struct PuckSectorList
{
    Name element;		/* used for pucks and sectors */
    int available;
}
 List;

/*  Sectors must keep track of who its neighbors are.		*/


typedef struct BoundaryInformation
{
    LineSegment line;
    Name neighbor;
    int direction;

} BoundInfo;


typedef struct Therm
{
    VTime time;
    double mass;
    Vector velocity;
} Thermal_Record;
