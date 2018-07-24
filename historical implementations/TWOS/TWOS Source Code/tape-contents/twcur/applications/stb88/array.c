/*
 * array.c - generalized utility functions to handle arrays
 *				 Fred Wieland, Jet Propulsion Laboratory, March 1988
 *
 * How to Use These Utility Functions
 *
 * This file contains a number of useful functions for manipulating
 * arrays.  Theses functions are capable of handling ANY type of
 * array--an array of structures, characters, integers, etc. Because
 * it is so general, the type of the array must be passed to the
 * routines in this file.
 *
 * These routines are compatible with the restrictions on the use of Time
 * Warp objects.  Specifically, a Time Warp Object (TWO) cannot
 * have static data that is outside of the state data structure.  Thus,
 * it is impossible to specify the exact format of an array and declare
 * it statically for use by these routines.  Instead, all of the information
 * about the array must be passed to these routines, which then 
 * implement general algorithms for doing its job.  The arrays must
 * be declared in the object's state, and the following arguments
 * are used in the various routines:
 * 
 * char *where - a pointer to the first byte of the array stored in
 *               an object's state.
 *
 * char *what - for search routines, a pointer to a key that you
 *              want to search for. (See restrictions on the use of
 *              keys below).
 *
 * int element_size - the size of each element in the array. For instance,
 *                    if you have an array of struct X, then 
 *                    element_size = sizeof(struct X).
 *
 * int max_number - the maximum number of elements in the array. For 
 *                  instance, if in the state you have a declaration that
 *                  reads 'struct X x_array[40]', then max_number = 40.
 *
 * enum Key_types key_type - the data type of the key field in the array.
 *                           The data type may be String, Floating, 
 *                           Double, integer, or Shortint.
 *
 * RESTRICTIONS ON THE USE OF KEYS: All routines in this package assumes
 * that the key field of the array is the FIRST field of each element
 * in the array.  It also assumes that you do not have a compound key
 * field.                  
 *
 * ROUTINES INCLUDED IN THIS FILE:
 *----------------------------------------------------------------------
 * int Find(what, where, element_size, max_number, key_type)
 * char *what, *where;
 * int element_size, max_number;
 * enum Key_types key_type;
 *
 * This function returns the index at which the key pointed to by
 * 'what' is found in the array pointed to by 'where.' If the key pointed
 * to by 'what' is not found, this function returns a -1.
 *----------------------------------------------------------------------
 * int find_blank(where, element_size, max_number, key_type)
 * char *where;
 * int element_size, max_number;
 * enum Key_types key_type;
 *
 * This function returns the index of the first blank element in the
 * array pointed to by 'where,' or -1 if there are no blanks.  Note
 * that for arrays where key_type = integer, Floating, or Double, a
 * blank is detected if there is a 0 (zero) in the key field.
 *----------------------------------------------------------------------
 * int insert(what, where, element_size, max_number, key_type)
 * char *what, *where;
 * int element_size, max_number;
 * enum Key_types key_type;
 *
 * this function inserts the record pointed to by 'what' in the 
 * array pointed to by 'where.'
 *----------------------------------------------------------------------
 * int arr_delete(what, where, element_size, max_number, key_type)
 * char *what, *where;
 * int element_size, max_number;
 * enum Key_types key_type;
 *
 * This function deletes the record pointed to by 'what' in the
 * array pointed to by 'where.'  Note that this function must call
 * 'find' in order to figure out the index which it will delete. Note
 * that the position of all elements beyond 'what' will be decremented
 * by one when using this function.
 *----------------------------------------------------------------------
 * int clearit(what, where, element_size, max_number, key_type)
 * char *what, *where;
 * int element_size, max_number;
 * enum Key_types key_type;
 *
 * This function clears the record pointed to by 'what' in the
 * array pointed to by 'where.' Note that this function must call
 * 'find' in order to figure out the index which it will clear. Unlike
 * the 'delete' function above, the 'clear' function does not change
 * the position of other elements in the array.
 *----------------------------------------------------------------------
 * int count_entries(where, element_size, max_number) 
 * char *where;
 * int element_size, max_number;
 *
 * This function returns the number of elements in the array pointed
 * to by 'where.'
 *----------------------------------------------------------------------
 * int print_keys(where, element_size, max_number, key_type)
 * char *where;
 * int element_size, max_number;
 * enum Key_types key_type;
 *
 * This function prints out the values of the keys in the array pointed
 * to by 'where.' 
 *----------------------------------------------------------------------
 *
 * SAMPLE USAGE:
 *
 * #include "sample.h"
 *
 * typedef struct Sample   /* data structure definition  
 *		{
 *    char EnemyName[20];
 *    float EnemyStrength;
 *    float EnemyXPos, EnemyYPos;
 *		} Sample;
 *
 * typedef struct state
 *		{
 *		/* declarations of state variables 
 *		Sample array[MAX_NUM_IN_SAMPLE];  /* array declaration
 *		/* perhaps more declarations 
 *		} state;
 *
 * int search_for_enemy(s, who)
 * state *s;
 * char who[20];
 * 	{
 *		int i;
 *
 *		i = Find(who, &(s->array[0]), sizeof(struct Sample), 
 *				MAX_NUM_IN_SAMPLE, String);
 *    }
 *
 * int insert_in_array(s)
 * state *s;
 * 	{
 *		int i;
 *		Sample new_element;
 *
 *		strcpy(new_element.EnemyName, "Big Bad Wolf");
 *		new_element.EnemyStrength = Mighty;
 *		new_element.EnemyXPos = Forest;
 *		new_element.EnemyYPos = Ground;
 *		insert(&new_element, &(s->array[0]), sizeof(new_element),
 *				MAX_NUM_IN_SAMPLE, String);
 *		}
 ************************************************************************/
#include "array.h"

/*
 * This function returns the index at which the key pointed to by
 * 'what' is found in the array pointed to by 'where.' If the key pointed
 * to by 'what' is not found, this function returns a -1.
 */
int Find(what, where, element_size, max_number, key_type)
char *what;
char *where;
int element_size, max_number;
enum Key_types key_type;
	{
	int i;
	char *p;

	for (i = 0, p = where; i < max_number; p += element_size, i++)
		{
		if (keys_are_equal(p, what, key_type) == ARR_TRUE) return i;
		}
	return -1;
	}

/*
 * This function returns the index of the first blank element in the
 * array pointed to by 'where,' or -1 if there are no blanks.  Note
 * that for arrays where key_type = integer, Floating, or Double, a
 * blank is detected if there is a 0 (zero) in the key field.
 */
int find_blank(where, element_size, max_number, key_type)
char *where;
int element_size, max_number;
enum Key_types key_type;
	{
	int i;	
	char *p;

	for (i = 0, p = where; i < max_number; p+=element_size, i++)
		{
		if (key_is_blank(p, key_type) == ARR_TRUE) return i;
		}
	return -1;
	}

/* 
 * this function inserts the record pointed to by 'what' in the 
 * array pointed to by 'where.'
 */
int insert(what, where, element_size, max_number, key_type)
char *what;
char *where;
int element_size, max_number;
enum Key_types key_type;
	{
	int i;
	char *p;

	i = find_blank(where, element_size, max_number);
	if (i == -1) return -1;
	entcpy(where+i*element_size, what, element_size);
	return 0;
	}

/*
 * This function deletes the record pointed to by 'what' in the
 * array pointed to by 'where.'  Note that this function must call
 * 'find' in order to figure out the index which it will delete.
 */
int arr_delete(what, where, element_size, max_number, key_type)
char *what;
char *where;
int element_size, max_number;
enum Key_types key_type;
	{
	int j, i;
	char *p;

	i = Find(what, where, element_size, max_number, key_type);
	if (i == -1) return -1;
	p = where + (i * element_size);
	for (j = i; j < max_number - 1; j++, p += element_size)
		{
		entcpy(p, p+element_size, element_size);
		}
	clear(p, element_size);
	}

int clearit(what, where, element_size, max_number, key_type)
char *what;
char *where;
int element_size, max_number;
enum Key_types key_type;
	{
	int j, i;
	char *p;

	i = Find(what, where, element_size, max_number, key_type);
	if (i == -1) return -1;
	p = where + (i * element_size);
	clear(p, element_size);
	return 0;
	}

/*
 * This function returns the number of elements in the array pointed
 * to by 'where.'
 */
int count_entries(where, element_size, max_number)
char *where;
int element_size, max_number;
	{
	int i;
	char *p;

	for (i = 0, p = where; i < max_number; p += element_size, i++)
		{
		if (p[0] == '\0') return i;
		}
	return max_number;
	}

/* print_keys(where, element_size, 10, String) */
/* print_keys(where, element_size, max_number, integer) */

/*
 * This function prints out the values of the keys in the array pointed
 * to by 'where.' 
 */
int print_keys(where, element_size, max_number, key_type)
char *where;
int element_size, max_number;
enum Key_types key_type;
	{
	int i;
	char *p;

	for (i = 0, p = where; i < max_number; p += element_size, i++)
		{
		switch((int)key_type)
			{
			case String:
				printf("%d: %s\n", i, p);
				break;
			case Floating:
				printf("%d: %f\n", i, *p);
				break;
			case Double:
				printf("%d: %lf\n", i, *p);
				break;
			case integer:
				printf("%d: %d\n", i, *p);
				break;
			default:
				printf("%d: %s\n", i, p);
				break;
			}
		}
	}

/*
 * The following functions are utility functions used within this file
 * but not visible outside this file.
 */

static int keys_are_equal(v1, v2, key_type)
char *v1, *v2;  /*PJH changed from void for CRAY!!*/
enum Key_types key_type;
	{
	int *t1, *t2;
	double *d1, *d2;
	float *f1, *f2;
	char *c1, *c2;
	short *s1, *s2;

	switch((int)key_type)
		{
		case String:
			c1 = (char *)v1;
			c2 = (char *)v2;
			if (strcmp(c1, c2) == 0) return ARR_TRUE;
			else return ARR_FALSE;
			break;
		case Floating:
			f1 = (float *)v1;
			f2 = (float *)v2;
			return (*f1 == *f2);
			break;
		case Double:
			d1 = (double *)v1;
			d2 = (double *)v2;
			return (*d1 == *d2);
			break;
		case integer:
			t1 = (int *)v1;
			t2 = (int *)v2;
			return (*t1 == *t2);
			break;
		case Shortint:
			s1 = (short *)v1;
			s2 = (short *)v2;
			return (*s1 == *s2);
			break;
		default:
			printf("Key type %d not recognized\n", key_type);
		}
	}

static int key_is_blank(k, key_type)
char *k;
enum Key_types key_type;
	{
	int *i;
	double *d;
	float *f;
	short *s;
	
	switch((int)key_type)
		{
		case String:
			if (k[0] == '\0') return ARR_TRUE;
			else return ARR_FALSE;
			break;
		case Floating:
			f = (float *)k;
			return (*f == 0.0);
			break;
		case Double:
			d = (double *)k;
			return (*d == 0.0);
			break;
		case integer:
			i = (int *)k;
			return (*i == 0);
			break;
		case Shortint:
			s = (short *)k;
			return (*s == 0);
			break;
		default:
			printf("Key type %d not recognized\n", key_type);
		}
	}
