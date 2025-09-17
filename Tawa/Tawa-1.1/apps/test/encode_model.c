/* Trie routines for PPM models. */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <math.h>
#include <string.h>
#include <unistd.h>

#ifdef SYSTEM_LINUX
#include <getopt.h> /* for getopt on Linux systems */
#endif

#include "SSS_coder.h"
#include "ppm_trie.h"
#include "ppm_model.h"
#include "io.h"
#include "coder.h"
#include "model.h"

#define MAX_FREQUENCY (1 << 27)

unsigned int Start, Step, Stop;

boolean Debug_symbols = FALSE;
int Max_order;

unsigned int Online_model;

void
usage (void)
{
    fprintf (stderr,
	     "Usage: encode [options] training-model <input-text\n"
	     "\n"
	     "options:\n"
	     "  -d\tdump out symbols\n"
	     "  -m fn\tmodel filename=fn\n"
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
    boolean model_found;
    boolean order_found;
    boolean SSS_coder_found;
    int opt;

    /* get the argument options */

    model_found = FALSE;
    order_found = FALSE;
    SSS_coder_found = FALSE;
    while ((opt = getopt (argc, argv, "dm:o:p:r1:2:3:")) != -1)
	switch (opt)
	{
	case 'd':
	    Debug_symbols = TRUE;
	    break;
	case 'm':
	    Online_model =
	        TLM_read_model (optarg, "Loading model from file",
				"Encode_model: can't open model file");
	    model_found = TRUE;
	    break;
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
    if (!model_found || !order_found || !SSS_coder_found)
    {
        fprintf (stderr, "\nFatal error: missing model\n\n");
        usage ();
    }
    for (; optind < argc; optind++)
	usage ();
}

unsigned int Nodes;

void
encode_trie (unsigned int model, unsigned int context, unsigned int coder,
	     unsigned int escape_symbol, struct PPM_trieType *trie, int node,
	     unsigned int d, unsigned int max_depth)
/* Encodes the model trie. */
{
    unsigned int sptr, tcount, next_sptr, symbol;
    int child;

    assert (node > NIL);

    Nodes++;
    if (Debug.progress && ((Nodes % Debug.progress) == 0))
        fprintf (stderr, "Processed %d nodes\n", Nodes);

    PPM_get_trie_node (trie, node, &tcount, &sptr); /* get start of list of symbols */
    /*SSS_encode (tcount-1, Start, Step, Stop);*/

    /* Now store nodes for each symbol if needed */
    while (sptr)
      {
	PPM_get_slist (trie, sptr, &symbol, &child, &next_sptr);

	if (Debug_symbols)
	    printf ("%c[%d]", symbol, d+1);
	else
	    TLM_encode_symbol (model, context, coder, symbol);
	if (child < 0)
	  if (Debug_symbols)
	    printf ("~[%d]", d+1);
	  else
	    TLM_encode_symbol (model, context, coder, escape_symbol);
	else if (d < max_depth)
	    encode_trie (model, context, coder, escape_symbol, trie, child,
			 d+1, max_depth);
	sptr = next_sptr; /* advance to next symbol in slist */
      }
    if (Debug_symbols)
        printf ("~[%d]", d+1);
    else
        TLM_encode_symbol (model, context, coder, escape_symbol);
}

void
encode_model (unsigned int online_model, unsigned int offline_model,
	      unsigned int coder, unsigned int escape_symbol)
/* Encodes the model. */
{
    unsigned int offline_context, max_order, ppm_model;
    struct PPM_trieType *trie;

    if (online_model != NIL)
      {
	ppm_model = Models [online_model].M_model;
	assert (PPM_valid_model (ppm_model));

	trie = PPM_Models [ppm_model].P_trie;
	max_order = PPM_Models [ppm_model].P_max_order;

	Nodes = 0;
	offline_context = TLM_create_context (offline_model);
	if (trie != NULL)
	  {
	    encode_trie (offline_model, offline_context, coder, escape_symbol,
			 trie, TRIE_ROOT_NODE, 0, max_order);
	    if (Debug_symbols)
	        putchar ('~');
	    else
	        TLM_encode_symbol (offline_model, offline_context, coder,
				   escape_symbol);
	  }
	TLM_release_context (offline_model, offline_context);
      }
}

int
main (int argc, char *argv[])
{
    unsigned int coder, offline_model;
    unsigned int alphabet_size, escape_method;

    coder = TLM_create_arithmetic_coder ();

    init_arguments (argc, argv);

    TLM_get_model (Online_model, PPM_Get_Alphabet_Size, &alphabet_size);
    TLM_get_model (Online_model, PPM_Get_Escape_Method, &escape_method);

    offline_model = TLM_create_model
        (TLM_PPM_Model, "Test", alphabet_size+1, Max_order, escape_method, TRUE);

    arith_encode_start (Stdout_File);

    encode_model (Online_model, offline_model, coder, alphabet_size);

    arith_encode_finish (Stdout_File);

    TLM_release_model (Online_model);
    TLM_release_model (offline_model);
    TLM_release_coder (coder);

    exit (1);
}
