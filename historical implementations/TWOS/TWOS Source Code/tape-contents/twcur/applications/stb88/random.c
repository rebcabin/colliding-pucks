#define MODULUS 65536       /* These first three are for the random   */
#define MULTIPLIER 25173
#define INCREMENT 13849     /* number generator.                      */

#if 1

int my_random(size,seed)
int size,*seed;
{
        int ret, value;

	value = *seed;
        value *= MULTIPLIER;
        value += INCREMENT;
        value %= MODULUS;

        ret = ( value * size/MODULUS + 1);
        return(ret);
}
#endif

#if 0

int my_random(size, seed)
int size, *seed;
{
	return 6;
}

#endif
