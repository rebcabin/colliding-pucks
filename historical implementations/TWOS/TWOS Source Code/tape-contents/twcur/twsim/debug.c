

/*debug functions for TWSIM */
/* if you are running the debugger you can turn on the internal TWSIM
debugging by assigning the variable debug_switch to 1. Also change
the step_switch if desired. */


#include "twcommon.h"
#include "tws.h"
#include "twsd.h"
#include <stdio.h>

/* The following function assumes that the global variables which
index the object currently running have not been injured.  Note in
particular, calls to newObj and delObj do change these variables and
restore them at the exit to the function */

prnt_data()
{
    char *strng[100];
    
    sprintVTime(strng, emq_current_ptr->rlvt);
    fprintf(stderr,"current time: %s\n",strng);
    fprintf(stderr,"obj name: %s\n",obj_hdr[gl_hdr_ind].name);
    fprintf(stderr,"no of events: %d\n",obj_bod[gl_bod_ind].num_events);
    fprintf(stderr, "gl_hdr: %d, gl_bod: %d, hdr: %d, bod: %d, type: %d\n",
gl_hdr_ind,gl_bod_ind,hdr_ind,bod_ind,type_ind);

    return;
}

prnt_allocdata()
{
    int idxx;
    Pointer ptrs;

    ptrs =  obj_bod[gl_bod_ind].memry_pointers_ptr;
    fprintf(stderr,"  memry_pointers_ptr: %x\n", ptrs);

    if ( ptrs != 0)
    for ( idxx = 0; idxx < 55;  idxx++)
	{
	if (obj_bod[gl_bod_ind].memry_pointers_ptr[idxx] != 0)
        fprintf(stderr,"  array idx: %d  pointer: %d\n",
	   idxx,obj_bod[gl_bod_ind].memry_pointers_ptr[idxx]);
	}
    else
        fprintf(stderr,"blockptrs array - null pointer!!\n");
    return;
}






