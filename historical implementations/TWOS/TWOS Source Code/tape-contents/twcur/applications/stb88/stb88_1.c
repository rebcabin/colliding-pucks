/* ******************************** */
/*     Portability definitions.     */
/* ******************************** */
#include "twcommon.h"

/************************************************************************
*
*	Object Type Table definition (compiled once - included in
*		executable module linkage - once compiled no recompile is
*		necessary unless function or object type names are
*	 	changed)
*
*	- the name field of the entry is the name referenced in the
*	- object create function call. this is not the created
*	- objects name, but allows the simulator to get the entry points
*	- (function pointers) for the object's init, event and query 
*	- code sections as well as the object's state size.
*	- (one created object may have the same name as its type)
*
*	- the function pointers are supplied at link time and the state
*	- sizes are supplied when an object is created through the
*	- obcreate call. (init code section is executed, which should
*	- have an init_State function call, which indirectly calls
*	- time warp simulator memory allocation routine, which saves
*	- state pointers. State size was passed in call)
*
*************************************************************************/

extern ObjectType divType, gridType, corpsType, distribType, initType,
                  statsType;

ObjectType *objectTypeList[] =
{
&divType, &gridType, &corpsType, &distribType, &initType, &statsType, 0
};


