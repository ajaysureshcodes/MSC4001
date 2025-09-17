/* Program to generate counts for two suffix models.
   Assumes that the suffix tries can all fit in memory. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>

#include "io.h"
#include "model.h"
#include "SM_model.h"

#define DUMP_STRING_SIZE 100      /* Used for dumping long char strings (for debug purposes) */
#define DUMP_NSTRING_SIZE 40      /* Used for dumping long uint strings (for debug purposes) */

#define COUNTS_SIZE 10000         /* Used for storing counts */
#define FILE_SIZE 256             /* Max. length of a filename string */
#define CLASS_SIZE 256            /* Max. length of a class name */

/* Types of counts */
#define COUNT_R_MEASURE 1
#define COUNT_R1_MEASURE 2
#define COUNT_C_MEASURE 3
#define COUNT_C1_MEASURE 4

char Model1_filename [FILE_SIZE]; /* Name of file containing model 1 */
unsigned int Model1_file;         /* File that contains the suffix model 1 */

char Model2_filename [FILE_SIZE]; /* Name of file containing model 2 */
unsigned int Model2_file;         /* File that contains the suffix model 2 */

char Output_filename [FILE_SIZE]; /* Name of output file for writing output to */
unsigned int Output_file;         /* Output file pointer */
boolean Output_file_found = FALSE;/* TRUE if we have an output file to write to. */

char Class_name [CLASS_SIZE];     /* Name of class */
boolean Class_name_found = FALSE;

void junk ()
/* For debugging purposes. */
{
    fprintf (stderr, "Got here\n");
}

void
usage (void)
{
    fprintf (stderr,
	     "Usage: generate_counts [options] <infile >outfile\n"
	     "\n"
	     "options:\n"
	     "  -d n\tDebug level=n\n"
	     "  -p n\tDebug progress=n\n"
	     "  -C str\tClass name=str\n"
	     "  -o fn\tOutput filename=fn (required argument)\n"
	     "  -1 fn\tModel 1 filename=fn (required argument)\n"
	     "  -2 fn\tModel 2 filename=fn (required argument)\n"
	     );
    exit (2);
}

void
init_arguments (int argc, char *argv[])
{
    extern char *optarg;
    extern int optind;
    int opt;
    boolean model_file1_found, model_file2_found;

    /* set defaults */
    model_file1_found = FALSE;
    model_file2_found = FALSE;

    /* get the argument options */
    while ((opt = getopt (argc, argv, "C:d:o:p:1:2:")) != -1)
	switch (opt)
	{
	case 'C':
	    Class_name_found = TRUE;
	    strcpy (Class_name, optarg);
	    break;
	case 'd':
            Debug_level = atol (optarg);
	    break;
	case 'p':
            Debug_progress = atol (optarg);
	    break;
	case 'o':
	    Output_file_found = TRUE;
	    strcpy (Output_filename, optarg);
	    Output_file = TXT_open_file
	      (Output_filename, "w", "Generate counts output file",
	       "Generate_counts: can't open output file" );
	    if (Debug_level > 1)
	        fprintf (stderr, "Writing to output file %s\n",
			 optarg);
	    break;
	case '1':
	    model_file1_found = TRUE;
	    strcpy (Model1_filename, optarg);
	    Model1_file = TXT_open_file
	      (Model1_filename, "r", "Generate counts model1 file",
	       "Generate_counts: can't open model1 file" );
	    if (Debug_level > 1)
	        fprintf (stderr, "Loading Model 1 from file %s\n",
			 optarg);
	    break;
	case '2':
	    model_file2_found = TRUE;
	    strcpy (Model2_filename, optarg);
	    Model2_file = TXT_open_file
	      (Model2_filename, "r", "Generate counts model2 file",
	       "Generate_counts: can't open model2 file" );
	    if (Debug_level > 1)
	        fprintf (stderr, "Loading Model 2 from file %s\n",
			 optarg);
	    break;
	default:
	    usage ();
	    break;
	}

    if (!model_file1_found || !model_file2_found)
        usage ();
    for (; optind < argc; optind++)
	usage ();
}

void
SM_write_output (unsigned int file, unsigned int size)
/* Writes out the output from the categorization. */
{
    fprintf (Files [file], "<Compare>\n");

    fprintf (Files [file], "<File1>%s</File1>\n", Model1_filename);
    fprintf (Files [file], "<File2>%s</File2>\n", Model2_filename);
    if (Class_name_found)
        fprintf (Files [file], "<Class>%s</Class>\n", Class_name);

    SM_dump_counts (file, size);

    fprintf (Files [file], "</Compare>\n");
}

int
main (int argc, char *argv[])
{
    struct SM_model_type *model1, *model2;
    struct SM_trie_type *trie1, *trie2;
    boolean dump_all_nodes, dump_ptrs;
    unsigned int count;

    init_arguments (argc, argv);

    SM_init_counts ();

    model1 = SM_load_model (Model1_file);
    model2 = SM_load_model (Model2_file);

    trie1 = SM_grow_trie (model1);
    SM_check_trie (model1, trie1);

    trie2 = SM_grow_trie (model2);
    SM_check_trie (model2, trie2);

    dump_all_nodes = TRUE;
    dump_ptrs = FALSE;
    if (Debug_level == 1)
        dump_all_nodes = FALSE;
      
    if (Debug_level > 3)
      {
	SM_dump_model (Stderr_File, model1, trie1, dump_all_nodes, dump_ptrs, "1");
	SM_dump_model (Stderr_File, model2, trie2, dump_all_nodes, dump_ptrs, "2");
      }

    count = SM_cotraverse_trie (model1, trie1, model2, trie2);
    SM_accumulate_counts ();

    if (!Output_file_found)
        SM_write_output (Stdout_File, model2->size);
    else
        SM_write_output (Output_file, model2->size);

    exit (0);
}
