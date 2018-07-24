/*
 * corpsmsg.h - Fred Wieland, Jet Propulsion Laboratory, January 1988
 * code for the data structures in the corps object.
 */

#define COMMAND_AND_CONTROL	2
#define								EVALUATE_OBJECTIVES	21
#define CINTEL			3
#define								CORPS_INTEL				31
#define COMMUNICATIONS	4
#define								DIV_SPOT_REPORT		41
#define CORPS_MOVE	5

typedef struct Init_corps
	{
	Int code;
	int formation_num;
	double phase_line;
	enum Postures posture;
	int corps_num;
	int sectors_in_corps;
	int side;
	} Init_corps;

typedef struct Evaluate_objectives
	{
	Int code;
	} Evaluate_objectives;

typedef struct Spot_report
	{
	Int code;
	Name_object div;
	double flot;
	double current_str;
	double full_up_str;
	Particle p;
	enum Postures posture;
	int in_battle;  /* TRUE if division is currently fighting */
	Name_object enemies[MAX_NUM_BATTLES]; /* list of engaged enemies */
	} Spot_report;


