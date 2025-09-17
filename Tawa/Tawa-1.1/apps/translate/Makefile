include ../../CONFIG

align: align.o
	$(CC) $(CFLAGS) -o align align.o $(LIBS)

align_delete: align_delete.o
	$(CC) $(CFLAGS) -o align_delete align_delete.o $(LIBS)

parse: parse.o
	$(CC) $(CFLAGS) -o parse parse.o $(LIBS)

realign: realign.o
	$(CC) $(CFLAGS) -o realign realign.o $(LIBS)

misalign: misalign.o
	$(CC) $(CFLAGS) -o misalign misalign.o $(LIBS)

clean:
	rm -f *.o align align_delete *~
