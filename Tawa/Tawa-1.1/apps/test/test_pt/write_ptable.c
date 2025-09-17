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
    unsigned int freq_text, word, freq;
    struct PTp_table_type table;

    init_arguments (argc, argv);

    PTp_init_table (&table);

    word = TXT_create_text ();
    freq_text = TXT_create_text ();

    while ((TXT_readline_text (Stdin_File, word) > 0) && (TXT_readline_text (Stdin_File, freq_text) > 0))
      {
	TXT_scanf_text (freq_text, "%d", &freq);
	/*
	fprintf (stderr, "Text = ");
	TXT_dump_text (Stderr_File, word, NULL);
	fprintf (stderr, ", freq = %d\n", freq);
	*/
	PTp_update_table (&table, word, freq);
      }

    if (Dump_table)
        PTc_dump_table (Stdout_File, &table);
    else
        PTp_write_table (Stdout_File, &table, TLM_Static);

    exit (1);
}
