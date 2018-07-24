/* 
 * Newinit.c - easier way of initializing CTLS game board
 * F. Wieland
 * Jet Propulsion Laboratory
 * August, 1987
 */

#include "twcommon.h"
#include "stb88.h"
#include "motion.h"
#include "divmsg.h"
#include "gridmsg.h"
#include "corpsmsg.h"
#include "data.h"  /* <-- defines init_data data structure */
#include "init.h"


#define tprintf if(1==1)printf

typedef union Output_Message
   {
	int behavior_selector;
	Add_unit a;
   } Output_Message;

typedef struct state
   {
   int empty;
   } state;

/************************************************************************** 
 *
 *         O B J E C T    T Y P E    D E F I N I T I O N
 *
 **************************************************************************
 These definitions are necessary for the Time Warp Operating System, and
 are not needed for the STB88 algorithms.
 */
ObjectType initType = {"initializer", i_INITIALIZER, e_INITIALIZER,
			 t_INITIALIZER, 0, 0, sizeof(state),0,0 };

init()
   {
   }

event()
   {
   Output_Message o;
   int i, tnumber, number, initnum, maxi, num_initializers;
   Int bread, error;
   Simulation_time now;
   char msg[2];
   Name_object myself, distributor;
	state *ps = (state *)myState;
   
   strcpy(msg, msgText(0));
   num_initializers = 1;
	/*strcpy(myself, myName);*/ myName(myself);
   initnum = atoi(&myself[4]);
	sprintf(distributor, "distrib%d", initnum);
   tnumber = TOTAL_DIVS;
   number = tnumber / num_initializers;
   maxi = initnum * number;
   
   o.behavior_selector = ADD_UNIT;

  /*
   * The init objects are numbered 1, 2, 3, ... num_initializers
   * and are assigned divisions as follows: If there are 32 divisions
   * and 16 initializers, then each one gets 2 divisions.  In this
   * case, tnumber = 32, number = 2, and the variable 'maxi' keeps
   * track of the maximum division number initializer #X will initialize.
   * Thus initializer 1, in this case, will initialize divisions 1 and 2
   * which will have array indices 0 and 1 in this scheme.  
   */
   for (i = maxi - number ; 
        i < maxi; 
        i++)
      {
		strcpy(o.a.UnitName, init_data[i].who);
		o.a.where.pos.x = init_data[i].x_pos;
		o.a.where.pos.y = init_data[i].y_pos;
		o.a.where.vel.x = 0.0;
		o.a.where.vel.y = 0.0;
		o.a.pradius = init_data[i].pradius;
		tell(distributor, now+SIM_TIME_SCALE, 0, sizeof(o.a), &o);
		tprintf("msg to %s about unit %s: pos=(%.2lf,%.2lf),vel=(%.2lf,%.2lf)\n",
			distributor, o.a.UnitName, o.a.where.pos.x, o.a.where.pos.y,
			o.a.where.vel.x, o.a.where.vel.y);
      }
  /*
   * This next if block handles the case where there are excess
   * divisions.  Suppose tnumber = 40 and num_initializers = 32;
   * there will be 8 excess divisions, which will be initialized
   * by initializers number 1 through 8.
   */
   if ((i = (num_initializers * number) + (initnum-1)) < tnumber )
      {
		strcpy(o.a.UnitName, init_data[i].who);
		o.a.where.pos.x = init_data[i].x_pos;
		o.a.where.pos.y = init_data[i].y_pos;
		o.a.where.vel.x = 0.0;
		o.a.where.vel.y = 0.0;
		o.a.pradius = init_data[i].pradius;
		tell(distributor, now+SIM_TIME_SCALE, 0, sizeof(o.a), &o);
		tprintf("msg to %s about unit %s: pos=(%.2lf,%.2lf),vel=(%.2lf,%.2lf)\n",
			distributor, o.a.UnitName, o.a.where.pos.x, o.a.where.pos.y,
			o.a.where.vel.x, o.a.where.vel.y);
      }
   }

query()
{}
