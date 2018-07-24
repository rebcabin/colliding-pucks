/* warpnet.h -- header file for the warpnet object */

/* This uses "warpconst.h" and "warpmsg.h" */

#define init i_warpnet
#define event e_warpnet
#define query q_warpnet
#define term t_warpnet

/* functions used by the event section */

int i_warpnet(), e_warpnet(), q_warpnet(), t_warpnet();
void panic();
Int set_line_pointer();

typedef struct State {
   Name_object myself;
   Int mynum;
   Vtime now1;
   Int number_msgs;
   Int message_count;
   Int creation_count;
   Int infobu;
   Vtime Seed;
   Vtime Creation_Delay;
   Int Max_Packet_Size;

   Node_Description Node_Map[NUMBER_NODES];   
   Line_Description Line_Map[NUMBER_LINES];
   } State;
