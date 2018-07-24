/*      Copyright (C) 1989, 1991, California Institute of Technology.
		U. S. Government Sponsorship under NASA Contract NAS7-918
		is acknowledged.        */

/*
 * $Log:	tester.h,v $
 * Revision 1.4  91/07/17  15:13:20  judy
 * New copyright notice.
 * 
 * Revision 1.3  91/07/09  15:30:30  steve
 * moved tw_node_num and tw_num_nodes moved to twsys.h. Ack_msg changed.
 * 
 * Revision 1.2  91/06/03  12:27:06  configtw
 * Tab conversion.
 * 
 * Revision 1.1  90/08/07  15:41:20  configtw
 * Initial revision
 * 
*/

/* here is a trick to give you globaldef and globalref portably */
#ifdef TDATAMASTER
#define EXTERN /**/
#else
#define EXTERN extern
#endif

typedef char String[80];
typedef int Symbol;
typedef int Hex;

#define NAME    1
#define STIME   2
#define STRING  3
#define INTEGER 4
#define SYMBOL  5
#define HEX     6
#define REAL    7

typedef struct
{
	char *arg_name;
	int arg_type;
	int * arg_address;
}
	ARG_DEF;

typedef struct
{
	char *func_name;
	char *func_desc;
	int bcast;
	int (* routine) ();
	int * arg_list[10];
}
	FUNC_DEF;

typedef struct
{
	char *symbol;
	int value;
}
	SYM_DEF;

#define manual_mode ( object_ended == FALSE )

#define MAX_ACKS 100
#define MAX_QACKS 100

typedef struct
{
	LowLevelMsgH low;
	Ulong num;
}
	Ack_msg;

#ifdef  TIMING

#define TESTER_TIMING_MODE      0
#define TIMEWARP_TIMING_MODE    1
#define OBJECT_TIMING_MODE      2
#define SYSTEM_TIMING_MODE      3
#define IDLE_TIMING_MODE        4

EXTERN int timing[20];
EXTERN int timing_mode;
EXTERN int timing_mode_stack[20];
EXTERN int start_time, end_time;

#endif

EXTERN int ioint_cnt; 
EXTERN int ioint_cp_cnt; 
EXTERN int ioint_merc_cnt; 
EXTERN int ioint_ack_cnt; 
EXTERN int ioint_twsys_cnt; 
EXTERN int ioint_msg_cnt;
EXTERN int ioint_rcvtim_cnt;
EXTERN int ioint_bad_ack_cnt; 
EXTERN int rcvack_cnt;

EXTERN int no_message_sendback;

EXTERN Msgh * rm_msg;
EXTERN Msgh * pmq;
EXTERN Msgh * brdcst_msg;
EXTERN Msgh * brdcst_buf;

EXTERN Ack_msg qack[MAX_QACKS];
EXTERN int qackst[MAX_QACKS];

EXTERN int acks_queued;
EXTERN int ack_q_head;
EXTERN int ack_q_tail;

EXTERN int acks_pending;

EXTERN VTime min_msg_time;

EXTERN int messages_to_send;
EXTERN int stdout_messages_to_send;

EXTERN int node_cputime;

EXTERN int ( * timrproc ) ();
#ifdef DLM

/* Added for DLM interval timer. */

EXTERN int ( * timlproc ) ();

#endif DLM

EXTERN struct
{
	Int busy;
	Int node;
	Ulong num;
	VTime time;
}
	ack[MAX_ACKS];

EXTERN int object_start_time, object_end_time;

EXTERN int prop_delay;
EXTERN double delay_factor;

#undef  EXTERN
