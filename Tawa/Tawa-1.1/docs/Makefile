docs:	TMT.ps
	(mgv TMT.ps)

TMT.dvi:	TMT.tex
	(latex TMT.tex)
	(latex TMT.tex) # Do it again to get rid of any undefined refs

TMT.ps:	TMT.dvi
	(dvips -o TMT.ps TMT.dvi)

dcc99:	dcc99.ps
	(mgv dcc99.ps)

dcc99.dvi:	dcc99.tex
	(latex dcc99.tex)
	(latex dcc99.tex) # Do it again to get rid of any undefined refs

dcc99.ps:	dcc99.dvi
	(dvips -o dcc99.ps dcc99.dvi)


clean:
	(rm -f *.log *.dvi *.aux)
	(find . -name "*~" -exec rm {} \;)
