
all:
	(cd classify; make -f Makefile)
	(cd encode; make -f Makefile)
	(cd transform; make -f Makefile)
	(cd precision; make -f Makefile)
	(cd stats; make -f Makefile)
	(cd test; make -f Makefile)
	(cd train; make -f Makefile)

clean:
	(cd classify; make -f Makefile clean)
	(cd encode; make -f Makefile clean)
	(cd transform; make -f Makefile clean)
	(cd precision; make -f Makefile clean)
	(cd stats; make -f Makefile clean)
	(cd test; make -f Makefile clean)
	(cd train; make -f Makefile clean)

demo:
	(cd encode; make -f Makefile demo)
