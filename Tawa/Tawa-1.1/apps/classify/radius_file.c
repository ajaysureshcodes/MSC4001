/* Works out the information radius for a file based
   on the average distribution of two models.
   Input consists of two aruments - the first is a
   filename containing successive lines that describe
   each model pair in the format "tag filename1 filename2";
   the second is a list of test filenames, one per line. */
#include <stdio.h>
#include <stdlib.h>

#ifdef SYSTEM_LINUX
#include <getopt.h> /* for getopt on Linux systems */
#endif

#include "io.h"
#include "text.h"
#include "model.h"

#define MAX_ALPHABET 256  /* Default maximum alphabet size for all PPM models */
#define MAX_ORDER 5       /* Maximum order of the model */
#define MAX_TAG 64        /* Maximum length of a tag string */
#define MAX_FILENAME 256  /* Maximum length of a filename */
#define MAX_MODELS 10000  /* Maximum number of models */

unsigned int Topic_models1 [MAX_MODELS];
unsigned int Topic_models2 [MAX_MODELS];
unsigned int Topic_models_count = 0;

FILE *Testfiles_fp;

void
usage (void)
{
    fprintf (stderr,
	     "Usage: radius_file [options] -m model-pairs-filename -t <test-list-of-filenames\n"
	     "\n"
	     );
    exit (2);
}

void
loadModels (FILE *fp)
/* Load the models from file FP. */
{
    char tag [MAX_TAG], model1_filename [MAX_FILENAME],
        model2_filename [MAX_FILENAME];
    unsigned int model1, model2;

    while ((fscanf (fp, "%s %s %s", tag, model1_filename,
		    model2_filename) != EOF))
    {
        model1 = TLM_read_model (model1_filename,
				 "Loading training model from file",
				 "Radius_file: can't open training file");
        model2 = TLM_read_model (model2_filename,
				"Loading training model from file",
				"Radius_file: can't open training file");

	TLM_set_tag (model1, tag);
	TLM_set_tag (model2, tag);
	Topic_models1 [Topic_models_count] = model1;
	Topic_models2 [Topic_models_count] = model2;
	Topic_models_count++;
    }
}

void
init_arguments (int argc, char *argv[])
{
    extern char *optarg;
    extern int optind;
    unsigned int models_found;
    unsigned int testfiles_found;
    FILE *fp;
    int opt;

    /* get the argument options */
    models_found = 0;
    testfiles_found = 0;
    while ((opt = getopt (argc, argv, "m:t:")) != -1)
	switch (opt)
	{
	case 'm':
	    fprintf (stderr, "Loading models from file %s\n",
		    optarg);
	    if ((fp = fopen (optarg, "r")) == NULL)
	    {
		fprintf (stderr, "Encode: can't open models file %s\n",
			 optarg);
		exit (1);
	    }
	    loadModels (fp);
	    models_found = 1;
	    break;
	case 't':
	    fprintf (stderr, "Loading test files from file %s\n",
		    optarg);
	    if ((Testfiles_fp = fopen (optarg, "r")) == NULL)
	    {
		fprintf (stderr, "Encode: can't open testfiles file %s\n",
			 optarg);
		exit (1);
	    }
	    testfiles_found = 1;
	    break;
	default:
	    usage ();
	    break;
	}
    if (!models_found || !testfiles_found)
    {
        fprintf (stderr, "\nFatal error: missing models or test files\n\n");
        usage ();
    }
    for (; optind < argc; optind++)
      usage ();
}

int
getline( FILE *fp, char *s, int max )
/* Read line from FP into S; return its length (maximum length = MAX). */
{
    int i;
    int cc;

    i = 0;
    cc = '\0';
    while ((--max > 0) && ((cc = getc(fp)) != EOF) && (cc != '\n'))
        s [i++] = cc;
    s [i] = 0;
    if (cc == EOF)
        return (EOF);
    else
        return( i );
}

float
radiusText (unsigned int model1, unsigned int model2, unsigned int text)
/* Returns the information radius for encoding the text between the
   two PPM models. */
{
    unsigned int context1, context2, pos, symbol;
    float codelength, radius;

    TLM_set_context_type (TLM_Get_Codelength);

    codelength = 0.0;
    context1 = TLM_create_context (model1);
    context2 = TLM_create_context (model2);

    /* Now compute radius for each symbol */
    pos = 0;
    while (TXT_get_symbol (text, pos++, &symbol))
    {
	TLM_update_context (model1, context1, symbol);
	codelength += TLM_Codelength;
	TLM_update_context (model2, context2, symbol);
	codelength += TLM_Codelength;
    }
    TLM_release_context (model1, context1);
    TLM_release_context (model2, context2);

    radius = codelength / 2.0;
    return (radius);
}

void
classifyTestfiles (void)
/* Classify the test files. */
{
    char test_filename [256];
    unsigned int file, test_text, m, small;
    float radius, smallest;
    int len;

    while ((len = getline (Testfiles_fp, test_filename, 256)) != EOF)
    {
        file = TXT_open_file (test_filename, "r", NULL,
			      "Radius_file: can't open testing file");
	test_text = TXT_load_text (file);
	TXT_close_file (file);

	smallest = 0.0;
	small = 0;
	fprintf (stderr, "\nClassifying test file %s:\n", test_filename);
	for (m = 0; m < Topic_models_count; m++)
	  {
	    radius = radiusText (Topic_models1 [m], Topic_models2 [m],
				 test_text);
	    fprintf (stderr, "%s: %.3f\n", TLM_get_tag (Topic_models1 [m]),
		     radius);
	    if (!m)
	        smallest = radius;
	    else if (radius < smallest)
	      {
		small = m;
		smallest = radius;
	      }
	  }
	fprintf (stderr, "Smallest radius for %s = %.3f\n",
		 TLM_get_tag (Topic_models1 [small]), smallest);
    }
}

int
main (int argc, char *argv[])
{
    init_arguments (argc, argv);

    /*dumpModels (stdout);*/

    classifyTestfiles ();

    return 0;
}
