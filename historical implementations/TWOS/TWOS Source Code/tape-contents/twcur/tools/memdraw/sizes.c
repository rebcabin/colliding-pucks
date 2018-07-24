#include "mplot.h"

/* vars for grouping sizes together in histogram */
int grouping_threshold = 0;

SIZE_ENTRY sizes[MAX_SIZES];
int nsize_entries, max_how_many;

extern OBJ_NAME_ENTRY obj_names[];
extern int show_object;

comp_size_entries (i, j)
/* Derek Prowe, summer 91
   Used to sort the size entries by size. */
	SIZE_ENTRY *i, *j;
{
	if ((*i).beg_size < (*j).beg_size)
		return -1;
	else if ((*i).beg_size > (*j).beg_size)
		return 1;
	else
		return 0;
}


SIZE_ENTRY temp_sizes[MAX_SIZES];

recluster (cluster_size)
/* Derek Prowe, summer 91 */
/* Copies sizes[0..nsize_entries] into temp_sizes, clustering sizes that have
   <= cluster_size entries together with adjacent small sizes.  It returns
   the number of sizes entries in temp_sizes. */  
	int cluster_size;
{
	register int i = 0, running_tot = 0;
	int place = 0;

	while ( i < nsize_entries )
		if ( sizes[i].how_many > cluster_size )
		{
/* This size_entry has enough members to stand on its own--
   do not put it in a cluster. */
			temp_sizes[place].how_many = sizes[i].how_many;
			temp_sizes[place].beg_size = sizes[i++].beg_size;
			temp_sizes[place++].end_size = 0;
		}
		else
		{
/* This size_entry does not have enough members to stand on its own--
   put it in a cluster. */
			running_tot = 0;
			temp_sizes[place].beg_size = sizes[i].beg_size;

			while ( i < nsize_entries &&
				sizes[i].how_many <= cluster_size)
				running_tot += sizes[i++].how_many;

			temp_sizes[place].how_many = running_tot;
			temp_sizes[place++].end_size = 
				sizes[i-1].beg_size > temp_sizes[place].beg_size
				? sizes[i-1].beg_size
				: 0;
		}

	return place;
}

cluster_sizes ()
/* Derek Prowe, summer 91 */
{
	int new_nsize_entries;
	register int i = 0;

	if ( grouping_threshold != 0 )
	{
/* cluster "automatically" to keep the right # of bars in the histogram */
		while ( ( new_nsize_entries = recluster (i++) ) 
			> grouping_threshold )
			;

		nsize_entries = new_nsize_entries;

	/* copy the results into the global array "sizes" */
		for ( i = 0;  i < nsize_entries;  i++ )
		{
			sizes[i].how_many = temp_sizes[i].how_many;
			sizes[i].beg_size = temp_sizes[i].beg_size;
			sizes[i].end_size = temp_sizes[i].end_size;
		}
	}
}


find_size(this_size)
/* Derek Prowe, summer 91 */
/* returns an index into the array sizes[] to the entry which has the same
	size as this_size, and -1 if there isn't such an entry in sizes[] */

	int this_size;
{
	register int j;

	for (j = 0;  j < nsize_entries; j++)
		if ( sizes[j].beg_size == this_size)
			return j;

	return -1;
}


collect_sizes (start, end)
/* Derek Prowe, summer 91.
   Called from page_start to collect all the sizes from out of mem_entry's. */
int start, end;
{
	register int i, j;

	if ( start < 0)
		start = 0;
	if (end > cno)
		end = cno;

	/* collect memory entries in individual size brackets */
	nsize_entries = 0;
	for (i = start;  i < end;  i++)

		if ( (mem_entry[i].type & show_mask) && 
			( show_object == -1 || mem_entry[i].objno ==
					obj_names[show_object].objno ) )
	/* only collect sizes that are showing (not hidden with "hide"
		or by object selection) */
		{
			if ( (j = find_size ( mem_entry[i].size)) != -1)
			/* This is not the first mem_entry of this size */
				sizes[j].how_many++;

			else
			/* This is the first mem_entry of this size */
			{
				sizes[nsize_entries].beg_size = mem_entry[i].size;
				sizes[nsize_entries].end_size = 0;
				sizes[nsize_entries].how_many = 1;
				nsize_entries++;
			}
		}

	/* Sort the size entries by size */
	qsort( sizes, nsize_entries, sizeof (SIZE_ENTRY), comp_size_entries );

	/* group clusters of memory sizes that have few entries together */
	cluster_sizes();

	/* Find largest number of any given size (max_how_many) */
	max_how_many = 0;
	for (i = 0; i < nsize_entries; i++ )
		max_how_many = MAX ( sizes[i].how_many, max_how_many );

	size_xmax = max_how_many;
	size_xmin = 0;
	size_xrange = size_xmax - size_xmin;
	size_ymax = nsize_entries;
	size_ymin = 0;
	size_yrange = size_ymax - size_ymin;
}
