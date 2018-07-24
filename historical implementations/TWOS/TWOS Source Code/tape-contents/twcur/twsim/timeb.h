/*	@(#)timeb.h 2.5 88/02/08 SMI; from UCB 4.2 81/02/19	*/

/*
 * Structure returned by ftime system call
 */
struct timeb
{
	int	time;
	unsigned short millitm;
	short	timezone;
	short	dstflag;
};
