/*
 * stat.c - Fred Wieland, Jet Propulsion Laboratory, March 1988
 *
 * Code for the statistics objects, which has two instances, one
 * for Red and one for Blue.
 */

/* Fixed bug in message format 	01/31/89  Philip Hontalas	*/

#define UPDATE_STATS	220
#define DRAW_GRAPHICS	230
 

typedef struct Update_stats
	{
	Int code;
	Name_object name;
	enum Sides side;
	double pct_strength;
	Name_object gridloc;
	int is_engaged;
        enum Sectors sector;
        enum Postures posture;
	} Update_stats;

typedef struct Draw_graphics
	{
	Int code;
	} Draw_graphics;
