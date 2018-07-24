/* warpinit.c -- initialization object for Warpnet Simulation */

/* Right now this is hard coded for 32 init objects */
/* It should be written to allow a variable number of objects */
/* 32 is a good number though, and we don't have any time left */
/* The whole initialization is started by sending any message to */
/* 0-init */

#include "twcommon.h"
#include "mapconst.h"
#include "msgs.h"
#include "warpinit.h"

/* This defines the warpinit object for the system */

ObjectType warpinitType = { "warpinit", i_warpinit, e_warpinit, t_warpinit, 
				0, 0, sizeof(State),0,0 };

/****************************************************************
 * The Init Section                         *
 ****************************************************************/

init()
{
   State *ps;
	ps  = myState;
  
	myName(ps->myself);
}


/****************************************************************
 * The Event Section                                              *   
 ****************************************************************/

event()

{
   State *ps;
   Create_Output Output_Msg;
   Int i,mynumber,compernode, remaind, strip();
   Name_object to_who;

   ps = myState;
   ps->now1 = now;

   clear(&Output_Msg,sizeof(Output_Msg));     /* set up message */
   Output_Msg.Msg_Type = CREATE;

   mynumber = strip(ps->myself);

   if (mynumber<15)      /* if you are less than 15 then wake up */
      {          /* two other init objects */
      for (i=0;i<2;i++)
         {
         itoa( (2*mynumber+i+1), to_who);
/*
printf("now is %ld, now1 is %ld,rcvtime is %ld\n",now,ps->now1,ps->now1+1);
*/
         strcat(to_who,"-init");
			tell(to_who, ps->now1+1, CREATE, sizeof(Output_Msg),
							(char *)(&Output_Msg));
         } /* end for i */
      } /* end if mynumber */

   compernode = NUMBER_NODES/32;

   for (i=0;i<compernode;i++) /* wake up your designated warpnet objects */
      {
      itoa( (compernode*mynumber)+i, to_who);
		tell(to_who, START, CREATE, sizeof(Output_Msg), (char *)(&Output_Msg));
      }

   remaind = NUMBER_NODES - compernode*32;

   if (mynumber>=0 && mynumber<remaind)    /* wake up remainding warps */
      {
      itoa(mynumber+compernode*32,to_who);
		tell(to_who,START,CREATE, sizeof(Output_Msg),(char *)(&Output_Msg));
      } /* end if mynumber... */

   if (mynumber == 0)   /* 0-init needs to wake 31-init as well */
		tell("31-init",ps->now1+1,CREATE,sizeof(Output_Msg),
						(char *)(&Output_Msg));

}  /* end event section */



/****************************************************************
 * The Query Section                                              *
 ****************************************************************/

query()
{}


/****************************************************************
 * The Term Section                                              *
 ****************************************************************/

term()
{}


/****************************************************************
 * and now for the functions...                                 *   
 ****************************************************************/

Int strip(s)   /* strips non-numerical characters off the end of a string */
char s[];
{
   Name_object s2;
   Int i = 0;

   strcpy(s2,s);

   while ( s2[i]>='0' && s2[i]<='9' )
      i++;
   s2[i] = '\0';

   return(atoi(s2));
}
