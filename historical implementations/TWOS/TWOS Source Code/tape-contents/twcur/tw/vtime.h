/*      Copyright (C) 1989, 1991, California Institute of Technology.
		U. S. Government Sponsorship under NASA Contract NAS7-918
		is acknowledged.        */

/*
 * $Log:	vtime.h,v $
 * Revision 1.3  91/07/17  15:14:14  judy
 * New copyright notice.
 * 
 * Revision 1.2  91/06/03  12:27:34  configtw
 * Tab conversion.
 * 
 * Revision 1.1  90/08/07  15:41:45  configtw
 * Initial revision
 * 
*/

#define GetSimTime(a) (a.simtime)

#define GetSequence1(a) (a.sequence1)

#define GetSequence2(a) (a.sequence2)


#define gtDTime(a1,a2,b1,b2) \
( \
	( a1 > b1 ) || \
	( a1 == b1 && a2 > b2 ) \
)

#define geDTime(a1,a2,b1,b2) \
( \
	( a1 > b1 ) || \
	( a1 == b1 && a2 >= b2 ) \
)

#define ltDTime(a1,a2,b1,b2) \
( \
	( a1 < b1 ) || \
	( a1 == b1 && a2 < b2 ) \
)

#define leDTime(a1,a2,b1,b2) \
( \
	( a1 < b1 ) || \
	( a1 == b1 && a2 <= b2 ) \
)

#define eqDTime(a1,a2,b1,b2) \
( \
	a1 == b1 && a2 == b2 \
)

#define neDTime(a1,a2,b1,b2) \
( \
	a1 != b1 || a2 != b2 \
)

#ifdef FAST_VTIME_MACROS

#define bothNeg(a,b) \
( \
	( *((int *)&a) & *((int *)&b) ) < 0 \
)

#define gtSTime(a,b) \
( \
	bothNeg(a,b) \
	? ltDTime(*((int *)&a),*(((Uint *)&a)+1),*((int *)&b),*(((Uint *)&b)+1)) \
	: gtDTime(*((int *)&a),*(((Uint *)&a)+1),*((int *)&b),*(((Uint *)&b)+1)) \
)

#define geSTime(a,b) \
( \
	bothNeg(a,b) \
	? leDTime(*((int *)&a),*(((Uint *)&a)+1),*((int *)&b),*(((Uint *)&b)+1)) \
	: geDTime(*((int *)&a),*(((Uint *)&a)+1),*((int *)&b),*(((Uint *)&b)+1)) \
)

#define ltSTime(a,b) \
( \
	bothNeg(a,b) \
	? gtDTime(*((int *)&a),*(((Uint *)&a)+1),*((int *)&b),*(((Uint *)&b)+1)) \
	: ltDTime(*((int *)&a),*(((Uint *)&a)+1),*((int *)&b),*(((Uint *)&b)+1)) \
)

#define leSTime(a,b) \
( \
	bothNeg(a,b) \
	? geDTime(*((int *)&a),*(((Uint *)&a)+1),*((int *)&b),*(((Uint *)&b)+1)) \
	: leDTime(*((int *)&a),*(((Uint *)&a)+1),*((int *)&b),*(((Uint *)&b)+1)) \
)

#define eqSTime(a,b) \
( \
	*((int *)&a) == *((int *)&b) && *(((int *)&a)+1) == *(((int *)&b)+1) \
)

#define neSTime(a,b) \
( \
	*((int *)&a) != *((int *)&b) || *(((int *)&a)+1) != *(((int *)&b)+1) \
)

#endif


#define gtVTime(a,b) \
( \
	gtSTime(a.simtime,b.simtime) || \
	( eqSTime(a.simtime,b.simtime) && \
		gtDTime(a.sequence1,a.sequence2,b.sequence1,b.sequence2) ) \
)

#define geVTime(a,b) \
( \
	gtSTime(a.simtime,b.simtime) || \
	( eqSTime(a.simtime,b.simtime) && \
		geDTime(a.sequence1,a.sequence2,b.sequence1,b.sequence2) ) \
)

#define ltVTime(a,b) \
( \
	ltSTime(a.simtime,b.simtime) || \
	( eqSTime(a.simtime,b.simtime) && \
		ltDTime(a.sequence1,a.sequence2,b.sequence1,b.sequence2) ) \
)

#define leVTime(a,b) \
( \
	ltSTime(a.simtime,b.simtime) || \
	( eqSTime(a.simtime,b.simtime) && \
		leDTime(a.sequence1,a.sequence2,b.sequence1,b.sequence2) ) \
)

#define eqVTime(a,b) \
( \
	eqSTime(a.simtime,b.simtime) && \
	eqDTime(a.sequence1,a.sequence2,b.sequence1,b.sequence2) \
)

#define neVTime(a,b) \
( \
	neSTime(a.simtime,b.simtime) || \
	neDTime(a.sequence1,a.sequence2,b.sequence1,b.sequence2) \
)
