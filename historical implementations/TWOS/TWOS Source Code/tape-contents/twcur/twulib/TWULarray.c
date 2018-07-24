/*  	Copyright (C) 1989, 1991, California Institute of Technology.
		U.S. Government Sponsorship under NASA Contract NAS7-918
		is acknowledged.        */
   
/*
 * TWULarray.c
 *
 * Dynamic Array ADT
 *
 * John Gieselman, Matthew Presley, Frederick Wieland, Lawrence Hawley
 * Jet Propulsion Laborartory
 * 4800 Oak Grove Dr.
 * Pasadena, CA 91109
 *
 * For documentation about the routines in this file, please see
 * the file TWULarray.h.
 *
 */

#include "twcommon.h"
#include "twusrlib.h"
#include "TWULarray.h"

/**********
 * CREATOR
 **********/

void	TWULInitArray(ptrTWULarray, maxElems, sizeElem, increment)
TWULarrayType	*ptrTWULarray;
int				maxElems;
int				sizeElem;
int				increment;
{
	ptrTWULarray->maxElem = maxElems;
	ptrTWULarray->sizeElem = sizeElem;
	ptrTWULarray->inc = increment;
	ptrTWULarray->number = 0;
	ptrTWULarray->ptrArray = newBlockPtr(maxElems * sizeElem);

	if (!(ptrTWULarray->ptrArray))
		userError("TWULInitArray: Unable to allocate memory");
}

/*************
 * DESTRUCTOR
 *************/

void	TWULDispose(ptrTWULarray)
TWULarrayType	*ptrTWULarray;
{
	disposeBlockPtr(ptrTWULarray->ptrArray);
	ptrTWULarray->ptrArray = NULL;
	ptrTWULarray->maxElem = 0;
	ptrTWULarray->sizeElem = 0;
	ptrTWULarray->inc = 0;
	ptrTWULarray->number = 0;
}
 
/************
 * MODIFIERS
 ************/

void	TWULAddElem(ptrTWULarray, ptrElem)
TWULarrayType	*ptrTWULarray;
void			*ptrElem;
{
	void	TWULOverflow();
	char	*ptr;

	if (ptrTWULarray->number == ptrTWULarray->maxElem)
		TWULOverflow(ptrTWULarray, 1);

	ptr = (char *)pointerPtr(ptrTWULarray->ptrArray);
	ptr += ptrTWULarray->number * ptrTWULarray->sizeElem;
	TWULfastcpy(ptr, ptrElem, ptrTWULarray->sizeElem);
	ptrTWULarray->number++;
}

void	TWULInsertElem(ptrTWULarray, ptrElem, n)
TWULarrayType	*ptrTWULarray;
void			*ptrElem;
int				n;
{
	void	TWULOverflow();
	char	*ptr;

	if (n < 0 || n > ptrTWULarray->number)
		return;
	
	if (n == ptrTWULarray->number)
		TWULAddElem(ptrTWULarray, ptrElem);
	else
		{
		ptr = (char *)pointerPtr(ptrTWULarray->ptrArray);
		ptr += n * ptrTWULarray->sizeElem;
		TWULfastcpy(ptr, ptrElem, ptrTWULarray->sizeElem);
		}
}

void	TWULRemoveElem(ptrTWULarray, n)
TWULarrayType	*ptrTWULarray;
int				n;
{
	char	*ptr1;
	char	*ptr2;
	int		len;

	if (n < 0 || n >= ptrTWULarray->number)
		return;

	if (n != (ptrTWULarray->number - 1))
		{
		ptr1 = (char *)pointerPtr(ptrTWULarray->ptrArray);
		ptr1 += n * ptrTWULarray->sizeElem;
		ptr2 = ptr1 + ptrTWULarray->sizeElem;
		len = (ptrTWULarray->number - n - 1) * ptrTWULarray->sizeElem;
		TWULfastcpy(ptr1, ptr2, len);
		}

	ptrTWULarray->number--;
}

void	TWULClearArray(ptrTWULarray)
TWULarrayType	*ptrTWULarray;
{
	ptrTWULarray->number = 0;
}

void	TWULCatArray(ptrTWULarray1, ptrTWULarray2)
TWULarrayType	*ptrTWULarray1;
TWULarrayType	*ptrTWULarray2;
{
	char	*ptr1;
	char	*ptr2;
	int		i;

	if (ptrTWULarray1->maxElem <= ptrTWULarray1->number + ptrTWULarray2->number)
		{
		i = ptrTWULarray1->number+ptrTWULarray2->number - ptrTWULarray1->maxElem;
		i = i / ptrTWULarray1->inc + 1;
		TWULOverflow(ptrTWULarray1, i);
		}
	
	ptr1 = (char *)pointerPtr(ptrTWULarray1->ptrArray);
	ptr1 += ptrTWULarray1->number * ptrTWULarray1->sizeElem;
	ptr2 = (char *)pointerPtr(ptrTWULarray2->ptrArray);
	ptrTWULarray1->number += ptrTWULarray2->number;
	TWULfastcpy(ptr1, ptr2, ptrTWULarray2->number * ptrTWULarray2->sizeElem);
}

/************
 * ACCESSORS
 ************/
 
void	*TWULGetElem(ptrTWULarray, n)
TWULarrayType	*ptrTWULarray;
int				n;
{
	if (n < 0 || n >= ptrTWULarray->number)
		return NULL;
	
	return (void *)((n * ptrTWULarray->sizeElem) + 
					(char *)pointerPtr(ptrTWULarray->ptrArray));
}
 
int		TWULSizeArray(ptrTWULarray)
TWULarrayType	*ptrTWULarray;
{
	return	ptrTWULarray->number;
}

/********************
 * INTERNAL ROUTINES
 ********************/

void	 TWULOverflow(ptrTWULarray, n)
TWULarrayType	*ptrTWULarray;
int				n;
{
	TWULarrayType	oldArray;

	oldArray = *ptrTWULarray;

	TWULInitArray(ptrTWULarray, oldArray.maxElem + n*oldArray.inc,
				oldArray.sizeElem, oldArray.inc);
	TWULCatArray(ptrTWULarray, &oldArray);
	TWULDispose(&oldArray);
}
