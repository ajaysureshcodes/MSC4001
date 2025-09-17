/* Definitions of various structures for judgments (used for
   storing, maintaining and comparing relevance judgments
   on text documents. */

#ifndef JUDGMENTS_H
#define JUDGMENTS_H

/* Different types of thresholds for calculating the break-even recall/precision
   point for a matrix: */
#define THRESHOLD_BYROW 0
#define THRESHOLD_BYCOLUMN 1
#define CUTOFF_BELOW 0
#define CUTOFF_ABOVE 1

/* Set debug level for this module: */
extern unsigned int Judgments_debuglevel;

void
TXT_set_judgment_text (unsigned int judgment_text, unsigned int document,
		       unsigned int topic);
/* Inserts the judgment (document = topic) text into the judgment text
   (used for assigning relevance judgments to documents for categorization
   purposes). */

void
TXT_get_judgment_text (unsigned int judgment_text, unsigned int document,
		       unsigned int topic);
/* Extracts from the judgment text the document and topic. */

void
TXT_load_judgments (char *filename, unsigned int judgments);
/* Load the list of documents and their relevance judgments (topics)
   from the specified file. */

boolean
TXT_find_judgment (unsigned int judgments, unsigned int document,
		   unsigned int topic);
/* Returns TRUE if the judgment is found in the judments table,
   FALSE otherwise */

float *
TXT_create_probs (unsigned int judgments, unsigned int topics);
/* Caclulates various probabilities. */

void
TXT_dump_probs (unsigned int file, unsigned int topics, float *probs);
/* Dumps out a human readable version of the probabilties to a file. */

void
TXT_release_probs (unsigned int topics, float *probs);
/* Releases the probabilities. */

boolean
TXT_valid_matrix (unsigned int matrix);
/* Returns non-zero if the matrix is valid, zero otherwize. */

unsigned int
TXT_create_matrix (unsigned int rows, unsigned int columns);
/* Creates a 2-D matrix of size [rows, columns]. */

void
TXT_release_matrix (unsigned int matrix);
/* Releases the memory allocated to the matrix and the matrix number
   (which may be reused in later TXT_create_matrix calls). */

void
TXT_dump_matrix (unsigned int file, unsigned int matrix,
		 unsigned int rows_table, unsigned int columns_table);
/* Dumps out a human readable version of the matrix to the file. */

void
TXT_write_matrix (unsigned int file, unsigned int matrix, unsigned int precision,
		  unsigned int rows_table, unsigned int columns_table);
/* Writes out the matrix to a file (which can later be reloaded). */

void
TXT_put_matrix (unsigned int matrix, unsigned int row, unsigned int column,
		float value);
/* Inserts the real value into the matrix ar [row, column]. */

float
TXT_get_matrix (unsigned int matrix, unsigned int row, unsigned int column);
/* Returns the real value in the matrix ar [row, column]. */

unsigned int
TXT_load_matrix (char *filename, unsigned int rows_table, unsigned int columns_table);
/* Create and return a new matrix of (rows, columns), and load the
   real values into it from the specified file. */

void
TXT_merge_matrix (unsigned int matrix, unsigned int matrix1,
		  float (*merge_function) (float, float));
/* Merges matrix with matrix1 using the merge function. */

void
TXT_merge_matrix1
  (unsigned int matrix, unsigned int matrix1,
   unsigned int judgments, unsigned int topics,
   float (*merge_function) (float, float, float));
/* Merges matrix with matrix1 using the merge function. */

float
TXT_recallprecision_matrix
(unsigned int matrix, unsigned int column, unsigned int threshold_type,
 unsigned int cutoff_type, unsigned int judgments_table, unsigned int rows_table,
 unsigned int columns_table);
/* Returns the breakeven recall/precision point for a column in the table.
   Processes each column in turn. */

#endif
