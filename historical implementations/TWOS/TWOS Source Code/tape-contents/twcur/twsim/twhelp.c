/* "Copyright (C) 1989, California Institute of Technology. 
     U. S. Government Sponsorship under NASA Contract 
   NAS7-918 is acknowledged." */

/*********************************************************************
* 
* Sumulator help function and various functions which respond to the
* interactive prompts and display messages and objects
*
* generally called from sim_debug
*
*
* functions removed from twsp1 and added to twhelp 12/7/87 JJW
*
* Modified dm_one_message by FPW on 2/3/88.
* Added HISP 8/18/88 JJW
* Moved list implementation of the 2 fcts which are different to end
* of file  -   dm_queue() and dm_select
* Above two functions are no longer queue specific.  JJW 2/6/89
* add capability to send DM output to a file.   JJW 4/30/91
* add stuff to check existance for file for DM output JJW 5/2/91
*
*********************************************************************/

#ifdef BF_MACH          /* Surely this repetitive nonsense   */
#include <stdio.h>      /* can be fix by a properly worked   */
#endif                  /* Makefile?                         */
#ifdef BF_PLUS
#include <stdio.h>
#endif

#ifdef SUN
#include <stdio.h>
#endif SUN


#ifdef MARK3
#include "stdio.h"
#endif

static char  twerrorid[] = "%W%\t%G%";

#include "twcommon.h"
#include "machdep.h"
#include "tws.h"
#include "twsd.h"
#include "twctype.h"

#define DTEXT_SIZE 65

/* for the help function */
#define P fprintf(stderr,
#define PXE fprintf(stderr,
#define PXF fprintf(dmf_file,
/* all messages are fprintf'd to stderr with the above definition */

char dtext[DTEXT_SIZE+1];
int dmf_sw = 0;		/* DM output is going to a file */
FILE *dmf_file = 0;	/* the file it is going to - accessed omly in this file */
/* The usual forward function defs since most of them are void */
void gnxt_option();
void gnxt_reply();
void gnxt_token();
int match();
void perform_option();
void dis_messages();
int dm_determine_options();
int dm_get_object_name();
int dm_get_virtual_time();
void dm_queue();
void dm_select();
void do_del_msgs();
void dm_one_message();
void dm_header();
char *dprep_mesg();
char *dprep_text();
void dis_objects();
void do_one_object();
void do_header();
FILE *opendmfile();
void twhelp ();
extern void warning();
extern void set_bp();
extern void dis_stats();
extern mesg_block  *get_msg_pointer();

extern FILE *arc_file;	/* in twsp1.c */
extern FILE *t_file;
extern FILE  *m_file;
extern FILE  *im_file;
extern FILE  *el_file;
extern FILE *data_file;
extern int   d_file_sw;
extern int   imfile_sw;

extern int mesgdefsize; /* packet length */
extern int ISLOG_enabled;
extern int last_q;   /* from twqueueh OR twqueues */
extern char versionno[];
double atof();

/************************************************************************
*
*	gnxt_option - get the next user option
*
*	called by - debug
*
*	- perform routine to get a user response
*	- perform routine to return the next word delimited by a space
*	-    (argument=2 implies forced upper case)
*
*************************************************************************/

void FUNCTION gnxt_option()
{
    PXE "twsim>");
    gnxt_reply();
    gnxt_token(2);
}

/************************************************************************
*
*	gnxt_reply - get a response from the terminal (user)
*
*	called by - gnxt_option
*
*	- vanilla flavored terminal input routine
*
*************************************************************************/

void FUNCTION gnxt_reply()
{
    int             c,
                    i;

    for (i = 0;
	 i < REPLY_SIZE
	 && (c = getchar()) != EOF
	 && c != EOL;
	 ++i)
	mreply[i] = c;
    mreply[i] = NULLCHAR;
    mreplyptr = 0;
}

/************************************************************************
*
*	gnxt_token - return next string of characters delimited by
*			 a space, tab or null character
*
*	called by - most display routines
*
*	- scan for delimiting characters, saving the intermediate
*	- optionally force the result to upper case
*
*************************************************************************/

void FUNCTION gnxt_token(option)
int             option;
{
    int             i;

    while ((mreply[mreplyptr] == SPACE
	    || mreply[mreplyptr] == TAB)
	   && mreply[mreplyptr] != NULLCHAR)
	++mreplyptr;
    for (i = 0;
	 i < TOKEN_SIZE
	 && mreply[mreplyptr] != SPACE
	 && mreply[mreplyptr] != TAB
	 && mreply[mreplyptr] != NULLCHAR;
	 ++i)
    {
	token[i] = mreply[mreplyptr];
	++mreplyptr;
    }
    token[i] = NULLCHAR;
    if (option == 2)
	for (i = 0; token[i] != NULLCHAR; i++)
	    token[i] = toupper(token[i]);
}

/************************************************************************
*
*	match - compare character strings
*
*	called by - innumerable routines
*
*	- return 1 (true) if string one is equal to or is contained in
*	-    string 2 otherwise return 0 (false)
*
*************************************************************************/

FUNCTION match(string1, string2)
char           *string1,
               *string2;
{

    for (;
	 *string1 == *string2;
	 string1++, string2++)
	if (*string1 == NULLCHAR)
	    return (1);
    return (0);
}

/************************************************************************
*
*	perform_option - transfer routine
*
*	called by - debug
*
*************************************************************************/

void FUNCTION perform_option()
{
    if (match(token, "EXIT")
	|| match(token, "E")
	|| match(token, "QUIT")
	|| match(token, "Q"))
    {
	prog_status = WSTERM;

#if defined(BF_PLUS) || defined(BF_MACH)
        if ( ISLOG_enabled )
		IS_dumplog ();
#endif
#ifndef HISP
	if ( (int)arc_file) fclose(arc_file);
	if (tfile_sw) HOST_fclose(t_file);
	if (mfile_sw) HOST_fclose(m_file);
	if (imfile_sw) HOST_fclose(im_file);
	if (elog_sw) HOST_fclose(el_file);
	if (d_file_sw) HOST_fclose(data_file);
#endif
	dis_stats();
	PXE "*** twsim exit ***\n");
	exit(0);
    }
    else
    if (match(token, "DT"))
	debug_switch = TRUE;
    else
    if (match(token, "D2"))
	debug_switch = 2;
    else
    if (match(token, "DF"))
	debug_switch = FALSE;
    else
    if (match(token, "ST"))
	step_switch = 1;
    else
    if (match(token, "SF"))
	step_switch = 0;
    else
    if (match(token, "AT"))
	archive_switch = TRUE;
    else
    if (match(token, "AF"))
	archive_switch = FALSE;
    else
    if (match(token, "TT"))
	step_switch = 2;
    else
    if (match(token, "TF"))
	step_switch = 0;	/* go back to single step */
    else
    if (match(token, "DM"))
	dis_messages();
    else
    if (match(token, "DMF"))
	{
	if ((void *) dmf_file == 0)
	   {
	    dmf_file = opendmfile();
	    if (!dmf_file)  PXE "can't open DISPLAYFILE\n");
	   }
	if ((void *) dmf_file)  dmf_sw = 1;
	dis_messages();
	dmf_sw = 0;
	}
    else
    if (match(token, "DO"))
	dis_objects();
    else
    if (match(token, "DS"))
	{
	dis_stats();
	 }
    else
    if (match(token, "H"))
	twhelp();
    else
    if (match(token, "BPO"))
	set_bp(1);
    else
    if (match(token, "BPT"))
	set_bp(0);
    else
    if (match(token, "BFO"))
	{if(bp_switch >= 2 ) bp_switch -= 2;}
    else
    if (match(token, "BFT"))
	{if(bp_switch == 1 || bp_switch == 3 ) bp_switch--;}
    else
    if (match(token, "DST"))
	state_display();
    else
	warning("invalid option");
}


/************************************************************************
*
*	dis_messages - displays event message queue
*
*	called by - perform_option
*
*	- perform routine to return the next token in user response
*	- if token is "DEL"
*	-    perform routine to display deleted  messages
*	- if token is "CUR"
*	-    perform routine to display current event's  messages
*	- else if token is null
*	-    perform routine to display only active messages
*	- else
*	-    perform routine to search for other options for message
*	-       selection
*
*************************************************************************/

void FUNCTION dis_messages()
{
    gnxt_token(1);
    if (match(token, "DEL") || match(token, "del"))
	dm_queue(1);
    else
    if (match(token, "PRE") || match(token, "pre"))
	dm_queue(3);
    else
    if (match(token, "NEW") || match(token, "new"))
	dm_queue(4);
    else
    if (match(token, ""))
	dm_queue(2);
    else
    if (dm_determine_options() == SUCCESS)
	dm_select();
    else
	warning("invalid display option");
}

/************************************************************************
*
*  message_display :  use user message display function to display the
*  ccontents of a message. 
*  called by debug option from keyboard. Only works inside an object 
*  because gl_type_ind must be pointing to object.
*
*  ####
*************************************************************************/
FUNCTION int message_display(sel,txtptr,index)
long sel;
Message  *txtptr;
int index;	/* must be set to a type_ind, usually gl_ */
{
  int lns;
  if (txtptr == NULL || index == -1)
     {
     PXE "display: null pointer or no index\n");
     return(1);
     }

  if (process[index].displayMsg != NULL)

     lns =  (*process[index].displayMsg)(sel, txtptr);
  else lns = 0;
  return(lns);
}

/************************************************************************
*
* find the type index of an object with a given name
*
*************************************************************************/
FUNCTION get_type_indx(name)
Name_object *name;
{
    int indx;
    int bod_idx;

    if (cr_sch_obj_table(name) == FAILURE )
	{PXE "get_type_indx: object name not found- %s\n",name);
	 return(-1);
	}
    else
	bod_idx = obj_hdr[hdr_ind].obj_bod_ptr;
	return( obj_bod[bod_idx].proc_ptr);
}


/************************************************************************
*
*	dm_queue - display the event message queue, a page
*				  at a time
*
*	called by - dis_messages
*
*	- depending on display option, display appropriate header
*	-    and set starting position in queue
*	- perform routine to display message header
*	- perform message display routine as required
*	- break for pages and perform message header routine
*	- allow for "quit" option from pause routine
*
*************************************************************************/


void FUNCTION dm_queue(option)
int             option;
{

    int             i, lcnt, cflag, qindx, tindx, mlns;
    mesg_block     *mptr;

    if (option == 1)	/* DEL */
    {
	mptr = get_msg_pointer(1,0);
	PXE "******  Deleted Event Message Queue ********\n");
	do_del_msgs(mptr);
	return;
    }
    if (option == 4)	/* NEW */
    {
	mptr = get_msg_pointer(4,0);
	if (!dmf_sw)
	  {
	  PXE "******  Message for Next Event ********\n");
	  dm_header();
	  dm_one_message(mptr, 0);
	  message_display(mptr->select,mptr->text,gl_type_ind);
	  }
	else
	  {
	  PXF "******  Message for Next Event ********\n");
	  dm_one_message(mptr, 0);
	  }
	return;
    }
    if (option == 3)	/* PRE */
    {
	mptr = mesg_ptr[0];
	if (mptr == NULL)
	  {
	  PXE "no previous event\n");
	  return;
	  }
	if (!dmf_sw) {
	    PXE "**********  Messages for Previous Event **********\n");
	    PXE "There are %d available messages\n\n",mesg_lmt);
	} else {
	    PXF "**********  Messages for Previous Event **********\n");
	    PXF "There are %d available messages\n\n",mesg_lmt);
	}
	i = 0;
	lcnt = 0;
	cflag = TRUE;
	if (!dmf_sw) dm_header();
	tindx = get_type_indx(mptr->rname);
	while (i < mesg_lmt && cflag)
	{
	    lcnt++;
	    dm_one_message(mptr, i++);
	    if (!dmf_sw) {
	      if (tindx != -1)
	         mlns = message_display(mptr->select,mptr->text, tindx);
	      else mlns = 0;
	      mptr = mesg_ptr[i];
	      if (lcnt >= 5 || mlns >= 3)
	      {
		lcnt = 0;
		if (cflag = pause2())
		    dm_header();
	      }
	    }
	}
	return;
    }
    if (option == 2)	/* no option on DM  (read everything) */
    {
	qindx = 1;
	mptr = get_msg_pointer(2,qindx);
	if (!dmf_sw)
	   PXE "**********  Active Event Messages **********\n");
	else
	   PXF "**********  Active Event Messages **********\n");

	if (mptr == 0  || last_q == 0) 
	  {
	  if (!dmf_sw)
	     PXE "\n            No messages exist\n");
	  else
	     PXF "\n            No messages exist\n");
	  return;
	  }
	i = 0;
	lcnt = 0;
	qindx = 1;
	cflag = TRUE;
	dm_header();
	while (qindx <= last_q && cflag)
	{
	    lcnt++;
	    dm_one_message(mptr, i++);
	    if (!dmf_sw) {
	       tindx = get_type_indx(mptr->rname);
	       if (tindx != -1)
	         mlns = message_display(mptr->select,mptr->text, tindx);
	         else mlns = 0;
	    }
	    qindx++;
	    mptr = get_msg_pointer(5,qindx);
	    if (mptr == NULL)
		{
		PXE "***** no more messages ****\n");
		if (dmf_sw) PXF "***** no more messages ****\n");
		break;
		}

	    if (!dmf_sw && (lcnt >= 5 || mlns >= 3))
	    {
		lcnt = 0;
		if (cflag = pause2())
		    dm_header();
	    }
	}
    }
    PXE "\n");
    return;
}

/************************************************************************
*
* cvt time to string even if +/- infinity    (MD)
*
*************************************************************************/
FUNCTION ttoc (string, time)		/* time to character */
    char           *string;
    VTime           time;
{
    if (time.simtime == POSINF)
	strcpy (string, "+inf  ");
    else
    if (time.simtime == POSINF+1)
	strcpy (string, "+inf+1");
    else
    if (time.simtime == NEGINF)
	strcpy (string, "-inf  ");
    else
    if (time.simtime == NEGINF+1)
	strcpy (string, "-inf+1");
    else
	sprintf (string, "%-10.4lf", time.simtime);
}

/************************************************************************
*
*	dm_select - search and display selected messages
*
*	called by - dis_messages
*
*	- set up search
*	- if specification is met, display message
*	- break for pages and perform routine to display message header
*
*************************************************************************/
    
void FUNCTION dm_select()
{
    int             i,
                    lcnt,
		    mlns,
                    cflag,
                    count,
		    tindx,
		    qindx;
    mesg_block     *mptr;

    qindx = 1;
    i = 0;
    lcnt = 0;
    count = 0;
    mptr = get_msg_pointer(2,qindx);
    cflag = TRUE;
    dm_header();
    while (qindx <= last_q && cflag)
    {
	i++;
	if
	    (
	     (dnopt == 1 && match(dname, mptr->rname)) ||
	     (dnopt == 2 && match(dname, mptr->sname)) ||
	     (dtopt == 1 && (dvtime.simtime == mptr->rlvt.simtime)) ||
	     (dtopt == 2 && (dvtime.simtime == mptr->slvt.simtime))
	    )
	{
	    lcnt++;
	    count++;
	    dm_one_message(mptr, i);
	    if (!dmf_sw) {
	       tindx = get_type_indx(mptr->rname);
	       if (tindx != -1)
	          mlns = message_display(mptr->select,mptr->text, tindx);
	       else mlns = 0;
	       if (lcnt >= 5 || mlns >= 3) {
		lcnt = 0;
		if (cflag = pause2())
		    dm_header();
	    }
	    } /* if not dmf_sw */
/* the message_display module has been written by the user and knows nothing
about DISPLAYFILE so don't use it.  Also line count and new hdrs not necessary */
	}
	qindx++;
        mptr = get_msg_pointer(5,qindx);
	    if (mptr == NULL)
		{
		if (dmf_sw) {
		    PXF "***** no more messages ****\n");
		    break;  }
		else {
		     PXE "***** no more messages ****\n");
		     break; }
		}
    }
    if (count == 0)
	PXE "\n            No messages meet criteria\n");
}


/************************************************************************
*
*	dm_determine_options - return either object name or virtual time
*
*	called by - dis_messages
*
*	- if first character of token is alpha
*	-    perform routine to return an object name
*	- else if first character is numeric
*	-    perform routine to return a virtual time
*
*************************************************************************/

FUNCTION dm_determine_options()
{
    dnopt = 0;
    dtopt = 0;
    if (isalpha(token[0]))
	return (dm_get_object_name());
    else
    if (isdigit(token[0]))
	return (dm_get_virtual_time());
    else
	warning("invalid display option");
    return (FAILURE);
}

/************************************************************************
*
*	dm_get_object_name - returns an object name with a specification
*			     as to whether it is to be considered a send
*			     or receive object
*
*	called by - dm_determine_options
*
*************************************************************************/

FUNCTION dm_get_object_name()
{
    if (strlen(token) < NOBJLEN)
    {
	strcpy(dname, token);
	gnxt_token(2);
	if (token[0] == 'R')
	{
	    dnopt = 1;
	    gnxt_token(1);
	    if (isdigit(token[0]))
		return (dm_get_virtual_time());
	    else
	    if (token[0] == '\0')
		return (SUCCESS);
	    else
		warning("invalid option");
	}
	else
	if (token[0] == 'S')
	{
	    dnopt = 2;
	    gnxt_token(1);
	    if (isdigit(token[0]))
		return (dm_get_virtual_time());
	    else
	    if (token[0] == '\0')
		return (SUCCESS);
	    else
		warning("invalid option");
	}
	else
	if (isdigit(token[0]))
	{
	    dnopt = 1;
	    return (dm_get_virtual_time());
	}
	else
	if (token[0] == '\0')
	{
	    dnopt = 1;
	    return (SUCCESS);
	}
	else
	    warning("S or R only");
    }
    else
	warning("object name too long");
    return (FAILURE);
}

/************************************************************************
*
*	dm_get_virtual_time - returns virtual time with a specification
*			     as to whether it is to be considered a send
*			     or receive virtual time
*
*	called by - dm_determine_options
*
*************************************************************************/

FUNCTION dm_get_virtual_time()
{
    if (strlen(token) <= 10)
    {
	dvtime.simtime = (STime)atof(token);
	gnxt_token(2);
	if (token[0] == 'R')
	{
	    dtopt = 1;
	    return (SUCCESS);
	}
	else
	if (token[0] == 'S')
	{
	    dtopt = 2;
	    return (SUCCESS);
	}
	else
	if (token[0] == '\0')
	{
	    dtopt = 1;
	    return (SUCCESS);
	}
	else
	    warning("S or R");
    }
    else
	warning("time value too large");
    return (FAILURE);
}

/************************************************************************
*
*	do_del_msgs(mptr);
*       get messages from free list and display them
*	called by dm_queue
*
*************************************************************************/
void FUNCTION do_del_msgs(mptr)
mesg_block  *mptr;
{
     mesg_block   *bl1, *bl2;
     int  i, lcnt, cflag;

     if (mptr == 0)
	PXE "\n            No deleted messages exist\n");
     else
     {
	i = 0;
	lcnt = 1;
	cflag = TRUE;
	dm_header();
	dm_one_message(mptr, i++);
	bl2 = mptr;
	while ((bl1 = bl2->next) != 0 && cflag)
	{
	    lcnt++;
	    dm_one_message(bl1, i++);
	    if (lcnt >= 5)
	    {
		lcnt = 0;
		if (cflag = pause2())
		    dm_header();
	    }
	bl2 = bl1;
	}
     }
     return;
}


/************************************************************************
*
*	dm_one_message - format and display one event message
*
*	called by - dm_queue, dm_select,
*		    error
*
*   Modified on 2/3/88 by FPW to print the first two bytes of each
*      message as a decimal indicating the message type. -- changed
*      by JJW to print selector
*
*************************************************************************/

void FUNCTION dm_one_message(mptr, ind)
int             ind;
mesg_block     *mptr;

{
	char	str1[30];
	char	str2[30];

	char *mg_ptr;

    if (mptr != NULL)
    {
	mg_ptr = dprep_mesg(mptr->mtlen, mptr);
	ttoc(str1, mptr->slvt);
	ttoc(str2, mptr->rlvt);
	if (!dmf_sw)

	PXE "%4d - sender: %s @ %s, %ld, %ld\n       receiver: %s @ %s, %ld, %ld\n       selector: %ld  - length: %d - status: %c - rltime: %-10.2lf\n       <%s>\n",
    ind,mptr->sname,str1,mptr->slvt.sequence1,mptr->slvt.sequence2,
    mptr->rname,str2,mptr->rlvt.sequence1,mptr->rlvt.sequence2,
    mptr->select, mptr->mtlen, mptr->status, mptr->msg_time,mg_ptr);

	else

	PXF "%4d - sender: %s @ %s, %ld, %ld\n       receiver: %s @ %s, %ld, %ld\n       selector: %ld  - length: %d - status: %c - rltime: %-10.2lf\n       <%s>\n",
    ind,mptr->sname,str1,mptr->slvt.sequence1,mptr->slvt.sequence2,
    mptr->rname,str2,mptr->rlvt.sequence1,mptr->rlvt.sequence2,
    mptr->select, mptr->mtlen, mptr->status, mptr->msg_time,mg_ptr);


/*	PXE "%4d - sender: %s @ %s, %ld, %ld\n",ind,mptr->sname,
		str1,mptr->slvt.sequence1,mptr->slvt.sequence2);
	PXE"       receiver: %s @ %s, %ld, %ld\n",mptr->rname,
		str2,mptr->rlvt.sequence1,mptr->rlvt.sequence2);

	PXE "       selector: %ld  - length: %d - status: %c - rltime: %-10.2lf\n",
		mptr->select, mptr->mtlen, mptr->status, mptr->msg_time);
	PXE "       <%s>\n", mg_ptr); */
    }
}
/************************************************************************
*
*	dm_header - display the message header
*
*	called by - dm_queue, dm_select
*
*************************************************************************/

void FUNCTION dm_header()
{
  PXE "----------------------------------------------------------------\n");
}

/************************************************************************
*
*	dprep_mesg 
*
*	called by - dm_one_message
*
*	- determine the maximum number of characters we will display
*	- copy into the display buffer the displayed chars
*	- (check for printability), print dot if not printable.
*	- return pointer to buffer
*
*************************************************************************/

char FUNCTION  *dprep_mesg(text_len, mptr)
int             text_len;
mesg_block     *mptr;
{
    int             xfer_size;
    char           *ptr1,
                   *ptr2,
                   *ptrn;

    ptr1 = dtext;
    xfer_size = text_len;
    if (xfer_size > DTEXT_SIZE)
	xfer_size = DTEXT_SIZE;  /* max print is one line this long */
    if (xfer_size > mesgdefsize) 
	    ptrn = ptr1 + mesgdefsize;
	else
	    ptrn = ptr1 + xfer_size;
    ptr2 = mptr->text;

    while (ptr1 < ptrn)
	{
	    if (isprint(*ptr2))
		*ptr1++ = *ptr2++;
	    else
	    {
		*ptr1++ = '.';
		ptr2++;
	    }
	}

    *ptr1++ = '\0';
    ptr1 = dtext;
    return (ptr1);
}

/************************************************************************
*
*	dprep_text - prepare text for display - debugging
*
*	called by - most of the time warp entry routines
*
*************************************************************************/

char FUNCTION   *dprep_text(text_len, text)
int             text_len;
char           *text;
{
    char           *ptr1,
                   *ptr2,
                   *ptrn;

    ptr1 = dtext;
    ptr2 = text;
    if (text_len > DTEXT_SIZE)
	ptrn = ptr1 + DTEXT_SIZE;
    else
	ptrn = ptr1 + text_len;
    while (ptr1 < ptrn)
	if (isprint(*ptr2))
	    *ptr1++ = *ptr2++;
	else
	{
	    *ptr1++ = '.';
	    ptr2++;
	}
    *ptr1++ = '\0';
    ptr1 = dtext;
    return (ptr1);
}

/************************************************************************
*
*	dis_objects - display the objects currently known by the
*			  simulator
*
*	called by - perform option
*
*	- perform routine to get the next user input token
*	- if token is NULL
*	-    perform routine display headers
*	-    perform routine to display object info
*	-    (allow page breaks and pause)
*	- else (assume user has specified a particular object name)
*	-    perform routine to search the object table
*	-    if found
*	-       perform routine to display header
*	-       perform routine to display the object
*
*************************************************************************/

void FUNCTION dis_objects()
{
    int             i,
                    lcnt,
                    cflag;

    PXE"       **********  Object(s) **********\n");
    gnxt_token(1);
    if (match(token, ""))
	if (num_objects == 0)
	    PXE "\n            No objects exist\n");
	else
	{
	    lcnt = 0;
	    cflag = TRUE;
	    do_header();
	    for (i = 0; i < num_objects && cflag; i++)
	    {
		lcnt++;
		do_one_object(i);
		if (lcnt >= 5)
		{
		    lcnt = 0;
		    if (cflag = pause2())
			do_header();
		}
	    }
	}
    else
    if (strlen(token) < NOBJLEN)
    {
	strcpy(dname, token);
	if (cr_sch_obj_table(dname) == SUCCESS)
	{
	    do_header();
	    do_one_object(hdr_ind);
	}
	else
	    warning("object does not exist");
    }
    else
	warning("invalid object name");
    PXE"\n");
}

/************************************************************************
*
*	do_one_object - format and display object information
*
*	called by - dis_objects, error
*
*************************************************************************/

void FUNCTION do_one_object(ind)
int             ind;
{
    int             jnd;
    char	str1[30];
    char	str2[30];
    char	str3[30];
 /*  int                 knd; */

    if (num_objects > 0)
    {
	jnd = obj_hdr[ind].obj_bod_ptr;
	ttoc(str1,obj_bod[jnd].current_lvt);
	ttoc(str2,obj_bod[jnd].create_lvt);
	ttoc(str3,obj_bod[jnd].destroy_lvt);
/*	knd = obj_bod[jnd].proc_ptr;	 JJW term */
	PXE "%4d - %-20s  %d\n", ind, obj_hdr[ind].name,
	       obj_bod[jnd].pnode);
	PXE "       %15s %15s %15s %-15.2lf\n",
	       str1, str2, str3, obj_bod[jnd].obj_time);
	PXE "current state ptr: %8ld        statesize: %4ld\n",
	       obj_bod[jnd].current, obj_bod[jnd].ssize);
    }
}

/************************************************************************
*
*	do_header - display object header
*
*	called by - dis_objects
*
*************************************************************************/

void FUNCTION do_header()
{
    PXE "\n       Object                Node\n");
    PXE "      Current Lvt");
    PXE "         Create Lvt         Destroy Lvt          Rltime\n");
    PXE "              Current State             State Size\n");
    PXE "---------------------------");
    PXE "------------------------------------------\n");
}

/************************************************************************
*
*  state_display :  use user state display function to display the
*  current state of the current object. 
*  called by debug option from keyboard.
*
*
*************************************************************************/
FUNCTION state_display()
{
  if (process[gl_type_ind].displayState != NULL)
         (*process[gl_type_ind].displayState)(obj_bod[gl_bod_ind].current);

}

/*************************************************************************
*
*  FILE *opendmfile();
*
*  try to open DISPLAYFILE and if you can, delete it.  If you can't, open
*  (create) it for append.  If something goes wrong return 0, else
*  return (FILE *).
*
***************************************************************************/
FILE  *FUNCTION opendmfile()
{
     FILE * xx;
     int yy;

     if ( (xx = fopen("DISPLAYFILE", "r")) );
	{
	  yy = fclose(xx);
	  yy = unlink("DISPLAYFILE");
	  if (yy) PXE "error deleting existing DISPLAYFILE");
	}
     xx = fopen("DISPLAYFILE", "a");
     return(xx);

}

/***************************************************************************
*
*   HELP MESSAGE
*
*************************************************************************/

void FUNCTION twhelp ()
{
 P "\n.............Time Warp Simulator Help...(v%s)....\n",versionno);
 P " program options: [-[t][c][q][m][M][d][n][s][a][k arg]] [config file]\n");
 P " q=stop at beginning with step switch true (ST).\n");
 P " t=make event trace: TRACE, m=make message trace: MTRACE\n");
 P " n=turn off all internal timing calls, d=make data file: SIMDATA\n");
 P " a=make archive: arc.jrn, c= find critical pathtime (needs -t also)\n");
 P " M=make input message trace: IMTRACE, k=print line every 'arg' events\n\n");
 P " SWITCHES: all switches followed by T for true or F for false. They apply\n");
 P " to all succeeding events until changed (e.g. ST= step true)\n");
 P " Type <cr> to restart after typing a switch\n");
 P " S step switch, stop after each event, show next event (default false) \n");
 P " B breakpoint switch, stop at given time or object\n");
 P "    (BPT <number> or BPO <object name>   (default neither)\n");
 P "    (BFT  or BFO  turn off specified breakpoint type\n");
 P " A archive switch, archive events, requires -a on command line (default false)\n");
 P " T trace switch, print events but don't stop (default false)\n");
 P " D debug switch, enter debug routine for much printout (default false)\n");
 P " other options when twsim prompts at an error or at a pause\n");
 P " a <cr>  when twsim prompts will continue the simulation \n");
 P "   DO                - Display all Objects\n");
 P "   DO [object name]  - Display all Objects with specified name\n");
 P "   DM                - Display active messages\n");
 P "   DM   [del]        - Display deleted messages\n");
 P "   DM   [pre]        - Display messages for previous event\n");
 P "   DM   [new]        - Display message initiating next event\n");
 P "   DM   [obj name] [R,S]         object as Snder or Rcver\n");
 P "   DM   [vir time] [R,S]         at Send or Receive time\n");
 P "   DMF               - same as DM but outputs to DISPLAYFILE (no dmf del)\n");
 P "   DS                - Display Statistics\n");
 P "   DST               - Display State for object if code provided by user\n");
 P "   Q                 - Quit\n\n");
 P "   H		 - Print this message\n\n");
 P " Timing starts after configuration.\n");
 P " To interrupt simulator and get prompt, type control-c\n");
}

