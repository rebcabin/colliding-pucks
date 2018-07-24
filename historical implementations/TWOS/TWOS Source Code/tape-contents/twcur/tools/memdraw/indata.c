#include "mplot.h"

#include <stdio.h>
#include <math.h>
#include <fcntl.h>

#include <X11/Xlib.h>

extern Display * x_display;
extern Window x_window;
extern GC x_GC;

MEM_ENTRY mem_entry[MAX_ITEMS];

#define MAX_NOBJECTS 200

OBJ_NAME_ENTRY obj_names[MAX_NOBJECTS];
int nobj_names;


comp_names (i, j)
	OBJ_NAME_ENTRY *i, *j;
{
	return strcmp ( i->name, j->name );
}


read_data ( flowname )

	char * flowname;
{
	char filename[50];
	FILE * fp;
	register int i, j;
	int temp1, temp2, temp3, temp4, temp5, n;

	strcpy ( filename, flowname );
	strcat ( filename, "memdump" );

	fp = fopen ( filename, "r" );

	if ( !fp )
	{
		fprintf ( stderr, "can't open %s\n", filename );
		exit ( 0 );
	}

	/* Read in object names */
	i = -1;
	do
	{
		i++;
		fscanf( fp, "%i %s", & (obj_names[i].objno ),
					obj_names[i].name );
	} while ( obj_names[i].objno != -1  &&  i < MAX_NOBJECTS );
	nobj_names = i;

	/* Sort them alphabetically */
	qsort( obj_names, nobj_names, sizeof (OBJ_NAME_ENTRY),
		comp_names );


	/* Read in address, size, type, and object info */
	i = 0;
	while((n =fscanf(fp, "%d %x %x %d %d", &temp1, &temp2, &temp3, &temp4, &temp5))==5)
	{
		mem_entry[i].type = temp1;
		mem_entry[i].address = temp2;
		mem_entry[i].size = temp4;
		mem_entry[i].objno = temp5;
		i++;
	}
	cno = i;

	fprintf ( stderr, "%d items\n", cno );

	fclose ( fp );

}


init_var()
{
	register int i;

	xorg = 60; xend = width  - 41; xlen = xend - xorg + 1;
	yorg = 90; yend = height - 51; ylen = yend - yorg + 1;

	addr_xmin = 0;
	addr_xmax = cno - 1;
	addr_ymin = 0.0;
	addr_ymax = 0.0;

	for ( i = 0; i < cno; i++ )
	{
		if ( addr_ymax < log ( (double) (mem_entry[i].size/16) ) )
			addr_ymax = log ( (double) (mem_entry[i].size/16) );
	}

	addr_xrange = addr_xmax - addr_xmin + 1;
	addr_yrange = addr_ymax - addr_ymin;

	xsmax = xmax; xsmin = xmin;
	ysmax = ymax; ysmin = ymin;

	printf ( "xmin %d xmax %d xrange %d ymin %f ymax %f yrange %f\n",
		xmin, xmax, xrange, ymin, ymax, yrange );
}
