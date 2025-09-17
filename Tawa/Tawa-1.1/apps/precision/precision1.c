/* Works out the recall/precision for the model on some testfiles.
   Uses specified thresholds. */
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

#define TESTS 3                /* Number of tests to apply */
#define TEST_CODELENGTH 0      /* Test uses just codelength as a measure */
#define TEST_AVG_CODELENGTH 1  /* Test uses just (codelength/textlength) */
#define TEST_AVG_CODEDLENGTH 2 /* Test uses just (codelength/codedlength) */
#define TEST_CODELENGTH_SCALE 100 /* For scaling codelength tests to same
				     scale as the other tests */

#define MAX_ORDER 5       /* Maximum order of the model */
#define MAX_ALPHABET 256  /* Default max. alphabet size for all PPM models */
#define MAX_FILENAME 256  /* Maximum length of a filename string */
#define MAX_TOPIC 256     /* Maximum size of a topic name to identify a model */

#define MAX_TESTFILES 20000 /* Maximum number of testfiles */

unsigned int debugLevel = 0;

float Threshold = 0;
float Threshold1 = 0;

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
	     "  -0 n\tthreshold=n\n"
	     "  -1 n\tthreshold1=n\n"
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
    boolean threshold_found;
    boolean threshold1_found;
    boolean judgments_found;
    boolean testfiles_found;
    unsigned int file;
    int opt;

    /* get the argument options */

    positive_model_found = FALSE;
    negative_model_found = FALSE;
    threshold_found = FALSE;
    threshold1_found = FALSE;
    topic_found = FALSE;
    judgments_found = FALSE;
    testfiles_found = FALSE;
    while ((opt = getopt (argc, argv, "0:1:cd:t:p:n:J:T:")) != -1)
	switch (opt)
	{
	case '0':
	    sscanf (optarg, "%f", &Threshold);
	    threshold_found = TRUE;
	    break;
	case '1':
	    sscanf (optarg, "%f", &Threshold1);
	    threshold1_found = TRUE;
	    break;
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
	    if (debugLevel > 2)
	        TXT_dump_table (Stderr_File, Judgments_table);
	    judgments_found = TRUE;
	    break;
	case 't':
	    loadTestFiles (optarg);
	    testfiles_found = TRUE;
	    break;
	case 'p':
	    Positive_model = TLM_read_model
	      (optarg, "Loading model from file",
	       "Codelength: can't open positive model file");
	    positive_model_found = TRUE;
	    break;
	case 'n':
	    Negative_model = TLM_read_model
	      (optarg, "Loading model from file",
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
    if (!threshold_found || !threshold1_found)
    {
        fprintf (stderr, "\nFatal error: missing threshold value\n\n");
        usage ();
    }
    fprintf (stderr, "Threshold = %.3f Threshold1 = %.3f\n",
	     Threshold, Threshold1);
    if (!topic_found)
    {
        fprintf (stderr, "\nFatal error: missing model topic name\n\n");
        usage ();
    }
    for (; optind < argc; optind++)
	usage ();
}

float
codelengthTestfile (unsigned int testfile, float threshold,
		    unsigned int *textlength, unsigned int *codedlength)
/* Returns the codelength (in bits) for encoding the text using the ppm model. */
{
    unsigned int p, symbol, textlen, codedlen;
    float positive_codelength, negative_codelength;
    float total_codelength, diff_codelength;

    textlen = TXT_length_text (Testfiles [testfile]);

    /* Now get codelength for encode each symbol */
    total_codelength = 0;
    codedlen = 0;
    for (p=0; p < textlen; p++) /* encode each symbol */
    {
      TLM_get_track (Test_positive_tracks [testfile], p, &positive_codelength);
      TLM_get_track (Test_negative_tracks [testfile], p, &negative_codelength);

      diff_codelength = negative_codelength - positive_codelength;
      if (diff_codelength >= threshold)
	{
	  codedlen++;
	  total_codelength += diff_codelength;
	}

      if (DumpCharacters)
	{
	  assert (TXT_get_symbol (Testfiles [testfile], p, &symbol));
	  fprintf (stderr, "%4d: (%c) = %7.3f (%.3f - %.3f)",
		   p, symbol, diff_codelength, negative_codelength,
		   positive_codelength);
	  if (diff_codelength < threshold)
	      fprintf (stderr, " *");
	  fprintf (stderr, "\n");
	}
    }

    if (debugLevel > 4)
      fprintf (stderr,
       "Model = %-8s Threshold = %4.1f Total = %8.3f Average = %6.3f Average1 = %6.3f\n",
       Topic, threshold, total_codelength, (total_codelength / textlen),
       (total_codelength / codedlen));

    *textlength = textlen;
    *codedlength = codedlen;

    return (total_codelength);
}

void
breakevenTestFiles (void)
/* Calculates the breakeven recall/precision point for all testfiles. */
{
    unsigned int topic, topic_id, topic_count, testfile, t;
    unsigned int retrieved_count [TESTS], correct_count [TESTS];
    unsigned int textlength, codedlength;
    boolean retrieved [TESTS], valid_topic, found_judgment;
    float codelength, recall [TESTS], precision [TESTS], average [TESTS];

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

    for (t = 0; t < TESTS; t++)
      {
	correct_count [t] = 0;
	retrieved_count [t] = 0;
      }

    for (testfile = 0; testfile < Testfiles_count; testfile++)
      {
	if (debugLevel > 3)
	  fprintf (stderr, "Test file: %s\n", Testfile_names [testfile]);
	codelength = codelengthTestfile
	    (testfile, Threshold1, &textlength, &codedlength);
	if (debugLevel > 2)
	  {
	    fprintf (stderr, "File ");
	    TXT_dump_text (Stderr_File, Testfile_keys [testfile], NULL);
	    fprintf (stderr, " codelength %6.3f Threshold %.3f Threshold1 %.3f",
		     codelength, Threshold, Threshold1);
	  }

	retrieved [TEST_CODELENGTH] =
	  ((codelength/TEST_CODELENGTH_SCALE) >= Threshold);
	retrieved [TEST_AVG_CODELENGTH] =
	  ((codelength/textlength) >= Threshold);
	retrieved [TEST_AVG_CODEDLENGTH] =
	  ((codedlength > 0) && ((codelength/codedlength) >= Threshold));

	found_judgment = FALSE;
	for (t = 0; t < TESTS; t++)
	  if (retrieved [t])
	    {
	      retrieved_count [t]++;
	      if (found_judgment || TXT_find_judgment
		  (Judgments_table, Testfile_keys [testfile], topic))
		{
		  found_judgment = TRUE;
		  correct_count [t]++;
		}
	    }
      }
    if (debugLevel > 1)
      for (t = 0; t < TESTS; t++)
	fprintf (stderr, "Threshold = %.2f Threshold1 = %.2f correct = %d retrieved = %d total = %d\n",
		 Threshold, Threshold1, correct_count [t], retrieved_count [t],
		 topic_count);

    for (t = 0; t < TESTS; t++)
      {
	recall [t]  = (float) correct_count [t] / topic_count;
	precision [t] = (float) correct_count [t] / retrieved_count [t];
	average [t] = (recall [t] + precision [t])/2;
      }

    for (t = 0; t < TESTS; t++)
      fprintf (stderr, "test: %d recall = %.3f precision = %.3f average = %.3f\n\n",
	       t, recall [t], precision [t], average [t]);

    TXT_release_text (topic);
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
