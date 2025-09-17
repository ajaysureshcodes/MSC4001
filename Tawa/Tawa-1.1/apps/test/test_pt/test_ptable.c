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
#include "pt_ptable.h"

unsigned int File0, File1;

void
usage (void)
{
    fprintf (stderr,
	     "Usage: ident_file [options]\n"
	     "\n"
	     "options:\n"
	     "  -0 n\torder 0 words(required)\n"
	     "  -1 n\torder 1 words(required)\n"
	);
    exit (2);
}

void
init_arguments (int argc, char *argv[])
{
    extern char *optarg;
    extern int optind;
    boolean file0_found;
    boolean file1_found;
    int opt;

    /* get the argument options */

    file0_found = FALSE;
    file1_found = FALSE;
    while ((opt = getopt (argc, argv, "0:1:")) != -1)
	switch (opt)
	{
	case '0':
	    File0 = TXT_open_file (optarg, "r", NULL,
				   "Error opening file 0");
	    file0_found = TRUE;
	    break;
	case '1':
	    File1 = TXT_open_file (optarg, "r", NULL,
				   "Error opening file 0");
	    file1_found = TRUE;
	    break;
	default:
	    usage ();
	    break;
	}
    if (!file0_found || !file1_found)
    {
        fprintf (stderr, "\nFatal error: missing exclusions file\n\n");
        usage ();
    }
    for (; optind < argc; optind++)
	usage ();
}

int
main (int argc, char *argv[])
{
    unsigned int word, key, total, lbnd1, hbnd1, target;
    unsigned int freq_text, freq;
    unsigned int lbnd, hbnd, totl;
    struct PTp_table_type table0, table1;

    init_arguments (argc, argv);

    PTp_init_table (&table0);
    PTp_init_table (&table1);

    word = TXT_create_text ();
    freq_text = TXT_create_text ();

    while ((TXT_readline_text (File0, word) > 0) && (TXT_readline_text (File0, freq_text) > 0))
      {
	TXT_scanf_text (freq_text, "%d", &freq);
	/*
	fprintf (stderr, "Text = ");
	TXT_dump_text (Stderr_File, word, NULL);
	fprintf (stderr, ", freq = %d\n", freq);
	*/
	PTp_update_table (&table0, word, freq);
      }
    PTp_dump_table (Stdout_File, &table0);

    while ((TXT_readline_text (File1, word) > 0) && (TXT_readline_text (File1, freq_text) > 0))
      {
	TXT_scanf_text (freq_text, "%d", &freq);
	/*
	fprintf (stderr, "Text = ");
	TXT_dump_text (Stderr_File, word, NULL);
	fprintf (stderr, ", freq = %d\n", freq);
	*/
	PTp_update_table (&table1, word, freq);
      }
    /* PTp_dump_table (Stdout_File, &table1); */

    PTp_check_arith_ranges (&table0, &table1, FALSE);

    for (;;)
      {
        printf ("word? ");
	if (TXT_readline_text (Stdin_File, word) <= 0)
	  break;

	PTp_encode_arith_range (&table0, &table1, word, &lbnd, &hbnd, &totl);
	printf ("lbnd = %d hbnd = %d totl = %d\n", lbnd, hbnd, totl);

	total = PTp_decode_arith_total (&table0, &table1);
	printf ("total = %d totl = %d\n", total, totl);


        printf ("target? ");
	scanf ("%u", &target);

	key = PTp_decode_arith_key (&table0, &table1, target, total, &lbnd1, &hbnd1);
	printf ("key = ");
	TXT_dump_text (Stdout_File, key, NULL);
	printf (", lbnd = %d hbnd = %d totl = %d\n", lbnd1, hbnd1, totl);
      }

    /*

    word = TXT_create_text ();

    PTp_init_table (&Table);
    for (;;)
      {
        printf ("word? ");
	if (TXT_readline_text (Stdin_File, word) <= 0)
	  break;

	printf ("freq? ");
	scanf ("%d", &freq);

	new = PTp_update_table (&Table, word, freq);
	printf ("new = %d\n", new);

	PTp_dump_table (Stdout_File, &Table);

	PTp_encode_arith_range (&Table, word, &lbnd, &hbnd, &totl);
	printf ("lbnd = %d hbnd = %d totl = %d\n", lbnd, hbnd, totl);
	totl1 = PTp_decode_arith_total (&Table);
	printf ("totl = %d\n", totl1);
	for (target=0; target<totl1; target++) {
	    key = PTp_decode_arith_key (&Table, target, totl1, &lbnd, &hbnd);
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
    */

    exit (1);
}
