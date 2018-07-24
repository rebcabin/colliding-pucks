typedef struct
{
    long	N ;		/* number of elements in sum */
    double	time ;		/* physical time */

    double	m ;		/* sum of masses */

    double	v[2] ;		/* sum of velocities */
    double	vv ;		/* sum of sqares of velocities */

    double	mv[2] ;		/* mass-weighted sum */
    double	mvv ;		/* mass-weighted sum of squares */

    double	K ;		/* sum of kinetic energies */
    double	KK ;		/* sum of squares of kinetic energies */
}
Accumulator ;

Accumulator newAccumulator () ;

Accumulator accumPuckAccumulator (/* Puck, Accumulator */) ;
