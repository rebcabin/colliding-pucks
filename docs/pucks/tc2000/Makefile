
SIMDIR	   = ../../twsim
TWDIR	= ../../tw
INCDIR	= ../../tw


CIRCLESDIR	= circles




IFLAGS	      =  -DBF_MACH  \
		 -I$(INCDIR) -I$(CIRCLESDIR)

CFLAGS	      =   -O -X18 -X39 -DSIMULATOR -DBF_MACH
LIB_FLAGS     =  

 


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


pucks:		$(OBJS) $(DEBUGOBJS) $(CIRCLESDIR)/circles.o 
		cc $(CFLAGS) $(IFLAGS)  -o pucks	\
		$(OBJS)					\
		$(DEBUGOBJS)			\
		$(CIRCLESDIR)/circles.o		\
		$(SIMLIB)			\
		-lm	

puckstw:		$(OBJS) $(DEBUGOBJS) $(CIRCLESDIR)/circles.o 
		cc $(CFLAGS) $(LIB_FLAGS)  $(IFLAGS)  -o puckstw	\
		$(OBJS)					\
		$(DEBUGOBJS)			\
		$(CIRCLESDIR)/circles.o		\
		$(TWLIB)			\
		-lm	


.c.o:
	cc $(CFLAGS) $(IFLAGS) -c $*.c
