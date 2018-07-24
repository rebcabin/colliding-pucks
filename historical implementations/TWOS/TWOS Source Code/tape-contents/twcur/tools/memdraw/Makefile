CC = cc

CFLAGS = -f68881 -O -DXVIEW -DX11 -L/u1f/xview/xview2/lib/libxview \
		 -L/u1f/xview/xview2/lib/libolgx -g
IFLAGS = -I.. -I/u1f/xview/xview2/usr_latest/usr/include

OBJS = ident.o indata.o magnify.o main.o menu.o text.o zoom.o sleep.o \
	event.o color.o page.o sizes.o color_settings.o


memdraw: $(OBJS)
	rm -f memdraw
	$(CC) $(CFLAGS) $(OBJS) -o memdraw -lxview -lolgx -lX11 -lm


.c.o:
	$(CC) $(CFLAGS)  -c $<

%.o: %.c
	cc $(CFLAGS) $(IFLAGS) -c $*.c


ident.o indata.o magnify.o main.o menu.o text.o zoom.o sleep.o \
	event.o color.o page.o sizes.o color_settings.o : mplot.h


