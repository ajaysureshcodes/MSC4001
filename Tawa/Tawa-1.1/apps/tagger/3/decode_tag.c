/* Decodes the input file of tags & words. */
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
#include "coder.h"
#include "pt_model.h"

#define MAX_FILENAME_SIZE 128      /* Maximum size of a filename */

#define DEFAULT_MODEL_TITLE "TMT"  /* Dummy name for model */
#define DEFAULT_TAGS_MAX_ORDER 2   /* Default max order for tag model */
#define DEFAULT_TAGS_ESCAPE_METHOD TLM_PPM_Method_D  /* Default escape method for tags model */
#define DEFAULT_CHARS_MAX_ORDER 4  /* Default max order for character model */
#define DEFAULT_CHARS_ESCAPE_METHOD TLM_PPM_Method_D /* Default escape method for chars model */
#define CHARS_ALPHABET_SIZE 256    /* Size of the character model's alphabet */

#define PERFORMS_EXCLS TRUE

char *Model_title;

unsigned int Coder;

unsigned int Tags_model;
unsigned int Tags_model_max_order = DEFAULT_TAGS_MAX_ORDER;
unsigned int Tags_alphabet_size = 0;

unsigned int Words_model;

unsigned int Chars_model;
unsigned int Chars_model_max_order = DEFAULT_CHARS_MAX_ORDER;

char *Title = NULL;
char Tagslist_filename [MAX_FILENAME_SIZE];
char Input_filename [MAX_FILENAME_SIZE];
char Output_filename [MAX_FILENAME_SIZE];

unsigned int Input_file;
unsigned int Input_tagslist_file;

unsigned int Output_file;

void
open_input_files (char *filename, char *tagslist_filename)
/* Opens the encoded input file. */
{
  Input_file = TXT_open_file (filename, "r",
      "Reading from input file",
      "Decode_tag: can't open input file" );

  sprintf (filename, "%s", tagslist_filename);
  Input_tagslist_file = TXT_open_file (filename, "r",
      "Reading from tagslist file",
      "Decode_tag: can't open text file" );
}

void
open_output_files (char *filename)
/* Opens all the input files for reading. */
{
  sprintf (filename, "%s", filename);
  Output_file = TXT_open_file (filename, "w",
      "Writing to text file",
      "Decode_tag: can't open text file" );
}

void
usage (void)
{
    fprintf (stderr,
	     "Usage: decode_tag [options] -i encoded-input-file -o output-file -t tagslist-file\n"
	     "(Tagged input text consists of two lines of text - one line\n"
             " for each term (word) and the second line the term's tag)\n"
	     "\n");
    fprintf (stderr,
	     "options:\n"
             "  -A n\tdebug arithmetic coder\n"
             "  -B n\tdebug arithmetic coder targets\n"
             "  -d n\tdebugging level=n\n"
	     "  -i fn\tencoded input filename=fn (required argument)\n"
	     "  -o fn\toutput filename=fn (required argument)\n"
	     "  -p n\tprogress report every n words\n"
             "  -r n\tdebug coding ranges\n"
	     "  -t fn\ttagslist filename=fn (required argument)\n"
	     "  -T n\tlong description (title) of model\n"
	     "  -1 n\tmax order of tag model=n (optional; default=2)\n"
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

    boolean Input_found = FALSE, Tagslist_found = FALSE;
    boolean Output_found = FALSE;

    Model_title = DEFAULT_MODEL_TITLE;

    /* get the argument options */
    Debug.level = 0;
    Debug.range = FALSE;
    while ((opt = getopt (argc, argv, "ABd:i:o:p:rt:T:1:2:")) != -1)
	switch (opt)
	{
	case 'A':
	    Debug.coder = TRUE;
	    break;
	case 'B':
	    Debug.coder_target = TRUE;
	    break;
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
	case 'r':
	    Debug.range = TRUE;
	    break;
	case 't':
	    Tagslist_found = TRUE;
	    sprintf (Tagslist_filename, "%s", optarg);
	    break;
	case 'T':
	    Model_title = (char *) malloc (strlen (optarg)+1);
	    strcpy (Model_title, optarg);
	    break;
	case '1':
	    Tags_model_max_order = atoi (optarg);
	    break;
	case '2':
	    Chars_model_max_order = atoi (optarg);
	    break;
	default:
	    usage ();
	    break;
	}

    fprintf (stderr, "\nCreating new models\n\n");

    if (!Tagslist_found)
        fprintf (stderr, "\nFatal error: missing tagslist filename\n\n");
    if (!Input_found)
        fprintf (stderr, "\nFatal error: missing input filename\n\n");
    if (!Output_found)
        fprintf (stderr, "\nFatal error: missing output filename\n\n");
    if (!Tagslist_found || !Input_found || !Output_found)
      {
	usage ();
	exit (1);
      }
    for (; optind < argc; optind++)
	usage ();
}

boolean
process_tag_word
(unsigned int tagslist_table, unsigned int tags_model,
 unsigned int tags_context,
 unsigned int words_model, unsigned int words_context,
 unsigned int chars_model, unsigned int chars_context,
 unsigned int tag, unsigned int prev_word, unsigned int word,
 unsigned int coder)
/* Decodes the tag, and word. */
{
    unsigned int tag_id, tag_key, word1, symbol;

    /* decode and update the tag context */
    tag_id = TLM_decode_symbol (tags_model, tags_context, coder);
    if (!tag_id) /* EOF */
        return (FALSE);

    if (Debug.range)
        fprintf (stderr, " tag id: %d\n", tag_id);

    tag_key = TXT_getkey_table (tagslist_table, tag_id - IS_ASCII); /* tag id was not printable */
    TXT_overwrite_text (tag, tag_key);

    PT_Novel_Symbols = TRUE;

    word1 = NIL;
    if (prev_word != NIL)
      { /* Decode and update the p(Wn | Tn Wn-1) context */
	TXT_setlength_text (words_context, 0);
	TXT_append_symbol (words_context, tag_id);
	TXT_append_text (words_context, prev_word);
	TXT_append_symbol (words_context, TXT_sentinel_symbol ());
	/* The sentinel symbol is used here to separate out the words in
	   the context since some "words" may in fact have spaces in them */
	word1 = TLM_decode_symbol (words_model, words_context, coder);
      }

    /* Create the higher order word contexts, and update the words model with them */
    /* Decode and update the p(Wn | Tn) context */
    TXT_setlength_text (words_context, 0);
    TXT_append_symbol (words_context, tag_id);
    if (PT_Novel_Symbols)
	word1 = TLM_decode_symbol (words_model, words_context, coder);

    /* Decode and update the order 0 word context */
    TXT_setlength_text (words_context, 0);
    TXT_append_symbol (words_context, TXT_sentinel_symbol ());
    if (PT_Novel_Symbols)
	word1 = TLM_decode_symbol (words_model, words_context, coder);
    TXT_overwrite_text (word, word1);

    /* Decode and update the character contexts */
    if (PT_Novel_Symbols)
      {
	TXT_setlength_text (word, 0);
	for (;;)
	  { /* Each novel word is terminated with a sentinel symbol as some "words"
	       may have nulls in them */
	    symbol = TLM_decode_symbol (chars_model, chars_context, coder);
	    if (symbol == TXT_sentinel_symbol ())
	        break;

	    TXT_append_symbol (word, symbol);
	  }

	if (Debug.level > 2)
	  {
	    fprintf (stderr, "Novel word: ");
	    TXT_dump_text (Stderr_File, word, TXT_dump_symbol);
	    fprintf (stderr, "\n");
	  }
      }

    /* Now update the contexts using the decoded word if necessary */
    PT_update_decode_context (words_model, word);

    return (TRUE);
}

void
process_file (unsigned int tagslist_table,
	      unsigned int tags_model,
	      unsigned int words_model,
	      unsigned int chars_model,
	      unsigned int coder)
/* Decodes the sequence of tags, words and characters in the text. */
{
    unsigned int tags_context, words_context, chars_context;
    unsigned int tag, prev_word, word, word_pos;
    boolean eof;

    TLM_set_context_operation (TLM_Get_Nothing);

    prev_word = NIL;
    tag = TXT_create_text ();
    word = TXT_create_text ();
    tags_context = TLM_create_context (tags_model);
    words_context = TXT_create_text ();
    chars_context = TLM_create_context (chars_model);

    word_pos = 0;
    for (;;)
    {
        word_pos++;
	if ((Debug.progress > 0) && ((word_pos % Debug.progress) == 0))
	  {
	    fprintf (stderr, "Processing word pos %d\n", word_pos);
	    /*dump_memory (Stderr_File);*/
	  }
	eof = !process_tag_word
	  (tagslist_table, tags_model, tags_context,
	   words_model, words_context, chars_model, chars_context,
	   tag, prev_word, word, coder);

	if (eof)
	    break;

	if (Debug.range)
	  {
	    fprintf (stderr, "Pos %d Decoded word {", word_pos);
	    TXT_dump_text (Stderr_File, word, TXT_dump_symbol);
	    fprintf (stderr, "} tag {");
	    TXT_dump_text (Stderr_File, tag, TXT_dump_symbol);
	    fprintf (stderr, "}\n");
	  }

	/* Write out the tag and word to output file on separate lines */
	TXT_dump_text (Output_file, tag, TXT_dump_symbol);
	fprintf (Files [Output_file], "\n");
	TXT_dump_text (Output_file, word, TXT_dump_symbol);
	fprintf (Files [Output_file], "\n");
	
	if (prev_word == NIL)
	    prev_word = TXT_create_text ();
	TXT_overwrite_text (prev_word, word);
    }

    TLM_release_context (tags_model, tags_context);
    TLM_release_context (words_model, words_context);
    TLM_release_context (chars_model, chars_context);

    TXT_release_text (tag);
    TXT_release_text (word);
    if (prev_word != NIL)
        TXT_release_text (prev_word);

    if (Debug.range || (Debug.progress != 0))
        fprintf (stderr, "Processed %d words\n", word_pos);
}

int
main (int argc, char *argv[])
{
    unsigned int tagslist_table, tagslist_type;
    unsigned int tagslist_types_count, tagslist_tokens_count;

    init_arguments (argc, argv);

    open_input_files (Input_filename, Tagslist_filename);
    open_output_files (Output_filename);

    Coder = TLM_create_coder
      (CODER_MAX_FREQUENCY, NIL, NIL, Input_file, Stdout_File,
       arith_encode, arith_decode, arith_decode_target);

    arith_decode_start (Input_file);

    tagslist_table = TXT_load_table_keys (Input_tagslist_file);
    TXT_getinfo_table (tagslist_table, &tagslist_type, &tagslist_types_count,
		       &tagslist_tokens_count);

    Tags_model = TLM_create_model
          (TLM_PPM_Model, Model_title, IS_ASCII + tagslist_types_count + 1,
	   Tags_model_max_order, DEFAULT_TAGS_ESCAPE_METHOD, PERFORMS_EXCLS);
    Words_model = TLM_create_model (TLM_PT_Model, Model_title);
    Chars_model = TLM_create_model
          (TLM_PPM_Model, Model_title, CHARS_ALPHABET_SIZE,
	   Chars_model_max_order, DEFAULT_CHARS_ESCAPE_METHOD, PERFORMS_EXCLS);

    process_file (tagslist_table, Tags_model, Words_model, Chars_model, Coder);

    arith_decode_finish (Input_file);

    if (Debug.level > 4)
      {
	fprintf (stderr, "\nDump of word model:\n" );
        TLM_dump_model (Stderr_File, Words_model, NULL);

	fprintf (stderr, "\nDump of character model:\n" );
        TLM_dump_model (Stderr_File, Chars_model, NULL);
      }

    TLM_release_model (Tags_model);
    TLM_release_model (Words_model);
    TLM_release_model (Chars_model);

    TLM_release_coder (Coder);

    TXT_release_table (tagslist_table);

    exit (0);
}
