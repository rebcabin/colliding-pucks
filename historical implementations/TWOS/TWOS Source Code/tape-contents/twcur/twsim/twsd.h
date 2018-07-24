/*      Copyright (C) 1989, California Institute of Technology.
        U. S. Government Sponsorship under NASA Contract NAS7-918
        is acknowledged.        */

/************************************************************************
*                      TIME WARP SIMULATOR                 v2113        *
*	      Global Variable declarations file TWSD.H                  *
*	    (See file TWSD.C for the actual definitions)		*
*									*
*									*
* Modified   JJW 10/3/86;  |   Mod JJW 10/28/86				*
* Unified    LVW 10/6/86;  |   unify with twsd.c 1-9-87			*
* new type table JJW 9/30/88 - see at end of file 			*
*									*
*************************************************************************/

/******************************************************************
*
* This is the ONLY file that needs to be changed to add or change new
* global variables to the simulator.
*
* variables preceded by EXTE will be declared if this file twsd.h is included.
*
* twsd.c file defines DEF and turns all these declarations into definitions.
* If the variable needs to be initialized you are stuck with using ifdef DEF
* as a conditional with an else clause. (see the end of this file for an
* example)
*
**********************************************************************/

#ifndef DEF 
#define EXTE extern
#else
#define EXTE
#endif

/************************************************************************
*
*	- the following indexes and tables hold information on all
*	- objects that may exist during the simulation
*
*	- gl_type_ind - index into the type table that contains the
*	-               entry points for an object's init, event and
*	-		other code sections as well as the object's
*	-               state size.
*	- type_ind    - local index into the object type table
*
*	- gl_hdr_ind  - index into the object name table (sorted)
*	- gl_bod_ind  - index into the object body table (not sorted)
*	- st_bod_ind  - index stack
*	- hdr_ind     - local index
*	- bod_ind     - local index
*	
*	- created object tables -
*	- at creation, an objects name is inserted into the object header
*	-    table in sorted order to facilitate binary searching.
*	- all other information about an object is inserted into the
*	-    object body table including an index to its type in the type
*	-    table, virtual times specifying current time, creation and
*	-    destruction time, and pointers to current and prior states.
*
*************************************************************************/

EXTE  int  type_ind;
EXTE  int  gl_type_ind;

EXTE  int  bod_ind;
EXTE  int  hdr_ind;
EXTE  int  gl_bod_ind;
EXTE  int  gl_hdr_ind;

EXTE  int  num_objects;
EXTE  struct obj_hdr_fmt obj_hdr [MAX_NUM_OBJECTS];
EXTE  struct obj_bod_fmt obj_bod [MAX_NUM_OBJECTS +1];
	/* extra obj_bod for typeinit */

/*************************************************************************
*
*  type table array   type_table  Typtbl
*
*  This struct defines the process array or type table.  It used to be an
* 
*  ObjectType array but now has some extra fields in it.  Consequently
*  a change in the definition of struct ObjectType will require a change
*  in the type table definition. ObjectType defined in twcommon.h.
*  typeType defined in tws.h
*
*  ObjectType fields filled in from the entry point, name, and statesize
*  data in table.c for the application.
*
************************************************************************/
EXTE  Typtbl   process[MAX_NUM_TYPES];
EXTE  int   num_types;


/**************************************************************************
*
*	- the following pointers are used for the event message queue
*	- emq_first_ptr points to the actual first message in the queue
*	- emq_current_ptr points to the first message in the queue that
*	-    is either being processed or is next to be processed
*	-    (after processing, a message is left queued, but is
*	-     marked as "deleted", and is available for reuse.
*	-     to be reused the message is reinitialized and relinked into
*	-     its proper place in the queue. ie. if any messages have been
*	-     processed, emq_first_ptr points to the beginning of the
*	-     reusable message block pool. only after these available 
*	-     message blocks have been used are new message blocks
*	-     physically allocated) (also, event messages consist
*	-     of possibly more than one submessage block. after deletion
*	-     an event message is decomposed and each submessage block
*	-     is linked in to make it available for reuse)
*	- the other pointers tend to be more temporary in nature
*	-     and are used locally in specific functions
*
*
*************************************************************************/

EXTE  mesg_block *mb_ptr;
EXTE  mesg_block *emq_first_ptr;
EXTE  mesg_block *emq_current_ptr;
EXTE  mesg_block *emq_next_ptr;
EXTE  mesg_block *emq_prior_ptr;
EXTE  mesg_block *mb_ptr_1;
EXTE  mesg_block *mb_ptr_2;
EXTE  mesg_block *mb_ptr_3;
EXTE  mesg_block *emq_endmsg_ptr;

#ifndef DEF
extern int  mb_size;
#else
int mb_size = sizeof (mesg_block);
#endif


/************************************************************************
*
*	- configuration file variables
*	- inode		- node to create first object on
*	- fmsg_flag 	- first message needs to be sent
*	- config_switch - user controlled switch
*
*************************************************************************/

EXTE  int  config_switch;
EXTE  int  fmsg_flag;
EXTE  int  inode;
EXTE  STime  cutoff_time;

/************************************************************************
*
*	- variables needed for debug routines
*
*************************************************************************/

extern char *dprep_mesg ();
extern  char *dprep_text ();

EXTE  int  dnopt, dtopt;
EXTE  VTime dvtime;
EXTE  Name_object dname;


/************************************************************************
*
*	- global switches
*
*************************************************************************/

EXTE int  prog_status;
EXTE int  debug_switch;
EXTE int  step_switch;	/* this is a tri-state switch */
EXTE int  archive_switch;
EXTE int  tfile_sw;
EXTE int  mfile_sw;
EXTE int  cpath_sw;	/* critical path switch */
EXTE int  elog_sw;	/* event log switch */
EXTE int  notime_sw;	/* remove all internal timing calls */
EXTE int  statfile_sw;
EXTE int  bp_switch;	/* breakpoint sw */
EXTE int  alrm_sw;
EXTE int  stkchk_sw;

/************************************************************************
*
*	- terminal input variables
*
*************************************************************************/

EXTE  char mreply [REPLY_SIZE + 1];
EXTE int   mreplyptr;
EXTE  char token [TOKEN_SIZE + 1];
EXTE  char reason [REPLY_SIZE + 1];
EXTE char bp_token [TOKEN_SIZE + 1];
EXTE char bp_timetoken [TOKEN_SIZE + 1];

/************************************************************************
*
*	- message pointer array and variables
*	- (an object may reference all event messages addressed to
*	-  itself to be received at the current virtual time. these
*	-  messages have their pointers stored here.)
*
*************************************************************************/

EXTE  int  qu_mesg_ind;
EXTE  int  mesg_ind;
EXTE  int  mesg_lmt;
EXTE  mesg_block *mesg_ptr [MAXMSGS];

/************************************************************************
*
*	- timing  variables
*
*************************************************************************/
EXTE  double  evt_realtime;
EXTE  double  evt_parttime;
EXTE  double  queue_time;
EXTE  double  dequeue_time;
EXTE  double  multiple;		/* multiplier for prop timing */


/************************************************************************
*
*	- counts for time warp module entry by application routines
*
*************************************************************************/

EXTE long simtm_cnt;
EXTE long me_cnt;
EXTE long evtmsg_cnt;
EXTE long event_cnt;
EXTE long stdout_cnt;
EXTE long time_cnt;
EXTE long obcre_cnt;
EXTE long obdes_cnt;
EXTE long mcount_cnt;
EXTE long getmsg_cnt;
EXTE long getsel_cnt;
EXTE long sender_cnt;
EXTE long sndtm_cnt;
EXTE int test_var1;	/* 2 gen purpose test vars printed in statistics */
EXTE int test_var2;

/************************************************************************
*
*	- variable for stats gathering
*
*************************************************************************/

EXTE long req_blk_cnt;
EXTE long all_blk_cnt;
EXTE long num_mesg_bytes;
EXTE long num_state_bytes;

/************************************************************************
*
*	- pointer variable containing address of memory alloc routine
*
*************************************************************************/
/* for new type table
extern char *_mi_alloc ();

#ifndef DEF
extern char *(*mi_alloc_ptr) ();
#else
char *(*mi_alloc_ptr) () = _mi_alloc;
#endif
*/

