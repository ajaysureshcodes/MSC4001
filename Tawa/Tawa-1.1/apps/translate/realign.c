/* Realigns the corpus by finding the best match in adjacent codelengths.
   Deletes lines where necessary. */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>

#ifdef SYSTEM_LINUX
#include <getopt.h> /* for getopt on Linux systems */
#endif

#include "io.h"
#include "text.h"
#include "model.h"
#include "ppm_model.h"

#define MAX_FILENAME_SIZE 256      /* Maximum size of a filename */
#define MAX_ALIGN_COUNTS_SIZE 100  /* Maximum size of align counts array */
#define MAX_ALIGN_COUNTS1_SIZE 100 /* Maximum size of decimal places for
				      align counts1 array */
#define WINDOW_SIZE 5              /* Size of the sliding window used for alignment */
#define PRUNE_SIZE 10              /* Double the window size used for pruning */

unsigned int Lang1_input_file;
char Lang1_input_filename [MAX_FILENAME_SIZE];
unsigned int Lang2_input_file;
char Lang2_input_filename [MAX_FILENAME_SIZE];

unsigned int Lang1_output_file;
char Lang1_output_filename [MAX_FILENAME_SIZE];
unsigned int Lang2_output_file;
char Lang2_output_filename [MAX_FILENAME_SIZE];

unsigned int Lang1_model_file;
char Lang1_model_filename [MAX_FILENAME_SIZE];
unsigned int Lang2_model_file;
char Lang2_model_filename [MAX_FILENAME_SIZE];

unsigned int Align_counts [MAX_ALIGN_COUNTS_SIZE+1];
unsigned int Align_counts1 [MAX_ALIGN_COUNTS1_SIZE]; /* For range 1.0 to 2.0 */

unsigned int Lang1_model;
unsigned int Lang2_model;

float Threshold = 0.0;

unsigned int Debug_level = 0;
unsigned int Debug_progress = 0;

void
usage (void)
{
    fprintf (stderr,
	     "Usage: train [options]\n"
	     "\n"
	     "options:\n"
	     "  -1 fn\tParallel input filename for language 1\n"
	     "  -2 fn\tParallel input filename for language 2\n"
	     "  -3 fn\tParallel output filename for language 1\n"
	     "  -4 fn\tParallel output filename for language 2\n"
	     "  -5 fn\tModel for language 1\n"
	     "  -6 fn\tModel for language 2\n"
	     "  -d n\tdebug level=n.\n"
	     "  -p n\tprogress report every n lines.\n"
	     "  -t n\tdelete sentences with information ratio > n.\n"
	);
    exit (2);
}

void
init_arguments (int argc, char *argv[])
{
    int opt;
    extern char *optarg;
    extern int optind;

    boolean Lang1_input_found = FALSE, Lang2_input_found = FALSE;
    boolean Lang1_output_found = FALSE, Lang2_output_found = FALSE;
    boolean Lang1_model_found = FALSE, Lang2_model_found = FALSE;
    boolean Threshold_found = FALSE;

    /* get the argument options */

    Debug_level = 0;
    Debug_progress = 0;
    while ((opt = getopt (argc, argv, "1:2:3:4:5:6:d:p:t:")) != -1)
	switch (opt)
	{
	case '1':
	    Lang1_input_found = TRUE;
	    sprintf (Lang1_input_filename, "%s", optarg);
	    break;
	case '2':
	    Lang2_input_found = TRUE;
	    sprintf (Lang2_input_filename, "%s", optarg);
	    fprintf (stderr, "Lang2 input filename = %s\n", Lang2_input_filename);
	    break;
	case '3':
	    Lang1_output_found = TRUE;
	    sprintf (Lang1_output_filename, "%s", optarg);
	    break;
	case '4':
	    Lang2_output_found = TRUE;
	    sprintf (Lang2_output_filename, "%s", optarg);
	    break;
	case '5':
	    Lang1_model_found = TRUE;
	    sprintf (Lang1_model_filename, "%s", optarg);
	    break;
	case '6':
	    Lang2_model_found = TRUE;
	    sprintf (Lang2_model_filename, "%s", optarg);
	    break;
	case 'd':
	    Debug_level = atoi (optarg);
	    break;
	case 't':
	    Threshold_found = TRUE;
	    Threshold = atof (optarg);
	    break;
	case 'p':
	    Debug_progress = atoi (optarg);
	    break;
	default:
	    usage ();
	    break;
	}

    if (!Lang1_input_found)
        fprintf (stderr, "\nFatal error: missing name of Language1 input filename\n\n");
    if (!Lang2_input_found)
        fprintf (stderr, "\nFatal error: missing name of Language2 input filename\n\n");
    if (!Lang1_output_found)
        fprintf (stderr, "\nFatal error: missing name of Language1 outputfilename\n\n");
    if (!Lang2_output_found)
        fprintf (stderr, "\nFatal error: missing name of Language2 outputfilename\n\n");

    if (!Lang1_model_found)
        fprintf (stderr, "\nFatal error: missing name of Language1 model filename\n\n");
    if (!Lang2_model_found)
        fprintf (stderr, "\nFatal error: missing name of Language2 model filename\n\n");

    if (!Lang1_input_found || !Lang2_input_found ||
	!Lang1_output_found || !Lang2_output_found ||
	!Lang1_model_found || !Lang2_model_found ||
	!Threshold_found)
      {
	usage ();
	exit (1);
      }

    for (; optind < argc; optind++)
	usage ();
}

unsigned int
get_text (unsigned int input_file, unsigned int texts, unsigned int pos)
/* Gets the text (i.e. sentence) stored at position pos in the list of
   texts. Reads it from the input_file if there is no text stored there
   yet. Do this so that only a sliding window is maintained of the
   latest texts (otherwise we will be in danger of running out of
   memory for very large texts. i.e. Texts older than the
   sliding window will get pruned out.*/
{
    unsigned int text, len, p;
    int cc;

    assert (TXT_valid_text (texts));

    len = TXT_length_text (texts);

    /* Read in the text lines out to pos if needed */
    for (p = len; p <= pos; p++)
      {
	cc = TXT_readline_text2 (input_file, text);
	if (!TXT_length_text (text))
	    fprintf (stderr,
		     "Fatal error: empty line found in text file at position %d\n", p);

        TXT_append_symbol (texts, text);
      }

    /* Prune out the old texts if necessary */
    if (len > PRUNE_SIZE)
      {
	for (p = len-PRUNE_SIZE; p < len-PRUNE_SIZE+WINDOW_SIZE; p++)
	  {
	    assert (TXT_get_symbol (texts, p, &text));
	    if (text)
	      {
		TXT_release_text (text);
		TXT_put_symbol (texts, 0, p); /* Reset to null */
	      }
	  }
      }

    assert (TXT_get_symbol (texts, pos, &text));
    return (text);
}


void
align_texts (unsigned int Lang1_input_file, unsigned int Lang2_input_file,
	     unsigned int Lang1_output_file, unsigned int Lang2_output_file,
	     unsigned int Lang1_model, unsigned int Lang2_model)
/* Deletes the parallel text lines when their information ratio exceeds
   some threshold based on using the models. */
{

    unsigned int context1, context2, text1, text2, symbol, p, q, i;
    unsigned int words1, words2, length1, length2, r;
    float codelength1, codelength2, ratio, overall, k;
    int cc1, cc2;

    text1 = TXT_create_text ();
    text2 = TXT_create_text ();

    context1 = TLM_create_context (Lang1_model);
    context2 = TLM_create_context (Lang2_model);

    TLM_set_context_operation (TLM_Get_Codelength);

    p = 0;
    q = 0;
    overall = 0.0;
    for (;;)
      {
	if (Debug_progress && ((p % Debug_progress) == 0))
	  {
	    fprintf (stderr, "Processing line %d\n", p);
	  }

	cc1 = TXT_readline_text2 (Lang1_input_file, text1);
	cc2 = TXT_readline_text2 (Lang2_input_file, text2);

	if ((cc1 == EOF) || (cc2 == EOF))
	  { /* Number of lines must equal */
	    if ((cc1 != EOF) || (cc2 != EOF))
	      {
	        fprintf (stderr, "Fatal error: number of lines in text files do not match\n");
		if (cc1 == EOF)
		    fprintf (stderr, "EOF found for file 1: %s\n", Lang1_input_filename);
		if (cc2 == EOF)
		    fprintf (stderr, "EOF found for file 2: %s\n", Lang2_input_filename);
	      }

	    /* assert ((cc1 == EOF) && (cc2 == EOF)); */ /* Number of lines must equal */
	    break;
	  }
	length1 = TXT_length_text (text1);
	length2 = TXT_length_text (text2);
	if (!length1 || !length2)
	  {
	    fprintf (stderr, "Fatal error: empty line found in text file at position %d\n", p);
	    /*assert (!length1 && !length2);*/ /* EOF must be in synch */
	  }
	else
	  {
	    codelength1 = 0;
	    codelength2 = 0;

	    words1 = 0;
	    assert (length1 > 0);

	    for (i = 0; i < length1; i++)
	      {
		assert (TXT_get_symbol (text1, i, &symbol));

		if (symbol == ' ')
		    words1++;

		TLM_update_context (Lang1_model, context1, symbol);
		codelength1 += TLM_Codelength;

		if (Debug_level > 2)
		  fprintf (stderr, "Text 1: Line %d Pos %d symbol %3d (%c) codelength %.3f tot %.3f\n",
			   p, i, symbol, symbol, TLM_Codelength, codelength1);
	      }
	    words1++;

	    words2 = 0;
	    assert (length2 > 0);

	    for (i = 0; i < length2; i++)
	      {
		assert (TXT_get_symbol (text2, i, &symbol));

		if (symbol == ' ')
		    words2++;

		TLM_update_context (Lang2_model, context2, symbol);
		codelength2 += TLM_Codelength;

		if (Debug_level > 2)
		  fprintf (stderr, "Text 2: Line %d Pos %d symbol %3d (%c) codelength %.3f tot %.3f\n",
			   p, i, symbol, symbol, TLM_Codelength, codelength2);
	      }
	    words2++;

	    if (codelength1 > codelength2)
	        ratio = codelength1/codelength2;
	    else
	        ratio = codelength2/codelength1;
	    r = ratio;

	    if (r < Threshold)
	      {
		q++;
		overall += log_two (ratio);

		if (r >= MAX_ALIGN_COUNTS_SIZE)
		  r = MAX_ALIGN_COUNTS_SIZE;
		for (i = 1; i <= MAX_ALIGN_COUNTS_SIZE; i++)
		  {
		    if (i <= r) /* Calculate cumulative counts */
		      Align_counts [i]++;
		  }
		for (i = 0; i < MAX_ALIGN_COUNTS1_SIZE; i++)
		  {
		    k = 1.0 + ((float) i)/MAX_ALIGN_COUNTS1_SIZE;
		    if (k <= r) /* Calculate cumulative counts */
		      Align_counts1 [i]++;
		  }

		/* Write out the lines to the output files but only if they are
		   well enough aligned (< Threshold) */
		TXT_dump_text (Lang1_output_file, text1, NULL);
		fprintf (Files [Lang1_output_file], "\n");
		TXT_dump_text (Lang2_output_file, text2, NULL);
		fprintf (Files [Lang2_output_file], "\n");
	      }

	    if (Debug_level > 1)
	      {
		fprintf (stderr, "Text 1 at pos %d: ", p);
		TXT_dump_text (Stderr_File, text1, NULL);
		fprintf (stderr, "\n");

		fprintf (stderr, "Text 2 at pos %d: ", p);
		TXT_dump_text (Stderr_File, text2, NULL);
		fprintf (stderr, "\n");
	      }

	    if (Debug_level > 0)
	      {
		fprintf (stderr, "Codelengths for lines %d : %.3f %.3f ratio %.3f max %.3f (words %d,%d; lengths %d,%d) cum. overall %.6f\n",
			 p,	codelength1, codelength2, codelength1/codelength2, ratio,
			 words1, words2, length1, length2, overall);
	      }
	  }

	/* Reset context for next sentence */
	TLM_update_context (Lang1_model, context1, TXT_sentinel_symbol ());
	TLM_update_context (Lang2_model, context2, TXT_sentinel_symbol ());

	p++;
      }

    fprintf (stderr, "Input  %d lines\n", p);
    fprintf (stderr, "Output %d lines\n", q);

    fprintf (stderr, "Cumulative percentages:\n");
    for (i = 1; i <= MAX_ALIGN_COUNTS_SIZE; i++)
      if (Align_counts [i])
	{
	  fprintf (stderr, "Percentage above ratio %3d = %.3f (%d/%d)\n",
		   i, (100.0 * Align_counts [i])/q,
		   Align_counts [i], q);
	}
    for (i = 0; i <= MAX_ALIGN_COUNTS1_SIZE; i++)
      if (Align_counts1 [i])
	{
	  k = 1.0 + ((float) i)/MAX_ALIGN_COUNTS1_SIZE;
	  fprintf (stderr, "Percentage above ratio %.3f = %.3f (%d/%d)\n",
		   k, (100.0 * Align_counts1 [i])/q,
		   Align_counts1 [i], q);
	}

    TLM_release_context (Lang1_model, context1);
    TLM_release_context (Lang2_model, context2);

    TXT_release_text (text1);
    TXT_release_text (text2);

    fprintf (stderr, "Aligned on %d lines (out of %d)\n", q, p);
    fprintf (stderr, "Overall alignment = %.6f\n", (overall/q));
}

int
main (int argc, char *argv[])
{
    unsigned int i;

    init_arguments (argc, argv);

    for (i = 0; i <= MAX_ALIGN_COUNTS_SIZE; i++)
        Align_counts [i] = 0;
    for (i = 0; i <= MAX_ALIGN_COUNTS1_SIZE; i++)
        Align_counts1 [i] = 0;

    Lang1_input_file = TXT_open_file
      (Lang1_input_filename, "r", "Reading Language1 input file",
       "Align_delete: can't open Language1 input text file" );
    fprintf (stderr, "Lang2 input filename = %s\n", Lang2_input_filename);
    Lang2_input_file = TXT_open_file
      (Lang2_input_filename, "r", "Reading Language2 input file",
       "Align_delete: can't open Language2 input text file" );

    Lang1_output_file = TXT_open_file
      (Lang1_output_filename, "w", "Writing Language1 output file",
       "Align_delete: can't open Language1 output text file" );
    Lang2_output_file = TXT_open_file
      (Lang2_output_filename, "w", "Writing Language2 output file",
       "Align_delete: can't open Language2 output text file" );

    Lang1_model = TLM_read_model (Lang1_model_filename, NULL, NULL);
    Lang2_model = TLM_read_model (Lang2_model_filename, NULL, NULL);


    align_texts (Lang1_input_file, Lang2_input_file,
		 Lang1_output_file, Lang2_output_file,
		 Lang1_model, Lang2_model);

    TLM_release_model (Lang1_model);
    TLM_release_model (Lang2_model);

    exit (0);
}
