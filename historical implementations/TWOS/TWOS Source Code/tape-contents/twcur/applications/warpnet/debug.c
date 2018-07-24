/* debug.c -- debugging routines for warpnet */

#include "twcommon.h"
#include "mapconst.h"
#include "msgs.h"
#include "map_defs.h"
#include "warpnet.h"
#include "debug.h"

#ifdef DEBUG
 
print_packet(Packet,Control_String,Rec_Time, myself, now1)
Message_Packet *Packet;
char *Control_String;
Int Rec_Time;
char *myself;
Vtime now1;
{
   printf("****************************************************\n");
   printf("%s:  I am node %s.  Time is %d.\n",Control_String,myself,now1);
   printf("Packet->Creating_Node: '%s'\n",Packet->Creating_Node);
   printf("Packet->Creation_Time:  %d\n",Packet->Creation_Time);
   printf("Packet->Starting_Size:  %d\n",Packet->Starting_Size);
   printf("Packet->Own_Size:       %d\n",Packet->Own_Size);
   printf("Packet->Next_Node:     '%s'\n",Packet->Next_Node);
   printf("Packet->Destin._Node:  '%s'\n",Packet->Destination_Node);
   printf("Packet->Receive_Time:   %d\n",Rec_Time);
   printf("****************************************************\n");
}
 
#endif   /* DEBUG */
 
/*********************************************/

#ifdef DBG_REC

print_rec(Packet, Rec_Time, myself)
   Message_Packet *Packet;
   Vtime Rec_Time;
   Name_object myself;
{
   printf("****************************************************\n");
   printf("Packet Received.  I am node %s. Time is %d.\n",myself,Rec_Time);
   printf("Packet->Creating_Node: '%s'\n",Packet->Creating_Node);
   printf("Packet->Creation_Time:  %d\n",Packet->Creation_Time);
   printf("Packet->Destin._Node:  '%s'\n",Packet->Destination_Node);
   printf("****************************************************\n");
}

#endif   /* DBG_REC */

/*********************************************/


 
#ifdef Q_DEBUG
 
print_query(ps)
State *ps;
{
   Int mp, i;
 
   mp = 0;
 
   printf("=====================================================\n");
   printf("I am node %s at time %d.\n",ps->myself,ps->now1);
   for (i=0;i<NUMBER_LINES;i++)
      printf("Line to %d has send time = %d, load = %d.\n",
      ps->Line_Map[i].Node_To,ps->Line_Map[i].Send_Time,
      ps->Line_Map[i].Load);
 
   printf("=====================================================\n");
}
 
#endif  /* Q_DEBUG */
 
 
#ifdef REASON_UPDATE
 
print_reason(ps, t1, t2, t3)
   State *ps;
   Int t1, t2, t3;
{
   Int i;

   printf("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");
   printf("I am %s, and I should update %d,\n", ps->myself, t1);
   printf("To send to %d I would have used %d.\n", t2, t3);
   for (i=0;i<NUMBER_LINES;i++)
      printf("Line to %d has send time = %d, load = %d.\n",
      ps->Line_Map[i].Node_To,ps->Line_Map[i].Send_Time,
      ps->Line_Map[i].Load);
   printf("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");
}

print_updates(ps, nodes)
   State *ps;
   Int nodes[];
{
   Int i, j;

   printf(":::::::::::::::::::::::::::::::::::::::::::::::::::\n");
   printf("Update these nodes:  \n");
   for (i=0; i<NODES_IN_UPDATE; i+=10)
      {  
      for (j=0;j<10;j++)
         if (i+j < NODES_IN_UPDATE)
            printf("%d ", nodes[i+j]);
      printf("\n");
      }  
   printf(":::::::::::::::::::::::::::::::::::::::::::::::::::\n");
}

#endif  /* REASON_UPDATE */


