/*
 * In this package, a 'vector' is simply one (x,y) coordinate point. The
 * vector is assumed to stretch from the origin of the coordinate system
 * to this point; that is, the tail at (0,0) and the head at (x,y).
 */
typedef struct vector
	{
	double x, y;  /* components are reals */
	} Vector;

/*
 * The 'pos' vector in particle is the position of the particle at time
 * 0; the 'vel' vector is the x and y velocity of the particle at time
 * 0. With this information, the routines in the package can calculate
 * collisions.
 */
typedef struct particle
	{
	Vector pos, vel; /* initial position and velocity */
	} Particle;

/*
 * The vectors 'e1' and 'e2' define the endpoints of the line. The    
 * difference between a line and a particle is that a line is fixed in
 * space at all moments of time (i.e. it has zero velocity), whereas
 * a particle can move through space with a linear velocity vector.
 */
typedef struct line
	{
	Vector e1, e2;  /* endpoints of line */
	} Line;

typedef struct answer
	{
	int yesno;		/* YES if there is a positive solution to the quadratic */
	double time;  	/* first solution of quadratic */
	double time2;  /* second solution of quadratic,if there is one */
	} Answer;

/* function defs */

double distance();
double square();
double dot();
double det();
double magnitude();
char *printline();

#ifndef YES
#define YES  1
#define NO   0
#endif

#define CANNOT_COMPUTE -1

#ifdef TRUE
#undef TRUE
#endif

#ifdef FALSE
#undef FALSE
#endif

#define TRUE 1
#define FALSE 0
