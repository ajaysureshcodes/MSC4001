/* Utility routines for Suffix Models.
   Assumes that the suffix trie can all fit in memory. */
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

unsigned int Debug_level = 0;
unsigned int Debug_progress = 0;

unsigned int Min_Prefix_Length = 0;

unsigned int *RCounts = NULL;     /* R-Measure counts for various orders */
unsigned int RCounts_Size = 0;    /* Maximum size of R-Measures array */
unsigned int RCounts_Max = 0;     /* Maximum R-Measure Count */
unsigned int *R1Counts = NULL;    /* R1-Measure counts for various orders */
unsigned int R1Counts_Size = 0;   /* Maximum size of R1-Measures array */
unsigned int R1Counts_Max = 0;    /* Maximum R1-Measure Count */

unsigned int *CCounts = NULL;     /* C-Measure counts for various orders */
unsigned int CCounts_Size = 0;    /* Maximum size of C-Measures array */
unsigned int CCounts_Max = 0;     /* Maximum C-Measure Count */
unsigned int *C1Counts = NULL;    /* C1-Measure counts for various orders */
unsigned int C1Counts_Size = 0;   /* Maximum size of C1-Measures array */
unsigned int C1Counts_Max = 0;    /* Maximum C1-Measure Count */

void
SM_increment_count (unsigned int count_type, unsigned int position,
		    unsigned int increment)
/* Increments the count at index position by increment. */
{
    unsigned int max, old_max, p;

    max = 0;
    switch (count_type)
      {
      case COUNT_R_MEASURE:
	max = RCounts_Size;

	if (position >= max)
	  { /* Need to extend counts array */
	    old_max = max;
	    max = ((position / COUNTS_SIZE) + 1) * COUNTS_SIZE;
	    assert (position < max);

	    RCounts = (unsigned int *)
	      realloc (RCounts, max * sizeof (unsigned int));

	    for (p = old_max; p < max; p++)
	        RCounts [p] = 0;
	    RCounts_Size = max;
	  }

	RCounts [position] += increment;
	if (position > RCounts_Max)
	    RCounts_Max = position;
	break;
      case COUNT_R1_MEASURE:
	max = R1Counts_Size;

	if (position >= max)
	  { /* Need to extend counts array */
	    old_max = max;
	    max = ((position / COUNTS_SIZE) + 1) * COUNTS_SIZE;
	    assert (position < max);

	    R1Counts = (unsigned int *)
	      realloc (R1Counts, max * sizeof (unsigned int));

	    for (p = old_max; p < max; p++)
	        R1Counts [p] = 0;
	    R1Counts_Size = max;
	  }

	R1Counts [position] += increment;
	if (position > R1Counts_Max)
	    R1Counts_Max = position;
	break;
      case COUNT_C_MEASURE:
	max = CCounts_Size;

	if (position >= max)
	  { /* Need to extend counts array */
	    old_max = max;
	    max = ((position / COUNTS_SIZE) + 1) * COUNTS_SIZE;
	    assert (position < max);

	    CCounts = (unsigned int *)
	      realloc (CCounts, max * sizeof (unsigned int));

	    for (p = old_max; p < max; p++)
	        CCounts [p] = 0;
	    CCounts_Size = max;
	  }

	CCounts [position] += increment;
	if (position > CCounts_Max)
	    CCounts_Max = position;
	break;

      case COUNT_C1_MEASURE:
	max = C1Counts_Size;

	if (position >= max)
	  { /* Need to extend counts array */
	    old_max = max;
	    max = ((position / COUNTS_SIZE) + 1) * COUNTS_SIZE;
	    assert (position < max);

	    C1Counts = (unsigned int *)
	      realloc (C1Counts, max * sizeof (unsigned int));

	    for (p = old_max; p < max; p++)
	        C1Counts [p] = 0;
	    C1Counts_Size = max;
	  }

	C1Counts [position] += increment;
	if (position > C1Counts_Max)
	    C1Counts_Max = position;
	break;
      default:
	fprintf (stderr, "Count type %d is not yet implemented\n", count_type);
	exit (1);
	break;
      }
}

unsigned int
SM_get_count (unsigned int count_type, unsigned int position)
/* Returns the value of the count at index position.
   Aborts with a run-time error if the position exceeds the current
   maximum size of the array. */
{
    unsigned int count;

    switch (count_type)
      {
      case COUNT_R_MEASURE:
	assert (position < RCounts_Size);

	count = RCounts [position];
	break;
      case COUNT_R1_MEASURE:
	assert (position < R1Counts_Size);

	count = R1Counts [position];
	break;
      case COUNT_C_MEASURE:
	assert (position < CCounts_Size);

	count = CCounts [position];
	break;
      case COUNT_C1_MEASURE:
	assert (position < C1Counts_Size);

	count = C1Counts [position];
	break;
      default:
	fprintf (stderr, "Count type %d is not yet implemented\n", count_type);
	exit (1);
	break;
      }

    return (count);
}

void
SM_init_counts ()
/* Initializes the various counts. */
{
    /* The following will initially allocate COUNTS_SIZE counts
       to each array and set their values to 0 */
    SM_increment_count (COUNT_R_MEASURE, 1, 0);
    SM_increment_count (COUNT_R1_MEASURE, 1, 0);
    SM_increment_count (COUNT_C_MEASURE, 1, 0);
    SM_increment_count (COUNT_C1_MEASURE, 1, 0);
}

void
SM_dump_counts (unsigned int file, unsigned int size)
/* Dumps out the various counts (for debugging purposes). */
{
    unsigned long long size1, size2;
    unsigned int count, count1, p;

    fprintf (Files [file], "<Size>%d</Size>\n", size);
    size1 = size * (size + 1) / 2;
    fprintf (Files [file], "<Size1>%lld</Size1>\n", size1);

    for (p = 0; p < CCounts_Size; p++)
      {
	count = SM_get_count (COUNT_C_MEASURE, p);
	count1 = size - p + 1 - count;

	if (count)
	  {
	    fprintf (Files [file], "<c-%d>%d</c-%d>\n", p, count, p);
	    fprintf (Files [file], "<C-%d>%.4f</C-%d>\n", p,
		     (float) count/(size - p + 1), p);
	    fprintf (Files [file], "<c*%d>%d</c*%d>\n", p, count1, p);
	    fprintf (Files [file], "<C*%d>%.4f</C*%d>\n", p,
		     (float) count1/(size - p + 1), p);
	  }
      }

    /*
    for (p = 0; p < C1Counts_Size; p++)
      {
	count = SM_get_count (COUNT_C1_MEASURE, p);
	if (count)
	    fprintf (Files [file], "<C1-%d>%d</C1-%d>\n", p, count, p);
      }
    */

    for (p = 0; p < RCounts_Size; p++)
      {
	count = SM_get_count (COUNT_R_MEASURE, p);
	if (count)
	  {
	    fprintf (Files [file], "<r-%d>%d</r-%d>\n", p, count, p);
	    size2 = (size - p + 1) * (size - p + 2) / 2;
	    fprintf (Files [file], "<R-%d>%.4f</R-%d>\n", p, (float) count/size2, p);
	  }
      }

    for (p = 0; p < R1Counts_Size; p++)
      {
	count = SM_get_count (COUNT_R1_MEASURE, p);
	if (count)
	  {
	    fprintf (Files [file], "<r*%d>%d</r*%d>\n", p, count, p);
	    size2 = p * size - p * (1 - p) / 2;
	    fprintf (Files [file], "<R*%d>%.4f</R*%d>\n", p, (float) count/size2, p);
	  }
      }
}

void
SM_accumulate_counts ()
/* Updates the cumulative counts. */
{
    unsigned int count, p, q;
    
    for (p = 1; p <= CCounts_Max; p++)
      {
	for (q = p; q <= CCounts_Max; q++)
	  {
	    count = SM_get_count (COUNT_C_MEASURE, q);
	    if (count)
	        SM_increment_count (COUNT_R_MEASURE, p, count);
	  }

	for (q = 1; q <= p; q++)
	  {
	    count = SM_get_count (COUNT_C_MEASURE, q);
	    if (count)
	        SM_increment_count (COUNT_R1_MEASURE, p, count);
	  }
      }
}

void
SM_init_sarray (unsigned int *sarray, unsigned int size)
/* Initializes the suffix array's index. */
{
    unsigned int p;

    for (p = 0; p < size; p++)
        sarray [p] = p;
}

void
SM_dump_string (unsigned int file, char *string, unsigned int p)
/* Dumps the string (of characters) to the file. */
{
    int cc, len;

    len = strlen (string + p);
    if (len < DUMP_STRING_SIZE)
        fprintf (Files [file], "%s\n", string + p);
    else
      {
	cc = string [p + DUMP_STRING_SIZE];
	string [p + DUMP_STRING_SIZE] = '\0';
	fprintf (Files [file], "%s\n", string + p);
	string [p + DUMP_STRING_SIZE] = cc;
      }
}

void
SM_dump_nstring (unsigned int file, unsigned int *string, unsigned int p)
/* Dumps the string (of numbers) to the file. */
{
    unsigned int q;

    q = p;
    fprintf (Files [file], "{");
    while (string [p] && (p - q < DUMP_NSTRING_SIZE))
      {
	if ((string [p] < 128) && (string [p] >= 32))
	    fputc (string [p], Files [file]);
	else
	    fprintf (Files [file], "<%d>", string [p]);
	p++;
      }
    fprintf (Files [file], " }\n");
    p = q;
    fprintf (Files [file], "{");
    while (string [p] && (p - q < DUMP_NSTRING_SIZE))
      {
	fprintf (Files [file], " %d:%d", p, string [p]);
	p++;
      }
    fprintf (Files [file], " }\n");
}

void
SM_dump_nstring1 (unsigned int file, unsigned int *string, unsigned int p, unsigned int len)
/* Dumps the string (of numbers) to the file. */
{
    unsigned int q;

    q = p;
    while (p < len)
      {
	if ((string [p] < 128) && (string [p] >= 32))
	    fputc (string [p], Files [file]);
	else
	    fprintf (Files [file], "<%d>", string [p]);
	p++;
      }
}

void
SM_dump_model_string (unsigned int file, struct SM_model_type *model, unsigned int p)
/* Dumps the string (of characters) to the file. */
{
    if (!model->numbers)
        SM_dump_string (file, model->string, p);
    else
        SM_dump_nstring (file, model->nstring, p);
}

int
SM_nstrcmp (unsigned int *string1, unsigned int *string2)
/* Compares the two arrays of unsigned ints. Returns zero if they are the same,
   negative if string1 < string2, and positive if string1 > string2. */
{
    unsigned int p;

    p = 0;
    for (p = 0; string1 [p] && string2 [p]; p++)
    {
      if (string1 [p] < string2 [p])
	  return (-1);
      else if (string1 [p] > string2 [p])
	  return (1);
    }
    if (!string2 [p])
      if (string1 [p])
	return (1);
      else
	return (0);
    else
	return (-1);
}

unsigned int
SM_nstrlen (unsigned int *string)
/* Returns the length of the string of numbers. */
{
    unsigned int len;

    len = 0;
    while (string [len])
        len++;

    return (len);
}

void
SM_check_sarray (unsigned int string_size, char *string, unsigned int *sarray)
/* Checks that the suffix array for a string of characters is sorted */
{
    unsigned int p, i, prev_i;
    char cc;

    prev_i = sarray [0];
    for (p = 1; p < string_size; p++)
      {
	i = sarray [p];

	if (strcmp (string + prev_i, string + i) > 0)
	  {
	    fprintf (stderr, "Error in suffix array at index %d :\n", p);

	    cc = string [prev_i + 40];
	    string [prev_i + 40] = '\0';
	    fprintf (stderr, "position %6d, string = %s\n",
		     prev_i, string + prev_i);
	    string [prev_i + 40] = cc;

	    cc = string [i + 40];
	    string [i + 40] = '\0';
	    fprintf (stderr, "position %6d, string = %s\n",
		     i, string + i);
	    string [i + 40] = cc;
	  }

	prev_i = i;
      }
}

void
SM_check_nsarray (unsigned int string_size, unsigned int *string,
		  unsigned int *sarray)
/* Checks that the suffix array for a string of numbers is sorted */
{
    unsigned int p, i, prev_i;
    char cc;

    prev_i = sarray [0];
    for (p = 1; p < string_size; p++)
      {
	i = sarray [p];

	if (SM_nstrcmp (string + prev_i, string + i) > 0)
	  {
	    fprintf (stderr, "Error in suffix array at index %d :\n", p);

	    cc = string [prev_i + 40];
	    string [prev_i + 40] = 0;
	    fprintf (stderr, "position %6d, string = ", prev_i);
	    SM_dump_nstring (Stderr_File, string, prev_i);
	    fprintf (stderr, "\n");
	    string [prev_i + 40] = cc;

	    cc = string [i + 40];
	    string [i + 40] = '\0';
	    fprintf (stderr, "position %6d, string = ", i);
	    SM_dump_nstring (Stderr_File, string, i);
	    fprintf (stderr, "\n");
	    string [i + 40] = cc;
	  }

	prev_i = i;
      }
}

static char *SM_String;           /* The string of characters */
static unsigned int *SM_nString;  /* The string of numbers (if model->numbers is true) */

int
SM_qsort_compare (const void *index1, const void *index2)
/* Comparison function (to compare strings of characters) used by qsort. */
{
  int p1, p2, cmp;

  /*fprintf (stderr, "Comparing %p with %p\n", index1, index2);*/
  p1 = *((unsigned int *) index1);
  p2 = *((unsigned int *) index2);
  /*fprintf (stderr, "Value     %d with %d\n", p1, p2);*/
  cmp = strcmp (SM_String + p1, SM_String + p2);
  if (!cmp)
    {
      if (p1 > p2)
	 return (1);
      if (p1 < p2)
	 return (-1);
    return 0;
    }

  return cmp;
}

int
SM_qsort_ncompare (const void *index1, const void *index2)
/* Comparison function (to compare strings of numbers) used by qsort. */
{
  int p1, p2, cmp;

  /*fprintf (stderr, "Comparing %p with %p\n", index1, index2);*/
  p1 = *((unsigned int *) index1);
  p2 = *((unsigned int *) index2);
  /*fprintf (stderr, "Value     %d with %d\n", p1, p2);*/
  cmp = SM_nstrcmp (SM_nString + p1, SM_nString + p2);
  if (!cmp)
    {
      if (p1 > p2)
	 return (1);
      if (p1 < p2)
	 return (-1);
    return 0;
    }

  return cmp;
}

unsigned int *
SM_make_sarray (unsigned int string_size, char *string)
/* Builds the suffix array in memory for a string of characters. */
{
    unsigned int *sarray;

    SM_String = string;

    sarray = (unsigned int *) malloc (string_size * sizeof (unsigned int));
    SM_init_sarray (sarray, string_size);

    qsort (sarray, string_size, sizeof (unsigned int), SM_qsort_compare);

    SM_check_sarray (string_size, string, sarray);

    return (sarray);
}

unsigned int *
SM_make_nsarray (unsigned int string_size, unsigned int *string)
/* Builds the suffix array in memory for a string of numbers. */
{
    unsigned int *sarray;

    SM_nString = string;

    sarray = (unsigned int *) malloc (string_size * sizeof (unsigned int));
    SM_init_sarray (sarray, string_size);

    qsort (sarray, string_size, sizeof (unsigned int), SM_qsort_ncompare);

    SM_check_nsarray (string_size, string, sarray);

    return (sarray);
}

void
SM_dump_sarray (unsigned int file, unsigned int string_size, char *string,
		unsigned int *sarray)
/* Dumps out the suffix array to the file (for debug purposes). */
{
    unsigned int p, i;

    for (p = 0; p < string_size; p++)
      {
	i = sarray [p];
	fprintf (Files [file], "%6d : %6d ", p, i);
	SM_dump_string (file, string, i);
      }
}

void
SM_dump_nsarray (unsigned int file, unsigned int string_size, unsigned int *string,
		 unsigned int *sarray)
/* Dumps out the suffix array to the file (for debug purposes). */
{
    unsigned int p, i;

    for (p = 0; p < string_size; p++)
      {
	i = sarray [p];
	fprintf (Files [file], "%6d : %6d ", p, i);
	SM_dump_nstring (file, string, i);
      }
}

void
SM_write_sarray (unsigned int file, unsigned int string_size,
		 char *string, unsigned int *sarray)
/* Writes out the suffix array of the string to the file. */
{
    /* Write string size at beginning of file */
    fwrite_int (file, string_size, sizeof (unsigned int));

    /* Write that it is a characters string */
    fwrite_int (file, FALSE, sizeof (unsigned int));

    /* Next write out the string. */
    fwrite (string, sizeof (char), string_size, Files [file]);

    /* Next write out the suffix array */
    fwrite_numbers (file, string_size, sarray);
}

void
SM_write_nsarray (unsigned int file, unsigned int string_size,
		  unsigned int *nstring, unsigned int *sarray)
/* Writes out the suffix array of the numbers nstring to the file. */
{
    /* Write string size at beginning of file */
    fwrite_int (file, string_size, sizeof (unsigned int));

    /* Write that it is a numbers string */
    fwrite_int (file, TRUE, sizeof (unsigned int));

    /* Next write out the string. */
    fwrite_numbers (file, string_size, nstring);

    /* Next write out the suffix array */
    fwrite_numbers (file, string_size, sarray);
}

struct SM_model_type *
SM_load_model (unsigned int file)
/* Builds the suffix model in memory by loading it from the file. */
{
    unsigned int *sarray, size;
    struct SM_model_type *model;
    unsigned int *nstring;
    boolean numbers;
    char *string;

    model = (struct SM_model_type *) malloc (sizeof (struct SM_model_type));
    
    size = fread_int (file, sizeof (unsigned int));
    assert (size > 0);
    model->size = size;

    numbers = fread_int (file, sizeof (unsigned int));
    model->numbers = numbers;
    if (!numbers)
      {
        string = (char *) malloc ((size+1) * sizeof (char));
	model->string = string;
	assert (model->string != NULL);
	model->nstring = NULL;

	assert (fread (model->string, sizeof (char), size, Files [file]));
      }
    else
      {
        nstring = (unsigned int *) malloc ((size+1) * sizeof (unsigned int));
	model->nstring = nstring;
	assert (model->nstring != NULL);
	model->string = NULL;

	fread_numbers (file, size, nstring);
      }

    sarray = (unsigned int *) malloc (size * sizeof (unsigned int));
    model->sarray = sarray;
    assert (model->sarray != NULL);
    SM_init_sarray (sarray, size);

    fread_numbers (file, size, sarray);
    if (!model->numbers)
      {
	assert (model->string != NULL);
	assert (model->nstring == NULL);
	SM_check_sarray (model->size, model->string, model->sarray);
      }
    else
      {
	assert (model->string == NULL);
	assert (model->nstring != NULL);
	SM_check_nsarray (model->size, model->nstring, model->sarray);
      }

    return (model);
}

unsigned int
SM_get_strie_symbol (struct SM_model_type *model, struct SM_trie_type *node,
		     unsigned int spos)
/* Gets the symbol in the strie node. */
{
    unsigned int symbol;

    if (node == NULL)
        return (0);
    else
      {
	if (node->slen == 0)
	    return (node->symbol);
	else /* non-branching pathway */
	  {
	    if (!model->numbers)
	        symbol = *((char *) node->sptr + spos);
	    else
		symbol = *((unsigned int *) node->sptr + spos);

	    /*fprintf (stderr, "pos = %d symbol = %c\n", spos, symbol);*/
	    return symbol;
	  }
      }
}

void
SM_init_strie_context (struct SM_model_type *model,
		       struct SM_trie_type *trie,
		       struct SM_strie_context_type *context)
/* Initialises the context for looking at the suffix array */
{
    assert (model != NULL);
    assert (context != NULL);

    context->node = trie;
    context->depth = 0;
    context->spos = 0;
    if (trie == NULL)
      {
        context->symbol = 0;
	context->parent_count = 0;
      }
    else
      {
        context->symbol = trie->symbol;
	context->parent_count = model->size;
      }
}

void
SM_dump_strie_context (unsigned int file, char *str,
		       struct SM_model_type *model,
		       struct SM_strie_context_type *context)
/* Dumps the strie context. */
{
    struct SM_trie_type *ptr;

    assert (model != NULL);
    assert (context != NULL);

    fprintf (Files [file], "Strie context %s: depth %d spos %d symbol %d node %p\n",
	     str, context->depth, context->spos, context->symbol,
	     (void *) context->node);
    if (context->node != NULL)
      {
	ptr = context->node;
	fprintf (Files [file], "Strie node: symbol %d count %d slen %d sptr %p child %p\n",
		 ptr->symbol, ptr->count, ptr->slen, (void *) ptr->sptr,
		 (void *) ptr->child);
      }
}

void
SM_check_strie_context (struct SM_model_type *model,
			struct SM_strie_context_type *context)
/* Checks the context for inconsistencies (for debugging purposes). */
{
    struct SM_trie_type *ptr;

    assert (model != NULL);
    assert (context != NULL);

    if (context->symbol >= 256)
      {
	fprintf (stderr, "*** Error in strie context:\n");
        SM_dump_strie_context (Stderr_File, "[check strie]", model, context);
      }
    assert (context->symbol < 256);

    ptr = context->node;
    if (ptr == NULL)
        return;

    if (ptr->sptr != NULL)
      {
	if ((ptr->slen == 0) && (ptr->sptr->symbol >= 256))
	  {
	    fprintf (stderr, "*** Error in strie context:\n");
	    SM_dump_strie_context (Stderr_File, "[check strie/sptr]",
				   model, context);
	  }
	assert (!((ptr->slen == 0) && (ptr->sptr->symbol >= 256)));
      }

    if (ptr->child != NULL)
      {
	if (ptr->child->symbol >= 256)
	  {
	    fprintf (stderr, "*** Error in strie context:\n");
	    SM_dump_strie_context (Stderr_File, "[check strie/child]",
				   model, context);
	  }
	assert (ptr->child->symbol < 256);
      }
}

unsigned int
SM_get_strie_context (struct SM_model_type *model,
		      struct SM_strie_context_type *context)
/* Gets the current symbol for the model and context. */
{
    assert (model != NULL);
    assert (context != NULL);

    return (context->symbol);
}

boolean
SM_isleaf_strie_context (struct SM_model_type *model,
			 struct SM_strie_context_type *context)
/* Returns TRUE if the strie context points at a leaf node.
   Assumes that we are not in a non-branching pathway. */
{
    struct SM_trie_type *ptr;
    unsigned int spos;

    assert (model != NULL);
    assert (context != NULL);
    ptr = context->node;
    spos = context->spos;

    if (ptr == NULL)
      return (TRUE);
    else if (ptr->child != NULL) /* All leaves must have no childs */
      return (FALSE);
    else if (spos+1 < ptr->slen)
      return (FALSE);
    else
      return (TRUE);
}

boolean
SM_next_strie_context (struct SM_model_type *model,
		       struct SM_strie_context_type *context)
/* Moves the context along to the next sibling. */
{
    struct SM_trie_type *ptr;

    assert (model != NULL);
    assert (context != NULL);

    SM_check_strie_context (model, context);

    ptr = context->node;

    context->symbol = 0;

    if (ptr == NULL)
      return (FALSE);

    if (ptr->slen != 0)
      { /* Non-branching pathway */
	/* move the context pointer onto next sibling; as there is only
	   one sibling in a non-branching path, there isn't a next sibling */
	context->node = NULL; 
		  
	return (FALSE);
      }
    else if (ptr->sptr == NULL)
      { /* No more siblings */
	context->node = NULL;
	context->symbol = 0;

	return (FALSE);
      }
    else
      {
	context->node = ptr->sptr; /* Moves the context pointer along to
				      sibling node */
	context->symbol = SM_get_strie_symbol (model, ptr->sptr, 0);

	return (TRUE);
      }
}

boolean
SM_down_strie_context (struct SM_model_type *model,
		       struct SM_strie_context_type *context,
		       struct SM_strie_context_type *new_context,
		       struct SM_model_type *model1,
		       struct SM_strie_context_type *context1,
		       struct SM_strie_context_type *new_context1)
/* Goes "down" the suffix tries given the current strie contexts. */
{
    struct SM_trie_type *ptr, *ptr1;
    unsigned int spos, spos1;
    boolean finish;

    assert (model != NULL);
    assert (context != NULL);
    assert (new_context != NULL);

    assert (model1 != NULL);
    assert (context1 != NULL);
    assert (new_context1 != NULL);

    ptr = context->node;
    spos = context->spos;

    new_context->node = ptr;
    new_context->depth = context->depth;
    new_context->spos = spos;
    new_context->symbol = 0;

    ptr1 = context1->node;
    spos1 = context1->spos;

    new_context1->node = ptr1;
    new_context1->depth = context1->depth;
    new_context1->spos = spos1;
    new_context1->symbol = 0;

    if (ptr == NULL)
        new_context->parent_count = 0;
    if (ptr1 == NULL)
        new_context1->parent_count = 0;
    if ((ptr == NULL) || (ptr1 == NULL))
        return (FALSE);

    for (;;)
      { /* Keep on going down until end of non-branching path
	   or symbols do not match */
	new_context->depth++;
	new_context->parent_count = ptr->count;

	new_context1->depth++;
	new_context1->parent_count = ptr1->count;

	finish = FALSE;

	/* Proceed down trie */
	if (spos+1 < ptr->slen)
	  { /* In non-branching pathway */
	    new_context->spos = spos + 1;
	    new_context->symbol = SM_get_strie_symbol (model, ptr, spos + 1);
	  }
	else
	  {
	    ptr = ptr->child;
	    new_context->node = ptr;
	    new_context->spos = 0;
	    new_context->symbol = SM_get_strie_symbol (model, ptr, 0);
	    finish = TRUE;
	  }

	/* Proceed down trie1 */
	if (spos1+1 < ptr1->slen)
	  { /* In non-branching pathway */
	    new_context1->spos = spos1 + 1;
	    new_context1->symbol = SM_get_strie_symbol (model, ptr1, spos1 + 1);
	  }
	else
	  {
	    ptr1 = ptr1->child;
	    new_context1->node = ptr1;
	    new_context1->spos = 0;
	    new_context1->symbol = SM_get_strie_symbol (model, ptr1, 0);
	    finish = TRUE;
	  }

	if (finish || !new_context->symbol || !new_context1->symbol ||
	    (new_context->symbol != new_context1->symbol))
	    break;

	spos++;
	spos1++;

	SM_increment_count (COUNT_C_MEASURE, new_context->depth+1, ptr1->count);
	
	/*
	fprintf (stderr, "0: depth = %d count = %d\n",
		 new_context->depth+1, ptr1->count);
	*/
	
	SM_increment_count (COUNT_C1_MEASURE, new_context->depth+1, 1);

      }

    return ((ptr != NULL) && (ptr1 != NULL));
}
 
int
SM_compare_strie_context (struct SM_model_type *model,
			  struct SM_strie_context_type *context,
			  struct SM_model_type *model1,
			  struct SM_strie_context_type *context1)
/* Returns -1, 0, 1 by comparing the symbols for the different contexts
   (similar to strcmp). */
{
    struct SM_trie_type *ptr, *ptr1;
    boolean eos, eos1;

    eos = FALSE;
    eos1 = FALSE;

    assert (model != NULL);
    assert (context != NULL);

    assert (model1 != NULL);
    assert (context1 != NULL);

    for (;;)
      { /* Repeat until a match is found or end of symbols' list */
	ptr = context->node;
	ptr1 = context1->node;

	if (ptr == NULL)
	  {
	    if (ptr1 == NULL)
	      return (0);
	    else
	      return (-1);
	  }
	if (ptr1 == NULL)
	  {
	    return (1);
	  }

	if (context->symbol == context1->symbol)
	    return (0);
	else if (context->symbol < context1->symbol)
	    return (-1);
	else
	    return (1);
      }
}

void
SM_dump_symbol (unsigned int file, unsigned int symbol, boolean nbflag)
/* Dump the symbol. */
{
    if ((symbol <= 32) || (symbol >= 127))
	fprintf (Files [file], "<%d>", symbol);
    else
	fprintf (Files [file], "%c", symbol);
    if (!nbflag)
	fprintf (Files [file], "'");
}

void
SM_dump_symbol1 (unsigned int file, unsigned int symbol)
/* Dump the symbol. */
{
    if ((symbol <= 32) || (symbol >= 127))
	fprintf (Files [file], "<%d>", symbol);
    else
	fprintf (Files [file], "%c", symbol);
}

unsigned int *SM_dump_symbols = NULL;
boolean *SM_dump_nbflag = NULL;

unsigned int
SM_get_symbol (struct SM_model_type *model, struct SM_trie_type *ptr, unsigned int pos)
/* gets the symbol from a non-branching path. */
{
    unsigned int symbol;

    if (!model->numbers)
        symbol = *((char *) ptr->sptr + pos);
    else
        symbol = *((unsigned int *) ptr->sptr + pos);

    return (symbol);
}

void
SM_dump_trie1 (unsigned int file, struct SM_model_type *model,
	       struct SM_trie_type *ptr, unsigned int pos,
	       boolean dump_all_nodes, boolean dump_ptrs)
/* Recursive routine to dump trie model called by SM_dump_trie. */
{
    unsigned int p, q, pos1;

    while (ptr != NULL)
      { /* proceed through the symbol list */

	if (!dump_all_nodes)
	  { /* Display only leaf nodes */
	    if (ptr->count == 1)
	      {
		for (p = 0; p < pos; p++)
		    SM_dump_symbol (file, SM_dump_symbols [p], TRUE);

		if (!ptr->slen)
		    SM_dump_symbol (file, ptr->symbol, TRUE);
		else /* found non-branching path node */
		  for (q = 0; q < ptr->slen; q++)
		    SM_dump_symbol (file, SM_get_symbol (model, ptr, q), TRUE);
		fprintf (Files [file], "\n");
	      }
	  }
	else
	  {
	    if (!dump_ptrs)
	        fprintf (Files [file], "%4d %6d [", pos, ptr->slen);
	    else
	        fprintf (Files [file], "%4d %9p %9p %9p %6d [", pos, (void *) ptr,
			 (void *) ptr->sptr, (void *) ptr->child, ptr->slen);
	    for (p = 0; p < pos; p++)
	        SM_dump_symbol (file, SM_dump_symbols [p], SM_dump_nbflag [p]);

	    if (!ptr->slen)
	        SM_dump_symbol (file, ptr->symbol, FALSE);
	    else /* found non-branching path node */
	      for (q = 0; q < ptr->slen; q++)
		SM_dump_symbol (file, SM_get_symbol (model, ptr, q), TRUE);

	    fprintf (Files [file], "] count = %d\n", ptr->count);
	  }

	if (ptr->child != NULL)
	  {
	    if (!ptr->slen)
	      {
		SM_dump_symbols [pos] = ptr->symbol;
	        SM_dump_nbflag [pos] = FALSE;
		pos1 = pos + 1;
	      }
	    else
	      {
		for (q = 0; q < ptr->slen; q++)
		  {
		    SM_dump_symbols [pos + q] = SM_get_symbol (model, ptr, q);
		    SM_dump_nbflag [pos + q] = TRUE;
		  }
		pos1 = pos + ptr->slen;
	      }

	    SM_dump_trie1 (file, model, ptr->child, pos1,
			   dump_all_nodes, dump_ptrs);
	  }
	if (ptr->slen)
	    ptr = NULL; /* No symbol list if non-branching pathway; sptr points
			   to string position instead */
	else
	    ptr = ptr->sptr;
      }
}

void
SM_dump_trie (unsigned int file, struct SM_model_type *model, struct SM_trie_type *trie,
	      boolean dump_all_nodes, boolean dump_ptrs)
/* Recursive routine to dump trie model. */
{
    assert (model != NULL);

    if (Debug_level > 1)
        fprintf (Files [file], "Dump of trie: (depth, node ptr, sptr, child ptr, slen, path, count)\n");

    SM_dump_symbols = (unsigned int *)
        realloc (SM_dump_symbols, (model->size+1) * sizeof (unsigned int));
    SM_dump_nbflag = (unsigned int *)
        realloc (SM_dump_nbflag, (model->size+1) * sizeof (unsigned int));

    SM_dump_trie1 (file, model, trie, 0, dump_all_nodes, dump_ptrs);
}

void
SM_dump_model (unsigned int file, struct SM_model_type *model,
	       struct SM_trie_type *trie,
	       boolean dump_all_nodes, boolean dump_ptrs,
	       char *ident)
/* Dumps out the model (for devbugging purposes). */
{
    fprintf (Files [file], "\nDump of model %s: ", ident);
    if (model == NULL)
        fprintf (Files [file], "\n");
    else
      {
        fprintf (Files [file], "[size = %d]\n", model->size );
	SM_dump_trie (file, model, trie, dump_all_nodes, dump_ptrs);
      }
}

void
SM_check_trie1 (struct SM_model_type *model,
		struct SM_trie_type *ptr, unsigned int pos,
		unsigned int parent_count)
/* Recursive routine to check suffix trie called by SM_check_trie. */
{
    unsigned int pos1, count;

    count = 0;
    while (ptr != NULL)
      { /* proceed through the symbol list */
	count += ptr->count;

	if (ptr->child != NULL)
	  {
	    if (!ptr->slen)
		pos1 = pos + 1;
	    else
		pos1 = pos + ptr->slen;

	    SM_check_trie1 (model, ptr->child, pos1, ptr->count);
	  }

	if (ptr->slen)
	    ptr = NULL; /* No symbol list if non-branching pathway; sptr points
			   to string position instead */
	else
	    ptr = ptr->sptr;
      }

    if (count != parent_count)
      fprintf (stderr, "Count mismatch at depth %d parent count %d count %d\n",
	       pos, parent_count, count);
}

void
SM_check_trie (struct SM_model_type *model, struct SM_trie_type *trie)
/* Recursive routine to check the suffix trie for consistent counts. */
{
    assert (model != NULL);

    SM_check_trie1 (model, trie, 0, model->size);
}

struct SM_trie_type *
SM_create_node (unsigned int symbol, unsigned int count, char *sptr, unsigned int slen)
{
    struct SM_trie_type *node;

    node = (struct SM_trie_type *) malloc (sizeof (struct SM_trie_type));
    node->symbol = symbol;
    node->count = count;
    node->sptr = (struct SM_trie_type *) sptr;
    node->child = NULL;
    node->slen = slen;

    return (node);
}

struct SM_trie_type *
SM_grow_ctrie (struct SM_model_type *model)
/* Builds the suffix trie given the suffix array model of a string of charcters. */
{
    struct SM_trie_type *newnode, *root, *ptr, *pptr;
    unsigned int ppos, i, prev_i, sinc, slen, count;
    unsigned int symbol, prev_symbol, sym;
    char *sptr;
    boolean nfound;
    int pos;

    assert (model != NULL);
    assert (model->size > 0);

    root = NULL;

    prev_i = model->size;
    /* Process suffix array in reverse order to ensure suffixes in suffix trie
       are sorted in ascending order */
    for (pos = model->size-1; pos >= 0; pos--)
      {
	i = model->sarray [pos];

	if ((Debug_level > 4) ||
	    (Debug_progress && ((pos % Debug_progress) == 0)))
	    fprintf (stderr, "*** Processing string pos %d, index %d\n", pos, i);

	if (Debug_level > 4)
	    SM_dump_string (Stderr_File, model->string, i);

	ppos = 0;
	sinc = 0;
	ptr = root;
	pptr = NULL;
	sptr = NULL;
	nfound = FALSE;
	for (ppos = 0;; ppos++)
	  {
	    if (i + ppos >= model->size)
	        symbol = '\0';
	    else
	        symbol = model->string [i + ppos];

	    if (prev_i + ppos >= model->size)
	        prev_symbol = '\0';
	    else
	        prev_symbol = model->string [prev_i + ppos];

	    if (ptr == NULL)
	      {
		nfound = TRUE;
	      }
	    else if (nfound)
	      {
		ptr->slen++;
		ptr->sptr = (struct SM_trie_type *) sptr; /* Now store saved string ptr */
	      }

	    if (symbol != prev_symbol)
	        break;

	    if (!ptr || (ptr->slen == 0))
	        pptr = ptr;
	    if (ptr && !nfound)
	      { /* Increment position in non-branching pathway */
		if (ptr->slen == 0)
		    sinc = 0;
		else
		    sinc++;
		if (!sinc || (sinc >= ptr->slen))
		  {
		    pptr = ptr;
		    ptr->count++;
		    ptr = ptr->child;
		    sinc = 0;
		  }
	      }	    
	  }

	/* Create new leaf node and add to siblings list as well */
	if (nfound)
	  {
	    if (pptr == NULL)
	        ptr = NULL;
	    else
	      { /* Don't add node for prev_symbol for first string to be added */
		newnode = SM_create_node (prev_symbol, 1, NULL, 0);
		ptr = newnode;
	      }
	  }
	else if (sinc > 0)
	  { /* Split non-branching pathway node */
	    /*fprintf (stderr, "slen = %d sinc = %d\n", ptr->slen, sinc);*/

	    slen = ptr->slen;
	    sptr = (char *) ptr->sptr;

	    count = ptr->count;

	    /* Overwrite old node to build a new right node */
	    if (slen - sinc <= 1)
	      { /* Length of right node is 1; i.e. no longer non-branching pathway of
		   multiple nodes*/
		ptr->symbol = *(sptr + sinc);
		ptr->sptr = NULL;
		ptr->slen = 0;
	      }
	    else
	      {
		ptr->sptr = (struct SM_trie_type *) (sptr + sinc + 1);
		ptr->slen = slen - sinc - 1;

		/* Create new middle node to whose symbol's list we add the new symbol latter */
		/* Also set "ptr" for new symbol to be added next at the head of the symbol list */
		sym = *(sptr + sinc);
		newnode = SM_create_node (sym, count, NULL, 0);
		newnode->child = ptr;
		ptr = newnode;
	      }

	    /* Create left node, and update parent's child pointer */
	    if (sinc == 1) /* No longer non-branching pathway */
	      {
		sym = *(sptr);
		newnode = SM_create_node (sym, count+1, NULL, 0);
	      }
	    else
	      {
		sym = *sptr;
		newnode = SM_create_node (sym, count+1, sptr, sinc);
	      }
	    newnode->child = ptr;
	    pptr->child = newnode;
	    pptr = newnode;
	  }

	/* Now add the new symbol at the head of the node ptr's symbol list */
	newnode = SM_create_node (symbol, 1, NULL, 0);
	newnode->sptr = ptr;
	if (pptr == NULL)
	    root = newnode; /* First node (head) in the trie */
	else
	    pptr->child = newnode;

	/* Now add a child pointer to the newly added node which contains
	   a non-branching path until the end of the string */
	if (i + ppos + 1 < model->size) /* Only add non-branching path to end of string
					   if we are not already at the end of the string */
	  { 
	    pptr = newnode;
	    symbol = 0; /* As node is a non-brancing path node, this symbol is ignored */
	    sptr = model->string + i + ppos + 1;
	    newnode = SM_create_node (symbol, 1, sptr, model->size - i - ppos - 1);
	    pptr->child = newnode;
	  }

	if (Debug_level > 4)
	    SM_dump_model (Stderr_File, model, root, TRUE, FALSE, "{Grow}");

	prev_i = i;
      }

    return (root);
}

struct SM_trie_type *
SM_grow_ntrie (struct SM_model_type *model)
/* Builds the suffix trie given the suffix array model of a string of unsigned int numbers. */
{
    struct SM_trie_type *newnode, *root, *ptr, *pptr;
    unsigned int ppos, i, prev_i, sinc, slen, count;
    unsigned int symbol, prev_symbol, sym;
    unsigned int *nsptr;
    boolean nfound;
    int pos;

    assert (model != NULL);
    assert (model->size > 0);

    root = NULL;

    prev_i = model->size;
    /* Process suffix array in reverse order to ensure suffixes in suffix trie
       are sorted in ascending order */
    for (pos = model->size-1; pos >= 0; pos--)
      {
	i = model->sarray [pos];

	if ((Debug_level > 4) ||
	    (Debug_progress && ((pos % Debug_progress) == 0)))
	    fprintf (stderr, "*** Processing string pos %d, index %d\n", pos, i);

	if (Debug_level > 4)
	    SM_dump_string (Stderr_File, model->string, i);

	ppos = 0;
	sinc = 0;
	ptr = root;
	pptr = NULL;
	nsptr = NULL;
	nfound = FALSE;
	for (ppos = 0;; ppos++)
	  {
	    if (i + ppos >= model->size)
	        symbol = '\0';
	    else
	        symbol = model->nstring [i + ppos];

	    if (prev_i + ppos >= model->size)
	        prev_symbol = '\0';
	    else
	        prev_symbol = model->nstring [prev_i + ppos];

	    if (ptr == NULL)
	      {
		nfound = TRUE;
	      }
	    else if (nfound)
	      {
		ptr->slen++;
		ptr->sptr = (struct SM_trie_type *) nsptr; /* Now store saved string ptr */
	      }

	    if (symbol != prev_symbol)
	        break;

	    if (!ptr || (ptr->slen == 0))
	        pptr = ptr;
	    if (ptr && !nfound)
	      { /* Increment position in non-branching pathway */
		if (ptr->slen == 0)
		    sinc = 0;
		else
		    sinc++;
		if (!sinc || (sinc >= ptr->slen))
		  {
		    pptr = ptr;
		    ptr->count++;
		    ptr = ptr->child;
		    sinc = 0;
		  }
	      }	    
	  }

	/* Create new leaf node and add to siblings list as well */
	if (nfound)
	  {
	    if (pptr == NULL)
	        ptr = NULL;
	    else
	      { /* Don't add node for prev_symbol for first string to be added */
		newnode = SM_create_node (prev_symbol, 1, NULL, 0);
		ptr = newnode;
	      }
	  }
	else if (sinc > 0)
	  { /* Split non-branching pathway node */
	    /*fprintf (stderr, "slen = %d sinc = %d\n", ptr->slen, sinc);*/

	    slen = ptr->slen;
	    nsptr = (unsigned int *) ptr->sptr;

	    count = ptr->count;

	    /* Overwrite old node to build a new right node */
	    if (slen - sinc <= 1)
	      { /* Length of right node is 1; i.e. no longer non-branching pathway of
		   multiple nodes*/
		ptr->symbol = *(nsptr + sinc);
		ptr->sptr = NULL;
		ptr->slen = 0;
	      }
	    else
	      {
		ptr->sptr = (struct SM_trie_type *) (nsptr + sinc + 1);
		ptr->slen = slen - sinc - 1;

		/* Create new middle node to whose symbol's list we add the new symbol latter */
		/* Also set "ptr" for new symbol to be added next at the head of the symbol list */
		sym = *(nsptr + sinc);
		newnode = SM_create_node (sym, count, NULL, 0);
		newnode->child = ptr;
		ptr = newnode;
	      }

	    /* Create left node, and update parent's child pointer */
	    if (sinc == 1) /* No longer non-branching pathway */
	      {
		sym = *(nsptr);
		newnode = SM_create_node (sym, count+1, NULL, 0);
	      }
	    else
	      {
		sym = *nsptr;
		newnode = SM_create_node (sym, count+1, (char *) nsptr, sinc);
	      }
	    newnode->child = ptr;
	    pptr->child = newnode;
	    pptr = newnode;
	  }

	/* Now add the new symbol at the head of the node ptr's symbol list */
	newnode = SM_create_node (symbol, 1, NULL, 0);
	newnode->sptr = ptr;
	if (pptr == NULL)
	    root = newnode; /* First node (head) in the trie */
	else
	    pptr->child = newnode;

	/* Now add a child pointer to the newly added node which contains
	   a non-branching path until the end of the string */
	if (i + ppos + 1 < model->size) /* Only add non-branching path to end of string
					   if we are not already at the end of the string */
	  { 
	    pptr = newnode;
	    symbol = 0; /* As node is a non-brancing path node, this symbol is ignored */
	    nsptr = model->nstring + i + ppos + 1;
	    newnode = SM_create_node (symbol, 1, (char *) nsptr, model->size - i - ppos - 1);

	    pptr->child = newnode;
	  }

	if (Debug_level > 4)
	    SM_dump_model (Stderr_File, model, root, TRUE, FALSE, "{Grow}");

	prev_i = i;
      }

    return (root);
}

struct SM_trie_type *
SM_grow_trie (struct SM_model_type *model)
/* Builds the suffix trie given the suffix array model. */
{
    if (!model->numbers)
        return (SM_grow_ctrie (model));
    else
        return (SM_grow_ntrie (model));
}

unsigned int *SM_traverse_path = NULL;           /* Symbols in trie path being traversed */
unsigned int *SM_traverse_suffix = NULL;         /* Current suffix being traversed */
unsigned int SM_traverse_suffix_length = 0;      /* Length of current suffix (chars) */
unsigned int SM_traverse_suffix_count = 0;       /* Count associated with current suffix */
unsigned int *SM_traverse_prev_suffix = NULL;    /* Previous suffix that was just traversed */
unsigned int SM_traverse_prev_suffix_length = 0; /* Length of previous suffix (#chars) */

boolean
SM_traverse_same_prefix (unsigned int prefix_length)
/* Returns TRUE if the current char suffix and the previous char suffix being traversed in the
   suffix model has the same prefix of length prefix_length. */
{
    unsigned int p;
    
    if ((SM_traverse_suffix_length < prefix_length) ||
	(SM_traverse_prev_suffix_length < prefix_length))
        return (FALSE);

    for (p = 0; p < prefix_length; p++)
      if (SM_traverse_prev_suffix [p] != SM_traverse_suffix [p])
	return (FALSE);

    return (TRUE);
}

void
SM_traverse_trie1 (struct SM_model_type *model, struct SM_trie_type *ptr,
		   unsigned int pos, boolean (*suffix_match_function) (unsigned int, unsigned int *))
/* Recursive routine to traverse the suffix trie model called by SM_traverse_trie. */
{
    unsigned int p, q, pos1, suffix_len;
    boolean match;

    while (ptr != NULL)
      { /* proceed through the symbol list */

	q = 0;
	for (p = 0; p < pos; p++)
	  {
	    SM_traverse_suffix [p] = SM_traverse_path [p];
	  }

	if (!ptr->slen)
	    SM_traverse_suffix [p++] = ptr->symbol;
	else /* found non-branching path node */
	    for (q = 0; q < ptr->slen; q++)
	      SM_traverse_suffix [p++] = SM_get_symbol (model, ptr, q);
	suffix_len = p;

	if (Debug_level > 2)
	  { /* This prints out each suffix */
	    fprintf (stderr, "suffix [");
	    SM_dump_nstring1 (Stderr_File, SM_traverse_suffix, 0, suffix_len);
	    fprintf (stderr, "] count = %d\n", ptr->count);
	  }

	SM_traverse_suffix_length = suffix_len;
	SM_traverse_suffix_count = ptr->count;
	match = suffix_match_function (suffix_len, SM_traverse_suffix);

	/* Copy current suffix to be previous suffix for future match */
	for (p = 0; p < suffix_len; p++)
	    SM_traverse_prev_suffix [p] = SM_traverse_suffix [p];
	SM_traverse_prev_suffix_length = suffix_len;

	if (ptr->child != NULL)
	  {
	    if (!ptr->slen)
	      {
		SM_traverse_path [pos] = ptr->symbol;
		pos1 = pos + 1;
	      }
	    else
	      {
		for (q = 0; q < ptr->slen; q++)
		    SM_traverse_path [pos + q] = SM_get_symbol (model, ptr, q);
		pos1 = pos + ptr->slen;
	      }

	    SM_traverse_trie1 (model, ptr->child, pos1, suffix_match_function);
	  }
	if (ptr->slen)
	    ptr = NULL; /* No symbol list if non-branching pathway; sptr points
			   to string position instead */
	else
	    ptr = ptr->sptr;
      }
}

void
SM_traverse_trie (struct SM_model_type *model, struct SM_trie_type *trie,
		  boolean (*suffix_match_function) (unsigned int, unsigned int *))
/* Recursive routine to traverse the trie model.

   The suffix match function is a user defined function that returns TRUE if the
   suffix matches some criteria as defined by the function (e.g. that it contains
   at least 5 characters, say, or two words). It takes two parameters - the first
   is an unsigned int that is the length of the suffix, and the second is an
   unsigned int * that is a pointer to the suffix itself (and array of unsigned ints). */
{
    assert (model != NULL);
    assert (suffix_match_function != NULL);

    SM_traverse_path = (unsigned int *)
        realloc (SM_traverse_path, (model->size+1) * sizeof (unsigned int));
    SM_traverse_suffix = (unsigned int *)
        realloc (SM_traverse_suffix, (model->size+1) * sizeof (unsigned int));
    SM_traverse_prev_suffix = (unsigned int *)
        realloc (SM_traverse_prev_suffix, (model->size+1) * sizeof (unsigned int));
    
    SM_traverse_prev_suffix_length = 0; /* Ensure that first suffix does not match */
    SM_traverse_prev_suffix [0] = 0; 

    SM_traverse_trie1 (model, trie, 0, suffix_match_function);
}

unsigned int
SM_cotraverse_trie1 (struct SM_model_type *model, struct SM_strie_context_type *context,
		     struct SM_model_type *model1, struct SM_strie_context_type *context1)
/* Recursive routine to calculate various measures called by SM_cotraverse_trie. */
{
    struct SM_strie_context_type new_context, new_context1;
    struct SM_trie_type *ptr, *ptr1;
    boolean nomore;
    unsigned int count, cnt, cnt1;
    int comp;

    assert (context->depth == context1->depth);

    cnt = 0;
    cnt1 = 0;

    count = 0;
    for (;;) /* proceed through trie1's and trie's symbol list */
    {
      ptr = context->node;
      ptr1 = context1->node;

      if ((ptr == NULL) || (ptr1 == NULL))
	  break;

      if (Debug_level > 3)
	{

	  fprintf (stderr, "depth  = %d symbol = %d count  = %d parent count  = %d\n",
		   context->depth, context->symbol, ptr->count, context->parent_count);
	  fprintf (stderr, "depth1 = %d symbol = %d count1 = %d parent count1 = %d\n",
		   context1->depth, context1->symbol, ptr1->count, context1->parent_count);
	}

      if ((comp = SM_compare_strie_context
	   (model, context, model1, context1)))
	{ /* Symbols do not match */
	  {
	    count += 2 * context->depth;

	    if (Debug_level > 2)
	      {
		if (ptr == NULL)
		    fprintf (stderr, "Count = 1 ");
		else
		    fprintf (stderr, "Count = %d ", ptr->count);

		fprintf (stderr, "depth = %d comp = %d\n",
			 context->depth, comp);
	      }
	  }
	}
      else
	{ /* Symbols match */

	  SM_increment_count (COUNT_C_MEASURE, context->depth+1, ptr1->count);
	  
	  /*
	  fprintf (stderr, "1: depth = %d count = %d\n",
		   context->depth+1, ptr1->count);
	  */
	  
	  SM_increment_count (COUNT_C1_MEASURE, context->depth+1, 1);

	  if (Debug_level > 2)
	    {
	      fprintf (stderr, "Symbols ");
	      SM_dump_symbol (Stderr_File, context->symbol, TRUE);
	      fprintf (stderr, " match at depth %d\n", context->depth+1);
	    }

	  if (SM_isleaf_strie_context (model, context))
	    {
	      count += context->depth + 1;

	      if (Debug_level > 2)
		{
		  fprintf (stderr, "Leaf count = %d, depth = %d\n",
			   ptr->count, context->depth + 1);
		}
	    }
		
	  if (SM_isleaf_strie_context (model1, context1))
	    {
	      count += context->depth + 1;

	      if (Debug_level > 2)
		{
		  fprintf (stderr, "Leaf count 1 = %d, depth = %d\n",
			   ptr->count, context->depth + 1);
		}
	    }

	  if (SM_down_strie_context
	      (model, context, &new_context, model1, context1, &new_context1))
	      count += SM_cotraverse_trie1
		  (model, &new_context, model1, &new_context1);
	}

      nomore = FALSE;
      if (context->symbol < context1->symbol)
	{
	  for (;;)
	  {
	    cnt += context->node->count;
	    if (!SM_next_strie_context (model, context))
	      {
		nomore = TRUE;
		break;
	      }
	    if (context->symbol >= context1->symbol)
	        break;
	  }
	}
      else if (context->symbol > context1->symbol)
	{
	  for (;;)
	  {
	    cnt1 += context1->node->count;
	    if (!SM_next_strie_context (model1, context1))
	      {
		nomore = TRUE;
	        break;
	      }
	    if (context->symbol <= context1->symbol)
	        break;
	  }
	}
      else
	{ /* symbols are equal - move on to next one */
	  boolean flag, flag1;

	  cnt += context->node->count;
	  cnt1 += context1->node->count;

	  flag = !SM_next_strie_context (model, context);
	  flag1 = !SM_next_strie_context (model1, context1);
	  if (flag && flag1)
	      nomore = TRUE;
	}

      if (nomore)
	  break;
    }

    /* Skip over remaining unmatched symbols if there are any */
    if (context->node != NULL)
      {
	do
	  cnt += context->node->count;
	while (SM_next_strie_context (model, context));
      }
    if (context1->node != NULL)
      {
	do
	  cnt1 += context1->node->count;
	while (SM_next_strie_context (model1, context1));
      }

    if (Debug_level > 3)
      {
        fprintf (stderr, "model 0: depth = %d count  = %d parent count  = %d",
		 context->depth, cnt, context->parent_count);
	if (cnt != context->parent_count)
	    fprintf (stderr, " ***");
	fprintf (stderr, "\n");

	fprintf (stderr, "model 1: depth = %d count  = %d parent count  = %d",
		 context1->depth, cnt1, context1->parent_count);
	if (cnt1 != context1->parent_count)
	    fprintf (stderr, " ***");
	fprintf (stderr, "\n");
      }

    assert (cnt == context->parent_count);
    assert (cnt1 == context1->parent_count);

    return (count);
}

unsigned int
SM_cotraverse_trie (struct SM_model_type *model, struct SM_trie_type *trie,
		  struct SM_model_type *model1, struct SM_trie_type *trie1)
/* Recursive routine to calculate various measures  given the two suffix tries - trie,
   the training model, and trie1, the testing model. */
{
    struct SM_strie_context_type context;
    struct SM_strie_context_type context1;

    assert (model != NULL);
    assert (model1 != NULL);

    SM_init_strie_context (model, trie, &context);
    SM_init_strie_context (model1, trie1, &context1);
    
    return (SM_cotraverse_trie1 (model, &context, model1, &context1));
}
