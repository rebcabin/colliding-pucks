/*      Copyright (C) 1989, California Institute of Technology.
        U. S. Government Sponsorship under NASA Contract NAS7-918
        is acknowledged.        */

/************************************************************************
*
*	Time Warp Simulator specific defines and structure declarations
*
*	7/28/87 added instrumentation variables to obj body table
*	8/3/87 added ssize to body table for "compatible" term section
*	8/20/87 add realtime for obj and for mesg hdr: msg_time, obj_time
*	3/16/88 add obj body things for fileio
*	5/18/89 changed mesg_block q.v.     JJW
*	6/18/91 updated message block comment text which was several years
*		out of date.
*
*************************************************************************/


/************************************************************************
*
*	global defines
*
*************************************************************************/

#define TW_ACTIVE 'A'
#define TW_DELETED 'D'
#define TOKEN_SIZE 25
#define REPLY_SIZE 80
/*  MAX_ADDRESSES   now uses Max_Addresses in twcommon.h  */
#define FUNCTION

#define WSCONTINUE 1
#define WSEOF 2
#define WSERROR 3
#define WSTERM 4

/* used in twhelp (at least)  */
#define NULLCHAR '\0'
#define TAB '\t'
#define EOL '\n'
#define SPACE ' '

/* some of the following are used to size arrays */
#define	MAX_NUM_OBJECTS	 2500	/* Maximum number of objects */
#define	MAX_NUM_TYPES	 32	/* Maximum number of object types */
#define MAXNPKT          64     /* Maximum number of packets in a message */

/* default packet length, MAXPKTL, defined in twcommon.h  */
#define MSGNEXIST         2 /* Error return -- msg does not exist          */
#define MSGTRUNC          3 /* Error return -- msg truncated               */
#define MSGERROR          4 /* Error return -- catchall                    */

/************************************************************************
* 
* stream structure for file io.  These are in obj_body_fmt
*
************************************************************************/

typedef struct
{  int open_flag;
   int file_num;
   int char_pos;
}   TW_STREAM;




/************************************************************************
*
*	obj_hdr_fmt - object header format
*
*	- format for object header array entry (the header array
*	- is sorted for possible binary search)
*
*************************************************************************/

struct obj_hdr_fmt
{
	Name_object	name;		/* object name */
	int		obj_bod_ptr;	/* pointer to object body */
};

/************************************************************************
*
*	obj_bod_fmt - object body format
*
*	- format for object body array element (contains information for
*	- all created objects)
*
*************************************************************************/

struct obj_bod_fmt
{
	char		status;		/* object status */
	int		proc_ptr;	/* process table index */
	int		pnode;		/* processor node */
	VTime		create_lvt;	/* creation time */
	VTime		destroy_lvt;	/* destroy time */
	VTime		current_lvt;	/* current lvt */
	char		*current;	/* current state pointer */
	int		ssize;		/* state size */
	int		cr_count;	/* creation count */
	int             dst_count;      /* destruction count */
	int		obj_new;	/* calls to newObj */
	int		obj_del;	/* calls to delObj */
        VTime           lastime;        /* last vt event section ran */
        int             num_events;     /* nor of events for object */
     	int		num_evtmsgs;    /* calls to tell  */
	int		obj_msglimit;	/* no of msgs queued for obj */
	int		obj_msgsent;	/* tells +new +del per event for tfile */
	int		obj_cemsgs;	/* cumulative evtmsgs received */
	double		obj_time;	/* real time for event */
	double		cum_obj_time;	/* cumul time for all events */
	int		seg_table_siz;  /* current alloc table size */
	TW_STREAM	stream[MAX_TW_STREAMS]; /* for file io          */
	Pointer		**memry_pointers_ptr;   /* alloc  pointer table */
	int		memry_limit;	/* current alloc for object */
	int		segMax;		/* max of memry_limit so far */
	int		what_evt;	/* event sequence number for invocation */
	int		twulib_memry;   /* pointer to user library state memory */
};

/************************************************************************
*
*  Typtbl type table structure format for process array.  Process used to be
*  an array of ObjectType but it is now an array of Typtbl which is
*  ObjectType and some extra fields.  If yhou change ObjectType you will
*  probably have to change Typtbl.

*************************************************************************/
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
    void* libTable;             /* for twusrlib */
/* above is ObjectType */
    void* initptr;		/* pointer to typeinit returned data */
}
  Typtbl;

/************************************************************************
*
*	Event Message Block
*
*	- each event message consists of one block.
*	- Each block holds up to MSGDEFSIZ characters of the message
*	- text. Each message block uses the next and
*	- prior pointers to point to respectively the next and prior
*	- messages in the event message queue.
*	- At message deletion after
*	- processing,  the message blocks are linkd into a list
*	- available for reuse. No blocks are released to the machine
*	- operating system after allocation.
*
*************************************************************************/

/*   ******* Event Message Definitions *******   */

typedef struct emq_elem_fmt {
	struct emq_elem_fmt	*uplink;	/* pts up one level in splay */
	struct emq_elem_fmt	*next;		/* next mesg */
	struct emq_elem_fmt	*prior;		/* prior mesg */
	struct emq_elem_fmt	*submesg;	/* next sub message */
	VTime			slvt;		/* sender local virtual time */
	VTime			rlvt;		/* rcvr local virtual time */
	char			status;		/* message status */
	Name_object		sname;		/* sender name */
	Name_object		rname;		/* receiver name */
	int			mtlen;		/* message text length */
	double			msg_time;	/* real time mesg was sent */
	Long			select;		/* selector field */
	int			what_evt;	/* tot_evtcount for this message */
	Message			*text;		/* message text */
} mesg_block;

/************************************************************************
*
*  Objects created or destroyed by the user use this struct as the text in
*  the create or destroy message.
*
*************************************************************************/

typedef struct {
	int  pnode;
	Name_object rcvr;
	VTime rcvtim;
	Type_object type;
        } create_mesg;



/************************************************************************
*
*  - declarations and macros for the library support routines
*
*************************************************************************/
Pointer obj_getLibPointer();
void* obj_getLibTable();
void  obj_setLibPointer();

#define getLibPointer      (obj_getLibPointer())
#define getLibTable     (obj_getLibTable())
#define setLibPointer(x)   (obj_setLibPointer(x))


