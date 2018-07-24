AWK=    awk
TOUCH=  touch -cf
GREP=   egrep
VPATH	=  ..

#must use -X18 and not -X130 for magnitudeVector() fix to work

CFLAGS	      = -O -X18
 



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

circles.o:	circles.h
