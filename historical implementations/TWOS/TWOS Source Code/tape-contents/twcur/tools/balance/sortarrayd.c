/*	Copyright (C) 1989, 1991, California Institute of Technology.
        	U. S. Government Sponsorship under NASA Contract NAS7-918 
                is acknowledged.	*/


/*****************************************************************************/


/*			S O R T   A R R A Y				     */


sortarrayd( valArray, indexArray, num )
double	valArray[];
int	indexArray[];
int	num;	
{
    register	j;
    double	aDouble;
    int		anInt, flag, i;

    for ( i = 0; i < num - 1; i++ )
    {
	if ( valArray[i] > valArray[i+1] )
	{

	    flag = 1;

	    for (j = i; j >= 0; j--)
               if (valArray[j] > valArray[j+1]) 
	       {

		  flag = 0;

	    	  aDouble = valArray[j];
		  valArray[j] = valArray[j+1];
		  valArray[j+1] = aDouble;

		  anInt = indexArray[j];
		  indexArray[j] = indexArray[j+1];
		  indexArray[j+1] = anInt;
		}

	    if (flag)
		break;
	}
    }
}


/*****************************************************************************/
