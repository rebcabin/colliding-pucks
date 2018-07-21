
SIMDIR	   = ../../../twsim/Sun4
TWDIR	= ../../../tw/Sun4
INCDIR	= ../../../tw




CIRCLESDIR	= ../circles
CIRCLESLINKDIR	= $(CIRCLESDIR)/Sun4
SOURCEDIR	=..




IFLAGS	      =  -DSUN -DFAST_VTIME_MACROS \
		 -I$(INCDIR) -I$(CIRCLESDIR)

CFLAGS	      =   -O 

 


OBJS = \
		pucktab.o\
                puck.o\
		cushion.o\
		sector.o\
		critsim.o


DEBUGOBJS	=  \
		p_debug.o


SIMOBJS	      = \
		$(SIMDIR)/simconfig.o\
		$(SIMDIR)/stdine.o\
		$(SIMDIR)/twsd.o\
		$(SIMDIR)/twsp1.o\
		$(SIMDIR)/twsp2.o\
		$(SIMDIR)/long.o\
		$(SIMDIR)/twerror.o\
		$(SIMDIR)/twhelp.o\
		$(SIMDIR)/float.o\
		$(SIMDIR)/stdout.o\
		$(SIMDIR)/itimer.o	\
		$(SIMDIR)/SUNclear.o\
		$(SIMDIR)/copy.o

SIMLIB   = $(SIMDIR)/twsim.a
TWLIB	=  $(TWDIR)/tw.a

all:	pucks	puckstw


pucks:		$(OBJS) $(DEBUGOBJS) $(CIRCLESLINKDIR)/circles.o 
		cc $(CFLAGS) $(IFLAGS)  -o pucks	\
		$(OBJS)					\
		$(DEBUGOBJS)			\
		$(CIRCLESLINKDIR)/circles.o		\
		$(SIMLIB)			\
		-lm	

puckstw:		$(OBJS) $(DEBUGOBJS) $(CIRCLESLINKDIR)/circles.o 
		cc $(CFLAGS)  $(IFLAGS)  -o puckstw	\
		$(OBJS)					\
		$(DEBUGOBJS)			\
		$(CIRCLESLINKDIR)/circles.o		\
		$(TWLIB)			\
		-lm	


.c.o:
	cc $(CFLAGS) $(IFLAGS) -c $*.c

%.o: ../%.c
	cc $(CFLAGS) $(IFLAGS) -c ../$*.c
