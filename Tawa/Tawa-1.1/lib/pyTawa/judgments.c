/* Module for storing, maintaining and comparing relevance judgments
   on text documents. */
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include <assert.h>

#ifdef SYSTEM_LINUX
#include <getopt.h> /* for getopt on Linux systems */
#endif

#define MATRICES_SIZE 20            /* Initial size of Matrices array */
#define THRESHOLD_BYROW_INCR 0.01   /* Increment of increasing the threshold */
#define THRESHOLD_BYROW_MAX 0.2     /* Maximum increasement allowed of the threshold */
#define THRESHOLD_BYCOLUMN_INCR 0.05/* Increment of increasing the threshold */

#include "io.h"
#include "model.h"
#include "text.h"
#include "table.h"
#include "judgments.h"

struct matrixType
{ /* A matrix */
    unsigned int Matrix_rows;       /* Number of rows in the matrix. */
    unsigned int Matrix_columns;    /* Number of rows in the matrix. */
    float Matrix_minvalue;          /* Minimum value in the matrix */
    float Matrix_maxvalue;          /* Maximum value in the matrix */
    float *Matrix_minrow;           /* Minimum value in each row of the matrix */
    float *Matrix_maxrow;           /* Maximum value in each row of the matrix */
    float *Matrix;                  /* Pointer to a two-dimensional matrix. */
};
#define Matrix_next Matrix_rows     /* Used to find the "next" on the used list
				       for deleted Matrices */

struct matrixType *Matrices = NULL; /* List of matrices */
unsigned int Matrices_max_size = 0; /* Current max. size of the Matrices
				       array */
unsigned int Matrices_used = NIL;   /* List of deleted Matrices */
unsigned int Matrices_unused = 1;   /* Next unused text record */

/* Set debug level for this module: */
unsigned int Judgments_debuglevel = 0;

void
judgments_debug ()
/* For debugging purposes only. */
{
  fprintf (stderr, "Got here\n");
}

void
TXT_set_judgment_text (unsigned int judgment_text, unsigned int document,
		       unsigned int topic)
/* Inserts the judgment (document = topic) text into the judgment text
   (used for assigning relevance judgments to documents for categorization
   purposes). */
{
    assert (TXT_valid_text (document));

    assert (TXT_valid_text (document));
    assert (TXT_valid_text (topic));

    TXT_setlength_text (judgment_text, 0);
    TXT_append_text (judgment_text, document);
    TXT_append_symbol (judgment_text, TXT_sentinel_symbol ());
    TXT_append_text (judgment_text, topic);
    TXT_append_symbol (judgment_text, TXT_sentinel_symbol ());
    if (Judgments_debuglevel > 3)
      {
	fprintf (stderr, "Setting judgment: ");
	TXT_dump_text (Stderr_File, judgment_text, NULL);
	fprintf (stderr, "\n");
      }
}

void
TXT_get_judgment_text (unsigned int judgment_text, unsigned int document,
		       unsigned int topic)
/* Extracts from the judgment text the document and topic. */
{
    unsigned int length, symbol, pos;
    boolean valid_judgment;

    assert (TXT_valid_text (document));
    assert (TXT_valid_text (topic));

    length = TXT_length_text (judgment_text);
    valid_judgment =
        (TXT_getpos_text (judgment_text, TXT_sentinel_symbol (), &pos) &&
	 (pos > 0) && (pos < length));
    assert (valid_judgment != FALSE);

    valid_judgment =
        (TXT_get_symbol (judgment_text, length-1, &symbol) &&
	 (symbol == TXT_sentinel_symbol ()));
    assert (valid_judgment != FALSE);

    TXT_extract_text (judgment_text, document, 0, pos);
    TXT_extract_text (judgment_text, topic, pos+1, length-pos-2);
}

void
TXT_load_judgments (char *filename, unsigned int judgments)
/* Load the list of documents and their relevance judgments (topics)
   from the specified file. */
{
    unsigned int judgment, document, topic, topic_id, topic_count;
    unsigned int judgment_id, judgment_count;
    unsigned int input_file, input_line, pos;
    unsigned int table_type, types, tokens;

    assert (TXT_valid_table (judgments));

    document = TXT_create_text ();
    topic = TXT_create_text ();
    judgment = TXT_create_text ();
    input_line = TXT_create_text ();

    input_file = TXT_open_file
      (filename, "r", "Loading relevance judgments from file",
       "TLM: can't open judgments file");

    while (TXT_readline_text (input_file, input_line) > 0)
    {
      pos = 0;
      if (TXT_getword1_text (input_line, document, &pos))
	while (TXT_getword1_text (input_line, topic, &pos))
	  {
	    TXT_set_judgment_text (judgment, document, topic);

	    /*
	    fprintf (stderr, "Document = ");
	    TXT_dump_text (Stderr_File, document, NULL);
	    fprintf (stderr, ", topic = ");
	    TXT_dump_text (Stderr_File, topic, NULL);
	    */

	    TXT_getinfo_table (judgments, &table_type, &types, &tokens);
	    

	    TXT_update_table (judgments, judgment, &judgment_id,
			      &judgment_count);
	    TXT_update_table (judgments, topic, &topic_id,
			      &topic_count);

	    /*
	    fprintf (stderr, " types = %d tokens = %d\n", types, tokens);
	    */
	  }
    }

    /*TXT_dump_table (Stderr_File, judgments);*/

    TXT_release_text (document);
    TXT_release_text (topic);
    TXT_release_text (judgment);
    TXT_release_text (input_line);

    TXT_close_file (input_file);
}

boolean
TXT_find_judgment (unsigned int judgments, unsigned int document,
		   unsigned int topic)
/* Returns TRUE if the judgment is found in the judgments table,
   FALSE otherwise */
{
    unsigned int judgment, judgment_id, judgment_count;
    boolean found;

    assert (TXT_valid_table (judgments));

    judgment = TXT_create_text ();

    if (Judgments_debuglevel > 3)
        fprintf (stderr, "Finding judgments for document %d topic %d\n", document, topic);

    TXT_set_judgment_text (judgment, document, topic);

    found = TXT_getid_table (judgments, judgment, &judgment_id,
			     &judgment_count);

    TXT_release_text (judgment);

    return (found);
}

float *
TXT_create_probs (unsigned int judgments, unsigned int topics)
/* Caclulates various probabilities. */
{
    unsigned int judgments_type, judgments_types, judgments_tokens;
    unsigned int topic, topic_key, topic_id, topic_count, total_count;
    unsigned int topics_type, topics_types, topics_tokens;
    boolean topic_found;
    float *probs;

    assert (TXT_valid_table (judgments));
    TXT_getinfo_table (judgments, &judgments_type, &judgments_types,
		       &judgments_tokens);

    assert (TXT_valid_table (topics));
    TXT_getinfo_table (topics, &topics_type, &topics_types,
		       &topics_tokens);

    probs = (float *) Malloc (152, (topics_types + 1) * sizeof (float));
    total_count = 0;
    for (topic = 0; topic < topics_types; topic++)
      {
	topic_key = TXT_getkey_table (topics, topic);
	assert (topic_key != NIL);

	topic_found = TXT_getid_table (judgments, topic_key, &topic_id,
				       &topic_count);
	assert (topic_found);

	total_count += topic_count;
	probs [topic] = (float) (topic_count);
      }

    for (topic = 0; topic < topics_types; topic++)
	probs [topic] /= (float) total_count;

    return (probs);
}

void
TXT_dump_probs (unsigned int file, unsigned int topics, float *probs)
/* Dumps out a human readable version of the probabilties to a file. */
{
    unsigned int topic, topic_key;
    unsigned int topics_type, topics_types, topics_tokens;

    assert (TXT_valid_file (file));
    assert (TXT_valid_table (topics));
    TXT_getinfo_table (topics, &topics_type, &topics_types,
		       &topics_tokens);

    for (topic = 0; topic < topics_types; topic++)
      {
	topic_key = TXT_getkey_table (topics, topic);
	assert (topic_key != NIL);

	fprintf (Files [file], "Topic ");
	TXT_dump_text (file, topic_key, NULL);
	fprintf (Files [file], " probability = %.3f\n", probs [topic]);
      }

}

void
TXT_release_probs (unsigned int topics, float *probs)
/* Releases the probabilities. */
{
    unsigned int topics_type, topics_types, topics_tokens;

    assert (TXT_valid_table (topics));
    TXT_getinfo_table (topics, &topics_type, &topics_types,
		       &topics_tokens);

    if (probs)
        Free (152, probs, (topics_types + 1) * sizeof (float));
}

boolean
TXT_valid_matrix (unsigned int matrix)
/* Returns non-zero if the matrix is valid, zero otherwize. */
{
    if (matrix == NIL)
        return (FALSE);
    else if (matrix >= Matrices_unused)
        return (FALSE);
    else if ((Matrices [matrix].Matrix_rows == 0) &&
	     (Matrices [matrix].Matrix_columns == 0))
        return (FALSE); /* The rows & columns get set to 0 when the model gets deleted;
			   this way you can test to see if the text has been deleted or not */
    else
        return (TRUE);
}

unsigned int
TXT_create_matrix (unsigned int rows, unsigned int columns)
/* Creates a 2-D matrix of size [rows, columns]. */
{
    struct matrixType *matrixp;
    unsigned int matrix, old_size, matrix_size, row;

    if (Matrices_used != NIL)
    {	/* use the first list of Matrices on the used list */
	matrix = Matrices_used;
	Matrices_used = Matrices [matrix].Matrix_next;
    }
    else
    {
	matrix = Matrices_unused;
	if (Matrices_unused+1 >= Matrices_max_size)
	{ /* need to extend Matrices array */
	    old_size = (Matrices_max_size+2) * sizeof (struct matrixType);
	    if (Matrices_max_size == 0)
		Matrices_max_size = MATRICES_SIZE;
	    else
		Matrices_max_size *= 2; /* Keep on doubling the array on
					   demand */
	    Matrices = (struct matrixType *)
	       Realloc (150, Matrices, (Matrices_max_size+2) * sizeof
			(struct matrixType), old_size);

	    if (Matrices == NULL)
	    {
		fprintf (stderr, "Fatal error: out of Matrices space\n");
		exit (1);
	    }
	}
	Matrices_unused++;
    }

    if (matrix != NIL)
    {
      matrixp = Matrices + matrix;

      matrixp->Matrix = NULL;
      matrixp->Matrix_rows = rows;
      matrixp->Matrix_columns = columns;
      matrixp->Matrix_minvalue = 0.0;
      matrixp->Matrix_maxvalue = 0.0;

      matrix_size = (matrixp->Matrix_rows+1) * (matrixp->Matrix_columns+1) * sizeof (float);
      matrixp->Matrix = (float *) Malloc (151, matrix_size);
      memset (matrixp->Matrix, 0, matrix_size); /* set array to all zeroes */

      matrix_size = (matrixp->Matrix_rows+1) * sizeof (float);
      matrixp->Matrix_minrow = (float *) Malloc (151, matrix_size);
      for (row = 0; row < rows; row++)
	  matrixp->Matrix_minrow [row] = 99999.9; /* default minimum value */
      matrixp->Matrix_maxrow = (float *) Malloc (151, matrix_size);
      for (row = 0; row < rows; row++)
	  matrixp->Matrix_maxrow [row] = 0; /* default maximum value */
    }
    return (matrix);
}

void
TXT_release_matrix (unsigned int matrix)
/* Releases the memory allocated to the matrix and the matrix number
   (which may be reused in later TXT_create_matrix calls). */
{
    struct matrixType *matrixp;

    if (matrix == NIL)
        return;

    assert (TXT_valid_matrix (matrix));
    matrixp = Matrices + matrix;

    if (matrixp->Matrix != NULL)
        Free (151, matrixp->Matrix, (matrixp->Matrix_rows+1) *
	      (matrixp->Matrix_columns+1) * sizeof (float));
    if (matrixp->Matrix_minrow != NULL)
        Free (151, matrixp->Matrix_minrow,
	      (matrixp->Matrix_rows+1) * sizeof (float));
    if (matrixp->Matrix_maxrow != NULL)
        Free (151, matrixp->Matrix_maxrow,
	      (matrixp->Matrix_rows+1) * sizeof (float));

    matrixp->Matrix_rows = 0;    /* Used for testing later on if the matrix
				    number is valid or not */
    matrixp->Matrix_columns = 0; /* Used for testing later on if the matrix
				    is valid or not */

    /* Append onto head of the used list */
    matrixp->Matrix_next = Matrices_used;
    Matrices_used = matrix;
}

void
TXT_dump_matrix (unsigned int file, unsigned int matrix,
		 unsigned int rows_table, unsigned int columns_table)
/* Dumps out a human readable version of the matrix to the file. */
{
    struct matrixType *matrixp;
    unsigned int rows, columns;
    unsigned int row, column, row_key, column_key;
    float value;

    assert (TXT_valid_file (file));
    assert (TXT_valid_matrix (matrix));

    matrixp = Matrices + matrix;
    rows = matrixp->Matrix_rows;
    columns = matrixp->Matrix_columns;

    fprintf (Files [file], "Rows = %d, columns = %d\n", rows, columns);

    for (row = 0; row < rows; row++)
      {
	for (column = 0; column < columns; column++)
	  {
	    value = matrixp->Matrix [row * columns + column];
	    if (value > 0)
	      {
		row_key = TXT_getkey_table (rows_table, row);
		assert (row_key != NIL);
		column_key = TXT_getkey_table (columns_table, column);
		assert (column_key != NIL);

		fprintf (Files [file], "Value = %.3f, Row = ", value);
		TXT_dump_text (file, row_key, NULL);
		fprintf (Files [file], ", Column = ");
		TXT_dump_text (file, column_key, NULL);
		fprintf (Files [file], "\n");
	      }
	  }
      }
}

void
TXT_write_matrix (unsigned int file, unsigned int matrix, unsigned int precision,
		  unsigned int rows_table, unsigned int columns_table)
/* Writes out the matrix to a file (which can later be reloaded). */
{
    struct matrixType *matrixp;
    unsigned int rows, columns;
    unsigned int row, column, row_key, column_key;
    char format [6];
    float value;

    assert (TXT_valid_matrix (matrix));

    matrixp = Matrices + matrix;
    rows = matrixp->Matrix_rows;
    columns = matrixp->Matrix_columns;

    sprintf (format, "%%.%df ", precision);
    for (row = 0; row < rows; row++)
      {
	for (column = 0; column < columns; column++)
	  {
	    value = matrixp->Matrix [row * columns + column];
	    if (value != 0)
	      {
		row_key = TXT_getkey_table (rows_table, row);
		assert (row_key != NIL);
		column_key = TXT_getkey_table (columns_table, column);
		assert (column_key != NIL);

		fprintf (Files [file], format, value);
		TXT_dump_text (file, row_key, NULL);
		fprintf (Files [file], " ");
		TXT_dump_text (file, column_key, NULL);
		fprintf (Files [file], "\n");
	      }
	  }
      }
}

float
TXT_get_matrix (unsigned int matrix, unsigned int row, unsigned int column)
/* Returns the real value in the matrix ar [row, column]. */
{
    struct matrixType *matrixp;
    unsigned int rows, columns;

    assert (TXT_valid_matrix (matrix));

    matrixp = Matrices + matrix;
    rows = matrixp->Matrix_rows;
    assert (row < rows);
    columns = matrixp->Matrix_columns;
    assert (column < columns);

    /*fprintf (stderr, "getting value at %d\n", row * columns + column);*/
    return (matrixp->Matrix [row * columns + column]);
}

void
TXT_put_matrix (unsigned int matrix, unsigned int row, unsigned int column,
		float value)
/* Inserts the real value into the matrix ar [row, column]. */
{
    struct matrixType *matrixp;
    unsigned int rows, columns;

    assert (TXT_valid_matrix (matrix));

    matrixp = Matrices + matrix;
    rows = matrixp->Matrix_rows;
    assert (row < rows);
    columns = matrixp->Matrix_columns;
    assert (column < columns);

    /*fprintf (stderr, "putting value at %d\n", row * columns + column);*/
    matrixp->Matrix [row * columns + column] = value;
    if ((value < matrixp->Matrix_minvalue) || ((matrixp->Matrix_minvalue == 0.0) &&
					       (matrixp->Matrix_maxvalue == 0.0)))
        matrixp->Matrix_minvalue = value;
    if (value > matrixp->Matrix_maxvalue)
        matrixp->Matrix_maxvalue = value;
    if (value < matrixp->Matrix_minrow [row])
        matrixp->Matrix_minrow [row] = value;
    if (value > matrixp->Matrix_maxrow [row])
        matrixp->Matrix_maxrow [row] = value;
}

unsigned int
TXT_load_matrix (char *filename, unsigned int rows_table, unsigned int columns_table)
/* Create and return a new matrix of (rows, columns), and load the
   real values into it from the specified file. */
{
    unsigned int matrix, row_key, column_key, value;
    unsigned int row_id, column_id, row_count, column_count;
    unsigned int rows_type, rows_types, rows_tokens;
    unsigned int columns_type, columns_types, columns_tokens;
    unsigned int input_file, input_text, input_line, line_count, pos, input_pos;
    boolean valid_row, valid_column;
    float value1;

    TXT_getinfo_table (rows_table, &rows_type, &rows_types, &rows_tokens);
    TXT_getinfo_table (columns_table, &columns_type, &columns_types, &columns_tokens);

    fprintf (stderr, "Number of rows = %d number of columns = %d\n",
	     rows_types, columns_types);
    matrix = TXT_create_matrix (rows_types, columns_types);

    row_key = TXT_create_text ();
    column_key = TXT_create_text ();
    value = TXT_create_text ();
    input_line = TXT_create_text ();

    input_file = TXT_open_file
      (filename, "r", "Loading matrix from file",
       "TLM: can't open matrix file");
    input_text = TXT_load_text (input_file);

    line_count = 0;
    input_pos = 0;
    while (TXT_getline_text (input_text, input_line, &input_pos))
    {
      line_count++;

      if (Judgments_debuglevel > 3)
	{
	  fprintf (stderr, "Input line = ");
	  TXT_dump_text (Stderr_File, input_line, NULL);
	  fprintf (stderr, "\n");
	}

      pos = 0;
      if (TXT_getword1_text (input_line, value, &pos) &&
	  TXT_getword1_text (input_line, row_key, &pos) &&
          TXT_getword1_text (input_line, column_key, &pos))
	  {
	    TXT_scanf_text (value, "%f", &value1);

	    /*
	    fprintf (stderr, "Value = %.3f, ", value1);
	    fprintf (stderr, "Row = ");
	    TXT_dump_text (Stderr_File, row_key, NULL);
	    fprintf (stderr, ", Column = ");
	    TXT_dump_text (Stderr_File, column_key, NULL);
	    fprintf (stderr, "\n");
	    */

	    valid_row = TXT_getid_table
	        (rows_table, row_key, &row_id, &row_count);
	    if (!valid_row)
	      {
		fprintf (stderr, "Fatal error: invalid row ");
		TXT_dump_text (Stderr_File, row_key, NULL);
		fprintf (stderr, "\nInput line = ");
		TXT_dump_text (Stderr_File, input_line, NULL);
		fprintf (stderr, "\n");
	      }

	    assert (valid_row);

	    valid_column = TXT_getid_table
	        (columns_table, column_key, &column_id, &column_count);
	    if (!valid_column)
	      {
		fprintf (stderr, "Fatal error: invalid column ");
		TXT_dump_text (Stderr_File, column_key, NULL);
		fprintf (stderr, "\nInput line = ");
		TXT_dump_text (Stderr_File, input_line, NULL);
		fprintf (stderr, "\n");
	      }
	    assert (valid_column);

	    TXT_put_matrix (matrix, row_id, column_id, value1);
	  }
      input_pos++;
    }

    fprintf (stderr, "Loading of matrix completed\n");

    /*TXT_dump_table (Stderr_File, judgments);*/

    TXT_release_text (row_key);
    TXT_release_text (column_key);
    TXT_release_text (value);
    TXT_release_text (input_text);
    TXT_release_text (input_line);

    TXT_close_file (input_file);

    return (matrix);
}

void
TXT_merge_matrix (unsigned int matrix, unsigned int matrix1,
		  float (*merge_function) (float, float))
/* Merges matrix with matrix1 using the merge function. */
{
    struct matrixType *matrixp, *matrixp1;
    unsigned int row, column;
    float value, value1;

    assert (merge_function != NULL);

    assert (TXT_valid_matrix (matrix));
    matrixp = Matrices + matrix;

    assert (TXT_valid_matrix (matrix1));
    matrixp1 = Matrices + matrix;

    assert (matrixp->Matrix_rows == matrixp1->Matrix_rows);
    assert (matrixp->Matrix_columns == matrixp1->Matrix_columns);

    for (row = 0; row < matrixp->Matrix_rows; row++)
      for (column = 0; column < matrixp->Matrix_columns; column++)
	{
	  value = TXT_get_matrix (matrix, row, column);
	  value1 = TXT_get_matrix (matrix1, row, column);
 	  value = merge_function (value, value1);
	  TXT_put_matrix (matrix, row, column, value);
	}
}

void
TXT_merge_matrix1
  (unsigned int matrix, unsigned int matrix1,
   unsigned int judgments, unsigned int topics,
   float (*merge_function) (float, float, float))
/* Merges matrix with matrix1 using the merge function. */
{
    struct matrixType *matrixp, *matrixp1;
    unsigned int row, column;
    float *topicProbs, value, value1;

    assert (merge_function != NULL);
    assert (TXT_valid_table (topics));
    assert (TXT_valid_table (judgments));

    assert (TXT_valid_matrix (matrix));
    matrixp = Matrices + matrix;

    assert (TXT_valid_matrix (matrix1));
    matrixp1 = Matrices + matrix;

    assert (matrixp->Matrix_rows == matrixp1->Matrix_rows);
    assert (matrixp->Matrix_columns == matrixp1->Matrix_columns);

    topicProbs = TXT_create_probs (judgments, topics);

    for (row = 0; row < matrixp->Matrix_rows; row++)
      for (column = 0; column < matrixp->Matrix_columns; column++)
	{
	  value = TXT_get_matrix (matrix, row, column);
	  value1 = TXT_get_matrix (matrix1, row, column);
 	  value = merge_function (value, value1, topicProbs [column]);
	  TXT_put_matrix (matrix, row, column, value);
	}
    TXT_release_probs (topics, topicProbs);
}

void
process_matrix_column
(unsigned int matrix, unsigned int column,
 float threshold, unsigned int threshold_type, unsigned int cutoff_type,
 unsigned int judgments_table, unsigned int rows_table,
 unsigned int columns_table, float *recall, float *precision)
/* Process a single column in the matrix. Calculates the recall and
   precision given the specified threshold. */
{
    unsigned int row, row_key, column_key, column_id;
    unsigned int retrieved_count, correct_count, column_count;
    boolean valid_column, retrieved;
    struct matrixType *matrixp;
    float value, minrow_value, maxrow_value;

    assert (TXT_valid_matrix (matrix));
    matrixp = Matrices + matrix;

    column_key = TXT_getkey_table (columns_table, column);
    assert (column_key != NIL);
    
    valid_column = TXT_getid_table (judgments_table, column_key, &column_id, &column_count);
    assert (valid_column);

    correct_count = 0;
    retrieved_count = 0;
    for (row = 0; row < matrixp->Matrix_rows; row++)
      {
	row_key = TXT_getkey_table (rows_table, row);
	assert (row_key != NIL);

	value = TXT_get_matrix (matrix, row, column);
 
	retrieved = FALSE;
	switch (threshold_type)
	  {
	  case THRESHOLD_BYROW:
	    switch (cutoff_type)
	      {
	      case CUTOFF_ABOVE:
		minrow_value = matrixp->Matrix_minrow [row];
		if (Judgments_debuglevel > 2)
		  fprintf (stderr, "positive value %.3f min %.3f threshold %.3f\n",
			   value, minrow_value, threshold);
		retrieved = (value <= minrow_value + threshold);
	        break;
	      case CUTOFF_BELOW:
		maxrow_value = matrixp->Matrix_maxrow [row];
		if (Judgments_debuglevel > 2)
		  fprintf (stderr, "positive value %.3f max %.3f threshold %.3f\n",
			   value, maxrow_value, threshold);
		retrieved = (value >= maxrow_value - threshold);
	        break;
	      }
	    break;
	  case THRESHOLD_BYCOLUMN:
	    switch (cutoff_type)
	      {
	      case CUTOFF_ABOVE:
		retrieved = (value <= threshold);
		break;
	      case CUTOFF_BELOW:
		retrieved = (value >= threshold);
		break;
	      }
	    break;
	  }

	if (retrieved)
	  {
	    retrieved_count++;
	    if (TXT_find_judgment (judgments_table, row_key, column_key))
	      correct_count++;
	  }
	if (Judgments_debuglevel > 2)
	  {
	    fprintf (stderr, "row = ");
	    TXT_dump_text (Stderr_File, row_key, NULL);
	    fprintf (stderr, " column = ");
	    TXT_dump_text (Stderr_File, column_key, NULL);
	    fprintf (stderr, "\n");

	  }
      }

    if (Judgments_debuglevel > 1)
        fprintf (stderr, "correct = %d retrieved = %d total = %d\n",
		 correct_count, retrieved_count, column_count);
    *recall = (float) correct_count / column_count;
    *precision = (float) correct_count / retrieved_count;
}

float
TXT_recallprecision_matrix
(unsigned int matrix, unsigned int column, unsigned int threshold_type,
 unsigned int cutoff_type, unsigned int judgments_table, unsigned int rows_table,
 unsigned int columns_table)
/* Returns the breakeven recall/precision point for a column in the table.
   Processes each column in turn. */
{
    float threshold, recall, precision, average, best_average, maxdiff;
    struct matrixType *matrixp;
    boolean finish;

    assert (TXT_valid_matrix (matrix));
    matrixp = Matrices + matrix;

    best_average = 0.0;
    maxdiff = (matrixp->Matrix_maxvalue - matrixp->Matrix_minvalue);
    if (maxdiff > THRESHOLD_BYROW_MAX + THRESHOLD_BYROW_INCR)
        maxdiff = THRESHOLD_BYROW_MAX + THRESHOLD_BYROW_INCR;
    switch (threshold_type)
      {
      case THRESHOLD_BYCOLUMN:
	  threshold = matrixp->Matrix_minvalue;
	  break;
      default:
	  threshold = 0.0;
	  break;
      }

    for (;;)
      {
	finish = FALSE;
	switch (threshold_type)
	  {
	  case THRESHOLD_BYROW:
	    if (threshold > maxdiff)
	        finish = TRUE;
	    break;
	  case THRESHOLD_BYCOLUMN:
	    if (threshold > matrixp->Matrix_maxvalue)
	        finish = TRUE;
	    break;
	  }
	if (finish)
	  break;

	process_matrix_column
	  (matrix, column, threshold, threshold_type,
	   cutoff_type, judgments_table, rows_table,
	   columns_table, &recall, &precision);

	average = (recall + precision) / 2.0;
	if (average > best_average)
	  best_average = average;
	if (Judgments_debuglevel > 0)
	    fprintf (stderr, "Threshold = %.2f recall = %.3f precision = %.3f average = %.3f\n",
		     threshold, recall, precision, average);

	switch (threshold_type)
	  {
	  case THRESHOLD_BYROW:
	    threshold += THRESHOLD_BYROW_INCR;
	    break;
	  case THRESHOLD_BYCOLUMN:
	    threshold += THRESHOLD_BYCOLUMN_INCR;
	    break;
	  default:
	    assert ((threshold_type == THRESHOLD_BYROW) ||
		    (threshold_type == THRESHOLD_BYCOLUMN));
	    break;
	  }
      }
    return (best_average);
}

/* Scaffolding for judgments module: 

int
main(int argc, char *argv [])
{
    unsigned int document, topic;

    Judgments = TXT_create_table (TLM_Dynamic, 0);

    document = TXT_create_text ();
    topic = TXT_create_text ();

    init_arguments (argc, argv);

    for (;;)
      {
	printf ("Input document: ");
	if (TXT_readline_text (Stdin_File, document) <= 0)
	  break;
	printf ("Input topic: ");
	if (TXT_readline_text (Stdin_File, topic) <= 0)
	  break;

	if (TXT_find_judgment (Judgments, document, topic))
	  fprintf (stderr, "Judgment is found\n");
	else
	  fprintf (stderr, "Judgment is not found\n");
      }
    return (1);
}
*/
