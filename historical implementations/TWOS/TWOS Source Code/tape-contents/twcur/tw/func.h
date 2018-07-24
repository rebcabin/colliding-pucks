/*      Copyright (C) 1989, 1991, California Institute of Technology.
		U. S. Government Sponsorship under NASA Contract NAS7-918
		is acknowledged.        */

/*
 * $Log:	func.h,v $
 * Revision 1.3  91/07/17  15:08:22  judy
 * New copyright notice.
 * 
 * Revision 1.2  91/06/03  12:24:12  configtw
 * Tab conversion.
 * 
 * Revision 1.1  90/08/07  15:38:23  configtw
 * Initial revision
 * 
*/

/*      func.h          */

typedef struct
{
	short name;
	long beg, end;
	char level;
	char extra;
	char ftype, fstars;
	char types[10];
	char stars[10];
	short args[10];
} FUNC;
