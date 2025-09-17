/* Generates a "random" sequence of text. */
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>

#ifdef SYSTEM_LINUX
#include <getopt.h> /* for getopt on Linux systems */
#endif

#include "io.h"
#include "text.h"
#include "coderanges.h"
#include "model.h"

#define MAX_ALPHABET 256 /* Default maximum alphabet size for all PPM models */

unsigned int Model;
unsigned int TextSize;

void
usage (void)
{
    fprintf (stderr,
	     "Usage: generate [options]\n"
	     "\n"
	     "options:\n"
	     "  -b fn\tnumber of bytes to generate=b (required)\n"
	     "  -m fn\tmodel filename=fn (required)\n"
	);
    exit (2);
}

void
init_arguments (int argc, char *argv[])
{
    extern char *optarg;
    extern int optind;
    unsigned int file;
    boolean textsize_found;
    boolean model_found;
    int opt;

    /* get the argument options */

    textsize_found = FALSE;
    model_found = FALSE;
    while ((opt = getopt (argc, argv, "b:m:")) != -1)
	switch (opt)
	{
	case 'b':
	    TextSize = atoi (optarg);
	    textsize_found = TRUE;
	    break;
	case 'm':
	    file = TXT_open_file (optarg, "r", NULL,
				  "Error opening model file");
	    Model = TLM_load_model (file);
	    model_found = TRUE;
	    break;
	default:
	    usage ();
	    break;
	}
    if (!textsize_found)
    {
        fprintf (stderr, "\nFatal error: missing number of bytes\n\n");
        usage ();
    }
    if (!model_found)
    {
        fprintf (stderr, "\nFatal error: missing models filename\n\n");
        usage ();
    }
    for (; optind < argc; optind++)
	usage ();
}

int randy (int lo, int hi)
/* Returns a random number in the range LO to HI. */
{
    return (lo + random() % (hi - lo + 1));
}

void
generateText (unsigned int model, unsigned int text, unsigned int text_size)
/* Generates the "random" text using the PPM model. */
{
    unsigned int coderanges, context, pos, symbol, lbnd, hbnd, totl, target, target_totl;
    float codelength;
    boolean found_symbol;

    TLM_set_context_operation (TLM_Get_Codelength);

    codelength = 0.0;
    context = TLM_create_context (model);
    TLM_set_context_operation (TLM_Get_Coderanges);  

    /* Now generate the symbols */
    for (pos = 0; pos < text_size; pos++)
    {
        found_symbol = FALSE;
	target = 0;
	target_totl = 0;
	TLM_reset_symbol (model, context);
        while (!found_symbol && TLM_next_symbol (model, context, &symbol))
        {
	  coderanges = TLM_Coderanges;
	  /*fprintf (stderr, "Symbol = %3d coderanges: ", symbol);
	    TLM_dump_coderanges (Stderr_File, coderanges);*/
	  /*TLM_reset_coderanges (coderanges);*/
	  TLM_next_coderange (coderanges, &lbnd, &hbnd, &totl);
	  /*fprintf (stderr, "lbnd = %d hbnd = %d totl = %d\n", lbnd, hbnd, totl);*/
	  if (target_totl != totl)
	    {
	      target = randy (0, totl);
	      target_totl = totl;
	    }

	  if (target < hbnd) 
	      found_symbol = TRUE;
	}
	/*fprintf (stderr, "generated symbol = %d\n", symbol);*/
	if (symbol < 128)
	  TXT_append_symbol (text, symbol);
	TLM_reset_symbol (model, context);

	TLM_update_context (model, context, symbol);
        coderanges = TLM_Coderanges;
        /*fprintf (stderr, "Codelength = %.3f ",
	  TLM_codelength_coderanges (coderanges));*/
        /*TLM_dump_coderanges (Stderr_File, coderanges);*/
    }
    TLM_release_context (model, context);

    /*TXT_append_symbol (text, '\n');*/
}

int
main (int argc, char *argv[])
{
    unsigned int test_text;

    srandom (123457);

    init_arguments (argc, argv);

    test_text = TXT_create_text ();
    /*test_text = TXT_load_text (Stdin_File);*/

    generateText (Model, test_text, TextSize);

    TXT_dump_file (Stdout_File, test_text);

    return 0;
}
