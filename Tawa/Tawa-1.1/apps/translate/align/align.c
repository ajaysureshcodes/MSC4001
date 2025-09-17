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
#include "index.h"
#include "model.h"
#include "ppm_model.h"
#include "aqueue.h"

#define MAX_FILENAME_SIZE 128      /* Maximum size of a filename */
#define MAX_SENTENCES_SIZE 100000  /* Maximum number of sentences (either language) */

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

float Codelengths [MAX_SENTENCES_SIZE+1][2];

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
process_text (unsigned int Lang, unsigned int Lang_text_file,
	      unsigned int Lang_model)
/* Processes the text file for the language. */
{

    unsigned int context, text, symbol, p, i;
    unsigned int words, length;
    float codelength;
    int cc;

    text = TXT_create_text ();

    context = TLM_create_context (Lang_model);

    TLM_set_context_operation (TLM_Get_Codelength);

    p = 0;
    for (;;)
      {
	if (Debug_progress && ((p % Debug_progress) == 0))
	  {
	    fprintf (stderr, "Processing line %d for Language %d\n", p, Lang);
	  }

	cc = TXT_readline_text (Lang_text_file, text);
	if (cc == EOF)
	  break;

	length = TXT_length_text (text);
	if (!length)
	  {
	    fprintf (stderr, "Fatal error: empty line found in text file at position %d\n", p);
	  }
	else
	  {
	    codelength = 0;

	    words = 0;
	    assert (length > 0);

	    for (i = 0; i < length; i++)
	      {
		assert (TXT_get_symbol (text, i, &symbol));

		if (symbol == ' ')
		    words++;

		TLM_update_context (Lang_model, context, symbol);
		codelength += TLM_Codelength;

		if (Debug_level > 2)
		  fprintf (stderr, "Lang %d: Line %d Pos %d symbol %3d (%c) codelength %8.3f tot %8.3f\n",
			   Lang, p, i, symbol, symbol, TLM_Codelength, codelength);
	      }
	    words++;


	    Codelengths [p][Lang - 1] = codelength;

	    if (Debug_level > 1)
	      {
		fprintf (stderr, "Language %d at line pos %d: ", Lang, p);
		TXT_dump_text (Stderr_File, text, NULL);
		fprintf (stderr, "\n");
	      }

	    if (Debug_level > 0)
	      {
		fprintf (stderr, "Language %d Line %d : chars %3d words %3d codelength %8.3f\n",
			 Lang, p, length, words, codelength);
	      }
	  }

	/* Reset context for next sentence */
	TLM_update_context (Lang_model, context, TXT_sentinel_symbol ());

	p++;
      }

    fprintf (stderr, "Language %d Processed %d lines\n", Lang, p);

    TLM_release_context (Lang_model, context);

    TXT_release_text (text);
}

void
align_texts (unsigned int Lang1_text_file, unsigned int Lang2_text_file,
	     unsigned int Lang1_model, unsigned int Lang2_model)
/* Aligns the parallel texts using the models. */
{
    /* Load the codelengths for the two texts in the different languages */
    process_text (1, Lang1_text_file, Lang1_model);
    process_text (2, Lang2_text_file, Lang2_model);

}

int
main (int argc, char *argv[])
{
    unsigned int i, j;
    unsigned int alignment;
    unsigned int aqueue;

    init_arguments (argc, argv);

    for (i = 0; i <= MAX_SENTENCES_SIZE; i++)
      for (j = 0; j < 2; j++)
        Codelengths [i][j] = 0.0;

    Lang1_text_file = TXT_open_file
      (Lang1_text_filename, "r", "Reading Language1 text file",
       "Align: can't open Language1 text file" );
    Lang2_text_file = TXT_open_file
      (Lang2_text_filename, "r", "Reading Language2 text file",
       "Align: can't open Language2 text file" );

    Output_file = TXT_open_file
      (Output_filename, "w", "Writing to output file",
       "Train: can't open output file" );

    /*
    Lang1_model = TLM_read_model (Lang1_model_filename, NULL, NULL);
    Lang2_model = TLM_read_model (Lang2_model_filename, NULL, NULL);


    align_texts (Lang1_text_file, Lang2_text_file, Lang1_model, Lang2_model);

    TLM_release_model (Lang1_model);
    TLM_release_model (Lang2_model);
    */

    alignment = TXT_create_index();
    TXT_put_index (alignment, 55, 476575);
    TXT_put_index (alignment, 155, 17575);
    TXT_put_index (alignment, 255, 27675);
    TXT_put_index (alignment, 555, 3765);

    TXT_dump_index (Stdout_File, alignment, NULL);

    aqueue = NIL;
    printf ("1: %d\n", aqueue);
    TXT_dump_aqueue (Stdout_File, aqueue, NULL);

    aqueue = TXT_insert_aqueue (aqueue, 55);
    printf ("2: %d\n", aqueue);
    TXT_dump_aqueue (Stdout_File, aqueue, NULL);

    aqueue = TXT_insert_aqueue (aqueue, 44);
    printf ("3: %d\n", aqueue);
    TXT_dump_aqueue (Stdout_File, aqueue, NULL);

    aqueue = TXT_insert_aqueue (aqueue, 33);
    printf ("4: %d\n", aqueue);
    TXT_dump_aqueue (Stdout_File, aqueue, NULL);

    aqueue = TXT_insert_aqueue (aqueue, 33);
    printf ("5: %d\n", aqueue);
    TXT_dump_aqueue (Stdout_File, aqueue, NULL);

    TXT_release_aqueue (aqueue);

    exit (0);
}
