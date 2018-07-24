/*      Copyright (C) 1989, 1991, California Institute of Technology.
		U. S. Government Sponsorship under NASA Contract NAS7-918
		is acknowledged.        */

/*
 * $Log:	BBNtime.c,v $
 * Revision 1.3  91/07/17  15:06:14  judy
 * New copyright notice.
 * 
 * Revision 1.2  91/06/03  12:16:19  configtw
 * Tab conversion.
 * 
 * Revision 1.1  90/08/07  15:37:42  configtw
 * Initial revision
 * 
*/

#include <stdio.h>

extern unsigned int node_cputime;
static unsigned int last_time;

butterflytime_init ( rtc_sync )

	unsigned int rtc_sync;
{
	last_time = rtc_sync;
	node_cputime = 0;
}

unsigned int butterflytime ()
{
	register unsigned int time, delta;

#ifdef BF_PLUS
	time = rtc;
#endif
#ifdef BF_MACH
	time = getrtc();
#endif

	if ( time > last_time )
		delta = time - last_time;
	else
		delta = last_time - time;

	node_cputime += delta;
	last_time = time;
	return delta;
}
