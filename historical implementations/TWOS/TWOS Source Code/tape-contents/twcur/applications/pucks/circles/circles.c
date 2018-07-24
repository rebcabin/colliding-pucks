/*		CHANGE LOG FOR CIRCLES.C

9-18-90		pls	put double precision conversion in magnitudeVector()

*/

#include <stdio.h>
#include "circles.h"

#define FUNCTION

#define ENL fprintf (stderr, "\n")
#define eprs(x)			fprintf (stderr,"%s\n",x)
#define eprn(x)			fprintf (stderr,"%s",x)
#define epr(t,x)		fprintf ( stderr, "x = %t ", x)
#define epr1(t,x)		epr(t,x) ; ENL
#define epr2(t,x,y)		epr(t,x) ; epr1(t,y)
#define epr3(t,x,y,z)		epr(t,x) ; epr2(t,y,z)
#define epr4(t,x,y,z,w)		epr(t,x) ; epr3(t,y,z,w)
#define epr5(t,x,y,z,w,u)	epr(t,x) ; epr4(t,y,z,w,u)
#define epr6(t,x,y,z,w,u,v)	epr(t,x) ; epr5(t,y,z,w,u,v)
#define epr7(t,x,y,z,w,u,v,r)	epr(t,x) ; epr6(t,y,z,w,u,v,r)


FUNCTION static void	circlesError (s)
char * s ;
{
    eprs (s) ;
    exit (-1) ;
}
        /*******************************************************************/
        /*                                                                 */
        /*                             T I M E S                           */
        /*                                                                 */
        /*******************************************************************/

FUNCTION void		promptAbsTime ()
{
    eprs ("Enter an absolute time:") ;
}

FUNCTION AbsTime	readAbsTime ()
{
    AbsTime t ;

    scanf ("%lf", &t) ;

    return t ;
}

FUNCTION AbsTime	scanAbsTime (string)
char * string ;
{
    AbsTime t ;

    sscanf (string,"%lf", &t) ;

    return t ;
}

FUNCTION void		printAbsTime (at)
AbsTime at ;
{
    eprs ("AbsTime:") ;
    epr1 (7.2lf, at) ;
}

FUNCTION void		promptDeltaTime ()
{
    eprs ("Enter a relative time:") ;
}

FUNCTION DeltaTime	readDeltaTime ()
{
    DeltaTime t ;

    scanf ("%lf", &t) ;

    return t ;
}

FUNCTION DeltaTime	scanDeltaTime (string)
char * string ;
{
    DeltaTime t ;

    sscanf (string, "%lf", &t) ;

    return t ;
}

FUNCTION void		printDeltaTime (at)
DeltaTime at ;
{
    eprs ("DeltaTime:") ;
    epr1 (7.2lf, at) ;
}

        /*******************************************************************/
        /*                                                                 */
        /*                           P O I N T S                           */
        /*                                                                 */
        /*******************************************************************/

FUNCTION void		promptPoint ()
{
    eprs ("Enter x, y for Point: ") ;
}

FUNCTION Point		readPoint ()
{
    double x, y ;

    scanf ("%lf %lf", &x,  &y) ;

    return constructPoint (x,y) ;
}

FUNCTION Point		scanPoint (string)
char * string ;
{
    double x, y ;

    sscanf (string, "%lf %lf", &x,  &y) ;

    return constructPoint (x,y) ;
}

FUNCTION void		printPoint (p)
Point p ;
{
    eprs ("Point:") ;
    epr2 (7.2lf, p.x, p.y) ;
}	

FUNCTION void		drawPoint (p)
Point p ;
{
    printf ("pad:dot9 %lf %lf\n", p.x, p.y) ;
}

FUNCTION void		erasePoint (p)
Point p ;
{
    printf ("pad:eraserect %lf %lf 3 3\n", p.x -1., p.y-1.) ;
}

FUNCTION Point		constructPoint (x, y)
double x, y;
{
    Point p ;

    p.x = x ;
    p.y = y ;

    return p ;
}

        /*******************************************************************/
        /*                                                                 */
        /*                         V E C T O R S                           */
        /*                                                                 */
        /*******************************************************************/

FUNCTION void		promptVector ()
{
    eprs ("Enter x, y for Vector: ") ;
}

FUNCTION Vector		readVector ()
{
    double x, y ;

    scanf ("%lf %lf", &x,  &y) ;

    return constructVector (x,y) ;
}

FUNCTION Vector		scanVector (string)
char * string ;
{
    double x, y ;

    sscanf (string, "%lf %lf", &x,  &y) ;

    return constructVector (x,y) ;
}

FUNCTION void		printVector (v)
Vector v ;
{
    eprs ("Vector:") ;
    epr2 (7.2lf, v.x, v.y) ;
}

FUNCTION void		drawVector (p, v)
Point  p ;
Vector v ;
{
    Point q ;

    q = otherEndPoint (p,v) ;

    printf ("pad:arrow %lf %lf %lf %lf\n", p.x, p.y, q.x, q.y) ;
}

FUNCTION void		eraseVector (p, v)
Point  p ;
Vector v ;
{
    Point q ;

    q = otherEndPoint (p,v) ;

    printf ("pad:un_arrow %lf %lf %lf %lf\n", 
	     p.x, p.y, q.x, q.y) ;
}

FUNCTION Vector		constructVector (x, y)
double x, y ;
{
    Vector v ;

    v.x = x ;
    v.y = y ;

    return v ;
}

FUNCTION Vector		constructVectFromPts (p1, p2)
Point	p1, p2;
{
    Vector v ;

    v.x = p2.x  -  p1.x ;
    v.y = p2.y  -  p1.y ;

    return v ;
}

FUNCTION Point		otherEndPoint (p,v)
Point  p ;
Vector v ;
{
    Point a ;

    a.x = p.x  +  v.x ;
    a.y = p.y  +  v.y ;

    return a ;
}

FUNCTION double		magnitudeVector (v)
Vector v ;
{
    double	tmpa;
    extern double sqrt () ;

/* do the following in 2 steps so that the intermediate result gets
    converted to double precision from extended precision (used by the
    881).  Otherwise the statistics come out differently on the Sun3 
    (which uses software for sqrt) and the GP-1000 (which uses the 881.) */

    tmpa = v.x * v.x + v.y * v.y;
    return sqrt(tmpa);
}

FUNCTION double		magnitudeSquaredVector (v)
Vector v;
{
    return v.x * v.x  + v.y * v.y ;
}

FUNCTION Vector		vectorTimesScalar (v, s)
Vector v;
double s;
{
    Vector out ;

    out.x = v.x * s ;
    out.y = v.y * s ;

    return out ;
}

FUNCTION Vector		vectorReverse (v)
Vector v;
{
    Vector a;

    a.x =  - v.x ;
    a.y =  - v.y ;

    return a ;
}

FUNCTION Vector		unitVector (v)
Vector v;
{
    Vector a;
    double mag;

    mag = magnitudeVector (v) ;

    if ( mag <= 0.0 )
    {
	printVector (v) ;
	circlesError ("Magnitude of vector <= 0") ;
    }

    a.x = v.x / mag ;
    a.y = v.y / mag ;

    return a ;
}

FUNCTION Vector		clockwiseNormal (v)
Vector v;
{
    Vector a;

    a.x = - v.y ;
    a.y =   v.x ;

    return a ;
}

FUNCTION Vector		anticlockwiseNormal (v)
Vector v;
{
    Vector a;

    a.x =   v.y ;
    a.y = - v.x ;

    return a ;
}

FUNCTION Vector		sumVectors (v1, v2)
Vector v1, v2 ;
{
    Vector v ;

    v.x = v1.x  +  v2.x ;
    v.y = v1.y  +  v2.y ;

    return v ;
}

FUNCTION Vector		differenceVectors (v1, v2)
Vector v1, v2 ;
{
    Vector v ;

    v.x = v1.x  -  v2.x ;
    v.y = v1.y  -  v2.y ;

    return v ;
}

FUNCTION double		dotVectors (v1, v2)
Vector v1, v2 ;
{
    return v1.x * v2.x  +  v1.y * v2.y ;
}

FUNCTION double		crossVectors (v1, v2)
Vector v1, v2 ;
{
    return v1.x * v2.y  -  v1.y * v2.x ;
}

        /*******************************************************************/
        /*                                                                 */
        /*                   L I N E   S E G M E N T S                     */
        /*                                                                 */
        /*******************************************************************/

FUNCTION void		promptLineSegment ()
{
    eprs ("Enter e.x, e.y, l.x, l.y for LineSegment: ") ;
}

FUNCTION LineSegment	readLineSegment ()
{
    double ex, ey, lx, ly ;

    scanf ("%lf %lf %lf %lf", &ex,  &ey, &lx, &ly) ;

    return 
    constructLineSegment 
    (
	constructPoint (ex,ey), 
	constructVector (lx,ly)
    ) ;
}

FUNCTION LineSegment	scanLineSegment (string)
char * string ;
{
    double ex, ey, lx, ly ;

    sscanf (string,"%lf %lf %lf %lf", &ex,  &ey, &lx, &ly) ;

    return 
    constructLineSegment 
    (
	constructPoint (ex,ey), 
	constructVector (lx,ly)
    ) ;
}

FUNCTION void		printLineSegment (l)
LineSegment l ;
{
    eprs ("LineSegment:") ;
    epr2 (7.2lf, l.e.x, l.e.y) ;
    epr2 (7.2lf, l.l.x, l.l.y) ;
    epr2 (7.2lf, l.n.x, l.n.y) ;
}

FUNCTION void		drawLineSegment (l)
LineSegment l ;
{
    drawPoint (l.e) ;
    drawVector (l.e, l.l) ;
}

FUNCTION void		eraseLineSegment (l)
LineSegment l ;
{
    erasePoint (l.e) ;
    eraseVector (l.e, l.l) ;
}

FUNCTION void		drawLineSegmentAndN (l)
LineSegment l ;
{
    Point p;
    Vector v;

    drawPoint (l.e) ;
    drawVector (l.e, l.l) ;
    v = vectorTimesScalar (l.l, 0.5) ;
    p = otherEndPoint (l.e, v) ;

    drawVector (p, vectorTimesScalar(l.n,30.)) ;

    /* 30.0 is just a number I picked to try to 
     * make the unit vector look acceptable.
     */
}

FUNCTION void		eraseLineSegmentAndN (l)
LineSegment l ;
{
    Point p;
    Vector v;

    erasePoint (l.e) ;
    eraseVector (l.e, l.l) ;
    v = vectorTimesScalar (l.l, 0.5) ;
    p = otherEndPoint (l.e, v) ;

    eraseVector (p, vectorTimesScalar(l.n,30.)) ;
}

FUNCTION LineSegment	constructLineSegment (e, l)
Point e;
Vector l;
{
    extern double sqrt () ;

    LineSegment seg;

    seg.e = e ;
    seg.l = l ;

    seg.n = clockwiseNormal (l) ;
    seg.n = unitVector (seg.n) ;

    return seg ;
}

        /*******************************************************************/
        /*                                                                 */
        /*                         C I R C L E S                           */
        /*                                                                 */
        /*******************************************************************/

FUNCTION void		promptCircle ()
{
    eprs ("Enter c.p.x, c.p.y, c.v.x, c.v.y, radius, mass, time") ;
}

FUNCTION Circle		readCircle ()
{
    double px, py, vx, vy ;
    double r, m, t ;

    scanf ("%lf %lf %lf %lf %lf %lf %lf", &px,  &py, &vx, &vy, &r, &m, &t) ;

    return 
    constructCircle
    (
	constructPoint (px,py), 
	constructVector (vx,vy),
	r, m, t
    ) ;
}

FUNCTION Circle		scanCircle (string)
char * string ;
{
    double px, py, vx, vy ;
    double r, m, t ;

    sscanf (string, 
	   "%lf %lf %lf %lf %lf %lf %lf", &px,  &py, &vx, &vy, &r, &m, &t) ;

    return 
    constructCircle
    (
	constructPoint (px,py), 
	constructVector (vx,vy),
	r, m, t
    ) ;
}

FUNCTION void		printCircle (pc)
Circle * pc ;
{
    Circle c; 

    c = * pc ;

    eprs ("Circle:") ;
    epr2 (7.2lf, c.p.x, c.p.y) ;
    epr2 (7.2lf, c.v.x, c.v.y) ;
    epr3 (7.2lf, c.r, c.m, c.t) ;
}

FUNCTION void		drawCircle (c)
Circle * c ;
{
    drawPoint (c->p) ;
    printf ("pad:circle16 %lf %lf %lf\n",c->p.x, c->p.y, c->r) ;
}

FUNCTION void		eraseCircle (c)
Circle * c ;
{
    erasePoint (c->p) ;
    printf ("pad:erasecircle16 %lf %lf %lf\n",c->p.x, c->p.y, c->r) ;
}

FUNCTION void		drawCircleAndVelocity (c)
Circle * c ;
{
    drawCircle (c) ;
    drawVector (c->p, c->v) ;
}

FUNCTION void		eraseCircleAndVelocity (c)
Circle * c ;
{
    eraseCircle (c) ;
    eraseVector (c->p, c->v) ;
}

FUNCTION Circle		constructCircle (p, v, r, m, t)
Point   p ;
Vector  v ;
double  r ;
double  m ;
AbsTime t ;
{
    Circle c ;

    c.p = p ;
    c.v = v ;
    c.r = r ;
    c.m = m ;
    c.t = t ;

    return c ;
}

FUNCTION Vector		momentumCircle (c)
Circle * c ;
{
    return vectorTimesScalar (c->v, c->m) ;
}

FUNCTION double		energyCircle (c)
Circle * c ;
{
    double e ;

    e = magnitudeSquaredVector (c->v) ;
    e *= c->m ;
    e /= 2 ;

    return e ;
}

FUNCTION Point		whereCircCtr (c, t)
Circle * c ;
AbsTime t ;
{
    return otherEndPoint (c->p, vectorTimesScalar (c->v, t - c->t) ) ;
}

FUNCTION Point		whereCircCtrDeltaT (c, t)
Circle * c ;
DeltaTime t ;
{
    return otherEndPoint (c->p, vectorTimesScalar (c->v, t) ) ;
}

FUNCTION Circle		moveCircle (c, t)
Circle * c ;
AbsTime t ;
{
    Circle a;

    a = (* c) ;

    a.p = otherEndPoint (a.p, vectorTimesScalar (a.v, t - a.t) ) ;

    a.t = t ;

    return a ;
}

FUNCTION Circle		moveCircleDeltaT (c, t)
Circle * c ;
DeltaTime t ;
{
    Circle a;

    a = (* c) ;

    a.p = otherEndPoint (a.p, vectorTimesScalar (a.v, t) ) ;

    a.t += t ;

    return a ;
}

        /*******************************************************************/
        /*                                                                 */
        /*                      C O L L I S I O N S                        */
        /*                                                                 */
        /*******************************************************************/

FUNCTION Collision	circCtrWLineSegment (c, l)
Circle *c ;
LineSegment l;
{
    Collision 	answer ;
    double 	LcrossV ;
    double 	alpha ;
    Vector 	temp ;

    answer.yes = YES ;
    answer.at   = 0.0 ;

    LcrossV = crossVectors (l.l, c->v) ;

    if (LcrossV == 0)
    {
	answer.yes = NO ;
	return answer ;
    }
    
    temp = constructVectFromPts (c->p, l.e) ;

    alpha = crossVectors (c->v, temp) / LcrossV ;

    if (! (alpha >= 0 && alpha <= 1) )
    {
	answer.yes = NO ;
	return answer ;
    }

    answer.at = c->t + crossVectors (l.l, temp) / LcrossV ;
/*
    fprintf ( stderr,"circCtrWLineSegment %f\n",answer.at );
	*/

    return answer ;
}

FUNCTION Collision	circEdgeWLineSegment (c, l)
Circle *c ;
LineSegment l ;
{
    LineSegment displaced ;
    Vector	k ;
    Vector	r ;
    double	kDotn ;
    double	vDotn ;
    Collision	a ;

    k = constructVectFromPts (c->p, l.e) ;

    kDotn = dotVectors (k, l.n) ;

    vDotn = dotVectors (c->v, l.n) ;

    if ( (vDotn * kDotn)  <=  0.000000 )
    {
	/* circle is bore-sighted on the LineSegment */
	/* or circle is heading away from the LineSegment */

	a.yes = NO ;
	return a ;
    }
    else if (kDotn < 0.00000)
    {
	/* circle clockwise of l */

	r = vectorTimesScalar (l.n, c->r) ;
    }
    else if (kDotn > 0.00000)
    {
	/* circle counter-clockwise of l */

	r = vectorTimesScalar (l.n, - ( c->r ) ) ;
    }

    displaced.e = otherEndPoint (l.e, r) ;
    displaced.l = l.l ;

    return circCtrWLineSegment (c, displaced) ;
}

FUNCTION Collision	circEdgeWithPoint (circle, p)
Circle * circle ;
Point p ;
{
    Collision 		answer ;
    Vector 		temp ;
    double		a, b, c, d ;
    double 		t, t1 ;
    double		tempr;

    extern double	sqrt () ;

    answer.yes = YES ;
    answer.at   = 0.0 ;

    temp = constructVectFromPts (circle->p, p) ;	

	/* temp = distance vector between circle and stationary point p */

	/* Solve quadratic in time distance squared = radius squared */

    a = magnitudeSquaredVector (circle->v) ;

    b = -2.0 * dotVectors (temp, circle->v) ;

    c = magnitudeSquaredVector (temp)  -  circle->r * circle->r ;

    d = b * b  -  4 * a * c ;	/* the discriminant */

    if ( d <= 0 )		/* no hit or just grazing */
    {
	answer.yes = NO ;
	return answer ;
    }

	/* case of two real roots, minimum is time of first hit */


    tempr = sqrt(d);
    t  = ( (-b) - tempr ) / (2 * a) ;
    t1 = ( (-b) + tempr ) / (2 * a) ;
/*???PJH
    t  = ( (-b) - sqrt(d) ) / (2 * a) ;
    t1 = ( (-b) + sqrt(d) ) / (2 * a) ;
	*/

	     
    if ( t1 < t )  t = t1 ;


 
    answer.at = circle->t + t ;

    return answer ;
}

FUNCTION Collision	circleWithCircle (c1, c2)
Circle * c1, * c2 ;
{
    Circle 	fake ;
    Point	p ;

    fake = moveCircle (c1, c2->t) ;



    fake.v = differenceVectors (c1->v, c2->v) ;

    fake.r = c1->r  +  c2->r ;

    p = c2->p ;

    return circEdgeWithPoint (&fake, p) ;
}


/****************************************************************/
/*???PJH Addition for poolballs implementation with circles.	*/
/*								*/
/*	circLineSegDepart_SE ( )				*/
/* This function returns the Collision structure which tells	*/
/* if a circle has departed  with the specified line segment.	*/
/* A departure is defined as the last point on the edge of a	*/
/* circle crossing away from a line segment. 			*/
/* This function is used to calculate the position and time 	*/
/* for DEPART_SECTOR events of balls.			 	*/
/*								*/
/****************************************************************/

FUNCTION Collision	circLineSegDepart_SE (c, l, d)
Circle	* c ;
LineSegment l ;
int	d;
{

#define  POSITIVE	1
#define  NEGATIVE	-1

    LineSegment displaced;
    Collision	When ;
    Vector	k,v;
    double	kDot,vDot;
    Vector	dv;
    LineSegment NL;


    switch ( d )
      {
	 case POSITIVE:
	  {
	      v = vectorTimesScalar ( l.n, ( c->r *2)  );
	  }
	  break;
         case  NEGATIVE:
          {
	      v = vectorTimesScalar ( l.n, -(c->r *2 ) );
          }
	  break;

         default:
	  {
	     When.yes = NO;
	     When.at = 0.0;
	     return When;
	  }
       }
    displaced = constructLineSegment ( otherEndPoint ( l.e, v ), l.l );

    dv = vectorTimesScalar ( unitVector ( displaced.l ),c->r );
    NL.l = sumVectors ( displaced.l, sumVectors ( dv, dv ) );
    NL = constructLineSegment ( 
		otherEndPoint (displaced.e, vectorReverse ( dv ) ),
				NL.l );

    k = constructVectFromPts ( c->p, NL.e );
    kDot = dotVectors (k, NL.n );
    vDot = dotVectors (c->v, NL.n );
    
    if ( (vDot * kDot) <= 0.0 )
     {				/* Circle is bore sighted on the */
	When.yes = NO;		/* LineSegment or circle is going*/
	return When;		/* away from the LineSegment.	 */
     }	      

    When = circEdgeWLineSegment ( c, NL ) ;

    if (When.yes == NO)
	return When ;

    /*  OK -- here's the side effect on c  */

    *c = moveCircle (c, When.at) ;

    return When ;
}


/****************************************************************/
/*???PJH Addition for poolballs implementation with circles.	*/
/*								*/
/*	circPointIntersect_SE ( )				*/
/* This function returns the Collision structure which tells	*/
/* if a circle edge has intersected with the specified point.	*/
/* If an intersect does occur the circle's position is updated	*/
/* to that intersection. But unlike the Collision functions, 	*/
/* the vector of the ball is not changed. This is used in 	*/
/* computing the time when balls cross sector boundaries. 	*/
/* 								*/
/****************************************************************/

FUNCTION Collision	circPointIntersect_SE (c, p)
Circle	* c ;
Point p ;
{

    Collision	When ;

    
    When = circEdgeWithPoint (c, p) ;

    if (When.yes == NO)
	return When ;

    /*  OK -- here's the side effect on c  */

    *c = moveCircle (c, When.at) ;

    return When ;
}


/****************************************************************/
/*???PJH Addition for poolballs implementation with circles.	*/
/*								*/
/*	circLineSegmentIntersect_SE ( )				*/
/* This function returns the Collision structure which tells	*/
/* if a circle edge has intersected with the specified line	*/
/* segment. If an intersect does occur the circle's position	*/
/* is updated to that intersection. But unlike the Collision	*/
/* functions, the vector of the ball is not change. This is	*/
/* used in computing the time when balls cross sector 		*/
/* boundaries.							*/
/****************************************************************/

FUNCTION Collision	circLineSegmentIntersect_SE (c, l)
Circle	* c ;
LineSegment l ;
{

    Collision	When ;

    /* Consider the components of the incoming velocity 
     * parallel and perpendicular to the line segment.
     * The parallel component is unchanged, the perpendicular
     * component is reflected.  
     */
    
    When = circEdgeWLineSegment (c, l) ;

    if (When.yes == NO)
	return When ;

    /*  OK -- here's the side effect on c  */

    *c = moveCircle (c, When.at) ;

    return When ;
}

FUNCTION Collision	circAfterLineSegColl_SE (c, l)
Circle	* c ;
LineSegment l ;
{
    Vector newv ;

    double	vDotl ;
    double	vDotn ;
    double	lCrossn ;

    double	Vax, Vay ;	/* components of velocity after collision */

    Collision	When ;

    /* Consider the components of the incoming velocity 
     * parallel and perpendicular to the line segment.
     * The parallel component is unchanged, the perpendicular
     * component is reflected.  
     */
    
    When = circEdgeWLineSegment (c, l) ;

    if (When.yes == NO)
	return When ;

    vDotl = dotVectors (c->v, l.l) ;
    vDotn = dotVectors (c->v, l.n) ;

    lCrossn = crossVectors (l.l, l.n) ;  /* cheap magnitude of l */

    Vax = ( vDotl * l.n.y  +  vDotn * l.l.y ) / lCrossn ;

    Vay = ( vDotl * l.n.x  +  vDotn * l.l.x ) / ( - lCrossn ) ;

    /*  OK -- here's the side effect on c  */

    *c = moveCircle (c, When.at) ;

    c->v = constructVector (Vax, Vay) ;

    return When ;
}

FUNCTION Collision	circlesAfterCircColl_SE (c1, c2)
Circle     * c1, * c2 ;
{
    Vector 	c, p;

    /*  C is the unit vector separating the centers of the two
     *  circles at collision time.  P is a unit vector normal to
     *  c.  These vectors define a new rectangular coordinate
     *  system.  The momentum of each circle is unchanged in the p
     *  direction and conservation of energy and momentum give
     *  straightforward expressions for the resulting momenta in the
     *  c direction.
     */

    double	v1Dotc ;
    double	v2Dotc ;
    double	v1Dotp ;

    Vector	new_v1, new_v2 ;

    double	mf[2] ;		/* mass factors, from conservation laws */
    double	mass_sum ;

    Vector	mom1, mom2, mom_sum ;

    Collision	When ;

    When = circleWithCircle (c1, c2) ;



    if (When.yes == NO)
	return When ;

    mass_sum = c1->m  +  c2->m ;

    if (c1->m == 0.0 || c2->m == 0.0 || mass_sum == 0.0)  
    {
	printCircle (c1) ;
	printCircle (c2) ;
	circlesError ("Circles have zero mass!") ;
    }

    mom1 = momentumCircle (c1) ;
    mom2 = momentumCircle (c2) ;

    /* total momentum before any changes */

    mom_sum = sumVectors (mom1, mom2) ;

    mf[0] = c1->m  -  c2->m ;
    mf[1] = 2 * c2->m ;

    mf[0] /= mass_sum ;
    mf[1] /= mass_sum ;

    /*  modify positions: side effects here  */

    *c1 = moveCircle (c1, When.at) ;
    *c2 = moveCircle (c2, When.at) ;

    c = constructVectFromPts (c1->p, c2->p) ;

    c = unitVector (c) ;
    p = clockwiseNormal (c) ;

    v1Dotc = dotVectors (c1->v, c) ;
    v2Dotc = dotVectors (c2->v, c) ;
    v1Dotp = dotVectors (c1->v, p) ;

    new_v1 = vectorTimesScalar (c, mf[0] * v1Dotc  +  mf[1] * v2Dotc ) ;

    c1->v = sumVectors (new_v1, vectorTimesScalar (p, v1Dotp) ) ;

    mom1 = momentumCircle (c1) ;
    mom2 = differenceVectors (mom_sum, mom1) ;

    c2->v = vectorTimesScalar (mom2, 1/c2->m) ;

    return When ;
}

FUNCTION Collision	circleAfterPointColl_SE (c, p)
Circle *c ;
Point p ;
{
    Collision When ;

    When = circEdgeWithPoint (c, p) ;

    if (When.yes == NO)
	return When ;

    else
    {
	c->v = vectorReverse (c->v) ;
	return When ;
    }
}
#if 0
/* this code does not seem to be used.
 * it is not being used for pucks.
 * it has 40 K of data, lets trash it.
 */
        /*******************************************************************/
        /*                                                                 */
        /*                         T E S T I N G                           */
        /*                                                                 */
        /*******************************************************************/

#include <math.h>

#define Ncircles 50
#define Nlines 4

static Circle          c[Ncircles];
static Circle          oldc[Ncircles];
static LineSegment     l[Nlines];

#define MinX 10.
#define MaxX 890.
#define LenX 880.

#define MinY 10.
#define MaxY 440.
#define LenY 430.

#define Slop 0.01

static int             go;

static ctrlc ()
{
    go = 0;
}

static Collision       ball_coll[Ncircles][Ncircles];
static Collision       line_coll[Ncircles][Nlines];
static int             no_coll[Ncircles];
static Vector          mom;
static double          bmom,
		       amom;
static double          benergy,
                       aenergy;

FUNCTION int testCircles ()
{
    double          time,
                    coll_time;
    int             i,
                    j,
                    collision,
                    ncircles;
    char            line[80];
    FILE           *fp = stdin;

    draw_border ();

    eprs ("c command, cleans picture") ;
    eprs ("g command == go, CTRL C interrupts running game") ;
    eprs ("s command == stop, stops the game") ;

fix_typo:
    fprintf (stderr, "How Many Circles (max %d)? ", Ncircles);
    scanf ("%d", &ncircles);
    if (ncircles < 1 || ncircles > Ncircles)
	goto fix_typo;

    for (i = 0; i < ncircles; i++)
    {
	promptCircle ();
	c[i] = readCircle ();
	drawCircleAndVelocity (&c[i]);
	oldc[i] = c[i];
	if (i == 0 || c[i].t < time)
	    time = c[i].t;
    }

    fprintf (stderr, "Time = %f\n", time);

    gets (line);
/*???PJH Turned off to link on Chrysalis...
    signal (SIGINT, ctrlc);
	*/

    for (;;)
    {
	for (i = 0; i < ncircles; i++)
	{
	    fprintf (stderr,
		     "Ball %d x = %8.2f y = %8.2f vx = %8.2f vy = %8.2f\n",
		     i, c[i].p.x, c[i].p.y, c[i].v.x, c[i].v.y);

	    if (c[i].p.x - c[i].r < MinX - Slop
		|| c[i].p.x + c[i].r > MaxX + Slop
		|| c[i].p.y - c[i].r < MinY - Slop
		|| c[i].p.y + c[i].r > MaxY + Slop)
	    {
		fprintf (stderr, "\7\7\7Ball %d is Off The Board!\n", i);
		go = 0;
	    }
	}

	if (go == 0)
	{

    get_again:
	    if (!fgets (line, 80, fp))
	    {
		fclose (fp);
		fp = fopen ("/dev/tty", "r");
		goto get_again;
	    }

	    if (line[0] == 'c')
	    {
		printf ("pad:clear\n");
		draw_border ();
		for (i = 0; i < ncircles; i++)
		{
		    drawCircleAndVelocity (&c[i]);
		    oldc[i] = c[i];
		}
		goto get_again;
	    }

	    if (line[0] == 'g')
		go = 1;

	    if (line[0] == 's')
		exit (0);
	}

	collision = 0;

	for (i = 0; i < ncircles; i++)
	{
	    for (j = i + 1; j < ncircles; j++)
	    {
		ball_coll[i][j] = circleWithCircle (&c[i], &c[j]);

		if (ball_coll[i][j].yes && ball_coll[i][j].at >= c[i].t)
		{
		    if (collision == 0 || ball_coll[i][j].at < coll_time)
		    {
			collision = 1;
			coll_time = ball_coll[i][j].at;
		    }
		}
	    }
	}

	for (i = 0; i < ncircles; i++)
	{
	    for (j = 0; j < Nlines; j++)
	    {
		line_coll[i][j] = circEdgeWLineSegment (&c[i], l[j]);

		if (line_coll[i][j].yes && line_coll[i][j].at > c[i].t)
		{
		    if (collision == 0 || line_coll[i][j].at < coll_time)
		    {
			collision = 1;
			coll_time = line_coll[i][j].at;
		    }
		}
	    }
	}

	if (collision == 0)
	{
	    fprintf (stderr, "\7\7\7NO COLLISION!\n");
	    time = c[0].t;
	    go = 0;
	}
	else
	{
	    time = coll_time;
	}

	fprintf (stderr, "Time = %f\n", time);

	for (i = 0; i < ncircles; i++)
	{
	    if (c[i].p.x != oldc[i].p.x
		|| c[i].p.y != oldc[i].p.y)
	    {
		eraseCircleAndVelocity (&oldc[i]);
		printf ("pad:un_arrow %f %f %f %f\n",
			oldc[i].p.x, oldc[i].p.y, c[i].p.x, c[i].p.y);
		drawCircleAndVelocity (&c[i]);
	    }
	    oldc[i] = c[i];
	    no_coll[i] = 1;
	}

	for (i = 0; i < ncircles; i++)
	{
	    for (j = 0; j < Nlines; j++)
	    {
		if (line_coll[i][j].yes && line_coll[i][j].at == time)
		{
		    fprintf (stderr, "Ball %d Collides with Line %d\n", i, j);
		    circAfterLineSegColl_SE (&c[i], l[j]);
		    no_coll[i] = 0;
		}
	    }
	}

	bmom = benergy = 0.;

	for (i = 0; i < ncircles; i++)
	{
	    mom = momentumCircle (&c[i]);
	    bmom += fabs (mom.x) + fabs (mom.y);
	    benergy += energyCircle (&c[i]);
	}

/***   
	fprintf ( stderr, "Total Momentum Before Collisions = %f\n", bmom );   
***/
	fprintf (stderr, "Total Energy Before Collisions = %f\n", benergy);

	for (i = 0; i < ncircles; i++)
	{
	    for (j = i + 1; j < ncircles; j++)
	    {
		if (ball_coll[i][j].yes && ball_coll[i][j].at == time)
		{
		    fprintf (stderr, "Ball %d Collides with Ball %d\n", i, j);
		    circlesAfterCircColl_SE (&c[i], &c[j]);
		    no_coll[i] = no_coll[j] = 0;
		}
	    }
	    if (no_coll[i])
	    {
		fprintf (stderr, "Ball %d Moves\n", i);
		c[i] = moveCircle (&c[i], time);
	    }
	}

	amom = aenergy = 0.;

	for (i = 0; i < ncircles; i++)
	{
	    mom = momentumCircle (&c[i]);
	    amom += fabs (mom.x) + fabs (mom.y);
	    aenergy += energyCircle (&c[i]);
	}
/***
	fprintf ( stderr, "Total Momentum After Collisions = %f\n", amom );

	if ( amom == bmom )
	    fprintf ( stderr, "Total Momentum Has Been Conserved\n" );
	else
	{
	    fprintf ( stderr, "Total Momentum Has Changed\n" );
	}
***/
	fprintf (stderr, "Total Energy After Collisions = %f\n", aenergy);

	if (benergy - aenergy <.000001)
	    fprintf (stderr, "Total Energy Has Been Conserved\n");
	else
	{
	    fprintf (stderr, "\7\7\7Total Energy Has Changed\n");
	    go = 0;
	}

	for (i = 0; i < ncircles; i++)
	{
	    if (c[i].p.x != oldc[i].p.x
		|| c[i].p.y != oldc[i].p.y)
	    {
		printf ("pad:arrow %f %f %f %f\n",
			oldc[i].p.x, oldc[i].p.y, c[i].p.x, c[i].p.y);
		drawCircleAndVelocity (&c[i]);
	    }
	    else if (c[i].v.x != oldc[i].v.x
		     || c[i].v.y != oldc[i].v.y)
	    {
		eraseCircleAndVelocity (&oldc[i]);
		drawCircleAndVelocity (&c[i]);
	    }
	}
    }
}

static draw_border ()
{
    l[0] = constructLineSegment
	(
	 constructPoint (MinX, MinY),
	 constructVector (LenX, 0.)
	);

    drawLineSegment (l[0]);

    l[1] = constructLineSegment
	(
	 constructPoint (MaxX, MinY),
	 constructVector (0., LenY)
	);

    drawLineSegment (l[1]);

    l[2] = constructLineSegment
	(
	 constructPoint (MaxX, MaxY),
	 constructVector (-LenX, 0.)
	);

    drawLineSegment (l[2]);

    l[3] = constructLineSegment
	(
	 constructPoint (MinX, MaxY),
	 constructVector (0., -LenY)
	);

    drawLineSegment (l[3]);
}
#endif
