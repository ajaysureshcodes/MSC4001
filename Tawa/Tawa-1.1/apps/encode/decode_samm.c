/* Semi-adaptive decoder (based on characters) */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef SYSTEM_LINUX
#include <getopt.h> /* for getopt on Linux systems */
#endif

#include "io.h"
#include "text.h"
#include "model.h"
#include "coder.h"

unsigned int Model;

void
usage (void)
{
    fprintf (stderr,
	     "Usage: encode1 [options] training-model <input-text\n"
	     "\n"
	     "options:\n"
	     "  -m fn\tmodel filename=fn\n"
	     "  -p n\treport progress every n chars\n"
	     "  -r\tdebug arithmetic coding ranges\n"
	);
    exit (2);
}

void
init_arguments (int argc, char *argv[])
{
    extern char *optarg;
    extern int optind;
    unsigned int model_found;
    int opt;

    /* get the argument options */

    model_found = 0;
    while ((opt = getopt (argc, argv, "d:m:p:r")) != -1)
	switch (opt)
	{
	case 'p':
	    Debug.progress = atoi (optarg);
	    break;
	case 'm':
	    Model = TLM_read_model (optarg, "Loading model from file",
				    "Encode: can't open model file");
	    model_found = 1;
	    break;
	case 'r':
	    Debug.range = TRUE;
	    break;
	default:
	    usage ();
	    break;
	}
    if (!model_found)
    {
        fprintf (stderr, "\nFatal error: missing model\n\n");
        usage ();
    }
    for (; optind < argc; optind++)
	usage ();
}

void
decodeText (unsigned int model, unsigned int coder)
/* Encodes the text using the ppm model. */
{
    int pos;
    unsigned int symbol;
    unsigned int context;

    context = TLM_create_context (model);

    pos = 0;
    for (;;)
    {
        pos++;
	if ((Debug.progress > 0) && ((pos % Debug.progress) == 0))
	    fprintf (stderr, "position %d\n", pos);

	symbol = TLM_decode_symbol (model, context, coder);
	if (symbol == TXT_sentinel_symbol ())
	    break;

	putc (symbol, stdout);
    }
    TLM_release_context (model, context);
}

int
main (int argc, char *argv[])
{
    unsigned int Coder;

    Coder = TLM_create_arithmetic_coder ();

    init_arguments (argc, argv);

    arith_decode_start (Stdin_File);

    decodeText (Model, Coder);

    arith_decode_finish (Stdin_File);

    /*TLM_release_model (Model);*/
    TLM_release_coder (Coder);

    return (0);
}
