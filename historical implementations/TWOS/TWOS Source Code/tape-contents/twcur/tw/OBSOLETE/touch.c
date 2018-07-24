/*
 * $Log:	touch.c,v $
 * Revision 1.1  90/08/07  15:41:27  configtw
 * Initial revision
 * 
*/
/*
 * touch - touch the page containing the provided address
 *
 */

void touch (addr)
    int *addr;
{
    int tmp;

    /* simply read the first byte, then write it back */

    tmp = *addr;
    *addr = tmp;
}
