/* itoa.c -- itoa function for warpnet */ 

#include "twcommon.h"

itoa(n,s)
Int n;
char s[];
{
   Int c,j,i;

   i=0;
   do {
	s[i++] = n%10 + '0';
   } while ((n/=10)>0);
   s[i] = '\0';

   for (i=0,j=strlen(s)-1;i<j;i++,j--)
	{
	c=s[i];
	s[i]=s[j];
	s[j]=c;

	}
}
