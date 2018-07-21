AWK=    awk
TOUCH=  touch -cf
GREP=   egrep


CFLAGS	      = -cubix
 



CIRCLES_CFILES =	\
	dumparray.c	\
	tc.c	\
	circles.c

CIRCLES_OFILES =	\
	dumparray.o	\
	tc.o	\
	circles.o

tc:	$(CIRCLES_OFILES)
	cc $(CFLAGS) -O -o tc $(CIRCLES_OFILES) -lm

lint:	$(CFILES)
	lint $(CIRCLES_CFILES) | tee LINT

.c.o:
	cc68 $(CFLAGS) -c $*.c

%.o: ../%.c
	cc68 $(CFLAGS) -c ../$*.c
	mv ../$*.o .

circles.o:	../circles.h
