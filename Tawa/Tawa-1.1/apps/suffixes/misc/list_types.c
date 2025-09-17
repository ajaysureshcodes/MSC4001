/* Program to list all the types for a single suffix model.
   Assumes that the suffix trie can fit in memory. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>

#include "io.h"
#include "SM_model.h"
#include "model.h"

#define FILE_SIZE 256             /* Max. length of a filename string */

char Model_filename [FILE_SIZE];  /* Name of file containing model */
unsigned int Model_file;          /* File that contains the suffix model */

char Output_filename [FILE_SIZE]; /* Name of output for writing output to */
unsigned int Output_file;         /* Output file pointer */
boolean Output_file_found = FALSE;/* TRUE if we have an output file to write to. */

boolean Calculate_rankfreqs = FALSE;  /* TRUE if rank frequency data is to be generated */
boolean List_matching_types = FALSE;  /* TRUE if matching types are to be listed */

unsigned int Type_length = 0;         /* Only list types that are at least this length */

unsigned int *SM_matching_suffixes = NULL;    /* List of matching suffixes */
unsigned int SM_matching_suffixes_length = 0; /* The m=number of matching suffixes */
unsigned int *SM_matching_counts = NULL;      /* List of matching suffixes counts */
unsigned int *SM_rankfreqs = NULL;            /* Used for calculating rank frequencies */

void junk ()
/* For debugging purposes. */
{
    fprintf (stderr, "Got here\n");
}

void
usage (void)
{
    fprintf (stderr,
	     "Usage: list_types [options] <infile >outfile\n"
	     "\n"
	     "options:\n"
	     "  -L\tList matching types\n"
	     "  -Z\tCalculate rank frequencies of types\n"
	     "  -d n\tDebug level=n\n"
	     "  -m fn\tModel filename=fn (required argument)\n"
	     "  -o fn\tOutput filename=fn (required argument)\n"
	     "  -1 n\tType length=n\n"
	     );
    exit (2);
}

void
init_arguments (int argc, char *argv[])
{
    extern char *optarg;
    extern int optind;
    int opt;
    boolean model_file_found;

    /* set defaults */
    model_file_found = FALSE;
    Output_file_found = FALSE;

    /* get the argument options */
    while ((opt = getopt (argc, argv, "LZd:m:o:1:")) != -1)
	switch (opt)
	{
	case 'L':
            List_matching_types = TRUE;
	    break;
	case 'Z':
            Calculate_rankfreqs = TRUE;
	    break;
	case 'd':
            Debug_level = atol (optarg);
	    break;
	case 'o':
	    Output_file_found = TRUE;
	    strcpy (Output_filename, optarg);
	    Output_file = TXT_open_file
	      (Output_filename, "w", "List types output file",
	       "List_types: can't open output file" );
	    if (Debug_level > 1)
	        fprintf (stderr, "Writing to output file %s\n",
			 optarg);
	    break;
	case 'm':
	    model_file_found = TRUE;
	    strcpy (Model_filename, optarg);	
	    Model_file = TXT_open_file
	      (Model_filename, "r", "List types model1 file",
	       "List_types: can't open model file" );
	    if (Debug_level > 1)
	        fprintf (stderr, "Loading Model from file %s\n",
			 optarg);
	    break;
	case '1':
            Type_length = atol (optarg);
	    break;
	default:
	    usage ();
	    break;
	}

    if (!Calculate_rankfreqs && !List_matching_types)
        List_matching_types = TRUE; /* Make sure at least some output is generated regardless */

    if (!model_file_found)
      {
	fprintf (stderr, "Missing model filename\n");
        usage ();
      }
    else if (!Output_file_found)
      {
	fprintf (stderr, "Missing output filename\n");
        usage ();
      }
    else
      {
	for (; optind < argc; optind++)
	  usage ();
      }
}

void
SM_update_matching_suffixes ()
/* Updates the counts and strings for the matching suffix. */
{
    unsigned int p, *str;

    SM_matching_counts [SM_matching_suffixes_length] = SM_traverse_suffix_count;

    str = (unsigned int *) SM_matching_suffixes [SM_matching_suffixes_length];
    if (str != NULL)
        free (str);
    str = calloc (Type_length, sizeof (unsigned int));
    SM_matching_suffixes [SM_matching_suffixes_length] = (unsigned int) str;

    for (p = 0; p < Type_length; p++)
        str [p] = SM_traverse_suffix [p]; /* Copy the suffix string to array of matching
					     suffixes */

    SM_matching_suffixes_length++;
}

void
SM_dump_matching_suffixes (unsigned int file)
{
    unsigned int p;

    for (p = 0; p < SM_matching_suffixes_length; p++)
      {
	fprintf (Files [file], "%5d [", SM_matching_counts [p]);
	SM_dump_nstring1 (file, (unsigned int *) SM_matching_suffixes [p], 0, Type_length);
	fprintf (Files [file], "]\n");
      }
}

boolean
SM_match_suffix (unsigned int suffix_length, unsigned int *suffix)
/* Function used by SM_traverse_trie to only match the suffix of a certain length. */
{
    boolean match;

    if (suffix_length < Type_length)
        match = FALSE;
    else if (SM_traverse_same_prefix (Type_length))
        match = FALSE;
    else
        match = TRUE;

    if (match)
      {
	if (Debug_level > 1)
	  {
	    fprintf (stderr, "suffix matches [");
	    SM_dump_nstring1 (Stderr_File, SM_traverse_suffix, 0, Type_length);
	    fprintf (stderr, "] count = %d\n", SM_traverse_suffix_count);
	  }

	SM_update_matching_suffixes ();
      }

    return (match);
}

void
SM_init_rankfreqs ()
/* Initialises the rank frequencies array which will be sorted by quick sort. Initally, the
   array is just set to increasing indexes into the array SM_matching_suffixes i.e. unsorted. */
{
    unsigned int p;

    for (p = 0; p < SM_matching_suffixes_length; p++)
      SM_rankfreqs [p] = p; /* Indexes are initially in increasing order */
}

int
SM_compare_rankfreqs (const void *index1, const void *index2)
/* Comparison function (to compare suffixes counts) used by qsort. */
{
  int p1, p2;

  /*fprintf (stderr, "Comparing %d with %d\n", *((unsigned int *) index1), *((unsigned int *) index2));*/
  p1 = SM_matching_counts [*((unsigned int *) index1)];
  p2 = SM_matching_counts [*((unsigned int *) index2)];
  /*fprintf (stderr, "Value     %d with %d\n", p1, p2);*/
  if (p1 > p2)
      return (-1);
  else if (p1 < p2)
      return (1);
  else
      return 0;
}

void
SM_calculate_rankfreqs ()
/* Calculates the rank frequencies of the matching suffixes. */
{
    qsort (SM_rankfreqs, SM_matching_suffixes_length, sizeof (unsigned int), SM_compare_rankfreqs);
}

void
SM_dump_rankfreqs (unsigned int file)
{
    unsigned int p, index;

    for (p = 0; p < SM_matching_suffixes_length; p++)
      {
	index = SM_rankfreqs [p];
	fprintf (Files [file], "%5d %5d [", p+1, SM_matching_counts [index]);
	SM_dump_nstring1 (file, (unsigned int *) SM_matching_suffixes [index], 0, Type_length);
	fprintf (Files [file], "]\n");
      }
}

int
main (int argc, char *argv[])
{
    struct SM_model_type *model;
    struct SM_trie_type *trie;

    init_arguments (argc, argv);

    model = SM_load_model (Model_file);

    trie = SM_grow_trie (model);
    SM_check_trie (model, trie);

    if (Debug_level > 3)
      {
	SM_dump_model (Stderr_File, model, trie, TRUE, TRUE, "1");
      }

    SM_matching_counts = (unsigned int *)
        calloc (model->size+1, sizeof (unsigned int));
    SM_matching_suffixes = (unsigned int *)
        calloc (model->size+1, sizeof (unsigned int));
    SM_rankfreqs = (unsigned int *)
        calloc (model->size+1, sizeof (unsigned int));

    SM_traverse_trie (model, trie, SM_match_suffix);

    if (List_matching_types)
        SM_dump_matching_suffixes (Output_file);

    if (Calculate_rankfreqs)
      {
	SM_init_rankfreqs ();
	SM_calculate_rankfreqs ();
	SM_dump_rankfreqs (Output_file);
      }

    exit (0);
}
