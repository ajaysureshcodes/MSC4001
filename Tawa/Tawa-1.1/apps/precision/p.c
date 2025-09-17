/* Works out the recall/precision for the model on some testfiles. */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>

#if defined (SYSTEM_LINUX)
#include <getopt.h> /* for getopt on Linux systems */
#endif

#include "io.h"
#include "track.h"
#include "judgments.h"
#include "text.h"
#include "table.h"
#include "model.h"

#define INCR_THRESHOLD 0.01    /* Increment for threshold */
#define MAX_THRESHOLD 0.5      /* Maximum value for threshold */
#define MIN_THRESHOLD -0.5     /* Minimum value for threshold */

#define MAX_ORDER 5       /* Maximum order of the model */
#define MAX_ALPHABET 256  /* Default max. alphabet size for all PPM models */
#define MAX_FILENAME 256  /* Maximum length of a filename string */
#define MAX_TOPIC 256     /* Maximum size of a topic name to identify a model */

#define MAX_TESTFILES 20000 /* Maximum number of testfiles */

unsigned int debugLevel = 0;

unsigned int Testfiles [MAX_TESTFILES];
unsigned int Testfile_keys [MAX_TESTFILES];
char Testfile_names [MAX_TESTFILES][MAX_FILENAME];
unsigned int Test_positive_tracks [MAX_TESTFILES];
unsigned int Test_negative_tracks [MAX_TESTFILES];
unsigned int Testfiles_count = 0;

unsigned int Positive_model;
unsigned int Negative_model;
unsigned int Judgments_table;
boolean DumpCharacters = FALSE;

char Topic [MAX_TOPIC];

void
loadTestFiles (char *filename)
/* Loads the test files from the list of filenames in the file filename. */
{
    char testfile_no [MAX_FILENAME];
    char testfile_name [MAX_FILENAME];
    unsigned int testfile, testfile_key, testfile_count;
    FILE *fp;

    fprintf (stderr, "Loading data files from file %s\n", filename);
    if ((fp = fopen (filename, "r")) == NULL)
      {
	fprintf (stderr, "Precision: can't open file %s\n", filename);
	exit (1);
      }

    testfile_count = 0;
    while ((fscanf (fp, "%s %s", testfile_no, testfile_name) != EOF))
    {
      testfile = TXT_open_file (testfile_name, "r", NULL, "Precision: can't open file");
      Testfiles [testfile_count] = TXT_load_file (testfile);
      testfile_key = TXT_create_text ();
      Testfile_keys [testfile_count] = testfile_key;
      TXT_append_string (testfile_key, testfile_no);
      strcpy (Testfile_names [testfile_count], testfile_name);
      TXT_close_file (testfile);

      testfile_count++;
    }
    fclose (fp);

    Testfiles_count = testfile_count;
}

void
usage (void)
{
    fprintf (stderr,
	     "Usage: codelength [options] <input-text\n"
	     "\n"
	     "options:\n"
	     "  -c\tdump codelength above threshold for each character\n"
	     "  -d n\tdebug level=n\n"
	     "  -t fn\tlist of test data files=fn (required)\n"
	     "  -j fn\tlist of test judgments=fn (required)\n"
	     "  -p fn\tpositive model filename=fn (required)\n"
	     "  -n fn\tnegative model filename=fn (required)\n"
	     "  -T s\tmodel topic name=s (required)\n"
	     );

    exit (2);
}

void
init_arguments (int argc, char *argv[])
{
    extern char *optarg;
    extern int optind;
    boolean positive_model_found;
    boolean negative_model_found;
    boolean topic_found;
    boolean judgments_found;
    boolean testfiles_found;
    unsigned int file;
    int opt;

    /* get the argument options */

    positive_model_found = FALSE;
    negative_model_found = FALSE;
    topic_found = FALSE;
    judgments_found = FALSE;
    testfiles_found = FALSE;
    while ((opt = getopt (argc, argv, "cd:t:p:n:J:T:")) != -1)
	switch (opt)
	{
	case 'c':
	    DumpCharacters = TRUE;
	    break;
	case 'd':
	    debugLevel = atoi (optarg);
	    break;
	case 'J':
	    file = TXT_open_file (optarg, "r", NULL,
				  "Test: can't open judgments table file");
	    Judgments_table = TXT_load_table (file);
	    if (debugLevel > 5)
	        TXT_dump_table (Stderr_File, Judgments_table);
	    judgments_found = TRUE;
	    break;
	case 't':
	    loadTestFiles (optarg);
	    testfiles_found = TRUE;
	    break;
	case 'p':
	    Positive_model = TLM_read_model
	      (optarg, NULL /*"Loading model from file"*/,
	       "Codelength: can't open positive model file");
	    positive_model_found = TRUE;
	    break;
	case 'n':
	    Negative_model = TLM_read_model
	      (optarg, NULL /*"Loading model from file"*/,
	       "Codelength: can't open negative model file");
	    negative_model_found = TRUE;
	    break;
	case 'T':
	    strcpy (Topic, optarg);
	    topic_found = TRUE;
	    break;
	default:
	    usage ();
	    break;
	}
    if (!positive_model_found || !negative_model_found)
    {
        fprintf (stderr, "\nFatal error: missing models\n\n");
        usage ();
    }
    if (!judgments_found)
    {
        fprintf (stderr, "\nFatal error: missing list of testfiles\n\n");
        usage ();
    }
    if (!testfiles_found)
    {
        fprintf (stderr, "\nFatal error: missing list of testfiles\n\n");
        usage ();
    }
    if (!topic_found)
    {
        fprintf (stderr, "\nFatal error: missing model topic name\n\n");
        usage ();
    }
    for (; optind < argc; optind++)
	usage ();
}

float
codelengthTestfile (unsigned int testfile)
/* Returns the codelength (in bits) for encoding the text using the ppm model. */
{
    unsigned int p, symbol, textlen;
    float positive_codelength, negative_codelength;
    float total_codelength, diff_codelength;

    textlen = TXT_length_text (Testfiles [testfile]);

    /* Now get codelength for encode each symbol */
    total_codelength = 0;
    for (p=0; p < textlen; p++) /* encode each symbol */
    {
      TLM_get_track (Test_positive_tracks [testfile], p, &positive_codelength);
      TLM_get_track (Test_negative_tracks [testfile], p, &negative_codelength);

      diff_codelength = negative_codelength - positive_codelength;
      total_codelength += diff_codelength;

      if (DumpCharacters)
	{
	  assert (TXT_get_symbol (Testfiles [testfile], p, &symbol));
	  fprintf (stderr, "%4d: (%c) = %7.3f (%.3f - %.3f)\n",
		   p, symbol, diff_codelength, negative_codelength,
		   positive_codelength);
	}
    }

    if (debugLevel > 4)
      fprintf (stderr,
	       "Model = %-8s Total = %8.3f Average = %6.3f\n",
	       Topic, total_codelength, (total_codelength / textlen));

    if (debugLevel > 2)
        fprintf (stderr, "codelength = %.3f textlen = %d\n",
		 total_codelength, textlen);
    return (total_codelength/textlen);
}

void
breakevenTestFiles (void)
/* Calculates the breakeven recall/precision point for all testfiles. */
{
    unsigned int topic, topic_id, topic_count, testfile;
    unsigned int retrieved_count, correct_count;
    boolean retrieved, valid_topic;
    float threshold, codelength;
    float recall, precision, average;
    float best_threshold, best_average;

    best_average = 0.0;
    best_threshold = 0.0;

    topic = TXT_create_text ();
    TXT_append_string (topic, Topic);

    valid_topic = TXT_getid_table (Judgments_table, topic, &topic_id, &topic_count);
    assert (valid_topic);

    /* Make track records for compressing each test file */
    for (testfile = 0; testfile < Testfiles_count; testfile++)
      {
        Test_positive_tracks [testfile] =
	  TLM_make_track (TLM_standard_tracking, Positive_model, Testfiles [testfile]);
        Test_negative_tracks [testfile] =
	  TLM_make_track (TLM_standard_tracking, Negative_model, Testfiles [testfile]);
      }
    fprintf (stderr, "loaded all test files...\n\n");

    for (threshold = MIN_THRESHOLD; threshold <= MAX_THRESHOLD;
	 threshold += INCR_THRESHOLD)
     {
	correct_count = 0;
	retrieved_count = 0;

	for (testfile = 0; testfile < Testfiles_count; testfile++)
	  {
	    if (debugLevel > 3)
	        fprintf (stderr, "Test file: %s\n", Testfile_names [testfile]);

	    codelength = codelengthTestfile (testfile);
	    if (debugLevel > 2)
	      {
		fprintf (stderr, "File ");
		TXT_dump_text (Stderr_File, Testfile_keys [testfile], NULL);
		fprintf (stderr, " codelength %6.3f threshold %.3f\n",
			 codelength, threshold);
	      }

	    retrieved = (codelength >= threshold);
	    if (debugLevel > 2)
	      fprintf (stderr, "Retrieved: %d\n", retrieved);

	    if (retrieved)
	      {
		retrieved_count++;
		if (TXT_find_judgment (Judgments_table,
				       Testfile_keys [testfile], topic))
		    correct_count++;
	      }
	  }

	if (debugLevel > 1)
	  fprintf (stderr, "threshold = %.2f correct = %d retrieved = %d total = %d\n",
		   threshold, correct_count, retrieved_count, topic_count);

	recall  = (float) correct_count / topic_count;
	precision = (float) correct_count / retrieved_count;
	average = (recall + precision)/2;

	if (debugLevel > 1)
	  fprintf (stderr, "recall = %.3f precision = %.3f average = %.3f\n\n",
		   recall, precision, average);

	if (average > best_average)
	  {
	    best_average = average;
	    best_threshold = threshold;
	  }
     }

    TXT_release_text (topic);

    fprintf (stderr, "best average = %.3f threshold = %.3f\n",
	     best_average, best_threshold);
}

int
main (int argc, char *argv[])
{
    init_arguments (argc, argv);

    if (TLM_numberof_models () < 1){
      usage();
    }

    breakevenTestFiles ();

    TLM_release_models ();

    exit (0);
}
