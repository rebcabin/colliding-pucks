/*      Copyright (C) 1989, 1991, California Institute of Technology.
		U. S. Government Sponsorship under NASA Contract NAS7-918
		is acknowledged.        */

/*
 * $Log:	machdep.h,v $
 * Revision 1.9  91/12/27  11:27:38  reiher
 * Turned on dynamic window throttling
 * 
 * Revision 1.8  91/11/01  09:33:23  pls
 * Change ifdef's and defines.
 * 
 * Revision 1.7  91/07/17  15:09:35  judy
 * *** empty log message ***
 * 
 * Revision 1.6  91/07/09  14:07:34  steve
 * Support for Sun MicroTime, ObjectTimingMode, Mercury like messaging.
 * 
 * Revision 1.5  91/06/03  14:26:10  configtw
 * Add GETRUSAGE to Suns.
 *
 * Revision 1.4  91/06/03  12:24:47  configtw
 * Tab conversion.
 *
 * Revision 1.3  90/08/27  10:44:08  configtw
 * Leave out page fault stuff for Sun.
 * 
 * Revision 1.2  90/08/16  10:28:54  steve
 * #defined GETRUSAGE for counting page faults functionality
 * 
 * Revision 1.1  90/08/07  15:40:04  configtw
 * Initial revision
 * 
*/

/* machdep.h Contains the definitions for most of the   */
/* machine dependent data strucutres and pulls in the   */
/* required system dependent .h files.                  */

/* Modified to run under MACH on the BBN GP1000 pjh     */



/* MARK3 refers to the Caltech/JPL Mark3 hypercube      */
/* BF_PLUS refers to the BBN Butterfly Plus running     */
/*      the Chrysalis Operating System.                 */
/* BF_MACH refers to the BBN GP1000 running MACH        */
/* BBN is used for code that applies to both BF_PLUS    */
/*      and BF_MACH.                                    */
/* SUN refers to any SUN workstation running BSD Unix   */
/* TRANSPUTER refers to T800 system.                    */

/* Some of the code between various implementations is  */
/* quite similar. For this reason, we set up some system*/
/* defines to handle these cases.                       */


#if BF_MACH
#undef BBN
#define BBN 1
#define SUN_OR_BF_MACH 1
#define MARK3_OR_BBN 1	/* Msg systems between BBN and MARK3 are */
						/* made to look as alike as possible     */
#define MARK3_OR_SUN_OR_BF_MACH 1	/* For HOST_fileio.c           */
#define UNIX_STREAM_FILES 1		/* fopen,fclose for HOST_fileio.c */
#define GETRUSAGE 1		/* for counting page faults              */
#define DYNWINTHROT		/* For dynamic window throttling - compiled in,
							but turned off; use config switch to turn on. */
#endif

#if MARK3 
#define MARK3_OR_BBN 1	/* Msg systems between BBN and MARK3 are */
						/* made to look as alike as possible     */
#define MARK3_OR_SUN_OR_BF_MACH 1	/* For HOST_fileio.c           */
#define UNIX_STREAM_FILES 1		/* fopen,fclose for HOST_fileio.c */
#endif

#if SUN4
#define SUN 1
#endif

#if SUN
#undef SUN
#define SUN 1
#define MARK3_OR_SUN_OR_BF_MACH 1	/* For HOST_fileio.c			*/
#define UNIX_STREAM_FILES 1		/* fopen,fclose for HOST_fileio.c	*/
#define GETRUSAGE 1			/* for counting page faults				*/

#define MICROTIME 1

#define TICKS_SHIFT 0   /* SUNtime.c */
#define MICROSECONDS_PER_TICK (1 << TICKS_SHIFT)
#define TICKS_PER_MILLISECOND (1000/(1 << TICKS_SHIFT))
#define TICKS_PER_SECOND (1000000/(1 << TICKS_SHIFT))

/* how objests are timed, wall is real time, user is process time */
#define NOOBJTIME 0
#define USEROBJTIME 1
#define WALLOBJTIME 2
extern int object_timing_mode;
#endif


#if TDATAMASTER
#define EXTERN  /**/
#else
#define EXTERN  extern
#endif

/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/

#if MARK3

#include <mercury.h>
#include <global.h>   /* used by timewarp.c */
#include <pitdef.h>  /* used by machdep.c, msgcntl.c, timewarp.c */
#include <signal.h>             /* timewarp.c */

#define MAX_NODES 128
#define BRDCST_ABLE 1

EXTERN MSG_STRUCT send;
EXTERN MSG_STRUCT recv;

EXTERN char * stats_name;

extern char now_async;

#ifndef SIMULATOR

EXTERN Msgh * stdout_q;
EXTERN int stdout_acks_pending;
EXTERN Msgh * save_sim_end_msg;

#endif

#endif

EXTERN int node_offset;
EXTERN int node_limit;

/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/

#if BBN

#include "BBN.h"

#define BRDCST_ABLE 1

EXTERN MSG_STRUCT send;
EXTERN MSG_STRUCT recv;

#define ANY     -1

EXTERN char * stats_name;

#ifndef SIMULATOR

EXTERN Msgh * stdout_q;
EXTERN int stdout_acks_pending;
EXTERN Msgh * save_sim_end_msg;

EXTERN char * rm_buf;

#endif

#endif BBN

/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/

#if SUN

/* for getime.c */

#include <sys/time.h>
#include <sys/resource.h>

#include <signal.h>             /* timewarp.c, network.c */

#include <sys/types.h>          /* network.c */
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <fcntl.h>

#define MAX_NODES 10


#define CP  (tw_num_nodes) 
#define ALL     -1
#define ANY     -1

#define OK      1
#define DONE    1

typedef struct mstrct {

  int   *buf;   /* message buffer       */
  int   blen;
  int   mlen;
  int   dest;
  int   source;
  int   type;

 } MSG_STRUCT,  *MSG_STRUCT_PTR;


extern int msg_ichan[MAX_NODES + 1];
extern int msg_ochan[MAX_NODES + 1];
extern int ctl_ichan;
extern int ctl_ochan;

EXTERN char * rm_buf;
EXTERN MSG_STRUCT send;
EXTERN MSG_STRUCT recv;

#endif

#undef  EXTERN
