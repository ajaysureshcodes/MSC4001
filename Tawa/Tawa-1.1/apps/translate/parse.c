/* Performs parsing/chunking on output from POS tagger. */
#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <assert.h>

#ifdef SYSTEM_LINUX
#include <getopt.h> /* for getopt on Linux systems */
#endif

#include "io.h"
#include "model.h"
#include "text.h"
#include "table.h"

#define MAX_FILENAME_SIZE 128      /* Maximum size of a filename */

unsigned int Input_file;
char Input_filename [MAX_FILENAME_SIZE];

unsigned int Output_file;
char Output_filename [MAX_FILENAME_SIZE];

unsigned int Rules_file;
char Rules_filename [MAX_FILENAME_SIZE];

unsigned int Debug_level = 0;
unsigned int Debug_progress = 0;

void
usage (void)
{
    fprintf (stderr,
	     "Usage: parse [options]\n"
	     "\n"
	     "options:\n"
	     "  -d n\tdebug level=n.\n"
	     "  -i fn\ttagged input filename=fn (required argument)\n"
	     "  -o fn\tparsed output filename=fn (required argument)\n"
	     "  -p n\tprogress report every n lines.\n"
	     "  -r fn\trules filename=fn (required argument)\n"
	);
    exit (2);
}

void
init_arguments (int argc, char *argv[])
{
    int opt;
    extern char *optarg;
    extern int optind;

    boolean Input_file_found = FALSE;
    boolean Output_file_found = FALSE;
    boolean Rules_file_found = FALSE;

    /* get the argument options */

    Debug_level = 0;
    Debug_progress = 0;
    while ((opt = getopt (argc, argv, "d:i:o:p:r:")) != -1)
	switch (opt)
	{
	case 'd':
	    Debug_level = atoi (optarg);
	    break;
	case 'i':
	    Input_file_found = TRUE;
	    sprintf (Input_filename, "%s", optarg);
	    break;
	case 'o':
	    Output_file_found = TRUE;
	    sprintf (Output_filename, "%s", optarg);
	    break;
	case 'p':
	    Debug_progress = atoi (optarg);
	    break;
	case 'r':
	    Rules_file_found = TRUE;
	    sprintf (Rules_filename, "%s", optarg);
	    break;
	default:
	    usage ();
	    break;
	}

    if (!Input_file_found)
        fprintf (stderr, "\nFatal error: missing name of Input text filename\n\n");

    if (!Output_file_found)
        fprintf (stderr, "\nFatal error: missing name of Output text filename\n\n");

    if (!Rules_file_found)
        fprintf (stderr, "\nFatal error: missing name of Rules text filename\n\n");

    if (!Input_file_found || !Output_file_found || !Rules_file_found)
      {
	usage ();
	exit (1);
      }

    for (; optind < argc; optind++)
	usage ();
}

void
dump_symbol (unsigned int file, unsigned int symbol)
/* Writes the symbol as a number. */
{
    assert (TXT_valid_file (file));

    fprintf (Files [file], "<%d>", symbol);
}

void
load_input (unsigned int input_file, unsigned int words, unsigned int tags)
/* Insert a sentinel word in 0th position to ensure replacement
   rules work properly for first word/tag */
{
    unsigned int word, tag, pos;

    TXT_append_symbol (words, TXT_createsentinel_text ());
    TXT_append_symbol (tags, TXT_createsentinel_text ());

    pos = 0;
    word = TXT_create_text ();
    tag = TXT_create_text ();
    while ((TXT_readline_text (input_file, word) > 0) &&
	   (TXT_readline_text (input_file, tag) > 0))
    {
      pos++;
      if (Debug_level > 1)
	{
	  fprintf (stderr, "Pos %d word: ", pos);
	  TXT_dump_text (Stderr_File, word, TXT_dump_symbol);
	  fprintf (stderr,  " tag: " );
	  TXT_dump_text (Stderr_File, tag, TXT_dump_symbol);
	  fprintf (stderr, "\n");
	}

      /* Save all the words and tags into a text array */
      TXT_append_symbol (words, word);
      TXT_append_symbol (tags, tag);

      word = TXT_create_text ();
      tag = TXT_create_text ();
    }

    /* Insert a sentinel word in last position to ensure
       chunks are recognized for last word/tag */
    TXT_append_symbol (words, TXT_createsentinel_text ());
    TXT_append_symbol (tags, TXT_createsentinel_text ());

    if (Debug_level > 2)
      {
	fprintf (stderr, "Dump of words:\n");
	TXT_dump_text (Stderr_File, words, dump_symbol);
	fprintf (stderr, "Dump of tags:\n");
	TXT_dump_text (Stderr_File, tags, dump_symbol);
      }
}

void
add_rule (unsigned int rules, unsigned int context, unsigned int replacement)
/* Adds a rule into the rules table. The LHS of the rule (the context) is specified
   by the context_string, and the RHS of the rule (what the string gets replaced with)
   is specified by the replacement_string. */
{
    TXT_insert_table (rules, context, replacement, 1);
}

unsigned int
load_rules (unsigned int file)
/* Loads from the file the rule table that defines how the contexts
   should be replaced when they are recognized. */
{
    unsigned int rules, comment, context, replacement, pos;

    pos = 0;

    comment = TXT_create_text ();

    rules = TXT_create_table (TLM_Dynamic, 0);

    for (;;)
      {
	context = TXT_create_text ();
	replacement = TXT_create_text ();

	if ((TXT_readline_text (file, comment) <= 0) ||
	    (TXT_readline_text (file, context) <= 0) ||
	    (TXT_readline_text (file, replacement) <= 0))
	    break;

	pos++;
	if (Debug_level > 1)
	  {
	    fprintf (stderr, "Loading rule %d, comment: ", pos);
	    TXT_dump_text (Stderr_File, comment, TXT_dump_symbol);
	    fprintf (stderr,  " context: " );
	    TXT_dump_text (Stderr_File, context, TXT_dump_symbol);
	    fprintf (stderr,  " replacement: " );
	    TXT_dump_text (Stderr_File, replacement, TXT_dump_symbol);
	    fprintf (stderr, "\n");
	  }

	add_rule (rules, context, replacement);
      }

    TXT_release_text (comment);

    return (rules);
}

void
check_rule (unsigned int context, unsigned int rules, unsigned int pos,
	    unsigned int replaced_symbols)
/* Check to see if there is a rule for the context in the rules table; if there is, then
   generate the symbol as specified by the rule and add it to the replaced_symbols
   list. */
{
    unsigned int replacement_symbol, rule_count;

    if (Debug_level > 2)
      {
        fprintf (stderr, "Pos %d, Checking rules table for context [", pos);
	TXT_dump_text (Stderr_File, context, NULL);
        fprintf (stderr, "]\n");
      }

    if (TXT_getid_table (rules, context, &replacement_symbol, &rule_count))
      { /* Rule matches; apply the rule by adding the symbol identifier
	   (i.e. replacement_symbol) to the replaced symbols list */
	TXT_append_symbol (replaced_symbols, replacement_symbol);

	if (Debug_level > 2)
	  {
	    fprintf (stderr, "Pos %d, Adding context [", pos);
	    TXT_dump_text (Stderr_File, replacement_symbol, NULL);
	    fprintf (stderr, "]\n");
	  }

      }
}

void
dump_channel (unsigned int file, unsigned int replaced_symbols,
	      unsigned int channel_text)
/* Dumps out the channel identified by the channel_text. */
{
    unsigned int symbols, symbol, pos, pos1, pos2;

    for (pos = 0; pos < TXT_length_text (replaced_symbols); pos++)
      {
	assert (TXT_get_symbol (replaced_symbols, pos, &symbols)); 
	if (symbols)
	  for (pos1 = 0; pos1 < TXT_length_text (symbols); pos1++)
	    {
	      assert (TXT_get_symbol (symbols, pos1, &symbol));

	      if (Debug_level > 3)
		{
		  fprintf (stderr, "Searching Pos %d Symbol ", pos);
		  TXT_dump_text (Stderr_File, symbol, NULL);
		  fprintf (stderr, "\n");
		}

	      if (TXT_find_text (symbol, channel_text, &pos2) &&
		  (pos2 == 0))
		{
		  /* Found a replaced symbol on this channel that
		     matches the channel text at the beginning */
		  fprintf (Files [file], "pos: %d ", pos);
		  TXT_dump_text (file, symbol, NULL);
		  fprintf (Files [file], "\n");
		}
	    }
      }
}

void
process_rules (unsigned int rules, unsigned int words, unsigned int tags)
/* Processes the words and tags using the rules table. */
{
  unsigned int all_replaced_symbols, rsymbol, rsymbol1, pos, rpos, rpos1;
  unsigned int curr_replaced_symbols, prev_replaced_symbols, curr_len;
  unsigned int context1, context2, context3;
  unsigned int word, tag, pword, ptag;

  /* Process combinations of two words & or two tags max.; if a "chunk" could be
     longer than this, there needs to be a rule in the rules table to allow for this;
     i.e. it has to be built up incrementally */

  all_replaced_symbols = TXT_create_text (); /* list of symbols replaced for current context
						based on the rules table at each word pos */
  for (pos = 0; pos < TXT_length_text (words); pos++)
      TXT_append_symbol (all_replaced_symbols, NIL);

  context1 = TXT_create_text ();
  context2 = TXT_create_text ();
  context3 = TXT_create_text ();

  for (pos = 1; pos < TXT_length_text (words); pos++)
    { /* For each pair of words/tags, apply replacement rules */

      assert (TXT_get_symbol (words, pos-1, &pword));
      assert (TXT_get_symbol (tags, pos-1, &ptag));

      assert (TXT_get_symbol (words, pos, &word));
      assert (TXT_get_symbol (tags, pos, &tag));

      /* Create a text list (curr_replaced_symbols) for storing replaced symbols
	 at this position */
      curr_replaced_symbols = TXT_create_text ();
      TXT_put_symbol (all_replaced_symbols, curr_replaced_symbols, pos);
      TXT_setlength_text (curr_replaced_symbols, 0);

      assert (TXT_get_symbol (all_replaced_symbols, pos-1, &prev_replaced_symbols));

      /* First, for current word/tag combinations, check if there are rules for them */
      /* (1) Check if just the word on the word channel exists by itself */
      TXT_setlength_text (context1, 0);
      TXT_append_string (context1, "w: "); /* This indicates the "word" channel */
      TXT_append_text (context1, word);
      check_rule (context1, rules, pos, curr_replaced_symbols);

      /* (2) Check if just the tag on the tag channel exists by itself */
      TXT_setlength_text (context2, 0);
      TXT_append_string (context2, "t: "); /* This indicates the "tag" channel */
      TXT_append_text (context2, tag);
      check_rule (context2, rules, pos, curr_replaced_symbols);

      /* (3) Check if both the word and tag on the word & tag channels exists */
      TXT_setlength_text (context3, 0);
      TXT_append_string (context3, "w: "); /* This indicates the "word" channel */
      TXT_append_text (context3, word);
      TXT_append_string (context3, " t: "); /* This is followed by the "tag" channel */
      TXT_append_text (context3, tag);
      check_rule (context3, rules, pos, curr_replaced_symbols);

      /* Next, for each word/tag pair (previous word/tag + current word/tag, check if there
	 is a rule for it */
      /* (1) Check if the two words at current pos on the word channel exist */
      TXT_setlength_text (context1, 0);
      TXT_append_string (context1, "w: "); /* This indicates the "word" channel */
      TXT_append_text (context1, pword);
      TXT_append_string (context1, " w: "); /* This indicates the "word" channel */
      TXT_append_text (context1, word);
      check_rule (context1, rules, pos, curr_replaced_symbols);

      /* (2) Check if the two tags at current pos on the tag channel exist */
      TXT_setlength_text (context2, 0);
      TXT_append_string (context2, "t: "); /* This indicates the "tag" channel */
      TXT_append_text (context2, ptag);
      TXT_append_string (context2, " t: "); /* This indicates the "tag" channel */
      TXT_append_text (context2, tag);
      check_rule (context2, rules, pos, curr_replaced_symbols);

      /* (3) Check if the two word & tag pairs tag on the word & tag channels exist */
      TXT_setlength_text (context3, 0);
      TXT_append_string (context3, "w: "); /* This indicates the "word" channel */
      TXT_append_text (context3, pword);
      TXT_append_string (context3, " t: "); /* This indicates the "tag" channel */
      TXT_append_text (context3, ptag);
      TXT_append_string (context3, " w: "); /* This indicates the "word" channel */
      TXT_append_text (context3, word);
      TXT_append_string (context3, " t: "); /* This indicates the "tag" channel */
      TXT_append_text (context3, tag);
      check_rule (context3, rules, pos, curr_replaced_symbols);

      /* Next, for each of the previous replaced symbols followed by current word/tag, check if
	 there is a rule for it */
      for (rpos = 0; rpos < TXT_length_text1 (prev_replaced_symbols); rpos++)
	{
	  assert (TXT_get_symbol (prev_replaced_symbols, rpos, &rsymbol));

	  /* (1) Check if the previous replaced symbol + current word exists */
	  TXT_setlength_text (context1, 0);
	  TXT_append_text (context1, rsymbol);
	  TXT_append_string (context1, " w: "); /* This indicates the "word" channel */
	  TXT_append_text (context1, word);
	  check_rule (context1, rules, pos, curr_replaced_symbols);

	  /* (2) Check if the previous replaced symbol + current tag exists */
	  TXT_setlength_text (context2, 0);
	  TXT_append_text (context2, rsymbol);
	  TXT_append_string (context2, " t: "); /* This indicates the "tag" channel */
	  TXT_append_text (context2, tag);
	  check_rule (context2, rules, pos, curr_replaced_symbols);

	  /* (3) Check if the previous replaced symbol + current word & tag exists */
	  TXT_setlength_text (context3, 0);
	  TXT_append_text (context3, rsymbol);
	  TXT_append_string (context3, " w: "); /* This indicates the "word" channel */
	  TXT_append_text (context3, word);
	  TXT_append_string (context3, " t: "); /* This is followed by the "tag" channel */
	  TXT_append_text (context3, tag);
	  check_rule (context3, rules, pos, curr_replaced_symbols);
	}

      /* Next, for each of the current replaced symbols, check if there
	 is a rule for it */
      curr_len = TXT_length_text1 (curr_replaced_symbols);
      for (rpos = 0; rpos < curr_len; rpos++)
	{
	  assert (TXT_get_symbol (curr_replaced_symbols, rpos, &rsymbol));
	  /* Check if current replaced symbol exists */
	  TXT_setlength_text (context1, 0);
	  TXT_append_text (context1, rsymbol);
	  check_rule (context1, rules, pos, curr_replaced_symbols);
	}

      /* Next, for each of the previous replaced symbols followed
	 by current replaced symbol, check if there is a rule for it */
      for (rpos = 0; rpos < TXT_length_text1 (prev_replaced_symbols); rpos++)
	{
	  for (rpos1 = 0; rpos1 < curr_len; rpos1++)
	    {
	      assert (TXT_get_symbol (prev_replaced_symbols, rpos, &rsymbol));
	      assert (TXT_get_symbol (curr_replaced_symbols, rpos1, &rsymbol1));
	      /* Check if the previous replaced symbol + current replaced symbol exists */
	      TXT_setlength_text (context1, 0);
	      TXT_append_text (context1, rsymbol);
	      TXT_append_symbol (context1, ' ');
	      TXT_append_text (context1, rsymbol1);
	      check_rule (context1, rules, pos, curr_replaced_symbols);
	    }
	}
    }

  /* Dump out the chunk channel */
  TXT_setlength_text (context1, 0);
  TXT_append_string (context1, "c: ");
  dump_channel (Output_file, all_replaced_symbols, context1);

  TXT_release_text (context1);
  TXT_release_text (context2);
  TXT_release_text (context3);
}

int
main (int argc, char *argv[])
{
    unsigned int rules, words, tags;

    init_arguments (argc, argv);

    Input_file = TXT_open_file
      (Input_filename, "r", "Reading from input file",
       "parse: can't open input file" );

    Output_file = TXT_open_file
      (Output_filename, "w", "Writing to output file",
       "parse: can't open output file" );

    Rules_file = TXT_open_file
      (Rules_filename, "r", "Reading from rules file",
       "parse: can't open rules file" );

    words = TXT_create_text ();
    tags = TXT_create_text ();

    rules = load_rules (Rules_file);

    load_input (Input_file, words, tags);

    process_rules (rules, words, tags);
	
    exit (0);
}
