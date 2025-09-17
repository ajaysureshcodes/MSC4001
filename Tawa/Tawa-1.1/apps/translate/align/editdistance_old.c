/* Calculates the editdistance between two files. */
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>

#ifdef SYSTEM_LINUX
#include <getopt.h> /* for getopt on Linux systems */
#endif

#include "io.h"
#include "text.h"

#define MAX_FILENAME_SIZE 128      /* Maximum size of a filename */

unsigned int Text1_file;
char Text1_filename [MAX_FILENAME_SIZE];
unsigned int Text2_file;
char Text2_filename [MAX_FILENAME_SIZE];

#define COST_DELETION 1
#define COST_INSERTION 1
#define COST_SUBSTITUTION 2

#define MIN3(a, b, c) ((a) < (b) ? ((a) < (c) ? (a) : (c)) : ((b) < (c) ? (b) : (c)))

void
usage (void)
{
    fprintf (stderr,
	     "Usage: train [options]\n"
	     "\n"
	     "options:\n"
	     "  -1 fn\tParallel text for language 1\n"
	     "  -2 fn\tParallel text for language 2\n"
	     "  -3 fn\tModel for language 1\n"
	     "  -4 fn\tModel for language 2\n"
	     "  -d n\tdebug level=n.\n"
	     "  -o fn\toutput filename=fn (required argument)\n"
	     "  -p n\tprogress report every n lines.\n"
	);
    exit (2);
}

void
init_arguments (int argc, char *argv[])
{
    int opt;
    extern char *optarg;
    extern int optind;

    boolean Text1_found = FALSE;
    boolean Text2_found = FALSE;

    /* get the argument options */

    while ((opt = getopt (argc, argv, "1:2:")) != -1)
	switch (opt)
	{
	case '1':
	    Text1_found = TRUE;
	    sprintf (Text1_filename, "%s", optarg);
	    break;
	case '2':
	    Text2_found = TRUE;
	    sprintf (Text2_filename, "%s", optarg);
	    break;
	default:
	    usage ();
	    break;
	}

    if (!Text1_found)
        fprintf (stderr, "\nFatal error: missing name of Text1 filename\n\n");
    if (!Text2_found)
        fprintf (stderr, "\nFatal error: missing name of Text2 filename\n\n");

    if (!Text1_found || !Text2_found)
      {
	usage ();
	exit (1);
      }

    for (; optind < argc; optind++)
	usage ();
}

unsigned int
LevenshteinDistance(unsigned int t1, unsigned int t2)
/* Returns the Levenshtein Distance between two texts t1 and t2. */
{
  unsigned int t1length, t2length, t1sym, t2sym, cost;
  unsigned int lastdiag, olddiag, x, y;
  unsigned int *row;

  /* degenerate cases */
  if (t1 == NIL) return 0; 
  if (t2 == NIL) return 0; 

  if (!TXT_compare_text (t1, t2)) return 0;
  t1length = TXT_length_text (t1);
  t2length = TXT_length_text (t2);
  printf ("Length of Text1 = %d, length of Text2 = %d\n",
	  t1length, t2length);

  if (t1length == 0) return t2length;
  if (t2length == 0) return t1length;

  row = (unsigned int *) calloc (t1length + 1, sizeof (unsigned int));

  for (y = 1; y <= t1length; y++)
    row[y] = y;
  for (x = 1; x <= t2length; x++)
    {
      row[0] = x;
      for (y = 1, lastdiag = x-1; y <= t1length; y++)
	{
	  TXT_get_symbol (t1, y-1, &t1sym);
	  TXT_get_symbol (t2, x-1, &t2sym);
	  olddiag = row[y];
	  row[y] = MIN3(
			row[y] + COST_DELETION,
			row[y-1] + COST_INSERTION,
			lastdiag + (t1sym == t2sym ? 0 : COST_SUBSTITUTION)
		       );
	  lastdiag = olddiag;
	}
    }

  return (row[t1length]);
}

unsigned int
LevenshteinDistance1(unsigned int t1, unsigned int t2)
/* Returns the Levenshtein Distance between two texts t1 and t2. *** Not tested fully yet ****/
{
  unsigned int t1length, t2length, t1sym, t2sym, cost, i, j;
  unsigned int *v0, *v1;

  /* degenerate cases */
  if (t1 == NIL) return 0; 
  if (t2 == NIL) return 0; 

  if (!TXT_compare_text (t1, t2)) return 0;
  t1length = TXT_length_text (t1);
  t2length = TXT_length_text (t2);
  printf ("Length of Text1 = %d, length of Text2 = %d\n",
	  t1length, t2length);

  if (t1length == 0) return t2length;
  if (t2length == 0) return t1length;

  /* create two work vectors of integer distances */
  v0 = (unsigned int *) calloc (t2length + 1, sizeof (unsigned int));
  v1 = (unsigned int *) calloc (t2length + 1, sizeof (unsigned int));

  /* initialize v0 (the previous row of distances) */
  /* this row is A[0][i]: edit distance for an empty t1 */
  /* the distance is just the number of characters to delete from t */
  for (i = 0; i < t2length + 1; i++)
      v0[i] = i;

  for (i = 0; i < t1length; i++)
    {
      /* calculate v1 (current row distances) from the previous row v0 */

      /* first element of v1 is A[i+1][0] */
      /*   edit distance is delete (i+1) chars from t1 to match empty t */
      v1[0] = i + 1;

      /* use formula to fill in the rest of the row */
      for (j = 0; j < t2length; j++)
        {
	  TXT_get_symbol (t1, i, &t1sym);
	  TXT_get_symbol (t2, i, &t2sym);

	  cost = (t1sym == t2sym) ? 0 : 1;
	  v1[j + 1] = min3(v1[j] + 1, v0[j + 1] + 1, v0[j] + cost);
        }

      /* copy v1 (current row) to v0 (previous row) for next iteration */
      for (j = 0; j < t2length + 1; j++)
	  v0[j] = v1[j];
    }

    return v1[t2length];
}

int
main (int argc, char *argv[])
{

  unsigned int t1, t2, d;
 
  init_arguments (argc, argv);

  Text1_file = TXT_open_file
      (Text1_filename, "r", "Reading Text1 file",
       "Editdistance: can't open Text1 file" );
  Text2_file = TXT_open_file
      (Text2_filename, "r", "Reading Text2 file",
       "Editdistance: can't open Text2 file" );

  t1 = TXT_load_text (Text1_file);
  t2 = TXT_load_text (Text2_file);
  /* Remove sentinel symbols */
  TXT_setlength_text (t1, TXT_length_text (t1) - 1);
  TXT_setlength_text (t2, TXT_length_text (t2) - 1);

  d = LevenshteinDistance (t1, t2);
  printf ("Levenshtein Distance between the two text files is = %d\n", d);

  exit (0);
}
