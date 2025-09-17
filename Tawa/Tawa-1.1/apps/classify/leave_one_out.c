/* Program for text categorization using leave one out strategy. */
#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <assert.h>
#include <strings.h>

#ifdef SYSTEM_LINUX
#include <getopt.h> /* for getopt on Linux systems */
#endif

#include "io.h"
#include "model.h"
#include "text.h"
#include "table.h"

#define MAX_TEXTS 512     /* Maximum number of input test files to process */
#define MAX_FILENAME 256  /* Maximum size of filename */
#define MAX_ALPHABET 256  /* Default maximum alphabet size for all PPM models */
#define MAX_ORDER 5       /* Maximum order of the model */
#define BREAK_SYMBOL 10   /* Symbol used for forcing a break (usually an eoln) */

FILE *Test_fp;

unsigned int Txts [MAX_TEXTS];
unsigned int Txts_max = 0;
unsigned int Txts_filenames [MAX_TEXTS];

#include "io.h"
#include "text.h"
#include "model.h"

boolean Entropy = FALSE;
boolean BothEntropy = FALSE;
boolean DynamicModelRequired = FALSE;
unsigned int DynamicAlphabetSize = 256;
unsigned int DynamicMaxOrder = 5;
unsigned int DynamicEscapeMethod = TLM_PPM_Method_D;
unsigned int MinLength = 0;
unsigned int Debug_progress = 0;

char *Model_title = NULL;
unsigned int Model_alphabet_size = 256;
unsigned int Model_max_order = MAX_ORDER;
unsigned int Model_escape_method = TLM_PPM_Method_D;
boolean Model_performs_full_excls = TRUE;
boolean Model_performs_update_excls = TRUE;
unsigned int Max_input_size = 0;
boolean Break_Eoln = FALSE;

void
usage (void)
{
    fprintf (stderr,
	     "Usage: ident_file [options] <input-text\n"
	     "\n"
	     "options:\n"
	     "  -B\tcalculate both the cross-entropy and the codelength\n"
	     "  -C\tcalculate cross-entropy rather than codelength\n"
	     "  -D\tinclude dynamic model in list of models\n"
	     "  -E n\tescape method for dynamic model=c\n"
	     "  -L n\tminimum length of data file=n\n"
	     "  -O n\tmax order of dynamic model=n\n"
	     "  -T n\tdescription (title) of model (required argument)\n"
	     "  -a n\tsize of alphabet for dynamic model=n\n"
	     "  -e n\tescape method for model=c\n"
	     "  -m fn\tlist of models filename=fn (required)\n"
	     "  -o n\tmax order of leave one out model=n (required argument)\n"
	     "  -p n\tprogress report every n chars.\n"
	     "  -r\tdebug arithmetic coding ranges\n"
	     "  -t fn\tlist of test data files filename=fn (required)\n"
	);
    exit (2);
}

void
init_arguments (int argc, char *argv[])
{
    extern char *optarg;
    extern int optind;
    boolean models_found;
    boolean data_found;
    boolean title_found;
    boolean max_order_found;

    int opt, escape;

    /* get the argument options */

    models_found = FALSE;
    data_found = FALSE;
    title_found = FALSE;
    max_order_found = FALSE;
    while ((opt = getopt (argc, argv, "BCDE:L:O:T:a:e:m:o:p:rt:")) != -1)
	switch (opt)
	{
	case 'B':
	    BothEntropy = TRUE;
	    break;
	case 'C':
	    Entropy = TRUE;
	    break;
	case 'D':
	    DynamicModelRequired = TRUE;
	    break;
	case 'E' :
	    escape = optarg [0] - 'A';
	    assert (escape >= 0);
	    DynamicEscapeMethod = escape;
	    break;
	case 'L':
	    MinLength = atoi (optarg);
	    break;
	case 'O':
	    DynamicMaxOrder = atoi (optarg);
	    break;
	case 'T':
	    title_found = (strlen (optarg) > 0);
	    if (title_found)
	      {
		Model_title = (char *) malloc (strlen (optarg)+1);
		strcpy (Model_title, optarg);
	      }
	    break;
        case 'a':
	    DynamicAlphabetSize = atoi (optarg);
	    break;
	case 'e' :
	    escape = optarg [0] - 'A';
	    assert (escape >= 0);
	    Model_escape_method = escape;
	    break;
	case 'm':
	    TLM_load_models (optarg);
	    models_found = TRUE;
	    break;
	case 'o':
	    max_order_found = TRUE;
	    Model_max_order = atoi (optarg);
	    break;
	case 'p':
	    Debug_progress = atoi (optarg);
	    break;
	case 'r':
	    Debug.range = TRUE;
	    break;
	case 't':
	    fprintf (stderr, "Loading data files from file %s\n",
		     optarg);
	    if ((Test_fp = fopen (optarg, "r")) == NULL)
	    {
		fprintf (stderr, "Encode: can't open models file %s\n",
			 optarg);
		exit (1);
	    }
	    data_found = TRUE;
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
    if (!title_found)
    {
        fprintf (stderr, "\nFatal error: missing model title\n\n");
        usage ();
    }
    if (!max_order_found)
    {
        fprintf (stderr, "\nFatal error: missing max order of model\n\n");
        usage ();
    }
    for (; optind < argc; optind++)
	usage ();
}

float
codelengthText (unsigned int model, unsigned int text)
/* Returns the codelength for encoding the text using the PPM model. */
{
    unsigned int context, pos, symbol;
    float codelength;

    TLM_set_context_type (TLM_Get_Codelength);

    codelength = 0.0;
    context = TLM_create_context (model);

    /* Now encode each symbol */
    pos = 0;
    while (TXT_get_symbol (text, pos++, &symbol))
    {
	if (Debug.range != 0)
	  {
	    if (symbol == TXT_sentinel_symbol ())
	      fprintf (stderr, "Encoding sentinel symbol\n");
	    else
	      fprintf (stderr, "Encoding symbol %d (%c)\n", symbol, symbol);
	  }

	TLM_update_context (model, context, symbol);
	codelength += TLM_Codelength;
    }

    TLM_release_context (model, context);

    return (codelength);
}

void
classifyModels (unsigned int filename, unsigned int text, unsigned int textlen)
/* Returns the number of bits required to compress each model. */
{
    unsigned int DynamicModel = NIL;
    float codelength, avg_codelength, min_avg_codelength;
    int model, min_model, m;

    if (DynamicModelRequired)
        DynamicModel = TLM_create_model
	   (TLM_PPM_Model, "Dynamic", DynamicAlphabetSize, DynamicMaxOrder,
	    DynamicEscapeMethod, TRUE);

    TLM_reset_modelno ();
    m = 0;
    min_model = 0;
    min_avg_codelength = 999.9;
    while ((model = TLM_next_modelno ()))
    {
        codelength = codelengthText (model, text);
	avg_codelength = codelength/textlen;

	if (!min_model || (avg_codelength < min_avg_codelength))
	  {
	    min_avg_codelength = avg_codelength;
	    min_model = model;
	  }
	if (BothEntropy)
	  {
	    fprintf (stderr, "%9.3f %6.3f ", codelength, avg_codelength);
	    TXT_dump_text (Stderr_File, filename, NULL);
	    fprintf (stderr, " %s\n", TLM_get_tag (model));
	  }
        else
	  {
	    if (Entropy)
	        codelength = avg_codelength;
	    fprintf (stderr, "%6.3f ", codelength);
	    TXT_dump_text (Stderr_File, filename, NULL);
	    fprintf (stderr, " %s\n", TLM_get_tag (model));
	  }

	m++;
    }
    fprintf (stderr, "Minimum codelength %6.3f for model %s\n", min_avg_codelength,
	     TLM_get_tag (min_model));

    if (DynamicModelRequired)
        TLM_release_model (DynamicModel);
}

void
loadTexts (FILE *fp)
{
    char filename [MAX_FILENAME];
    unsigned int file, text, text1;

    Txts_max = 0;
    while ((fscanf (fp, "%s", filename) != EOF))
    {
        file = TXT_open_file (filename, "r", NULL,
			      "Leave_one_out: can't open data file");
	text = TXT_create_text ();
	TXT_append_string (text, filename);
	text1 = TXT_load_text (file);
	TXT_setlength_text (text1, TXT_length_text (text1)-1);

	Txts_filenames [Txts_max] = text;
	Txts [Txts_max] = text1;
	Txts_max++;
	TXT_close_file (file);
      }

    fprintf (stderr, "Number of files to process = %d\n", Txts_max);
}

void
trainModel (unsigned int model, unsigned int context, unsigned int text)
/* Trains the model from the characters in the text file. */
{
    unsigned int textlen, p, sym;

    /* Start off the training with a sentinel symbol to indicate a break */
    /*TLM_update_context (model, context, TXT_sentinel_symbol ());*/

    textlen = TXT_length_text (text);

    for (p = 0; p < textlen; p++)
    {
	if ((Debug_progress > 0) && ((p % Debug_progress) == 0))
	    fprintf (stderr, "text file %d training pos %d\n", text, p);
        /* repeat until EOF or max input */
	if (Max_input_size && (p >= Max_input_size))
	  break;
        if (!TXT_get_symbol (text, p, &sym))
	    break;

	if (Break_Eoln && (sym == BREAK_SYMBOL))
	    sym = TXT_sentinel_symbol ();

	TLM_update_context (model, context, sym);
    }
}

unsigned int
leaveOneOut_train (unsigned int this_one)
/* Train a new model on all the test data files except this_one. */
{
    unsigned int model, context, text;

    fprintf (stderr, "Building leave one out model\n");

    model = TLM_create_model (TLM_PPM_Model, Model_title, Model_alphabet_size, Model_max_order,
			      Model_escape_method, Model_performs_full_excls,
			      Model_performs_update_excls);
    TLM_set_tag (model, Model_title);

    context = TLM_create_context (model);
    for (text = 0; text < Txts_max; text++)
      if (text != this_one)
	{
	  /*
	  fprintf (stderr, "Training on ");
	  TXT_dump_text (Stderr_File, Txts_filenames [text], NULL);
	  fprintf (stderr, "\n");
	  */
	  trainModel (model, context, Txts [text]);
	}
    TLM_suspend_update (model); /* This will ensure model is static without the need to
				   write it out */

    TLM_release_context (model, context);

    fprintf (stderr, "Building of model complete\n");

    /*TLM_dump_model (Stderr_File, model, NULL);*/
    return (model);
}

void
leaveOneOut_classify (unsigned int this_one)
/* Classify the test data files using the current models but leaving
   out this_one. */
{
    unsigned int textlen;

    textlen = TXT_length_text (Txts [this_one]);
    if (!MinLength || (textlen >= MinLength))
        classifyModels (Txts_filenames [this_one], Txts [this_one], textlen);
}

void
classifyTexts ()
/* Classify the Texts using a leave one out scheme. */
{
    unsigned int text, model;

    loadTexts (Test_fp);
    for (text = 0; text < Txts_max; text++)
      {
	fprintf (stderr, "\nLeaving out text %d filename ", text);
	TXT_dump_text (Stderr_File, Txts_filenames [text], NULL);
	fprintf (stderr, "\n");

	/* Train a brand new model, leaving out the specific text file */
	model = leaveOneOut_train (text);

	/* Now classify using the new model plus other models specified in the aruments */
	leaveOneOut_classify (text);

	TLM_release_model (model);
      }
}

int
main (int argc, char *argv[])
{
    init_arguments (argc, argv);

    /*dumpModels (stdout);*/

    classifyTexts ();

    return 0;
}
