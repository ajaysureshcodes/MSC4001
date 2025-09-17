/* Evaluates the performance of the tagger by comparing the tagger output
   against the ground trugh (both input files to the program). */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>

#ifdef SYSTEM_LINUX
#include <getopt.h> /* for getopt on Linux systems */
#endif

#include "io.h"
#include "text.h"
#include "list.h"
#include "index.h"
#include "table.h"
#include "paths.h"
#include "model.h"
#include "tag_model.h"
#include "transform.h"
#include "coderanges.h"

#define MAX_FILENAME_SIZE 128      /* Maximum size of a filename */

char Tagged_filename [MAX_FILENAME_SIZE];
char GTruth_filename [MAX_FILENAME_SIZE];

void
debug_tag ()
/* Dummy routine for debugging purposes. */
{
    fprintf (stderr, "Got here\n");
}

void
usage (void)
{
    fprintf (stderr,
	     "Usage: tag_eval [options] -g ground-truth-filename -t tagged-text\n"
	     "\n");
    fprintf (stderr,
	     "options:\n"
	);
    exit (2);
}

void
init_arguments (int argc, char *argv[])
{
    extern char *optarg;
    extern int optind; 
    int opt;

    boolean Tagged_found = FALSE, GTruth_found = FALSE;

    /* set defaults */
    Debug.level = 0;

    /* get the argument options */

    while ((opt = getopt (argc, argv, "g:t:")) != -1)
	switch (opt)
	{	
	case 'g':
	    GTruth_found = TRUE;
	    sprintf (GTruth_filename, "%s", optarg);
	    break;
	case 't':
	    Tagged_found = TRUE;
	    sprintf (Tagged_filename, "%s", optarg);
	    break;
	default:
	    usage ();
	    break;
	}

    if (!Tagged_found)
        fprintf (stderr, "\nFatal error: missing tagged text filename\n\n");
    if (!GTruth_found)
        fprintf (stderr, "\nFatal error: missing ground truth text filename\n\n");
    if (!Tagged_found || !GTruth_found)
      {
	usage ();
	exit (1);
      }

    for (; optind < argc; optind++)
	usage ();
}

void
process_files (unsigned int tagged_file, unsigned int gtruth_file)
/* Compares the tagged text file against the ground truth. */
{
    unsigned int confusion_id, confusion_count;
    unsigned int confusion_table, confusion;
    unsigned int tword, gword, word_pos;
    unsigned int correct, incorrect;
    unsigned int ttag, gtag;
    boolean eof;

    tword = TXT_create_text ();
    gword = TXT_create_text ();

    confusion = TXT_create_text ();
    ttag = TXT_create_text ();
    gtag = TXT_create_text ();

    confusion_table = TXT_create_table (TLM_Dynamic, 0);

    word_pos = 0;
    correct = 0;
    incorrect = 0;
    eof = FALSE;
    for (;;)
    {
	eof = (TXT_readline_text (tagged_file, tword) == EOF);
	if (eof)
	    break;

        word_pos++;

	eof = (TXT_readline_text (gtruth_file, gword) == EOF);
	assert (!eof);

	eof = (TXT_readline_text (tagged_file, ttag) == EOF);
	assert (!eof);
	eof = (TXT_readline_text (gtruth_file, gtag) == EOF);
	assert (!eof);

	if (!TXT_compare_text (ttag, gtag))
	    correct++;
	else
	  {
	    incorrect++;

	    TXT_setlength_text (confusion, 0);
	    TXT_append_text (confusion, gtag);
	    TXT_append_symbol (confusion, ' ');
	    TXT_append_text (confusion, ttag);
	    TXT_append_symbol (confusion, '=');
	    TXT_append_text (confusion, gword);
	    TXT_append_symbol (confusion, ' ');
	    TXT_append_text (confusion, tword);
	    TXT_update_table (confusion_table, confusion, &confusion_id,
			      &confusion_count);
	  }
    }

    TXT_release_text (tword);
    TXT_release_text (gword);
    TXT_release_text (ttag);
    TXT_release_text (gtag);

    fprintf (stdout, "Number correct = %d out of %d ( %.3f%% ) [ errors = %d ]\n",
	     correct, word_pos, 100.0 * correct / word_pos, incorrect);
    fprintf (stdout, "List of confusions:\n");
    TXT_dump_table_keys1 (Stdout_File, confusion_table);
}

int
main (int argc, char *argv[])
{
    unsigned int Tagged_file;
    unsigned int GTruth_file;

    init_arguments (argc, argv);

    Tagged_file = TXT_open_file
        (Tagged_filename, "r",
	 "Reading from tagged text file",
	 "tag: can't open tagged text file");
    GTruth_file = TXT_open_file
        (GTruth_filename, "r",
	 "Reading from ground truth text file",
	 "tag: can't open ground trugh text file");

    process_files (Tagged_file, GTruth_file);

    exit (0);
}
