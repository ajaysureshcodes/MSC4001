include ../../CONFIG

encode_model:	encode_model.o SSS_coder.o
	$(CC) $(CFLAGS) -o encode_model encode_model.o SSS_coder.o $(LIBS)


decode_model:	decode_model.o SSS_coder.o
	$(CC) $(CFLAGS) -o decode_model decode_model.o SSS_coder.o $(LIBS)

SSS_coder:	SSS_coder.o
	$(CC) $(CFLAGS) -o SSS_coder SSS_coder.o $(LIBS)

test1:	test1.o
	$(CC) $(CFLAGS) -o test1 test1.o $(LIBS)

test_load:	test_load.o
	$(CC) $(CFLAGS) -o test_load test_load.o $(LIBS)

test_cache:	test_cache.o
	$(CC) $(CFLAGS) -o test_cache test_cache.o $(LIBS)

test_coderanges:	test_coderanges.o
	$(CC) $(CFLAGS) -o test_coderanges test_coderanges.o $(LIBS)

test_index:	test_index.o
	$(CC) $(CFLAGS) -o test_index test_index.o $(LIBS)

test_list:	test_list.o
	$(CC) $(CFLAGS) -o test_list test_list.o $(LIBS)

demo_load:	test_load
	test_load -m ../../models/english.model -e 10

test_dump:	test_dump.o
	$(CC) $(CFLAGS) -o test_dump test_dump.o $(LIBS)

test_rules:	test_rules.o
	$(CC) $(CFLAGS) -o test_rules test_rules.o $(LIBS)

test_pcfg:	test_pcfg.o
	$(CC) $(CFLAGS) -o test_pcfg test_pcfg.o $(LIBS)

test_pgg:	test_pgg.o
	$(CC) $(CFLAGS) -o test_pgg test_pgg.o $(LIBS)

test_check:	test_check.o
	$(CC) $(CFLAGS) -o test_check test_check.o $(LIBS)

test_sets:	test_sets.o
	$(CC) $(CFLAGS) -o test_sets test_sets.o $(LIBS)

test_table:	test_table.o
	$(CC) $(CFLAGS) -o test_table test_table.o $(LIBS)

test_track:	test_track.o
	$(CC) $(CFLAGS) -o test_track test_track.o $(LIBS)

test_tuples:	test_tuples.o
	$(CC) $(CFLAGS) -o test_tuples test_tuples.o $(LIBS)

test_tuples1:	test_tuples1.o
	$(CC) $(CFLAGS) -o test_tuples1 test_tuples1.o $(LIBS)

test_text:	test_text.o
	$(CC) $(CFLAGS) -o test_text test_text.o $(LIBS)

test_text1:	test_text1.o
	$(CC) $(CFLAGS) -o test_text1 test_text1.o $(LIBS)

test_user:	test_user.o
	$(CC) $(CFLAGS) -o test_user test_user.o $(LIBS)

test_generate:	test_generate.o
	$(CC) $(CFLAGS) -o test_generate test_generate.o $(LIBS)

test_judgments:	test_judgments.o
	$(CC) $(CFLAGS) -o test_judgments test_judgments.o $(LIBS)

vectors:	vectors.o
	$(CC) $(CFLAGS) -c vectors.c

sets:	vectors.o
	$(CC) $(CFLAGS) -o sets sets.c vectors.o

xml:	xml.o
	$(CC) $(CFLAGS) -o xml xml.o $(LIBS)

clean:
	rm -f junk* *.o decode_model encode_model test1 test_rules test_pgg test_check test_sets test_table test_text test_generate test_track test_pcfg xml
