/*      Copyright (C) 1989, 1991, California Institute of Technology.
		U. S. Government Sponsorship under NASA Contract NAS7-918
		is acknowledged.        */

/*
 * $Log:	services.c,v $
 * Revision 1.5  91/11/01  12:36:56  pls
 * 1.  Change ifdef's and version id.
 * 2.  Add type_Area() function.
 * 
 * Revision 1.4  91/07/17  15:12:27  judy
 * New copyright notice.
 * 
 * Revision 1.3  91/06/03  12:26:43  configtw
 * Tab conversion.
 * 
 * Revision 1.2  91/03/26  09:41:47  pls
 * 1.  Add type_myArea() and type_malloc() routines for type init.
 * 2.  Add obj_getLibPointer(), obj_setLibPointer() and
 *     obj_getLibTable() routines for library support.
 * 3.  Add Steve's RBC code.
 * 
 * Revision 1.1  90/08/07  15:41:01  configtw
 * Initial revision
 * 
*/
char services_id [] = "@(#)services.c   $Revision: 1.5 $\t$Date: 91/11/01 12:36:56 $\tTIMEWARP";


/*
Purpose:

		services.c contains a number of entry points into the operating
		system.  Unlike the entry points in serve.c, these entry points
		do not cause a trap into the operating system.  The implication
		of this is that calls to routines in this module will not
		cause an object to stop executing in favor of a different
		object.  All of these calls are fairly simple, and generally
		only return information, rather than do work that changes
		any aspect of the system.

Functions:

Implementation:

*/

#include "twcommon.h"
#include "twsys.h"

int     initing_type = FALSE;

VTime obj_now ()
{
	return ( xqting_ocb->svt );
}

VTime IncSimTime ( incr )

	double incr;
{
	VTime next;

	next = xqting_ocb->svt;

	next.simtime += incr;

	return ( next );
}

VTime IncSequence1 ( incr )

	Ulong incr;
{
	VTime next;

	next = xqting_ocb->svt;

	next.sequence1 += incr;

	return ( next );
}

VTime IncSequence2 ( incr )

	Ulong incr;
{
	VTime next;

	next = xqting_ocb->svt;

	next.sequence2 += incr;

	return ( next );
}

void * type_malloc(size)

	int size;
	
{   /* m_allocate memory for the type */
	if (initing_type)
		return ((void *)(m_allocate(size)));
	else
		twerror("Illegal call to typeMalloc");
		return(NULL);
}

void * type_Area(type)

	Type	type;
{
	Typtbl	*typepointer;

	typepointer = find_object_type(type);
	if (typepointer == NULL)
		{	/* bad type */
		twerror("typeArea: type not found: %s",type);
		tester();
		}
	return(typepointer->typeArea);
}

void * type_myArea()
{
	return (xqting_ocb->typepointer->typeArea);
}

Pointer obj_getLibPointer()
{
	return (xqting_ocb->libPointer);
}

void *  obj_getLibTable()
{
	return (xqting_ocb->typepointer->libTable);
}

void    obj_setLibPointer(ptr)
	Pointer     ptr;
{
	xqting_ocb->libPointer = ptr;
}

void * obj_myState ()
{
#if RBC
	if ( xqting_ocb->uses_rbc )
	{
		return ( (void *)(xqting_ocb->footer) );
	}
	else
#endif
	return ( (void *)(xqting_ocb->sb + 1) );
}

char * obj_myName ( name )

	char * name;
{
	strcpy ( name, xqting_ocb->name );

	return ( name );
}

int obj_numMsgs ()
{
	return ( xqting_ocb->ecount );
}

void * msgText ( n )

	int n;
{
	return ( (void *) (*(xqting_ocb->msgv + n) + 1) );
}

Long msgSelector ( n )

	int n;
{
	return ( (*(xqting_ocb->msgv + n))->selector );
}

int msgLength ( n )

	int n;
{
	return ( (*(xqting_ocb->msgv + n))->txtlen );
}

char * msgSender ( n )

	int n;
{
	return ( (*(xqting_ocb->msgv + n))->snder );
}

VTime msgSendTime ( n )

	int n;
{
	return ( (*(xqting_ocb->msgv + n))->sndtim );
}

int tw_packetLen ()
{
	return ( pktlen );
}
