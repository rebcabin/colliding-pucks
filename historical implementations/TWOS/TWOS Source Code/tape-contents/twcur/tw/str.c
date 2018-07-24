/*      Copyright (C) 1989, 1991, California Institute of Technology.
		U. S. Government Sponsorship under NASA Contract NAS7-918
		is acknowledged.        */

/*
 * $Log:	str.c,v $
 * Revision 1.3  91/07/17  15:13:03  judy
 * New copyright notice.
 * 
 * Revision 1.2  91/06/03  12:27:00  configtw
 * Tab conversion.
 * 
 * Revision 1.1  90/08/07  15:41:14  configtw
 * Initial revision
 * 
*/
char str_id [] = "@(#)str.c     1.1\t7/7/87\t17:01:29\tTIMEWARP";

#include "twcommon.h"
#include "twsys.h"

FUNCTION Char *strcat(s1, s2)
register Char *s1;      /* target string to which characters are appended */
register Char *s2;      /* source string of characters which are appended */
{
	Char *s0=s1;

	while( *s1 )
		s1++;
	while( *s1++ = *s2++ )
		;
	return s0;
	}


FUNCTION Char *strchr(s, c)
register Char *s;       /* string to be searched */
register Char c;        /* character to be searched for */
{
	for (; *s; s++)
		if (*s == c)
			return s;
	return NULL;
	}


FUNCTION Int strcmp(s1, s2)
register Char *s1;      /* string to be compared */
register Char *s2;      /* string which is the standard of comparison */
{
	for (; (*s1) && (*s1==*s2); s1++,s2++)
		;
	return (*s1-*s2);
	}


FUNCTION Char *strcpy(s1, s2)
register Char *s1;      /* destination of string copy */
register Char *s2;      /* source of characters for string copy */
{
	Char *s0=s1;

	while (*s1++ = *s2++)
		;
	return s0;
	}


FUNCTION Int strlen(s)
register Char *s;       /* string whose length is taken */
{
	register i=0;

	while (*s++)
		i++;
	return i;
	}


FUNCTION Char *strncat(s1, s2, n)
register Char *s1;      /* target string to which characters are appended */
register Char *s2;      /* source string of characters which are appended */
register Int n;         /* maximum number of characters to move */
{
	Char *s0=s1;

	if ((n <= 0) || (!(*s2)))
		return s0;
	while (*s1)
		s1++;
	while ((n--) && (*s1++ = *s2++))
		;
	if (!n)
		*s1 = '\0';
	return s0;
	}


FUNCTION Int strncmp(s1, s2, n)
register Char *s1;      /* string to be compared */
register Char *s2;      /* string which is the standard of comparison */
register Int n;         /* maximum number of characters to compare */
{
	if (n <= 0)
		return 0;
	for (; (--n>0) && (*s1) && (*s1==*s2); s1++,s2++)
		;
	return (*s1-*s2);
	}


FUNCTION Char *strncpy(s1, s2, n)
register Char *s1;      /* destination of string copy */
register Char *s2;      /* source of characters for string copy */
register Int n;         /* maximum number of characters to copy */
{
	Char *s0=s1;

	if (n <= 0)
		return s0;
	while ((*s1++ = *s2++) && (--n))
		;
	while (--n > 0)
		*s1++ = '\0';
	return s0;
	}


/* CAUTION: the following routine may be wrong.  Use at your own risk. */
FUNCTION Char *strrchr(s, c)
register Char *s;       /* string to be searched */
register Char c;        /* character to be searched for */
{
	register Char *s0=s;

	while (*s)
		s++;
	for (s--; s>=s0; s--)
		if (*s == c)
			return s;
	return NULL;
	}


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
