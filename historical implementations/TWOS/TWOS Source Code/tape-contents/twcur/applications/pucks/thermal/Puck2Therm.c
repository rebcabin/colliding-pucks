#include <stdio.h>

#include "prdefs.h"

#include "Puck.h"
#include "Accumulator.h"
#include "ThermData.h"

main ()
{
    puck2therm () ;
}

puck2therm ()
{
    Puck       p ;
    double     theCurrentTime = 0 ;
    static     first = 1 ;
    Accumulator a ;

    a = newAccumulator () ;

    while (1)
    {
	p = scanPuck () ;

	if (feof(stdin))
	{
	    printThermData (computeThermData(a)) ; NL ;
	    fflush (stdout) ;
	    return ;
	}

	p.time /= 1000000 ;

	if (first)
	{
	    first = 0 ;
	    a = newAccumulator () ;
	    theCurrentTime = p.time ;
	}

	if (p.time != theCurrentTime)
	{
	    printThermData (computeThermData(a)) ; NL ;

            a = accumPuckAccumulator (p, newAccumulator ()) ;

	    theCurrentTime = p.time ;

	    continue ;
	}
	else
	{
	    a = accumPuckAccumulator (p, a) ;
	}
    }
}
