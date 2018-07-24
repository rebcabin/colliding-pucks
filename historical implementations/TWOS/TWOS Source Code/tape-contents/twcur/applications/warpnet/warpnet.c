/* warpnet.c - code for the node object of Warpnet */

#include "twcommon.h"
#include "mapconst.h"
#include "msgs.h"
#include "map_defs.h"
#include "map.h"
#include "warpnet.h"
#include "exit.h"
#include "debug.h"

/* This defines the warpnet object for the system */

ObjectType warpnetType = { "warpnet", i_warpnet, e_warpnet, t_warpnet, 0, 0, 
				sizeof(State),0,0 };

/****************************************************************
 * The Init Section                         *
 ****************************************************************/

init()
{
   State *ps = myState;
   Int node, i;

   ps->number_msgs = 0;
   myName(ps->myself);
   ps->mynum = atoi(ps->myself);
   ps->message_count = 0;
   ps->creation_count = 0;
   ps->infobu = 0; 

   node = ps->mynum;

   ps->Seed = node+1;
   ps->Creation_Delay = Init_Extras[node].Creation_Delay;
   ps->Max_Packet_Size = Init_Extras[node].Max_Packet_Size;

   for (i=0;i<NUMBER_NODES;i++)
      ps->Node_Map[i] = Init_Node_Map[i];

   for (i=0;i<NUMBER_LINES;i++)
      ps->Line_Map[i] = Init_Line_Map[i];

#ifdef GRAPHICS
   make_network(); 
#endif
}


/****************************************************************
 * The Event Section                         *
 ****************************************************************/

event()
{
   State *ps = myState;
   short i,bread,error,No_To_Send;
   Input_Message *Input_Msg;
   extern void create_packet(), process_packet(),
               update_map(), update_me();
   extern Int check_exit();
   
   ps->now1 = now;
   ps->number_msgs = numMsgs;
   ps->message_count++;

   /* exit simulation here on EXIT 0 or EXIT 2 */
   if (check_exit(ps, EXIT2)) return;
   if (check_exit(ps, EXIT0)) return;
   
   /*
    * Main Event Loop
    */

   for (i = 0; i < ps->number_msgs; i++)
      {

      clear(&Input_Msg,sizeof(Input_Msg));
      Input_Msg = (Input_Message *)msgText(i);


      update_me(ps); 

      switch (Input_Msg->Msg_Type) {
         case CREATE:  create_packet(&(Input_Msg->Msg.Packet), ps);
         case PACKET:  process_packet(&(Input_Msg->Msg.Packet), ps); 
                       break;
         case UPDATE:  update_map(&(Input_Msg->Msg.Update), ps);
                       break;
         default:      panic("Bad message type\n");
         } /* end switch */
      } /* end of for statement */

}       /* end of event section */



/****************************************************************
 * The Query Section                         *
 ****************************************************************/

query()
{
}  /* end of query section */


/****************************************************************
 * The Term Section                         *
 ****************************************************************/

term()
{
} /* end of term section */


/****************************************************************
 * Now for miscellaneous functions            *
 ****************************************************************/

Int set_line_pointer(node, ps)
   Int node;
   State *ps;
{
   Int i, line_index;

   line_index = 0;

   for (i=0; i<node; i++)
      line_index += ps->Node_Map[i].Number_Lines;

   return line_index;
}

/*********************************************/

void panic(s)
   char *s;
{
   printf("Panic:  %s\n", s);
   exit();
}


/**********************************************/

print_Update(ps,update,c)
	State *ps;
	Update_Info *update;
   char c;
{
	int i;

	printf("**** Message Info *****\n");

	if (c=='R')
		printf("Recd by %s at time %d\n\n", ps->myself, ps->now1);	
	else
		printf("Sent by %s at time %d\n\n", ps->myself, ps->now1);	

	for (i=0; i<NODES_IN_UPDATE; i++)
		printf("Node %d has currency %d\n", update->Nodes_To_Update[i],
					update->Node_Currencies[i]);	

	printf("**********************\n");	
}
