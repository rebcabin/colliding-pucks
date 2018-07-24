/*  	Copyright (C) 1989, 1991, California Institute of Technology.
		U.S. Government Sponsorship under NASA Contract NAS7-918
		is acknowledged.        */

/***************************************************************
 * twusrlib.h -- Header file for the Time Warp User's Library. *
 *                                                             *
 * This file should be included in the source file of all user *
 * library functions.  It should also be included into all     *
 * simulation objects which use the Time Warp User's Library.  *
 ***************************************************************/


typedef  int (*LibFncPtr)();

/*************************************************************
 * This is the packageType structure definition used for
 * defining the entry points of a package to the library.
 * The package programmer should add a packageType structure
 * to the source code for the package.
 *************************************************************/
typedef struct
   {
   LibFncPtr libinit;
   LibFncPtr libsoe;
   int soePrio;
   LibFncPtr libeoe;
   int eoePrio;
   LibFncPtr libtell;
   int tellPrio;
   LibFncPtr libterm;
   int stateSize;
   } packageType;


void * myPState();



typedef struct   
	{
   Pointer  ptrArray; /* TW pointer to the array                     */
   int      maxElem;  /* Number of elements array can currently hold */
   int      sizeElem; /* Size of each element                        */  
   int      inc;      /* Number of elements to add on array overflow */
   int      number;   /* Number of elements currently in array       */
   } TWULarrayType;


