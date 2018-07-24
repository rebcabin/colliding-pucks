#	Copyright (C) 1989, 1991, California Institute of Technology.
#		U. S. Government Sponsorship under NASA Contract NAS7-918 
#		is acknowledged.
#

#	makefile.config 8/4/88  8/16/88  1/19/89 4/18/89
#	modified by JJW 3/6/90 for tw2.4 simulator
#	modified by JJW 6/13/90 for Mach memory changes
#       JJW for TC2000
#

# ******************************************************************************
# Special version for the TC2000.  Because Make seems to be much inferior to
# the Sunpro make, this version hs only one CFLAGS. set it up fro -g or -O
# as desired. (Of course we could also try to compile GNU make and use it)
# *****************************************************************************

DESTDIR=
INCDIR= ../tw
SPFLAGS=
 
CFLAGS= -DBF_MACH -DBFLY2 -DSIMULATOR -O -X18 -X39 -I$(INCDIR)


CFILES      =   newconf.c\
		itimer.c \
		stdine.c \
		stdout.c \
		twhelp.c \
		twsd.c \
		twsp1.c \
		twsp2.c \
		simlex.c\
		simpar.c\
		twsp3.c\
		twqueues.c \
		oldtw.c \
		HOST_fileio.c \
		statsio.c \
		simmem.c \
		faults.c

SFILES =        SUNcopy.s \
		SUNclear.s \
		SUNcomp.s


SUN4CFILES =    SUN4clear.c \
		SUN4comp.c \
		SUN4copy.c

TCCFILES =    TCclear.c \
		TCcomp.c \
		BBNclock.c \
		TCcopy.c


OBJS          = twsp1.o\
		twsp2.o\
		twsp3.o\
		HOST_fileio.o \
		itimer.o \
		newconf.o\
		simlex.o\
		simpar.o\
		statsio.o \
		stdout.o\
                TCcopy.o\
                TCcomp.o\
		BBNclock.o \
                TCclear.o\
		twhelp.o\
		twqueues.o \
		twsd.o\
		vtime.o \
		simmem.o \
		faults.o \
		oldtw.o

# objects  for standalone config language tester

OBJS2        =  newconf.o \
		simlex.o\
		simpar.o\
		vtime.o\
		HOST_fileio.o \
		simmem.o




all:	twsim 


twsim:	$(OBJS)
	ar rcv twsim.a $(OBJS)
	ranlib twsim.a


dbgsim:	$(OBJS)
	ar rcv dbgsim.a $(OBJS)
	ranlib dbgsim.a


conftest:	$(OBJS2)
	cc -o conftest $(CFLAGS) $(OBJS2)

spotless:
	rm -f *.o simlex.c simpar.c simpar.h y.output newconf.c

clean:
	rm -f *.o

# should stamp last make for twsim or dbgsim so we don't have to clean the
# .o files out each time because we can't remember.

.c.a:
	$(AR) $(ARFLAGS) $@ $%

.s.a:
	$(AR) $(ARFLAGS) $@ $%

simpar.c + simpar.h:	simpar.y
	yacc -d simpar.y
	sed -e s/yy/yXy/g -e s/YY/YxY/g y.tab.c >simpar.c
	sed -e s/yy/yXy/g -e s/YY/YxY/g y.tab.h >simpar.h
	rm -f y.tab.c
	rm -f y.tab.h


simlex.c:	simlex.l simpar.h
	lex simlex.l
	sed -e s/yy/yXy/g -e s/YY/YxY/g lex.yy.c >simlex.c
	rm -f lex.yy.c


newconf.c:	newconf.cyy
	sed -e s/yy/yXy/g -e s/YY/YxY/g newconf.cyy >newconf.c

# **DO NOT** edit the file newconf.c.  Instead edit newconf.cyy. The
# above creates newconf.c with the appropriate changes in the yy and YY
# prefixes that yacc uses. This change is needed for applications which
# use yacc internally to avoid confusing symbols from the two invocations
# of yacc.

# Currently we do not use make depend.

