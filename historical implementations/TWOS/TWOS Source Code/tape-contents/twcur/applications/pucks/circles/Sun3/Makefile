AWK=    awk
TOUCH=  touch -cf
GREP=   egrep


CFLAGS	      = -O1 -f68881 -Bstatic
 



CIRCLES_CFILES =	\
	dumparray.c	\
	tc.c	\
	circles.c

CIRCLES_OFILES =	\
	dumparray.o	\
	tc.o	\
	circles.o

tc:	$(CIRCLES_OFILES)
	cc $(CFLAGS) -o tc $(CIRCLES_OFILES) -lm

lint:	$(CFILES)
	lint $(CIRCLES_CFILES) | tee LINT

.c.o:
	cc $(CFLAGS) -c $*.c

%.o: ../%.c
	cc $(CFLAGS) -c ../$*.c

circles.o:	../circles.h
