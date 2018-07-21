AWK=    awk
TOUCH=  touch -cf
GREP=   egrep

########plug in the name of your application here
A = 	Puck2Therm

F = 	-f68881 -O
I = 	-I/usr/tw/brian/include

C =	\
	Puck2Therm.c	\
	PuckIO.c	\
	Accumulator.c	\
	ThermData.c	\
	ThermDataIO.c

O =	\
	Puck2Therm.o	\
	PuckIO.o	\
	ThermData.o	\
	Accumulator.o	\
	ThermDataIO.o

L =	\
	-lm

$A:	$O
	cc $F -o $A $O $L

backup:
	chmod 0777 back/*
	cp *.[ch] Makefile back/.
	chmod 0444 back/*
	ls -l back

lint:	$C
	lint $C | tee LINT

.c.o:
	cc $I $F -c $*.c

depend: $C
	${GREP} '^#include' $C | sed \
		-e '/</d' \
		-e 's/:[^"]*"\([^"]*\)".*/: \1/' \
		-e 's,^../[a-zA-Z0-9]*/\([^\.]*\)\.[cs],\1.o \1.L,' | \
	${AWK} ' { if ($$1 != prev) { print rec; rec = $$0; prev = $$1; } \
		else { if (length(rec $$2) > 78) { print rec; rec = $$0; } \
		       else rec = rec " " $$2 } } \
	      END { print rec } ' > makedep
	echo '/^# DO NOT DELETE THIS LINE/+1,$$d' >eddep
	echo '$$r makedep' >>eddep
	echo 'w' >>eddep
	cp Makefile Makefile.bak
	ex Makefile < eddep
	rm eddep makedep

# DO NOT DELETE THIS LINE -- make depend uses it

Puck2Therm.c: Puck.h Accumulator.h ThermData.h
PuckIO.c: Puck.h
Accumulator.c: Puck.h Accumulator.h
ThermData.c: Accumulator.h ThermData.h
ThermDataIO.c: ThermData.h
