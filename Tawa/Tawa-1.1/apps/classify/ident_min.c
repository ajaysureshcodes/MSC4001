/* Works out which models compress parts of a file best. */
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
#define MAX_MODELS 128    /* Maximum number of models; used when storing
			     the Tags */

char Tags [MAX_MODELS][MAX_TAG];
unsigned int Tags_count = 0;
FILE *Data_fp;

void
usage (void)
{
    fprintf (stderr,
	     "Usage: ident_file [options] <input-text\n"
	     "\n"
	     "options:\n"
	     "  -m fn\tlist of models filename=fn (required)\n"
	     "  -d fn\tlist of data files filename=fn (required)\n"
	);
    exit (2);
}

unsigned int
loadModels (FILE * fp)
/* Load the models from file FP. */
{
    char tag [MAX_TAG], filename [128];
    unsigned int count;

    count = 0;
    while ((fscanf (fp, "%s %s", tag, filename) != EOF))
    {
        count++;
        strcpy (Tags [count], tag);
	TLM_read_model (filename, "Loading training model from file",
			"Ident_lang: can't open training file");
    }
    Tags_count = count;
    return (count);
}

char *
get_tag (unsigned int model)
/* Return the tag associated with the model. */
{
    if ((model > 0) && (model <= Tags_count))
        return (Tags [model]);
    else
        return (NULL);
}

void
init_arguments (int argc, char *argv[])
{
    extern char *optarg;
    extern int optind;
    FILE *fp;
    unsigned int models_found;
    unsigned int data_found;
    int opt;

    /* get the argument options */

    models_found = 0;
    data_found = 0;
    while ((opt = getopt (argc, argv, "d:m:")) != -1)
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
	case 'd':
	    fprintf (stderr, "Loading data files from file %s\n",
		     optarg);
	    if ((Data_fp = fopen (optarg, "r")) == NULL)
	    {
		fprintf (stderr, "Encode: can't open models file %s\n",
			 optarg);
		exit (1);
	    }
	    data_found = 1;
	    break;
	default:
	    usage ();
	    break;
	}
    if (!models_found)
    {
        fprintf (stderr, "\nFatal error: missing models filename\n\n");
        usage ();
    }
    if (!data_found)
    {
        fprintf (stderr, "\nFatal error: missing data filename\n\n");
        usage ();
    }
    for (; optind < argc; optind++)
	usage ();
}

void
identifyModels (unsigned int text, unsigned int textlen)
/* Returns the number of bits required to compress each model. */
{
    unsigned int model, numberof_models, m, minmodel;
    unsigned int symbol, prev_bestmodel, pos;
    unsigned int *models, *contexts, *min_counts;
    float codelen, mincodelen;

    TLM_set_context_type (TLM_Get_Codelength);

    TLM_reset_modelno ();

    numberof_models = TLM_numberof_models ();
    models = (unsigned int *)
        malloc ((numberof_models+1) * sizeof (unsigned int));
    contexts = (unsigned int *)
        malloc ((numberof_models+1) * sizeof (unsigned int));
    min_counts = (unsigned int *)
        malloc ((numberof_models+1) * sizeof (unsigned int));
    for (m=1; m <= numberof_models; m++)
        min_counts [m] = 0;

    prev_bestmodel = numberof_models + 1;

    /* Create a context for each model */
    m = 1;
    TLM_reset_modelno ();
    while ((model = TLM_next_modelno ()))
      {
        models [m++] = model;
        contexts [m++] = TLM_create_context (model);
      }

    /* Find which model compresses each symbol in the text best */
    pos = 0;
    while (TXT_get_symbol (text, pos++, &symbol))
    {
      mincodelen = 0;
      minmodel = 0;
      for (m=1; m <= numberof_models; m++)
      {
	TLM_update_context (models [m], contexts [m], symbol);
	codelen = TLM_Codelength;
        /*fprintf (stderr, "%-32s %9.3f\n", get_tag (m), codelen);*/
	if ((mincodelen == 0) || (codelen < mincodelen))
	  {
	    minmodel = m;
	    mincodelen = codelen;
	  }
      }
      min_counts [minmodel]++;
      /*
      TXT_dump_symbol (stdout, symbol);
      if ((prev_bestmodel > numberof_models) || (minmodel != prev_bestmodel))
	{
	  if (prev_bestmodel <= numberof_models)
	    fprintf (stdout, "</%s>", get_tag(prev_bestmodel));
	  prev_bestmodel = minmodel;
	  fprintf (stdout, "<%s>", get_tag(minmodel));
	}
      */
    }

    /* Dump out the min_counts */

    /*
    fprintf (stderr, "\n");
    for (m=1; m <= numberof_models; m++)
        fprintf (stderr, "%-32s %9.3f\n", get_tag (m),
		 100.0 * ((float) min_counts [m] / pos));
    */
    minmodel = 1;
    for (m=2; m <= numberof_models; m++)
      if (min_counts [m] > min_counts [minmodel])
	minmodel = m;
    fprintf (stderr, "%-32s %9.3f\n", get_tag (minmodel),
	     100.0 * ((float) min_counts [minmodel] / pos));

    /* Release each context */
    m = 1;
    TLM_reset_modelno ();
    for (m=1; m <= numberof_models; m++)
        TLM_release_context (models [m], contexts [m]);
}

void
identifyFiles (FILE * fp)
/* Load the test data files from file FP. */
{
    char filename [128];
    unsigned int file, text, textlen;

    while ((fscanf (fp, "%s", filename) != EOF))
    {
      /*dump_memory (stderr);*/

        file = TXT_open_file (filename, "r", NULL,
			      "Ident_min: can't open data file");
	text = TXT_load_text (file);
	textlen = TXT_length_text (text);
	identifyModels (text, textlen);
	TXT_release_text (text);
	TXT_close_file (file);
    }
}

int
main (int argc, char *argv[])
{
    init_arguments (argc, argv);

    /*dumpModels (stdout);*/

    identifyFiles (Data_fp);

    return 0;
}
