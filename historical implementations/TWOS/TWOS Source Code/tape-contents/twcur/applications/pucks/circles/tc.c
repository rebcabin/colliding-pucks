#include "circles.h"
#include <stdio.h>
#include <signal.h>
#include <math.h>

#define Ncircles 50
#define Nlines 4

Circle c[Ncircles];
Circle oldc[Ncircles];
LineSegment l[Nlines];

#define MinX 10.
#define MaxX 890.
#define LenX 880.

#define MinY 10.
#define MaxY 440.
#define LenY 430.

#define Slop 0.01

#ifdef TRACE
#define ENDTIME 100.
#endif

int go;

ctrlc ()
{
    go = 0;
}

    Collision ball_coll[Ncircles][Ncircles];
    Collision line_coll[Ncircles][Nlines];
    int no_coll[Ncircles];
    Vector mom;
    double bmom, amom;
    double benergy, aenergy;

main ()
{
    double time, coll_time;
    int i, j, collision, ncircles;
    char line[80];
    FILE * fp = stdin;
#ifdef TRACE
    FILE * trace = fopen ( "tctrace", "w" );
#endif
    draw_border ();

fix_typo:
    fprintf ( stderr, "How Many Circles (max %d)? ", Ncircles );
    scanf ( "%d", &ncircles );
    if ( ncircles < 1 || ncircles > Ncircles )
	goto fix_typo;

    for ( i = 0; i < ncircles; i++ )
    {
	promptCircle ();
	c[i] = readCircle ();
	drawCircleAndVelocity ( &c[i] );
	oldc[i] = c[i];
	if ( i == 0 || c[i].t < time )
	    time = c[i].t;
    }

    fprintf ( stderr, "Time = %f\n", time );

    gets ( line );

    signal ( SIGINT, ctrlc );

    for ( ;; )
    {
	for ( i = 0; i < ncircles; i++ )
	{
	    fprintf ( stderr,
		"Ball %d x = %8.2f y = %8.2f vx = %8.2f vy = %8.2f\n",
		    i, c[i].p.x, c[i].p.y, c[i].v.x, c[i].v.y );

	    if ( c[i].p.x - c[i].r < MinX - Slop
	    ||   c[i].p.x + c[i].r > MaxX + Slop
	    ||   c[i].p.y - c[i].r < MinY - Slop
	    ||   c[i].p.y + c[i].r > MaxY + Slop )
	    {
		fprintf ( stderr, "\7\7\7Ball %d is Off The Board!\n", i );
		go = 0;
	    }
	}

	if ( go == 0 )
	{

get_again:
	    if ( ! fgets ( line, 80, fp ) )
	    {
		fclose ( fp );
		fp = fopen ( "/dev/tty", "r" );
		goto get_again;
	    }

	    if ( line[0] == 'c' )
	    {
		printf ( "pad:clear\n" );
		draw_border ();
		fflush (stdout);

		for ( i = 0; i < ncircles; i++ )
		{
		    drawCircleAndVelocity ( &c[i] );
		    fflush (stdout);
		    oldc[i] = c[i];
		}
		goto get_again;
	    }

	    if ( line[0] == 'g' )
		go = 1;

	    if ( line[0] == 's' )
		exit (0);
	}

#ifdef TRACE
	for ( i = 0; i < ncircles; i++ )
	{
	    fprintf ( trace, "Time %f Ball %d px %f py %f vx %f vy %f\n",
		time, i, c[i].p.x, c[i].p.y, c[i].v.x, c[i].v.y );
	}

	if ( time > ENDTIME )
	    exit (0);
#endif

	collision = 0;

	for ( i = 0; i < ncircles; i++ )
	{
	    for ( j = i + 1; j < ncircles; j++ )
	    {
		ball_coll[i][j] = circleWithCircle ( &c[i], &c[j] );

		if ( ball_coll[i][j].yes && ball_coll[i][j].at >= c[i].t )
		{
		    if ( collision == 0 || ball_coll[i][j].at < coll_time )
		    {
			collision = 1;
			coll_time = ball_coll[i][j].at;
		    }
		}
	    }
	}

	for ( i = 0; i < ncircles; i++ )
	{
	    for ( j = 0; j < Nlines; j++ )
	    {
		line_coll[i][j] = circEdgeWLineSegment ( &c[i], l[j] );

		if ( line_coll[i][j].yes && line_coll[i][j].at > c[i].t )
		{
		    if ( collision == 0 || line_coll[i][j].at < coll_time )
		    {
			collision = 1;
			coll_time = line_coll[i][j].at;
		    }
		}
	    }
	}

	if ( collision == 0 )
	{
	    fprintf ( stderr, "\7\7\7NO COLLISION!\n" );
	    time = c[0].t;
	    go = 0;
	}
	else
	{
	    time = coll_time;
	}

	fprintf ( stderr, "Time = %f\n", time );

	for ( i = 0; i < ncircles; i++ )
	{
	    if ( c[i].p.x != oldc[i].p.x
	    ||   c[i].p.y != oldc[i].p.y )
	    {
		eraseCircleAndVelocity ( &oldc[i] );
		printf ( "pad:un_arrow %f %f %f %f\n",
		    oldc[i].p.x, oldc[i].p.y, c[i].p.x, c[i].p.y );
		drawCircleAndVelocity ( &c[i] );
	        fflush (stdout);
	    }
	    oldc[i] = c[i];
	    no_coll[i] = 1;
	}

	for ( i = 0; i < ncircles; i++ )
	{
	    for ( j = 0; j < Nlines; j++ )
	    {
		if ( line_coll[i][j].yes && line_coll[i][j].at == time )
		{
		    fprintf ( stderr, "Ball %d Collides with Line %d\n", i, j );
		    circAfterLineSegColl_SE ( &c[i], l[j] );
		    no_coll[i] = 0;
		}
	    }
	}

	bmom = benergy = 0.;

	for ( i = 0; i < ncircles; i++ )
	{
	    mom = momentumCircle ( &c[i] );
	    bmom += fabs ( mom.x + mom.y );
	    benergy += energyCircle ( &c[i] );
	}
/*
	fprintf ( stderr, "Total Momentum Before Collisions = %f\n", bmom );
*/
	fprintf ( stderr, "Total Energy Before Collisions = %f\n", benergy );

	for ( i = 0; i < ncircles; i++ )
	{
	    for ( j = i + 1; j < ncircles; j++ )
	    {
		if ( ball_coll[i][j].yes && ball_coll[i][j].at == time )
		{
		    fprintf ( stderr, "Ball %d Collides with Ball %d\n", i, j );
		    circlesAfterCircColl_SE ( &c[i], &c[j] );
		    no_coll[i] = no_coll[j] = 0;
		}
	    }
	    if ( no_coll[i] )
	    {
		fprintf ( stderr, "Ball %d Moves\n", i );
		c[i] = moveCircle ( &c[i], time );
	    }
	}

	amom = aenergy = 0.;

	for ( i = 0; i < ncircles; i++ )
	{
	    mom = momentumCircle ( &c[i] );
	    amom += fabs ( mom.x + mom.y );
	    aenergy += energyCircle ( &c[i] );
	}
/*
	fprintf ( stderr, "Total Momentum After Collisions = %f\n", amom );

	if ( amom == bmom )
	    fprintf ( stderr, "Total Momentum Has Been Conserved\n" );
	else
	{
	    fprintf ( stderr, "\7\7\7Total Momentum Has Changed\n" );
	    go = 0;
	}
*/
	fprintf ( stderr, "Total Energy After Collisions = %f\n", aenergy );

	if ( benergy - aenergy < .000001 )
	    fprintf ( stderr, "Total Energy Has Been Conserved\n" );
	else
	{
	    fprintf ( stderr, "\7\7\7Total Energy Has Changed\n" );
	    go = 0;
	}

	for ( i = 0; i < ncircles; i++ )
	{
	    if ( c[i].p.x != oldc[i].p.x
	    ||   c[i].p.y != oldc[i].p.y )
	    {
		printf ( "pad:arrow %f %f %f %f\n",
		    oldc[i].p.x, oldc[i].p.y, c[i].p.x, c[i].p.y );
		drawCircleAndVelocity ( &c[i] );
		fflush (stdout);
	    }
	    else
	    if ( c[i].v.x != oldc[i].v.x
	    ||   c[i].v.y != oldc[i].v.y )
	    {
		eraseCircleAndVelocity ( &oldc[i] );
		drawCircleAndVelocity ( &c[i] );
		fflush (stdout);
	    }
	}
    }
}

draw_border ()
{
    l[0] = constructLineSegment
	(
	    constructPoint ( MinX, MinY ),
	    constructVector ( LenX, 0. )
	);

    drawLineSegment ( l[0] );

    l[1] = constructLineSegment
	(
	    constructPoint ( MaxX, MinY ),
	    constructVector ( 0., LenY )
	);

    drawLineSegment ( l[1] );

    l[2] = constructLineSegment
	(
	    constructPoint ( MaxX, MaxY ),
	    constructVector ( -LenX, 0. )
	);

    drawLineSegment ( l[2] );

    l[3] = constructLineSegment
	(
	    constructPoint ( MinX, MaxY ),
	    constructVector ( 0., -LenY )
	);

    drawLineSegment ( l[3] );
    fflush (stdout);
    

}
