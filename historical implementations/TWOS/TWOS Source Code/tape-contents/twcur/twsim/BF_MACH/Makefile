#	Copyright (C) 1989, 1991, California Institute of Technology.
#		U. S. Government Sponsorship under NASA Contract NAS7-918 
#		is acknowledged.
#

#	makefile.config 8/4/88  8/16/88  1/19/89 4/18/89
#	modified by JJW 3/6/90 for tw2.4 simulator
#	modified by JJW 11/1/91 for tw2.6 simulator
DESTDIR=
INCDIR= ../../tw
CFLAGS=	 -O -X18  -I$(INCDIR) \
	  -DBF_MACH -DSIMULATOR -DFAST_VTIME_MACROS

# -X130 -X140

SPFLAGS=
LFLAGS= -DTWUSRLIB
 
LIB_SRC = ../../twulib
LIB_DIR = ../../twulib/BF_MACH

VPATH = ..

# tlib for BF_MACH only, some applications need it and TW has similar one.

CFILES      =   version.c\
		newconf.c\
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
		cubeio.c	\
		faults.c	\
		simmem.c \
		tlib.c



OBJS          = version.o\
		twsp1.o\
		twsp2.o\
		twsp3.o\
		twhelp.o\
		twsd.o\
		stdout.o\
		simlex.o\
		simpar.o\
		itimer.o \
		vtime.o \
		twqueues.o \
		cubeio.o \
		oldtw.o \
		HOST_fileio.o \
		BBNclock.o \
                BF_MACHcopy.o\
                BF_MACHcomp.o\
                BF_MACHclear.o		 \
		faults.o	\
		simmem.o \
		tlib.o

# objects  for standalone config language tester

OBJS2        =  newconf.o \
		simlex.o\
		simpar.o\
		vtime.o\
		HOST_fileio.o

NONLIB_OBJS =			\
	newconf.o

LIB_CODE =		        \
    $(LIB_SRC)/twpackages.c    \
    $(LIB_SRC)/TWULarray.c \
    $(LIB_SRC)/outList.c \
    $(LIB_DIR)/TWULfastcpy.s   \
    $(LIB_SRC)/twusrlib.c       \
    $(LIB_SRC)/evtList.c   \
    $(LIB_SRC)/msgList.c   \
    $(LIB_SRC)/multiPacket.c    \
    newconf.z

LIB_OBJS =				\
    $(LIB_DIR)/twpackages.o    \
    $(LIB_DIR)/TWULarray.o \
    $(LIB_DIR)/outList.o \
    $(LIB_DIR)/TWULfastcpy.o   \
    $(LIB_DIR)/twusrlib.o       \
    $(LIB_DIR)/evtList.o   \
    $(LIB_DIR)/msgList.o   \
    $(LIB_DIR)/multiPacket.o    \
	newconf.z


SFILES =        BF_MACHcopy.s \
		BF_MACHclear.s \
		BF_MACHcomp.s


all:	twsim 


twsim:	$(PARALLEL)	$(OBJS) $(NONLIB_OBJS)
	ar rcv twsim.a $(OBJS) $(NONLIB_OBJS)
	ranlib twsim.a

twsiml: $(OBJS) $(LIB_CODE)
	cd $(LIB_DIR); rm -f *.o; make -f Makefile.twsim
	make newconf.z
	ar rcv twsiml.a $(OBJS) $(LIB_OBJS)
	ranlib twsiml.a

dbgsim:	$(OBJS)
	ar rcv dbgsim.a $(OBJS)
	ranlib dbgsim.a


conftest:	$(OBJS2)
	cc -o conftest $(CFLAGS) $(OBJS2)

clean:
	rm -f *.o simlex.c simpar.c simpar.h y.output

.s.o:
	gas -o $*.o $*.s

.c.a:
	$(AR) $(ARFLAGS) $@ $%

.s.a:
	$(AR) $(ARFLAGS) $@ $%

simpar.c + simpar.h:	simpar.y
	yacc -d simpar.y
	sed -e s/yy/yXy/g -e s/YY/YxY/g y.tab.c >../simpar.c
	sed -e s/yy/yXy/g -e s/YY/YxY/g y.tab.h >../simpar.h
	rm -f y.tab.c


simlex.c:	simlex.l simpar.h
	lex simlex.l
	sed -e s/yy/yXy/g -e s/YY/YxY/g lex.yy.c >../simlex.c
	rm -f lex.yy.c

newconf.z:	newconf.c
	cc $(CFLAGS) $(LFLAGS) -c ../newconf.c
	mv newconf.o newconf.z


newconf.c:	newconf.cyy
	sed -e s/yy/yXy/g -e s/YY/YxY/g newconf.cyy >../newconf.c

# DO NOT edit the file newconf.c.  Instead edit newconf.cyy. The
# above creates newconf.c with the appropriate changes in the yy and YY
# prefixes that yacc uses.


libsim:	
	make -f Make.bbn

# Before making libsim, see that simlex.c, simpar.c, simpar.tab,
# and newconf.c exists and are up to date.
# make libsim will first remove ALL *.o68 files. 
# make -f Make.bbn libsim.a will update the appropriate files only. If you
# use this make sure the .o68 have not been made for modsim.

