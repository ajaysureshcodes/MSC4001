/* Program to produce suffix model and write it to disk.
   Assumes that the suffix array can all fit in memory. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>

#include "io.h"
#include "model.h"
#include "SM_model.h"

unsigned int Array_size = 0;    /* The size of the suffix array */
char Input_filename [100];      /* The name of the input file. */
unsigned int Input_file;        /* The input file */
char Output_filename [100];     /* The name of the output file. */
unsigned int Output_file;       /* The output file */

boolean Numbers = FALSE;        /* Input sequence is numbers, not characters */

void
usage (void)
{
    fprintf (stderr,
	     "Usage: train_suffixes [options] -i infile -o outfile\n"
	     "\n"
	     "options:\n"
	     "  -i fn\tinput filename=fn\n"
	     "  -o fn\toutput filename=fn\n"
	     "  -N\tinput is a sequence of numbers, not chars\n"
	     );
    exit (2);
}

void
init_arguments (int argc, char *argv[])
{
    extern char *optarg;
    extern int optind;
    int opt, cc;
    boolean input_filename_found, output_filename_found;

    /* set defaults */
    input_filename_found = FALSE;
    output_filename_found = FALSE;

    /* get the argument options */
    while ((opt = getopt (argc, argv, "i:o:N")) != -1)
	switch (opt)
	{
	case 'i':
	    input_filename_found = TRUE;
	    strcpy (Input_filename, optarg);
	    Input_file = TXT_open_file
	      (Input_filename, "r", "Train suffixes input file",
	       "Train_suffixes: can't open train suffixes input file" );

	    /* Read in the file to determine its size */
	    Array_size = 0;
	    if (!Numbers)
	      {
		while ((cc = fgetc (Files [Input_file])) != EOF)
		  Array_size++;
		assert (Array_size > 0);
		fclose (Files [Input_file]);
	      }
	    else
	      {
		unsigned int number, eof;
		int result;

		eof = FALSE;
		for (;;)
		  { /* Repeat until EOF or array_size exceeded */
		    result = fscanf (Files [Input_file], "%u", &number);
		    switch (result)
		      {
		      case 1: /* one number read successfully */
			break;
		      case EOF: /* eof found */
			eof = TRUE;
			break;
		      case 0:
			fprintf (stderr, "Formatting error in file\n");
			exit (1);
		      default:
			fprintf (stderr, "Unknown error (%i) reading file\n", result);
			exit (1);
		      }
		    if (eof)
		        break;
		    Array_size++;
		  }
	      }

	    Input_file = TXT_open_file
	      (Input_filename, "r", "Train suffixes input file",
	       "Train_suffixes: can't open train suffixes input file" );

	    fprintf (stderr, "Loading %d symbols\n", Array_size);

	    break;
	case 'o':
	    output_filename_found = TRUE;
	    strcpy (Output_filename, optarg);
	    
	    Output_file = TXT_open_file
	      (Output_filename, "w", "Train suffixes output file",
	       "Train_suffixes: can't open train suffixes output file" );
	    break;
	case 'N':
	    Numbers = TRUE;
	    break;
	default:
	    usage ();
	    break;
	}

    if (!input_filename_found || !output_filename_found)
        usage ();
    for (; optind < argc; optind++)
	usage ();
}

int
main (int argc, char *argv[])
{
    unsigned int *sarray, string_size;
    char *string;
    unsigned int *nstring;

    init_arguments (argc, argv);

    /* Read in the input file */
    if (!Numbers)
      {
        string = (char *) malloc ((Array_size+2) * sizeof (char));
	string_size = fread (string, sizeof (char), Array_size,
			     Files [Input_file]);

	if (string_size == Array_size)
	  { /* Add a null */
	    string_size++;
	    string [Array_size] = '\0';
	  }

	sarray = SM_make_sarray (string_size, string);

	/* SM_dump_sarray (stderr, string, string_size, sarray); */

	SM_write_sarray (Output_file, string_size, string, sarray);
      }
    else
      {
        nstring = (unsigned int *) malloc ((Array_size+2) * sizeof (unsigned int));

	string_size = fread_numbers1 (Input_file, Array_size, nstring);

	/* Add a null */
	string_size++;
	nstring [string_size] = 0;

	sarray = SM_make_nsarray (string_size, nstring);

	/* SM_dump_nsarray (stderr, nstring, string_size, sarray); */

	SM_write_nsarray (Output_file, string_size, nstring, sarray);
      }

    exit (0);
}
