#include <stdio.h>


dumparray (u, n)
    char           *u;
    int             n;
{
    char            s[80];
    char            t[4],
                    c;
    int             i,
                    j;
    int             VAXBUG;

    s[78] = '\n';
    s[79] = '\0';

#define toupper(x) (x)>='a' && (x)<='z' ? (x)-'a'+'A' : (x)
#define clrs    {int i; for(i=0; i<78; i++) s[i] = ' ';}

    for (i = 0; i < n;)
    {
	clrs

	VAXBUG = i;
	sprintf (t, "%3.3x:", i % 0x1000);
	i = VAXBUG;


	s[0] = toupper (t[0]);
	s[1] = toupper (t[1]);
	s[2] = toupper (t[2]);
	s[3] = t[3];

	for (j = 0; j < 16 && i < n; j++, i++)
	{
	    sprintf (t, "%2x ", 0377 & (c = 0377 & u[i]));
	    s[5 + 3 * j] = toupper (t[0]);
	    s[5 + 3 * j + 1] = toupper (t[1]);
	    s[5 + 52 + j] = c < 32 || c > 127 ? '.' : c;
	}
	fprintf (stderr, "%s", s);
    }
}
