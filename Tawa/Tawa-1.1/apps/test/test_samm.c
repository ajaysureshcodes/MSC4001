/* Program for testing whether a model has been coccrectly loaded. */
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>

#ifdef SYSTEM_LINUX
#include <getopt.h> /* for getopt on Linux systems */
#endif

#include "io.h"
#include "text.h"
#include "coderanges.h"
#include "samm_model.h"
#include "model.h"

unsigned int Model;

unsigned int DebugLevel = 0;

void
usage (void)
{
    fprintf (stderr,
	     "Usage: generate [options]\n"
	     "\n"
	     "options:\n"
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
    boolean model_found;
    int opt;

    /* get the argument options */

    model_found = FALSE;
    while ((opt = getopt (argc, argv, "d:m:")) != -1)
	switch (opt)
	{
	case 'd':
	    DebugLevel = atoi (optarg);
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
    if (!model_found)
    {
        fprintf (stderr, "\nFatal error: missing models filename\n\n");
        usage ();
    }
    for (; optind < argc; optind++)
	usage ();
}

int
main (int argc, char *argv[])
{
    init_arguments (argc, argv);

    TLM_check_model (Stderr_File, Model, NULL);

    if (DebugLevel > 2)
        TLM_dump_model (Stderr_File, Model, NULL);

    return 0;
}
