include ../../CONFIG

precision:	precision.o 
	$(CC) $(CFLAGS) -o precision precision.o $(LIBS)
precision1:	precision1.o 
	$(CC) $(CFLAGS) -o precision1 precision1.o $(LIBS)
p:	p.o 
	$(CC) $(CFLAGS) -o p p.o $(LIBS)
p1:	p1.o 
	$(CC) $(CFLAGS) -o p1 p.o $(LIBS)

precision_word:	precision_word.o 
	$(CC) $(CFLAGS) -o precision_word precision_word.o $(LIBS)
precision_word1:	precision_word1.o 
	$(CC) $(CFLAGS) -o precision_word1 precision_word1.o $(LIBS)
p_word:	p_word.o 
	$(CC) $(CFLAGS) -o p_word p_word.o $(LIBS)
p1_word:	p1_word.o 
	$(CC) $(CFLAGS) -o p1_word p_word.o $(LIBS)

clean:
	rm -f junk* *.o p p1 p_word p1_word precision precision1 precision_word precision1_word
