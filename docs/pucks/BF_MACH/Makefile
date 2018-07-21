
SIMDIR	   = ../../../twsim/BF_MACH
TWDIR	= ../../../tw/BF_MACH
INCDIR	= ../../../tw




CIRCLESDIR	= ../circles
CIRCLESLINKDIR	= $(CIRCLESDIR)/BF_MACH
SOURCEDIR	=..




IFLAGS	      =  -DBF_MACH -DFAST_VTIME_MACROS \
		 -I$(INCDIR) -I$(CIRCLESDIR)

CFLAGS	      =   -O  -X130 
LIB_FLAGS     =
VPATH	= ..

 


OBJS = \
		pucktab.o\
                puck.o\
		cushion.o\
		sector.o\
		critsim.o


DEBUGOBJS	=  \
		p_debug.o


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
		cc $(CFLAGS) $(LIB_FLAGS)  $(IFLAGS)  -o puckstw	\
		$(OBJS)					\
		$(DEBUGOBJS)			\
		$(CIRCLESLINKDIR)/circles.o		\
		$(TWLIB)			\
		-lm	


.c.o:
	cc $(CFLAGS) $(IFLAGS) -c $*.c

