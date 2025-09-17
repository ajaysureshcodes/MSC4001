/* Writes out only the symbols that have been successfully predicted
   by the highest context. (Uses TLM_previous_symbol ()). */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef SYSTEM_LINUX
#include <getopt.h> /* for getopt on Linux systems */
#endif

#include "io.h"
#include "text.h"
#include "model.h"

unsigned int Model, Context;

void
usage (void)
{
    fprintf (stderr,
	     "Usage: probs [options] training-model <input-text\n"
	     "\n"
	     "options:\n"
	     "  -m fn\tmodel filename=fn\n"
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
    while ((opt = getopt (argc, argv, "d:m:")) != -1)
	switch (opt)
	{
	case 'm':
	    Model = TLM_read_model (optarg, "Loading model from file",
				    "Test1: can't open model file");
	    model_found = 1;
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

int
getText (FILE *fp, unsigned int *text, int max)
/* Read from FP into TEXT; return its length (maximum length = MAX). */
{
    int i;
    int cc;

    i = 0;
    cc = '\0';
    while ((--max > 0) && ((cc = getc(fp)) != EOF) && (cc != '\n'))
        text [i++] = cc;
    text [i] = TXT_sentinel_symbol ();
    text [i+1] = '\0';
    return (i+1);
}

void
symbolsText (unsigned int model, unsigned int *text, unsigned int textlen)
/* Returns the probabilities for encoding the text using the ppm model. */
{
    unsigned int p, sym;

    /* Now encode each symbol */
    for (p=0; p < textlen; p++) /* encode each symbol */
    {
        fprintf (stdout, "\nPosition = %d\n", p);
        while (TLM_next_symbol (Context, &sym))
	{
	    fprintf (stdout, "Sym = %3d\n", sym);
	}
	sym = text [p];
	TLM_update_context (Context, sym);
    }
}

int
main (int argc, char *argv[])
{
    unsigned int text [1024];
    int textlen;

    init_arguments (argc, argv);

    Context = TLM_create_context (Model);

    printf ("\nEnter text:\n");
    while ((textlen = getText (stdin, text, 1024)) != EOF)
    {
	symbolsText (Model, text, textlen);
	printf ("\nEnter text:\n");
    }        

    TLM_release_context (Context);
    TLM_release_model (Model);

    exit (0);
}
