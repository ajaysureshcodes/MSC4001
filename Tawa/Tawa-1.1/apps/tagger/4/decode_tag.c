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

#define MAX_FILENAME_SIZE 128                       /* Maximum size of a filename */

#define DEFAULT_MODEL_TITLE "TMT"                   /* Dummy name for model */
#define DEFAULT_TAGS_MAX_ORDER 2                    /* Default max order for tag model */
#define DEFAULT_TAGS_ESCAPE_METHOD TLM_PPM_Method_D /* Default escape method for tags model */
#define DEFAULT_CHARS_MAX_ORDER 4                   /* Default max order for character model */
#define DEFAULT_CHARS_ESCAPE_METHOD TLM_PPM_Method_D/* Default escape method for chars model */
#define DEFAULT_MULTIPLE_CHARS_MODELS TRUE          /* Default multiple chars models for each tag */
#define CHARS_ALPHABET_SIZE 256                     /* Size of the character model's alphabet */

#define PERFORMS_EXCLS TRUE

char *Model_title;

unsigned int Coder;

unsigned int Tags_model_max_order = DEFAULT_TAGS_MAX_ORDER;

unsigned int Chars_model_max_order = DEFAULT_CHARS_MAX_ORDER;

boolean Has_multiple_chars_models = DEFAULT_MULTIPLE_CHARS_MODELS;

char *Title = NULL;
char Input_filename [MAX_FILENAME_SIZE];
char Output_filename [MAX_FILENAME_SIZE];
char Tagset_filename [MAX_FILENAME_SIZE];

unsigned int Input_file;
unsigned int Input_tagset_file;

unsigned int Output_file;

void
open_input_files (char *filename, char *tagset_filename)
/* Opens all the input files for reading. */
{
  Input_file = TXT_open_file (filename, "r",
      "Reading from input file",
      "Decode_tag: can't open input file" );

  sprintf (filename, "%s", tagset_filename);
  Input_tagset_file = TXT_open_file (filename, "r",
      "Reading from tagset file",
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
	     "Usage: decode_tag [options] -i encoded-input-file -o output-file -t tagset-file\n"
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
	     "  -p n\tprogress report every n words\n");
    fprintf (stderr,
             "  -r n\tdebug coding ranges\n"
	     "  -t fn\ttagset filename=fn (required argument)\n"
	     "  -T n\tlong description (title) of model\n"
	     "  -1 n\tmax order of tag model=n (optional; default=2)\n"
	     "  -2 n\tmax order of character model=n (optional; default=4)\n"
	     "  -3\thas single character model i.e.not one for each tag (optional; default=no)\n"
	);
    exit (2);
}

void
init_arguments (int argc, char *argv[])
{
    extern char *optarg;
    extern int optind;
    int opt;

    boolean Input_found = FALSE, Tagset_found = FALSE;
    boolean Output_found = FALSE;

    Model_title = DEFAULT_MODEL_TITLE;

    /* get the argument options */
    Debug.level = 0;
    Debug.range = FALSE;
    while ((opt = getopt (argc, argv, "ABd:i:o:p:rt:T:1:2:3")) != -1)
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
	    Tagset_found = TRUE;
	    sprintf (Tagset_filename, "%s", optarg);
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
	case '3':
	    Has_multiple_chars_models = FALSE;
	    break;
	default:
	    usage ();
	    break;
	}

    fprintf (stderr, "\nCreating new models\n\n");

    if (!Tagset_found)
        fprintf (stderr, "\nFatal error: missing tagset filename\n\n");
    if (!Input_found)
        fprintf (stderr, "\nFatal error: missing input filename\n\n");
    if (!Output_found)
        fprintf (stderr, "\nFatal error: missing output filename\n\n");
    if (!Tagset_found || !Input_found || !Output_found)
      {
	usage ();
	exit (1);
      }
    for (; optind < argc; optind++)
	usage ();
}

void
process_file (unsigned int tag_model, unsigned int coder)
/* Decodes the sequence of tags, words and characters in the text. */
{
    unsigned int tag_context, tag_word_symbol, word_pos;
    unsigned int tags_model, chars_model, chars_models;
    unsigned int tagset_table, tags_alphabet_size;
    unsigned int words_table, words_index;
    unsigned int tag_id, tag, word;

    TLM_get_model (tag_model, &tagset_table, &words_table, &words_index,
		   &tags_alphabet_size, &tags_model, &chars_model, &chars_models);
    TLM_set_context_operation (TLM_Get_Nothing);

    tag_context = TLM_create_context (tag_model);

    word_pos = 0;
    for (;;)
    {
        word_pos++;
	if ((Debug.progress > 0) && ((word_pos % Debug.progress) == 0))
	  {
	    fprintf (stderr, "Processing word pos %d\n", word_pos);
	    /*dump_memory (Stderr_File);*/
	  }
	tag_word_symbol = TLM_decode_symbol (tag_model, tag_context, coder);
	if (tag_word_symbol == NIL)
	    break; /* EOF */

	/* Both tag_id and word have been encoded as a single symbol */
	tag_id = tag_word_symbol % tags_alphabet_size;
	word = tag_word_symbol / tags_alphabet_size;
	tag = TXT_getkey_table (tagset_table, tag_id);

	if (Debug.range)
	  {
	    fprintf (stderr, "Pos %d Decoded word {", word_pos);
	    TXT_dump_text (Stderr_File, word, TXT_dump_symbol);
	    fprintf (stderr, "} tag {");
	    TXT_dump_text (Stderr_File, tag, TXT_dump_symbol);
	    fprintf (stderr, "} tag_id %d\n", tag_id);
	  }

	/* Write out the tag and word to output file on separate lines */
	TXT_dump_text (Output_file, tag, TXT_dump_symbol);
	fputc ('\n', Files [Output_file]);
	TXT_dump_text (Output_file, word, TXT_dump_symbol);
	fputc ('\n', Files [Output_file]);
    }

    TLM_release_context (tag_model, tag_context);

    if (Debug.range || (Debug.progress != 0))
        fprintf (stderr, "Processed %d words\n", word_pos);
}

int
main (int argc, char *argv[])
{
    unsigned int tagset_table;
    unsigned int tag_model;

    init_arguments (argc, argv);

    open_input_files (Input_filename, Tagset_filename);
    open_output_files (Output_filename);

    Coder = TLM_create_coder
      (CODER_MAX_FREQUENCY, NIL, NIL, Input_file, Stdout_File,
       arith_encode, arith_decode, arith_decode_target);

    arith_decode_start (Input_file);

    tagset_table = TXT_load_table_keys (Input_tagset_file);

    tag_model = TLM_create_model
      (TLM_TAG_Model, Model_title, tagset_table,
       Tags_model_max_order, Chars_model_max_order,
       Has_multiple_chars_models); 

    process_file (tag_model, Coder);

    arith_decode_finish (Input_file);

    TLM_release_model (tag_model);
    TLM_release_coder (Coder);
    TXT_release_table (tagset_table);

    exit (0);
}
