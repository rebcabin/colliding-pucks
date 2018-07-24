/* msgs.h -- message format for warpnet object               */

/* The following are message codes for incoming event messages.    */

/* This file needs warpconst.h */

#define CREATE 0
#define PACKET 1
#define UPDATE 2

typedef struct Message_Packet  {
   Name_object Creating_Node;
   Vtime Creation_Time;
   Int Starting_Size;
   Int Own_Size;
   Int Start_Byte;
   Name_object Next_Node;
   Name_object Last_Node;
   Name_object Destination_Node;
   } Message_Packet;

typedef struct Update_Info {
   short Nodes_To_Update[NODES_IN_UPDATE],
         Node_Currencies[NODES_IN_UPDATE],
         Line_Loads[QUERY_LINES];
   } Update_Info;

typedef union Message_Type
   {
   Message_Packet Packet;
   Update_Info Update;
   } Message_Type;

typedef struct Input_Message {
   Int Msg_Type;
   Message_Type Msg;
   } Input_Message;

typedef struct Packet_Output {
	Int Msg_Type;
	Message_Packet Packet;
   } Packet_Output;

typedef struct Update_Output {
	Int Msg_Type;
   Update_Info Update;
   } Update_Output;

typedef struct Create_Output {
   Int Msg_Type;
} Create_Output;
