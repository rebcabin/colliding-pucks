/* warpinit.h -- header file for the warpnet initialization object    */

#define init i_warpinit
#define event e_warpinit
#define query q_warpinit
#define term t_warpinit

int i_warpinit(), e_warpinit(), q_warpinit(), t_warpinit();

#define START 100       /* time the warpnet objects all start */
			/* this should be at least 10 above the time */
			/* 0-init receives its start message from */
			/* the config file */	

typedef struct State {
   Name_object myself;
   Vtime now1;
   } State;
