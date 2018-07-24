/* senile.c -- senility testing routines for warpnet */

#include "twcommon.h"
#include "mapconst.h"
#include "msgs.h" 
#include "map_defs.h"
#include "warpnet.h"
#include "senile.h"

Int senile(node, ps)
   Int node;
   State *ps;
{
   if ( (ps->now1 - ps->Node_Map[node].Line_Currency) > OLD_CURRENCY)
      return 1;
   else
      return 0;
}

