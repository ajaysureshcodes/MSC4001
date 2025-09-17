include ../../../CONFIG
DATA = ../../../data
DEMO_DATA = ../../../data/suffixes

generate_counts: generate_counts.o
	$(CC) $(CFLAGS) -o generate_counts generate_counts.o $(LIBS)

Word_numbers: Word_numbers.o
	$(CC) $(CFLAGS) -o Word_numbers Word_numbers.o $(LIBS)

ReplaceSpace: ReplaceSpace.o
	$(CC) $(CFLAGS) -o ReplaceSpace ReplaceSpace.o $(LIBS)

list_types: list_types.o
	$(CC) $(CFLAGS) -o list_types list_types.o $(LIBS)

demo:
	./generate_counts -1 $(DEMO_DATA)/S.model -2 $(DEMO_DATA)/S1.model -o demo.lis
	./generate_counts -1 $(DEMO_DATA)/N.model -2 $(DEMO_DATA)/N1.model -o demo.lis.1
	diff demo.lis demo.lis.1

demo1:
	./list_types -m $(DEMO_DATA)/A.model -o demo.lis -Z -L -1 5
	./list_types -m $(DEMO_DATA)/S.model -o demo1.lis -Z -L -1 5

demo2:
	./ReplaceSpace <$(DATA)/bible/english.txt >english.words
	./Word_numbers -i english.words -o english.numbers
	../train/train_suffixes -N -i english.numbers -o english.model
	./list_types -m english.model -o demo2.unigrams -Z -1 1
	./list_types -m english.model -o demo2.bigrams -Z -1 2
	./list_types -m english.model -o demo2.trigrams -Z -1 3

clean:
	rm -f *.o generate_counts *~
