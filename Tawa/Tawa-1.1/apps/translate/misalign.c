/* Aligns two input files of lines of text written in two different languages. */
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

#define MAX_FILENAME_SIZE 128      /* Maximum size of a filename */
#define MAX_ALIGN_COUNTS_SIZE 1000 /* Maximum size of align counts array */
#define MAX_ALIGN_COUNTS1_SIZE 100 /* Maximum size of decimal places for
				      align counts1 array */

unsigned int Lang1_text_file;
char Lang1_text_filename [MAX_FILENAME_SIZE];
unsigned int Lang2_text_file;
char Lang2_text_filename [MAX_FILENAME_SIZE];

unsigned int Lang1_model_file;
char Lang1_model_filename [MAX_FILENAME_SIZE];
unsigned int Lang2_model_file;
char Lang2_model_filename [MAX_FILENAME_SIZE];

unsigned int Output_file;
char Output_filename [MAX_FILENAME_SIZE];

unsigned int Align_counts [MAX_ALIGN_COUNTS_SIZE+1];
unsigned int Align_counts1 [MAX_ALIGN_COUNTS1_SIZE]; /* For range 1.0 to 2.0 */

unsigned int Lang1_model;
unsigned int Lang2_model;

unsigned int Debug_level = 0;
unsigned int Debug_progress = 0;

void
usage (void)
{
    fprintf (stderr,
	     "Usage: train [options]\n"
	     "\n"
	     "options:\n"
	     "  -1 fn\tParallel text for language 1\n"
	     "  -2 fn\tParallel text for language 2\n"
	     "  -3 fn\tModel for language 1\n"
	     "  -4 fn\tModel for language 2\n"
	     "  -d n\tdebug level=n.\n"
	     "  -o fn\toutput filename=fn (required argument)\n"
	     "  -p n\tprogress report every n lines.\n"
	);
    exit (2);
}

void
init_arguments (int argc, char *argv[])
{
    int opt;
    extern char *optarg;
    extern int optind;

    boolean Lang1_text_found = FALSE, Lang1_model_found = FALSE;
    boolean Lang2_text_found = FALSE, Lang2_model_found = FALSE;
    boolean Output_found = FALSE;

    /* get the argument options */

    Debug_level = 0;
    Debug_progress = 0;
    while ((opt = getopt (argc, argv, "1:2:3:4:d:o:p:")) != -1)
	switch (opt)
	{
	case '1':
	    Lang1_text_found = TRUE;
	    sprintf (Lang1_text_filename, "%s", optarg);
	    break;
	case '2':
	    Lang2_text_found = TRUE;
	    sprintf (Lang2_text_filename, "%s", optarg);
	    break;
	case '3':
	    Lang1_model_found = TRUE;
	    sprintf (Lang1_model_filename, "%s", optarg);
	    break;
	case '4':
	    Lang2_model_found = TRUE;
	    sprintf (Lang2_model_filename, "%s", optarg);
	    break;
	case 'd':
	    Debug_level = atoi (optarg);
	    break;
	case 'o':
	    Output_found = TRUE;
	    sprintf (Output_filename, "%s", optarg);
	    break;
	case 'p':
	    Debug_progress = atoi (optarg);
	    break;
	default:
	    usage ();
	    break;
	}

    if (!Lang1_text_found)
        fprintf (stderr, "\nFatal error: missing name of Language2 text filename\n\n");
    if (!Lang2_text_found)
        fprintf (stderr, "\nFatal error: missing name of Language2 text filename\n\n");

    if (!Lang1_model_found)
        fprintf (stderr, "\nFatal error: missing name of Language2 model filename\n\n");
    if (!Lang2_model_found)
        fprintf (stderr, "\nFatal error: missing name of Language2 model filename\n\n");

    if (!Output_found)
        fprintf (stderr, "\nFatal error: missing name of Output text filename\n\n");

    if (!Lang1_text_found || !Lang2_text_found || !Lang1_model_found || !Lang1_model_found ||
	!Output_found)
      {
	usage ();
	exit (1);
      }

    for (; optind < argc; optind++)
	usage ();
}

void
align_texts (unsigned int Lang1_text_file, unsigned int Lang2_text_file,
	     unsigned int Lang1_model, unsigned int Lang2_model)
/* Aligns the parallel texts using the models. */
{

    unsigned int context1, context2, text1, text2, symbol, p, i;
    unsigned int words1, words2, length1, length2;
    float codelength1, codelength2, ratio, overall, r, k;
    int cc1, cc2;

    text1 = TXT_create_text ();
    text2 = TXT_create_text ();

    context1 = TLM_create_context (Lang1_model);
    context2 = TLM_create_context (Lang2_model);

    TLM_set_context_operation (TLM_Get_Codelength);

    p = 0;
    overall = 0.0;
    for (;;)
      {
	if (Debug_progress && ((p % Debug_progress) == 0))
	  {
	    fprintf (stderr, "Processing line %d\n", p);
	  }

	cc1 = TXT_readline_text (Lang1_text_file, text1);
	cc2 = TXT_readline_text (Lang2_text_file, text2);

	if ((cc1 == EOF) || (cc2 == EOF))
	  { /* Number of lines must equal */
	    if ((cc1 != EOF) || (cc2 != EOF))
	      {
	        fprintf (stderr, "Fatal error: number of lines in text files do not match\n");
		if (cc1 == EOF)
		    fprintf (stderr, "EOF found for file 1: %s\n", Lang1_text_filename);
		if (cc2 == EOF)
		    fprintf (stderr, "EOF found for file 2: %s\n", Lang2_text_filename);
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
	    overall += log_two (ratio);

	    if (r >= MAX_ALIGN_COUNTS_SIZE)
	        r = MAX_ALIGN_COUNTS_SIZE;
	    for (i = 1; i <= MAX_ALIGN_COUNTS_SIZE; i++)
	      if (i <= r) /* Calculate cumulative counts */
		Align_counts [i]++;
	    for (i = 0; i < MAX_ALIGN_COUNTS1_SIZE; i++)
	      {
		k = 1.0 + ((float) i)/MAX_ALIGN_COUNTS1_SIZE;
		if (k <= r) /* Calculate cumulative counts */
		  Align_counts1 [i]++;
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
		fprintf (stderr, "Codelengths for lines %d : %.3f %.3f ratio %.3f max %.3f (words %d,%d; lengths %d,%d) cum. overall %.3f\n",
			 p,	codelength1, codelength2, codelength1/codelength2, ratio,
			 words1, words2, length1, length2, overall);
	      }
	  }

	/* Reset context for next sentence */
	TLM_update_context (Lang1_model, context1, TXT_sentinel_symbol ());
	TLM_update_context (Lang2_model, context2, TXT_sentinel_symbol ());

	p++;
      }

    fprintf (stderr, "Processed %d lines\n", p);

    fprintf (stderr, "Cumulative percentages:\n");
    for (i = 1; i <= MAX_ALIGN_COUNTS_SIZE; i++)
      if (Align_counts [i])
	{
	  fprintf (stderr, "Percentage above ratio %3d = %.3f (%d/%d)\n",
		   i, (100.0 * Align_counts [i])/p, Align_counts [i], p);
	}
    for (i = 0; i < MAX_ALIGN_COUNTS1_SIZE; i++)
      if (Align_counts1 [i])
	{
	  k = 1.0 + ((float) i)/MAX_ALIGN_COUNTS1_SIZE;
	  fprintf (stderr, "Percentage above ratio %.3f = %.3f (%d/%d)\n",
		   k, (100.0 * Align_counts1 [i])/p,
		   Align_counts1 [i], p);
	}

    TLM_release_context (Lang1_model, context1);
    TLM_release_context (Lang2_model, context2);

    TXT_release_text (text1);
    TXT_release_text (text2);

    fprintf (stderr, "Aligned on %d lines\n", p);
    fprintf (stderr, "Overall alignment sum: %.6f\n", overall);
    fprintf (stderr, "Overall alignment normalised: %.3f\n", (overall/p));

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


    Lang1_text_file = TXT_open_file
      (Lang1_text_filename, "r", "Reading Language1 text file",
       "Align: can't open Language1 text file" );
    Lang2_text_file = TXT_open_file
      (Lang2_text_filename, "r", "Reading Language2 text file",
       "Align: can't open Language2 text file" );

    Output_file = TXT_open_file
      (Output_filename, "w", "Writing to output file",
       "Train: can't open output file" );

    Lang1_model = TLM_read_model (Lang1_model_filename, NULL, NULL);
    Lang2_model = TLM_read_model (Lang2_model_filename, NULL, NULL);


    align_texts (Lang1_text_file, Lang2_text_file, Lang1_model, Lang2_model);

    TLM_release_model (Lang1_model);
    TLM_release_model (Lang2_model);

    exit (0);
}
