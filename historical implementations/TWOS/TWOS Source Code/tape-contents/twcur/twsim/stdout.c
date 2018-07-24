/* "Copyright (C) 1989, California Institute of Technology. 
     U. S. Government Sponsorship under NASA Contract 
   NAS7-918 is acknowledged." */

/*******************************************************************
*  Generic stdout for Time Warp simulator.  This version measures
*  the length of the message by calling strlen on the pointer.
* 
*  If you don't like this one note that you can get YOUR OWN stdout
*  in the simulator by writing one and putting it in the link command
*  ahead of the simulator library file.  In this case always include
*  the variable no_stdout which is set from the simulator itself and
*  turns the output on or off.  
**********************************************************************/

#ifdef MARK3
#include "stdio.h"
#else
#include <stdio.h>
#endif

#include "twcommon.h"


#define init   i_stdout
#define event  e_stdout
#define term   t_stdout
#define displayMsg m_stdout

static char stdoutid[] = "%W%\t%G%";
int no_stdout = 0;		/* a highly questionable kluge */

#define BFRSIZE MAXPKTL


typedef struct State {
	int dummy;
} State;

init()
{
State  *ps;

ps = (State *) obj_myState();
}

event()
{
	State *ps;
	int i, j, count;
	Message *textptr;
	int siz;
	char msg[BFRSIZE +1];

   ps = (State *)myState;
   if (!no_stdout)
   {
	count = obj_numMsgs();
	for	(i = 0; i < count; i++)
	{
		textptr = (Message *)msgText (i);
		siz = strlen(textptr);
		if (siz > BFRSIZE) siz = BFRSIZE;
		strncpy(msg,textptr,siz);
		msg[siz] = 0;
		printf ("%s", msg);
	}
   }
}


term ()
{
State *ps;
}


displayMsg(sel,ptr)
long sel;
char *ptr;
{
    char            s[BFRSIZE];
    char            t[10],
                    c;
    int             i,
                    j,
                    n;
    n = strlen(ptr);
    printf("  Message Display (len: %d)\n",n);

    if (n > BFRSIZE) {
	n = BFRSIZE;
	printf("overlength msg truncated to %d bytes\n",n);
	}
 
    s[78] = '\n';
    s[79] = '\0';
 
#define clrs    {int i; for(i=0; i<78; i++) s[i] = ' ';}
#define toupper(c) (c>0x60?c-0x20:c)

    for (i = 0; i < n;)
    {
        clrs
 
        sprintf (t, "%3.3x:", i % 0x1000);
 
        s[0] = toupper (t[0]);
        s[1] = toupper (t[1]);
        s[2] = toupper (t[2]);
        s[3] = t[3];
 
        for (j = 0; j < 16 && i < n; j++, i++)
        {
            sprintf (t, "%2x ", 0377 & (c = 0377 & ptr[i]));
            s[5 + 3 * j] = toupper (t[0]);
            s[5 + 3 * j + 1] = toupper (t[1]);
            s[5 + 52 + j] = c < 32 || c > 127 ? '.' : c;
        }
        printf ("%s", s);
    }    
 return(0);
}


ObjectType stdouttype = {"stdout", i_stdout, e_stdout,
		 t_stdout,m_stdout,0, sizeof(State),0,0 };




