/* Writes out the codelength divergences for two models on a string of text. */
#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
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

FILE *Input_fp;

char *Args_input_filename;
char *Args_model_filename1;
char *Args_model_filename2;

boolean Absolute = FALSE;
boolean Entropy = FALSE;
boolean Lines = FALSE;
boolean Words = FALSE;

void
usage (void)
{
    fprintf (stderr,
	     "Usage: ident_file [options] <input-text\n"
	     "\n"
	     "options:\n"
	     "  -A\tcalculate absolute values\n"
	     "  -C\tcalculate cross-entropy rather than codelength\n"
	     "  -L\tprint out line numbers\n"
	     "  -W\tcalculate for each word rather than each character\n"
	     "  -1 fn\tmodel filename 1=fn (required)\n"
	     "  -2 fn\tmodel filename 2=fn (required)\n"
	     "  -i fn\tinput filename=fn (required)\n"
	);
    exit (2);
}

void
init_arguments (int argc, char *argv[])
{
    extern char *optarg;
    extern int optind;
    boolean model1_found;
    boolean model2_found;
    boolean input_found;
    int opt;

    /* get the argument options */

    model1_found = FALSE;
    model2_found = FALSE;
    input_found = FALSE;
    while ((opt = getopt (argc, argv, "ACLWi:1:2:")) != -1)
	switch (opt)
	{
	case 'A':
	    Absolute = TRUE;
	    break;
	case 'C':
	    Entropy = TRUE;
	    break;
	case 'L':
	    Lines = TRUE;
	    break;
	case 'W':
	    Words = TRUE;
	    break;
	case '1':
	    Args_model_filename1 = (char *)
	        malloc ((strlen (optarg)+1) * sizeof (char));
	    strcpy (Args_model_filename1, optarg);
	    model1_found = TRUE;
	    break;
	case '2':
	    Args_model_filename2 = (char *)
	        malloc ((strlen (optarg)+1) * sizeof (char));
	    strcpy (Args_model_filename2, optarg);
	    model2_found = TRUE;
	    break;
	case 'i':
	    Args_input_filename = (char *)
	        malloc ((strlen (optarg)+1) * sizeof (char));
	    strcpy (Args_input_filename, optarg);  
	    input_found = TRUE;
	    break;
	default:
	    usage ();
	    break;
	}
    if (!model1_found || !model2_found)
    {
        fprintf (stderr, "\nFatal error: missing model filename\n\n");
        usage ();
    }
    if (!input_found)
    {
        fprintf (stderr, "\nFatal error: missing input filename\n\n");
        usage ();
    }
    for (; optind < argc; optind++)
	usage ();
}

void
divergenceText (unsigned int model1, unsigned int model2, unsigned int text)
/* Returns the divergences in codelength for encoding the text using the two PPM models. */
{
    unsigned int context1, context2, pos, wpos, symbol, words, max_order;
    int model1_max_order;
    float total1, total2, diverge;

    TLM_get_model (model1, PPM_Get_Max_Order, &max_order);
    model1_max_order = (int) max_order;
    fprintf (stderr, "max order = %d\n", model1_max_order);

    TLM_set_context_operation (TLM_Get_Codelength);

    context1 = TLM_create_context (model1);
    context2 = TLM_create_context (model2);

    /* Now encode each symbol */
    pos = 0;
    wpos = 0;
    words = 0;
    total1 = 0.0;
    total2 = 0.0;
    while (TXT_get_symbol (text, pos, &symbol))
    {
        if (isspace (symbol) && (symbol != ' '))
	  {
	    symbol = ' ';
	    TXT_put_symbol (text, symbol, pos);
	  }
	TLM_update_context (model1, context1, symbol);
	total1 += TLM_Codelength;
	if (Lines)
	  fprintf (stderr, "%5d: ", pos);
	if (!Words)
	    fprintf (stderr, "[%c %3d] model1 - %.3f\n", symbol, symbol, TLM_Codelength);
	TLM_update_context (model2, context2, symbol);
	if (!Words)
	    fprintf (stderr, "[%c %3d] model2 - %.3f\n", symbol, symbol, TLM_Codelength);
	total2 += TLM_Codelength;
	if (Words && (symbol == ' '))
	  {
	    diverge = total1 - total2;
	    if (Absolute)
	      {
		if (diverge < 0)
		  diverge = - diverge;
	      }
	    if (Lines)
	        fprintf (stderr, "%5d: ", pos);
	    if (wpos > model1_max_order)
	      {
		fprintf (stderr, "%7.3f [%6.3f, %6.3f] ", diverge, total1, total2);
		TXT_dump_text2 (Stderr_File, text, wpos - model1_max_order - 1,
				model1_max_order + 1, TXT_dump_symbol);
		TXT_dump_word (Stderr_File, text, wpos, TXT_dump_symbol);
		fprintf (stderr, "\n");
	      }
	    total1 = 0.0;
	    total2 = 0.0;
	    wpos = pos + 1;
	    words++;
	  }

	pos++;
    }
    TLM_release_context (model1, context1);
    TLM_release_context (model2, context2);
}

int
main (int argc, char *argv[])
{
  unsigned int Model1, Model2, text, input_file;

    init_arguments (argc, argv);

    input_file = TXT_open_file
      (Args_input_filename, "r", "Reading from text file",
       "Divergence: can't open input text file" );

    Model1 = TLM_read_model (Args_model_filename1, NULL, NULL);
    Model2 = TLM_read_model (Args_model_filename2, NULL, NULL);

    input_file = TXT_open_file
      (Args_input_filename, "r", NULL,
       "Divergence: can't open input file");
    text = TXT_load_text (input_file);

    divergenceText (Model1, Model2, text);

    return 0;
}
