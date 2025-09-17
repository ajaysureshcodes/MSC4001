include ../../../CONFIG

tag: tag.o
	$(CC) $(CFLAGS) -o tag tag.o $(LIBS)

tag_eval: tag_eval.o
	$(CC) $(CFLAGS) -o tag_eval tag_eval.o $(LIBS)

train_tag: train_tag.o
	$(CC) $(CFLAGS) -o train_tag train_tag.o $(LIBS)

encode_tag: encode_tag.o
	$(CC) $(CFLAGS) -o encode_tag encode_tag.o $(LIBS)

decode_tag: decode_tag.o
	$(CC) $(CFLAGS) -o decode_tag decode_tag.o $(LIBS)

clean:
	rm -f *.o *~
