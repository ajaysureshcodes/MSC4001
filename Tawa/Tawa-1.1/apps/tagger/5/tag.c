/* Tags text given a trained model. */
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

#define MAX_FILENAME_SIZE 128                       /* Maximum size of a filename */
#define MAX_TAGS_FOR_NOVEL_WORDS 8                  /* Max.no. of tags to try for novel words */
#define DEFAULT_TAGS_FOR_NOVEL_WORDS 3              /* Def.no. of tags to try for novel words */

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

unsigned int Tags_alphabet_size = 0;
unsigned int Tagset_table = NIL;
unsigned int Tag_model = NIL;
unsigned int Tags_model = NIL;
unsigned int Chars_model = NIL;
unsigned int *Chars_models = NULL;
unsigned int Words_context = NIL;
unsigned int Chars_context = NIL;
unsigned int *Chars_contexts = NULL;
unsigned int Words_Table = NIL;
unsigned int Words_Index = NIL;

char *Title = NULL;
char Model_filename [MAX_FILENAME_SIZE];
char Input_filename [MAX_FILENAME_SIZE];
char Output_filename [MAX_FILENAME_SIZE];

unsigned int Input_file = NIL;
unsigned int Output_file = NIL;

boolean Segment_Viterbi = TRUE;
unsigned int Segment_stack_depth = 0;

unsigned int Tags_For_Novel_Words = DEFAULT_TAGS_FOR_NOVEL_WORDS;

void
debug_tag ()
/* Dummy routine for debugging purposes. */
{
    fprintf (stderr, "Got here\n");
}

void
open_input_files (char *input_filename, char *model_filename,
		  unsigned int *tag_model)
/* Opens all the input files for reading. */
{
  Input_file = TXT_open_file (input_filename, "r",
      "Reading from text file",
      "tag: can't open input text file" );

  *tag_model = TLM_read_model (Model_filename,
      "Loading model from file",
      "tag: can't open model file");
}

void
open_output_files (char *filename)
/* Opens the encoded output file. */
{
  Output_file = TXT_open_file (filename, "w",
      "Writing to output file",
      "Encode_tag: can't open output file" );
}

void
usage (void)
{
    fprintf (stderr,
	     "Usage: tag [options] -m training-model-filename -i input-text -o tagged-output-text\n"
	     "(Tagged output text consists of two lines of text - one line\n"
             " for each term (word) and the second line the term's tag)\n"
	     "\n");
    fprintf (stderr,
	     "options:\n"
	     "  -D n\tstack algorithm (not Viterbi, the default): stack depth=n\n"
	     "  -d n\tdebug paths=n\n"
	     "  -i fn\tinput filename=fn (required argument)\n"
	     "  -l n\tdebug level=n\n"
	     "  -m fn\tmodel filename=fn (required argument)\n"
	     "  -n n\tnumber of tags to try for each novel word (default=3)\n"
	     "  -o fn\ttagged output filename=fn (required argument)\n"
	     "  -p n\tprogress report every n words\n"
	     "  -r n\tdebug ranges\n"
	);
    exit (2);
}

void
init_arguments (int argc, char *argv[])
{
    extern char *optarg;
    extern int optind; 
    int opt;

    boolean Input_found = FALSE, Output_found = FALSE;
    boolean Model_found = FALSE;

    /* set defaults */
    Debug.level = 0;

    /* get the argument options */

    while ((opt = getopt (argc, argv, "D:d:i:l:m:n:o:p:r")) != -1)
	switch (opt)
	{
	case 'D':
	    Segment_stack_depth = atoi (optarg);
	    Segment_Viterbi = FALSE;
	    break;
	case 'd':
	    Debug.level1 = atoi (optarg);
	    break;
	case 'i':
	    Input_found = TRUE;
	    sprintf (Input_filename, "%s", optarg);
	    break;
	case 'l':
	    Debug.level = atoi (optarg);
	    break;
	case 'm':
	    Model_found = TRUE;
	    sprintf (Model_filename, "%s", optarg);
	    break;
	case 'n':
	    Tags_For_Novel_Words = atoi (optarg);
	    if (Tags_For_Novel_Words > MAX_TAGS_FOR_NOVEL_WORDS)
	        Tags_For_Novel_Words = MAX_TAGS_FOR_NOVEL_WORDS;
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
	default:
	    usage ();
	    break;
	}

    if (!Input_found)
        fprintf (stderr, "\nFatal error: missing input filename\n\n");
    if (!Output_found)
        fprintf (stderr, "\nFatal error: missing output filename\n\n");
    if (!Model_found)
        fprintf (stderr, "\nFatal error: missing model filename\n\n");
    if (!Model_found || !Input_found || !Output_found)
      {
	usage ();
	exit (1);
      }

    for (; optind < argc; optind++)
	usage ();
}

void
dump_tag_symbol (unsigned int file, unsigned int tag_word_symbol)
/* Writes the ASCII symbol out in human readable form (excluding white space). */
{
    unsigned int tag_id, tag, word;
    /*char line [12];*/

    assert (TXT_valid_file (file));

    /*
    sprintf (line, "%d\n", tag_word_symbol);
    TXT_write_file (file, line);
    */

    tag_id = tag_word_symbol % Tags_alphabet_size;
    tag = TXT_getkey_table (Tagset_table, tag_id);
    word = tag_word_symbol / Tags_alphabet_size;
	
    /*fprintf (Files [file], "[%4d,%4d,%4d]: ", tag_id, tag, word);*/
    TXT_dump_text (file, word, NULL);
    fprintf (Files [file], "\n");
    TXT_dump_text (file, tag, NULL);
    fprintf (Files [file], "\n");
}

unsigned int Tag_Range_Text;    /* Used by tag_transform_function */

void
tag_transform_initialise (unsigned int tagset_table)
/* Initialises the data to be used by tag_transform_function. */
{
  unsigned int tag_key, tag_key_id, tag_key_count;

    /* Set Tag_Range_Text to the same size as a list of tag keys */
    Tag_Range_Text = TXT_create_text ();
    TXT_reset_table (tagset_table);
    while (TXT_next_table (tagset_table, &tag_key, &tag_key_id,
			   &tag_key_count))
      {
	/*
	fprintf (stderr, "key id %3d count %3d text {", tag_key_id,
		 tag_key_count);
	TXT_dump_text (Stderr_File, tag_key, NULL);
	fprintf (stderr, "}\n");
	*/

	TXT_append_symbol (Tag_Range_Text, 0); /* Insert dummy symbol */
      }
}

void
tag_find_confusions (unsigned int model, unsigned int source_text,
		     unsigned int source_pos, unsigned int source_symbol,
		     unsigned int previous_symbol, unsigned int range_text,
		     unsigned int *symbols_to_expand)
/* Support function used to mark up each word. Uses the Tags PPM model to
   generate possible tags for the word conditioned on the previous tag
   and rejects those with low probability. */
{
    unsigned int tag_word_symbol, tags_context, tags_symbol;
    unsigned int prev_tag_id, tag_id, p;
    unsigned int lbnd, hbnd, totl, count;
    unsigned int symbols_length = 0;
    boolean first_time;
    float prob = 0.0;

    /*
    fprintf (stderr, "Transform Function: source pos %d symbol %d prev %d\n",
	     source_pos, source_symbol, previous_symbol);

    fprintf (stderr, "Previous symbol = %d prev. tag id = %d word = %d\n",
	     previous_symbol, (previous_symbol % Tags_alphabet_size),
	     (previous_symbol / Tags_alphabet_size));
    if (TXT_valid_text (previous_symbol / Tags_alphabet_size))
      {
	fprintf (stderr, "{");
        TXT_dump_text (Stderr_File, (previous_symbol / Tags_alphabet_size), NULL);
	fprintf (stderr, "}\n");
      }
    */

    prev_tag_id = (previous_symbol % Tags_alphabet_size);

    TLM_set_context_operation (TLM_Get_Coderanges);

    tags_context = TLM_create_context (Tags_model);
    if (source_pos > 0)
        TLM_update_context (Tags_model, tags_context, prev_tag_id);

    /*fprintf (stderr, "tags symbol begin\n");*/
    while (TLM_next_symbol (Tags_model, tags_context, &tags_symbol))
      {
	/*fprintf (stderr, "tags symbol = %d", tags_symbol);*/

	TLM_reset_coderanges (TLM_Coderanges);

	prob = 0.0;
	count = 0;
	first_time = TRUE;
	while (TLM_next_coderange (TLM_Coderanges, &lbnd, &hbnd, &totl))
	  {
	    /*fprintf (stderr, " lbnd = %d hbnd = %d totl = %d",
	               lbnd, hbnd, totl);*/
	    if (first_time)
	      {
		count = hbnd - lbnd;
		prob = ((float) count) / totl;
		first_time = FALSE;
	      }
	  }
	/*fprintf (stderr, "\n");*/

	if (TLM_length_coderanges (TLM_Coderanges) > 1)
	    break;

	if (tags_symbol != TXT_sentinel_symbol ())
	  {
	    if (prob > 0.01)
	      { /* Remove symbols which aren't very frequent */
		symbols_to_expand [symbols_length++] = tags_symbol;
		/*fprintf (stderr, "Adding symbol %d\n", tags_symbol);*/
	      }	    
	  }

      }
    /*fprintf (stderr, "tags symbol end\n");*/

    TLM_set_context_operation (TLM_Get_Codelength);

    /*
    fprintf (stderr, "alphabet_size %d lang model = %d source pos = %d\n",
	     Tags_alphabet_size, Tags_model, source_pos);
    */

    /*assert (symbols_length > 0);*/
    TXT_setlength_text (Tag_Range_Text, 0);
    for (p = 0; p < symbols_length; p++)
      {
	tag_id = symbols_to_expand [p];

	tag_word_symbol = source_symbol * Tags_alphabet_size + tag_id;
	/* The source symbol should be the current word text record */

	/* Insert tag_word_symbol as a transform otion in the Tag_Range_Text */
	TXT_append_symbol (Tag_Range_Text, tag_word_symbol);
      }

    if (symbols_length == 0) /* This won't happen */
    for (p = 0; p < TXT_length_text (Tag_Range_Text); p++)
      {
	tag_id = p; /* The tag ids are in incremental order */

	tag_word_symbol = source_symbol * Tags_alphabet_size + tag_id;
	/* The source symbol should be the current word text record */

	/* Insert tag_word_symbol as a transform otion in the Tag_Range_Text */
	TXT_put_symbol (Tag_Range_Text, tag_word_symbol, p);
      }

    /*TLM_release_context (Tags_model, tags_context);*/

    /*
    for (p = 0; p < TXT_length_text (Tag_Range_Text); p++)
      {
	assert (TXT_get_symbol (Tag_Range_Text, p, &tag_word_symbol));
	tag_id = tag_word_symbol % Tags_alphabet_size;
	word = tag_word_symbol / Tags_alphabet_size;
	fprintf (stderr, "Tag Range %3d: %d, %d, %d\n",
		 p, tag_word_symbol, tag_id, word);
      }
    */
    /*
    fprintf (stderr, "Tag Range length %d\n",
	     TXT_length_text (Tag_Range_Text));
    */

    TXT_overwrite_text (range_text, Tag_Range_Text);
}

boolean Tag_best_tag_id_init = FALSE;
unsigned int Tag_best_tag_id [MAX_TAGS_FOR_NOVEL_WORDS];
/* Text record for storing the best tag ids for new words to save on recomputing it */

void
tag_find_best_tag_ids (unsigned int source_text, unsigned int source_pos,
		       unsigned int source_symbol, unsigned int previous_symbol)
/* Returns the best tag id so that in subsequent passes for the same source position it does
   not get re-calculated. */
{
    float codelength, best_codelength [MAX_TAGS_FOR_NOVEL_WORDS];
    unsigned int best_tag_id [MAX_TAGS_FOR_NOVEL_WORDS];
    unsigned int prev_tag_id, tag_id, tag, best_tag;
    unsigned int chars_model, chars_context;
    unsigned int text_length, symbol, pos, p, i, j;

    if (!Tag_best_tag_id_init)
      {
	Tag_best_tag_id_init = TRUE;
	for (i = 0; i < Tags_For_Novel_Words; i++)
	    Tag_best_tag_id [i] = TXT_create_text ();

	text_length = TXT_length_text (source_text);
	for (p = 0; p < text_length; p++) /* Set it all to sentinels */
	  for (i = 0; i < Tags_For_Novel_Words; i++)
	    TXT_append_symbol (Tag_best_tag_id [i], TXT_sentinel_symbol ());
      }
    assert (TXT_get_symbol (Tag_best_tag_id [0], source_pos, &best_tag));

    if (best_tag == TXT_sentinel_symbol ())
      { /* Hasn't been calculated yet - need to find out what it is and store it */
	for (i = 0; i < Tags_For_Novel_Words; i++)
	  {
	    best_tag_id [i] = 0;
	    best_codelength [i] = 999999.9;
	  }

	prev_tag_id = (previous_symbol % Tags_alphabet_size);
	for (tag_id = 0; tag_id < Tags_alphabet_size; tag_id++)
	  {
	    TXT_setlength_text (Words_context, 0);
	    TXT_append_symbol (Words_context, tag_id);

	    TLM_find_symbol (TAG_Models [Tag_model].TAG_words_model,
			     Words_context, source_symbol);

	    if (Debug.level1 > 5)
	      {
		tag = TXT_getkey_table (Tagset_table, tag_id);
		fprintf (stderr, "tag = ");
		TXT_dump_text (Stderr_File, tag, NULL);
		fprintf (stderr, " tag id = %d codelength = %.3f\n", tag_id,
			 TLM_Codelength);
	      }

	    /* Reset the context first */
	    if (Chars_contexts == NULL)
	      {
		chars_model = Chars_model;
		chars_context = Chars_context;
	      }
	    else
	      {
		chars_model = Chars_models [tag_id];
		chars_context = Chars_contexts [tag_id];
	      }
	    TLM_update_context (chars_model, chars_context, TXT_sentinel_symbol ());
	    pos = 0;
	    codelength = 0;
	    while (TXT_get_symbol (source_symbol, pos, &symbol))
	      {
		TLM_update_context (chars_model, chars_context, symbol);
		if (Debug.level1 > 5)
		    fprintf (stderr, "pos = %d symbol = %c codelength = %.3f\n",
			     pos, symbol, TLM_Codelength);
		codelength += TLM_Codelength;
		pos++;
	      }
	    TLM_update_context (chars_model, chars_context, TXT_sentinel_symbol ());
	    if (Debug.level1 > 5)
	        fprintf (stderr, "pos = %d symbol = <sentinel> codelength = %.3f\n",
			 pos, TLM_Codelength);
	    codelength += TLM_Codelength;
	    if (Debug.level1 > 5)
	        fprintf (stderr, "Total codelength = %.3f\n", codelength);

	    for (i = 0; i < Tags_For_Novel_Words; i++)
	      {
		if (codelength < best_codelength [i])
		  { /* shift everything along by 1 */
		    for (j = Tags_For_Novel_Words; j > i; j--)
		      {
			best_tag_id [j] = best_tag_id [j-1];
			best_codelength [j] = best_codelength [j-1];
		      }

		    /* insert new value */
		    best_tag_id [i] = tag_id;
		    best_codelength [i] = codelength;

		    break;
		  }
	      }
	  }

	if (Debug.level1 > 5)
	  {
	    fprintf (stderr, "Adding at pos %d for ", source_pos);
	    fprintf (stderr, "{");
	    TXT_dump_text (Stderr_File, source_symbol, NULL);
	    fprintf (stderr, "} tag ");
	  }

	for (i = 0; i < Tags_For_Novel_Words; i++)
	  {
	    if (Debug.level1 > 5)
	      {
		tag = TXT_getkey_table (Tagset_table, best_tag_id [i]);
		TXT_dump_text (Stderr_File, tag, NULL);
		fprintf (stderr, " tag id = %d codelength = %.3f ", best_tag_id [i],
			 best_codelength [i]);
	      }

	    TXT_put_symbol (Tag_best_tag_id [i], best_tag_id [i], source_pos);
	  }

	if (Debug.level1 > 5)
	  {
	    fprintf (stderr, "\n");
	  }
      }
}

void
tag_find_confusions1 (unsigned int model, unsigned int source_text,
		      unsigned int source_pos, unsigned int source_symbol,
		      unsigned int previous_symbol, unsigned int range_text,
		      unsigned int *symbols_to_expand)
/* Support function used to mark up each word. Finds the highest probabilities
   for p(Wn | Tn) and uses those. */
{
    unsigned int tag_id, best_tag_id [MAX_TAGS_FOR_NOVEL_WORDS];
    unsigned int tag_word_symbol, p, i;
    unsigned int symbols_length = 0;

    /*
    fprintf (stderr, "Transform Function: source pos %d symbol %d prev %d\n",
	     source_pos, source_symbol, previous_symbol);

    fprintf (stderr, "Previous symbol = %d prev. tag id = %d word = %d\n",
	     previous_symbol, (previous_symbol % Tags_alphabet_size),
	     (previous_symbol / Tags_alphabet_size));
    if (TXT_valid_text (previous_symbol / Tags_alphabet_size))
      {
	fprintf (stderr, "{");
        TXT_dump_text (Stderr_File, (previous_symbol / Tags_alphabet_size), NULL);
	fprintf (stderr, "}\n");
      }

    fprintf (stderr, "model = %d tag_model = %d\n", model, Tag_model);
    */

    tag_find_best_tag_ids (source_text, source_pos, source_symbol, previous_symbol);
    for (i = 0; i < Tags_For_Novel_Words; i++)
      {
        assert (TXT_get_symbol (Tag_best_tag_id [i], source_pos, &best_tag_id [i]));
	symbols_to_expand [symbols_length++] = best_tag_id [i];
      }

    assert (symbols_length > 0);
    TXT_setlength_text (Tag_Range_Text, 0);
    for (p = 0; p < symbols_length; p++)
      {
	tag_id = symbols_to_expand [p];

	tag_word_symbol = source_symbol * Tags_alphabet_size + tag_id;
	/* The source symbol should be the current word text record */

	/* Insert tag_word_symbol as a transform option in the Tag_Range_Text */
	TXT_append_symbol (Tag_Range_Text, tag_word_symbol);
      }

    TXT_overwrite_text (range_text, Tag_Range_Text);
}

void
tag_transform_function (unsigned int model, unsigned int source_symbol,
		     unsigned int previous_symbol, unsigned int source_text,
		     unsigned int source_pos, unsigned int range_text)
/* Function used to mark up each word. */
{
    unsigned int *symbols_to_expand;

    symbols_to_expand = (unsigned int *)
      malloc ((Tags_alphabet_size+1) * sizeof (unsigned int));

    tag_find_confusions (model, source_text, source_pos, source_symbol,
			 previous_symbol, range_text, symbols_to_expand);

    TXT_overwrite_text (range_text, Tag_Range_Text);

    free (symbols_to_expand);
}

void
tag_transform_function1 (unsigned int model, unsigned int source_symbol,
		      unsigned int previous_symbol, unsigned int source_text,
		      unsigned int source_pos, unsigned int range_text)
/* Function used to mark up each word. */
{
    unsigned int tag_word_symbol;
    unsigned int word_taglist, word_id, word_count, word;
    unsigned int prev_tag_id, tag_id, tag_freq, p;
    unsigned int *symbols_to_expand;
    unsigned int symbols_length = 0;

    symbols_to_expand = (unsigned int *)
      malloc ((Tags_alphabet_size+1) * sizeof (unsigned int));

    prev_tag_id = (previous_symbol % Tags_alphabet_size);

    word = source_symbol;
    if (TXT_getid_table (Words_Table, word, &word_id, &word_count))
      {
	/*
	fprintf (stderr, "Got word ");
	TXT_dump_text (Stderr_File, word, NULL);
	fprintf (stderr, " id = %d count = %d\n", word_id, word_count);
	*/

	assert (TXT_get_index (Words_Index, word_id, &word_taglist));
	/*TXT_dump_list (Stderr_File, word_taglist, NULL);*/

	/*fprintf (stderr, "Tag ids list = [");*/
	TXT_reset_list (word_taglist);
	while (TXT_next_list (word_taglist, &tag_id, &tag_freq))
	  {
	    /*fprintf (stderr, " %d:%d", tag_id, tag_freq);*/
	    symbols_to_expand [symbols_length++] = tag_id;
	  }
	/*fprintf (stderr, " ]\n");*/
      }

    TXT_setlength_text (Tag_Range_Text, 0);

    if (symbols_length == 0)
      { /* The word is new - it hasn't had any tags assigned to it in the
	   training data; so resort back to method used for tag_transform_function
	   above */
	tag_find_confusions1 (model, source_text, source_pos, source_symbol,
			      previous_symbol, range_text, symbols_to_expand);

      }
    else
      {
	for (p = 0; p < symbols_length; p++)
	  {
	    tag_id = symbols_to_expand [p];

	    tag_word_symbol = source_symbol * Tags_alphabet_size + tag_id;
	    /* The source symbol should be the current word text record */

	    /* Insert tag_word_symbol as a transform otion in the Tag_Range_Text */
	    TXT_append_symbol (Tag_Range_Text, tag_word_symbol);
	  }
      }

    /*fprintf (stderr, "Tag Range length %d\n", TXT_length_text (Tag_Range_Text));*/

    TXT_overwrite_text (range_text, Tag_Range_Text);

    free (symbols_to_expand);
}

void
tag_dump_symbol_function (unsigned int file, unsigned int symbol)
/* Dump function used for debugging transform process. */
{
      fputc ('<', Files [file]);
      if (!TXT_valid_text (symbol))
	  TXT_dump_symbol1 (file, symbol);
      TXT_dump_text (file, symbol, NULL);
      fputc ('>', Files [file]);
}

void
tag_dump_symbols_function (unsigned int file, unsigned int text)
/* Dump function used for debugging transform process. */
{
    unsigned int tag_word_symbol, tag_id, tag, word, len, pos;

    len = TXT_length_text (text);
    assert (len > 3);

    for (pos = 3; pos < len; pos++)
      {
	assert (TXT_get_symbol (text, pos, &tag_word_symbol));
	tag_id = tag_word_symbol % Tags_alphabet_size;
	tag = TXT_getkey_table (Tagset_table, tag_id);
	word = tag_word_symbol / Tags_alphabet_size;
	
	fprintf (Files [file], "[%4d,%4d,%4d]:\n", tag_id, tag, word);
	TXT_dump_text (file, tag, NULL);
	fputc ('\n', Files [file]);
	TXT_dump_text (file, word, NULL);
	fputc ('\n', Files [file]);
      }
}

void
tag_dump_confusion_function (unsigned int file, unsigned int confusion_text)
/* Dump function used for debugging transform process. */
{
    unsigned int tag_word_symbol, tag_id, tag, word;

    assert (TXT_length_text (confusion_text) == 1);
    assert (TXT_get_symbol (confusion_text, 0, &tag_word_symbol));

    tag_id = tag_word_symbol % Tags_alphabet_size;
    tag = TXT_getkey_table (Tagset_table, tag_id);
    word = tag_word_symbol / Tags_alphabet_size;

    fputc ('<', Files [file]);
    TXT_dump_text (file, word, NULL);
    fputc ('/', Files [file]);
    TXT_dump_text (file, tag, NULL);
    fputc ('>', Files [file]);

    fprintf (stderr, "{tag_word:%d,tag_id:%d,word:%d}",
	     tag_word_symbol, tag_id, word);
}

void
process_input_file (unsigned int input_file, unsigned int input_text)
/* Loads the input sequence of words and writes them as a sequence
   of word text symbols to the input text record. */
{
    unsigned int word, word_pos;
    boolean eof;

    TXT_setlength_text (input_text, 0);

    word = TXT_create_text ();

    word_pos = 0;
    eof = FALSE;
    for (;;)
    {
        word_pos++;
	eof = (TXT_readline_text (input_file, word) == EOF);
	/* Copy the word and append its text record number
	   as a "symbol" to the input text record */

	if (eof)
	    break;

	TXT_append_symbol (input_text, TXT_copy_text (word));
    }

    TXT_release_text (word);
}

int
main (int argc, char *argv[])
{
    unsigned int tagset_table_type, tagset_types_count, tagset_tokens_count;
    unsigned int input_text;   /* The input text to correct */
    unsigned int transform_text;  /* The marked up text */
    unsigned int transform_model;
    unsigned int tag_id;
    init_arguments (argc, argv);

    open_input_files (Input_filename, Model_filename, &Tag_model);
    open_output_files (Output_filename);

    TLM_get_model (Tag_model, &Tagset_table, &Words_Table, &Words_Index,
		   &Tags_alphabet_size, &Tags_model, &Chars_model,
		   &Chars_models);

    if (TLM_numberof_models () < 4)
      {
	fprintf (stderr, "Fatal error - incorrect number of models loaded\n");
	fprintf (stderr, "Number of models = %d (should be 4 or more)\n",
		 TLM_numberof_models ());
        usage();
	exit (1);
      }
    TXT_getinfo_table (Tagset_table, &tagset_table_type,
		       &tagset_types_count, &tagset_tokens_count);

    /*TLM_dump_model (Stderr_File, Tag_model, NULL);*/

    Words_context = TXT_create_text ();
    if (Chars_models == NULL)
        Chars_context = TLM_create_context (Chars_model);
    else
      {
	Chars_contexts = calloc (tagset_types_count + 1, sizeof (unsigned int));
	for (tag_id = 0; tag_id < tagset_types_count; tag_id++)
	  Chars_contexts [tag_id] = TLM_create_context (Chars_models [tag_id]);
      }

    input_text = TXT_create_text ();
    process_input_file (Input_file, input_text);

    if (Segment_Viterbi)
        transform_model = TMM_create_transform (TMM_Viterbi);
    else
        transform_model = TMM_create_transform (TMM_Stack, TMM_Stack_type0, Segment_stack_depth, 0);
    /*if (Debug.level1 > 1)*/
        TMM_debug_transform (tag_dump_symbol_function, tag_dump_symbols_function,
			  tag_dump_confusion_function);

    /* add transform for every possible tag */
    TMM_add_transform (transform_model, 0.0, "%w", "%f", tag_transform_function1);

    /*TMM_dump_transform (stdout, transform_model);*/

    tag_transform_initialise (Tagset_table);

    TMM_start_transform (transform_model, TMM_transform_multi_context, input_text,
		      Tag_model);

    transform_text = TMM_perform_transform (transform_model, input_text);
    /* Ignore the sentinel and model symbols that are inserted
       at the head of the marked up text. Also ignore trailing symbol. */
    TXT_dump_text1 (Output_file, transform_text, 3, dump_tag_symbol);

    TXT_release_text (input_text);
    TXT_release_text (transform_text);
    TXT_release_text (Words_context);
    TMM_release_transform (transform_model);

    if (Chars_models == NULL)
        TLM_release_context (Chars_model, Chars_context);
    else
      {
	for (tag_id = 0; tag_id < tagset_types_count; tag_id++)
	  TLM_release_context (Chars_models [tag_id], Chars_contexts [tag_id]);
      }

    exit (0);
}
