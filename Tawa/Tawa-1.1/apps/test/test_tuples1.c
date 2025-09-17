/* Test program. */
#include <stdio.h>
#include <stdlib.h>

#ifdef SYSTEM_LINUX
#include <getopt.h> /* for getopt on Linux systems */
#endif

#include "io.h"
#include "model.h"
#include "sets.h"
#include "text.h"
#include "table.h"
#include "tuples.h"

int
main()
{
    unsigned int n, k, textpos, nonword_textpos, word_textpos, seed;
    unsigned int nonword, word;
    unsigned int tuple_set, *tuple_set_members;
    unsigned int tuples, tuple_id;
    unsigned int file, document;

    seed = time (NULL);
    fprintf (stderr, "seed = %d\n", seed);
    srandom (seed);

    tuples = TXT_create_tuples ();

    word = TXT_create_text ();
    nonword = TXT_create_text ();

    TXT_init_sets (32, 4, 16, TRUE);

    file = TXT_open_file ("test.txt", "r", NULL, NULL);
    document = TXT_load_text (file);

    textpos = 0;
    while (TXT_getword_text (document, nonword, word, &textpos, &nonword_textpos, &word_textpos))
      {
	fprintf (stdout, "Word = ");
	TXT_dump_text (Stdout_File, word, NULL);
	fprintf (stdout, "\n");

	tuple_set = TXT_start_query (tuples, word);
	/*tuple_set = TXT_refine_query (tuples, tuple_set, word);*/

	fprintf (stderr, "\nTuple set: ");
	TXT_dump_set (Stderr_File, tuple_set);
	fprintf (stderr, "\n");

	tuple_set_members = TXT_get_setmembers (tuple_set);

	TXT_dump_setmembers (Stderr_File, tuple_set_members);
	TXT_release_setmembers (tuple_set_members);

	if (tuple_set_members && tuple_set_members [0])
	    tuple_id = tuple_set_members [1];
	else
	  { /* found a new word */
	    tuple_id = TXT_create_tuple (tuples);
	    TXT_addkey_tuple (tuples, tuple_id, word);
	  }

	for (k = 0; k < 4; k++)
	  {
	    n = random ();
            if (SetMaxCard)
              n %= SetMaxCard;

	    TXT_update_tuple (tuples, tuple_id, n);
	  }
      }

    TXT_dump_tuples (Stderr_File, tuples, NULL);

    exit (1);
}
