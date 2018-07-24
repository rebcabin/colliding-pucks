/*      Copyright (C) 1989, 1991, California Institute of Technology.
		U. S. Government Sponsorship under NASA Contract NAS7-918
		is acknowledged.        */

/*
 * $Log:	BBNclock.c,v $
 * Revision 1.3  91/07/17  15:05:58  judy
 * New copyright notice.
 * 
 * Revision 1.2  91/06/03  12:16:18  configtw
 * Tab conversion.
 * 
 * Revision 1.1  90/08/07  15:37:33  configtw
 * Initial revision
 * 
*/

/* Return time in ticks of 62.5  micro-seconds for Butterfly Plus       */

#ifdef BF_PLUS
long clock()
{
   return ( (*(unsigned long*)0xfff7b000));
}
#endif
#ifdef BF_MACH
long clock()
{
   return ( (unsigned long)getrtc());
}
#endif

