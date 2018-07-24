
/************************************************************************
*
*    Object Type Table element format
*
*    - this format describes the entries in the object type
*    -     table.
*    - the name field of the entry is the name referenced in the
*    - object create function call. this is not the created
*    - objects name, but allows the simulator to get the entry points
*    - (function pointers) for the object's init, event and query 
*    - code sections as well as the object's state size.
*    - (one created object may have the same name as its type)
*
*    JJW 8/3/87  Modify for termination section
*
*************************************************************************/

typedef int (*Pfi) ();

typedef struct 
{
    char*   	   name;  /* object type name                     */
    Pfi            init;  /* object initialize module entry point */
    Pfi            event; /* object event module entry point      */
    Pfi            query; /* object query module entry point      */
    Pfi            term;  /* object termination entry point       */
    int            statesize; /* sizeof object's state            */
}
ObjectType;
