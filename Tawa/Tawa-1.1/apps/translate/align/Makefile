include ../../../CONFIG

align: align.o aqueue.o
	$(CC) $(CFLAGS) -o align align.o aqueue.o $(LIBS)

align2: align2.o
	$(CC) $(CFLAGS) -o align2 align2.o $(LIBS)

a2: a2.o
	$(CC) $(CFLAGS) -o a2 a2.o $(LIBS)

editdistance: editdistance.o
	$(CC) $(CFLAGS) -o editdistance editdistance.o $(LIBS)

editdistance1: editdistance1.o
	$(CC) $(CFLAGS) -o editdistance1 editdistance1.o $(LIBS)

editdistance2: editdistance2.o
	$(CC) $(CFLAGS) -o editdistance2 editdistance2.o $(LIBS)

editdistance3: editdistance3.o
	$(CC) $(CFLAGS) -o editdistance3 editdistance3.o $(LIBS)


queue:	aqueue.c
	$(CC) $(CFLAGS) -c aqueue.c

clean:
	rm -f *.o align *~
