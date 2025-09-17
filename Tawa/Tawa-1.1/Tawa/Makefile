include ../CONFIG
AR = ar
RANLIB = :

libTawa.a: model.o ppm_model.o ppmo_model.o ppmq_model.o ppm_context.o ppm_trie.o ppm_cache.o ptable.o pt_model.o pt_ptable.o pt_ctable.o tag_model.o cpt_model.o cpt_ptable.o cpt_ctable.o samm_model.o hmm_model.o sss_model.o tag_model.o user_model.o transform.o paths.o confusion.o hash.o tuples.o track.o list.o index.o table.o word.o text.o coderanges.o io.o sets.o judgments.o rules.o pcfg.o coder.o SM_model.o
	$(AR) r $@ model.o ppm_model.o ppmo_model.o ppmq_model.o ppm_context.o ppm_trie.o ppm_cache.o ptable.o pt_model.o pt_ptable.o pt_ctable.o tag_model.o cpt_model.o cpt_ptable.o cpt_ctable.o samm_model.o hmm_model.o sss_model.o tag_model.o user_model.o transform.o paths.o confusion.o hash.o tuples.o track.o list.o index.o table.o word.o text.o coderanges.o io.o sets.o judgments.o rules.o pcfg.o coder.o SM_model.o
	$(RANLIB) $@

clean:
	rm -f *.o *~ *.a bits_test

#coderanges:	coderanges.o
#	$(CXX) $(CXXFLAGS) -o coderanges coderanges.o

backup:
	cp model.c model.c.bak
	cp model.h model.h.bak
	cp ppm_model.c ppm_model.c.bak
	cp ppm_model.h ppm_model.h.bak
	cp ppmq_model.c ppmq_model.c.bak
	cp ppmq_model.h ppmq_model.h.bak
	cp ppmo_model.c ppmo_model.c.bak
	cp ppmo_model.h ppmo_model.h.bak
	cp ppm_context.c ppm_context.c.bak
	cp ppm_context.h ppm_context.h.bak
	cp ppm_trie.c ppm_trie.c.bak
	cp ppm_trie.h ppm_trie.h.bak
	cp ppm_cache.c ppm_cache.c.bak
	cp ppm_cache.h ppm_cache.h.bak
	cp ptable.c ptable.c.bak
	cp ptable.h ptable.h.bak
	cp pt_model.c pt_model.c.bak
	cp pt_ptable.c pt_ptable.c.bak
	cp pt_ptable.h pt_ptable.h.bak
	cp tag_model.c tag_model.c.bak
	cp cpt_model.c cpt_model.c.bak
	cp cpt_ptable.c cpt_ptable.c.bak
	cp cpt_ptable.h cpt_ptable.h.bak
	cp samm_model.c samm_model.c.bak
	cp samm_model.h samm_model.h.bak
	cp hmm_model.c hmm_model.c.bak
	cp hmm_model.h hmm_model.h.bak	
	cp sss_model.c sss_model.c.bak
	cp sss_model.h sss_model.h.bak
	cp tag_model.c tag_model.c.bak
	cp tag_model.h tag_model.h.bak
	cp user_model.c user_model.c.bak
	cp user_model.h user_model.h.bak
	cp track.c track.c.bak
	cp track.h track.h.bak
	cp list.h list.h.bak
	cp list.c list.c.bak
	cp index.c index.c.bak
	cp index.h index.h.bak
	cp table.c table.c.bak
	cp table.h table.h.bak
	cp tuples.c tuples.c.bak
	cp tuples.h tuples.h.bak
	cp transform.c transform.c.bak
	cp transform.h transform.h.bak
	cp text.c text.c.bak
	cp text.h text.h.bak
	cp word.c word.c.bak
	cp word.h word.h.bak
	cp coderanges.c coderanges.c.bak
	cp coderanges.h coderanges.h.bak
	cp confusion.c confusion.c.bak
	cp confusion.h confusion.h.bak
	cp hash.c hash.c.bak
	cp hash.h hash.h.bak
	cp io.c io.c.bak
	cp io.h io.h.bak
	cp judgments.c judgments.c.bak
	cp judgments.h judgments.h.bak
	cp rules.c rules.c.bak
	cp rules.h rules.h.bak
	cp pcfg.c pcfg.c.bak
	cp pcfg.h pcfg.h.bak
	cp sets.c sets.c.bak
	cp sets.h sets.h.bak
	cp coder.c coder.c.bak
	cp coder.h coder.h.bak
	cp SM_model.c SM_model.c.bak
	cp SM_model.h SM_model.h.bak

install: libTawa.a
	cp *.a ../lib
	cp *.h ../include
