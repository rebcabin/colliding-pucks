/*      Copyright (C) 1989, 1991, California Institute of Technology.
		U. S. Government Sponsorship under NASA Contract NAS7-918
		is acknowledged.        */

/*
 * $Log:	turboq2.c,v $
 * Revision 1.5  91/12/27  09:22:27  pls
 * Fix up TIMING code.
 * 
 * Revision 1.4  91/07/17  15:13:50  judy
 * New copyright notice.
 * 
 * Revision 1.3  91/06/03  12:27:21  configtw
 * Tab conversion.
 * 
 * Revision 1.2  91/04/01  15:49:39  reiher
 * Code to check problems when find() hits a fatal condition.  These changes
 * may eventually be removed.
 * 
 * Revision 1.1  90/08/07  15:41:33  configtw
 * Initial revision
 * 
*/
char turboq2_id [] = "@(#)turboq2.c     1.25\t6/2/89\t12:46:32\tTIMEWARP";


#include "twcommon.h"
#include "twsys.h"

#if TIMING
#define IQCMP_TIMING_MODE 15
extern int timing_mode;
#endif

/*#define Q_INPUT_PARANOIA  /* undefine for speed and less error checking */
/*#define Q_OUTPUT_PARANOIA  /* undefine for speed and less error checking */
#define Q_TWERROR 1             /* 0 */
/* define Q_TWERROR to be 1 if you want error reporting via twerror
   to be enabled, and define Q_TWERROR to be zero if you don't want
   error reporting via twerror
 */

#ifdef Q_INPUT_PARANOIA

#define check_i_arg_macro(X) \
if ( m == NULLMSGH ) {\
if ( Q_TWERROR ) { twerror ( X ) ; twerror ( "NULL argument" ) ; }\
return NULLMSGH ;\
}\
if ( l_ishead_macro(m) ) {\
if ( Q_TWERROR ) { twerror ( X ) ; twerror ( "list header argument" ) ; }\
}

#define check_o_arg_macro(X) \
if ( m == NULLMSGH ) {\
if ( Q_TWERROR ) { twerror ( X ) ; twerror ( "NULL argument" ) ; }\
return NULLMSGH ;\
}\
if ( l_ishead_macro(m) ) {\
if ( Q_TWERROR ) { twerror ( X ) ; twerror ( "list header argument" ) ; }\
}

#else

#define check_i_arg_macro(X)
#define check_o_arg_macro(X)

#endif                          /* Q_INPUT_PARANOIA */


#ifdef Q_OUTPUT_PARANOIA

#define check_i_retval_macro(s,X) \
if ( s == NULLMSGH ) {twerror(X) ; twerror("NULL return value");}\
if ( l_ishead_macro(s) ) {twerror(X) ; twerror("list header return value");}

#define check_o_retval_macro(s,X) \
if ( s == NULLMSGH ) {twerror(X);twerror("NULL return value");}\
if ( l_ishead_macro(s) ) {twerror(X);twerror("list header return value");}

#else

#define check_i_retval_macro(s,X)
#define check_o_retval_macro(s,X)

#endif                          /* Q_OUTPUT_PARANOIA */

#define END_FUNC                /* textual sugar for easy vi yanking */


/******************************************************************************/

FUNCTION Msgh * fstigb (m)      /* first input group in a bundle        */
	register Msgh  *m;          /*      (same receive time)             */
{
	register Msgh  *s, *t;

	check_i_arg_macro ("fstigb")

	s = t = m;

not_found_yet:

	t =  (Msgh *) l_prev_macro (t);

	if (l_ishead_macro (t))
	{
		check_i_retval_macro (s, "fstigb")
		return s;
	}

	if ( neVTime ( t->rcvtim, m->rcvtim ) )
	{
		check_i_retval_macro (s, "fstigb")
		return s;
	}

	s = t;

	goto not_found_yet;
}

END_FUNC



/*****************************************************************************/

FUNCTION Msgh * nxtigb (m)      /* next input group in a bundle         */
	register Msgh  *m;          /*      (same receive time)             */
{
	register Msgh  *t;

	check_i_arg_macro ("nxtigb")

	t = m;

not_found_yet:

	t = (Msgh *) l_next_macro (t);

	if (l_ishead_macro (t))
	{
		return NULLMSGH;
	}

	if ( neVTime ( t->rcvtim, m->rcvtim ) )
	{
		return NULLMSGH;
	}

	check_i_retval_macro (t, "nxtigb")
	return t;
}

END_FUNC


/******************************************************************************/

FUNCTION Msgh * nxtibq (m)      /* next input bundle in a queue         */
	register Msgh  *m;          /*      (different receive time)        */
{
	register Msgh  *t;

	check_i_arg_macro ("nxtibq")

	t = m;

not_found_yet:

	t = (Msgh *) l_next_macro (t);

	if (l_ishead_macro (t))
	{
		return NULLMSGH;
	}

	/* The second half of this if statement is an evil kluge to get the
		system to process dynamic destruction messages that are coincident
		in virtual time with event messages.  The right way is to have a
		system-only low order part of virtual time that sets destroys to
		be at a later virtual time than creates. */

	if ( neVTime ( t->rcvtim, m->rcvtim ) ||
	   ( eqVTime ( t->rcvtim, m->rcvtim ) && t->mtype == DYNDSMSG))
	{
		check_i_retval_macro (t, "nxtibq")
		return t;
	}

	goto not_found_yet;
}

END_FUNC


/******************************************************************************/

FUNCTION Msgh * prvigb (m)      /* previous input group in a bundle     */
	register Msgh  *m;          /*      (same receive time)             */
{
	register Msgh  *t;

	check_i_arg_macro ("prvigb")

	t = (Msgh *) l_prev_macro (m);

	if ( l_ishead_macro (t) )
	{
		return NULLMSGH;
	}

	if ( neVTime ( t->rcvtim, m->rcvtim ) )
	{
		return NULLMSGH;
	}

	check_i_retval_macro (t, "prvigb")
	return t;
}

END_FUNC

/******************************************************************************/

FUNCTION Msgh * prvibq (m)      /* previous input bundle in a queue     */
	register Msgh  *m;          /*      (different receive time)        */
{
	register Msgh  *t, *s;

	check_i_arg_macro ("prvibq")

	t = m;

searching_for_prv_bundle:

	t = (Msgh *) l_prev_macro (t);

	if (l_ishead_macro (t))
	{
		return NULLMSGH;        /* there is no previous bundle */
	}

	if ( eqVTime ( t->rcvtim, m->rcvtim ) )
	{
		goto searching_for_prv_bundle;
	}

	s = fstigb (t);

	return s;
}

END_FUNC


/******************************************************************************/

FUNCTION Msgh * fstomb (m)      /* first output message in a bundle     */
	register Msgh  *m;
{
	register Msgh  *s, *t;

	check_o_arg_macro ("fstomb")

	s = t = m;

not_found_yet:

	t = (Msgh *) l_prev_macro (t);

	if (l_ishead_macro (t))
	{
		check_o_retval_macro (s, "fstomb")
		return s;
	}

	if ( neVTime ( t->sndtim, m->sndtim ) )
	{
		check_o_retval_macro (s, "fstomb")
		return s;
	}

	s = t;

	goto not_found_yet;
}

END_FUNC


/******************************************************************************/

FUNCTION Msgh * nxtomb (m)      /* next output message in a bundle      */
	register Msgh  *m;
{
	register Msgh  *t;

	check_o_arg_macro ("nxtomb")

	t = m;

	t = (Msgh *) l_next_macro (t);

	if (l_ishead_macro (t))
	{
		return NULLMSGH;
	}

	if ( neVTime ( t->sndtim, m->sndtim ) )
	{
		return NULLMSGH;
	}

	check_o_retval_macro (t, "nxtomb")
	return t;
}

END_FUNC


/******************************************************************************/

FUNCTION Msgh * nxtobq (m)      /* next output bundle in a queue        */
	register Msgh  *m;          /*      (different send time)           */
{
	register Msgh  *t;

	check_o_arg_macro ("nxtobq")

	t = m;

not_found_yet:

	t = (Msgh *) l_next_macro (t);

	if (l_ishead_macro (t))
	{
		return NULLMSGH;
	}

	if ( eqVTime ( t->sndtim, m->sndtim ) )
	{
		goto not_found_yet;
	}

	check_o_retval_macro (t, "nxtobq")
	return t;
}

END_FUNC


/******************************************************************************/

FUNCTION Msgh * prvobq (m)      /* previous output bundle in a queue    */
	register Msgh  *m;          /*      (different send time)           */
{
	register Msgh  *t, *s;

	check_o_arg_macro ("prvobq")

	t = m;

searching_for_prv_bundle:

	t = (Msgh *) l_prev_macro (t);

	if (l_ishead_macro (t))
	{
		return NULLMSGH;        /* there is no previous bundle */
	}

	if ( eqVTime ( t->sndtim, m->sndtim ) )
	{
		goto searching_for_prv_bundle;
	}

	s = fstomb (t);

	return s;
}

END_FUNC


/****************************************************************************
********************************   FIND   ***********************************
*****************************************************************************

	Search for a thing in an ordered queue; return a pointer to the thing
	if found, else a pointer to the greatest lower bound of the thing
	(rather that the least upper bound since "find" is meant to be used
	with an insert-after function).  In the case of equals, find returns
	a pointer to the last of a sequence of equals, so that FIFO order is 
	maintained (you get to insert after the last of equals).

	Note that "find" does not check the
	signs of input or output msgs and states; in this, it differs from the
	NXT and PRV classes of functions.  The input function comp tests the
	order of two elements in the queue.  "find" uses the lowest level of
	queueing primitives (list processing for Mark II), and so must be
	modified if a new queue processing method is implemented.

	A requirement on the function comp: it must return neg, 0, pos as
	its first arg is earlier than, equal to, or later than its second
	arg in the queue.  It will NOT do anything reasonable with a queue 
	governed by a comp function that returns only equal or not-equal 
	(zero or non-zero); such a queue is governed by the wrong algebra.
	find decides the direction to continue hunting for the desired
	element based on the results of the first call of comp.
	*/

int find_count = 0; /* for quelog */

FUNCTION Byte  *find (
									  qhd,
									  s,
									  t,
									  comp,
									  comprtn
)

	register Byte  *qhd;
	register Byte  *s;          /* starting place in some queue */
	register Byte  *t;          /* target (not necessarily in a queue) */

Long (*comp) ();                /* user-supplied order routine */
register Long  *comprtn;        /* for checking equality */

{
	register Byte  *tmp;
	register Long   tmprtn;


	*comprtn = 1;               /* don't return zero by default */

#ifdef  PARANOID                /* ???PJH OPTIMIZATION...        */

	if (qhd == NULL)
	{
		return (Byte *) NULL;
	}

	if (t == NULL)
	{
		return (Byte *) NULL;
	}

#endif PARANOID

	find_count = 0;

	if (s == NULL || l_ishead_macro (s))
	{
		s = (Byte *) l_prev_macro (qhd);
		if (l_ishead_macro (s))
		{                       /* We have an empty queue  */
			return qhd;
		}
	}

	/* best to do this as a tiny state machine */

	find_count++;

	*comprtn = (*comp) (s, t);
	if (*comprtn > 0)
	{   /* s > t */
		goto s_is_later;
	}
	else
	if (*comprtn == 0)
	{   /* s == t */
		goto s_is_equal;
	}
	else
	{   /* s < t */
		goto s_is_earlier;
	}

s_is_later:{                    /* also know that *comprtn has been set */
		s = (Byte *) l_prev_macro (s);
		if (l_ishead_macro (s)) /* hit the q head going backwards, so... */
			return qhd;         /* insert t right after q head */
		find_count++;
#if 0
		if (timing_mode == 13)
			{
			start_timing(IQCMP_TIMING_MODE);
			*comprtn = (*comp) (s, t);
			stop_timing();
			}
		else
#endif
		*comprtn = (*comp) (s, t);
		if (*comprtn > 0)
			goto s_is_later;
		else                    /* s is earlier or equal */
			return s;           /* return latest or latest earlier s */
	}

s_is_equal:{                    /* need to find the latest of equals */
		tmp = (Byte *) l_next_macro (s);
		if (l_ishead_macro (tmp))       /* hit the q head going forward */
			return s;
		find_count++;
		*comprtn = (*comp) (tmp, t);
		if (*comprtn == 0)
		{
			s = tmp;
			goto s_is_equal;
		}
		else
		if (*comprtn > 0)
		{                       /* s was the latest */
			*comprtn = 0;       /* return old values */
			return s;
		}
		else
		{
			twerror ("find F fatal ordering snafu");
 _pprintf("hi there\n");
_pprintf("s = %x, tmp = %x, t = %x\n",s, tmp, t);
tester();
		  if ( s != NULL )
			  showocb ( (Ocb *) s );
		  else
				_pprintf("s is NULL\n");

		  if ( tmp != NULL )
			  showocb ( (Ocb *) tmp );
		  else
				_pprintf("tmp is NULL\n");

		  if ( t != NULL )
			  showocb ( (Ocb *) t );
		  else
				_pprintf("t is NULL\n");

			tester ();

			exit ();
		}
	}

s_is_earlier:{
		tmp = (Byte *) l_next_macro (s);
		if (l_ishead_macro (tmp))
			return s;           /* hit q head so return s */
		tmprtn = *comprtn;      /* save old value */
		find_count++;
		*comprtn = (*comp) (tmp, t);
		if (*comprtn == 0)
		{                       /* found an equal one */
			s = tmp;
			goto s_is_equal;
		}
		else
		if (*comprtn > 0)
		{
			*comprtn = tmprtn;  /* restore old value */
			return s;
		}
		else
		{                       /* s is still earlier:  loop back */
			s = tmp;
			goto s_is_earlier;
		}
	}
}


FUNCTION nqocb ( o, next )

/* Put object o in time ordered sequence in next's list */

	register Ocb * o, * next;
{
  Debug

	if ( l_ishead ( (List_hdr *) next) || geVTime ( o->svt, next->svt ) )
	{   /* search forward from next */
		for ( next = (Ocb *) l_next_macro ( next );
				   ! l_ishead_macro ( next );
			  next =  (Ocb *) l_next_macro ( next ) )
		{
			if ( ltVTime ( o->svt, next->svt ) )
				break;
		}
		/* now insert o just before next */
		l_insert ( l_prev_macro ( next ), (List_hdr *) o );
	}
	else
	{   /* search backward from next */
		for ( next = (Ocb *) l_prev_macro ( next );
				   ! l_ishead_macro ( next );
			  next = (Ocb *) l_prev_macro ( next ) )
		{
			if ( geVTime ( o->svt, next->svt ) )
				break;
		}
		/* now insert o just after next */
		l_insert ( (List_hdr *) next, (List_hdr *) o );
	}
}
