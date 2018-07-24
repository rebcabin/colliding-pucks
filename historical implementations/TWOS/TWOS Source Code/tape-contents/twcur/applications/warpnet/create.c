/* create.c -- code for message creation for warpnet */

#include "twcommon.h"
#include "mapconst.h"
#include "msgs.h"
#include "map_defs.h"
#include "warpnet.h"
#include "exit.h"
#include "debug.h"
#include "create.h"


void create_packet(Packet, ps)
   Message_Packet *Packet;
   State *ps;
{
   Name_object Random_Destination;
   Create_Output Output_Msg;
   Vtime random();
   extern Int check_exit();

   clear(&Output_Msg, sizeof(Output_Msg));
   clear(Packet, sizeof (Message_Packet));
   Packet->Start_Byte = 0;
   strcpy(Packet->Creating_Node,ps->myself);
   strcpy(Packet->Next_Node,ps->myself);
   strcpy(Packet->Last_Node,ps->myself);
   Packet->Creation_Time = ps->now1;
   Packet->Starting_Size = Packet->Own_Size =
       random((Vtime)ps->Max_Packet_Size,&ps->Seed);
         
   Packet->Start_Byte = 0;
 
   strcpy(Random_Destination,ps->myself);
   while( strcmp(Random_Destination,ps->myself)==0 )
      itoa(random((Vtime)NUMBER_NODES,&ps->Seed)-1,Random_Destination);
 
   strcpy(Packet->Destination_Node, Random_Destination);

   ps->creation_count++;
   if (check_exit(ps,EXIT1)) return;
   else
      {
      Output_Msg.Msg_Type = CREATE;
		tell(ps->myself,ps->now1 + random(ps->Creation_Delay,&ps->Seed),
			CREATE, sizeof(Output_Msg),(char *)(&Output_Msg));
      }
}
 
/*********************************************/
 
Vtime random(size,seed)
Vtime size,*seed;
{
        Vtime ret;
 
        (*seed) *= MULTIPLIER;
        (*seed) += INCREMENT;
        (*seed) %= MODULUS;
 
        ret = ( (*seed) * size/MODULUS + 1);
 
        return(ret);
}
