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
    unsigned int key, attribute, relation, tuple_id;
    unsigned int n, n1, k;
    unsigned int seed, count, total;
    unsigned int tuple_set, query_set;
    unsigned int tuples;
    unsigned int file, file1;

    seed = time (NULL);
    fprintf (stderr, "seed = %d\n", seed);
    srandom (seed);

    key = TXT_create_text ();
    attribute = TXT_create_text ();
    relation = TXT_create_text ();

    TXT_init_sets (32, 4, 16, TRUE);

    TXT_set_file (File_Ignore_Errors);
    file1 = TXT_open_file ("junk.tuples.1", "r", NULL, NULL);
    if (!file1)
        tuples = TXT_create_tuples ();
    else
      {
	tuples = TXT_load_tuples (file1);
	TXT_close_file (file1);
	TXT_dump_tuples (Stderr_File, tuples, TXT_dump_relationtext);
      }

    total = 0;
    for (;;)
      {
	count = 0;
	tuple_id = 0;
	for (;;)
	  {
	    printf ("Input key: ");
	    if (TXT_readline_text (Stdin_File, key) <= 0)
	        break;
	    printf ("Input attribute: ");
	    if (TXT_readline_text (Stdin_File, attribute) <= 0)
	        break;
    
	    if (!count)
	        tuple_id = TXT_create_tuple (tuples);
	    TXT_put_relationtext (relation, key, attribute);
	    TXT_addkey_tuple (tuples, tuple_id, relation);
	    count++;
	  }

	if (!count)
	    break;
	total += count;

        n = random ();
        n %= 20;

	fprintf (stderr, "Adding approx. %d elements to tuple %d\n", n, tuple_id);

	for (k = 0; k < n; k++)
	  {
	    n1 = random ();
            if (SetMaxCard)
              n1 %= SetMaxCard;

	    TXT_update_tuple (tuples, tuple_id, n1);
	  }
      }

    total = 0;
    for (;;)
      {
	TXT_dump_tuples (Stderr_File, tuples, TXT_dump_relationtext);

	count = 0;
	tuple_id = 0;
	tuple_set = NULL;
	query_set = NULL;

	for (;;)
	  {
	    printf ("\nInput query key: ");
	    if (TXT_readline_text (Stdin_File, key) <= 0)
	        break;
	    printf ("Input query attribute: ");
	    if (TXT_readline_text (Stdin_File, attribute) <= 0)
	        break;

	    TXT_put_relationtext (relation, key, attribute);
	    if (!count)
	        tuple_set = TXT_start_query (tuples, relation);
	    else
	        tuple_set = TXT_refine_query (tuples, tuple_set, relation);

	    fprintf (stderr, "\nTuple set: ");
	    TXT_dump_set (Stderr_File, tuple_set);
	    fprintf (stderr, "\n");

	    count++;
	  }

	if (!count)
	    break;
	total += count;

	query_set = TXT_complete_query (tuples, tuple_set);

	fprintf (stderr, "List of matching candidates: ");
	TXT_dump_set (Stderr_File, query_set);
	fprintf (stderr, "\n");
      }

    TXT_set_file (File_Exit_On_Errors);
    file = TXT_open_file ("junk.tuples.1", "w", NULL,
			  "Test: can't open tuples file");
    if (!file)
      {
	fprintf (stderr, "Test: can't open tuples file\n");
	exit (1);
      }
    TXT_write_tuples (file, tuples);
    TXT_close_file (file);

    exit (1);
}
