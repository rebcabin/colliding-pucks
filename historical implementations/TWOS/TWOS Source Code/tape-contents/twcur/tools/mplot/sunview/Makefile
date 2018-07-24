CC = cc

CFLAGS = -f68881 -O -DSUNVIEW -Bstatic
IFLAGS = -I..

OBJS = ident.o indata.o magnify.o main.o menu.o text.o zoom.o sleep.o \
	event.o color.o page.o


mplot: $(OBJS)
	rm -f mplot
	$(CC) $(CFLAGS) $(OBJS) -o mplot -lsuntool -lsunwindow -lpixrect -lm


.c.o:
	$(CC) $(CFLAGS)  -c $<

%.o: ../%.c
	cc $(CFLAGS) $(IFLAGS) -c ../$*.c


ident.o indata.o magnify.o main.o menu.o text.o zoom.o sleep.o \
	event.o color.o page.o: ../mplot.h


