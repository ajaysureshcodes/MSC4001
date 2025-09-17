/* Writes out statistics about the model. */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#ifdef SYSTEM_LINUX
#include <getopt.h> /* for getopt on Linux systems */
#endif

#include "io.h"
#include "model.h"

unsigned int Model;

void
usage (void)
{
    fprintf (stderr,
	     "Usage: stats [options] training-model <input-text\n"
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
				    "Stats: can't open model file");
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
main (int argc, char *argv[])
{
    init_arguments (argc, argv);

    TLM_stats_model (Stdout_File, Model);

    TLM_release_model (Model);

    exit (0);
}
