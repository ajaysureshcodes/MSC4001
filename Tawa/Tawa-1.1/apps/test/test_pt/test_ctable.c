/* Test program. */
#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>

#ifdef SYSTEM_LINUX
#include <getopt.h> /* for getopt on Linux systems */
#endif

#include "io.h"
#include "text.h"
#include "model.h"
#include "pt_ctable.h"


int
main()
{
    boolean new;
    unsigned int word, context;
    unsigned int key;
    unsigned int lbnd, hbnd, totl, target, totl1, freq;
    struct PTc_table_type Table;

    word = TXT_create_text ();
    context = TXT_create_text ();

    PTc_init_table (&Table);
    for (;;)
      {
        printf ("context? ");
	if (TXT_readline_text (Stdin_File, context) <= 0)
	  break;

        printf ("word? ");
	if (TXT_readline_text (Stdin_File, word) <= 0)
	  break;

	printf ("freq? ");
	scanf ("%d", &freq);

	new = PTc_update_table (&Table, context, NULL, word, freq);
	printf ("new = %d\n", new);

	PTc_dump_table (Stdout_File, &Table);

	PTc_encode_arith_range (&Table, NULL, context, word, &lbnd, &hbnd, &totl);
	printf ("lbnd = %d hbnd = %d totl = %d\n", lbnd, hbnd, totl);
	totl1 = PTc_decode_arith_total (&Table, NULL, context);
	printf ("totl = %d\n", totl1);
	for (target=0; target<totl1; target++) {
	    key = PTc_decode_arith_key (&Table, NULL, context, target, totl1, &lbnd, &hbnd);
	    printf ("lbnd = %d hbnd = %d totl = %d ", lbnd, hbnd, totl1);
	    if (key == NIL)
	        printf ("key = <ESCAPE>\n");
	    else
	      {
	        printf ("key = ");
		TXT_dump_text (Stdout_File, key, NULL);
		printf ("\n");
	      }
	}
      }

    return (1);
}
