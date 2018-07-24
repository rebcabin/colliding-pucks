#include <stdio.h>
#include "Puck.h"
#define IO_FUNCTION

/*****************************************************************/
/*********************       scanPuck       **********************/
/*****************************************************************/

IO_FUNCTION Puck scanPuck()
{
    Puck ret ;

    scanf ("%s", ret.name) ;
    scanf ("%ld", &ret.time) ;
    scanf ("%lf", &ret.mass) ;
    scanf ("%lf", &ret.xdot) ;
    scanf ("%lf", &ret.ydot) ;

    return ret ;
}

/*****************************************************************/
/*********************       printPuck       *********************/
/*****************************************************************/

IO_FUNCTION void printPuck (inst)

    Puck inst ;
{
    printf ("%s", inst.name) ;
    printf ("\t") ;
    printf ("%ld", inst.time) ;
    printf ("\t") ;
    printf ("%lf", inst.mass) ;
    printf ("\t") ;
    printf ("%lf", inst.xdot) ;
    printf ("\t") ;
    printf ("%lf", inst.ydot) ;
}

/*****************************************************************/
/*********************       readPuck        *********************/
/*****************************************************************/

IO_FUNCTION Puck readPuck()
{
    Puck ret ;

    fread (&ret, sizeof(ret), 1, stdin) ;
    return ret ;
}

/*****************************************************************/
/*********************       writePuck       *********************/
/*****************************************************************/

IO_FUNCTION void writePuck (inst)

    Puck inst ;
{
    fwrite (&inst, sizeof(inst), 1, stdout) ;
}

/*****************************************************************/
/*********************       fscanPuck       *********************/
/*****************************************************************/

IO_FUNCTION Puck fscanPuck(f)

    FILE * f ;
{
    Puck ret ;

    fscanf (f, "%s", ret.name) ;
    fscanf (f, "%ld", &ret.time) ;
    fscanf (f, "%lf", &ret.mass) ;
    fscanf (f, "%lf", &ret.xdot) ;
    fscanf (f, "%lf", &ret.ydot) ;

    return ret ;
}

/*****************************************************************/
/********************       fprintPuck       *********************/
/*****************************************************************/

IO_FUNCTION void fprintPuck (f, inst)

    FILE * f ;
    Puck inst ;
{
    fprintf (f, "%s", inst.name) ;
    fprintf (f, "\t") ;
    fprintf (f, "%ld", inst.time) ;
    fprintf (f, "\t") ;
    fprintf (f, "%lf", inst.mass) ;
    fprintf (f, "\t") ;
    fprintf (f, "%lf", inst.xdot) ;
    fprintf (f, "\t") ;
    fprintf (f, "%lf", inst.ydot) ;
}

/*****************************************************************/
/********************        freadPuck       *********************/
/*****************************************************************/

IO_FUNCTION Puck freadPuck(f)

    FILE * f ;
{
    Puck ret ;

    fread (&ret, sizeof(ret), 1, f) ;
    return ret ;
}

/*****************************************************************/
/********************       fwritePuck       *********************/
/*****************************************************************/

IO_FUNCTION void fwritePuck (f, inst)

    FILE * f ;
    Puck inst ;
{
    fwrite (&inst, sizeof(inst), 1, f) ;
}

/*****************************************************************/
/********************       vprintPuck       *********************/
/*****************************************************************/

IO_FUNCTION void vprintPuck (inst)

    Puck inst ;
{
    printf ("name                             %s\n", inst.name) ;
    printf ("time                             %ld\n", inst.time) ;
    printf ("mass                             %lf\n", inst.mass) ;
    printf ("xdot                             %lf\n", inst.xdot) ;
    printf ("ydot                             %lf\n", inst.ydot) ;
}

/*****************************************************************/
/********************        vscanPuck       *********************/
/*****************************************************************/

IO_FUNCTION Puck vscanPuck ()
{
    Puck ret ;
    char junk [80] ;

    scanf ("%s %s", junk, ret.name) ;
    scanf ("%s %ld", junk, &ret.time) ;
    scanf ("%s %lf", junk, &ret.mass) ;
    scanf ("%s %lf", junk, &ret.xdot) ;
    scanf ("%s %lf", junk, &ret.ydot) ;
    return ret ;
}

/*****************************************************************/
/********************       fvprintPuck       ********************/
/*****************************************************************/

IO_FUNCTION void fvprintPuck (f, inst)

    FILE * f ;
    Puck inst ;
{
    fprintf (f, "name                             %s\n", inst.name) ;
    fprintf (f, "time                             %ld\n", inst.time) ;
    fprintf (f, "mass                             %lf\n", inst.mass) ;
    fprintf (f, "xdot                             %lf\n", inst.xdot) ;
    fprintf (f, "ydot                             %lf\n", inst.ydot) ;
}

/*****************************************************************/
/********************       fvscanPuck        ********************/
/*****************************************************************/

IO_FUNCTION Puck fvscanPuck (f)

    FILE * f ;
{
    Puck ret ;
    char junk [80] ;

    fscanf (f, "%s %s", junk, ret.name) ;
    fscanf (f, "%s %ld", junk, &ret.time) ;
    fscanf (f, "%s %lf", junk, &ret.mass) ;
    fscanf (f, "%s %lf", junk, &ret.xdot) ;
    fscanf (f, "%s %lf", junk, &ret.ydot) ;
    return ret ;
}

