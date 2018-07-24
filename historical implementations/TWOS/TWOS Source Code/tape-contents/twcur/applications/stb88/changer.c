#include <stdio.h>

main()
{
	FILE *in, *out, *fopen();
	int i=0;
	char name[20], temp[200], t1[200], t2[200];

	printf("Enter name:  ");
	scanf("%s", name);

	in = fopen(name,"r");
	out = fopen("temp", "w");
	
	while ( fgets(temp, 200, in) )
		{
		sscanf(temp, "%s %s %s", name,t1,t2);
		if (strcmp(name,"evtmsg")!=0)
			fprintf(out,"%s", temp);
		else
			fprintf(out,"tell %s %s 0 %s", t1, t2, 
				temp+strlen(name)+strlen(t1)+strlen(t2)+3);
		}

	fclose(in);
	fclose(out);
}
