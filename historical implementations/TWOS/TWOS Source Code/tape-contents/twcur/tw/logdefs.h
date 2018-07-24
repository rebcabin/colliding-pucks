/*      Copyright (C) 1989, 1991, California Institute of Technology.
		U. S. Government Sponsorship under NASA Contract NAS7-918
		is acknowledged.        */

/*
 * $Log:	logdefs.h,v $
 * Revision 1.4  91/07/17  15:09:20  judy
 * New copyright notice.
 * 
 * Revision 1.3  91/06/03  12:24:40  configtw
 * Tab conversion.
 * 
 * Revision 1.2  91/03/25  16:21:05  csupport
 * Declare snder[] and rcver[] properly.
 * 
 * Revision 1.1  90/08/07  15:39:59  configtw
 * Initial revision
 * 
*/


/******************************************************************/
/*  logdefs.h                           04-01-90  pjh             */
/*                                                                */
/* This file contains all of the definitions for the various      */
/* data logging facilities within Time Warp. These logging        */
/* facilities include 'flowlog', 'msglog', 'islog' and 'quelog'.  */
/* Some of the definitions given here are of two types ( as is    */
/* flowlog ): 1.) is a definition of the data as kept internally  */
/* within Time Warp. 2.) is a defintion of the data as written    */
/* out to a log file.                                             */
/******************************************************************/


/******************************************************************/
/* Definitions for Instantaneous Speedup logging. (islog)         */
/******************************************************************/


typedef struct
{
	int seqnum;
	VTime minvt;
	double cputime;
}
	IS_LOG_ENTRY;

/******************************************************************/
/* Definitions for Queue logging.                                 */
/******************************************************************/


 
typedef struct
{
#ifdef DLM
	int loadCount;
#endif DLM
	VTime gvt;
	float utilization;
}
	Q_LOG_ENTRY;
 

/******************************************************************/
/* Definitions for event logging. (flowlog)                       */
/******************************************************************/

 
typedef struct          /* This is the log record as stored     */
{                       /* internally withing Time Warp.        */
	int start_time;
	int end_time;
	float svt;
	char object[16];
}
	FLOW_LOG_ENTRY;
 

 
typedef struct          /* This is the log record as stored   */
{                       /* on the disk.                       */
	int objno;  /* To save space we id the object with an int */
	float vt;
	int cpuf;
	int cput;
}
	FLOW_COBJ;
 
/******************************************************************/
/* Definitions for message logging. (msglog)                      */
/******************************************************************/


typedef struct          /* This is the log record as stored     */
{                       /* internally withing Time Warp.        */
	int twtimef;
	int twtimet;
	int hgtimef;
	int hgtimet;
	Name snder;
	float sndtim;
	Name rcver;
	float rcvtim;
	int id_num;
	unsigned char mtype;
	unsigned char flags;
	short len;
}
	MSG_LOG_ENTRY;

typedef struct          /* This is the log record as stored   */
{                       /* on the disk.                       */
	int twtimef;
	int twtimet;
	int hgtimef;
	int hgtimet;
	float sndtim;
	float rcvtim;
	int id_num;
	short snder;
	short rcver;
	short len;
	char mtype;
	char flags;
}
	MSG_COBJ;


