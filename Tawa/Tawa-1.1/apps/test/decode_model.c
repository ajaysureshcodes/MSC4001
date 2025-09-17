/* Trie routines for PPM models. */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <math.h>
#include <malloc.h>
#include <string.h>

#ifdef SYSTEM_LINUX
#include <getopt.h> /* for getopt on Linux systems */
#endif

#include "SSS_coder.h"
#include "io.h"
#include "coder.h"
#include "trie.h"
#include "context.h"
#include "model.h"

#define MAX_FREQUENCY (1 << 27)

unsigned int Start, Step, Stop;

int Max_order;
FILE *Online_model_fp;

void
usage (void)
{
    fprintf (stderr,
	     "Usage: encode [options] training-model <input-text\n"
	     "\n"
	     "options:\n"
	     "  -o n\torder of model=n\n"
	     "  -p n\treport progress every n nodes\n"
	     "  -r\tdebug arithmetic coding ranges\n"
	     "  -1 n\tstart code\n"
	     "  -2 n\tstep code\n"
	     "  -3 n\tstop code\n"
	);
    exit (2);
}

void
init_arguments (int argc, char *argv[])
{
    extern char *optarg;
    extern int optind;
    boolean order_found;
    boolean SSS_coder_found;
    int opt;

    /* get the argument options */

    order_found = FALSE;
    SSS_coder_found = FALSE;
    while ((opt = getopt (argc, argv, "d:o:p:r1:2:3:")) != -1)
	switch (opt)
	{
	case 'o':
	    order_found = TRUE;
	    Max_order = atoi (optarg);
	    break;
	case 'p':
	    Debug.progress = atoi (optarg);
	    break;
	case 'r':
	    Debug.range = TRUE;
	    break;
	case '1':
	    SSS_coder_found = TRUE;
	    Start = atoi (optarg);
	    break;
	case '2':
	    SSS_coder_found = TRUE;
	    Step = atoi (optarg);
	    break;
	case '3':
	    SSS_coder_found = TRUE;
	    Stop = atoi (optarg);
	    break;
	default:
	    usage ();
	    break;
	}
    if (!order_found || !SSS_coder_found)
    {
        fprintf (stderr, "\nFatal error: missing model\n\n");
        usage ();
    }
    for (; optind < argc; optind++)
	usage ();
}

void
decode_trie (unsigned int model, unsigned int context, unsigned int coder,
	     unsigned int escape_symbol)
/* Encodes the model trie. */
{
    unsigned int symbol, depth;

    depth = 1;
    for (;;)
      {
	symbol = TLM_decode_symbol (model, context, coder);
	if (symbol == escape_symbol)
	  {
	    depth--;
	    printf ("~[%d]", depth);
	  }
	else
	  {
	    printf ("%c[%d]", symbol, depth);
	    depth++;
	  }

	if (depth == 0)
	    break;
      }
    printf ("\n");
}

void
decode_model (unsigned int offline_model, unsigned int coder, unsigned int escape_symbol)
/* Decodes the model. */
{
    unsigned int offline_context;

    offline_context = TLM_create_context (offline_model);
    decode_trie (offline_model, offline_context, coder, escape_symbol);
    TLM_release_context (offline_model, offline_context);
}

int
main (int argc, char *argv[])
{
    unsigned int alphabet_size, escape_method;
    unsigned int coder, offline_model;

    alphabet_size = 256;

    coder = TLM_create_arithmetic_coder ();

    init_arguments (argc, argv);

    offline_model = TLM_create_model
        (TLM_PPM_Model, "Dummy title", alphabet_size+1, Max_order, escape_method, TRUE);

    arith_decode_start (Stdin_File);

    decode_model (offline_model, coder, alphabet_size);

    arith_decode_finish (Stdin_File);

    TLM_release_model (offline_model);
    TLM_release_coder (coder);

    exit (0);
}
