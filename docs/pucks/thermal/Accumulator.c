#include "Puck.h"
#include "Accumulator.h"

Accumulator newAccumulator ()
{
    Accumulator a ;

    a.N = 
    a.time =
    a.m =
    a.v[0] = a.v[1] = 
    a.vv = 
    a.mv[0] = a.mv[1] = 
    a.mvv =
    a.K = 
    a.KK = 

    0 ;

    return a ;
}

Accumulator accumPuckAccumulator (p, a)

    Puck p ;
    Accumulator a ;
{
    double KE ;

    KE =
    (   
        0.5 * p.mass *
        (
            (p.xdot * p.xdot) +
            (p.ydot * p.ydot)
        )
    ) ;

    a.N ++ ;

    a.time = p.time ;

    a.m += p.mass ;

    a.v[0] += p.xdot ;
    a.v[1] += p.ydot ;

    a.vv += p.xdot * p.xdot + p.ydot * p.ydot ;

    a.mv[0] += p.mass * p.xdot ;
    a.mv[1] += p.mass * p.ydot ;

    a.mvv += p.mass * (p.xdot * p.xdot  +  p.ydot * p.ydot) ;

    a.K  += KE ;
    a.KK += KE * KE ;

    return a ;
}
