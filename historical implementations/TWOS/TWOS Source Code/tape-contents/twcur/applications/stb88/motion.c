/*
 * Motion.c - Fred Wieland, Jet Propulsion Laboratory, October 26, 1987
 *					 These routines are similar to the 'circles' package 
 *					 produced by Brian Beckman. However, they apply to moving
 *					 line segments.
 *
 *	CREATED		27 October 1987
 * MODIFIED
 */

#include <math.h>
#include "motion.h"
#include "ctlslib.h" /* definitions of max, min, dmax, dmin */

/* #define TEST_MOTION  /* define if you want a static test */
#define tprintf if(0==1)printf

int create_vector(x, y, result)
double x, y;
Vector *result;
	{
	(*result).x = x;
	(*result).y = y;
	}

int difference(v1, v2, v3) 
Vector v1, v2, *v3;
	{
   (*v3).x = v1.x - v2.x;
   (*v3).y = v1.y - v2.y;
   }

double square(v1)
Vector v1;
	{
	double result;

   result = v1.x * v1.x + v1.y * v1.y;
   return result;
   }

double dot(v1, v2) /* dot product of v1 and v2 */
Vector v1, v2;
 	{
	double i;

	i = (v1.x * v2.x) + (v1.y * v2.y);
	return i;
	}

double det(v1, v2)  /* determinant of v1 and v2 */
Vector v1, v2;
   {
   double i;

   i = v1.x * v2.y - v1.y * v2.x;
   return i;
   }

int addv(v1, v2, result)
Vector v1, v2, *result;
   {
   (*result).x = v1.x + v2.x;
   (*result).y = v1.y + v2.y;
   }

int multiply(v, s, result)
Vector v;
int s;
Vector *result;
	{
	 if (v.x == 0.0) v.x = 0.0;
         if (v.y == 0.0) v.y = 0.0;
   	 result->x = v.x * s;
         result->y = v.y * s;
   }

int dmultiply(v, s, result)
Vector v;
double s;
Vector *result;
	{
   (*result).x = v.x * s;
   (*result).y = v.y * s;
   }

int create_particle(pos, vel, result)
Vector pos, vel;
Particle *result;
	{
	(*result).pos = pos;
	(*result).vel = vel;
	}

/*
 * Whereis particle 'p' at time 't'?
 */
int whereis(p, t, result)
Particle p;
long t;
Particle *result;
	{
	Vector v1, v2;

	multiply(p.vel, (int)t, &v1);
	addv(p.pos, v1, &result->pos);
   /* (*result).pos = addv(p.pos, multiply(p.vel, (int)t)); */
   (*result).vel = p.vel;
	}

/*
 * Caution! The following routine only works if line 'l' is either
 * horizontal or vertical.  It will not work with sloping lines.
 */
double howfar(p, l)
Particle p;
Line l;
	{
	return (dmin(abs(p.pos.x-l.e1.x),abs(p.pos.y-l.e1.y)));
	}

double distance(p1, p2)
Particle p1, p2;
	{
	double result, sum;
	double d1, d2;

   d1 = p1.pos.x - p2.pos.x;
	d2 = p1.pos.y - p2.pos.y;
	sum = d1*d1 + d2*d2;
  /*
	* Counterpoint cross compiler cannot handle small square roots.
	*/
	if (sum < 1.0) 
		{
		sum *= 10000.0;
		result = sqrt(sum) / 100.0;
		}
	else
		{
		result = sqrt(sum);
		}
	return result;
	}

double magnitude(v)
Vector v;
	{
	double square_v, result, result2;

	square_v = square(v);
	if (square_v < 1.0)
		{
		square_v *= 10000.0;
		result = sqrt(square_v) / 100.0;
		}
	else
		{
		result = sqrt(v.x*v.x + v.y*v.y);
		}
	result = sqrt(square_v);
	return result;
	}

int null_vector(v)
Vector *v;
	{
	(*v).x = 0.0;
	(*v).y = 0.0;
	}

int vcomp(v1, v2)
Vector v1, v2;
	{
	if (v1.x == v2.x && v1.y == v2.y) return 0;
	else return 1;
	}

int create_line(e1, e2, result)
Vector e1, e2;
Line *result;
	{
	(*result).e1 = e1;
	(*result).e2 = e2;
	}

int isNS(l)
Line l;
	{
	if (l.e1.x == l.e2.x) return TRUE;
	else return FALSE;
	}

int isEW(l)
Line l;
	{
	if (l.e1.y == l.e2.y) return TRUE;
	else return FALSE;
	}

/*
 * line_ge(l1, l2) checks to see if l1 >= l2. It works only if
 * l1 and l2 are either vertical (north-south) or horizontal
 * (east-west). It fails otherwise.
 */
int line_ge(l1, l2)
Line l1, l2;
	{
	if (isNS(l1) && isNS(l2))
		{
		if (l1.e1.x >= l2.e1.x) return TRUE;
		}
	if (isEW(l1) && isEW(l2))
		{
		if (l1.e1.y >= l2.e1.y) return TRUE;
		}
	else return FALSE;
	}

/*
 * line_gt(l1, l2) checks to see if l1 > l2. It works only if
 * l1 and l2 are either vertical (north-south) or horizontal
 * (east-west). It fails otherwise.
 */
int line_gt(l1, l2)
Line l1, l2;
	{
	if (isNS(l1) && isNS(l2))
		{
		if (l1.e1.x > l2.e1.x) return TRUE;
		}
	if (isEW(l1) && isEW(l2))
		{
		if (l1.e1.y > l2.e1.y) return TRUE;
		}
	else return FALSE;
	}

int exact_particle_collision(p1, p2, ans)
Particle p1, p2;
Answer *ans;
   {
   inexact_particle_collision(p1, p2, 0.0, ans);
	}
	
/*
 * In the following routine, a collision will occur if particles p1 and p2
 * move within one 'radius' of each other at any time. If, upon entering
 * this routine, p1 and p2 are already within one radius of one another,
 * then the returned collision time will be negative.
 *
 * The mathematical model is as follows:
 * p1 and p2 are two particles whose position at any moment of time is
 * described by the vector equations:
 *
 * 	p1 = P0 + V*t							p2 = S0 + U*t
 *
 * where P0 = initial position of p1 = k1 <x> + k2 <y>
 *		    V = velocity of p1 = Vx <x> + Vy <y> 
 * 		S0 = initial position of p2 = l1 <x> + l2 <y>
 *			 U = velocity of p2 = Ux <x> + Uy <y>
 *
 * Note that p1, p2, P0, V, S0, and U are vectors, and k1, k2, Vx, Vy, l1, l2,
 * Ux, and Uy are scalars.  <x> and <y> are the unit vectors along the x and
 * y axes respectively. 't' is the time variable, also a scalar.
 *
 * If we want to determine at what time two particles are within one 'radius'
 * of each other, we must solve the following quadratic:
 *
 * (p1 - p2) ** 2 = (P0 - S0 + t * (V - U)) ** 2,
 *
 * where ** is the exponential operator. If you multiply out all terms, you
 * get a vector equation which is quadratic in t.  Solving via the quadratic
 * formula yields the following coefficients:
 *
 * 	a = (V - U) ** 2
 *		b = 2 * (V - u) * (P0 - S0)
 *		c = ((P0 - S0) ** 2) - radius
 *  	t = -b +/- sqrt( b ** 2 - 4 * a * c) / 2 * a, which is just the quadratic
 * formula. Note that the value of the discriminant is important. If the
 * discriminant is negative, there is no collision within 'radius' between 
 * p1 and p2.  If the discriminant is zero, there is a glancing collision.
 * If it is positive, there is a collision.
 */
 

int inexact_particle_collision(p1, p2, r, result)
Particle p1, p2;
int r;
Answer *result;
   {
   double a, b, c, temp;
   double d, t1, t2, radius;
	Vector v1, v2;

  /*
   * solve the equation of motion for the two particles 
   */
	tprintf("p1 = %.2lf %.2lf %.2lf %.2lf ; p2 = %.2lf %.2lf %.2lf %.2lf",
		p1.pos.x, p1.pos.y, p1.vel.x, p1.vel.y, p2.pos.x, p2.pos.y,
		p2.vel.x, p2.vel.y);
	radius = (double) r;
	tprintf(" r=%.2lf\n", radius);
	difference(p1.vel, p2.vel, &v1);
	difference(p1.pos, p2.pos, &v2);
   a = square(v1);
   /* b = 2 * dot(difference(p1.vel, p2.vel),difference(p1.pos, p2.pos)); */
	b = 2 * dot(v1, v2);
   /* c = square(difference(p1.pos, p2.pos)) - radius; */
	c = square(v2) - radius;
   d = b*b - 4*a*c;
   tprintf("a=%.2lf, b=%.2lf, c=%.2lf, disc = %.2lf\n", a, b, c, d);
  /*
	* We will account for the following cases:
	* (1) The two particles are stationary and within range (one 'radius').
  	*     In this case, a = b = 0, c <= 0.
	* (2) The two particles are stationary and out of range.
	*     In this case, a = b = 0, c > 0.
	* (3) One or both particles are moving, and they do not pass within range.
   *     In this case, d < 0. 
	* (4) One or both particles are moving, and they pass within range.
	*     In this case, we calculate t1 and t2 (the roots of the quadratic)
	*     and return the minimum.  Note that the minimum will be < 0 if the
	*     two particles are already within range (i.e. their distance is
	*	   less than 'radius.')
   */
   if (a == 0.0 && b == 0.0 && c <= 0) /* stationary particles within range */
		{
      (*result).yesno = NO;
		(*result).time = 0.0;
		(*result).time2 = 0.0;
		}
	else if (a == 0.0 && b == 0.0 && c > 0)/*stationary particles out of range */
		{
		(*result).yesno = NO;
		(*result).time = 0.0;
		(*result).time2 = 0.0;
		}
   else if (d < 0.0) /* non-colliding particles, at least one is moving */
		{
		(*result).yesno = NO;	
		(*result).time = 0.0;
		(*result).time2 = 0.0;
		}
	else  /* colliding particles; figure out time */
		{
		(*result).yesno = YES;
		temp = sqrt(d);
		t1 = (-b + temp) / (2*a);
		t2 = (-b - temp) / (2*a);
	   (*result).time =  dmin(t1, t2);
		(*result).time2 = dmax(t1, t2);
		}
	tprintf("result.yesno=%d, result.time=%.2lf, (*result).time2=%.2lf\n",
		(*result).yesno, (*result).time, (*result).time2);
	}

int inexact_line_collision(p, l, r, result)
Particle p;
Line l;
int r;  /* radius of collision */
Answer *result;
   {
   Vector unitNorm, L, R, D, V;
   double time, place, temp1, temp;

   difference(l.e2, l.e1, &L);
  /*
   * NOTE: The unit normal should point towards the particle p for this
   * routine to work.  If dot(u, v) > 0, where u is the unit normal and
   * v is the velocity, then we have to reverse the unit normal for the
   * equations to work correctly.
   */
	temp1 = square(L);
	temp = sqrt(temp1);
   unitNorm.x = L.y / temp;
   unitNorm.y = L.x / temp;
   if (dot(unitNorm, p.vel) > 0.0) multiply(unitNorm, -1, &unitNorm);
   multiply(unitNorm, r, &R);
   D = p.pos;
  	V = p.vel;
   time = (double) (det(l.e2, L) + det(R, L) + det(L, D)) / det(V,L);
   place = (double) (det(D,V) + det(V, l.e2) + det(V, R)) / det(V,L);
   tprintf("time=%.2lf, place=%.2lf\n", time, place);
   if (place >= 0.0 && place <= 1.0)
		{
		(*result).yesno = YES;
		(*result).time = time;
		tprintf("Line collision at a distance %d will take place at %.2lf\n", r, time);
		}
	else
		{
		(*result).yesno = NO;
		(*result).time = 0.0;
		tprintf("Line collision at a distance %d will not occur\n", r);
		}
	}

/*
 * The following function is a faster (in machine instructions) than 
 * the previous function, but will work only if line 'l' is either
 * horizontal or vertical.
 */

int fast_inexact_line_collision(p, l, radius, result)
Particle p;
Line l;
int radius;  /* radius of collision */
Answer *result;
   {
	double t1, t2, r;
	Particle p1;

   tprintf("fast_inexact_line_collision:\n");
	tprintf("line (%.2lf, %.2lf) to (%.2lf, %.2lf);\nparticle pos=(%.2lf, %.2lf), vel=(%.2lf, %.2lf)", l.e1.x, l.e1.y, l.e2.x, l.e2.y, p.pos.x, p.pos.y, p.vel.x, p.vel.y);
	r = (double) radius;
	tprintf(" r=%.2lf\n", r);
  /*
   * The if-test below corresponds to a line that is not vertical or
   * horizontal.  This routine could compute the collision if a coordinate
   * transformation is made to make the line vertical or horizontal.
   */
   if (l.e1.x != l.e2.x && l.e1.y != l.e2.y) 
		{
	   (*result).yesno = CANNOT_COMPUTE;
		(*result).time = 0.;
		tprintf("Cannot compute (*result)\n");
		}
	else /* line is OK */
		{
		if (l.e1.x == l.e2.x) /* line is vertical */
			{
			if (p.vel.x == 0.) /* no solution; particle not moving */
				{
			   (*result).yesno = NO;
				(*result).time = 0.;
				}
			else /* two solutions */
				{
			   t1 = (l.e1.x - (p.pos.x + r)) / p.vel.x;
			   t2 = (l.e1.x - (p.pos.x - r)) / p.vel.x;
				(*result).time = dmin(t1, t2);
				(*result).time2 = dmax(t1, t2);
				if ((*result).time2 < 0) (*result).yesno = NO;
				else
					{	
				   whereis(p, (long)(*result).time, &p1);
					tprintf("(*result).time=%.2lf, p1.pos.y=%.2lf\n",(*result).time,p1.pos.y);
				   if (p1.pos.y > dmax(l.e1.y, l.e2.y)) (*result).yesno = NO;
				   else if (p1.pos.y < dmin(l.e1.y, l.e2.y)) (*result).yesno = NO;
					else (*result).yesno = YES;
			      }
				}
			}
		else /* line is horizontal */
			{
			if (p.vel.y == 0)  /* no solution; particle not moving */
			   {
			   (*result).yesno = NO;
			   (*result).time = 0;
			   }
			else /* two solutions */
				{	
			   t1 = (l.e1.y - (p.pos.y + r)) / p.vel.y;
			   t2 = (l.e1.y - (p.pos.y - r)) / p.vel.y;
				(*result).time = dmin(t1, t2);
				(*result).time2 = dmax(t1, t2);
				if ((*result).time2 < 0) (*result).yesno = NO;
				else
					{	
				   whereis(p, (long)(*result).time, &p1);
					tprintf("(*result).time=%.2lf, p1.pos.x=%.2lf\n", (*result).time, p1.pos.x);
				   if (p1.pos.x > dmax(l.e1.x, l.e2.x)) (*result).yesno = NO;
				   else if (p1.pos.x < dmin(l.e1.x, l.e2.x)) (*result).yesno = NO;
					else (*result).yesno = YES;
			      }
				}
			}
		}
	}
		
		


#ifdef TEST_MOTION

main()
  	{
	Particle p1, p2;
   Line l;
   double time, radius;
   int r, whichone;
   Answer result;

   printf("Enter 1 for particle-particle collision; 2 for particle-line:");
	scanf("%d", &whichone);
	if (whichone == 1)
		{
		while (1)
			{
			printf("Enter posx, posy, velx, vely for particle 1:\n");
			scanf("%lf %lf %lf %lf", &p1.pos.x, &p1.pos.y, &p1.vel.x, &p1.vel.y);
			printf("Enter posx, posy, velx, vely for particle 2:\n");
			scanf("%lf %lf %lf %lf", &p2.pos.x, &p2.pos.y, &p2.vel.x, &p2.vel.y);
			printf("Enter detection radius:\n");
			scanf("%d", &r);
			result = inexact_particle_collision(p1, p2, r);
			if (result.yesno == YES)printf("Collision time is %lf\n", result.time);
			else printf("No collision\n");
			}
		}

   else
		{
		while (1)
			{
			printf("Enter posx, posy, velx, vely for particle 1:\n");
			scanf("%lf %lf %lf %lf", &p1.pos.x, &p1.pos.y, &p1.vel.x, &p1.vel.y);
			printf("Enter x0, y0, x1, y1 for the line:\n");
			scanf("%lf %lf %lf %lf", &l.e1.x, &l.e1.y, &l.e2.x, &l.e2.y);
			printf("Enter collision radius:\n");
			scanf("%d", &r);
			result = fast_inexact_line_collision(p1, l, r);
			if (result.yesno == YES) 
				{
				printf("Collision time is %lf\n", result.time);
				p2 = whereis(p1, (long)result.time);
				printf("The particle is at (%lf, %lf) at time of collision\n",
				p2.pos.x, p2.pos.y);
				}
			else 
				{
				printf("No collision; time = %lf\n", result.time);
				}
      	}
		}

   p1.pos.x = 1;
   p1.pos.y = 1;
   p1.vel.x = 1;
 	p1.vel.y = 1;

   p2.pos.x = 3;
   p2.pos.y = 3;
	p2.vel.x = 1;
	p2.vel.y = -1;
 
   printf("Case 1: two particles should miss\n");
   result = exact_particle_collision(p1, p2);
   printf("Collision time is %lf\n\n", result.time);
 
   p1.pos.x = 4;
   p1.pos.y = 1;
   p1.vel.x = 1;
   p1.vel.y = 0;

   p2.pos.x = 4;
   p2.pos.y = -1;
   p2.vel.x = 1;
   p2.vel.y = -1;

   printf("Case 2: two particles hit in the past\n");
   result = exact_particle_collision(p1, p2);
   printf("Collision time is %lf\n\n", result.time);

   p1.pos.x = 2;
   p1.pos.y = 2;
   p1.vel.x = 2;
   p1.vel.y = 1;

   p2.pos.x = 6;
   p2.pos.y = 3;
   p2.vel.x = -2;
   p2.vel.y = -1;

   printf("Case 3: two particles are parallel\n");
   result = exact_particle_collision(p1, p2);
   printf("Collision time is %lf\n\n", result.time);

   p1.pos.x = -2;
   p1.pos.y = 0;
   p1.vel.x = 0;
   p1.vel.y = 2;

   p2.pos.x = -4; 
   p2.pos.y = 1;
   p2.vel.x = 2;
   p2.vel.y = 1;

   printf("Case 4: hit in the future\n");
   result = exact_particle_collision(p1,p2);
   printf("Collision time is %lf\n\n", result.time);

   p1.pos.x = -3;
   p1.pos.y = -1;
   p1.vel.x = -1;
   p1.vel.y = -1;

   p2.pos.x = -4;
   p2.pos.y = -1;
   p2.vel.x = 1;
   p2.vel.y = -1;

   printf("Case 5: noninteger hit\n");
   result = exact_particle_collision(p1, p2);
   printf("Collision time is %lf\n\n", result.time);

   p1.pos.x = 1;
   p1.pos.y = 1;
   p1.vel.x = 1;
   p1.vel.y = 1;

   p2.pos.x = 3;
   p2.pos.y = 1;
   p2.vel.x = 0;
   p2.vel.y = 1;

   printf("Case 6: inexact collision around 1.0\n");
   result = inexact_particle_collision(p1, p2, 1.0);
   printf("Collision time is %lf\n\n", result.time);

   p1.pos.x = 2;
   p1.pos.y = 1;
   p1.vel.x = 0;
   p1.vel.y = 1;

   p2.pos.x = 3;
   p2.pos.y = 1;
   p2.vel.x = 0;
   p2.vel.y = -1;

   printf("Case 7: particles in range at the beginning\n");
   result = inexact_particle_collision(p1, p2, 2.0);
   printf("Collision time is %lf\n\n", result.time);

   p1.pos.x = 1;
   p1.pos.y = 1;
   p1.vel.x = 0;
   p1.vel.y = 0;

   p2.pos.x = 2;
   p2.pos.y = 1;
   p2.vel.x = 0;
   p2.vel.y = 0;

   printf("Case 8: stationary particles within detection radius\n");
   result = inexact_particle_collision(p1, p2, 2.0);
   printf("Collision time is %lf\n\n", result.time);

   p1.pos.x = 1;
   p1.pos.y = 1;
   p1.vel.x = 0;
   p1.vel.y = 0;

   p2.pos.x = 3;
   p2.pos.y = 1;
   p2.vel.x = 0;
   p2.vel.y = 0;

   printf("Case 8: stationary particles NOT within detection radius\n");
   result = inexact_particle_collision(p1, p2, 1.0);
   printf("Collision time is %lf\n\n", result.time);

   p1.pos.x = 2;
   p1.pos.y = 1;
   p1.vel.x = 0;
   p1.vel.y = 0;

   p2.pos.x = 5;
   p2.pos.y = 1;
   p2.vel.x = -1;
   p2.vel.y = 3;

   printf("Case 9: one stationary, one moving; no detection\n");
   result = inexact_particle_collision(p1, p2, 1.0);
   printf("Collision time is %lf\n\n", result.time);

   }

#endif /*ifdef TEST_MOTION */

char *printline(l, result)
Line l;
char *result;
	{
	strcpy(result, "                   ");
	sprintf(result, "(%d,%d)-(%d,%d)", l.e1.x,l.e1.y,l.e2.x,l.e2.y);
	printf("e1.x=%d, e1.y = %d, e2.x=%d, e2.y=%d\n", l.e1.x, l.e1.y,
		l.e2.x, l.e2.y);
	return result;
	}
