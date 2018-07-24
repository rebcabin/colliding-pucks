/*      Copyright (C) 1989, 1991, California Institute of Technology.
		U. S. Government Sponsorship under NASA Contract NAS7-918
		is acknowledged.        */

/*
 * $Log:	HOST_fileio.c,v $
 * Revision 1.3  91/07/17  15:06:21  judy
 * New copyright notice.
 * 
 * Revision 1.2  91/06/03  12:23:29  configtw
 * Tab conversion.
 * 
 * Revision 1.1  90/08/07  15:37:47  configtw
 * Initial revision
 * 
*/

/****************************************************************/
/*  HOST_fileio.c                       10-01-88 pjh            */
/*                                                              */
/*  This module provides a series of host computer file I/O     */
/*  functions for the TRANSPUTER, The Butterfly Plus running    */
/*  Chrysalis, the BBN GP1000 running MACH, SUN workstations    */
/*  runnint BSD Unix and the Mark 3 hypercube.                  */
/*  The purpose of these functions is to centralize all of the  */
/*  machine dependent code dealing with the file I/O of         */
/*  the various systems. Hence making the Time Warp & the       */
/*  Time Warp Simulator code alot cleaner.                      */
/*                                                              */
/*  For the Mark3 running Time Warp, the host working directory */
/*  is established at TW init time by the run program.          */
/*  If no initialization is made or <if we are using this for   */
/*  the simulator, all files will be made on cwd. JJW 1/23/89>  */
/*                                                              */
/*  For the Butterfly Plus running Chrysalis, there is no       */
/*  easy way to determine the working directory. So, it is      */
/*  currently implemented as a compile time constant.           */
/*                                                              */
/*                                                              */
/*  for tw114 jjw 11/12/88                                      */
/*  add fgetc for sun jjw 2/27/89                               */
/*                                                              */
/****************************************************************/

#include  <stdio.h>


#ifndef SIMULATOR
#include  "twcommon.h"
#include  "twsys.h"
#endif

#include "machdep.h"

#ifndef FUNCTION
#define FUNCTION
#endif



#ifdef BF_PLUS

#define NET_BUF_LEN     4096

typedef struct BBN_FCB
{
	FILE * fp;
	int buff_ctr;
	char * buff_ptr;
	char buff[100];
}HOST_FCB;

HOST_FCB host_fcb[20];

char host_wd[60] = "";
#endif

#ifdef MARK3_OR_SUN_OR_BF_MACH
char host_wd[60];
#endif

#ifdef TRANSPUTER 
char host_wd[60];
#endif




FUNCTION FILE  * HOST_fopen ( name, mode )

	char * name;
	char * mode;
{
	FILE * fp;
	char path[100];

#ifdef BF_PLUS
   FILE *n_open();
   HOST_FCB * fcb_ptr;
#endif

/**********************************************************/
/* If the caller is already giving us the full path then  */
/* just pass it on. If not then concatenate the file name */
/* to the path given in "host_wd".                        */
/**********************************************************/

#ifdef MARK3
/* The MARK3 requires the preface of "cp:" on its path    */
/* names.                                                 */

   if ( strncmp ( name, "cp:", 3 ) == 0 )
	{
		strcpy ( path, name );
	}

   else
	{
	   strcpy ( path, host_wd );
	   strcat ( path, name );
	}
#else
	   strcpy ( path, host_wd );
	   strcat ( path, name );
#endif


#ifdef TRANSPUTER

	SEMGET;
	fp = fopen ( path, mode );
	SEMFREE;    

#endif

#ifdef BF_PLUS 

	for ( fcb_ptr = host_fcb; fcb_ptr->fp != 0; fcb_ptr++ )
		;

	fp = n_open ( path, mode );
	fcb_ptr->fp = fp;
	fcb_ptr->buff_ctr = 0;
#endif

#ifdef MARK3_OR_SUN_OR_BF_MACH
	fp = fopen ( path, mode );
#endif

	return ( fp );
}

FUNCTION HOST_fclose ( fp )

	FILE * fp;
{

#ifdef TRANSPUTER

	SEMGET;
	fclose ( fp );
	SEMFREE;

#endif

#ifdef BF_PLUS 
	HOST_FCB * fcb_ptr;

	for ( fcb_ptr = host_fcb; fcb_ptr->fp != fp; fcb_ptr++ )
		;

	fcb_ptr->fp = 0;

	n_close ( fp );
#endif

#ifdef MARK3_OR_SUN_OR_BF_MACH
	fclose ( fp );
#endif


}


FUNCTION HOST_fwrite  ( ptr, size, nitems, stream )
char *ptr;
int  size, nitems;
FILE *stream;

{


#ifdef MARK3_OR_SUN_OR_BF_MACH

   fwrite ( ptr, size, nitems, stream );

#endif


#ifdef TRANSPUTER

	SEMGET;
	fwrite ( ptr, size, nitems, stream );
	SEMFREE;

#endif
 

#ifdef BF_PLUS
   int  i;

	for ( i=0; i< (size / NET_BUF_LEN ); i++ )
	 {
		 n_write ( stream, 
				   ( ptr + (NET_BUF_LEN * i)),
					 NET_BUF_LEN 
				 );
	 }
	n_write ( stream,
			  ( ptr + (NET_BUF_LEN *i) ),
			  ( size  % NET_BUF_LEN ) 
			);
				  
#endif


}



FUNCTION HOST_fprintf
		 ( fp, 
		   form, 
			arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10,
			arg11, arg12, arg13, arg14, arg15 
		 )
	FILE *fp;
	char * form;
	int *arg1, *arg2, *arg3, *arg4, *arg5, *arg6, *arg7, *arg8, *arg9,
		*arg10, *arg11, *arg12, *arg13, *arg14, *arg15;

{

	static char buff[256];

	sprintf ( buff,
				form, arg1, arg2, arg3, arg4, arg5,
				arg6, arg7, arg8, arg9, arg10,
				arg11, arg12, arg13, arg14, arg15 );
	HOST_fputs ( buff, fp );

	HOST_fflush ( fp );

 

}

 


FUNCTION HOST_fputs ( buff, fp )

	char * buff;
	FILE * fp;
{

#ifdef TRANSPUTER

	SEMGET;
	fputs ( buff, fp );
	SEMFREE;

#endif

#ifdef BF_PLUS
   int  i;

	for ( i=0; i< (strlen (buff) / NET_BUF_LEN ); i++ )
	 {
		 n_write ( fp, 
				   ( buff + (NET_BUF_LEN * i)),
					 NET_BUF_LEN 
				 );
	 }
	n_write ( fp,
			  ( buff + (NET_BUF_LEN *i) ),
			  ( ( strlen ( (buff + (NET_BUF_LEN *i ) ) ) )
				 % NET_BUF_LEN )
			);
				  
#endif

#ifdef MARK3_OR_SUN_OR_BF_MACH
	fputs ( buff, fp );
#endif



}

FUNCTION int HOST_fgets ( buff, size, fp )

	char * buff;
	int size;
	FILE * fp;
{
	int stat;

#ifdef TRANSPUTER

	SEMGET;
	stat = fgets ( buff, size, fp );
	SEMFREE;

#endif

#ifdef BF_PLUS
	char * rtn_ptr = buff;
	char * buff_ptr;
	int buff_ctr;
	HOST_FCB * fcb_ptr;

	for ( fcb_ptr = host_fcb; fcb_ptr->fp != fp; fcb_ptr++ )
		;

	buff_ctr = fcb_ptr->buff_ctr;
	buff_ptr = fcb_ptr->buff_ptr;
	stat = 0;

	for ( ;; )
	{
		if ( buff_ctr == 0 )
		{
			buff_ptr = &fcb_ptr->buff[0];
			buff_ctr = n_read ( fp, buff_ptr, 100 );
			if ( buff_ctr == 0 )
				break;
		}
		buff_ctr--;
		*rtn_ptr++ = *buff_ptr;
		stat++;

		if ( *buff_ptr++ == '\n' )
		{
			*rtn_ptr = 0;
			break;
		}
	}

	fcb_ptr->buff_ptr = buff_ptr;
	fcb_ptr->buff_ctr = buff_ctr;
#endif

#ifdef MARK3_OR_SUN_OR_BF_MACH
	stat = fgets ( buff, size, fp );
#endif


	return ( stat );
}

FUNCTION HOST_fflush ( fp )

	FILE * fp;
{

#ifdef TRANSPUTER

	 SEMGET;
	 fflush ( fp );
	 SEMFREE;

#endif

#ifdef BF_PLUS  /* Alas ! The Butterfly+  does not have fflush */
#endif


#ifdef MARK3_OR_SUN_OR_BF_MACH 
	fflush ( fp );
#endif


}

int FUNCTION HOST_fgetc( fp )

	FILE * fp;
{
#ifdef SUN
	return( fgetc(fp) );
#endif

#ifdef MARK3_OR_SUN_OR_BF_MACH
	return( fgetc(fp) );
#endif

#ifdef BF_PLUS



	char  buffch = 0;
	int buff_ctr;
	HOST_FCB * fcb_ptr;

	for ( fcb_ptr = host_fcb; fcb_ptr->fp != fp; fcb_ptr++ )
		;

	buff_ctr = fcb_ptr->buff_ctr;

	if ( buff_ctr > 0 ) buff_ctr = n_read ( fp, &buffch, 1 );
		else return(-1);

	if (buff_ctr <= 0) return(-1);
	return(buffch);


#endif
}


int FUNCTION HOST_fputc( xx,fp )

   char xx;
   FILE * fp;
{
#ifdef MARK3_OR_SUN_OR_BF_MACH
	 fputc(xx, fp) ;
#endif

#ifdef BF_PLUS
	int buff_ctr;
	HOST_FCB * fcb_ptr;

	for ( fcb_ptr = host_fcb; fcb_ptr->fp != fp; fcb_ptr++ )
		;

	buff_ctr = fcb_ptr->buff_ctr;

	buff_ctr = n_write ( fp, &xx, 1 );

	return(xx);
#endif
}

