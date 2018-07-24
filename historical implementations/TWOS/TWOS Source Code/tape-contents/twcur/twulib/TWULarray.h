/*  	Copyright (C) 1989, 1991, California Institute of Technology.
		U.S. Government Sponsorship under NASA Contract NAS7-918
		is acknowledged.        */
   
/*
 * TWULarray.h
 *
 * Dynamic Array ADT
 *
 * John Gieselman, Matthew Presley, Frederick Wieland, Lawrence Hawley
 * Jet Propulsion Laborartory
 * 4800 Oak Grove Dr.
 * Pasadena, CA 91109
 *
 * This ADT handles an array of user specified elements.  The array
 * storage is dynamically created using the Time Warp memory allocation
 * functions.  These arrays will dynamically resize themselves assuming
 * that any access to an element is done through this ADT.  Array elements
 * are numbered 0 through the current size - 1.
 *
 * This model is implemented with the following routines:
 *
 * CREATORS:
 *
 * TWULInitArray(ptrTWULarray, maxElems, sizeElem, increment)
 *		This function initializes a dynamic array.  Parameters are a
 *		pointer to the TWULarrayType structure being initialized, the
 *		initial number of elements in the array, the size in bytes of
 *		each element in the array, and the number of elements to add
 *		when the array is resized on overflow.
 *
 * DESTRUCTORS:
 *
 * TWULDispose(ptrTWULarray)
 *		This function frees all of the memory used by a dynamic array.
 *		This function should be called when the array is no longer needed.
 *		The parameter is a pointer to the TWULarrayType structure for
 *		the array.
 *
 * MODIFIERS
 *
 * TWULAddElem(ptrTWULarray, elemPtr)
 *		This function adds a new element to the end of the array.  If 
 *		there is an overflow, the array will be increased by the size
 *		in its increment amount defined through the TWULInitArray function.
 *		Parameters are a pointer to the TWULarrayType structure for the 
 *		array and a pointer to the new data element.
 *
 * TWULInsertElem(ptrTWULarray, elemPtr, n)
 *		This function replaces the nth element in the array.  If n is
 *		greater than the current size of the array, nothing will happen.
 *		If n is equal to the current size of the array, a new element
 *		will be added at the end of the array.  If n is less than the
 *		current size of the array, the nth element of the array will
 *		be overwritten with new data.  Parameters are a pointer to the
 *		TWULarrayType structure for the array, a pointer to the new data
 *		element, and the element number being replaced.
 *
 * TWULRemoveElem(ptrTWULarray, n)
 *		This function removes the nth element of the array and then
 *		collapses the array.  If n is greater than the current size of
 *		the array, nothing will happen.  Parameters are a pointer to
 *		the TWULarrayType structure for the array and and the element
 *		number being removed.
 *
 * TWULClearArray(ptrTWULarray)
 *		This function removes all elements of the array.  The memory
 *		allocate for the array is NOT freed.  The parameter is a pointer
 *		to the TWULarrayType structure for the array.
 *
 * TWULCatArray(ptrTWULarray1, ptrTWULarray2)
 *		This function concatenates the contents of one dynamic array
 *		to another.  Note that both arrays must be initialized and have
 *		the same size data elements.  Array 2 is concatenated into Array 1.
 *		Parameters are pointers to the TWULarrayType structure for each
 *		array.
 *
 * ACCESSORS
 *
 * TWULGetElem(ptrTWULarray, n)
 *		This functions returns a pointer to the nth element of the array.
 *		If n is greater than the current size of the array, the value
 *		NULL will be returned.  Parameters are a pointer to the
 *		TWULarrayType structure for the array and the element number
 *		being retrieved.
 *
 * TWULSizeArray(ptrTWULarray)
 *		This function returns the number of elements currently in the
 *		array.  The parameter is a pointer to the TWULarrayType
 *		structure for the array.
 *
 */

/*
 *
 * FUNCTION DECLARATIONS
 */
 
/**********
 * CREATOR
 **********/

void	TWULInitArray();
/*	void	TWULInitArray(ptrTWULarray, maxElems, sizeElem, increment)
 *		TWULarrayType	*ptrTWULarray;
 *		int				maxElems;
 *		int				sizeElem;
 *		int				increment;
 */

/*************
 * DESTRUCTOR
 *************/

void	TWULDispose();
/*	void	TWULDispose(ptrTWULarray)
 *		TWULarrayType	*ptrTWULarray;
 */
 
/************
 * MODIFIERS
 ************/

void	TWULAddElem();
/*	void	TWULAddElem(ptrTWULarray, elemPtr)
 *		TWULarrayType	*ptrTWULarray;
 *		void			*elemPtr;
 */

void	TWULInsertElem();
/*	void	TWULInsertElem(ptrTWULarray, elemPtr, n)
 *		TWULarrayType	*ptrTWULarray;
 *		void			*elemPtr;
 *		int				n;
 */

void	TWULRemoveElem();
/*	void	TWULRemoveElem(ptrTWULarray, n)
 *		TWULarrayType	*ptrTWULarray;
 *		int				n;
 */

void	TWULClearArray();
/*	void	TWULClearArray(ptrTWULarray)
 *		TWULarrayType	*ptrTWULarray;
 */

void	TWULCatArray();
/*	void	TWULCatArray(ptrTWULarray1, ptrTWULarray2)
 *		TWULarrayType	*ptrTWULarray1;
 *		TWULarrayType	*ptrTWULarray2;
 */

/************
 * ACCESSORS
 ************/
 
void	*TWULGetElem();
/*	void	*TWULGetElem(ptrTWULarray, n)
 *		TWULarrayType	*ptrTWULarray;
 *		int				n;
 */
 
int		TWULSizeArray();
/*	int		TWULSizeArray(ptrTWULarray)
 *		TWULarrayType	*ptrTWULarray;
 */

