/* Works out the language of a file.. Calculates the
   Mean Average Precision (MAP). */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>

#ifdef SYSTEM_LINUX
#include <getopt.h> /* for getopt on Linux systems */
#endif

#include "io.h"
#include "text.h"
#include "model.h"

#define MAX_ALPHABET 256  /* Default maximum alphabet size for all PPM models */
#define MAX_ORDER 5       /* Maximum order of the model */
#define MAX_ROWS 100      /* Default maximum number or rows in the matrix */
#define MAX_COLUMNS 100   /* Default maximum number or columns in the matrix */

unsigned int debug_level = 0;
unsigned int Ignore = 0;
boolean By_Columns = TRUE;

float **matrix; /* Matrix of codelengths */
unsigned int matrix_rows = 0; /* Number of rows in the matrix */
unsigned int matrix_columns = 0; /* Number of columns in the matrix */
unsigned int matrix_row_alloc = 0; /* Number of rows allocated in the matrix */
unsigned int matrix_column_alloc= 0; /* Number of columns allocated in the matrix */
char **matrix_row_labels; /* Row labels for the matrix */
char **matrix_column_labels; /* Column labels for the matrix */

void
usage (void)
{
    fprintf (stderr,
	     "Usage: ident_file [options] <input-text\n"
	     "\n"
	     "  -R\tcalculate by rows (default is by columns)\n"
	     "  -d n\tdebug level=n\n"
	     "  -i n\tignore this number of least-ranked relevant documents=n\n"
	     "options:\n"
	);
    exit (2);
}

void
init_arguments (int argc, char *argv[])
{
    extern char *optarg;
    extern int optind;
    int opt;

    /* get the argument options */

    while ((opt = getopt (argc, argv, "Rd:i:")) != -1)
	switch (opt)
	{
	case 'R':
	    By_Columns = FALSE;
	    break;
	case 'd':
	    debug_level = atoi (optarg);
	    break;
	case 'i':
	    Ignore = atoi (optarg);
	    break;
	default:
	    usage ();
	    break;
	}
    for (; optind < argc; optind++)
	usage ();
}

void
loadMatrix (FILE *fp)
/* Loads the confusion matrix from the file fp. */
{
    char word [128];
    unsigned int rows, columns, column;
    int f, p, old;
    float val;

    if ((fscanf (fp, "%s", word) != 1) ||
	(strcmp (word, "%c") != 0))
      {
	fprintf (stderr, "Fatal error reading matrix file\n");
	exit(1);
      }

    matrix_row_alloc = MAX_ROWS;
    matrix_column_alloc = MAX_COLUMNS;
    matrix = (float **) calloc (MAX_ROWS, sizeof (char *));
    matrix_row_labels = (char **) calloc (MAX_ROWS, sizeof (char *));
    matrix_column_labels = (char **) calloc (MAX_COLUMNS, sizeof (char *));

    columns = 0;
    for (;;)
      { /* Read in the column labels */
	f = fscanf (fp, "%s", word);
	if ((f != 1) || (strcmp (word, "%t") == 0))
	    break;

	if (debug_level > 2)
	    fprintf (stderr, "col %d = %s\n", columns, word);
	if (columns >= matrix_column_alloc)
	  { /* Extend allocation for the matrix columns */
	    old = matrix_column_alloc;
	    matrix_column_alloc *= 2; /* Keep on doubling allocation */
	    matrix_column_labels = (char **)
	      realloc (matrix_column_labels, matrix_column_alloc * sizeof (char **));
	    for (p = old; p < matrix_column_alloc; p++)
	      matrix_column_labels [p] = NULL;
	  }

	matrix_column_labels [columns] = (char *) malloc (strlen (word) + 1);
	strcpy (matrix_column_labels [columns], word);

	columns++;
      }
    if (debug_level > 2)
        fprintf (stderr, "Number of columns = %d\n", columns);
    matrix_columns = columns;

    rows = 0;
    for (;;)
      { /* Read each row in turn */
	if (f == EOF)
	    break;

	if (strcmp (word, "%t") != 0)
	  {
	    fprintf (stderr, "Fatal error reading matrix file\n");
	    exit(1);
	  }

	f = fscanf (fp, "%s", word);
	if (f != 1)
	  {
	    fprintf (stderr, "Fatal error reading matrix file\n");
	    exit(1);
	  }

	if (rows >= matrix_row_alloc)
	  { /* Extend allocation for the matrix rows */
	    old = matrix_row_alloc;
	    matrix_row_alloc *= 2; /* Keep on doubling allocation */
	    matrix = (float **)
	      realloc (matrix, matrix_row_alloc * sizeof (float **));
	    matrix_row_labels = (char **)
	      realloc (matrix_row_labels, matrix_row_alloc * sizeof (char **));
	    for (p = old; p < matrix_row_alloc; p++)
	      matrix_row_labels [p] = NULL;
	  }

	matrix_row_labels [rows] = (char *) malloc (strlen (word) + 1);
	strcpy (matrix_row_labels [rows], word);

	if (debug_level > 2)
	    fprintf (stderr, "row %d = %s\n", rows, word);
	matrix [rows] = (float *) calloc (matrix_columns, sizeof(float *));

	column = 0;
	for (;;)
	  { /* Read in all the column data */
	    if (column > matrix_columns)
	      {
		fprintf (stderr, "Fatal error reading matrix file - too much column data\n");
		exit(1);
	      }
	    f = fscanf (fp, "%s", word);
	    if (f == EOF)
	      break;
	    if (f != 1)
	      {
		fprintf (stderr, "Fatal error reading matrix file\n");
		exit(1);
	      }
	    if (strcmp (word, "%t") == 0)
	        break; /* Start of new column found */

	    f = sscanf (word, "%f", &val);
	    if (f != 1)
	      {
		fprintf (stderr, "Fatal error reading matrix value\n");
		exit(1);
	      }

	    if (debug_level > 3)
	        fprintf (stderr, "Value = %.3f\n", val);
	    matrix [rows][column] = val;

	    column++;
	  }
	if (column != matrix_columns)
	  {
	    fprintf (stderr, "Fatal error reading matrix file - not enough column data: found %d, should be %d\n", column, matrix_columns);
	    exit(1);
	      }

	rows++;
      }

    if (debug_level > 2)
        fprintf (stderr, "Number of rows = %d\n", rows);
    matrix_rows = rows;
}

void
dumpMatrix (FILE *fp)
/* Dump all the results from the matrix. */
{
    unsigned int r, c;

    fprintf (stderr, "Dump of matrix\n");
    fprintf (stderr, "--------------\n");

    fprintf (stderr, "Dump of matrix column labels:\n");
    for (c = 0; c < matrix_columns; c++)
        fprintf (stderr, "%d %s\n", c, matrix_column_labels [c]);

    fprintf (stderr, "Dump of matrix row labels:\n");
    for (r = 0; r < matrix_rows; r++)
        fprintf (stderr, "%d %s\n", r, matrix_row_labels [r]);

    fprintf (stderr, "Dump of matrix values:\n");
    for (r = 0; r < matrix_rows; r++)
      for (c = 0; c < matrix_columns; c++)
      {
	fprintf (fp, "%d %d %.4f\n", r, c, matrix [r][c]);
      }
}

static float *Values = NULL;
static unsigned int *Index = NULL;

int
qsort_compare (const void *index1, const void *index2)
/* Comparison function used by qsort. */
{
  float val1, val2;
  int p1, p2;

  if (debug_level > 3)
      fprintf (stderr, "Comparing %p with %p\n", index1, index2);
  p1 = *((unsigned int *) index1);
  p2 = *((unsigned int *) index2);
  if (debug_level > 3)
      fprintf (stderr, "Index %d with %d\n", p1, p2);

  val1 = *(Values + p1);
  val2 = *(Values + p2);
  if (debug_level > 3)
      fprintf (stderr, "Value %.3f with %.3f\n", val1, val2);

  if (val1 == val2)
    {
      if (p1 > p2)
	 return (1);
      if (p1 < p2)
	 return (-1);
      return 0;
    }
  else if (val1 < val2)
      return -1;
  else
      return 1;
}

void
computeMAP_by_columns ()
/* Computes the MAP for the matrix using column data. */
{
    unsigned int row, column;
    unsigned int relevant, not_relevant, last_relevant;
    int srow, ignore;
    float prec, avg_prec, map;

    Values = (float *) malloc (matrix_rows * sizeof (float));
    Index = (unsigned int *) malloc (matrix_rows * sizeof (unsigned int));

    map = 0;
    for (column = 0; column < matrix_columns; column++)
      {
	avg_prec = 0;

	/* Copy data from this column */
	for (row = 0; row < matrix_rows; row++)
	  {
	    Values [row] = matrix [row][column];
	    Index [row] = row;
	  }

	/* Sort the column data in Values */
	qsort (Index, matrix_rows, sizeof (unsigned int), qsort_compare);

	/* Dump out the sorted data */
	if (debug_level > 2)
	  for (srow = 0; srow < matrix_rows; srow++)
	    {
	      fprintf (stderr, "%d at sort row %d value %.3f\n", srow, Index [srow],
		       matrix [Index [srow]][column]);
	    }

	/* Work out the average precision */
	ignore = 0;
	relevant = 0;
	not_relevant = 0;
	last_relevant = 0;
	for (srow = matrix_rows-1; srow >= 0; srow--)
	  { /* find out last relevant item */
	    if (!strcmp (matrix_column_labels [column], matrix_row_labels [Index [srow]]))
	      {
		last_relevant = srow;
		ignore++;
		if (ignore > Ignore)
		    break;
	      }
	  }
	for (srow = 0; srow <= last_relevant; srow++)
	  {
	    if (strcmp (matrix_column_labels [column], matrix_row_labels [Index [srow]]))
	        not_relevant++;
	    else
	      {
		relevant++;
		if (debug_level > 1)
		    fprintf (stderr, "Found relevant label for column %d row %d sort-row %d\n",
			     column, Index [srow], srow);
	      }
	    prec = (float) relevant/(relevant + not_relevant);
	    avg_prec += prec;
	    if (debug_level > 0)
	        fprintf (stderr, "Precision %.3f total %.3f relevant %d/%d for column %d row %d sort-row %d\n",
			 prec, avg_prec, relevant, (relevant + not_relevant), column, Index [srow], srow);
	  }

	avg_prec /= (last_relevant+1);
	if (debug_level > 0)
	    fprintf (stderr, "Average precision for column %d label %s = %.3f\n",
		     column, matrix_column_labels [column], avg_prec);

	map += avg_prec;
      }

    map /= matrix_columns;
    fprintf (stdout, "Mean average precision = %.4f\n", map);

    free (Values);
    free (Index);
}

void
computeMAP_by_rows ()
/* Computes the MAP for the matrix using row data. */
{
    unsigned int row, column;
    unsigned int relevant, not_relevant, last_relevant;
    int scolumn, ignore;
    float prec, avg_prec, map;

    Values = (float *) malloc (matrix_columns * sizeof (float));
    Index = (unsigned int *) malloc (matrix_columns * sizeof (unsigned int));

    map = 0;
    for (row = 0; row < matrix_rows; row++)
      {
	avg_prec = 0;

	/* Copy data from this row */
	for (column = 0; column < matrix_columns; column++)
	  {
	    Values [column] = matrix [row][column];
	    Index [column] = column;
	  }

	/* Sort the row data in Values */
	qsort (Index, matrix_columns, sizeof (unsigned int), qsort_compare);

	/* Dump out the sorted data */
	if (debug_level > 2)
	  for (scolumn = 0; scolumn < matrix_columns; scolumn++)
	    {
	      fprintf (stderr, "%d at sort column %d value %.3f\n", scolumn, Index [scolumn],
		       matrix [row][Index [scolumn]]);
	    }

	/* Work out the average precision */
	ignore = 0;
	relevant = 0;
	not_relevant = 0;
	last_relevant = 0;
	for (scolumn = matrix_columns-1; scolumn >= 0; scolumn--)
	  { /* Find out last relevant item */
	    if (!strcmp (matrix_row_labels [row], matrix_column_labels [Index [scolumn]]))
	      {
		last_relevant = scolumn;
		ignore++;
		if (ignore > Ignore)
		    break;
	      }
	  }
	for (scolumn = 0; scolumn <= last_relevant; scolumn++)
	  {
	    if (strcmp (matrix_row_labels [row], matrix_column_labels [Index [scolumn]]))
	        not_relevant++;
	    else
	      {
		relevant++;
		if (debug_level > 1)
		    fprintf (stderr, "Found relevant label for row %d column %d sort-column %d\n",
			     row, Index [scolumn], scolumn);
	      }
	    prec = (float) relevant/(relevant + not_relevant);
	    avg_prec += prec;
	    if (debug_level > 0)
	        fprintf (stderr, "Precision %.3f total %.3f relevant %d/%d for row %d column %d sort-column %d\n",
			 prec, avg_prec, relevant, (relevant + not_relevant), row, Index [scolumn], scolumn);
	  }

	avg_prec /= (last_relevant+1);
	if (debug_level > 0)
	    fprintf (stderr, "Average precision for row %d label %s = %.3f\n",
		     row, matrix_row_labels [row], avg_prec);

	map += avg_prec;
      }

    map /= matrix_rows;
    fprintf (stdout, "Mean average precision = %.4f\n", map);

    free (Values);
    free (Index);
}

void
computeMAP (boolean by_columns)
/* Computes the MAP for the matrix by rows or by columns. */
{
    if (by_columns)
        computeMAP_by_columns ();
    else
        computeMAP_by_rows ();
}

int
main (int argc, char *argv[])
{
    init_arguments (argc, argv);

    /*dumpModels (stdout);*/

    loadMatrix (stdin);
    if (debug_level > 1)
        dumpMatrix (stdout);

    computeMAP (By_Columns);

    free(matrix);

    return 0;
}
