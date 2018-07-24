
/*
 * stat.c - Fred Wieland, Jet Propulsion Laboratory, March 1988
 *
 * Code for the statistics objects, which has two instances, one
 * for Red and one for Blue.
 */
#include <stdio.h>
#ifdef BF_MACH
#include "pjhmath.h"

#ifdef HUGE
#undef HUGE
#define HUGE MAXFLOAT
#endif

#else
#include <math.h>
#endif

#include "twcommon.h"
#include "motion.h"
#include "stb88.h"
#include "divmsg.h"
#include "statmsg.h"
#include "stat.h"

#define MAX_BAR_HEIGHT 100 /* maximum height of bars, in pixels */   



/*************************************************************************
 *
 *                S T A T E    S T R U C T U R E
 *
 *************************************************************************/
typedef struct state
	{
	Int number;
	Name_object myself;
	enum Sides side;
	struct Divinfo
		{
		Name_object name;
		enum Sides side;
		double pct_strength;
		Name_object gridloc;
      enum Sectors sector;
      enum Postures posture;
		} d[MAX_DIVS];
	struct Bar_info
		{
		int height;
		int has_changed;
		} bi[10];

	struct Posture_info
		{
		int height;
		int has_changed;
		} pi[4];

	Vector base;
	Vector pbase;
	int max_num_units;
	int received_update;  /* set FALSE when graphics is updated; TRUE
									 when update event received. */
	Simulation_time next_drawing;
	Simulation_time last_delay_calc;
	} state;

/*************************************************************************
 *
 *                    M E S S A G E    S T R U C T U R E S
 *
 *************************************************************************/
typedef union Input_message
	{
	int behavior_selector;
	Update_stats us;
	Draw_graphics dg;
	} Input_message;

typedef union Output_message
	{
	int behavior_selector;
	Draw_graphics dg;
	} Output_message;

/************************************************************************** 
 *
 *         O B J E C T    T Y P E    D E F I N I T I O N
 *
 **************************************************************************
 These definitions are necessary for the Time Warp Operating System, and
 are not needed for the STB88 algorithms.
 */
ObjectType statsType = {"stats", i_STATS, e_STATS, t_STATS, 0, 0,sizeof(state),0,0};

/*************************************************************************
 *
 *                         I N I T    S E C T I O N 
 *
 *************************************************************************/
init()
	{
	state *s = (state *)myState;
	s->myself[0] = '\0';
	s->base.x = 0;
	s->base.y = 0;
	s->received_update = FALSE;
	s->next_drawing = NEGINF;
	s->last_delay_calc = 0;
	}

/*************************************************************************
 *
 *                P R E    P R O C E S S    F U N C T I O N
 *
 *************************************************************************/
static int pre_process(s)
state *s;
	{
	s->number = numMsgs;
	if (s->myself[0] == '\0') initialize(s);
	calc_delay_time(s);
	}

static int initialize(s)
state *s;
	{
	int width, i;
	Output_message o;

   /* width of gameboard in pixels */

	width = EW_BOARD_SIZE / SCALE_FACTOR;
	s->base.x = width + 40;	
	s->pbase.x = s->base.x+100;

	myName(s->myself);

	if (strncmp(s->myself, "red", 3) == 0)
		{
		s->side = Red;
		s->base.y = MAX_BAR_HEIGHT + 10;
      s->pbase.y = 500; /* y value of posture histogram */
		s->max_num_units = NUM_RED_DIVS;
#     ifdef TURN_ON_GRAPHICS
      printf("pad:@redBarInit %d %d %d %d\n", (int)s->base.x, (int)s->base.y,
         (int)s->base.x + 450, MAX_BAR_HEIGHT);
      printf("pad:@redPosInit %d %d %d %d\n", (int)s->pbase.x,(int)s->pbase.y,
         (int)s->pbase.x + (70*4), MAX_BAR_HEIGHT);
#     endif

		}
	else
		{
		s->side = Blue;
		s->base.y = 2 * (MAX_BAR_HEIGHT + 10) + 100;
 		s->pbase.y = 750; /* y value of posture histogram */
		s->max_num_units = NUM_BLUE_DIVS;

#		ifdef TURN_ON_GRAPHICS
      printf("pad:@blueBarInit %d %d %d %d\n", (int)s->base.x, (int)s->base.y,
         (int)s->base.x + 450, MAX_BAR_HEIGHT);
      printf("pad:@bluePosInit %d %d %d %d\n", (int)s->pbase.x, (int)s->pbase.y,         (int)s->pbase.x + (70*4), MAX_BAR_HEIGHT);
# 	  	endif

		}
	for (i = 0; i < 10; i++)
		{
		s->bi[i].has_changed = YES;
		s->bi[i].height = 0;
		}
#	ifdef TURN_ON_GRAPHICS
	printf("pad:setfont cour.b.24\n");
#	endif
	}

/*************************************************************************
 *
 *                     M A I N    E V E N T    L O O P
 *
 *************************************************************************/
event()
	{
	Input_message m;
	int i;
	Int bread, error, *which_process;
	state *s = (state *)myState; 

        pre_process(s);
	if (now > CUTOFF_TIME) return;
 
	for (i = 0; i < s->number; i++)
		{
	   
		m = * (Input_message *) msgText(i);

		which_process = (Int )(m.us.code);



/*PJH MAJOR KLUDGE DUE TO STRANGE STACK/MESSAGE CORRUPTION...
	MUST LOOK INTO
		switch ( which_process )
			{
			case UPDATE_STATS:
				update_stats(s, &m);
				break;
			case DRAW_GRAPHICS:	
				draw_graphics(s, &m);
				break;
			default:
				printf("Error object %s time %d, message code %d not known\n",
					s->myself, now, m.behavior_selector);
				break;
			}	*/

		if ( which_process == UPDATE_STATS )
			update_stats ( s, &m );
		else
			draw_graphics( s, &m );


		}
	schedule_next_drawing(s);
	}

/*************************************************************************
 *
 *             U P D A T E    S T A T S    B E H A V I O R
 *
 *************************************************************************/
static int update_stats(s, m)
state *s;
Input_message *m;
	{
	int i;


	i = find_divinfo(s, m->us.name);

	strcpy(s->d[i].name, m->us.name);
	s->d[i].side = m->us.side;
	if (m->us.pct_strength < 0. || m->us.pct_strength > 1.0)
		{
		fprintf(stderr, "Error object %s time %ld : strength %lf out of bounds\n",
			s->myself, now, m->us.pct_strength);
		return;
		}
	s->d[i].pct_strength = m->us.pct_strength;
	strcpy(s->d[i].gridloc, m->us.gridloc);
   s->d[i].posture = m->us.posture;
   s->d[i].sector = m->us.sector;
	s->received_update = TRUE;
	}

static int find_divinfo(s, who)
state *s;
Name_object who;
	{
	int i;

	for (i = 0; i < MAX_DIVS; i++)
		{
		if (strcmp(s->d[i].name, who) == 0 || s->d[i].name[0] == '\0')
			return i;
		}
	return -1;
	}

static int del_divinfo(s, who)
state *s;
Name_object who;
	{
	int i, j;

	i = find_divinfo(s, who);
	if (i != -1)
		{
		for (j = i; j < MAX_DIVS-1; j++)
			{
			s->d[j] = s->d[j+1];
			}
		clear(&s->d[MAX_DIVS], sizeof(struct Divinfo));
		}
	}

static int schedule_next_drawing(s)
state *s;
	{
	Output_message o;
	int interval;
	Simulation_time next_drawing;

	if (s->next_drawing < now + 90 and s->next_drawing > now) return;
  /*
	* If (this is the beginning of the simulation (now < 90), then
	* schedule the draw graphics behavior for now + 1
	*/
   if (now <= 0) next_drawing = ( 1 * SIM_TIME_SCALE );
  /* 
	* else, calculate the next interval, which should be the nearest
	* multiply of 90 from now.
	*/
	else
		{
		interval = now % ( 90 * SIM_TIME_SCALE );
		interval = ( 90 * SIM_TIME_SCALE) - interval;
		if (interval == 0) interval = ( 90 * SIM_TIME_SCALE );
		next_drawing = now + interval;
		}
  /*
	* If you've already scheduled the next drawing for the calculated
	* time, then there's no use in doing it again! 
	*/
	if (next_drawing == s->next_drawing) return;
	o.behavior_selector = DRAW_GRAPHICS;


	tell(s->myself, next_drawing, 0, sizeof(o.dg), &o);
	s->next_drawing = next_drawing;
	}

/*************************************************************************
 *
 *            D R A W    G R A P H I C S    B E H A V I O R
 *
 *************************************************************************/
static int draw_graphics(s, m)
state *s;
Input_message *m;
	{
	int i, xpos, ypos;
	Output_message o;

	if (s->received_update == FALSE) return;
	calc_bar_heights(s);
	calc_posture_heights(s); /* for posture histogram */

 /*
   * FIRST, draw the histogram for percent strength.
   */

	for (i = 0; i < 10; i++)
		{
		draw_bar(s, i, s->bi[i]);
		}
/*
   * NEXT, draw the histogram for the postures.
   */
   for (i = 0; i < 4; i++)
      {  
      draw_posture_bar(s, i, s->pi[i]);
      }  
	s->received_update = FALSE;
	}

static int calc_bar_heights(s)
state *s;
	{
	int i, index, num[10];

   clear(&(num[0]), 10 * sizeof(int));
  /*
	* FIRST, loop through each division and assign its strength
	* to one of 10 bins (each bin is a 10% strength span).
	*/
	for (i = 0; i < s->max_num_units; i++)
		{
		if (s->d[i].name[0] == '\0') continue;
		index = floor(s->d[i].pct_strength * 10.0);
		if (index == 10) index = 9;
		if (index < 0) index = 0;
		num[index]++;
		}
  /*
	* NEXT, loop through each bin and determine if its height
	* has changed since the last draw_graph command.
	*/
	for (i = 0; i < 10; i++)
		{
		if (s->bi[i].height == num[i]) /* height hasn't changed */
			{
			s->bi[i].has_changed = NO;  /* mark no change */
			}
		else  /* height has changed; store new height */
			{
			s->bi[i].height = num[i];  /* store new height */
			s->bi[i].has_changed = YES;/* mark changed */
			}
		}
	}


static int calc_posture_heights(s)
state *s;
   {
   int i, index, num[4];
 
   clear(&(num[0]), 4 * sizeof(int));
  /*
   * FIRST, loop through each division and assign its strength
   * to one of 4 bins.
   */
   for (i = 0; i < s->max_num_units; i++)
      {
      if (s->d[i].name[0] == '\0') continue;
      index = (int) s->d[i].posture;
      index = 3 - index; /* causes Attack posture to be highest */
      if (index > 3 or index < 0)
         {
         printf("CTLS runtime debugger has detected a coding error.\n");
         printf("object %s time %ld routine calc_posture_heights\n",
            s->myself, now);
         printf("posture is not in the range 0-3; rather, it is %d\n",
            s->d[i].posture);
         }
      num[index]++;
      }
  /*
   * NEXT, loop through each bin and determine if its height
   * has changed since the last draw_graph command.
   */
   for (i = 0; i < 4; i++)
      {
      if (s->pi[i].height == num[i]) /* height hasn't changed */
         {
         s->pi[i].has_changed = NO;  /* mark no change */
         }
      else  /* height has changed; store new height */
         {
         s->pi[i].height = num[i];  /* store new height */
         s->pi[i].has_changed = YES;/* mark changed */
         }
      }
   }
static int draw_bar(s, i, b)
state *s;
int i;
struct Bar_info b;
	{
	int x_offset, y_offset, height;
	char padstr[80];

   if (b.has_changed == NO) return;
	x_offset = s->base.x + 45 * i;
	height = (b.height * MAX_BAR_HEIGHT)/s->max_num_units;
	y_offset = floor(s->base.y);
	sprintf(padstr, "@bar %d %d %d %d\n", x_offset, y_offset,
			height, MAX_BAR_HEIGHT);
#	ifdef TURN_ON_GRAPHICS
	printf("pad %ld :%s", now, padstr);
#	endif
	}

 
static int draw_posture_bar(s, i, p)
state *s;
int i;
struct Posture_info p;
   {
   int x_offset, y_offset, height;
   char padstr[80];

   if (p.has_changed == NO) return;
   x_offset = s->pbase.x + 70 * i;
   height = (p.height * MAX_BAR_HEIGHT)/s->max_num_units;
   y_offset = floor(s->pbase.y);
   if (s->side == Red)
      {  
       sprintf(padstr, "@redbar %d %d %d %d\n", x_offset, y_offset,
             height, MAX_BAR_HEIGHT);
      }  
   else
      {
       sprintf(padstr, "@bluebar %d %d %d %d\n", x_offset, y_offset,
             height, MAX_BAR_HEIGHT);
      }
#  ifdef TURN_ON_GRAPHICS
   printf("pad %ld :%s", now, padstr);
#  endif
   }


static int calc_delay_time(s)
state *s;
	{
	Simulation_time delta_t;
	double unit;
	int days, hours, minutes;


	unit = 1000/300;  /* 1 second real time per 5 hours of simtime */
	delta_t = ( now / SIM_TIME_SCALE ) - s->last_delay_calc;

   /*
    * If enough time has elapsed, or we are in the initial stages of
    * the game, then printout the value of the clock.
    */
   if (delta_t > 120 or (now >= 0 and now <= ( 720 * SIM_TIME_SCALE)))
      {  
#  ifdef TURN_ON_GRAPHICS
      if ((int)(delta_t * unit) > 100.)
         printf("pad %ld :sleep %d\n", now, (int)(delta_t * unit));
      days = (now / SIM_TIME_SCALE) / 1440;
      hours = ( (now / SIM_TIME_SCALE) / 60) % 24;
      minutes = (now / SIM_TIME_SCALE) % 60;
      printf("pad %ld :printstring 10 20 Time  %d days %d hours %d min \n",
         now, days, hours, minutes);
#  endif




		s->last_delay_calc = now;
		}
	}

query()
	{}	
