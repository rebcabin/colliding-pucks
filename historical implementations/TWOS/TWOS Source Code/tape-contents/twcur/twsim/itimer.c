/* "Copyright (C) 1989, California Institute of Technology. 
     U. S. Government Sponsorship under NASA Contract 
   NAS7-918 is acknowledged." */

#include <stdio.h>
#include "twcommon.h"
#include "machdep.h"


/*itimer */
#ifdef HISP
double
itimer()

{
fprintf(stderr,"sys error, itimer called\n");
return (0.0);
}
#else

#ifdef SUN 

/* THIS IS FOR THE SUN */
/* structure definitions copied from Sun include files */

/* The Sun interval timer has two values: seconds and microseconds.
 * It can be set and read and delivers a SIGALRM signal when it
 * overflows. The resolution is 20 milliseconds */

#define	ITIMER_REAL	0
#define	ITIMER_VIRTUAL	1
#define	ITIMER_PROF	2
/*
struct timeval {
	long	tv_sec;		/* seconds 
	long	tv_usec;	/* and microseconds 
};


struct	itimerval {
	struct	timeval it_interval;	/* timer interval 
	struct	timeval it_value;	/* current value 
};

*/
#include <signal.h>
#include <stdio.h>

/*static structures in this file */

static struct itimerval interval;    /* value read or old value */
static struct itimerval interval2;   /* new value set */
static char itimerid[] = "%W%\t%G%";

/*******************************************************************
*  signal function for SIGALRM
**********************************************************************/
/*
void
brkfcttim()
{
fprintf(stderr,"Timer expired\n");
}
*/

/*************************************************************************
*
*  itimer()
*
*     Sun realtime interval timer function.
*
*     itimer() sets the interval timer to the value given in this file
*     and returns the old value.   The set values
*     for seconds will overflow at about 4290 seconds.  Resolution
*     seems to be 20 milliseconds. (arrgh!!)
*
***************************************************************************/
double
itimer ()
{
    /* signal(SIGVTALRM,brkfcttim); */

    unsigned long sss,uuu;

    interval2.it_value.tv_sec = 4290L;
    interval2.it_value.tv_usec = 999000L;
    interval2.it_interval.tv_sec = 4290L;
    interval2.it_interval.tv_usec = 999000L;
    if (setitimer(ITIMER_VIRTUAL,&interval2,&interval) != 0)
       fprintf(stderr,"setitimer failed\n");

    sss = interval2.it_value.tv_sec - interval.it_value.tv_sec;
    uuu = interval2.it_value.tv_usec - interval.it_value.tv_usec;
    return ((double)uuu + ((double)sss) * 1000000L);
     
}

#endif
#ifdef MARK3

/* THIS IS FOR THE MARK III */

/* The Mark III timers consist of an interval timer that delivers units of
 * 4.34 microseconds, has a max value of 65536, and is read and reset 
 * automatically when one calls microtime() AND the sytem clock, read by
 * calling clock() which is not reset by this operation. */

static int micro_time;
static long over_time1, over_time2;

#define COUNTER_OVERFLOW 65536
#include "stdio.h"
extern long clock();

/***********************************************************************
*
* start_clock()
*
* Record the initial value of the clock in over_time1.
* function deleted 7/26/89.  Initial call to itimer does this and
* Its return value is not used
*
********************************************************************/


/***********************************************************************
*
* itimer()
*
* determine elapsed interval in microseconds and reset our clock
* (over_time1) to time at end of interval. itimer(1) does this.
* because of the strange clock, the resolution is difficult to 
* determine.
*
*******************************************************************/

double
itimer ()

{
   double xxx;

   micro_time = microtime();
   if (micro_time == COUNTER_OVERFLOW)
      {
      over_time2 = clock();
      xxx = over_time2 - over_time1;
      over_time1 =over_time2;
      }
   else
      {
      xxx = micro_time * 4.34;
      over_time1 = clock();
      }
   return(xxx);
}

#endif
#ifdef BF_PLUS 
 
/* THIS IS FOR THE Butterfly Plus */
 
/* The Butterfly timer consist of a real time clock that is set at
 * the boot time of the system. This clock returns ticks of 62.5	 
 * microseconds. In the future we may be able to implement a 	
 * high resolution interval timer using the timer interrupts
 * on each node's DUART  */

/* The routine clock() defined in BBNclock.c returns a memory location
 * cast to an unsigned long.  In itimer ONLY where it is used many
 * times we use the following instead of the routine. */

#define CLOCK  (*(unsigned long*)0xfff7b000)
 
static int micro_time;
static long over_time1, over_time2;
 
extern long clock();
 
/***********************************************************************
*
* start_clock()
*
* Record the initial value of the clock in over_time1.
* 
* function deleted 7/26/89.  Initial call to itimer does this and
* Its return value is not used
*
********************************************************************/
 

/*******************************************************************
* itimer()
*
* determine elapsed interval in microseconds and reset our clock
* (over_time1) to time at end of interval. itimer(1) does this.
* because of the strange clock, the resolution is difficult to
* determine.
*
*******************************************************************/
 
double
itimer ()
 
{
   double xxx;
 
   over_time2 = CLOCK;
   xxx = (over_time2 - over_time1) * 62.5;
   over_time1 =over_time2;
   return(xxx);

}
#endif

#ifdef BF_MACH 
 
/* THIS IS FOR THE BBN GP1000  */
 
/* The Butterfly timer consist of a real time clock that is set at
 * the boot time of the system. This clock returns ticks of 62.5	 
 * microseconds. In the future we may be able to implement a 	
 * high resolution interval timer using the timer interrupts
 * on each node's DUART  */

/* The routine clock() defined in BBNclock.c returns a memory location
 * cast to an unsigned long.  In itimer ONLY where it is used many
 * times we use the following instead of the routine. */

 
static int micro_time;
static long over_time1, over_time2;
 
#include "stdio.h"
extern int getrtc();
 
/***********************************************************************
*
* start_clock()
*
* Record the initial value of the clock in over_time1.
* 
* function deleted 7/26/89.  Initial call to itimer does this and
* Its return value is not used
*
********************************************************************/
 

/*******************************************************************
* itimer()
*
* determine elapsed interval in microseconds and reset our clock
* (over_time1) to time at end of interval. itimer(1) does this.
* because of the strange clock, the resolution is difficult to
* determine.
*
*******************************************************************/
 
double
itimer ()
 
{
   double xxx;
 
   over_time2 = getrtc();
   xxx = (double)(over_time2 - over_time1) * 62.5;
   over_time1 =over_time2;
   return(xxx);

}
#endif


#endif  /* ifdef HISP else part */

/*--------------------------------------------------------------------*/
/*--------------------------------------------------------------------*/
#ifdef TRANSPUTER
/*--------------------------------------------------------------------*/
/*--------------------------------------------------------------------*/


/*--------------------------------------------------------------------*/

#define LOW_PRIO 1     

/*--------------------------------------------------------------------*/

double
itimer ()

{
    int		time;

    if ( ProcGetPriority() == LOW_PRIO )
    {
        time = 64 * Time ();
    }
    else
    {
        time = 16 * Time ();
    }
    
    return ( (double) time );
}

/*--------------------------------------------------------------------*/
/*--------------------------------------------------------------------*/
#endif  /* TRANSPUTER */
/*--------------------------------------------------------------------*/
/*--------------------------------------------------------------------*/
