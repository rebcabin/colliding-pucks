/*      Copyright (C) 1989, 1991, California Institute of Technology.
		U. S. Government Sponsorship under NASA Contract NAS7-918
		is acknowledged.        */

/*
 * $Log:	tlib.c,v $
 * Revision 1.3  91/07/17  15:13:33  judy
 * New copyright notice.
 * 
 * Revision 1.2  91/06/03  12:27:14  configtw
 * Tab conversion.
 * 
 * Revision 1.1  90/08/07  15:41:25  configtw
 * Initial revision
 * 
*/
char tlib_id [] = "@(#)tlib.c   1.4\t6/2/89\t12:46:24\tTIMEWARP";


/*

Purpose:
		
		tlib.c contains a lot of archaic, largely unused code.  Apparently,
		most of it was written because of problems with the Mark2 version
		of the Hypercube.  These problems no longer exist in the Mark3
		Hypercube.

		The code in tlib.c performs several basic arithmetic functions
		on long integers, performs one comparison on long integers,
		converts integers and longs into strings, and converts integers
		and longs into hexidecimal strings.

Functions:
		
		ladd(x,y) - add two longs
				Parameters - long x, long y
				Return - the sum of the two longs

		lsub(x,y) - subtract one long from another long
				Parameters - long x, long y
				Return - the difference of the two longs

		lmul(x,y) - multiply two longs
				Parameters - long x, long y
				Return - the product of two longs

		ldiv(x,y) - divide one long by another long
				Parameters - long x, long y
				Return - the quotient of the two longs

		lmod(x,y) - take one long modulus the other
				Parameters - long x, long y
				Return - the remainder of the two longs

		lcmp(x,y) - compare two long integers
				Parameters - long x, long y
				Return - -1 if x<y, 0 if x=y, 1 if x>y

		itoa(n,s) - convert an integer to a character string
				Parameters - int n, char *s
				Return - a pointer to the string

		ltoa(n,s) - convert a long integer to a string
				Parameters - long n, char *s
				Return - a pointer to the string

		itoh(n,s) - convert an integer to a hexidecimal string
				Parameters -  int n, char *s
				Return - a pointer to the string

		ltoh(n,s) - convert a long integer to a hexidecimal string
				Parameters -  int n, char *s
				Return - a pointer to the string

Implementation:

		The ladd(), lsub(), etc. functions have absolutely no 
		interesting features.

		itoa() first determines if the number is negative.  If it
		is, a "-" sign is put in the string's first position.
		Then, iteratively, itoa() runs through the number, extracting
		each least significant digit and converting it to a character.
		Each character is put into a string.  When the integer has been
		fully converted, put a null at the end of the string.  The resulting 
		string is in reverse order, so itoa() next runs through the string,
		reversing it.

		ltoa() is precisely the same as itoa(), except that it starts
		with a long, rather than with an integer.

		itoh() iteratively converts the four lowest order bits in the
		provided integer into a hexidecimal digit.  Then it shifts the
		integer four positions to the right, masks off the top four
		bits (so that, if ones were shifted in due to the shifting
		strategy of the underlying implementation, they will be changed
		to zeros), and continues until all bits have been converted.
		Again, the string winds up in reverse order, so, after putting
		a null at the end of it, itoh() reverses the string.

		ltoh() is like itoh(), but it works on a long, rather than an
		integer.

*/

long ladd(x,y) long x,y; { return x+y; }
long lsub(x,y) long x,y; { return x-y; }
long lmul(x,y) long x,y; { return x*y; }
long ldiv(x,y) long x,y; { return x/y; }
long lmod(x,y) long x,y; { return x%y; }
int  lcmp(x,y) long x,y; { if (x<y) return -1; else return ((x==y)?0:1); }
char *itoa(n,s)
int n;
char *s;
{
	char *p=s, c;
	if (n<0) {
		*p++ = '-';
		n = -n;
	}
	for (;;) {
		*p++ = (n%10) + '0';
		n /= 10;
		if (n==0) break;
	}
	*p-- = n = 0;
	if (*s=='-') {s++; n++;}
	for (;;) {
		c= *p; *p= *s; *s=c;
		p--; s++; n++;
		if (p<s) break;
	}
	return s-n;
}

char *ltoa(n,s)
long n;
char *s;
{
	char *p=s, c;
	if (n<0) {
		*p++ = '-';
		n = -n;
	}
	for (;;) {
		*p++ = (n%10) + '0';
		n /= 10;
		if (n==0L) break;
	}
	*p-- = n = 0;
	if (*s=='-') {s++; n++;}
	for (;;) {
		c= *p; *p= *s; *s=c;
		p--; s++; n++;
		if (p<s) break;
	}
	return s-n;
}

char *itoh(n,s)
int n;
char *s;
{
	char *p=s, c;
	for (;;) {
		c = n & 0xf;
		*p++ = (c>9) ? (c+('A'-10)) : (c+'0');
		n = (n >> 4) & 0xFFF;   /* '& 0xfff' is there for safety */
		if (n==0) break;
	}
	*p-- = 0;
	for (;;) {
		c= *p; *p= *s; *s=c;
		p--; s++; n++;
		if (p<s) break;
	}
	return s-n;
}

char *ltoh(n,s)
long n;
char *s;
{
	char *p=s, c;
	for (;;) {
		c = n & 0xfL;
		*p++ = (c>9) ? (c+('A'-10)) : (c+'0');
		n = (n >> 4) & 0xFFFFFFFL;      /* '& 0xFFFFFFFL' is there for safety */
		if (n==0L) break;
	}
	*p-- = n = 0;
	for (;;) {
		c= *p; *p= *s; *s=c;
		p--; s++; n++;
		if (p<s) break;
	}
	return s-n;
}
