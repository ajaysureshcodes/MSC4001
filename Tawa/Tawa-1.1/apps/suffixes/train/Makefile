include ../../../CONFIG
DEMO_DATA = ../../../data/suffixes

train_suffixes: train_suffixes.o
	$(CC) $(CFLAGS) -o train_suffixes train_suffixes.o $(LIBS)

demo:
	./train_suffixes -i $(DEMO_DATA)/A.string -o $(DEMO_DATA)/A.model
	./train_suffixes -i $(DEMO_DATA)/S.string -o $(DEMO_DATA)/S.model
	./train_suffixes -i $(DEMO_DATA)/S1.string -o $(DEMO_DATA)/S1.model 
	./train_suffixes -N -i $(DEMO_DATA)/N.string -o $(DEMO_DATA)/N.model 
	./train_suffixes -N -i $(DEMO_DATA)/N1.string -o $(DEMO_DATA)/N1.model 

clean:
	rm -f *.o train_suffixes *~
