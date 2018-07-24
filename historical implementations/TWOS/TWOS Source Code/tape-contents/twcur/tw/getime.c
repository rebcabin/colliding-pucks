/*      Copyright (C) 1989, 1991, California Institute of Technology.
		U. S. Government Sponsorship under NASA Contract NAS7-918
		is acknowledged.        */

/*
 * $Log:	getime.c,v $
 * Revision 1.4  91/07/17  15:08:27  judy
 * New copyright notice.
 * 
 * Revision 1.3  91/07/09  13:42:50  steve
 * Added MicroTime support for Sun version.
 * 
 * Revision 1.2  91/06/03  12:24:14  configtw
 * Tab conversion.
 * 
 * Revision 1.1  90/08/07  15:38:25  configtw
 * Initial revision
 * 
*/
char getime_id [] = "@(#)getime.c       1.16\t6/2/89\t12:43:47\tTIMEWARP";


/*

Purpose:

		getime() contains the routines necessary for getting a time
		for the user.  How this is to be done depends on details of the
		underlying hardware and software.  This module is therefore
		highly dependent on what machine Time Warp is running on.  In
		fact, some of the routines in this module are not even compiled
		for some types of hardware.

Functions:

		getime(time) - return a timing to the user
				Parameters - int *time
				Return - the current time

		clock() - read a system clock
				Parameters - none
				Return - the clock's reading

		print_resource_utilization() - print out some systems statistics
				Parameters - none
				Return - always returns 0

Implementation:

		getime() has two implementations, one for the Mark3 Hypercube,
		another for anything else.  On the Hypercube, it simply makes
		a system call to clock(), and returns the result.  On any other
		hardware currently in use, it calls the system function
		getrusage(), which fills a data structure with all sorts of 
		interesting information.  In this case, all that is needed is
		a seconds reading and a microseconds reading, which are converted
		into a single microseconds reading.  In either implementation,
		the calling program expects to get an absolute reading of time,
		not a reading relative to a previous reading.  (In the case of
		both clock() and getrusage(), when we speak of "system function",
		we are talking about a level below Time Warp that provides these
		services.  If a new machine we are running on does not provide them,
		then they must be implemented in Time Warp.  For instance, the
		non-Mark3 Hypercube systems do not provide clock(), so, for those
		systems, we must write a clock() function outselves.  See below.)

		If we are not compiling the system for the Mark3 Hypercube,
		then two other functions are compiled.  One is clock(), which
		simply calls getim().  The other is print_resource_utilization(),
		which makes a call to getrusage(), and then prints out much of
		the information returned from that call.

*/

#include <stdio.h> 
#include "twcommon.h"
#include "twsys.h"
#include "machdep.h"

#ifdef TRANSPUTER

getime ( time )

	int * time;
{
	*time = 64 * Time ();       /* This assumes a low priority clock */
}

long clock ()
{
	int time ;

	getime (& time) ;

	return time ;
}

transputer_time_init ( time )

int     time;

{
	SetTime ( time );
}


clockval ()
{
	int         tmp;
	float       secs;
	
	tmp = Time ();
	secs = 64.0 * (float) tmp / 1000000;

}

#endif


#ifdef MARK3
getime ( time )

	int * time;
{
	*time = clock ();
}
#endif

#ifdef BBN
getime ( time )

	int * time;
{
	*time = clock ();
}
#endif

#ifdef SUN


long clock ()
{
#ifdef MICROTIME
	extern long node_cputime;

	MicroTime();
	return node_cputime;
#else
	int time ;
	getime (& time) ;

	return time ;
#endif
}

getime ( time )

	int * time;
{
#ifdef MICROTIME
	*time = clock ();
#else
	struct rusage r;

	getrusage ( RUSAGE_SELF, &r );

	*time = r.ru_utime.tv_sec * 1000000 + r.ru_utime.tv_usec;
#endif
}

print_resource_utilization ()
{
	struct rusage r;

	getrusage ( RUSAGE_SELF, &r );

	printf ( "\n" );
	printf ( "utime = %d,%d stime = %d,%d\n",   r.ru_utime.tv_sec,
									            r.ru_utime.tv_usec,
									            r.ru_stime.tv_sec,
									            r.ru_stime.tv_usec );

	printf ( "maxrss = %d ixrss = %d idrss = %d isrss = %d\n",  r.ru_maxrss,
									                            r.ru_ixrss,
									                            r.ru_idrss,
									                            r.ru_isrss );

	printf ( "minflt = %d majflt = %d nswap = %d\n",    r.ru_minflt,
									                    r.ru_majflt,
									                    r.ru_nswap ) ;

	printf ( "inblock = %d oublock = %d msgsnd = %d msgrcv = %d\n",
									            r.ru_inblock,
									            r.ru_oublock,
									            r.ru_msgsnd,
									            r.ru_msgrcv );

	printf ( "nsignals = %d nvcsw = %d nivcsw = %d\n",  r.ru_nsignals,
									                    r.ru_nvcsw,
									                    r.ru_nivcsw );
}
#endif
