/* "Copyright (C) 1989, California Institute of Technology. 
     U. S. Government Sponsorship under NASA Contract 
   NAS7-918 is acknowledged." */
/*
	atol(s) - convert a string to a number
		Parameters - Char *s
		Return - the corresponding number

	itoa(n,s) - convert integer to ASCII string

	atol() converts strings containing numeric characters into 
	numbers.  First, it skips over leading white space.  Then it
	looks for a positive or negative sign.  If none is found, it
	assumes positive.  Then, as long as the characters are in the
	range "0" to "9", it converts them into a running total.  (Though
	a comment suggests that white space between the (optional) sign
	and the number is skipped, it appears that such space would cause
	the routine to end.)  Finally, the total is multiplied by the
	sign, to get the final result.

*/

#include "twcommon.h"
#ifndef FUNCTION
#define FUNCTION
#endif


FUNCTION Long atol(s)
char *s;
{
    Long i, n, sign;

    for(i = 0; s[i] == ' ' || s[i] == '\n' || s[i] == '\t'; i++)
        ;

    sign = 1;

    if(s[i] == '+' || s[i] == '-') {
        sign =(s[i++] == '+') ? 1 : -1;
        }

    for(n = 0; s[i] >= '0' && s[i] <= '9'; i++) {
        /* Skip white space. */
        n = 10 * n + s[i] - '0';
        }

    return(sign * n);
    }

/* Convert integer to ASCII  */

FUNCTION char *itoa(n,s)
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

