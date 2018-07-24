/*
 * $Log:	faults.c,v $
 * Revision 1.1  90/08/07  15:38:15  configtw
 * Initial revision
 * 
*/
#ifdef BF_MACH
#include <sys/time.h>
#include <sys/resource.h>

int myPid;
int initFaults = 0;
int lastFaultCount = 0;
int printCount = 0;

set_faults()
{
    struct rusage u;

    getrusage (0, &u);
    lastFaultCount = initFaults =  u.ru_majflt;
}

num_faults()
{
    struct rusage u;

    getrusage (0, &u);
    return  u.ru_majflt - initFaults;
}

print_faults()
{
    int thisCount;
    struct rusage u;

    if ( !printCount )
    {
	myPid = getpid();
    }

    printCount++;
    getrusage (0, &u);
    thisCount =  u.ru_majflt;

    if ( thisCount > lastFaultCount )
	printf
	( "%4d pid <%d> %d more faults -- total: %d\n", 
	    printCount, myPid, thisCount - lastFaultCount,
	    thisCount - initFaults );

    lastFaultCount = thisCount;
}
#endif
