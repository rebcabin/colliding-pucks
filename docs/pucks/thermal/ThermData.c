#include "Accumulator.h"
#include "ThermData.h"

ThermData computeThermData (a)
    Accumulator a ;
{
    ThermData t ;
    double sqrt () ;

    double avgMass ;
    double avgMV[2] ;
    double sqrAvgMV ;
    double avgMVSquared ;

    t.N = a.N ;
    t.time = a.time ;

    t.avgKE = a.K / a.N ;

    if (a.N > 1)
    {
	t.stdDevKE = 
	(
	    a.KK - 
	    (
		a.N * (t.avgKE * t.avgKE)
	    )
	) 
	/ (a.N - 1) ;

	t.stdDevKE = sqrt (t.stdDevKE) ;
    }
    else
    {
	t.stdDevKE = 0 ;
    }

    avgMass = a.m / a.N ;

    avgMV[0] = a.mv[0] / a.N ;
    avgMV[1] = a.mv[1] / a.N ;

    sqrAvgMV =  avgMV[0] * avgMV[0]  +  avgMV[1] * avgMV[1] ;

    avgMVSquared = a.mvv / a.N ;

    t.avgKECM =
    (
	(0.5  *  avgMVSquared) -
	(0.5  *  sqrAvgMV / avgMass)
    ) ;

    return t ;
}
