OBJS   = cg.o gridmap.o gmap.o 

cg:	$(OBJS)
	cc $(OBJS) -lm -o cg

cgtest:	$(OBJS)
	cc $(OBJS) -lm -o cgtest

clean:
	rm -f $(OBJS) y.tab.[ch]
