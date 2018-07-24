/*
 * corpsdefs.h - Fred Wieland, Jet Propulsion Laboratory
 *
 * This file contains the data structures for the corps object.
 */

typedef struct CEnvironment
	{
	Name_object myself;
	Int number;
	} CEnvironment;

typedef struct Parameters
	{
	int corps_num;
	int first_div_num;
	int last_div_num;
	int units_in_corps;
	int sectors_in_corps;
	enum Postures posture;
	enum Sides side;
	double phase_line;
	int formation;
	enum Sectors attack_sector;
	Simulation_time attack_sector_t;
	} Params;

typedef struct Movement
	{
 	Particle p;
	int front_offset;
	} Movement;

typedef struct Formation
	{
	enum Sectors sector;
	int units_in_sector;
	int sector_width;
	} Formation;

typedef struct Divisions
	{
	Name_object name;
	double flot;
	double full_up_strength;
	double current_strength;
	Particle p;
	Simulation_time last_report;
	enum modes move_mode;
	enum Postures posture;
	enum Roles role;
	enum Sectors sector;
	int is_engaged;
	} Divisions;

typedef struct Enemies
	{
	Name_object enemy_name;
	double enemy_strength;
	Particle enemy_flot;
	int enemy_up; /* TRUE or FALSE */
	} Enemies;

typedef struct Reinforcements
	{
	Name_object div_name;
	enum Sectors originating_sector;
	enum Sectors destination_sector;
	} Reinforcements;

typedef struct Reserves
	{
	int num_units_in_reserve;
	int reserve_index[MAX_UNITS_IN_SECTOR];
	} Reserves;

typedef struct Sector_info
	{
	enum Sectors sector;
	int at_objective;
	int num_enemy_units;
	int num_friendly_units;
  /*
	* Each element in the following two arrays is a pointer to and element in
	* another array.  Specifically, each element is an index to the enemy
	* information in the s->e[] array.
	*/
	int enemy_index[MAX_UNITS_IN_SECTOR]; /* index of enemy in Enemies array */
	int friendly_index[MAX_UNITS_IN_SECTOR]; /*index of unit in Divisions array*/
	double reinforcing_div_strength;
	enum Postures sector_posture;
	double most_advanced_flot;
	double up_enemy_strength;
	double up_div_strength;
	Vector up_div_loc;
	} Sector_info;

typedef struct Miscellaneous
	{
	int next_avail_divnum;
	Vector corps_top, corps_bottom; /* for pad graphics */
	int late_div_sitreps;
	} Miscellaneous;

typedef struct CFlags
	{
	int evaluate_objectives;
	int draw_graphics;
	int wrote_corps_boundaries;
	} CFlags;

	
	

