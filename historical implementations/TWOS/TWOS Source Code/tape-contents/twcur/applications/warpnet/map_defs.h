/* map_defs.h -- map structure definitions for warpnet */

typedef struct Line_Description
   {
   Int Node_To, Send_Time;
   Vtime Load;
   } Line_Description;

typedef struct Node_Description
   {
   Vtime Line_Currency;
   Int Number_Lines;
      } Node_Description;

typedef struct Node_Extras
   {
   Vtime Creation_Delay;
        Int Max_Packet_Size;
   } Node_Extras;


