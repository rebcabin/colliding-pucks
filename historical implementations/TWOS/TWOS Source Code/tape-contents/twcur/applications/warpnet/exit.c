/* exit.c -- exit routine for warpnet */

#include "twcommon.h"
#include "mapconst.h"
#include "msgs.h"
#include "map_defs.h"
#include "warpnet.h"
#include "exit.h"
#include "diad.h"

 
Int check_exit(ps,exit)
   State *ps;
   Int exit;
{
   switch (EXIT)
      {  
      case 0: if ( (ps->message_count>DIAD)&&(exit==1) ) return 1;
              break;
      case 1: if ( (ps->creation_count>=DIAD)&&(exit==1) ) return 1;
              break;
      case 2: if ( (ps->now1>DIAD)&& exit) return 1;
              break;
      default: panic("Bad exit routine");
               break;
      }  
   return 0;
}
