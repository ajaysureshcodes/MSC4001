/* Test program. */
#include <stdio.h>
#include <stdlib.h>

#ifdef SYSTEM_LINUX
#include <getopt.h> /* for getopt on Linux systems */
#endif

#include "io.h"
#include "word.h"
#include "text.h"
#include "track.h"
#include "transform.h"
#include "model.h"

boolean Model_found = FALSE, Words_model_found = FALSE;
unsigned int Model, Words_model;

void
usage (void)
{
    fprintf (stderr,
	     "Usage: test_track [options] training-model <input-text\n"
	     "\n"
	     "options:\n"
	     "  -m fn\tmodel filename=fn\n"
	     "  -w fn\twords model filename=fn\n"
	);
    exit (2);
}

void
init_arguments (int argc, char *argv[])
{
    extern char *optarg;
    extern int optind;
    int opt;

    /* set defaults */
    Model_found = FALSE;
    Words_model_found = FALSE;

    /* get the argument options */

    while ((opt = getopt (argc, argv, "m:w:")) != -1)
	switch (opt)
	{
	case 'm':
	    Model = TLM_read_model
	      (optarg, "Loading words model from file",
	       "Test_track: can't open words model file");
	    Model_found = TRUE;
	    break;
	case 'w':
	    Words_model = TLM_read_words_model
	      (optarg, "Loading words model from file",
	       "Test_track: can't open words model file");
	    Words_model_found = TRUE;
	    break;
	default:
	    usage ();
	    break;
	}

    if (!Model_found || !Words_model_found)
        usage ();
    for (; optind < argc; optind++)
	usage ();
}

int
main (int argc, char *argv[])
{
    unsigned int track, words_track, pos, symbol, text, textlen;
    float codelength, words_codelength;

    init_arguments (argc, argv);

    text = TXT_load_text (Stdin_File);

    track = NIL;
    words_track = NIL;

    if (Model_found)
        track = TLM_make_track (TLM_standard_tracking, Model, text);

    if (Words_model_found)
        words_track = TLM_make_words_track (TLM_standard_tracking, Words_model, text);

    textlen = TXT_length_text (text);

    for (pos = 0; pos < textlen; pos++)
      {
	if (!Model_found)
	    codelength = 0;
	else
	    TLM_get_track (track, pos, &codelength);
	if (!Words_model_found)
	    words_codelength = 0;
	else
	    TLM_get_track (words_track, pos, &words_codelength);
	if (codelength || words_codelength)
	  {
	    TXT_get_symbol (text, pos, &symbol);
	    fprintf (stderr, "%6d [%c] - codelengths :", pos, symbol);
	    if (codelength)
	        fprintf (stderr, " chars %6.3f", codelength);
	    if (words_codelength)
	        fprintf (stderr, " words %6.3f", words_codelength);
	    fprintf (stderr, "\n");
	  }
      }

    exit (1);
}
