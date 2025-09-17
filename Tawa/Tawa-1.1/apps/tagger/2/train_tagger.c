/* Loads or creates a tag-based model using cumulative probability
   tables (pt), adds some text to the model, then writes out the
   changed model. */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <unistd.h>

#ifdef SYSTEM_LINUX
#include <getopt.h> /* for getopt on Linux systems */
#endif

#include "io.h"
#include "word.h"
#include "text.h"
#include "table.h"
#include "model.h"
#include "pt_model.h"

#define CHAR_ALPHABET_SIZE 256   /* Size of the character model's alphabet */
#define MAX_FILENAME_SIZE 128    /* Maximum size of a filename */
#define DEFAULT_WORD_MAX_ORDER 1 /* Default max order for word model */
#define DEFAULT_CHAR_MAX_ORDER 4 /* Default max order for character model */
#define DEFAULT_CHAR_ESCAPE_METHOD TLM_PPM_Method_D /* Default escape method for character model */
#define PERFORMS_EXCLS TRUE

char *Model_title;

unsigned int Words_model;
unsigned int Words_model_max_order = DEFAULT_WORD_MAX_ORDER;
unsigned int Chars_model;
unsigned int Chars_model_max_order = DEFAULT_CHAR_MAX_ORDER;

boolean Input_found = FALSE, Output_found = FALSE, Title_found = FALSE;

char *Title = NULL;
char Input_filename [MAX_FILENAME_SIZE];
char Output_filename [MAX_FILENAME_SIZE];

unsigned int Input_text_file;
unsigned int Input_tags_file;

unsigned int Output_tags_model_file;
unsigned int Output_words_model_file;
unsigned int Output_chars_model_file;

void
open_input_files (char *file_prefix)
/* Opens all the input files for reading. */
{
  char filename [MAX_FILENAME_SIZE];

  sprintf (filename, "%s.text", file_prefix);
  Input_text_file = TXT_open_file (filename, "r",
      "Reading from text file",
      "Train: can't open text file" );

  sprintf (filename, "%s.tags", file_prefix);
  Input_tags_file = TXT_open_file (filename, "r",
      "Reading from tags file",
      "Train: can't open tags file" );

  fprintf (stderr, "\n");
}

void
open_output_files (char *file_prefix)
/* Opens all the output files for writing. */
{
  char filename [MAX_FILENAME_SIZE];

  sprintf (filename, "%s_tags.model", file_prefix);
  Output_tags_model_file = TXT_open_file (filename, "w",
      "Writing tags model into file",
      "Train: can't open tags model file" );

  sprintf (filename, "%s_words.model", file_prefix);
  Output_words_model_file = TXT_open_file (filename, "w",
      "Writing word model into file",
      "Train: can't open words model file" );

  sprintf (filename, "%s_chars.model", file_prefix);
  Output_chars_model_file = TXT_open_file (filename, "w",
      "Writing chars model into file",
      "Train: can't open chars model file" );

  fprintf (stderr, "\n");
}

void
usage (void)
{
    fprintf (stderr,
	     "Usage: train [options] <input-text\n"
	     "\n"
	     "options:\n"
             "  -d n\tdebugging level=n\n"
	     "  -i fn\tgeneric input filename=fn (required argument)\n"
	     "  -o fn\tgeneric output filename=fn (required argument)\n"
	     "  -p n\tprogress report every n words\n"
	     "  -T n\tlong description (title) of model (required argument)\n"
	     "  -1 n\tmax order of word model=n (optional; default=1)\n"
	     "  -2 n\tmax order of character model=n (optional; default=4)\n"
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

    Output_found = FALSE;
    Title_found = FALSE;
    Debug.level = 0;
    while ((opt = getopt (argc, argv, "T:d:i:o:p:1:2:")) != -1)
	switch (opt)
	{
	case 'd':
	    Debug.level = atoi (optarg);
	    break;
	case 'i':
	    Input_found = TRUE;
	    sprintf (Input_filename, "%s", optarg);
	    break;
	case 'o':
	    Output_found = TRUE;
	    sprintf (Output_filename, "%s", optarg);
	    break;
	case 'p':
	    Debug.progress = atoi (optarg);
	    break;
	case 'T':
	    Title_found = TRUE;
	    Model_title = (char *) malloc (strlen (optarg)+1);
	    strcpy (Model_title, optarg);
	    break;
	case '1':
	    Words_model_max_order = atoi (optarg);
	    break;
	case '2':
	    Chars_model_max_order = atoi (optarg);
	    break;
	default:
	    usage ();
	    break;
	}

    fprintf (stderr, "\nCreating new models\n\n");

    if (!Output_found)
        fprintf (stderr, "\nFatal error: missing output filename\n\n");
    if (!Title_found)
        fprintf (stderr, "\nFatal error: missing title of the model\n\n");
    if (!Output_found || !Title_found)
      {
	usage ();
	exit (1);
      }
    for (; optind < argc; optind++)
	usage ();
}

void
process_word (unsigned int words_model, unsigned int chars_model,
	      unsigned int words_context, unsigned int chars_context,
	      unsigned int *prev_words, unsigned int word)
/* Trains the model from the word. */
{
    unsigned int pos, symbol, p, q;

    /* Update the order 0 word contexts */
    TLM_update_context (words_model, NIL, word);
    if (PT_Novel_Symbols)
      {
	if (Debug.level > 2)
	  {
	    fprintf (stderr, "Novel word: ");
	    TXT_dump_text (Stderr_File, word, TXT_dump_symbol);
	    fprintf (stderr, "\n");
	  }
	pos = 0;
	while (TXT_get_symbol (word, pos, &symbol))
	  {
	    TLM_update_context (chars_model, chars_context, symbol);
	    pos++;
	  }
	TLM_update_context (chars_model, chars_context, 0); /* Terminate with null */
      }

    /* Create the higher order contexts, and update the words model with them */
    for (p = 0; p < Words_model_max_order; p++)
      {
	if (prev_words [p] == NIL)
	    break;

	TXT_setlength_text (words_context, 0);
	for (q = 0; q <= p; q++)
	  {
	    TXT_append_text (words_context, prev_words [q]);
	    TXT_append_symbol (words_context, TXT_sentinel_symbol ());
	    /* The sentinel symbol is used here to separate out the words in
	       the context since some "words" may in fact have spaces in them */
	  }
 
	TLM_update_context (words_model, words_context, word);
      }
}

void
process_file (unsigned int file,
	      unsigned int words_model,
	      unsigned int chars_model)
/* Trains the model from the words and characters in the text. */
{
    unsigned int words_context, chars_context;
    unsigned int word, word_pos, p;
    unsigned int *prev_words;
    boolean eof;

    TLM_set_context_operation (TLM_Get_Nothing);

    word = TXT_create_text ();
    words_context = TXT_create_text ();
    chars_context = TLM_create_context (chars_model);

    prev_words = (unsigned int *) malloc (Words_model_max_order * sizeof (unsigned int));
    for (p = 0; p < Words_model_max_order; p++)
        prev_words [p] = NIL;

    word_pos = 0;
    eof = FALSE;
    for (;;)
    {
        word_pos++;
	if ((Debug.progress > 0) && ((word_pos % Debug.progress) == 0))
	  {
	    fprintf (stderr, "Processing word pos %d\n", word_pos);
	    /*dump_memory (Stderr_File);*/
	  }
        /* repeat until EOF or max input */
	eof = (TXT_readline_text (file, word) == EOF);

	if (Debug.range)
	  {
	    fprintf (stderr, "Processed word {");
	    TXT_dump_text (Stderr_File, word, TXT_dump_symbol);
	    fprintf (stderr, "}\n");
	  }

	if (eof)
	    break;

	process_word (words_model, chars_model, words_context, chars_context,
		      prev_words, word);

	/* Rotate the words through the previous history buffers */
	for (p = 0; p < Words_model_max_order; p++)
	    prev_words [p+1] = prev_words [p];
	prev_words [0] = word;
    }

    TLM_release_context (words_model, words_context);
    TLM_release_context (chars_model, chars_context);

    TXT_release_text (word);

    if (Debug.range || (Debug.progress != 0))
        fprintf (stderr, "Processed %d words\n", word_pos);
}

int
main (int argc, char *argv[])
{
    unsigned int model_form;

    init_arguments (argc, argv);

    model_form = TLM_Static; /* TLM_Dynamic not yet implemented */

    Words_model = TLM_create_model (TLM_PT_Model, Model_title);
    Chars_model = TLM_create_model
          (TLM_PPM_Model, Model_title, CHAR_ALPHABET_SIZE,
	   Chars_model_max_order, DEFAULT_CHAR_ESCAPE_METHOD, PERFORMS_EXCLS);

    open_input_files (Input_filename);
    open_output_files (Output_filename);

    process_file (Input_text_file, Words_model, Chars_model);

    TLM_write_model (Output_words_model_file, Words_model, model_form);
    TLM_write_model (Output_chars_model_file, Chars_model, model_form);

    if (Debug.level > 4)
      {
	fprintf (stderr, "\nDump of word model:\n" );
        TLM_dump_model (Stderr_File, Words_model, NULL);

	fprintf (stderr, "\nDump of character model:\n" );
        TLM_dump_model (Stderr_File, Chars_model, NULL);
      }

    TLM_release_model (Words_model);
    TLM_release_model (Chars_model);

    exit (0);
}
