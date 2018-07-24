/*      Copyright (C) 1989, 1991, California Institute of Technology.
		U. S. Government Sponsorship under NASA Contract NAS7-918
		is acknowledged.	*/

/* Return time in ticks of 62.5  micro-seconds for Butterfly Plus	*/

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

