/*      Copyright (C) 1989, 1991, California Institute of Technology.
		U. S. Government Sponsorship under NASA Contract NAS7-918
		is acknowledged.        */

/*
 * $Log:	BBN.h,v $
 * Revision 1.6  91/07/17  15:05:33  judy
 * New copyright notice.
 * 
 * Revision 1.5  91/06/03  12:22:49  configtw
 * Tab conversion.
 * 
 * Revision 1.4  90/12/12  10:05:24  configtw
 * fix a #if 0 bug
 * 
 * Revision 1.3  90/12/10  10:42:30  configtw
 * add comments for future TC2000 changes
 * 
 * Revision 1.2  90/08/12  21:55:02  steve
 * added MIN_NUM_BUFFS 16; reduces DEFAULT_NUM_BUFFS to 32
 * and MAX_NUM_BUFFS to 64.
 * 
 * Revision 1.1  90/08/07  15:37:30  configtw
 * Initial revision
 * 
*/
/* BBN.h Contains some of the required parameter for    */
/* the BBN implementations of the message system.       */


#define CP  (tw_num_nodes) 
#define ALL     -1
#define MAX_NODES       128
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

#ifdef BF_MACH


#if 0

#ifdef TC2000                   /* TC2000       */
#define MAX_NUM_BUFFS 256       
#define DEFAULT_NUM_BUFFS       64      
#define MIN_NUM_BUFFS   32
#else                           /* GP1000       */
#define MAX_NUM_BUFFS   64
#define DEFAULT_NUM_BUFFS       32
#define MIN_NUM_BUFFS   16

#endif TC2000

#endif

/* just use the following unless the TC2000 gets more memory */
#define MAX_NUM_BUFFS   64
#define DEFAULT_NUM_BUFFS       32
#define MIN_NUM_BUFFS   16

typedef struct Node_Arg_Str
{
   int num_nodes;
   char config_path[60];
   char stats_path[60];
   char stdout_path[60];
   double meg;
   int  num_buffs;
} Node_Arg_Str;
#endif


#define TIMER_EVENT     1
#define INTERRUPT_EVENT 2
#define COMMAND_EVENT   3
#define PROMPT_EVENT    4
#define END_EVENT       5
#define START_EVENT     6
#define MTIMER_EVENT    7
#ifdef DLM
#define DLM_EVENT       8
#endif DLM
#define NODE_READY_EVENT        9


#ifdef BF_PLUS
 OID  goid;
#endif

#ifdef BF_MACH

/* This data structure is used internally to maintain   */
/* the buffer pool.                                     */

typedef struct
{
	short int use_count;
	short int length;
	int src;
	int dest;
	int type;
	char data[];
}
	MESSAGE;

typedef int QUEUE_ITEM;

typedef struct
{
	QUEUE_ITEM *data;
	int size; /* number of items */
	int in;
	int out;
	short lock;
}
	FIFO_QUEUE;

#endif BF_MACH

