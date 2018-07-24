/*      Copyright (C) 1989, 1991, California Institute of Technology.
		U. S. Government Sponsorship under NASA Contract NAS7-918
		is acknowledged.        */

/*
 * $Log:	twsys.dcl,v $
Revision 1.4  91/12/27  08:50:34  reiher
Added declaration of findPhaseInDeadQ

Revision 1.3  91/07/17  15:15:02  judy
New copyright notice.

Revision 1.2  91/07/17  09:05:54  configtw
Fix for redefinition of bytcmp() on Sun4.

Revision 1.1  90/08/07  15:41:38  configtw
Initial revision

*/

#ifdef BF_PLUS  /* Unfortunate kludge because of illegal  */
#define BBN     /* redefinitions if we include 'machdep.h */
#endif          /* and stdio.h'.                          */
#ifdef BF_MACH
#define BBN
#endif



FUNCTION List_hdr * l_create () ;
FUNCTION List_hdr * l_hcreate () ;
FUNCTION List_hdr * l_next () ;
FUNCTION List_hdr * l_prev () ;
FUNCTION List_hdr * l_remove () ;
FUNCTION Byte * m_allocate () ;
FUNCTION Char *strcat () ;
FUNCTION Char *strchr () ;
FUNCTION Char *strcpy () ;
FUNCTION Char *strncat () ;
FUNCTION Char *strncpy () ;
FUNCTION Char *strrchr () ;
FUNCTION Char *itoa ();
FUNCTION char toupper();

FUNCTION int ocbcmp ();
FUNCTION int uidcmp ();
#if !SUN4
FUNCTION int bytcmp ();
#endif
FUNCTION int imcmp ();
FUNCTION int omcmp ();
FUNCTION int priocmp ();
FUNCTION int statecmp ();

FUNCTION Msgh * mkimsg () ;
FUNCTION Msgh * mkomsg () ;
FUNCTION Msgh * make_message ();
FUNCTION Msgh * make_static_msg() ;
FUNCTION Msgh * nthgrp () ;
FUNCTION Mem_hdr * m_memory () ;
FUNCTION Msgh * get_memory_or_denymsg();
FUNCTION Msgh * next_unprocessed_message () ;
FUNCTION Msgh * earliest_later_inputbundle () ;
FUNCTION Msgh * specified_outputbundle () ;
FUNCTION Msgh * fstactivegb () ;
FUNCTION Byte * m_create () ;

FUNCTION Msgh * msgfind () ;

FUNCTION Msgh * fstigb () ;
FUNCTION Msgh * nxtimg () ;
FUNCTION Msgh * nxtigb () ;
FUNCTION Msgh * nxtibq () ;
FUNCTION Msgh * prvigb () ;
FUNCTION Msgh * prvibq () ;

FUNCTION Msgh * fstomb () ;
FUNCTION Msgh * nxtomb () ;
FUNCTION Msgh * nxtobq () ;
FUNCTION Msgh * nxtomg () ;
FUNCTION Msgh * prvobq () ;
FUNCTION Byte * find () ;

FUNCTION Msgh * sysbuf () ;
FUNCTION Msgh * output_buf ();
FUNCTION Ocb * mkocb () ;
FUNCTION State * latest_earlier_state () ;
FUNCTION State * mk_savedstate () ;
FUNCTION State * mk_workingstate () ;
FUNCTION State * statefind () ;
FUNCTION Typtbl * find_object_type () ;
FUNCTION int l_maxelt () ;
FUNCTION int m_available () ;
FUNCTION int m_contiguous () ;
FUNCTION int m_next () ;
FUNCTION int percent_free () ;
FUNCTION int percent_used () ;
FUNCTION long bytes_free () ;
FUNCTION long uniq () ;

FUNCTION Objloc *Replace();
FUNCTION Objloc *GetLocation();
FUNCTION Cache_entry *FindInCache();
FUNCTION Cache_entry *CacheReplace();
FUNCTION Cache_entry *ChoosePosition();
FUNCTION Pending_entry *FindInPendingList();
FUNCTION int NullRestart();
FUNCTION int FinishDeliver();

FUNCTION struct HomeList_str *FindInHomeList();
FUNCTION struct HomeList_str *AddToHomeList();
FUNCTION Ocb * FindInSchedQueue();

FUNCTION Ocb * MakeObject();
FUNCTION Ocb * createproc();
FUNCTION Typtbl * ChangeType();

#ifndef MARK3
FUNCTION long clock () ;
#endif

VTime migr_min ();
VTime MinPendingList ();

#ifdef BBN
VTime butterfly_min ();
#endif

FUNCTION Ocb * split_object ();
FUNCTION Ocb * splitNearFuture ();
FUNCTION Ocb * splitMinimal ();
FUNCTION Ocb * splitLimitEMsgs ();
struct HomeList_str *SplitHomeListEntry();
FUNCTION Ocb * bestFit ();
FUNCTION Ocb * nextBestFit ();

float calculateUtilization() ;
Ocb *chooseObject();

FUNCTION deadOcb * findPhaseInDeadQ ();
