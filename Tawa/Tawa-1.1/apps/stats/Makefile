include ../../CONFIG

all: stats

stats: stats.o 
	$(CC) $(CFLAGS) -o stats stats.o $(LIBS)

clean:
	rm -f *.o stats *~

