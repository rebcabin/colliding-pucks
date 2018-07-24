/* router.c -- router routines for warpnet */

#include "twcommon.h"
#include "mapconst.h"
#include "msgs.h"
#include "map_defs.h"
#include "warpnet.h"

void route_packet(Packet,Send_List,No_To_Send,ps)
   Message_Packet *Packet, Send_List[];
   short *No_To_Send;
   State *ps;
{
   Int t1, t2, t3;
   void shortest_path();
 
   /* The No_To_Send (number of packets to send) and the    *
    * Send_List are in place just in case a max. flow algorithm *
    * is ever implemented along with the shortest path routine. *
    * If it is we will need data structures to handle to event  *
    * of a number (MAX_NODE_LINES) of packets to be sent out    *
    * since max. flow will break up the initial packet into     *
    * smaller ones.  For right now I leave a kludge of       *   
    * No_To_Send being set to one.  This is correct with only    *
    * the shortest path algorithm being used.         */ 
 
   *No_To_Send = 1;
   t1 = atoi(ps->myself);
   t2 = atoi(Packet->Destination_Node);
   t3 = atoi(Packet->Next_Node);
 
   shortest_path(t1, t2, &t3, ps);
   itoa(t3,Packet->Next_Node);
 
   Send_List[0] = *Packet;  /* should be made loop for max. flow */
}
 
/*********************************************/
 
void shortest_path(From_Node, To_Node, Next_Node,ps)
   Int From_Node, To_Node, *Next_Node;
   State *ps;
{
   Int V2link[NUMBER_NODES], V2link_Start, Vset[NUMBER_NODES],
       d[NUMBER_NODES], Current_Node, Connected_Node,
       Parent[NUMBER_NODES], No_Path, Line, mp,i, Send_Time,
       Find_Min_E2(), set_line_pointer();

   /* initialization section */

   for (Current_Node=0;Current_Node<NUMBER_NODES;Current_Node++)
      if (Current_Node != From_Node)
   Vset[Current_Node] = 3;
      else
         Vset[Current_Node] = 1;

   Current_Node = From_Node;
   d[From_Node] = 0;
   V2link_Start = -1;
   No_Path = 0;
 
   /* traverse nodes */
 
 
   while (Current_Node != To_Node)
      {
      mp = set_line_pointer(Current_Node, ps);
       
      for (Line=mp;Line<mp + ps->Node_Map[Current_Node].Number_Lines;Line++)
         {
         Connected_Node = ps->Line_Map[Line].Node_To;
         Send_Time = ps->Line_Map[Line].Send_Time;

         if (ps->Line_Map[Line].Load > ps->now1)    
            Send_Time += ps->Line_Map[Line].Load - ps->now1;
 
         /* for max. flow packet size should be added */
 
         if ( (Vset[Connected_Node]==2)&&
                   (d[Current_Node]+Send_Time<d[Connected_Node]) )
            {
            /* replace edge to Connected_Node in E2 by XY */
            Parent[Connected_Node] = Current_Node;
            d[Connected_Node] = d[Current_Node]+Send_Time;
            }  /* end if */
 
         if (Vset[Connected_Node]==3)
            {
            /* put Connected_Node in V2 and XY in E2 */
            Vset[Connected_Node]=2;
            V2link[Connected_Node] = V2link_Start;
            V2link_Start = Connected_Node;
            Parent[Connected_Node] = Current_Node;
            d[Connected_Node] = d[Current_Node]+Send_Time;
            } /* end if */
         } /* end for line */
 
      if (V2link_Start == -1)
         No_Path = 1;
 
      Connected_Node = Find_Min_E2(V2link,&V2link_Start,d);
 
      Vset[Connected_Node] = 1;
      Current_Node = Connected_Node;
      } /* end while Current_Node */
 
   while (Current_Node != From_Node)
      {
      *Next_Node = Current_Node;
      Current_Node = Parent[Current_Node];
      }
 
} /* end shortest_path */
 
/*********************************************/

Int Find_Min_E2(V2link, V2link_Start, d)
Int V2link[NUMBER_NODES],*V2link_Start, d[NUMBER_NODES];
{
   Int Ptr, Min, Old_Ptr, q, out;

   out = Old_Ptr = *V2link_Start;
   q = -1;
   Min = d[Old_Ptr];
   Ptr = V2link[Old_Ptr];

   while (Ptr != -1)
      {  
      if (Min>d[Ptr])
    {
    Min = d[Ptr];
    out = Ptr;
    q = Old_Ptr;
    }
      Old_Ptr = Ptr;
      Ptr = V2link[Old_Ptr];
      }  

   if (q==(-1))
      *V2link_Start = V2link[out];
   else
      V2link[q] = V2link[out];

   return(out);
}  /* end Find_Min_E2 */
