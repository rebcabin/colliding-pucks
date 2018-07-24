SIMDIR=
INCDIR=

CFLAGS = -DSUN3   -f68881 -O


OBJS   =    pp.o pongtab.o


.KEEP_STATE:

pong112:	 $(OBJS)
	cc $(CFLAGS) -I$(INCDIR)  -O -o pong112  \
	$(OBJS) $(SIMDIR)/twsim.a

pong112 debug:= INCDIR = /usr/local/src/twsim/tw112
pong112 debug:= SIMDIR = /usr/local/src/twsim/tw112


pong110:	$(OBJS)
	cc $(CFLAGS) -I$(INCDIR)  -O  -o pong110 \
	$(OBJS) $(SIMDIR)/twsim110.a


pong110 := INCDIR = /usr/local/src/twsim/tw111
pong110 := SIMDIR = /usr/local/src/twsim/tw111


pong111:	$(OBJS)
	cc $(CFLAGS) -I$(INCDIR)  -O  -o pong111 \
	$(OBJS) $(SIMDIR)/twsim111.a

pong111 := INCDIR = /usr/local/src/twsim/tw111
pong111 := SIMDIR = /usr/local/src/twsim/tw111




debug:	$(OBJS)
	cc $(CFLAGS)  -I$(INCDIR)  -g -o pongdebug  \
	$(OBJS)  $(SIMDIR)/dtwsim.a


profile:	$(OBJS)
	cc $(CFLAGS)  -I$(INCDIR)  -p -O -o pongprofile  \
	$(OBJS)  $(SIMDIR)/ptwsim110.a

gccpong:= CFLAGS= -DSUN3 -Dvax
gccpong:= CC= gcc
gccpong:= INCDIR =/usr/local/src/twsim/tw112
gccpong:= SIMDIR =/usr/local/src/twsim/tw112

gccpong:	$(OBJS)
	$(CC) -o gccpong $(OBJS) $(SIMDIR)/gccsim112.a 

profile:= CFLAGS = -DSUN3  -f68881 -O -p 

debug:= CFLAGS = -DSUN3  -f68881 -g

.c.o:
	$(CC) $(CFLAGS) -I$(INCDIR) -c $<

