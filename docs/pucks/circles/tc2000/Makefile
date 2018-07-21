AWK=    awk
TOUCH=  touch -cf
GREP=   egrep

CFLAGS	      = -O -X18 -X39



CIRCLES_CFILES =	\
	dumparray.c	\
	tc.c	\
	circles.c

CIRCLES_OFILES =	\
	dumparray.o	\
	tc.o	\
	circles.o

circles:	circles.o

tc:	$(CIRCLES_OFILES)
	cc $(CFLAGS) -O -o tc $(CIRCLES_OFILES) -lm

lint:	$(CFILES)
	lint $(CIRCLES_CFILES) | tee LINT

.c.o:
	cc $(CFLAGS) -c $*.c

circles.o:	circles.h
