/* Test program for witing out a ptable. */
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
#include "pt_ctable.h"

boolean Dump_table = FALSE;

void
usage (void)
{
    fprintf (stderr,
	     "Usage: ident_file [options]\n"
	     "\n"
	     "options:\n"
	     "  -d\tdump out model (for debugging) instead of writing it out\n"

	);
    exit (2);
}

void
init_arguments (int argc, char *argv[])
{
    extern char *optarg;
    extern int optind;
    int opt;

    /* get the argument options */

    while ((opt = getopt (argc, argv, "d")) != -1)
	switch (opt)
	{
	case 'd':
	    Dump_table = TRUE;
	    break;
	default:
	    usage ();
	    break;
	}
    for (; optind < argc; optind++)
	usage ();
}

int
main (int argc, char *argv[])
{
    unsigned int freq_text, context, word, freq;
    struct PTc_table_type ctable;

    init_arguments (argc, argv);

    PTc_init_table (&ctable);

    context = TXT_create_text ();
    word = TXT_create_text ();
    freq_text = TXT_create_text ();

    for (;;) /* Read in the table for each context */
      {
	if (TXT_readline_text (Stdin_File, context) <= 0)
	    break;

	/*
	  fprintf (stderr, "Context = ");
	  TXT_dump_text (Stderr_File, context, NULL);
	  fprintf (stderr, "\n");
	*/
	for (;;) /* Read in each word prediction and its frequency */
	  {
	    if (TXT_readline_text2 (Stdin_File, word) <= 0)
	        break;
	    if (TXT_length_text (word) == 0)
	        break; /* move on to next context */
	    if (TXT_readline_text (Stdin_File, freq_text) <= 0)
	        break;

	    TXT_scanf_text (freq_text, "%d", &freq);

	    /*
	      fprintf (stderr, "Text = ");
	      TXT_dump_text (Stderr_File, word, NULL);
	      fprintf (stderr, ", freq = %d\n", freq);
	    */

	    PTc_update_table (&ctable, context, NULL, word, freq);
	  }
      }

    if (Dump_table)
        PTc_dump_table (Stdout_File, &ctable);
    else
        PTc_write_table (Stdout_File, &ctable, TLM_Static);

    exit (1);
}
