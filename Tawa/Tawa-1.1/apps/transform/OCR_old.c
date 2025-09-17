/* Corrects OCR output text given a trained model. */
#include <stdio.h>
#include <stdlib.h>

#ifdef SYSTEM_LINUX
#include <getopt.h> /* for getopt on Linux systems */
#endif

#include "io.h"
#include "text.h"
#include "paths.h"
#include "model.h"
#include "transform.h"

void
debug_OCR ()
/* Dummy routine for debugging purposes. */
{
    fprintf (stderr, "Got here\n");
}

void
usage (void)
{
    fprintf (stderr,
	     "Usage: OCR [options] training-model <input-text\n"
	     "\n"
	     "options:\n"
	     "  -l n\tdebug model=n\n"
	     "  -d n\tdebug paths=n\n"
	     "  -p n\tdebug progress=n\n"
	     "  -m fn\tlanguage model filename=fn\n"
	     "  -c fn\tconfusion matrix filename=fn\n"
	);
    exit (2);
}

void
init_arguments (int argc, char *argv[])
{
    extern char *optarg;
    extern int optind; 
    FILE *cfp, *mfp;
    int opt;

    /* set defaults */
    debugModel = 0;

    /* get the argument options */

    while ((opt = getopt (argc, argv, "d:l:m:p:c:")) != -1)
	switch (opt)
	{
        case 'c':
            fprintf( stderr, "Loading confusion matrix from file %s\n", optarg);
	    if ((cfp = fopen (optarg, "r")) == NULL)
	    {
		fprintf (stderr, "Encode: can't confusion matrix file %s\n",
			 optarg);
		exit (1);
	    }
	    loadConfusions (cfp);
	    /*dumpConfusions (stdout);*/
            break;
	case 'p':
	    debugProgress = atoi (optarg);
	    break;
	case 'l':
	    debugModel = atoi (optarg);
	    break;
	case 'd':
	    debugPaths = atoi (optarg);
	    break;
	case 'm':
	    fprintf (stderr, "Loading training model from file %s\n",
		    optarg);
	    if ((mfp = fopen (optarg, "r")) == NULL)
	    {
		fprintf (stderr, "Encode: can't open training model file %s\n",
			 optarg);
		exit (1);
	    }
	    Model = SMI_load_model (mfp);
	    break;
	default:
	    usage ();
	    break;
	}
    for (; optind < argc; optind++)
	usage ();
}

void
correctText (struct textType *text)
/* Corrects the input using the PPM models. */
{
    float codelen;
    unsigned int pos;

    initPaths ();

    pos = 0;
    while (pos < text->Text_length)
    {
	if ((debugProgress) && ((pos % debugProgress) == 0))
	{
	    fprintf (stderr, "%4d... ", pos);
	    printMalloc (stderr);
	}

	updatePaths (text, pos);

	if (debugPaths > 0)
	{
	    fprintf (stderr, "Best path so far:\n");
	    dumpBestPath (stderr);
	    fprintf (stderr, "\n");
	}

	if (debugPaths > 2)
	    dumpPaths (stderr);
	if (debugPaths > 3)
	{
	    dumpHashp (stderr);
	    dumpLeaves (stderr);
	}
	/* fprintf (stderr, "Pos %d Char %c Number of hashc entries = %d\n",
	   pos, Input [pos], countHashp ());*/
	pos++;
    }
    if (debugProgress)
    {
	fprintf (stderr, "%4d... ", pos);
	printMalloc (stderr);
    }
    codelen = dumpBestPath (stdout);
    /*fprintf (stderr, "Min code length = %.3f\n", codelen);*/
}

int
main (int argc, char *argv[])
{
    struct textType input;      /* The input text to correct */

    init_arguments (argc, argv);

    loadText (stdin, &input);

    correctText (&input);

    SMI_release_model (Model);
    return 0;
}
