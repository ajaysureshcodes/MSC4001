/* Replaces spaces with eolns. */

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

void load_source_file(FILE *fp)
/* load file FP */
{
    int cc;

    while ((cc = getc(fp)) != EOF) {
	if (cc != ' ')
	    putchar( cc );
	else
	    putchar ('\n');
    }
}

int
main()
{
     load_source_file( stdin );

     exit (0);
}
