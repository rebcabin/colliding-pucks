/* 
 * Transform.c - Fred Wieland, Jet Propulsion Laboratory, November 1987
 *
 * This package transforms Tartesian (x,y) coordinates into PAD screen
 * coordinates.  PAD uses the SUNVIEW convention that (0,0) is on the top
 * left hand side of the window, with positive x to the right and positive
 * y down.  Tartesian coordinates assumes (0,0) is on the bottom left
 * corner of the screen, with positive x to the left and postive y up.
 * This package is not general; it depends upon constants defined in
 * ctls.h and thus is specific to the TTLS SWTB88-1 model.
 */
#include "twcommon.h"
#include "stb88.h"
#include "motion.h"

int Tvector(s, x0, y0, x1, y1)
char *s;
double x0, y0, x1, y1;
	{
	double topy = NORTH_BOUND / SCALE_FACTOR;

	sprintf(s,  "vector %d %d %d %d\n", (int)x0, (int)(topy-y0), 
				(int)x1, (int)(topy-y1) );
	}

int Terasevector(s, x0, y0, x1, y1)
char *s;
double x0, y0, x1, y1;
	{
	double topy = NORTH_BOUND / SCALE_FACTOR;

	sprintf(s,  "erasevector %d %d %d %d\n", (int)x0, (int)(topy-y0), 
				(int)x1, (int)(topy-y1) );
	}

int Tcircle16(s, x0, y0, radius) 
char *s;
double x0, y0, radius;
	{
	double topy = NORTH_BOUND / SCALE_FACTOR;

	sprintf(s, "circle16 %d %d %d\n", (int)x0, (int)(topy-y0), (int)radius);
	}

int Trectangle(s, x0, y0, w, h) 
char *s;
double x0, y0, w, h;
	{
	double topy = NORTH_BOUND / SCALE_FACTOR;
	
	sprintf(s, "rectangle %d %d %d %d\n", (int)x0, (int)(topy-y0), 
				(int)w, (int)h);
	}

int Tfilledrect(s, x0, y0, w, h)
char *s;
double x0, y0, w, h;
	{
 	double topy = NORTH_BOUND / SCALE_FACTOR;

	sprintf(s, "filledrect %d %d %d %d\n", (int)x0, (int)(topy-y0), 
				(int)w, (int)h);
	}

int Tpaintstring(s, x0, y0, t)
char *s;
double x0, y0;
char *t;
	{
	double topy = NORTH_BOUND / SCALE_FACTOR;

	sprintf(s, "paintstring %d %d %s\n", (int)x0, (int)(topy-y0), t);
	}

int Tprintstring(s, x0, y0, t)
char *s;
double x0, y0;
char *t;
	{
	double topy = NORTH_BOUND / SCALE_FACTOR;
	
	sprintf(s, "printstring %d %d %s\n", (int)x0, (int)(topy-y0), t);
	}

int Terasestring(s, x0, y0, t)
char *s;
double x0, y0;
char *t;
	{
	double topy = NORTH_BOUND / SCALE_FACTOR;
	
	sprintf(s, "erasestring %d %d %s\n", (int)x0, (int)(topy-y0), t);
	}

int Teraserect(s, x0, y0, w, h)
char *s;
double x0, y0, w, h;
	{
	double topy = NORTH_BOUND / SCALE_FACTOR;

	sprintf(s, "eraserect %d %d %d %d\n", (int)x0, (int)(topy-y0), 
			(int)w, (int)h);
	}

int Terasecircle16(s, x0, y0, r)
char *s;
double x0, y0, r;
	{
	double topy = NORTH_BOUND / SCALE_FACTOR;

	sprintf(s, "erasecircle16 %d %d %d\n", (int)x0, (int)(topy-y0), (int)r);
	}

int Tbig_dot(s, x0, y0)
char *s;
double x0, y0;
	{
	double topy = NORTH_BOUND / SCALE_FACTOR;

	sprintf(s, "big_dot %d %d\n", (int)x0, (int)(topy-y0));
	}

int Tcoord(v1, v2)
Vector v1, *v2;
	{
	double topy = NORTH_BOUND / SCALE_FACTOR;

	v2->x = v1.x;
	v2->y = topy - v1.y;
	}


