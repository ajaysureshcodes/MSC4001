include ../../CONFIG

load_table:	load_table.o
	$(CC) $(CFLAGS) -o load_table load_table.o $(LIBS)

dump_table:	dump_table.o
	$(CC) $(CFLAGS) -o dump_table dump_table.o $(LIBS)

clean:
	rm -f junk* *.o decode_model encode_model test1 test_rules test_pgg test_sets test_table test_text test_track test_pcfg
