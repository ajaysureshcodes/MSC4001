/* Works out the language of a string of text. */
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>

#ifdef SYSTEM_LINUX
#include <getopt.h> /* for getopt on Linux systems */
#endif

#include "io.h"
#include "text.h"
#include "model.h"
#include "ppm_model.h"

#define MAX_ALPHABET 256 /* Default maximum alphabet size for all PPM models */

unsigned int Model, Expand;

void
usage (void)
{
    fprintf (stderr,
	     "Usage: ident_file [options]\n"
	     "\n"
	     "options:\n"
	     "  -e n\texpand alphabet size by n (required)\n"
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
    boolean expand_found;
    boolean model_found;
    int opt;

    /* get the argument options */

    expand_found = FALSE;
    model_found = FALSE;
    while ((opt = getopt (argc, argv, "e:m:")) != -1)
	switch (opt)
	{
	case 'e':
	    Expand = atoi (optarg);
	    expand_found = TRUE;
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
    if (!expand_found)
    {
        fprintf (stderr, "\nFatal error: missing size of alphabet expansion\n\n");
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

void
codelengthText (unsigned int model, unsigned int text)
/* Returns the codelength for encoding using the PPM model. */
{
    unsigned int context, pos, symbol;
    float codelength;

    TLM_set_context_type (TLM_Get_Codelength);

    codelength = 0.0;
    context = TLM_create_context (model);

    /* Now encode symbol */
    pos = 0;
    while (TXT_get_symbol (text, pos++, &symbol))
    {
	TLM_update_context (model, context, symbol);
	codelength += TLM_Codelength;
	fprintf (stderr, "Codelength for symbol %d = %.3f\n", symbol,
		 TLM_Codelength);
    }
    TLM_release_context (model, context);
}

int
main (int argc, char *argv[])
{
    unsigned int alphabet_size;
    unsigned int test_text, symbol;

    test_text = TXT_create_text ();

    init_arguments (argc, argv);

    TLM_get_model (Model, PPM_Get_Alphabet_Size, &alphabet_size);

    for (symbol = 0; symbol < alphabet_size; symbol++)
        TXT_append_symbol (test_text, symbol);
    codelengthText (Model, test_text);

    TLM_set_model (Model, PPM_Set_Alphabet_Size, alphabet_size + Expand);
    /* expands the alphabet size by Expand size */

    fprintf (stderr, "Testing expanded alphabet:\n");
    TXT_setlength_text (test_text, 0);
    for (symbol = 0; symbol < alphabet_size + Expand; symbol++)
        TXT_append_symbol (test_text, symbol);
    codelengthText (Model, test_text);

    return 0;
}
