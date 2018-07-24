/* process.c -- message packet processing functions for warpnet */

#include "twcommon.h"
#include "mapconst.h"
#include "msgs.h"
#include "map_defs.h"
#include "warpnet.h"
#include "debug.h"
#include "diags.h"

void process_packet(Packet, ps)
   Message_Packet *Packet;
   State *ps;
{
   Message_Packet Send_List[MAX_NODE_LINES];  /* just in case max. flow */
   short No_To_Send = 0;
   void packet_arrived(), mail_packet();
   extern void check_sender(), route_packet();

   if ( strcmp(Packet->Last_Node, ps->myself) != 0)                      
      check_sender(Packet, ps);
 
   if ( strcmp(ps->myself, Packet->Destination_Node) == 0 )
      packet_arrived(Packet, ps);
   else
      {
      route_packet(Packet, Send_List, &No_To_Send, ps);
      mail_packet(Send_List, No_To_Send, ps);
      }  
}

/*********************************************/
 
void packet_arrived(Packet,ps)
   Message_Packet *Packet;
   State *ps;
{
#ifdef DBG_REC
   print_rec(Packet,ps->now1,ps->myself);
#endif
 
   if (Packet->Starting_Size != Packet->Own_Size)
      {
      /* If max. flow is being used:            *
       * Search through ps->Pending_Msgs[] to find a match in   *
       * Packet->Creating_Node and Packet->Creation_Time fields *
       * If there is no match use an empty slot.  If no empty   *
       * slot then send away msg for a certain time. If there   *
       * was a match then increase the bytes received field and *
       * empty the slot if bytes received = starting size.      */
      }
}
 
/*********************************************/

void mail_packet(Send_List,No_To_Send,ps)
   Message_Packet Send_List[];
   short No_To_Send;
   State *ps;
{
   Packet_Output Output_Msg;
   Vtime Rec_Time, Load;
   Int i,mp,j, Send_Time, set_line_pointer();

   clear(&Output_Msg,sizeof(Output_Msg));

   /* The time delay is a function of the line being used, the     *
    * size of the packet being sent, and the next available send   *
    * time.  The currency and the internal map (i.e. line loads)   *
    * must be modified.  Actually, the packet size isn't imple-      *   
    * mented yet.  It should be included when max. flow is done.   */
 
   for (i=0; i < No_To_Send; i++)
      {
      /* Put all the Time_Delay calculation and internal map       *
       * modifications here.                                       */
 
      mp = set_line_pointer(ps->mynum, ps);
 
      while (ps->Line_Map[mp].Node_To != atoi(Send_List[i].Next_Node))
         mp++;    /* find correct line of this node */
 
      Send_Time = ps->Line_Map[mp].Send_Time;
      Rec_Time = ( (ps->now1 > ps->Line_Map[mp].Load) ? ps->now1 :
                  ps->Line_Map[mp].Load ) + Send_Time;

      ps->infobu += Rec_Time - (ps->now1 + Send_Time);
#ifdef INFO_BU
      printf("%s %d\n",ps->myself, ps->infobu);
#endif
 
      Output_Msg.Msg_Type = PACKET;
      Output_Msg.Packet = Send_List[i];
      strcpy(Output_Msg.Packet.Last_Node,ps->myself);     /* return addr */
      tell(Output_Msg.Packet.Next_Node, Rec_Time, PACKET, sizeof(Output_Msg),
						(char *)(&Output_Msg));
 
      ps->Line_Map[mp].Load = Rec_Time;
 
#ifdef GRAPHICS
      graphic_out(Send_List[i],ps,Rec_Time,Send_Time);
#endif
 
#ifdef DEBUG
      print_packet(&Send_List[i],"Packet Sent",Rec_Time,ps->myself,ps->now1);
#endif
      }  
}  /* end mail_packet */
 
