/*  	Copyright (C) 1989, 1991, California Institute of Technology.
		U.S. Government Sponsorship under NASA Contract NAS7-918
		is acknowledged.        */

#include "twcommon.h"

#ifdef SIMULATOR
#include "tws.h"
#define m_allocate sim_malloc
#else
#include "twsys.h"
#endif

#include "twusrlib.h"
#include "outList.h"


/**** Uncomment this for debugging
#define TWULIB_DEBUG
*******************************/



extern packageType * libPackageTable[];


/* This is the definition for an element in the library
 * state offset table.  This table stores the package instance
 * pointer along with the state offset.  A package's state
 * offset is the number of bytes from the state of package
 * state memory that an individual package's state can be found.
 */
typedef struct LibStatePtrEntry
   {
   packageType * package;
   int stateOffset;
   } LibStatePtrEntry;


/* This is the definition for the object library table.  This
 * structure contains pointers to all of the tables necessary
 * for library use for each object type.  This will be placed
 * at the start of the same dynamically allocated block of memory 
 * that holds the library function tables and the library state
 * offset table.
 */
typedef struct ObjectLibraryTable
   {
   LibFncPtr * evtTable;
   LibFncPtr * tellTable;
   LibStatePtrEntry * stateTable;
   LibFncPtr init;
   LibFncPtr term;
   int stateSize;
   } ObjectLibraryTable;   
 

/* This structure is just used to organize data used by the
 * library table allocation and building functions.  Its
 * purpose is for code readability.
 */
typedef struct LibFncCounts
   {
   int numSoe, numEoe, numTell, numEvt, numPackages;
   } LibFncCounts;


/* If the libBits flag is non-null then we must allocate and build the
 * function tables and the state offset table.  These tables
 * are stable (once setup they don't change) and only one table
 * per object type per node is needed. 
 *  
 * These tables are placed in a dynamically allocated piece of
 * memory.  This memory is pointed to by the object type as
 * specified in the TW object type table through the libTable
 * field (see twsys.h).  The tw parameter to this function is
 * a pointer to the type table element for this object type.
 *  
 * The exact packages to be included are specified by the object
 * type through the libBits flag.  Each bit in this flag
 * represents a library package type to be included in this object
 * type.  The details of each package can be found in the
 * libPackageTable (see twpackages.c).  This table is a list
 * of packageType pointers that are currently available.  Each
 * packageType is defined within the modules that make up the
 * package.
 */    
void * twulib_init_type(tw, ob)
   Typtbl * tw;
   ObjectType * ob;
{
   LibFncCounts libCounts;
   ObjectLibraryTable * libTable, * allocate_libTable();
   void build_libTable();
   int twulib_init_handler(); 
   int twulib_event_handler(); 
   int twulib_term_handler();

#ifdef TWULIB_DEBUG
   printM_packageTable();
#endif

   /* First check if the libBits flag in the object type definition
    * is 0.  If so then this object doesn't use the library and
    * nothing need be done.
    */
   if (ob->libBits == 0)
      return NULL;

   
   /* Calculate and allocate the space needed for the tables.
    * This function returns a pointer to the dynamically allocated
    * package tables (event, tell, and state offset).  It takes
    * a pointer to the object definition and a pointer to the libCounts
    * structure.  This structure will contain information essential
    * for building the tables.
    */
   libTable = allocate_libTable(ob, &libCounts);


   /* This function builds the three package tables.  It uses the
    * information in the libCounts structure to build the event
    * fnc table, the tell fnc table, and the state offset stable.
    */
   build_libTable(ob, libTable, &libCounts);


   /* Save the init and term function pointers.
    */
   libTable->init = tw->init;
   libTable->term = tw->term;

#ifdef TWULIB_DEBUG
   printM_libTable(libTable);
#endif
   
   /* Now change the type table so that the library handler
    * is called instead of the appropriate section of the object.
    */   
   tw->init = twulib_init_handler;
   tw->event = twulib_event_handler;
   tw->term = twulib_term_handler;

   return (void *)libTable;
}


ObjectLibraryTable * allocate_libTable(ob, lc)
   ObjectType * ob;
   LibFncCounts * lc;
{
   packageType ** packageList = libPackageTable;
   packageType * package;
   Ulong libCode;
   int spaceNeeded;
   ObjectLibraryTable * libTable;

   clear(lc, sizeof(LibFncCounts));
        

   /* Each bit cooresponds to the package pointer's position
    * in the libPackageTable.  First we must determine the number
    * of packages being used, and how many use each type of
    * function supported by the library (start of event, end of
    * event, and tell).  Note that special function tables for
    * the init and term functions are not built.  Since these
    * are run only once, they are checked through the state offset
    * table.
    */
   for (libCode = ob->libBits; libCode != 0; libCode >>= 1, packageList++)
      {
      if (libCode & 1)
         {
         package = *packageList;

         /* Count the start of event, end of event, and tell functions
          * used by all packages included in this object type.  These
          * will be used to calculate the size of the function tables.
          * The number of packages included in counted to calculate
          * the size of the state offset table.
          */
         if (package->libsoe != NULL) (lc->numSoe)++;
         if (package->libeoe != NULL) (lc->numEoe)++;
         if (package->libtell != NULL) (lc->numTell)++;
         (lc->numPackages)++;
         } /* end if libCode & 1 */
    
      } /* end for libCode . . . */
 
 
   /* The number of entries in the event table is the number
    * of soe fncs + the number of eoe fncs + the event section
    * pointer + a NULL terminator.
    * Also add one to the number of tells for its NULL terminator.
    * Also add one to the number of packages for the state offset
    * table terminator.
    */
   lc->numEvt = lc->numSoe + lc->numEoe + 2;
   (lc->numTell)++;
   (lc->numPackages)++;

   /* Now allocate the table space and build the tables.
    */
   spaceNeeded = (lc->numEvt + lc->numTell) * sizeof(LibFncPtr) +
       lc->numPackages * sizeof(LibStatePtrEntry) + sizeof(ObjectLibraryTable);

   libTable = (ObjectLibraryTable *)m_allocate(spaceNeeded);                  

   
#ifdef TWULIB_DEBUG
   printM_table_sizes(lc, spaceNeeded);
#endif


   /* The event function table comes directly after the library table
    * structure.  Then comes the tell function table, and finally the 
    * state offset table.
    */
   libTable->evtTable = (LibFncPtr *)(libTable + 1);
   libTable->tellTable = libTable->evtTable + lc->numEvt;
   libTable->stateTable = 
                  (LibStatePtrEntry *)(libTable->tellTable + lc->numTell);

#ifdef TWULIB_DEBUG
   printM_libTable(libTable);
#endif

   return libTable;
} 




/* The function tables are built by first putting pointers to the included
 * packages into the space where the function tables will go.
 * These tables are then sorted by the appropriate priority (the
 * prio is accessed through the package pointer).  Finally, the
 * package pointers are replaced by the appropriate library function.
 *
 * Note that the state offset table includes all packages used
 * by the object type.
 */
void build_libTable(ob, libTable, lc)
   ObjectType * ob;
   ObjectLibraryTable * libTable;
   LibFncCounts * lc;
{
   packageType ** packageList = libPackageTable;
   packageType * package;
   int tellIndex, soeIndex, eoeIndex, packageIndex;
   Ulong libCode;
   int i, soe_prioCmp(), eoe_prioCmp(), tell_prioCmp();
   void sort_fncTable();
   
   LibStatePtrEntry * e;


   /* First we copy the package definition pointers into the tables
    * in an unsorted manner.  We will sort them next.
    */
   tellIndex = soeIndex = packageIndex = 0;
   eoeIndex = lc->numSoe + 1; /* +1 to make room for the evt section ptr */
   libTable->stateSize = 0;
 
   for (libCode = ob->libBits; libCode > 0; libCode >>= 1, packageList++)
      {
      if ( libCode & 1 )
         {
         package = *packageList;

         if (package->libsoe != NULL)
            libTable->evtTable[soeIndex++] = (LibFncPtr)package;
         if (package->libeoe != NULL)
            libTable->evtTable[eoeIndex++] = (LibFncPtr)package;
         if (package->libtell != NULL)
            libTable->tellTable[tellIndex++] = (LibFncPtr)package;

         libTable->stateTable[packageIndex].package = package;  
         libTable->stateTable[packageIndex++].stateOffset= libTable->stateSize;
         libTable->stateSize += package->stateSize;
         }
      }
 

   /* Now sort each table by the appropriate priority.
    * The comparison function is passed as a parameter.
    * Note that lc->numSoe and lc->numEoe have the exact
    * number of soe and eoe fncs included.  However, the lc->numTell
    * has been incremented by one (to account for the NULL sentinel).
    * Thus, to pass the correct number of tell fncs to be sorted
    * we must deduct one from lc->numTell (yes, I did discover this
    * during debugging!).
    */
   sort_fncTable(libTable->evtTable, lc->numSoe, soe_prioCmp);
   sort_fncTable(libTable->evtTable + lc->numSoe + 1, lc->numEoe, eoe_prioCmp);
   sort_fncTable(libTable->tellTable, lc->numTell-1, tell_prioCmp);
 

   /* Now replace the package pointers by the actual function ptrs.
    */
   for (i=0; i<lc->numSoe; i++)
      libTable->evtTable[i] = ((packageType *)(libTable->evtTable[i]))->libsoe;
   for (i = lc->numSoe+1; i<lc->numEvt-1; i++)
      libTable->evtTable[i] = ((packageType *)(libTable->evtTable[i]))->libeoe;
   for (i=0; i<lc->numTell-1; i++)
      libTable->tellTable[i]= 
                        ((packageType *)(libTable->tellTable[i]))->libtell;

                                                                                   /* Now put in the NULL terminators and the event section pointer.
    */
   libTable->evtTable[lc->numEvt-1] = NULL;
   libTable->tellTable[lc->numTell-1] = NULL;
   libTable->evtTable[lc->numSoe] = (LibFncPtr)ob->event;
   libTable->stateTable[lc->numPackages-1].package = NULL;


#ifdef TWULIB_DEBUG
   printM_libTable_contents(libTable, "Library Tables");
#endif
}
 


void sort_fncTable(table, num, comp_fnc)
   LibFncPtr * table;
   int num;
   LibFncPtr comp_fnc;
{
   int i, j;
   LibFncPtr tmp;

   for (i=0; i<num-1; i++)
      for (j=i+1; j<num; j++)
         {
         if ( comp_fnc((packageType *)(table[j]), (packageType *)(table[i])) )
            {
            tmp = table[j];
            table[j] = table[i];
            table[i] = tmp;
            }
         }
}   

int soe_prioCmp(p1, p2)
   packageType * p1, * p2;
{   
   if ( p1->soePrio < p2->soePrio ) return 1;
   return 0;
}

int eoe_prioCmp(p1, p2)
   packageType * p1, * p2;
{   
   if ( p1->eoePrio < p2->eoePrio ) return 1;
   return 0;
}

int tell_prioCmp(p1, p2)
   packageType * p1, * p2;
{   
   if ( p1->tellPrio < p2->tellPrio ) return 1;
   return 0;
}



/************************************************************/

int twulib_init_handler()
{
   ObjectLibraryTable * libTable;
   LibStatePtrEntry * stateTable;
   Pointer stateMemory;

   libTable = (ObjectLibraryTable *)getLibTable;
   stateTable = libTable->stateTable;

   
#ifdef TWULIB_DEBUG
   printM_libTable(libTable);
#endif

   /* First we must allocate the state needed by the library
    * packages.  This is a contiguous block of memory kept
    * through the TW dynamic memory.  Macros have been set
    * up to allow easy access by the library to this state
    * memory.  The library state pointer table keeps track
    * of each package's state offset from the beginning of
    * the block of memory.
    */
   stateMemory = newBlockPtr(libTable->stateSize);
   if (stateMemory == 0)
      userError("TWULIB ERROR:  Can't allocate state memory in init_handler");

   setLibPointer(stateMemory);


   /* Now call all of the package init sections.
    */
   for ( ; stateTable->package != NULL; stateTable++)
      if (stateTable->package->libinit != NULL)
         stateTable->package->libinit();

   /* Now call the object init section.
    */
   libTable->init();
}
      
/************************************************************/

int twulib_term_handler()
{
   ObjectLibraryTable * libTable;
   LibStatePtrEntry * stateTable;

   libTable = (ObjectLibraryTable *)getLibTable;   
   stateTable = libTable->stateTable;


   /* Now call all of the package term sections.
    */
   for ( ; stateTable->package != NULL; stateTable++)
      if (stateTable->package->libterm != NULL)
         stateTable->package->libterm();

   /* Now call the object term section.
    */
   if (libTable->term != NULL)
      libTable->term();
}


/************************************************************/

int twulib_event_handler()
{
   ObjectLibraryTable * libTable;
   LibFncPtr * evtTable;
   
   libTable = (ObjectLibraryTable *)getLibTable;   
   evtTable   = libTable->evtTable;

#ifdef TWULIB_DEBUG
   printM_libTable_contents(libTable, "Library Tables");
#endif

   for (; *evtTable != NULL; evtTable++)
      (*evtTable)();
}
 

/************************************************************/

int twulib_tell_handler(rec, recTime, sel, len, txt)
   char * rec;
   VTime recTime;
   long sel;
   int len;
   char * txt;
{
   ObjectLibraryTable * libTable;
   LibFncPtr * tellTable;
   OutListType outList;
   OutMsgType msg, * msgPtr;
   int size, i;

	
	init_outList(&outList);
	add_outListMsg(&outList, rec, recTime, sel, len, txt, NULL);
	
   
   libTable = (ObjectLibraryTable *)getLibTable;   
   tellTable   = libTable->tellTable;
   
   for (; *tellTable != NULL; tellTable++)
      (*tellTable)(&outList);


	send_outList(&outList);
	dispose_outList(&outList);
}
 

/************************************************************/


void * myPState(package)
   packageType * package;
{
   ObjectLibraryTable * libTable;
   LibStatePtrEntry * stateTable;

   libTable = (ObjectLibraryTable *)getLibTable;   
   stateTable = libTable->stateTable; 

   for ( ; stateTable->package != NULL; stateTable++)
      if ( package == stateTable->package )
         return (char *)pointerPtr(getLibPointer) + stateTable->stateOffset;

   userError("TWULIB ERROR:  Can't find package type in myPState.");
}


#ifdef TWULIB_DEBUG

printM_table_sizes(lc, space)
   LibFncCounts * lc;
   int space;
{
   printf("*******************************************\n");
   printf("** Space needed for tables = %d bytes.\n", space);
   printf("** Number of soe functions = %d.\n", lc->numSoe);
   printf("** Number of eoe functions = %d.\n", lc->numEoe);
   printf("** Number of tell functions = %d.\n", lc->numTell);
   printf("** Number of evt functions = %d.\n", lc->numEvt);
   printf("** Number of packages = %d.\n", lc->numPackages);
   printf("*******************************************\n");
}


printM_libTable(l)
   ObjectLibraryTable * l;
{
   printf("*******************************************\n");
   printf("**Object library table %d:\n", (int)l);
   printf("\t Event table at location %d.\n", (int)l->evtTable);
   printf("\t Tell table at location %d.\n", (int)l->tellTable);
   printf("\t State table at location %d.\n", (int)l->stateTable);
   printf("\t Init function is %d.\n", (int)l->init);
   printf("\t Term function is %d.\n", (int)l->term);
   printf("\t State size is %d.\n", l->stateSize);
   printf("*******************************************\n");
}


printM_libTable_contents(l, s)
   ObjectLibraryTable * l;
   char * s;
{
   LibFncPtr * f;
   LibStatePtrEntry * e;

   printf("*******************************************\n");
   printf("**%s:\n", s);

   printf("\t Event table\n");
   for (f = l->evtTable; *f != NULL; f++)
      printf("\t\t %d\n", (int)(*f));

   printf("\t Tell table\n");
   for (f = l->tellTable; *f != NULL; f++)
      printf("\t\t %d\n", (int)(*f));

   printf("\t State table\n");
   for (e = l->stateTable; e->package!=NULL; e++)
      printf("\t\t Package %d, offset %d\n", (int)e->package, e->stateOffset);
   printf("*******************************************\n");
}
   

printM_packageTable()
{
   int i;
   packageType * p;

   printf("*******************************************\n");
   printf("** Library Packages Available:\n");
   for (i=0; libPackageTable[i]!=NULL; i++)
      {
      p = libPackageTable[i];

      printf("\t Package %d:\n", (int)(p));
      printf("\t\t Libinit = %d\n", (int)(p->libinit));
      printf("\t\t Libterm = %d\n", (int)(p->libterm));
      printf("\t\t Libsoe = %d\n", (int)(p->libsoe));
      printf("\t\t LibsoePrio = %d\n", p->soePrio);
      printf("\t\t Libeoe = %d\n", (int)(p->libeoe));
      printf("\t\t LibeoePrio = %d\n", p->eoePrio);
      printf("\t\t Libtell = %d\n", (int)(p->libtell));
      printf("\t\t LibtellPrio = %d\n", p->tellPrio);
      printf("\t\t State Size = %d\n", p->stateSize);
   
      printf("-------------------------------------------\n");
      }

   printf("*******************************************\n");
}

#endif  /* End of debug functions */
