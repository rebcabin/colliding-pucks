#include <stdio.h>
#include "ThermData.h"
#define IO_FUNCTION

/*****************************************************************/
/*******************       scanThermData       *******************/
/*****************************************************************/

IO_FUNCTION ThermData scanThermData()
{
    ThermData ret ;

    scanf ("%ld", &ret.N) ;
    scanf ("%lg", &ret.time) ;
    scanf ("%lg", &ret.avgKE) ;
    scanf ("%lg", &ret.avgKECM) ;
    scanf ("%lg", &ret.stdDevKE) ;

    return ret ;
}

/*****************************************************************/
/******************       printThermData       *******************/
/*****************************************************************/

IO_FUNCTION void printThermData (inst)

    ThermData inst ;
{
    printf ("%ld", inst.N) ;
    printf ("\t") ;
    printf ("%lg", inst.time) ;
    printf ("\t") ;
    printf ("%lg", inst.avgKE) ;
    printf ("\t") ;
    printf ("%lg", inst.avgKECM) ;
    printf ("\t") ;
    printf ("%lg", inst.stdDevKE) ;
}

/*****************************************************************/
/******************        readThermData       *******************/
/*****************************************************************/

IO_FUNCTION ThermData readThermData()
{
    ThermData ret ;

    fread (&ret, sizeof(ret), 1, stdin) ;
    return ret ;
}

/*****************************************************************/
/******************       writeThermData       *******************/
/*****************************************************************/

IO_FUNCTION void writeThermData (inst)

    ThermData inst ;
{
    fwrite (&inst, sizeof(inst), 1, stdout) ;
}

/*****************************************************************/
/******************       fscanThermData       *******************/
/*****************************************************************/

IO_FUNCTION ThermData fscanThermData(f)

    FILE * f ;
{
    ThermData ret ;

    fscanf (f, "%ld", &ret.N) ;
    fscanf (f, "%lg", &ret.time) ;
    fscanf (f, "%lg", &ret.avgKE) ;
    fscanf (f, "%lg", &ret.avgKECM) ;
    fscanf (f, "%lg", &ret.stdDevKE) ;

    return ret ;
}

/*****************************************************************/
/******************       fprintThermData       ******************/
/*****************************************************************/

IO_FUNCTION void fprintThermData (f, inst)

    FILE * f ;
    ThermData inst ;
{
    fprintf (f, "%ld", inst.N) ;
    fprintf (f, "\t") ;
    fprintf (f, "%lg", inst.time) ;
    fprintf (f, "\t") ;
    fprintf (f, "%lg", inst.avgKE) ;
    fprintf (f, "\t") ;
    fprintf (f, "%lg", inst.avgKECM) ;
    fprintf (f, "\t") ;
    fprintf (f, "%lg", inst.stdDevKE) ;
}

/*****************************************************************/
/******************       freadThermData        ******************/
/*****************************************************************/

IO_FUNCTION ThermData freadThermData(f)

    FILE * f ;
{
    ThermData ret ;

    fread (&ret, sizeof(ret), 1, f) ;
    return ret ;
}

/*****************************************************************/
/******************       fwriteThermData       ******************/
/*****************************************************************/

IO_FUNCTION void fwriteThermData (f, inst)

    FILE * f ;
    ThermData inst ;
{
    fwrite (&inst, sizeof(inst), 1, f) ;
}

/*****************************************************************/
/******************       vprintThermData       ******************/
/*****************************************************************/

IO_FUNCTION void vprintThermData (inst)

    ThermData inst ;
{
    printf ("N                                %ld\n", inst.N) ;
    printf ("time                             %lg\n", inst.time) ;
    printf ("avgKE                            %lg\n", inst.avgKE) ;
    printf ("avgKECM                          %lg\n", inst.avgKECM) ;
    printf ("stdDevKE                         %lg\n", inst.stdDevKE) ;
}

/*****************************************************************/
/******************       vscanThermData        ******************/
/*****************************************************************/

IO_FUNCTION ThermData vscanThermData ()
{
    ThermData ret ;
    char junk [80] ;

    scanf ("%s %ld", junk, &ret.N) ;
    scanf ("%s %lg", junk, &ret.time) ;
    scanf ("%s %lg", junk, &ret.avgKE) ;
    scanf ("%s %lg", junk, &ret.avgKECM) ;
    scanf ("%s %lg", junk, &ret.stdDevKE) ;
    return ret ;
}

/*****************************************************************/
/*****************       fvprintThermData       ******************/
/*****************************************************************/

IO_FUNCTION void fvprintThermData (f, inst)

    FILE * f ;
    ThermData inst ;
{
    fprintf (f, "N                                %ld\n", inst.N) ;
    fprintf (f, "time                             %lg\n", inst.time) ;
    fprintf (f, "avgKE                            %lg\n", inst.avgKE) ;
    fprintf (f, "avgKECM                          %lg\n", inst.avgKECM) ;
    fprintf (f, "stdDevKE                         %lg\n", inst.stdDevKE) ;
}

/*****************************************************************/
/*****************        fvscanThermData       ******************/
/*****************************************************************/

IO_FUNCTION ThermData fvscanThermData (f)

    FILE * f ;
{
    ThermData ret ;
    char junk [80] ;

    fscanf (f, "%s %ld", junk, &ret.N) ;
    fscanf (f, "%s %lg", junk, &ret.time) ;
    fscanf (f, "%s %lg", junk, &ret.avgKE) ;
    fscanf (f, "%s %lg", junk, &ret.avgKECM) ;
    fscanf (f, "%s %lg", junk, &ret.stdDevKE) ;
    return ret ;
}

