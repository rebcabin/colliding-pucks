/* "Copyright (C) 1989, California Institute of Technology. 
     U. S. Government Sponsorship under NASA Contract 
   NAS7-918 is acknowledged." */
 /**********************************************************************
 *
 *  TWSP2   Timewarp simulator part 2
 *	    Timewarp Entry Points
 *
 *  fix for negative start virtual times JJW 10/16/86
 *  fix errors  JJW 10/23/86 
 *  fix void declarations for internal subfunctions JJW 12/9/86
 *  change sorting algorithm in cm_find_emq_position JJW 6/16/87
 *  fix cm_find_emq_position to start at end of msg group JJW 6/18/87
 *  include all three sorting algorithms by conditional #'s JJW 6/19/87
 *  merge with markIII under conditional MARK3  7/1/87
 *  fix some bugs incl entcpy and put entcpy under ifnot CPROG  JJW
 *  add instrumentation to evtmsg JJW 7/28/87
 *  fix _mi_alloc for termination section (ssiz) JJW 8/3/87
 *  add CRIT stuff, 8/26/87; and reestablish normal timing 9/22/87
 *  change CRIT strategy  10/18/87
 *  put in proportional delay 12/7/87
 *  add notime switch 1/22/88
 *  add always compiled stuff for twqueue heap. see %&%&
 *  put in assy lang clear (if SUN ) for mi_clear 3/17/88
 *  make MSGDEFSIZ a variable mesgdefsize set by pktlen or default 256
 *  8/19/88  take out CRIT (always in ), put in HISP
 *  9/15/88  put in create and destroy using parts of old obcreate, obdestroy
 *  9/30/88  fix for new init and type table
 *  10/26/88  new user interface
 *  11/14/88  fix for all machines
 *  1/31/89  remove queries
 *  3/28/89  JJW and MD  remove CPROG and bytecomp
 *  5/8/89  change to VTime with double and 2 longs
 *  12/6/89  remove multipacket message support in cm_create etc.
 *  2/19/90 Improve output data when dt is turned on.
 *  8/28/90  Fix bug which permanently changed indexes inside newObj delObj - see
 *		cr_create_obj.
 *
 ********************************************************************/


#ifdef SUN
#include <stdio.h>
typedef int     time_t;
#endif 

#ifdef MARK3
#include "stdio.h"
#endif 

#ifdef BF_MACH		/* Surely this repetitive nonsense   */
#include <stdio.h>	/* can be fix by a properly worked   */
#endif			/* Makefile?			     */
#ifdef BF_PLUS
#include <stdio.h>
#endif

#ifdef TRANSPUTER
#include "stdio.h"
#endif

#include "twcommon.h"
#include "machdep.h"
#include "tws.h"
#include "twsd.h"

#define STRSIZ 40

/************************************************************************
*
*	type table - entry points for the application's init, event, and
*		     term code sections (considered processes when
*	             executed)
*
*	- data for this table is created by the
*	- application programmer in a separately compiled module,
*	- (generally table.c). the application code section entry
*	- points are resolved at linkage time. the routine cr_typetable
*	- creates the table from that data and automatically puts in
*	- a stdout type if the user does not specify one.
*
*************************************************************************/

extern char dtext[];
extern int      num_types;
extern int elap_time;
extern int tot_evtcount;	/* in twsp1.c */
extern int now_msg_flag;	/* in newconf.cyy */
extern FILE *arc_file;
extern char    *sim_malloc();
extern double   itimer();
extern double tot_evt_time;
extern void error();
extern void sim_debug();
extern FILE *m_file;
extern void * cr_make_memryptr_array();  /* now in twsp3.c */

void twerror();
int tot_mcount;
int	mesgdefsize = MAXPKTL;  /* default mesg packet length. see pktlen */
				/* defined in tws.h */
int type_malloc_flag = 0;


static char twsp2id[] = "%W%\t%G%";


/************************************************************************
*************************************************************************
*
*               Time Warp Simulator - (Time Wrinkle?)
*               Perpetrator - S. Hughes
*		Reconstituted by J. J. Wedel
*
*************************************************************************
*
*			Time Warp Entry Point
*
*	newObject - create object  (newObj)
*
*	called by - application routines
*
*	- check length of object name and type
*	- if object is to be created at the current virtual time
*	-    perform routine to create the object
*	- else
*	-    set up an object create message in message text field
*	-    perform routine to create an event message
*	-    perform routine to queue the event message
*
*************************************************************************/

FUNCTION  msgRef newObj(object, time, type)
Name_object     *object;
VTime            time;
Type_object     *type;
{
    create_mesg obcre_mesg;
    char strng[STRSIZ];
    msgRef  reslt;

    if (debug_switch)
      {
	fprintf(stderr, "twsim>          name:%s\n", object);
	fprintf(stderr, "twsim>          type:%s\n", type);
	sprintVTime(strng,time);
	fprintf(stderr, "twsim>          time:%s\n", strng);
	sim_debug("newObj");
      }

    if (strlen(object) < NOBJLEN && strlen(type) < TOBJLEN)
      {
	obj_bod[gl_bod_ind].obj_new++;
	if (eqVTime(emq_current_ptr->rlvt, time))
	    cr_create_object(object, time, type);
	else
	{
	    strcpy(obcre_mesg.rcvr, object);
	    strcpy(obcre_mesg.type, type);
	    obcre_mesg.rcvtim = time;
	    if (cm_create_event_message
		("tws_driver",
		 "+obcreate",
		 emq_current_ptr->rlvt,
		 time,
		 0L,
		 sizeof(obcre_mesg),
		 &obcre_mesg
		) == SUCCESS)
	    {
		cm_queue_event_message();
		reslt.gid.num = -1;
		return(reslt);  /* JJW 9-91 */
	    }
	    else   error("create - failed to create message");
	}
      }

    else
	error("create - names too long");
    reslt.gid.num = 0;
    return(reslt); /* JJW 9-91 */
}

/************************************************************************
*
*	cr_create_object
*
*	 - object creation performed here
*
*	called by - config_obj, create, pr_create_object 
*
*	- handle debug
*	- perform routine to update the process table (created objects)
*	- perform the applications initialization routine
*	- (we get the pointer to the application's initialization
*	-  routine from the process table. we call the application
*	-  routine sending it the pointer to the memory allocation
*	-  routine, so that the application gets the honor of
*	-  creating its own state. we of course grab the state pointer)
*
*  This is supposed to be optimized for large numbers of creates. Because of
*  the global index variables (yuuck) The indexes must be saved and restored
*  because the object creation subroutines will change them so they would
*  point to the new object after exit. Also the creation may put an entry
*  into the binary sorted header table so they can't be directly restored. First
*  we restore them then check to see if the saved object name is same as name
*  they now point to.  (the hdr table was modified below the current index) If
*  names are same all is OK. Only if names differ we search the table again
*  and if we find the saved name we  use returned data to
*  restore the header indexes.  If we don't find it either we have a system
*  error or this is the initial create which is preset to name NULL by the
*  initialization.  If the save name is NULL indexes are OK, other wise end
*  up in sim_debug after some messages.
*************************************************************************/

FUNCTION cr_create_object(rcvr, rcvtim, type)
Name_object     *rcvr;
VTime           rcvtim;
Type_object     *type;
{
    int  savhdr, savbod, savghdr, savgbod, savgtyp, reslt;
    char strng[STRSIZ];
    Name_object savename;
    obcre_cnt++;
    if (debug_switch)
    {
	fprintf(stderr, "twsim>          name:%s\n", rcvr);
	fprintf(stderr, "twsim>          type:%s\n", type);
	sprintVTime(strng,rcvtim);
	fprintf(stderr, "twsim>          time:%s\n", strng);
	sim_debug("obcreate");
    }
    strcpy(savename, obj_hdr[hdr_ind].name);
    savhdr = hdr_ind;
    savbod = bod_ind;
    savghdr = gl_hdr_ind;
    savgbod = gl_bod_ind;
    if ((cr_sch_type_table(type) == SUCCESS)
	&&
        (cr_upd_obj_table(rcvr, rcvtim) == SUCCESS))
    {
        if (cr_makestate(process[type_ind].statesize) == 0)
	   reslt = FAILURE;
	else 
	{
	savgtyp = gl_type_ind;
	gl_type_ind = type_ind;	/* JJWXXX set gl ind for init routines (twusrlib)*/
	(*process[type_ind].init) (obj_bod[bod_ind].current);
	obj_bod[bod_ind].obj_time = 0.0;
	obj_bod[bod_ind].cum_obj_time = 0.0;
	reslt =  SUCCESS;
	}
    }
    else 
    {
    error("failed to create object");
    reslt = FAILURE;
    }
/* restore hdr indexes which were changed to refer to newly created object */
    hdr_ind = savhdr;  /* pretend they are right */
    gl_hdr_ind = savghdr;
    if ( strcmp(savename, obj_hdr[savhdr].name) != 0)
	{   /* hdr table changed */
	if (cr_sch_obj_table(savename) == SUCCESS)  /*sets hdr_ind */
	    gl_hdr_ind = hdr_ind;
	else    /* either there is an error or this is very first create */
	   if( strcmp(savename,"NULL") == 0 )
		 hdr_ind = savhdr;  /*sch doesn't change fg_hdr_ind */
		else 
		{
		fprintf(stderr,"system failure, create object\n");
		sim_debug("can't find current entry");
		}
	}
	bod_ind = savbod;
	gl_bod_ind  = savgbod;
	gl_type_ind = savgtyp;
	return(reslt);
}


/************************************************************************
*
*	cr_sch_type_table - do a linear search of the type table
*			       trying to find the objects name
*
*	called by -  cr_makestate
*
*************************************************************************/

FUNCTION cr_sch_type_table(type)
Type_object     *type;
{
    for (type_ind = 0; type_ind < num_types; type_ind++)
	if (match(type, process[type_ind].type))
	    return (SUCCESS);
    error("object type not found");
    return (FAILURE);
}

/************************************************************************
*
*	cr_inittype(objecttype,filestrng) - run the users type initialization function
*	called by init_type from the typeinit configuration command
*	- see if the type exists
*	-see if the function exists
*	-run it if it does, else there is an error.
*	-return a pointer to the data it found.
************************************************************************/
#define UNUSED_S 0
/* other use and main definition for UNUSED_S is in twsp3.c */
FUNCTION  cr_inittype(objecttype, filestrng)
Type_object  *objecttype;
char * filestrng;
{
   int svind, idx2;

   svind = gl_bod_ind;
   if (cr_sch_type_table(objecttype) == SUCCESS)
      {
      if (process[type_ind].initType == 0)
	 { fprintf(stderr,"type id %s\n",objecttype);
	   error("no init function for this type");
	   return (FAILURE);
	 }
      else
	 {
/* can this ever happen or will it crash earlier if null?  */
	if (filestrng == 0)
	   {
	   error("null string for typeinit");
	   return (FAILURE);
	   }
	type_malloc_flag = 1;
	gl_bod_ind = MAX_NUM_OBJECTS;  /* fake obj_bod at end of array. */
	process[type_ind].initptr = (*process[type_ind].initType) (filestrng);
	type_malloc_flag = 0;
	for (idx2 = 0; idx2 < MAX_TW_STREAMS; idx2++)
	   if (obj_bod[gl_bod_ind].stream[idx2].open_flag != UNUSED_S)
	     {
	     fprintf(stderr,"Init function left files open\n");
	     obj_bod[gl_bod_ind].stream[idx2].open_flag = UNUSED_S;
	     }
	gl_bod_ind = svind;
	 }
      }
}

/************************************************************************
*
*  type_malloc (size)
*  calls malloc if you are in a typeinit section of application code
*  otherwise an error
*
************************************************************************/
FUNCTION  void *type_malloc(size)
int size;
{
 if (type_malloc_flag == 0)  error("typeMalloc outside of typeinit section");
 return( (void*) sim_malloc((unsigned)size));
 
}    

/************************************************************************
*
*  type_myArea()
*  returns pointer to the results of the typinit which are in
*  process[type_ind].initptr
*
************************************************************************/
FUNCTION void* type_myArea()
{
 return (process[obj_bod[gl_bod_ind].proc_ptr].initptr);
}

/********************************************
*
* void* type_Area(type)
*   get a pointer to the typeinit data for some specified type
*
*********************************************/

FUNCTION  void * type_Area(objecttype)

	Type_object	*objecttype;
{
	void	*typepointer;
	int	sv_type_ind,type_ind2;

	sv_type_ind = type_ind;
	if (cr_sch_type_table(objecttype) == SUCCESS)
           {
	   typepointer = process[type_ind].initptr;
           if (typepointer == 0)
	      { fprintf(stderr,"type id %s\n",objecttype);
	      error("typearea not initialized");
	      type_ind = sv_type_ind;
	      return ( (void*)FAILURE);
	      }
	   else
	      {
	       type_ind = sv_type_ind;
	       return(typepointer);
	      }
	   }
	else
           {
	     twerror("typeArea: type not found: %s",objecttype);
	     type_ind = sv_type_ind;
	     return( (void*)FAILURE);
           }


      }


/************************************************************************
*
*	cr_upd_obj_table - make the created object active
*
*	called by - cr_create_object
*
*	- perform routine to find object name in object table
*	- if object names does not exist
*	-     perform routine to insert the object's name in the 
*	-        object header table (sorted for binary search,
*	-        contains only name and pointers)
*	-     add and initialize the object's entry in the object
*	-        body table
*	- else
*	-    if object has been deleted
*	-       make object active and init entry
*
*************************************************************************/

FUNCTION cr_upd_obj_table(rcvr, rcvtim)
Name_object     *rcvr;
VTime           rcvtim;
{
    if (cr_sch_obj_table(rcvr) == FAILURE)
	if (cr_insert_object_name(rcvr) == SUCCESS)
	{
	    obj_hdr[hdr_ind].obj_bod_ptr = num_objects;
	    bod_ind = num_objects;
	    num_objects++;
	    obj_bod[bod_ind].proc_ptr = type_ind;
	    obj_bod[bod_ind].pnode = 0;			/* not used */
	    obj_bod[bod_ind].status = TW_ACTIVE;
	    obj_bod[bod_ind].create_lvt = rcvtim;
	    obj_bod[bod_ind].destroy_lvt = newVTime(0.0, 0L, 0L);
	    obj_bod[bod_ind].current_lvt = rcvtim;
	    obj_bod[bod_ind].current = NULL;
            obj_bod[bod_ind].lastime.simtime = NEGINF; 
            obj_bod[bod_ind].num_events = 0;
            obj_bod[bod_ind].num_evtmsgs = 0;
	    obj_bod[bod_ind].cr_count = 1;
	    obj_bod[bod_ind].dst_count = 0;
	    obj_bod[bod_ind].obj_new = 0;
	    obj_bod[bod_ind].obj_del = 0;
	    obj_bod[bod_ind].memry_limit = 0;
	    obj_bod[bod_ind].obj_cemsgs = 0;
	    obj_bod[bod_ind].memry_pointers_ptr = 0; /* see make_memryptr */
	    gl_hdr_ind = hdr_ind;	/* for me() */
	    gl_bod_ind = bod_ind;	/* for tw_fopen */
	    return (SUCCESS);
	}
	else
	    return (FAILURE);
    else	/* resurrection of destroyed object */
    {
	bod_ind = obj_hdr[hdr_ind].obj_bod_ptr;
	if (obj_bod[bod_ind].status == TW_DELETED)
	{
	    obj_bod[bod_ind].status = TW_ACTIVE;
	    obj_bod[bod_ind].create_lvt = rcvtim;
	    obj_bod[bod_ind].destroy_lvt = newVTime(0.0, 0L, 0L);
	    obj_bod[bod_ind].current_lvt = rcvtim;
	    obj_bod[bod_ind].cr_count++;
	    obj_bod[bod_ind].memry_limit = 0;
	    return (SUCCESS);
	}
	else   /* its already there, but now this is OK in tw2.5 */
	    {
	    fprintf(stderr,"object already active: %s\n", rcvr);
	    return (SUCCESS);
	    }
    }
/*    return (FAILURE);   not reached */
}

/************************************************************************
*
*	cr_sch_obj_table - do a binary search on the object header
*				 table for the object's name
*
*	called by - cr_upd_obj_table, ds_destroy_object,
*		    pr_find_next_process,
*		    dis_objects
*
*	sets hdr_ind to the right entry if it exists.
*************************************************************************/
/* Name_object     *object_name;   */
FUNCTION cr_sch_obj_table(object_name)
Name_object     object_name;
{
    int             beg_ptr, end_ptr, result;

    beg_ptr = 0;
    end_ptr = num_objects - 1;
    while (beg_ptr <= end_ptr)
    {
	hdr_ind = (beg_ptr + end_ptr) >> 1;
	result = strcmp(object_name, obj_hdr[hdr_ind].name);
	if (result > 0)
	    beg_ptr = hdr_ind + 1;
	else
	if (result < 0)
	    end_ptr = hdr_ind - 1;
	else
	    return (SUCCESS);
    }
    return (FAILURE);
}

/************************************************************************
*
*	cr_insert_object_name - insert the objects name in the object
*				header table
*
*	called by - cr_upd_obj_table
*
*	- search for proper position for insertion of name
*	- shift lower half of table down
*
*************************************************************************/

FUNCTION cr_insert_object_name(rcvr)
Name_object     *rcvr;
{
    int             i, result;

    /* Search to see if rcvr is already in table. */
    for
	(
	 hdr_ind = 0;
	 ((result = strcmp(rcvr, obj_hdr[hdr_ind].name)) > 0) &&
	 hdr_ind < num_objects;
	 hdr_ind++
	);

    /* If rcvr not already in table, put it there. */
    if (result != 0)
    {
	if (num_objects + 1 < MAX_NUM_OBJECTS)
	{
	    for (i = num_objects; i > hdr_ind; i--)
	    {
		strcpy(obj_hdr[i].name, obj_hdr[i - 1].name);
		obj_hdr[i].obj_bod_ptr =
		    obj_hdr[i - 1].obj_bod_ptr;
	    }
	    strncpy(obj_hdr[hdr_ind].name, rcvr,NOBJLEN-1);
	    obj_hdr[hdr_ind].name[NOBJLEN-1] = 0;
	    return (SUCCESS);
	}
	else
	{
	    error("object table overflow ");
	}
    }
    else
    {
	error("duplicate object name insert");
    }

    return (FAILURE);
}

/************************************************************************
*
*	mi_clear - clear memory blocks
*
*	called by - cr_makestate
*	use assy lang routine 3/17/88
*
*************************************************************************/

void FUNCTION mi_clear(ptr, size)
char           *ptr;
int             size;
#if 0
{
    int             i;

    for (i = 0; i < size; i++)
	*ptr++ = '\0';
}
#else
{
    clear(ptr,size);
}
#endif

/************************************************************************
*
*	cr_makestate - memory allocation routine for states
*
*	called by - cr_create object to make the state when an
*	   object is created
*
*	- if the objects current  state pointer is NULL
*	-    (object is being newly created)
*	-    perform sim_malloc routine for proper number of bytes
*	-    set pointers and sizes
*	- else (object has been previously created and destroyed
*	-    clear the states
*	return 0 if there is no memory, else ptr cast to int
*
*************************************************************************/

FUNCTION cr_makestate(size)
int             size;
{
    char           *ptr;
    char           *cptr;

    cptr = obj_bod[bod_ind].current;
    if (cptr == NULL )
	if (size > 0 )
	    if (ptr = sim_malloc((unsigned)size))
	    {
	    num_state_bytes += size;
	    obj_bod[bod_ind].current = ptr;
	    mi_clear(ptr, size);
	    obj_bod[bod_ind].ssize = size;
	    return  (int)(ptr);
	    }
	    else
		error("alloc current state mem - no memory");
	else
	    error("twsp2: Object statesize is zero.");
    else
    {
	mi_clear(cptr, size);
	return (int)(cptr);
    }
    return ( 0 );
}

/************************************************************************
*
*			Time Warp Entry Point
*
*	delObject - destroy objects  (delObj)
*
*	called by - application routines
*
*	- check length of object name and type
*	- if object is to be destroyed at the current virtual time
*	-    perform routine to destroy the object
*	- else
*	-    set up an object create (uses same format) message text field
*	       for an event message
*	-    perform routine to create an event message
*	-    perform routine to queue the event message
*
*************************************************************************/

FUNCTION msgRef delObj(object,time)
Name_object     *object;
VTime            time;
{
    create_mesg obcre_mesg;
    msgRef  reslt;

    if (debug_switch)
      {
	sim_debug("delObj");
      }
    if (strlen(object) < NOBJLEN)
      {
	obj_bod[gl_bod_ind].obj_del++;
	if (eqVTime(emq_current_ptr->rlvt, time))
	    ds_destroy_object(object,time);
	else
	{
	    strcpy(obcre_mesg.rcvr, object);
	    strcpy(obcre_mesg.type, "");
	    obcre_mesg.rcvtim = time;
	    if (cm_create_event_message
		("tws_driver",
		 "}obdestroy",
		 emq_current_ptr->rlvt,
		 time,
		 0L,
		 sizeof(obcre_mesg),
		 &obcre_mesg) == SUCCESS)
	    {
		cm_queue_event_message();
 	        reslt.gid.num = -1;
		return(reslt); /* JJW 9-91 */
	    }
	    else   error("destroy - failed to create message");	    
	}
      }
    else
	error("destroy - name too long");
    reslt.gid.num = 0;
    return(reslt); /* JJW 9-91 */
}

/************************************************************************
*
*	ds_destroy_object - make an object inactive
*
*	called by - destroy, find_next_event
*
*	- handle debug
*	- perform routine to find object's name in the object table
*	- set time of destroy
*	- set status
*
*************************************************************************/

FUNCTION ds_destroy_object(rcvr, rcvtim)
Name_object     *rcvr;
VTime           rcvtim;
{
    int  savhdr, savbod, savghdr, savgbod, reslt;
    char strng[STRSIZ];

    obdes_cnt++;
    if (debug_switch)
    {
	fprintf(stderr, "twsim>      name:%s\n", rcvr);
	sprintVTime(strng,rcvtim);
	fprintf(stderr, "twsim>      time:%s\n", strng);
	sim_debug("obdestroy");
    }
    savhdr = hdr_ind;
    savbod = bod_ind;
    savghdr = gl_hdr_ind;
    savgbod = gl_bod_ind;
    if (cr_sch_obj_table(rcvr) == SUCCESS)
    {
	bod_ind = obj_hdr[hdr_ind].obj_bod_ptr;
	if (obj_bod[bod_ind].status == TW_ACTIVE)
	{
	    obj_bod[bod_ind].destroy_lvt = rcvtim;
	    obj_bod[bod_ind].status = TW_DELETED;
	    obj_bod[bod_ind].dst_count++;
	    reslt = SUCCESS;
	}
	else
	    {
	    if (debug_switch)
		{
		fprintf(stderr, "twsim> object destroyed at");
		sprintVTime(strng,obj_bod[bod_ind].destroy_lvt);
		fprintf(stderr, " %s\n", strng);
		}
	    error("destroy: object already destroyed");
	    }
    }
    else
    {
	error("destroy: object never existed");
        reslt = FAILURE;
    }
hdr_ind = savhdr;  /* restore indexes which were changed to refer to new object */
bod_ind = savbod;
gl_hdr_ind = savghdr;
gl_bod_ind  = savgbod;
return(reslt);
}

/************************************************************************
*
*			Time Warp Entry Point
*
*	schedule - queue event message
*
*	called by - application routines
*
*	- handle debug
*	- perform routine to format and init the event message
*	- perform routine to link the message into the queue
*
*************************************************************************/

FUNCTION msgRef schedule( rcvr, rcvtime, msg_selector, len, textptr )
Name_object     rcvr;
VTime           rcvtime;
Long		msg_selector;
int	        len;
Message        *textptr;
{
    char strng[STRSIZ];
    msgRef   reslt;

    if (debug_switch)
    {
	fprintf(stderr, "twsim>      receiver:%s\n", rcvr);
	sprintVTime(strng,rcvtime);
	fprintf(stderr, "twsim>  receive time:%s\n", strng);
	fprintf(stderr, "twsim>      selector:%ld\n", msg_selector);
	fprintf(stderr, "twsim>   text length:%d\n", len);
	dprep_text(len, textptr);
	fprintf(stderr, "twsim>          text:%s\n", dtext);
	message_display(msg_selector,textptr,gl_type_ind);
	sim_debug("schedule");
    }
    reslt.gid.num = 0;
    if (rcvtime.simtime > cutoff_time) return(reslt);     /* JJW 9-91 */
    evtmsg_cnt++;

/* the next timer block (all inside ifndef HISP) was here to
	include message creation time in the queue time */


    if
    (
	cm_create_event_message
	(
	    emq_current_ptr->rname,
	    rcvr,
	    emq_current_ptr->rlvt,
	    rcvtime,
	    msg_selector,
	    len,
	    textptr
	) == SUCCESS
    )
    {
#ifndef HISP
   if (notime_sw == FALSE)
     {
     evt_parttime = itimer() * multiple;
     evt_realtime += evt_parttime;	/* timestamp for the message */
				/* used in cm_create_event_message */
     tot_evt_time += evt_parttime;  
    }
#endif
/* fix up the timestamp in the message since we have already created it */
	mb_ptr->msg_time = evt_realtime;
	cm_queue_event_message();
	obj_bod[gl_bod_ind].num_evtmsgs++; /* update count of schedules */
#ifndef HISP
	if (notime_sw == FALSE)  queue_time += itimer();
#endif
    }   /* end of success */
    else
    { 
	fprintf(stderr, "schedule: failed to create message.\n");
	erprnt();
    }

#ifndef HISP
    if(archive_switch)
    {
	HOST_fprintf
	(
	    arc_file,
	    "entry ( schedule %s %ld );\n",
	    rcvr,
	    rcvtime.simtime
	);
    }
#endif
reslt.gid.num = -1;
return(reslt);     /* JJW 9-91  */

}

/************************************************************************
*
*			Time Warp Entry Point
*
*	unschedule - cancel event message
*	cancel - cancel event message by reference
*
*	called by - application routines
*
*      not yet implemented
*
*************************************************************************/

FUNCTION  unschedule( rcvr, rcvtime, msg_selector, len, textptr )
Name_object     rcvr;
VTime           rcvtime;
Long		msg_selector;
int	        len;
Message        *textptr;
{
  char	strng[STRSIZ];
  if (debug_switch)
    {
	fprintf(stderr, "twsim>      name:%s\n", rcvr);
	sprintVTime(strng,rcvtime);
	fprintf(stderr, "twsim>      time:%s\n", strng);
	sim_debug("unschedule");
    }
return(0);
}

FUNCTION cancel( ref )
msgRef ref;
{
return(0);
}

/*************************************************************************
*
*	speculative computing interface. null routines in
*	simulator
*
*	guess()
*	unguess()
*
*************************************************************************/

FUNCTION  guess(sndtime, rcvtime, rcvr, selector, txtlen, text)
VTime           sndtime;
VTime           rcvtime;
Name_object     rcvr;
Long		selector;
int	        txtlen;
Message        *text;
{
  char	strng[STRSIZ];
  if (debug_switch)
    {
	fprintf(stderr, "twsim>      name:%s\n", rcvr);
	sprintVTime(strng,rcvtime);
	fprintf(stderr, "twsim>      time:%s\n", strng);
	sim_debug("guess");
    }
  return(0);
}


FUNCTION  unguess(sndtime, rcvtime, rcvr, selector, txtlen, text)
VTime           sndtime;
VTime           rcvtime;
Name_object     rcvr;
Long		selector;
int	        txtlen;
Message        *text;
{
  char	strng[STRSIZ];
  if (debug_switch)
    {
	fprintf(stderr, "twsim>      name:%s\n", rcvr);
	sprintVTime(strng,rcvtime);
	fprintf(stderr, "twsim>      time:%s\n", strng);
	sim_debug("unguess");
    }
return(0);
}



/************************************************************************
*
*	cm_create_event_message - format and init the event message
*
*	called by - obcreate, obdestroy, schedule, newconfO.cs_send_mesg
*
*	- do a simple check of parameters
*	- perform routine that creates a linked list of message blocks,
*	-    each one capable of handling up to mesgdefsize number of
*	-    characters of the message text. (processed message blocks
*	-    are reused before new ones are created - blocks are never
*	-    returned to VMS)
*	- initialize the message fields (to, from, times, status)
*	- copy in the message text
*	- handle elapsed times
*	- output to the file
*
*************************************************************************/

FUNCTION cm_create_event_message(snd_name, rcv_name,
			snd_time, rcv_time, msg_selector,
			mesg_len, mesg_text)
Name_object     snd_name, rcv_name;
VTime           snd_time, rcv_time;
Long		msg_selector;
int             mesg_len;
Message        *mesg_text;
{
    int         t1, t2;
    char        *ptr1, *ptr2, *ptrn;
    char	strng[STRSIZ];

/* no longer checks for names too long; does a strncpy instead */
    if (!(
	  mesg_len >= 0
	  && mesg_len <=  mesgdefsize
	  && geVTime(rcv_time, snd_time))
	||
  ( eqVTime(rcv_time, snd_time) && (strcmp(rcv_name, "stdout"))
        && !now_msg_flag ))
    {
	fprintf(stderr, "\nsender: %s    receiver: %s  \n",snd_name,rcv_name);

	sprintVTime(strng,snd_time);
	fprintf(stderr, "twsim>  msg create: send time:%s\n", strng);
	sprintVTime(strng,rcv_time);
	fprintf(stderr, "twsim>   rcv time:%s\n", strng);
	fprintf(stderr, "twsim>     length: %d\n", mesg_len);
	fprintf(stderr, "invalid message parameters (bad length, now, or back in time\n)");
	return (FAILURE);
    }
    if (cm_get_mesg_block(mesg_len) == SUCCESS)
    {
	mb_ptr->slvt = snd_time;
	mb_ptr->rlvt = rcv_time;
	mb_ptr->status = TW_ACTIVE;
	mb_ptr->mtlen = mesg_len;
	mb_ptr->what_evt = tot_evtcount;
	mb_ptr->select = msg_selector;
/*	mb_ptr->msg_time = evt_realtime;  done just before queueing now */
	strncpy(mb_ptr->sname, snd_name, NOBJLEN-1);
	mb_ptr->sname[NOBJLEN-1] = 0;
	strncpy(mb_ptr->rname, rcv_name, NOBJLEN-1);
	mb_ptr->rname[NOBJLEN-1] = 0;
#ifndef HISP
	if (mfile_sw == TRUE)
	{
	HOST_fprintf(m_file,"%s\t%.3f [%d,%d]\t%s\t%.3f [%d,%d]\t%ld\n",
		  snd_name,snd_time.simtime, snd_time.sequence1,
                  snd_time.sequence2, rcv_name, rcv_time.simtime,
                  rcv_time.sequence1, rcv_time.sequence2,
			msg_selector);
	tot_mcount++;
	}
#endif
	ptr2 = mesg_text;
	ptr1 = mb_ptr->text;
        entcpy(ptr1, ptr2, mesg_len);
	obj_bod[gl_bod_ind].obj_msgsent++;	/* counts scheduless, newobj, delobj */
	return (SUCCESS);
    }
    return (FAILURE);
}

/************************************************************************
*
*	cm_get_mesg_block - get the block required for the message
*
*	called by - cm_create_event_message
*
*	- perform routine to get the required block
*
*************************************************************************/

FUNCTION cm_get_mesg_block(mesg_len)
int             mesg_len;
{
    num_mesg_bytes += mesg_len;
    req_blk_cnt++;

    if (emq_first_ptr != NULL
	&& emq_first_ptr->status == TW_DELETED)
    /* no error msg when TW_DELETED is false */
    {
	mb_ptr = emq_first_ptr;
	emq_first_ptr = emq_first_ptr->next;
	if (emq_first_ptr) emq_first_ptr->prior = NULL;
	else emq_endmsg_ptr = NULL;
    }
    else
    {
	mb_ptr = (mesg_block *) sim_malloc((unsigned)mb_size);
	if (mb_ptr != NULL)
	{
	    all_blk_cnt++;
	    mb_ptr->text = (Message * ) sim_malloc((unsigned)mesgdefsize);
	    if (mb_ptr->text == NULL) {
	        error("mem alloc - message block text");
		return( FAILURE );
	    }		
	}
	else {
	    error("mem alloc - message block");
	    return( FAILURE );
	    }
    }
	return (SUCCESS);
}

/************************************************************************
*
*			Time Warp Entry Point
*       numMsgs
*	mcount - return the number of messages queued for this object
*	         at the current virtual time
*
*	called by - application routines
*
*	-    number of messages in message pointer array
*	-    (set up prior)
*
*************************************************************************/

FUNCTION int obj_numMsgs()
{
    mcount_cnt++;
    if (debug_switch)
      {
	fprintf(stderr,"twsim>  numMsgs: %d\n",mesg_lmt);
        sim_debug("numMsgs");
      }
    return( mesg_lmt);
}

/************************************************************************
*
*			Time Warp Entry Point
*
*  msgLength()   return length of current message with index n
*
*************************************************************************/
FUNCTION msgLength(n)
int n;
{
    mesg_block  *mptr;
    char strng[STRSIZ];
    if (debug_switch)
    {
	sprintVTime(strng,emq_current_ptr->rlvt);
	fprintf(stderr, "twsim>  current time:%s\n", strng);
	fprintf(stderr, "twsim> message index:%d\n", n);
    }
    if (n >= 0 && n < mesg_lmt)
    {
	mptr = mesg_ptr[n];
   if (debug_switch)
     {
	fprintf(stderr, "twsim>        length:%d\n", mptr->mtlen);
	sim_debug("msgLength");
     }
	return(mptr->mtlen);
    }
    else
    {
	fprintf(stderr, "twsim> msgLength: index out of range:%d\n", n);
	if (debug_switch)
            sim_debug("msgLength");
	return(0);
    }

}

/************************************************************************
*
*			Time Warp Entry Point
*
*	msgText - get a ptr to the message text of one of the current messages
*		- specifying a message index (0 origin)
*
*	called by - application routine
*
*	- handle debug
*	- else (event message mode)
*	-    if user has specified index for existing message
*	-        return message text pointer
*	-    else
*	-       give error message and return null pointer.
*
*************************************************************************/

FUNCTION void  *msgText(n)
int            n;
{
    Message     *textptr;
    mesg_block  *mptr;
    int		len;
    char strng[STRSIZ];

    getmsg_cnt++;
    if (debug_switch)
    {
	sprintVTime(strng,emq_current_ptr->rlvt);
	fprintf(stderr, "twsim>  current time:%s\n", strng);
	fprintf(stderr, "twsim> message index:%d\n", n);
    }
    if (n >= 0 && n < mesg_lmt)
    {
	mptr = mesg_ptr[n];
	len = mptr->mtlen;
	if (len <= mesgdefsize)
	    textptr = mptr->text;
	else
	  {
	  fprintf(stderr, "twsim> msgText: dryrot, msg too long\n");
	  }
	if (textptr == 0)
	   error( "----> msgText: null text pointer\n");

        if (debug_switch)
	   {
	   fprintf(stderr, "twsim>        sender:%s\n", mptr->sname);
	   sprintVTime(strng,mptr->slvt);
	   fprintf(stderr, "twsim>      sendtime:%s\n", strng);
	   fprintf(stderr, "twsim>        length:%d\n", len);
	   message_display(mptr->select,textptr,gl_type_ind);
	   }
    }
    else
    {
	fprintf(stderr, "twsim> msgText: index out of range:%d\n", n);
	textptr = NULL;
	error("no text pointer\n");
    }
    if (debug_switch)
      {
	sim_debug("msgText");
      }
    return((void *) textptr);
}

/************************************************************************
*
*			Time Warp Entry Point
*
*	msgSelector - get the selector (Long) value of one of the
*		    current messages
*
*	called by - application routine
*
*	- handle debug
*	- else (event message mode)
*	-    if user has specified index within range
*	-	return selector
*	-    else
*	-       give error MESSAGE and return value of -1
*
*************************************************************************/

FUNCTION Long msgSelector(n)
int n;
{
    Long	val;
    mesg_block     *mptr;
    char strng[STRSIZ];

    getsel_cnt++;
    if (debug_switch)
    {
	sprintVTime(strng,emq_current_ptr->rlvt);
	fprintf(stderr, "twsim>  current time:%s\n", strng);
	fprintf(stderr, "twsim> message index:%d\n", n);
    }
    if (n >= 0 && n < mesg_lmt)
    {
	mptr = mesg_ptr[n];
	val = mptr->select;
        if (debug_switch)
	  {
 	  fprintf(stderr, "twsim>        sender:%s\n", mptr->sname);
	  sprintVTime(strng,mptr->slvt);
	  fprintf(stderr, "twsim>      sendtime:%s\n", strng);
	  fprintf(stderr, "twsim>      selector:%ld\n", val);
	  }
    }
    else
    {
 	  fprintf(stderr, "twsim>  msgSelector: index out of range: %d\n",n);
	  val = -1L;
    }
    if (debug_switch)
      {
	sim_debug("msgSelector");
      }
    return( val);
}


/************************************************************************
*
*			Time Warp Entry Point
*
*	sender - return the name of object sending message
*
*	called by - application routine
*
*	- handle debug
*	- return the name of the object sending the specified message
*	- handle debug
*
*************************************************************************/

FUNCTION char *msgSender(msgnum)
int             msgnum;	
{
    char  *ptr;

    sender_cnt++;
    if (debug_switch)
    {
	fprintf(stderr, "twsim> message index:%d\n", msgnum);
	sim_debug("msgSender");
    }
    if (msgnum >= 0 && msgnum < mesg_lmt)
    {
	ptr =  mesg_ptr[msgnum]->sname;
    }
    else
	{
	fprintf(stderr,"msgSender: index out of range: %d\n",msgnum);
	return(0);
	}
    if (debug_switch)
    {
	fprintf(stderr, "twsim>     msgSender:%s\n", ptr);
    }
    return(ptr);
}

/************************************************************************
*
*			Time Warp Entry Point
*
*	sendtime - return the send virtual time of a message
*
*	called by - application routines
*
*	- handle debug
*	- return the send time of the specified message
*	- handle debug
*
*************************************************************************/

FUNCTION VTime msgSendTime(msgnum)
int             msgnum;
{
    VTime   val;
    char strng[STRSIZ];

    sndtm_cnt++;
    if (debug_switch)
    {
	fprintf(stderr, "twsim> message index:%d\n", msgnum);
    }
    if (msgnum >= 0 && msgnum < mesg_lmt)
    {
	val = mesg_ptr[msgnum]->slvt;
    }
    else
    {
	  fprintf(stderr, "twsim> msgSendTime: index out of range:%d\n",
		 msgnum);
	  val = newVTime(-1.0,0L,0L);
   }
    if (debug_switch)
    {
 	sprintVTime(strng,val);
	fprintf(stderr, "twsim>     send time:%s\n", strng);
	sim_debug("msgSendTime");
    }
   return(val);
}

/************************************************************************
*
*			Time Warp Entry Point
*
*	obj_myName - return the name of the current process (executing object)
*
*	called by - application routine as macro myName
*
*************************************************************************/

FUNCTION char *obj_myName(me)
Name_object me;
{
    me_cnt++;
    if (debug_switch > 1)
    {
	fprintf(stderr, "twsim>     name:%s\n", obj_hdr[gl_hdr_ind].name);
	sim_debug("myName");
    }
    strcpy(me, obj_hdr[gl_hdr_ind].name);
    return((char *)me);
}

/************************************************************************
*
*			Time Warp Entry Point
*
*	obj_myState - return pointer to  state of the executing object
*	- return pointer to appropriate state vector.
*
*	called by - application routine as macro myState (or macro MY(x) )
*
*************************************************************************/

FUNCTION void *obj_myState()
{

    if (debug_switch > 1)
    {
	fprintf(stderr, "twsim>   myState:  name:%s\n",
		  obj_hdr[gl_hdr_ind].name);
	sim_debug("myState");
    }
    return((void *)obj_bod[gl_bod_ind].current);
}


/************************************************************************
*
*  twerror function
*
*************************************************************************/

FUNCTION void twerror(string)
char *string;
{
	VTime xnow;

	xnow = obj_now();

	schedule("stdout", xnow, -1L, strlen(string), string);
        sim_debug(" \ntwerror: now in sim_debug ( q to quit )");

}


/************************************************************************
*
*			Time Warp Entry Point
*
*	obj_now - return the current virtual time
*
*	called by - application routine with MACRO "now"
*
*	- return the receive time of the current event message
*
*************************************************************************/

FUNCTION  VTime obj_now()
{

    char strng[STRSIZ];

    simtm_cnt++;
    if (debug_switch > 1)
    {
	sprintVTime(strng,emq_current_ptr->rlvt);
	fprintf(stderr, "twsim>  current time:%s\n", strng);
	sim_debug("now");
    }
    return( emq_current_ptr->rlvt);
}

/************************************************************************
*
*			Time Warp Entry Point
*
*	called by - application routine
*
*************************************************************************/

FUNCTION VTime IncSimTime ( incr )

    double incr;
{
    char strng[STRSIZ];
    VTime next;

    next = emq_current_ptr->rlvt;

    next.simtime += incr;

    if (debug_switch)
    {
	sprintVTime(strng,next);
	fprintf(stderr, "twsim> new SimTime:%s\n", strng);
	sim_debug("IncSimTime");
    }

    return ( next );
}

/************************************************************************
*
*			Time Warp Entry Point
*
*	called by - application routine
*
*************************************************************************/

FUNCTION VTime IncSequence1 ( incr )

    Ulong incr;
{
    VTime next;

    next = emq_current_ptr->rlvt;

    next.sequence1 += incr;

    return ( next );
}

/************************************************************************
*
*			Time Warp Entry Point
*
*	called by - application routine
*
*************************************************************************/

FUNCTION VTime IncSequence2 ( incr )

    Ulong incr;
{
    VTime next;

    next = emq_current_ptr->rlvt;

    next.sequence2 += incr;

    return ( next );
}

/************************************************************************
*
*			Time Warp Entry Point
*
*	userError
*
*	called by user when user wants an error stop
*
*************************************************************************/

FUNCTION userError(string)
char *string;

{
     char  strng[STRSIZ];

     fprintf(stderr,"userError: %s\n",string);
     ttoc(strng, emq_current_ptr->rlvt.simtime);
     fprintf(stderr,"time: %s  %d,  %d\n",strng, emq_current_ptr->rlvt.sequence1,
		emq_current_ptr->rlvt.sequence2);  
     fprintf(stderr,"name: %s\n",obj_hdr[gl_hdr_ind].name); 
     sim_debug(" \nnow in sim_debug ( q to quit )");
     exit(1);
}
     

/************************************************************************
*
*			Time Warp Entry Point
*
*	objectAddressError
*
*	called by user when user wants to check a pointer
*
*	NOT IMPLEMENTED (except trivially) in simulator
*
*************************************************************************/

FUNCTION objectAddressError(addr, size)
Byte *addr;
int   size;

{
    if (addr == NULL) return(TRUE);
    if (debug_switch)
      {
      fprintf(stderr,"objectAddressError not implemented in simulator\n");
      sim_debug("objectAddressError");
    }
    return(FALSE);

}

/************************************************************************
*
* tw_packetLen. Return value of the packet length currently in effect.
* Normally this is the default defined at beginning of this file.
*
*************************************************************************/
FUNCTION int tw_packetLen()
{

     if (debug_switch)
       {
	 fprintf(stderr,"packet length: %d\n",mesgdefsize);
	 sim_debug("tw_packetLen");
       }
     return (mesgdefsize);
}

/************************************************************************
*
*			Time Warp Entry Point
*
*	tw_interrupt
*
*	called by user 
*
*	This allows TimeWarp to preeempt the users process and reschedule
*	Does nothing in the simulator.
*
*************************************************************************/

FUNCTION int tw_interrupt()
{
	return(0);
}




/************************************************************************
*
*        Time Warp User Library Functions
*
*************************************************************************/
FUNCTION void * obj_getLibTable()
{
   return process[gl_type_ind].libTable;
}
 
FUNCTION Pointer obj_getLibPointer()
{
   return (Pointer)(obj_bod[gl_bod_ind].twulib_memry);
}
 
FUNCTION void obj_setLibPointer(memBlock)
   Pointer memBlock;
{
   obj_bod[gl_bod_ind].twulib_memry = (int)memBlock;
}



/**********************************/



