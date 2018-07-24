/*
 * Distributor.c - Fred Wieland Jet Propulsion Laboratory December 1987
 *
 * This object takes initial grid coordinates from the division and 
 * calculates what grid cell the division is in.  It sends a CHANGE_LOCATION
 * message to the division, informing it of the grid cell location, and
 * an ADD_UNIT message to the grid cell, informing it about the division.
 * Its input is an ADD_UNIT message.
 */

#include "twcommon.h"
#include "motion.h"
#include "stb88.h"
#include "gridmsg.h"
#include "divmsg.h"
#include "distributor.h"

#define tprintf if(0==1)printf
#define gprintf if(0==1)printf

typedef struct state
	{
	Int number;
	} state;

typedef union Output_Message
	{
	Int behavior_selector;
	Change_location cloc;
	Add_unit aunit;
	} Output_message;

typedef Output_message Input_message;

/************************************************************************** 
 *
 *         O B J E C T    T Y P E    D E F I N I T I O N
 *
 **************************************************************************
 These definitions are necessary for the Time Warp Operating System, and
 are not needed for the STB88 algorithms.
 */
ObjectType distribType = {"distributor", i_DISTRIB, e_DISTRIB, t_DISTRIB, 0, 0,
				 sizeof(state),0,0 };

init()
	{
	}

event()
	{
	Output_message o;
	Input_message m;
	double frac_y, frac_x, frac_obj_y, frac_obj_x;
	int i, x_grid, y_grid;
	Int bread, error;
	Name_object who, grid;
	state *s = (state *)myState;

   s->number = numMsgs;
	gprintf("pad:setfont cour.b.24\n");
	frac_y = (double) 1.0 / NUM_NS_GRIDS;
	frac_x = (double) 1.0 / NUM_EW_GRIDS;
	tprintf("NUM_NS_GRIDS = %d, NUM_EW_GRIDS = %d\n", 
		NUM_NS_GRIDS, NUM_EW_GRIDS);
	tprintf("frac_y = %lf, frac_x=%lf\n", frac_y, frac_x);

   for (i = 0; i < s->number; i++)
		{
		m = * (Input_message *) msgText(i);
		strcpy(who, m.aunit.UnitName);
		frac_obj_y = (double) m.aunit.where.pos.y / NS_BOARD_SIZE;
		frac_obj_x = (double) m.aunit.where.pos.x / EW_BOARD_SIZE;
		tprintf("frac_obj_y = %lf, frac_obj_x = %lf\n", frac_obj_y,
			frac_obj_x);
		y_grid = (int)(frac_obj_y / frac_y);
		x_grid = (int)(frac_obj_x / frac_x);
		sprintf(grid, "G%d,%d", x_grid, y_grid);
		tprintf("Object %s at (%.2lf, %.2lf) placed in grid %s\n",
			who, m.aunit.where.pos.x, m.aunit.where.pos.y, grid);
		tell(grid, now+( 2 * SIM_TIME_SCALE ), 0, sizeof(m.aunit), &m);
		clear(&o, sizeof(o));
		o.behavior_selector = CHANGE_LOCATION;
		strcpy(o.cloc.whereto, grid);
		tell(who, now+( 2 * SIM_TIME_SCALE ), 0, sizeof(o.cloc), &o);
		}
#	if 0
	o.behavior_selector = TRACE_ON;
	tell("red_div6/9", 2188, 0, sizeof(o.behavior_selector), &o);
#	endif
	clear(&o, sizeof(o));
	}

query()
{}
