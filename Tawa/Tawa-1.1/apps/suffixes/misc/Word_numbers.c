/* Barfs out a stream of numbers corresponding to the words in the input file. */

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#ifdef SYSTEM_LINUX
#include <getopt.h> /* for getopt on Linux systems */
#endif

#include "io.h"
#include "text.h"
#include "table.h"

unsigned int Input_file, Output_file;

void
usage (void)
{
    fprintf (stderr,
	     "Usage: Words -i word-input-file -o numbers-output-file\n"
	     "(Word input text consists of lines of text - one word per line\n"
	     "\n");
    fprintf (stderr,
	     "options:\n"
	     "  -i fn\tinput filename=fn (required argument)\n"
	     "  -o fn\toutput filename=fn (required argument)\n"
	);
    exit (2);
}

void
init_arguments (int argc, char *argv[])
{
    extern char *optarg;
    extern int optind;
    int opt;

    boolean Input_found = FALSE;
    boolean Output_found = FALSE;

    /* get the argument options */
    Debug.level = 0;
    Debug.range = FALSE;
    while ((opt = getopt (argc, argv, "i:o:")) != -1)
	switch (opt)
	{
	case 'i':
	    Input_found = TRUE;
	    Input_file = TXT_open_file
	      (optarg, "r", "Reading from text file",
	       "Words: can't open input text file" );
	    break;
	case 'o':
	    Output_found = TRUE;
	    Output_file = TXT_open_file
	      (optarg, "w", "Writing from text file",
	       "Words: can't open output text file" );
	    break;
	default:
	    usage ();
	    break;
	}

    if (!Input_found)
        fprintf (stderr, "\nFatal error: missing input filename\n\n");
    if (!Output_found)
        fprintf (stderr, "\nFatal error: missing output filename\n\n");
    if (!Input_found || !Output_found)
      {
	usage ();
	exit (1);
      }
    else
      for (; optind < argc; optind++)
	usage ();
}

void load_source_file (unsigned int input_file, unsigned int output_file)
{
    unsigned int word, word_id, word_count;
    unsigned int word_table;
    boolean eof;

    word = TXT_create_text ();
    word_table = TXT_create_table (TLM_Dynamic, 0);
    for (;;)
      {
	eof = (TXT_readline_text (input_file, word) == EOF);

	if (eof)
	    break;

	TXT_update_table (word_table, word, &word_id, &word_count);

	fprintf (Files [output_file], "%d\n", word_id);
      }
}

int main (int argc, char *argv [])
{
    init_arguments (argc, argv);

    load_source_file (Input_file, Output_file);
  
    exit (0);
}
