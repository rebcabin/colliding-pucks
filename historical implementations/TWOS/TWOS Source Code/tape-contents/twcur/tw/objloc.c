/*      Copyright (C) 1989, 1991, California Institute of Technology.
		U. S. Government Sponsorship under NASA Contract NAS7-918
		is acknowledged.        */

/*
 * $Log:	objloc.c,v $
 * Revision 1.8  91/11/04  10:20:53  pls
 * Add sequence times to pending list display.
 * 
 * Revision 1.7  91/11/01  09:49:15  reiher
 * Statistics code, an optimization to minimize the number of HOMEASKS sent,
 * and all occurrences of miparm.me changed to tw_node_num (PLR)
 * 
 * Revision 1.6  91/07/17  15:11:34  judy
 * New copyright notice.
 * 
 * Revision 1.5  91/06/07  13:48:29  configtw
 * Use m_create instead of l_create.
 * 
 * Revision 1.4  91/06/03  12:25:57  configtw
 * Tab conversion.
 * 
 * Revision 1.3  91/05/31  15:05:53  pls
 * 1.  Allow for case where object is not stored locally when
 *     its phase is completed.
 * 2.  GVT calculation checks send time for messages in pending list.
 * 
 * Revision 1.2  90/12/10  10:51:58  configtw
 * fix PARANOID call to FindInCache()
 * 
 * Revision 1.1  90/08/07  15:40:42  configtw
 * Initial revision
 * 
*/
char objloc_id [] = "@(#)objloc.c       1.30\t10/2/89\t15:42:10\tTIMEWARP";


/*

Purpose:

		objloc.c contains the basic routines for finding objects in 
		the Time Warp Operating System.  The method used is a variant on
		forward addressing.  Every nameable object in the system has
		a home node, whose identity can be found by hashing the object's
		name.  The home node always keeps track of the location of the
		object.  If a message is to be sent to the object, the sending
		node consults the home node to find the object's current location.      
		Each node keeps a cache of object location data, so that not all
		message sends require consulting the home node.  The scheme requires
		that, in some cases, the sending node defer the actual sending of
		a message until it gets a response from the destination object's
		home node.  While not yet implemented, this object location
		facility should work well in the face of both dynamic creation
		and destruction of objects, and object movement.

		This object location scheme replaces the old world map data
		structure.  Any references to the world map should be deleted
		from the rest of the Time Warp system code.

Functions:

		GetLocation(name,time,message,restart) - Respond to a object location
						request
				Parameters - Name *name,VTime time,Msgh *message, 
								Int (*restart) ()
				Return - A pointer to the right cache entry, or to NULL, if 
								not found

		FindInCache(name,time,bucket) - Look in the cache for an object location
						record
				Parameters -  Name *name, VTime time, Int bucket
				Return - A pointer to the cache entry, or to NULL, if not found

		CacheReplace(name,phase_begin,phase_end,node,ocb,bucket) - Put an entry
						into the cache
				Parameters - Name *name,VTime phase_begin, VTime phase_end,
								Int node, Ocb *ocb, Int bucket
				Return - A pointer to the new entry

		RemoveFromCache(name,time) - Remove an entry from the cache and put
						it into the free pool of cache entries
				Parameters - Name *name, VTime time
				Return - 1 on success, 0 if the entry isn't in the cache

		FindObject(name,time,message,restart,HomeNode) - Send an inquiry to
						the home node about an object's location
				Parameters - Name *name,VTime time, Msgh *message, 
						Int (*restart) (), Int HomeNode
				Return - Always returns 0

		ChoosePosition() - Find the cache entry to be replaced when a new entry
						must be loaded
				Parameters - None
				Return - A pointer to an entry that can be used, or a NULL
						pointer if no entry is available (the latter case should
						never happen)

		name_hash_function(name,hashtype) - Hash a name into an offset in one
						of several types of table
				Parameters - Name *name, char hashtype
				Return - The correct offset for that name and table type

		HomeInit() - Initialize a node's as yet empty home list
				Parameters - None
				Return - Always returns 0

		FindInHomeList(name,time) - Find a phase of an object in its home list
				Parameters - Name *name, VTime time
				Return - A pointer to the home list entry, or a NULL pointer if
						the requested phase isn't found

		AddToHomeList(name,phase_begin,phase_end,node) - Add an entry to the 
						home list
				Parameters - Name *name,VTime phase_begin, VTime phase_end,
						Int node
				Return - Always returns 0

		RemoveFromHomeList(name) - Remove an entry from the home list of the
						local node
				Parameters - Name *name
				Return - Returns 0 on success, traps to tester if name cannot
						be found

		RemoteRemoveFromHomeList(name,node) - Request that another node remove
						an entry from its home list
				Parameters - Name *name, Int node
				Return - Always returns 0

		RemoteSplitHomeListEntry(name,time,node) - Request that another node
						split one of its homw list entries
				Parameters - Name *name, VTime time, Int node
				Return - Always returns 0

		ChangeHLEntry(name,node) - Change the contents of a home list entry
				Parameters - Name *name, VTime time, Int node
				Return - Always returns 0

		SplitHomeListEntry(name,time) - Divide a home list entry into two 
						entries, in response to a temporal split
				Parameters - Name *name, VTime time
				Return - Always returns 0

		ServiceHLRequest(message) - Handle a remote request to the local home
						list
				Parameters - Msgh *message
				Return - Always returns 0

		ObjectFound(message) - Handle the response to a home list request that
						was sent to a remote node
				Parameters - Msgh *message
				Return - Always returns 0

		DumpHomeList() - Show the contents of the local home list on the monitor
				Parameters - None
				Return - Always returns 0

		PendingListInit() - Initialize the local list of pending requests to
						remote node's home lists
				Parameters - None
				Return - Always returns 0

		FindInPendingList(name,phase_begin,phase_end,StartSearch) - Look for
						an entry in the local pending list
				Parameters - Name *name, VTime phase_begin,VTime phase_end, 
						Pending_entry *StartSearch
				Return - A pointer to the requested entry in the pending list,
						or a NULL pointer if the entry wasn't found

		AddToPendingList(element) - Add an entry to the local pending list
				Parameters - Pending_entry *element
				Return - Always returns 0

		RemoveFromPendingList(element,locptr) - Finish off the work left undone
						for a pending list entry and remove it from the list
				Parameters - Pending_entry *element, Objloc *locptr
				Return - Always returns 0

		DumpPendingList() - Show the contents of the local pending list on the
						monitor
				Parameters - None
				Return - Always returns 0 

		CacheInit() - Initialize the local node's cache
				Parameters - None
				Return - Always returns 0 

		DumpCache() - Show the contents of the local cache on the monitor
				Parameters - None
				Return - Always returns 0 

		NullRestart(message) - Does nothing; for testing purposes
				Parameters - Msgh *message
				Return - Always returns 0 

		ChooseNode() - Choose a node on which to locate a newly created object
				Parameters - None
				Return - The node's number

		MakeObject(name,node,time) - Dynamically create an object on the 
						specified node
				Parameters - Name *name, Int node, VTime time
				Return - A pointer to the new object's ocb, or a pointer to 
						NULL if the request failed

		wm_hash_function(name) - A currently unused set of hash functions
				Parameters - Name *name
				Return - An integer hashed from the name


Implementation:

*/

#include "twcommon.h"
#include "twsys.h"


#define CACHE_SIZE 63
#define CACHE_POOL 356
#define HOMELISTSIZE 31


Cache_entry     *objloc_cache[CACHE_SIZE];
Pending_entry *PendingListHeader;
struct HomeList_str *HomeListHeader[HOMELISTSIZE];

Cache_entry     *CacheFreePool;
static Ulong    rcount;
Ulong   chit;
Ulong   cmiss;
int homeListSize;
int pendingEntries = 0;
int totHomeAns = 0;
int totHomeAsks = 0;
int totHomeAsksRecv = 0;
int totHomeAnsRecv = 0;

extern int tw_num_nodes;

/*  GetLocation() is the function used to find an object's location in the
		system.  All access to object location information by other parts of 
		the system should go through this function.  GetLocation() checks the 
		local cache, to see if the object in question has an entry there.  If 
		not, it calls FindObject() to fetch an entry and to put that entry 
		into the cache.  If restart is NULL, then GetLocation() is being called
		from debugging mode, and the system will not call FindObject().  
		(Messages don't work from tester, anyway.)
*/

FUNCTION Objloc *GetLocation(name,time)
	Name        *name;
	VTime       time;
{
		Int HomeNode;
		Int CacheBucket;
		Ocb     *o;
		Cache_entry *position, *CachePtr;
		VTime   Ctime;
		VTime   phase_begin;
		VTime   phase_end;

/* Look for the object in the cache, returning a pointer to the cache entry
		if it is there. */

  Debug

		/* get cache address for object */
		CacheBucket = name_hash_function(name,CACHE);

		position = FindInCache(name,time,CacheBucket);

		if (position != (Cache_entry *) NULL)
		{  /* object w/ correct phase found */
				position->replace = rcount++;   /* LRU clock */
				chit++;                         /* cache hit */
				return( &(position->cache_entry));              
		}

/*  Next, check to see if the object is stored locally, in which case, simply
		put it into the cache and return its location in the cache. */

		cmiss++;        /* cache miss */

		o = FindInSchedQueue(name,time);
		if (o != (Ocb *) NULL)
		{  /* local match found in prqhd, so put info in cache */
				position = CacheReplace(name,o->phase_begin,
						o->phase_end,tw_node_num,o,CacheBucket);
				return (&(position->cache_entry));
		}

		/* The object did not have a cache entry, and is not stored locally.
				Check to see if the object's home node is this node. */

		HomeNode = name_hash_function(name,HOME_NODE);

		if (HomeNode == tw_node_num)
		{  /* this is home node */
				Int Hnode;

				struct HomeList_str *HLEntry;

				HLEntry = FindInHomeList(name,time);    /* search home list */
				if (HLEntry != (struct HomeList_str *) NULL)
				{  /* object was found */
						Hnode = HLEntry->node;

/* We need a special check here for the case when the home node is equal to
		the local node.  We already know that the object isn't in our 
		scheduler queue, so if we, the home node, say that it's here, there
		is some kind of problem.  Perhaps the notification that it is being
		sent to this node hasn't gotten here yet, but the change in the home
		node message has. */

						if ((Hnode == tw_node_num) &&
						(ltVTime(HLEntry->phase_end,gvt)))
						{
							return ((Objloc *) NULL);
						}

						phase_begin = HLEntry->phase_begin;
						phase_end = HLEntry->phase_end;
				}
				else
						Hnode = -1;

				if (Hnode == -1)
				{
				/* If the home node doesn't know about the object's location,
						then the object doesn't exist yet.  Create it. */

						Ctime = time;
						Hnode = ChooseNode();   /* node for new object */
						o = MakeObject(name,Hnode,Ctime);       /* make it */

						/* Objects are created with a single phase covering
								the entire period from NEGINF to POSINF. */
						phase_begin = neginf;
						phase_end = posinfPlus1;

						/* put it in this node's home list */
						AddToHomeList ( name, phase_begin, phase_end, Hnode);
				}

				/* now put this in the cache */
				CachePtr = CacheReplace(name,phase_begin,phase_end,Hnode,o,
									    CacheBucket);
				return (&(CachePtr->cache_entry));
		}

		return ((Objloc *) NULL);       /* don't know where object is */
}

/* FindInCache() searches the cache for a particular entry.  It calls the hash
		function to determine which cache bucket the entry would sit in, then
		searches that bucket to determine if it is actually there.  */

FUNCTION Cache_entry   *FindInCache (name,time,bucket)
	Name       *name;
	VTime       time;
	Int         bucket;
{
	register Cache_entry  * position ;
	register int n = 0;

  Debug

	if ( objloc_cache[bucket] )
	{
		for (position=(Cache_entry *) l_next_macro(objloc_cache[bucket]);
				!l_ishead_macro (position);
				position =  (Cache_entry *) l_next_macro (position))
		{       /* search through this bucket's list */
				if (namecmp (position->cache_entry.name, name) == 0 &&
						leVTime (position->cache_entry.phase_begin, time ) &&
						ltVTime (time, position->cache_entry.phase_end ) )
				{ /* name's the same, and it's in this phase */
				/* move record around so next search will be faster */

						if ( n > 2 )    /* ???MAD */
						{ /* if not close, move closer to front */
							l_remove ( (List_hdr *) position);
							l_insert ((List_hdr *) objloc_cache[bucket], 
									  (List_hdr *) position);
						}
						return position;
				}
				n++;
		}
	}
	return ((Cache_entry *) NULL);
}

/*  CacheReplace() puts a new entry into the cache.  It returns a pointer to
		the new cache entry. */

FUNCTION Cache_entry *CacheReplace(name,phase_begin,phase_end,node,ocb,bucket)
		Name *name;
		VTime phase_begin;
		VTime phase_end;
		Int node;
		Ocb *ocb;
		Int bucket;
{
	Cache_entry    * new_entry;

  Debug

	new_entry = ChoosePosition();       /* get an element to use */

	/* set up the fields */
	strcpy(new_entry->cache_entry.name,name);
	new_entry->cache_entry.node = node;
	new_entry->cache_entry.phase_begin = phase_begin;
	new_entry->cache_entry.phase_end = phase_end;
	new_entry->cache_entry.po = ocb;
	new_entry->replace = rcount++;      /* LRU counter value */

	/* link in the element at the beginning */
	l_insert((List_hdr *) objloc_cache[bucket],(List_hdr *) new_entry);

	return (new_entry);
}

/* RemoveFromCache() clears an invalid entry out of the cache.  Since it is
		called only to get rid of an old entry, not to replace it, the entry
		that was removed is returned to the free pool of unused cache entries.
*/

int noHomeAskNeeded = 0;

RemoveFromCache(name,time)
	Name *name;
	VTime time;
{
		Int CacheBucket;
		Cache_entry *position;

		CacheBucket = name_hash_function(name,CACHE);

		position = FindInCache(name,time,CacheBucket);

		if (position == (Cache_entry *) NULL)
		{
			return 0;
		}

		l_remove( (List_hdr *) position);
		l_insert ((List_hdr *) CacheFreePool, (List_hdr *) position);

		return 1;
}

/* FindObject() finds the location of an object that is not in the cache, puts 
		it into the cache, and returns.  The object is found by putting an
		entry for this request into the pending list, allocating and formatting
		a message to send to the home node making the location request, and
		then sending the message.
*/

FUNCTION FindObject(name,time,message,restart,isAMsg)
		Name    *name;
		VTime   time;
		Msgh    *message;
		Int     (*restart) ();
		Int     isAMsg;
{
	Msgh * objlocmsg;
	HLmsg  * locmsg;
	Pending_entry  *pending, *searchPending;
	Int         HomeNode;
    VTime       gvtContrib;

  Debug

	/* One way or another, we'll need a pending list entry.  Set one up. */

    if ( isAMsg )
        gvtContrib = message->sndtim;
    else
        gvtContrib = time;

    pending = ( Pending_entry * ) m_create ( sizeof ( Pending_entry ),
                gvtContrib, NONCRITICAL );

	if (pending == (Pending_entry *) NULL)
	{  /* no room in the pending queue */

		twerror("unable to create pending list entry for time %f, msg %x\n",
				time.simtime, message);

		/*  As an emergency measure, we could try sending the message back,
			if there is a message.  However, some care is necessary to
			permit that sendback to work reliably.  It's possible that
			there is no object location information cached for either the
			sender or receiver of a message, in which case we might enter
			an infinite loop of reversing the message and trying to send
			it, failing due to unavailable location information and no 
			memory for a pending list entry, and yet again reversing and
			trying to send. */

		tester();
	}

	strcpy(pending->object,name);
	pending->Message = message; /* message to be sent */
	pending->restart = restart; /* pending action routine */
	pending->time = time;
	pending->isMsg = isAMsg;

#define PENDING_SEARCH
#ifdef PENDING_SEARCH
	/* Check the pending list for an exact duplicate of the name and
		virtual time of this request.  If such a duplicate exists, we
		want to still put this request in the pending list, but we don't
		want to send off a HOMEASK to the home node, since one is already
		on the way.  Most often, if there is a duplicate, it's just been
		put in the pending list, and is at the end, so start the search
		there and work backwards.  */

	for ( searchPending = l_prev ( PendingListHeader ) ; 
			!l_ishead_macro ( searchPending ) ; 
			searchPending = l_prev ( ( List_hdr * ) searchPending ) )
	{
		if ( eqVTime ( time, searchPending->time ) &&
			 strcmp ( name, searchPending->object ) == 0 )
		{

			/* We have an exact match already in the pending list.  Just
				add the new entry without generating another HOMEASK. */

			AddToPendingList(pending);  /* put this in the queue */
			noHomeAskNeeded++;
			return;
		}

	}
#endif PENDING_SEARCH

	AddToPendingList(pending);  /* put this in the queue */

	/* There is no pending list entry yet for this object, so we'd better
		go look for one. */

	HomeNode = name_hash_function(name,HOME_NODE);

	/* This happens when weird timing conditions mess things up temporarily
		during an object migration.  This pending list entry will be cleared
		by code in migr.c, rather than by the arrival of a system message
		from the home node. */

	if (HomeNode == tw_node_num)
	{
		twerror("Home node request for an object not here yet,%s\n",name);
		tester();
		l_remove ( (List_hdr * ) pending);
		l_destroy((List_hdr *) pending);
		return;
	}

	objlocmsg = sysbuf();       /* get a system buffer even if we wait */
	locmsg = (HLmsg *) (objlocmsg+1);

	/* fill in the blanks */
	locmsg->sender = tw_node_num;
	strcpy(locmsg->object,name);
	locmsg->time = time;

	/* fill in the times */
    if ( isAMsg )
    {
        objlocmsg->sndtim = message->rcvtim;
        objlocmsg->rcvtim = message->rcvtim;
    }
    else
    {
        objlocmsg->sndtim = time;
        objlocmsg->rcvtim = time;
    }

	/* fiddle with the snder & rcver fields */
	sprintf ( objlocmsg->snder, "HOMEASK%d", tw_node_num );
	sprintf ( objlocmsg->rcver, "HOMEANS%d", HomeNode );

	/* send out a system message */

	totHomeAsks++;
	sysmsg(HOMEASK, objlocmsg, sizeof (HLmsg), HomeNode );

}

/* ChoosePosition() chooses the cache entry that will be replaced, when
		replacement is necessary.  The current replacement algorithm is LRU.
		LRU is implemented by keeping a monotonically increasing counter that
		is incremented once for each cache probe.  The cache entry used in the
		probe gets the value of the counter as its replacement value.  When
		something in the cache must be replaced, the lowest replacement value in
		the cache is chosen.  The replacement algorithm is dependent on this
		routine, plus on the parts of FindObject() and GetLocation() that deal 
		with updating the replacement field in the cache entry. */

FUNCTION Cache_entry *ChoosePosition()
{
		Int  cindex,min;
		Cache_entry *CachePtr, *MinPtr;

  Debug

		/* If there are still entries in the free pool, use one of those. */

		CachePtr = (Cache_entry *) l_next_macro(CacheFreePool);

		if (CachePtr && !l_ishead_macro(CachePtr))
		{  /* found a legit entry in the pool */
				l_remove((List_hdr *) CachePtr);        /* remove from pool */
				return (CachePtr);                      /* use it */
		}

		/* Otherwise, something will have to be swapped out. */

		cindex = 0;

		/* Find the first actual entry in the cache, and set the minimum
				replacement value to its replacement value. Since we have
				no guarantee that any given bucket will contain any entries
				at all, the code is a bit complex.*/


		for (cindex = 0;cindex < CACHE_SIZE;cindex++)
		{
			 CachePtr = (Cache_entry *) l_next_macro( 
								(List_hdr *) objloc_cache[cindex]);
			 if (!l_ishead_macro(CachePtr))
			 {  /* cache entry found--make it the initial candidate */
				min = CachePtr->replace;        /* LRU counter value */
				MinPtr = CachePtr;
				break;
			 }
		} 

		if (cindex == CACHE_SIZE)
		{
				_pprintf("no entries in free pool or in cache\n");
				return ( (Cache_entry *) NULL);
		}

		/* Now search the entire cache for the least recently used entry. */

		for (; cindex < CACHE_SIZE; cindex++)
		{

				CachePtr = (Cache_entry *) 
						l_next_macro((List_hdr *) objloc_cache[cindex]);
				while (!l_ishead_macro(CachePtr))
				{
					if (CachePtr->replace < min)
					{   /* new candidate found */
						min = CachePtr->replace;
						MinPtr = CachePtr;
					}

					/* check the next one in the queue */
					CachePtr = (Cache_entry *) 
						l_next_macro((List_hdr *) CachePtr);
				}

		}

/* Now remove the chosen entry from its current position in the cache, so that
		it can be put into the new position.  */

		l_remove((List_hdr *) MinPtr);
		return(MinPtr); /* reuse this element */
}


/* name_hash_function() hashes a name to a number in a range specified by
		the choice of hashtype, allowing the function to be used for hashing
		for several purposes.  */

#define MULTHASH
int    name_hash_function (name,hashtype)
	Name           *name;
	char            hashtype;
{
#ifdef MULTHASH
	register unsigned int i , j ;
	register char * s ;

	j = 0 ;
	i = 1 ;
	s = (char *) name ;

	for ( ; *s ; i++ , s++ )
	{   /* add the ascii values of "name" */
		j += (*s) * i ; 
	} 

	switch (hashtype)
		{
				case HOME_NODE:
								return j % tw_num_nodes;
				case CACHE:     /* hash to cache */
								return j % CACHE_SIZE;
				case HOMELIST:
								return j % homeListSize;
				default:
								_pprintf("error in hash switch\n");
		}

#else
	register unsigned int j ;
	register char * s ;

	j = 0 ; 
	for ( s = name ; *s ; s++ ) 
	{
		j <<= 4 ;
		j += *s ;
		if ( ( j & 0xf000 ) != 0 ) 
		{
			j = j ^ ( j >> 12 ) ;
			j &= 0x0fff ;
		}
	}
	switch (hashtype)
		{
				case HOME_NODE:
								return ( j & 0x7fff ) % tw_num_nodes ;
				case CACHE:
								return ( j & 0x7fff ) % CACHE_SIZE ;
				case HOMELIST:
								return ( j & 0x7fff ) % homeListSize ;
				default:
								_pprintf("error in hash switch\n");
		}
#endif MULTHASH

return 0;

}


/* HomeInit() initializes a node's home list, ie the list of objects
   which call this node home. */

HomeInit()
{
   int i;

	/* If the number of nodes is equal to the number of buckets in the home 
		list, change the variable controlling the hashing to buckets.  
		Otherwise, all home list entries would appear in one bucket on each
		node, for that number of nodes.  */

	if ( tw_num_nodes != HOMELISTSIZE )
		homeListSize = HOMELISTSIZE;
	else
		homeListSize = HOMELISTSIZE - 1;

	for (i = 0;i < HOMELISTSIZE; i++)
	{

		HomeListHeader[i] = (struct HomeList_str *) l_hcreate();
	}

}

/* FindInHomeList () finds an object in the home list, if it is there.  It
		returns the object's node number, if found, and -1 otherwise. */

FUNCTION  struct HomeList_str *FindInHomeList(name,time)
		Name    *name;
		VTime   time;
{
	struct HomeList_str *ListElem;
	Int         bucket;

  Debug

	/* first determine which home list queue to search */
	bucket = name_hash_function (name,HOMELIST);

	for (ListElem = (struct HomeList_str *) 
						l_next_macro(HomeListHeader[bucket]);
		 !l_ishead_macro(ListElem);
		 ListElem = (struct HomeList_str *) l_next_macro(ListElem))
	{  /* do the comparison */
		if (namecmp(ListElem->name,name) == 0 &&
				leVTime (ListElem->phase_begin, time) &&
				ltVTime (time, ListElem->phase_end) )
			return(ListElem);   /* if found */
	}

	return ((struct HomeList_str *) NULL); /* not found */
}

/* AddToHomeList () adds a name to the home list. */

FUNCTION struct HomeList_str *AddToHomeList ( name,phase_begin,phase_end,node )
	Name        *name;
	VTime       phase_begin;
	VTime       phase_end;
	Int         node;
{
	struct HomeList_str *NewPtr;
	Int         bucket;

  Debug

	/* make a home list element */
	NewPtr =  (struct HomeList_str *) m_create (sizeof (struct HomeList_str), 
									    neginf, CRITICAL);

	entcpy ( NewPtr->name, name, sizeof (Name) );       /* copy the name */

	/* fill in the blanks */
	NewPtr->phase_begin = phase_begin;
	NewPtr->phase_end = phase_end;
	NewPtr->node = node;
	NewPtr->generation = 0;

	bucket = name_hash_function ( NewPtr->name, HOMELIST );

	/* put it in the proper queue */
	l_insert ( HomeListHeader[bucket], NewPtr );

	return ( NewPtr );
}

/* When an object is destroyed, and that destruction is committed, its home
		list entry should be removed.  (Unless, of course, the destroyed
		object is re-created or otherwise has active messages.)  Otherwise,
		requests to the home site for the object's location would be answered
		improperly.  In addition, the home site's cache entry for the object
		should be cleared.  
*/ 

FUNCTION RemoveFromHomeList(name,time)
	Name *name;
	VTime time;
{
	struct HomeList_str *ListElem;
	Int         bucket;

  Debug

	bucket = name_hash_function (name,HOMELIST);

	for (ListElem = (struct HomeList_str *) 
				l_next_macro(HomeListHeader[bucket]);
		 !l_ishead_macro(ListElem);
		 ListElem = (struct HomeList_str *) l_next_macro(ListElem))
	{
		if (namecmp(ListElem->name,name) == 0) 
		{
			break;
		}
	}

	if (!l_ishead_macro(ListElem))
	{
		l_remove((List_hdr *) ListElem);
		l_destroy((List_hdr *) ListElem);

		RemoveFromCache(name,time);
	}
	else
	{
		twerror("RemoveFromHomeList: no home list entry for %s\n",name);
		tester();
	}
}

/* RemoteRemoveFromHomeList() sends a message to another node, asking it
		to remove an object from its home list. 
*/

FUNCTION RemoteRemoveFromHomeList(name,time,node)
	Name *name;
	VTime time;
	Int node;
{
	Msgh * objlocmsg;
	HLmsg  * locmsg;

  Debug

	objlocmsg = sysbuf();
	locmsg = (HLmsg *) (objlocmsg+1);
	locmsg->sender = tw_node_num;
	if (node != -1)
	{
		locmsg->response = node;
	}
	else
	{
		locmsg->response = -1;
	}

	locmsg->time = time;
	/* A location of -1 means remove the object from the home list. */
	locmsg->newloc = -1;
	strcpy(locmsg->object,name);

	sysmsg(HOMECHANGE, objlocmsg, sizeof (HLmsg), node);
}


/* RemoteSplitHomeListEntry() requests a split of a home list entry at a 
		remote site.  It packages the request and sends a message to the
		home node, asking for that node to call ChangeHLEntry(), which
		will, in turn, call SplitHomeListEntry().
*/

FUNCTION RemoteSplitHomeListEntry(name,time,node)
	Name *name;
	VTime time;
	Int node;
{
	Msgh * objlocmsg;
	HLmsg  * locmsg;
	Int   there;

  Debug

	objlocmsg = sysbuf();
	locmsg = (HLmsg *) (objlocmsg+1);
	locmsg->sender = tw_node_num;
	locmsg->time = time;

	/* A location of -2 means split the home list entry as of the time in
		this message. */

	locmsg->newloc = -2;
	strcpy(locmsg->object,name);

	there = RemoveFromCache(name,time);
	if (!there)
	{
		/* There was no entry in the local cache for the split phase. 
				We may want to keep statistics on this, or not.  At any
				rate, it isn't an error. */
	}

	sysmsg(HOMECHANGE, objlocmsg, sizeof (HLmsg), node);
}

/* RemoteChangeHListEntry() sends a message to an object's home node, asking
		it to change the entry for one of the object's phase to indicate that
		the phase is on a new node.
*/

FUNCTION RemoteChangeHListEntry(name,time,HomeNode,node,generation)
	Name *name;
	VTime time;
	Int HomeNode;
	Int node;
	int generation;
{
	Msgh * objlocmsg;
	HLmsg  * locmsg;
	Int there;

  Debug

	objlocmsg = sysbuf();
	locmsg = (HLmsg *) (objlocmsg+1);
	locmsg->sender = tw_node_num;
	locmsg->time = time;
	locmsg->newloc = node;
	locmsg->generation = generation;
	strcpy(locmsg->object,name);

	there = RemoveFromCache(name,time);
	if (!there)
	{
		/* There was no entry in the local cache for the split phase. 
				We may want to keep statistics on this, or not.  At any
				rate, it isn't an error. */
	}

	sysmsg(HOMECHANGE, objlocmsg, sizeof (HLmsg), HomeNode);
}

/* SendCacheInvalidate() sends a system message to a node requesting that
		it remove a particular entry from its cache.  This routine is called
		when one node has reason to believe that another node has an invalid
		cache entry.
*/

FUNCTION SendCacheInvalidate(name,time,node)
	Name *name;
	VTime time;
	Int node;
{
	Msgh * objlocmsg;
	HLmsg  * locmsg;

  Debug

	objlocmsg = sysbuf();       /* grab a buffer, even if we have to wait */
	locmsg = (HLmsg *) (objlocmsg+1);

	/* create the message */
	locmsg->sender = tw_node_num;
	locmsg->time = time;
	locmsg->newloc = node;
	strcpy(locmsg->object,name);

	/* send it out as a system message */
	sysmsg(CACHEINVAL, objlocmsg, sizeof (HLmsg), node);
}

/* ChangeHLEntry() is called when a home list entry must be changed because
		an object has been moved to another node, or because its deletion
		has been committed.  (The latter is the case if the new location is
		-1.)  Also, if the object is being split, a message must be sent 
		detailing the split.  The new location is -2, in such a case.  The
		time field is set with the time of the split, with the understanding
		being that the existing home list entry will be found and divided in
		two.
*/
FUNCTION ChangeHLEntry(name,time,node,generation)
	Name *name;
	VTime time;
	Int node;
	int generation;
{
	struct HomeList_str *ListElem;
	Int there;

  Debug

	if (node == -1)
	{
		RemoveFromHomeList(name,time);
	}
	else
	{
		/* Try to find this phase in the home list.  If it's not there, then
				we may need to split the object.  If it is there, go ahead
				and change it.  If not, try to find a phase containing this
				time.  If there is one, split it. */

		struct HomeList_str     *ListElem;
		Int             bucket;

		bucket = name_hash_function (name,HOMELIST);

		for (ListElem = (struct HomeList_str *)
				l_next_macro(HomeListHeader[bucket]);
				!l_ishead_macro(ListElem);
				 ListElem = (struct HomeList_str *) l_next_macro(ListElem))
		{
				if (namecmp(ListElem->name,name) == 0 &&
						eqVTime (ListElem->phase_begin, time) )
				{
						break;
				}
		}

		if (l_ishead_macro(ListElem))
		{
				ListElem = FindInHomeList(name,time);

				if (ListElem == (struct HomeList_str *) NULL)
				{
						twerror("Phase(%s , %f) not in home list\n to change",
								name,time.simtime);
						tester();
				}
				else
				{
						ListElem = SplitHomeListEntry(ListElem,time);
#ifdef PARANOID
						if (ListElem == NULL)
						{
								twerror("Failed attempt to split home list entry for %s\n",name);
								tester();
						}
#endif PARANOID
						ListElem->generation = generation;
/*
						_pprintf("New phase %s, %f\n",ListElem->name,
								ListElem->phase_begin.simtime);
*/
				}

		}

		if (ListElem->generation > generation)
		{
				_pprintf("Old home list change rejected, obj %s, time %f, generation %d\n",
						name,time,generation);
				tester();
				return;
		}
		else
		{
				ListElem->node = node;
				ListElem->generation = generation;
		}
	}

	there = RemoveFromCache(name,time);
	if (!there)
	{
		/* There was no entry in the local cache for the split phase. 
				We may want to keep statistics on this, or not.  At any
				rate, it isn't an error. */
	}
}


/* SplitHomeListEntry() splits an existing home list entry into two parts, as
		the result of a temporal split of the existing phase of the object.
		The home list is searched for the phase that currently handles the
		time of the split.  A new home list entry is allocated for the new
		phase.  The old entry handles the earlier phase, the new entry handles 
		the later phase.  If the requested phase could not be found, the
		routine calls twerror().
*/

struct HomeList_str * SplitHomeListEntry ( ListElem, time )

	struct HomeList_str *ListElem;
	VTime time;
{
	Name * name;
	struct HomeList_str *NewElem;
	Int         there;

	name = (Name *) ListElem->name;

	there = RemoveFromCache(name,time);
	if (!there)
	{
		/* There was no entry in the local cache for the split phase. 
				We may want to keep statistics on this, or not.  At any
				rate, it isn't an error. */
	}

	NewElem = AddToHomeList(name,time,ListElem->phase_end,ListElem->node);
	ListElem->phase_end = time;

	return (NewElem);
}


/* ServiceHLRequest () is called whenever a HOMEASK message comes in.  The
		home node looks up the name of the requested object in its home node
		list, and extracts the object's host node.  It then allocates a system
		message, and sends it back to the requesting node.  */

FUNCTION ServiceHLRequest ( inmsg, Ctime )
	HLmsg  *inmsg;
	VTime  Ctime;
{
	Name        *name;
	Int         node;
	Msgh * objlocmsg;
	HLmsg  * locmsg;
	VTime  time;
	struct HomeList_str *HLPtr;

  Debug

	totHomeAsksRecv++;
	name = (Name *) inmsg -> object;
	time = inmsg-> time;
	HLPtr = FindInHomeList(name,time);

	if (HLPtr == (struct HomeList_str *) NULL)
	{
		/* Create the needed object. */

		node = ChooseNode();
		MakeObject(name,node,Ctime);
		HLPtr = AddToHomeList ( name, neginf, posinfPlus1, node );
	}

	node = HLPtr->node;

	objlocmsg = sysbuf();
	locmsg = (HLmsg *) (objlocmsg+1);
	locmsg->sender = tw_node_num;
	if (node != -1)
	{
		/* Either the entry should have been there, or the object should
				have been created, so this branch should always be taken. */

		locmsg->response = node;
		locmsg->phase_begin = HLPtr->phase_begin;
		locmsg->phase_end = HLPtr->phase_end;
	}
	else
	{
		twerror("Failure of object creation in ServiceHLRequest\n");
		locmsg->response = -1;
	}

	strcpy(locmsg->object,name);

	objlocmsg->sndtim = Ctime;
	objlocmsg->rcvtim = Ctime;

	sprintf ( objlocmsg->snder, "HOMEANS%d", tw_node_num );
	sprintf ( objlocmsg->rcver, "HOMEASK%d", inmsg->sender );

	totHomeAns++;
	sysmsg(HOMEANS, objlocmsg, sizeof (HLmsg), inmsg -> sender);
}

/* ObjectFound() handles incoming responses from home nodes.  These are 
		contained in HOMEANS messages.  ObjectFound() looks for the request 
		that caused the message, fulfills it, and updates the cache.
*/

FUNCTION ObjectFound(message)
	HLmsg           *message;   
{
	Pending_entry *request,*next;
	Name        *name;
	Cache_entry *position;
	VTime       phase_begin;
	VTime       phase_end;
	Ocb         *o;

  Debug

	totHomeAnsRecv++;
	name = (Name *) message->object;
	phase_begin = message->phase_begin;
	phase_end = message->phase_end;

	request = FindInPendingList(name,phase_begin,phase_end,
				(Pending_entry *) l_next_macro(PendingListHeader));

/* If there is no entry in the pending list, a previous response probably
		cleared it out.  Just ignore this request. */

	if (request != NULL)
	{
		Int CacheBucket;

/* If we are doing migration, in some cases we may be told that we have an
		object when we no longer do, or do not yet have it.  Therefore, here
		we must test for home node responses that indicate that the object
		is on this node. */

	if (message->response == tw_node_num)
	{
		o = FindInSchedQueue(name,phase_begin);
		if (o == (Ocb *) NULL)
		{
			/* We were told we had it, and we don't.  Let's ask again, in
				the hope of getting better information.  We don't really
				have to put a new entry in the pending list, but FindObject() 
				is going to do so, anyway, and it also contains all
				of the code to send the home ask message, so it's easier to
				reuse its code.*/

			FindObject(name,request->time,request->Message,request->restart,
				request->isMsg);

			/* Now remove the old request from the pending list, so we don't
				have two of them. */

			l_remove((List_hdr *) request);
			l_destroy((List_hdr *) request);

		   /* We can go no further, since the information that the home node
				gave us was bum, so return. */

			return;
		}
	}
	else
	{
		/* The home node gave us a plausible, off-node answer. */
		o = (Ocb *) NULL;
	}
		/* Put the new entry into the cache. */

		CacheBucket = name_hash_function(name,CACHE);

#ifdef PARANOID
		if (FindInCache(name,phase_begin,CacheBucket) != NULL)
		{
				_pprintf("About to make 2nd cache entry for %s, %f\n",
						name, phase_begin.simtime);
				tester();
		}
#endif
		position = CacheReplace(name,phase_begin,phase_end,message->response,
								o,CacheBucket);
	}

	/* Find all entries in the pending list that match this return value. */

	while (request != NULL)
	{
		/*Remove this entry from the pending list. */
		next = (Pending_entry *) l_next_macro(request);
		RemoveFromPendingList(request,&(position->cache_entry));

		request = FindInPendingList(name,phase_begin,phase_end,next);
	}
}


DumpHomeList ()
{
	struct HomeList_str *DumpPtr;
	Int i;

	_pprintf("Dumping home list\n");

	for (i = 0; i < homeListSize; i++)
	{
		if (!l_ishead_macro( l_next_macro (HomeListHeader[i])))
		{
			_pprintf("Bucket %d:\n",i);
		}
		else
			continue;

		for ( DumpPtr = (struct HomeList_str *) l_next_macro(HomeListHeader[i]);
				!l_ishead_macro(DumpPtr);
				DumpPtr =(struct HomeList_str *) l_next_macro(DumpPtr))
		{
			DumpHomeListEntry(DumpPtr);
		}
	}
}

DumpHomeListEntry ( DumpPtr )

	struct HomeList_str *DumpPtr;
{
	_pprintf("  Object %s, Phase Start %f, Phase End %f,Node %d,generation  = %d\n",
				DumpPtr->name,DumpPtr->phase_begin.simtime,
				DumpPtr->phase_end.simtime, DumpPtr->node, DumpPtr->generation);}

/* PendingListInit() initializes the list of requests pending responses from
		home sites.  */

FUNCTION PendingListInit()
{
	PendingListHeader = (Pending_entry *) l_hcreate();
}

/* FindInPendingList() finds a particular entry, by object name, in a node's
		list of requests pending responses from home sites.  It returns a
		pointer to the entry, if found, and a NULL pointer, otherwise. */

FUNCTION Pending_entry *FindInPendingList(name,phase_begin,phase_end,
									    StartSearch)
	Name        *name;
	VTime       phase_begin;
	VTime       phase_end;
	Pending_entry *StartSearch;
{
	Pending_entry       *current;

  Debug

	for (current = StartSearch; !l_ishead_macro(current); 
				current = (Pending_entry *) l_next_macro(current))
	{
		if (namecmp(name,current->object) == 0 &&
				leVTime (phase_begin, current->time) &&
				ltVTime (current->time, phase_end) )
		{
			break;
		}
	}   

	if (current == PendingListHeader)
	{
		return( (Pending_entry *) NULL);
	}
	else
	{
		return (current);
	}
}


/* AddToPendingList() puts a new element at the end of the current pending
		list. */

FUNCTION AddToPendingList(element)
	Pending_entry *element;
{
	Pending_entry       *position;

  Debug

	position = (Pending_entry *) l_prev((List_hdr *) PendingListHeader);
	l_insert((List_hdr *) position, (List_hdr *) element);
	pendingEntries++;
}

FUNCTION RemoveFromPendingList(element,locptr)
	Pending_entry *element;
	Objloc *locptr;
{
  Debug

	/* Call the function that resumes the job that was being
		done when the home site request was generated. */

	l_remove((List_hdr *) element);

	(*(element->restart)) (element->Message,locptr);

	l_destroy((List_hdr *) element);

	pendingEntries--;
}

/*  Find the minimum time in the pending action queue */

VTime FUNCTION MinPendingList ()
{
	VTime min;
	VTime thisTime;
	Pending_entry * pending;

  Debug

	min = posinfPlus1;  /* init to the max */

	for ( pending = (Pending_entry *) l_next_macro ( PendingListHeader );
				  ! l_ishead_macro ( pending );
		  pending = (Pending_entry *) l_next_macro ( pending ) )
	{   /* use a message's send time */
		thisTime = pending->isMsg ? pending->Message->sndtim : pending->time;
		if ( gtVTime ( min, thisTime) )
			min = thisTime;     /* found an earlier time */
	}

	return ( min );
}


/* FindInSchedQueue() looks for an object by name and time in the local
		scheduler queue.  It returns an ocb pointer, if found, and a NULL
		pointer, otherwise. */

FUNCTION Ocb *FindInSchedQueue(name,time)
	Name *name;
	VTime time;
{
	Ocb *o;

  Debug

	for (o = fstocb_macro; o; o = nxtocb_macro (o))
	{  /* look in prqhd */
		if (namecmp(name,o->name) == 0 &&
				leVTime (o->phase_begin, time) &&
				ltVTime (time, o->phase_end) )
			return o;
	}
	return ((Ocb *) NULL);
}

/* DumpPendingList() dumps the pending list for examination. */

DumpPendingList()
{
	Pending_entry *DumpPtr;

	_pprintf("Dumping pending list\n");

	for(DumpPtr = (Pending_entry *) l_next_macro(PendingListHeader); 
		!l_ishead_macro(DumpPtr);
		DumpPtr = (Pending_entry *) l_next_macro(DumpPtr))
	{
		_pprintf("Object %s, Time %f, %d, %d\n",DumpPtr->object,
			DumpPtr->time.simtime,DumpPtr->time.sequence1,
			DumpPtr->time.sequence2);
	}
}

/*  CacheInit() is called once at the beginning of a run to initialize the
		object location cache.  It zeroes all cache replacement fields and 
		nulls out the cache entry pointers.  In addition, it initializes the 
		replacement counter (a monotonically increasing counter used to assign 
		cache entries a LRU status) and zeroes the number of cache hits and 
		misses that have occurred.  */

FUNCTION CacheInit()
{
		Int cindex,poolsize;
		Cache_entry *buf;

  Debug

/* Create a queue header for each bucket in the cache table. */

		for (cindex = 0; cindex < CACHE_SIZE; cindex++)
		{
				objloc_cache[cindex] = (Cache_entry *) l_hcreate ();
		}

/* Create a header for the pool of unused cache entries. */

		CacheFreePool= (Cache_entry *) l_hcreate ();

/* Create a pool of unused cache entries. */

		for (poolsize = 0; poolsize < CACHE_POOL; poolsize++)
		{

				buf =  (Cache_entry *) m_create ( sizeof (Cache_entry), 
								neginf, CRITICAL );
				l_insert ((List_hdr *) CacheFreePool, (List_hdr *) buf);
		}
		rcount = 0;
		chit = 0;
		cmiss = 0;
}

DumpCache ()
{
	Cache_entry  * DumpPtr ;
	Int i;

	_pprintf("Dumping cache\n");

	for (i = 0; i < CACHE_SIZE; i++)
	{
		DumpPtr = (Cache_entry *) l_next_macro(objloc_cache[i]);
		if (!l_ishead_macro(DumpPtr))
				_pprintf("Bucket %d:\n",i);
		else
				continue;
		for(;!l_ishead_macro(DumpPtr); 
				DumpPtr = (Cache_entry *) l_next_macro(DumpPtr))
		{
			_pprintf("  Object %s,Phase Start %f Phase End %f, Node %d, Ocb Ptr %d, Replace %d\n",
				DumpPtr->cache_entry.name, DumpPtr->cache_entry.phase_begin.simtime, 
				DumpPtr->cache_entry.phase_end.simtime, DumpPtr->cache_entry.node, 
				DumpPtr->cache_entry.po, DumpPtr->replace);
		}
	}

}

FUNCTION ShowCacheEntry(name)
	Name        *name;
{
	Int bucket;
	register Cache_entry  * position ;

	bucket = name_hash_function(name,CACHE);

	if ( objloc_cache[bucket] )
	{
		for (position=(Cache_entry *) l_next_macro(objloc_cache[bucket]);
				!l_ishead_macro (position);
				position =  (Cache_entry *) l_next_macro (position))
		{
				if (namecmp (position->cache_entry.name, name) == 0)
				{
			_pprintf("  Object %s,Phase Start %f Phase End %f, Node %d, Ocb Ptr %d, Replace %d\n",
				position->cache_entry.name, position->cache_entry.phase_begin.simtime,
				position->cache_entry.phase_end.simtime, position->cache_entry.node,
				position->cache_entry.po, position->replace);
				}
		}
	}
}

ShowHomeListEntry(name)
	Name        *name;
{
	struct HomeList_str *DumpPtr;
	int bucket;

	bucket = name_hash_function (name,HOMELIST);

	if (HomeListHeader[bucket])
	{
		for (DumpPtr = (struct HomeList_str *) l_next_macro(HomeListHeader[bucket]);
				!l_ishead_macro(DumpPtr);
				DumpPtr = (struct HomeList_str *) l_next_macro(DumpPtr))
		{
			if (namecmp (DumpPtr->name, name) == 0)
				DumpHomeListEntry(DumpPtr);
		}
	}     
}

/* NullRestart() is just a temporary function used to pass to the object
		finding routine to test the code used to handle replies to home
		node requests.  It does nothing. */

NullRestart(message)
	Msgh        *message;
{
}

ChooseNode()
{
	return (tw_node_num); /* put the object on this node */
}


FUNCTION Ocb * MakeObject(name,node,time)
	Name        *name;
	Int         node;
	VTime       time;
{
  Debug

	if (node == tw_node_num)
	{  /* create object on this node */
		return (createproc(name,"NULL",time));
	}
	else
	{
		_pprintf("remote create not yet implemented; object %s, node %d\n",
						name,node);
		return ((Ocb *) NULL);
	}

}

/*

#define TRACE_MODE
#define ASCII_ADD_HASH_FUNC
#define PJW_HASH_FUNC

#define ASCII_SCRAMBLE_HASH_FUNC
*/


/* This function isn't used, but we may want the hashing algorithms described
		here for later use.  (PLR)
*/

#define HASH_TABLE_SIZE 0

int    wm_hash_function (name)
	Name           *name;
{

#ifdef ASCII_ADD_HASH_FUNC

	register unsigned int    i; 
	register char  *s; 

	for (i = 0, s = name; *s; s++) 
	{ 
		i += *s;
	}

	return i % HASH_TABLE_SIZE ;

#endif

#ifdef ASCII_SCRAMBLE_HASH_FUNC

	register unsigned int i , j ;
	register char * s ;

	j = 0 ;
	i = 1 ;
	s = name ;

	for ( ; *s ; i++ , s++ )
	{
		j += (*s) * i ; 
	}

	return j % HASH_TABLE_SIZE ;

#endif

#ifdef PJW_HASH_FUNC

	register unsigned int j ;
	register char * s ;

	j = 0 ; 
	for ( s = name ; *s ; s++ ) 
	{
		j <<= 4 ;
		j += *s ;
		if ( ( j & 0xf000 ) != 0 ) 
		{
			j = j ^ ( j >> 12 ) ;
			j &= 0x0fff ;
		}
	}

	return ( j & 0x7fff ) % HASH_TABLE_SIZE ;

#endif
}
