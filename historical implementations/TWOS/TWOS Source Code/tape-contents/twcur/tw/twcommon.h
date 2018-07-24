/*      Copyright (C) 1989, 1991, California Institute of Technology.
		U. S. Government Sponsorship under NASA Contract NAS7-918
		is acknowledged.        */

/*
 * $Log:	twcommon.h,v $
 * Revision 1.9  91/12/27  09:26:00  pls
 * 1.  Combine MAXPKTL's.
 * 2.  Bump Max_Addresses to 120.
 * 3.  Add support for variable address table length (SCR 214).
 * 
 * Revision 1.8  91/11/07  08:47:50  configtw
 * Change Max_Addresses back to 100.
 * 
 * Revision 1.7  91/11/01  13:13:37  pls
 * 1.  Change ifdef's and defines.
 * 2.  Add speculative computing interface support (SCR 172).
 * 3.  Add xTypeArea support.
 * 
 * Revision 1.6  91/07/17  15:13:56  judy
 * New copyright notice.
 * 
 * Revision 1.5  91/07/09  15:35:39  steve
 * added Sun4 Macro's to use bzero, bcopy, and bcmp for clear, entcpy & bytcmp
 * 
 * Revision 1.4  91/06/03  12:27:24  configtw
 * Tab conversion.
 * 
 * Revision 1.3  91/03/26  09:55:35  pls
 * 1.  Change Max_Addresses from 100 to 200.
 * 2.  Change tell to schedule.
 * 3.  Add support for TYPEINIT and library hook.
 * 
 * Revision 1.2  90/10/15  12:50:15  reiher
 * moved FILE_NAME_LENGTH constant from twsys.h
 * 
 * Revision 1.1  90/08/07  15:41:36  configtw
 * Initial revision
 * 
*/

#ifndef TWCOMMON
#define TWCOMMON 1
/*--------------------------------------------------------------------------*
   PORTABLE DATA TYPES
		machine-        machine-
		dependent       independent
		field           type definition
*--------------------------------------------------------------------------*/
typedef long int        Addr;           /* Type of non-aliased address     */
typedef unsigned char   Byte;           /* One-byte integer                */
typedef char            Char;           /* Just for completeness' sake     */
typedef double          Dbl;            /*                                 */
typedef float           Float;          /*                                 */
typedef int            (*pfi)();        /* pointer to function             */
typedef int             Int;            /* standard integer                  */
typedef long int        Long;           /* Hopefully a 4 byte integer      */
typedef unsigned int    Uint;           /*                                 */
typedef unsigned long   Ulong;          /*                                 */
typedef int             Pointer;        /* offset into address table       */

/*--------------------------------------------------------------------------*
   BASIC TIMEWARP DATA TYPES
*--------------------------------------------------------------------------*/
#define NOBJLEN          20 /* Length of an object name                    */
#define TOBJLEN          20 /* Length of an object type name               */
#define FILE_NAME_LENGTH 41 /* Length of a file name                       */
typedef Char            Name_object[ NOBJLEN ];
typedef Char            Type_object[ TOBJLEN ];
typedef Char            Message;

typedef Name_object     Name;   /* TW's names for these types              */
typedef Type_object     Type;   /*  ''    ''   ''   ''    ''               */

typedef double STime;

typedef struct VTimeStruct
{
	STime simtime;
	Ulong sequence1;
	Ulong sequence2;
}
	VTime;

/*--------------------------------------------------------------------------*
   UID -- SYSTEM-WIDE UNIQUE ID (MESSAGES, PROCESSES, GROUPS, ETC.)
*--------------------------------------------------------------------------*/

typedef struct uid_str {
	long  num ;                 /*                                         */
	Int   node ;                /* For message id only.                    */
	}                   Uid;

typedef struct	msgRefStr
	{							/* used for message cancellation */
	Name	rcver;				/* name of receiving object */
	VTime	rcvtim;				/* receive time of message */
	Uid		gid;				/* unique message id */
	}	msgRef;

#define POSINF 999999999.0      /* positive infinity for virtual time */
#define NEGINF -999999999.0     /* negative infinity for virtual time */

#define MAXINT 0x7fffffff

#define MAXPKTL         556     /* maximum message packet length */
#if SIMULATOR
#define MINPKTL         256     /* minimum pktlen for TW proper */
#else
#define MINPKTL         540
#endif

#define MAX_TW_STREAMS  5       /* maximum file streams per object */
#define MAX_TW_FILES    60      /* maximum input files for getfile */
#define SEG_TABLE_INCREMENT 20	/* enlarge seg table by this much */
#define Max_Addresses   120     /* maximum allocated blocks per object */
#define MAXMSGS         500     /* maximum messages per event */

#define SUCCESS           0 /* only one way to succeed                     */
#define FAILURE           1 /* anything else is failure                    */

#ifdef  TRUE                /* This is to avoid an illegal redefinition    */
							/* error on the Butterfly.                     */
#undef  TRUE
#undef  FALSE
#endif

#define TRUE              1 /* Values of "Logical" type                    */
#define FALSE             0 /*                                             */

#define CRITICAL        1       /* critical memory allocation */
#define NONCRITICAL     0       /* non-critical memory allocation */

#define YES      1
#define NO       0

#define ON       1
#define OFF      0

#ifndef NULL
#define NULL            0
#endif

#define tell schedule

void *type_Area();
void *type_malloc();
void *type_myArea();

#define myTypeArea      (type_myArea())
#define xTypeArea(x)	(type_Area(x))
#define typeMalloc(x)   (type_malloc(x))

VTime obj_now();
char *obj_myName();
void *obj_myState();
int   obj_numMsgs();

#define myName(x) (obj_myName(x))
#define myState (obj_myState())
#define numMsgs (obj_numMsgs())

msgRef	delObj();
msgRef	newObj();
msgRef	schedule();

#define msgRefRcvr(x)		(x.rcver)
#define msgRefRcvTime(x)	(x.rcvtim)
#define msgRefUID(x)		(x.gid)

#define MY(x) (((State *)obj_myState())->x)

void * msgText ();
char * msgSender ();
Long   msgSelector ();
int    msgLength ();
void * pointerPtr ();
Pointer newBlockPtr ();

typedef struct
{
	char *type;
	pfi init;
	pfi event;
	pfi term;
	pfi displayMsg;
	pfi displayState;
	int statesize;
	void* (*initType)();        /* pointer to type init routine */
	Ulong libBits;              /* 1 bit reserved for each library routine */
}
  ObjectType;

VTime newVTime ();
VTime SetSimTime ();
VTime SetSequence1 ();
VTime SetSequence2 ();
VTime sscanVTime ();
VTime IncSimTime ();
VTime IncSequence1 ();
VTime IncSequence2 ();

#include "vtime.h"

#if OLDTW
typedef int Vtime;
#define now (obj_nowI())
#define tell(rcver,rcvtim,sel,len,text) tellI(rcver,rcvtim,sel,len,text)
#define msgSendTime(i) ((Vtime)msgSendTimeI(i))
#define newObj(rcver,rcvtim,objtype) newObjI(rcver,rcvtim,objtype)
#define delObj(rcver,rcvtim) delObjI(rcver,rcvtim)
#else
#define now (obj_now())
VTime  msgSendTime ();
#endif

int tw_packetLen ();
#endif

#if SUN4
#define clear(addr,numbytes) bzero(addr,numbytes)
#define entcpy(dest,src,numbytes) bcopy(src,dest,numbytes)
#define bytcmp(a,b,len) bcmp(b,a,len)
#endif
