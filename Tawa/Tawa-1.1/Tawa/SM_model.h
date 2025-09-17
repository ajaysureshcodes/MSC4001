/* Routines for updating Suffix Models. */

#ifndef SM_MODEL_H
#define SM_MODEL_H

extern unsigned int Debug_level;
extern unsigned int Debug_progress;

struct SM_model_type
{ /* Suffix array model record */
  boolean numbers;               /* TRUE if string is sequence of unsigned ints;
				    FALSE if string is a sequence of charaters */ 
  unsigned int size;             /* The size of the string & suffix array */
  char *string;                  /* The input string (as string of characters) */
  unsigned int *nstring;         /* The input string (as string of numbers) */
  unsigned int *sarray;          /* The suffix array list of indexes */
};

struct SM_sarray_context_type
{ /* Suffix array context record */
  unsigned int index_pos;        /* Current position in suffix array
				    list of indexes */
  unsigned int string_pos;       /* Current position in input string */
};

struct SM_strie_context_type
{ /* Suffix trie context record */
  struct SM_trie_type *node;    /* Current trie node */
  unsigned int depth;            /* Current depth in trie. */
  unsigned int spos;             /* Current position in non-branching pathway, if applicable */
  unsigned int symbol;           /* Current symbol */
  unsigned int parent_count;     /* Sum of all sibling counts */
};

struct SM_trie_type
{ /* Suffix array model record */
  unsigned int symbol;           /* The symbol associated with this node */
  unsigned int count;            /* The symbol's count */
  struct SM_trie_type *sptr;    /* Sibling pointer to next symbol in list;
				    Also typecast as (char *) or (unsigned int *)
				    to give string start position
				    for non-branching pathway */
  struct SM_trie_type *child;   /* Child pointer to further down the tree */
  unsigned int slen;             /* Length of input string used for
				    non-branching pathway */
};

void
SM_init_counts ();
/* Initializes the various counts. */

void
SM_dump_counts (unsigned int file, unsigned int size);
/* Dumps out the various counts (for debugging purposes). */

void
SM_accumulate_counts ();
/* Updates the cumulative counts. */

void
SM_dump_nstring1 (unsigned int file, unsigned int *string, unsigned int p, unsigned int len);
/* Dumps the string (of numbers) to the file. */

void
SM_check_sarray (unsigned int string_size, char *string, unsigned int *sarray);
/* Checks that the suffix array for a string of characters is sorted */

void
SM_check_nsarray (unsigned int string_size, unsigned int *string,
		  unsigned int *sarray);
/* Checks that the suffix array for a string of numbers is sorted */

unsigned int *
SM_make_sarray (unsigned int string_size, char *string);
/* Builds the suffix array in memory for a string of characters. */

unsigned int *
SM_make_nsarray (unsigned int string_size, unsigned int *string);
/* Builds the suffix array in memory for a string of numbers. */

void
SM_dump_sarray (unsigned int file, unsigned int string_size, char *string,
		unsigned int *sarray);
/* Dumps out the suffix array to the file (for debug purposes). */

void
SM_dump_nsarray (unsigned int file, unsigned int string_size, unsigned int *string,
		 unsigned int *sarray);
/* Dumps out the suffix array to the file (for debug purposes). */

void
SM_write_sarray (unsigned int file, unsigned int string_size,
		 char *string, unsigned int *sarray);
/* Writes out the suffix array of the string to the file. */

void
SM_write_nsarray (unsigned int file, unsigned int string_size,
		  unsigned int *nstring, unsigned int *sarray);
/* Writes out the suffix array of the numbers nstring to the file. */

struct SM_model_type *
SM_load_model (unsigned int file);
/* Builds the suffix model in memory by loading it from the file. */

void
SM_dump_trie (unsigned int file, struct SM_model_type *model, struct SM_trie_type *trie,
	      boolean dump_all_nodes, boolean dump_ptrs);
/* Recursive routine to dump trie model. */

void
SM_dump_model (unsigned int file, struct SM_model_type *model,
	       struct SM_trie_type *trie,
	       boolean dump_all_nodes, boolean dump_ptrs,
	       char *ident);
/* Dumps out the model (for devbugging purposes). */

void
SM_check_trie (struct SM_model_type *model, struct SM_trie_type *trie);
/* Recursive routine to check the suffix trie for consistent counts. */

struct SM_trie_type *
SM_grow_trie (struct SM_model_type *model);
/* Builds the suffix trie given the suffix array model. */

extern unsigned int *SM_traverse_suffix;            /* Current suffix being traversed */
extern unsigned int SM_traverse_suffix_length;      /* Length of current suffix (#chars) */
extern unsigned int SM_traverse_suffix_count;       /* Count associated with current suffix */
extern unsigned int *SM_traverse_prev_suffix;       /* Previous suffix that was just traversed */
extern unsigned int SM_traverse_prev_suffix_length; /* Length of previous suffix (#chars) */

boolean
SM_traverse_same_prefix (unsigned int prefix_length);
/* Returns TRUE if the current char suffix and the previous char suffix being traversed in the
   suffix model has the same prefix of length prefix_length. */

boolean
SM_traverse_same_prefix (unsigned int prefix_length);
/* Returns TRUE if the current word suffix and the previous word suffix being traversed in the
   suffix model has the same prefix of length prefix_length. */

void
SM_traverse_trie (struct SM_model_type *model, struct SM_trie_type *trie,
		  boolean (*suffix_match_function) (unsigned int, unsigned int *));
/* Recursive routine to traverse the trie model.

   The suffix match function is a user defined function that returns TRUE if the
   suffix matches some criteria as defined by the function (e.g. that it contains
   at least 5 characters, say, or two words). It takes two parameters - the first
   is an unsigned int that is the length of the suffix, and the second is an
   unsigned int * that is a pointer to the suffix itself (and array of unsigned ints).
   If the function is NULL, it will match all suffixes. */

unsigned int
SM_cotraverse_trie (struct SM_model_type *model, struct SM_trie_type *trie,
		    struct SM_model_type *model1, struct SM_trie_type *trie1);
/* Recursive routine to calculate various measures  given the two suffix tries - trie,
   the training model, and trie1, the testing model. */

#endif

