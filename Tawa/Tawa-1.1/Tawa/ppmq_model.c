/* Routines for PPMq models. */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include "io.h"
#include "text.h"
#include "model.h"
#include "ppmq_model.h"

#define PPMq_MODELS_SIZE 4         /* Initial max. number of models */
#define PPMq_CONTEXTS_SIZE 128     /* Initial max. number of contexts */

#define PPMq_MAX_BYTE_COUNT 254    /* Maximum count that fits in a byte without overflow */
#define PPMq_COUNT_OVERFLOW 255    /* Indicates count does not fit in a byte */

/* Increments used to update the counts of the model: */
#define PPMq_INCR_C 1
#define PPMq_INCR_D 2

/* Offsets to data stored in the model. */
/* These fields are for counts and symbols that can't fit in a byte;
   at the moment, if either count or symbol can't fit, then 8 bytes
   are added to allow room for both possibilities - this could be
   reduced to only 4 extra bytes, but I haven't bothered to go to the
   extra effort although there could be extra wastage since in most
   cases, the symbol number usually fits in a byte only */
/* Offsets to data in node record: */
#define PPMq_NODE_SIZE 10 /* symbol(1) + sibling ptr(4) + child ptr(4) + count(1) */
#define PPMq_NODE_SYMBOL 0
#define PPMq_NODE_NEXT 1
#define PPMq_NODE_CHILD 5
#define PPMq_NODE_COUNT 9

/* Offsets to data in node1 record: */
#define PPMq_NODE1_SIZE 18 /* symbol(1) + sibling ptr(4) + child ptr(4) + count(1) +
			      count1(4) + symbol1(4) */
#define PPMq_NODE1_COUNT 10
#define PPMq_NODE1_SYMBOL 14

/* Offsets to data in leaf record: */
#define PPMq_LEAF_SIZE 6 /* symbol(1) + sibling ptr(4) + count(1) */
#define PPMq_LEAF_SYMBOL 0
#define PPMq_LEAF_NEXT 1
#define PPMq_LEAF_COUNT 5

/* Offsets to data in leaf1 record: */
#define PPMq_LEAF1_SIZE 14 /* symbol(1) + sibling ptr(4) + count(1) + count1(4) +
			      symbol1(4) */
#define PPMq_LEAF1_COUNT 6
#define PPMq_LEAF1_SYMBOL 10

#define PPMq_LARGE_SYMBOL 254   /* Indicates that symbol doesn't fit in a byte */
#define PPMq_NOMORE_SYMBOLS 255 /* Indicates no more siblings in trie */

/* Macros for getting and putting data from context trie nodes */
#define PPMq_GET_CHAR(node,field)    *(node + field)
#define PPMq_PUT_CHAR(node,field,n)  *(node + field) = n
#define PPMq_GET_PTR(node,field)     (unsigned char *) *((unsigned int *) (node + field))
#define PPMq_PUT_PTR(node,field,ptr) (*((unsigned int *) (node + field))) = \
                                       (unsigned int) ptr
#define PPMq_GET_INT(node,field)     (unsigned int) *((unsigned int *) (node + field))
#define PPMq_PUT_INT(node,field,n)   (*((unsigned int *) (node + field))) = n
#define PPMq_INC_INT(node,field,n)   (*((unsigned int *) (node + field))) += n

#define PPMq_DEFAULT_ALPHABET_SIZE 256
#define PPMq_DEFAULT_ESCAPE_METHOD TLM_PPM_Method_D
#define PPMq_DEFAULT_MODEL_ORDER 4
#define PPMq_DEFAULT_PERFORMS_FULL_EXCLS TRUE

/* Global variables used for storing the PPM models */
struct PPMq_modelType *PPMq_Models;     /* List of PPMq models */
unsigned int PPMq_Models_max_size = 0;  /* Current max. size of models array */
unsigned int PPMq_Models_used = NIL;    /* List of deleted model records */
unsigned int PPMq_Models_unused = 1;    /* Next unused model record */

/* Global variables used for storing the PPMq contexts */
/* Global variables used for storing the PPM models */
struct PPMq_contextType *PPMq_Contexts; /* List of PPMq contexts */
unsigned int PPMq_Contexts_max_size = 0;/* Current max. size of models array */
unsigned int PPMq_Contexts_used = NIL;  /* List of deleted model records */
unsigned int PPMq_Contexts_unused = 1;  /* Next unused model record */
boolean PPMq_initialized_context = FALSE;

/* Global variables used for arithmetic coding and for PPMq_encode_contexts (),
   PPMq_decode_contexts (). */
unsigned int PPMq_symbol = 0;
unsigned int PPMq_target = 0;
unsigned int PPMq_lbnd = 0;
unsigned int PPMq_hbnd = 0;
unsigned int PPMq_totl = 0;
unsigned int PPMq_escape = 0;
boolean *PPMq_exclusions = NULL;
unsigned int PPMq_decoding_pos = 0;
unsigned int *PPMq_decoding_ranges = NULL;
unsigned int *PPMq_decoding_symbols = NULL;

void
PPMq_junk ()
/* For debugging purposes. */
{
  fprintf (stderr, "Got here\n");
}

void
PPMq_dump_symbol (unsigned int file, unsigned int symbol,
		  void (*dump_symbol_function) (unsigned int, unsigned int))
/* Dump the symbol */
{
    assert (TXT_valid_file (file));

    if (dump_symbol_function)
        dump_symbol_function (file, symbol);
    else if (symbol == TXT_sentinel_symbol ())
	fprintf (Files [file], "<sentinel>");
    else if ((symbol <= 32) || (symbol >= 127))
	fprintf (Files [file], "<%d>", symbol);
    else
	fprintf (Files [file], "%c", symbol);
}

unsigned int
PPMq_eof_symbol (unsigned int ppmq_model)
/* Returns the eof symbol for the model. */
{
    return (TXT_sentinel_symbol ());
}

boolean
PPMq_valid_model (unsigned int ppmq_model)
/* Returns non-zero if the PPM model is valid, zero otherwize. */
{
    if (ppmq_model == NIL)
        return (FALSE);
    else if (ppmq_model >= PPMq_Models_unused)
        return (FALSE);
    else if (PPMq_Models [ppmq_model].P_max_order < -1)
        /* The max order gets set to -2 when the model gets deleted;
	   this way you can test to see if the model has been deleted or not */
        return (FALSE);
    else
        return (TRUE);
}

unsigned int
create_PPMq_model (void)
/* Creates and returns a new pointer to a PPMq model record. */
{
    unsigned int ppmq_model, old_size;

    if (PPMq_Models_used != NIL)
    { /* use the first record on the used list */
        ppmq_model = PPMq_Models_used;
	PPMq_Models_used = PPMq_Models [ppmq_model].P_next;
    }
    else
    {
	ppmq_model = PPMq_Models_unused;
        if (PPMq_Models_unused >= PPMq_Models_max_size)
	{ /* need to extend PPMq_Models array */
	    old_size = PPMq_Models_max_size * sizeof (struct modelType);
	    if (PPMq_Models_max_size == 0)
	        PPMq_Models_max_size = PPMq_MODELS_SIZE;
	    else
	        PPMq_Models_max_size = 10*(PPMq_Models_max_size+50)/9;

	    PPMq_Models = (struct PPMq_modelType *)
	        Realloc (84, PPMq_Models, PPMq_Models_max_size *
			 sizeof (struct PPMq_modelType), old_size);

	    if (PPMq_Models == NULL)
	    {
	        fprintf (stderr, "Fatal error: out of PPM models space\n");
		exit (1);
	    }
	}
	PPMq_Models_unused++;
    }

    PPMq_Models [ppmq_model].P_max_order = PPMq_DEFAULT_MODEL_ORDER;
    PPMq_Models [ppmq_model].P_alphabet_size = PPMq_DEFAULT_ALPHABET_SIZE;
    PPMq_Models [ppmq_model].P_performs_full_excls = PPMq_DEFAULT_PERFORMS_FULL_EXCLS;
    PPMq_Models [ppmq_model].P_escape_method = PPMq_DEFAULT_ESCAPE_METHOD;

    PPMq_Models [ppmq_model].P_trie = NULL;
    PPMq_Models [ppmq_model].P_next = NIL;

    return (ppmq_model);
}

unsigned int
PPMq_create_model (unsigned int alphabet_size, int max_order,
		   unsigned int escape_method, boolean performs_full_excls)
/* Creates and returns a new pointer to a PPMq model record. */
{
    unsigned int ppmq_model;

    ppmq_model = create_PPMq_model ();

    PPMq_Models [ppmq_model].P_trie = NULL;
    assert (alphabet_size != 0);
    PPMq_Models [ppmq_model].P_alphabet_size = alphabet_size;
    assert (max_order >= -1);
    if (max_order < 0)
        /* The (order -1) models never need to perform any exclusions */
        performs_full_excls = FALSE;
    PPMq_Models [ppmq_model].P_max_order = max_order;

    PPMq_Models [ppmq_model].P_performs_full_excls = performs_full_excls;
    PPMq_Models [ppmq_model].P_escape_method = escape_method;
    assert ((TLM_PPM_Method_A <= escape_method) && (escape_method <= TLM_PPM_Method_D));
    if ((escape_method == TLM_PPM_Method_A) || (escape_method == TLM_PPM_Method_B))
      {
	fprintf (stderr, "PPMq escape method %d is not yet implemented\n", escape_method);
	exit (1);
      }

    return (ppmq_model);
}

void
PPMq_get_model (unsigned int model, unsigned int type, unsigned int *value)
/* Returns information about the PPM model. The arguments type is
   the information to be returned (i.e. PPMq_Get_Alphabet_Size,
   PPMq_Get_Max_Symbol, PPMq_Get_Max_Order, PPMq_Get_Escape_Method or
   PPMq_Get_Performs_Full_Excls). */
{
    unsigned int ppmq_model;

    ppmq_model = TLM_verify_model (model, TLM_PPMq_Model, PPMq_valid_model);

    switch (type)
      {
      case PPMq_Get_Alphabet_Size:
	*value = PPMq_Models [ppmq_model].P_alphabet_size;
	break;
      case PPMq_Get_Max_Order:
	*value = (unsigned int) PPMq_Models [ppmq_model].P_max_order;
	break;
      case PPMq_Get_Escape_Method:
	*value = PPMq_Models [ppmq_model].P_escape_method;
	break;
      case PPMq_Get_Performs_Full_Excls:
	*value = PPMq_Models [ppmq_model].P_performs_full_excls;
	break;
      default:
	fprintf (stderr, "Invalid type (%d) specified for PPMq_get_model ()\n", type);
	exit (1);
	break;
      }
}

void
PPMq_set_model (unsigned int model, unsigned int type, unsigned int value)
/* Sets information about the PPM model. The argument type is the information
   to be set (i.e. PPMq_Set_Alphabet_Size or PPMq_Set_Max_Symbol)
   and its value is set to the argument value.

   The type PPMq_Alphabet_Size is used for specifying the size of the
   expanded alphabet (which must be equal to 0 - indicating an unbounded
   alphabet, or greater than the existing size to accomodate symbols used by
   the existing model). */
{
    unsigned int ppmq_model;
    unsigned int alphabet_size;

    ppmq_model = TLM_verify_model (model, TLM_PPMq_Model, PPMq_valid_model);

    switch (type)
      {
      case PPMq_Set_Alphabet_Size:
	alphabet_size = value;
	assert ((alphabet_size == 0) ||
		(alphabet_size > PPMq_Models [ppmq_model].P_alphabet_size));
	PPMq_Models [ppmq_model].P_alphabet_size = alphabet_size;
	break;
      default:
	fprintf (stderr, "Invalid type (%d) specified for PPMq_set_model ()\n", type);
	exit (1);
	break;
      }
}

void
PPMq_release_trie (unsigned int ppmq_model, unsigned char *ptr, unsigned int pos)
/* Recursive routine to release the trie. */
{
    unsigned int symbol, offset, size;
    unsigned char *snext, *child;
    boolean is_leaf;
    int max_order;

    max_order = PPMq_Models [ppmq_model].P_max_order;
    is_leaf = (pos >= max_order);
    while (ptr != NULL)
      { /* proceed through the symbol list */
	snext = PPMq_GET_PTR (ptr, PPMq_NODE_NEXT);
	if (is_leaf)
	    child = NULL;
	else
	    child = PPMq_GET_PTR (ptr, PPMq_NODE_CHILD);

	if (child != NULL)
	  {
	    PPMq_release_trie (ppmq_model, child, pos+1);
	  }

	if (is_leaf)
	    offset = PPMq_LEAF_SYMBOL;
	else
	    offset = PPMq_NODE_SYMBOL;

	symbol = PPMq_GET_CHAR (ptr, offset);
	if (is_leaf)
	  {
	    if (symbol == PPMq_LARGE_SYMBOL)
	      size = PPMq_LEAF1_SIZE;
	    else
	      size = PPMq_LEAF_SIZE;
	  }
	else
	  {
	    if (symbol == PPMq_LARGE_SYMBOL)
	      size = PPMq_NODE1_SIZE;
	    else
	      size = PPMq_NODE_SIZE;
	  }

	Free (90, ptr, size);
	ptr = snext;
      }
}

void
PPMq_release_model (unsigned int ppmq_model)
/* Releases the memory allocated to the model and the model number (which may
   be reused in later PPMq_create_model or PPMq_load_model calls).
   A run-time error will be generated if an attempt is made to release
   a model that still has active contexts pointing at it. */
{
    assert (PPMq_valid_model (ppmq_model));

    /* Release the model's trie as well */
    PPMq_release_trie (ppmq_model, PPMq_Models [ppmq_model].P_trie, 0);

    /* add model record at the head of the PPMq_Models_used list */
    PPMq_Models [ppmq_model].P_next = PPMq_Models_used;
    PPMq_Models_used = ppmq_model;

    PPMq_Models [ppmq_model].P_max_order = -2; /* Used for testing if model no.
						is valid or not */
}

unsigned char *
PPMq_create_node (unsigned int symbol)
{
    unsigned int size, bytes;
    unsigned char *ptr;
    boolean is_small_record;

    is_small_record = (symbol < PPMq_LARGE_SYMBOL);
    if (is_small_record)
        size = PPMq_NODE_SIZE;
    else
        size = PPMq_NODE1_SIZE;

    /* Create the node */
    bytes = size * sizeof (unsigned char);
    ptr = (unsigned char *) Calloc (90, bytes, 1);
    assert (ptr != NULL);

    /*fprintf (stderr, "Created node ptr %p for symbol %d\n", ptr, symbol);*/

    if (is_small_record)
        PPMq_PUT_CHAR (ptr, PPMq_NODE_SYMBOL, symbol);
    else
      { /* Either the symbol or count can't fit in a byte */
        PPMq_PUT_CHAR (ptr, PPMq_NODE_SYMBOL, PPMq_LARGE_SYMBOL);
	PPMq_PUT_INT (ptr, PPMq_NODE1_SYMBOL, symbol);
      }
    return (ptr);
}

unsigned char *
PPMq_create_leaf (unsigned int symbol)
{
    unsigned int size, bytes;
    unsigned char *ptr;
    boolean is_small_record;

    is_small_record = (symbol < PPMq_LARGE_SYMBOL);
    if (is_small_record)
        size = PPMq_LEAF_SIZE;
    else
        size = PPMq_LEAF1_SIZE;

    /* Create the leaf */
    bytes = size * sizeof (unsigned char);
    ptr = (unsigned char *) Calloc (90, bytes, 1);
    assert (ptr != NULL);

    /*fprintf (stderr, "Created leaf ptr %p for symbol %d\n", ptr, symbol);*/

    if (is_small_record)
        PPMq_PUT_CHAR (ptr, PPMq_LEAF_SYMBOL, symbol);
    else
      { /* Either the symbol or count can't fit in a byte */
        PPMq_PUT_CHAR (ptr, PPMq_LEAF_SYMBOL, PPMq_LARGE_SYMBOL);
	PPMq_PUT_INT (ptr, PPMq_LEAF1_SYMBOL, symbol);
      }
    return (ptr);
}

unsigned int
PPMq_getnode_symbol (unsigned char *trie_ptr, boolean is_leaf)
/* Returns the count for the node at trie_ptr */
{
    unsigned int symbol;

    if (is_leaf)
        symbol = PPMq_GET_CHAR (trie_ptr, PPMq_LEAF_SYMBOL);
    else
        symbol = PPMq_GET_CHAR (trie_ptr, PPMq_NODE_SYMBOL);
    if (symbol == PPMq_LARGE_SYMBOL)
      {
	if (is_leaf)
	    symbol = PPMq_GET_INT (trie_ptr, PPMq_LEAF1_SYMBOL);
	else
	    symbol = PPMq_GET_INT (trie_ptr, PPMq_NODE1_SYMBOL);
      }
    return (symbol);
}

unsigned int
PPMq_getnode_count (unsigned char *trie_ptr, boolean is_leaf)
/* Returns the count for the node at trie_ptr */
{
    unsigned int count;

    if (is_leaf)
        count = PPMq_GET_CHAR (trie_ptr, PPMq_LEAF_COUNT);
    else
        count = PPMq_GET_CHAR (trie_ptr, PPMq_NODE_COUNT);
    if (count == PPMq_COUNT_OVERFLOW)
      {
	if (is_leaf)
	    count = PPMq_GET_INT (trie_ptr, PPMq_LEAF1_COUNT);
	else
	    count = PPMq_GET_INT (trie_ptr, PPMq_NODE1_COUNT);
      }
    return (count);
}

boolean
PPMq_valid_context (unsigned int ppmq_context)
/* Returns non-zero if the PPM context is valid, zero otherwize. */
{
    if (ppmq_context == NIL)
        return (FALSE);
    else if (ppmq_context >= PPMq_Contexts_unused)
        return (FALSE);
    else if (PPMq_Contexts [ppmq_context].P_context == NULL)
        /* The context field gets set to NULL when the context gets deleted;
	   this way you can test to see if the context has been deleted or not */
        return (FALSE);
    else
        return (TRUE);
}

unsigned int
create_PPMq_context (void)
/* Creates and returns a new pointer to a PPMq context record. */
{
    unsigned int ppmq_context, old_size;

    if (PPMq_Contexts_used != NIL)
    { /* use the first record on the used list */
        ppmq_context = PPMq_Contexts_used;
	PPMq_Contexts_used = PPMq_Contexts [ppmq_context].P_next;
    }
    else
    {
	ppmq_context = PPMq_Contexts_unused;
        if (PPMq_Contexts_unused >= PPMq_Contexts_max_size)
	{ /* need to extend PPMq_Contexts array */
	    old_size = PPMq_Contexts_max_size * sizeof (struct PPMq_contextType);
	    if (PPMq_Contexts_max_size == 0)
	        PPMq_Contexts_max_size = PPMq_CONTEXTS_SIZE;
	    else
	        PPMq_Contexts_max_size = 10*(PPMq_Contexts_max_size+50)/9;

	    PPMq_Contexts = (struct PPMq_contextType *)
	        Realloc (84, PPMq_Contexts, PPMq_Contexts_max_size *
			 sizeof (struct PPMq_contextType), old_size);

	    if (PPMq_Contexts == NULL)
	    {
	        fprintf (stderr, "Fatal error: out of PPM contexts space\n");
		exit (1);
	    }
	}
	PPMq_Contexts_unused++;
    }

    PPMq_Contexts [ppmq_context].P_context = NULL;
    PPMq_Contexts [ppmq_context].P_next = NIL;

    return (ppmq_context);
}

unsigned int
PPMq_create_context (unsigned int model)
/* Creates and returns a new pointer to a PPMq context record. */
{
    unsigned int ppmq_context, ppmq_model;

    ppmq_model = TLM_verify_model (model, TLM_PPMq_Model, PPMq_valid_model);

    ppmq_context = create_PPMq_context ();

    if (PPMq_Models [ppmq_model].P_max_order >= 0)
      PPMq_Contexts [ppmq_context].P_context = (unsigned char **)
        Calloc (91, PPMq_Models [ppmq_model].P_max_order+1,
		sizeof (unsigned char *));

    if (!PPMq_initialized_context)
      {
	PPMq_initialized_context = TRUE;

	if (PPMq_Models [ppmq_model].P_performs_full_excls)
	  PPMq_exclusions = (boolean *)
	    Calloc (92, PPMq_Models [ppmq_model].P_alphabet_size+1,
		    sizeof (boolean));

	PPMq_decoding_ranges = (unsigned int *)
	  Calloc (92, PPMq_Models [ppmq_model].P_alphabet_size+2,
		  sizeof (unsigned int));
	PPMq_decoding_symbols = (unsigned int *)
	  Calloc (92, PPMq_Models [ppmq_model].P_alphabet_size+2,
		  sizeof (unsigned int));
      }

    return (ppmq_context);
}

void
update_PPMq_context (unsigned int ppmq_model, unsigned int ppmq_context)
/* Updates the context for the PPMq model using the new symbol. */
{
    register int pos;

    /* Roll all the contexts along by one */
    for (pos = PPMq_Models [ppmq_model].P_max_order; pos > 0; pos--)
      {
	PPMq_Contexts [ppmq_context].P_context [pos] =
	    PPMq_Contexts [ppmq_context].P_context [pos-1];
      }
    PPMq_Contexts [ppmq_context].P_context [0] = NULL; /* At the Root */
}

void
PPMq_release_context (unsigned int model, unsigned int context)
/* Releases the memory allocated to the PPMq context and the context number
   (which may be reused in later PPM_create_context calls). */
{
    unsigned int ppmq_model, ppmq_context;

    ppmq_model = TLM_verify_model (model, TLM_PPMq_Model, PPMq_valid_model);

    ppmq_context = context;
    assert (PPMq_valid_context (ppmq_context));

    if (PPMq_Models [ppmq_model].P_max_order >= 0)
      Free (91, PPMq_Contexts [ppmq_context].P_context,
	  (PPMq_Models [ppmq_model].P_max_order+1) * sizeof (unsigned char *));
    PPMq_Contexts [ppmq_context].P_context = NULL;

    /* add context record at the head of the PPMq_Contexts_used list */
    PPMq_Contexts [ppmq_context].P_next = PPMq_Contexts_used;
    PPMq_Contexts_used = ppmq_context;
}

void
PPMq_reset_encode_counts ()
/* Resets the counts used for arithmetic encoding. */
{
    PPMq_lbnd = 0;
    PPMq_hbnd = 0;
    PPMq_totl = 0;
    PPMq_escape = 0;
}

void
PPMq_reset_decode_counts (unsigned int ppmq_model)
/* Resets the counts used for arithmetic decoding. */
{
    PPMq_lbnd = 0;
    PPMq_hbnd = 0;
    PPMq_totl = 0;
    PPMq_escape = 0;

    PPMq_decoding_pos = 0;
    memset (PPMq_decoding_ranges, 0, (PPMq_Models [ppmq_model].P_alphabet_size+2) * sizeof (unsigned int));
    memset (PPMq_decoding_symbols, 0, (PPMq_Models [ppmq_model].P_alphabet_size+2) * sizeof (unsigned int));
}

void
PPMq_encode (int order, unsigned int coder,
	     unsigned int lbnd, unsigned int hbnd, unsigned int totl)
/* Encodes the current arithmetic coding range. */
{
    if (Debug.range)
      {
        fprintf (stderr, "Symbol = %d order = %d\n", PPMq_symbol, order);
        fprintf (stderr, "lbnd = %d hbnd = %d totl = %d\n", lbnd, hbnd, totl);
      }

    Coders [coder].A_arithmetic_encode
      (Coders [coder].A_encoder_output_file, lbnd, hbnd, totl);
}

void
PPMq_decode (int order, unsigned int coder, unsigned int target,
	     unsigned int lbnd, unsigned int hbnd, unsigned int totl)
/* Decodes the current arithmetic coding range. */
{
    if (Debug.range)
      {
        fprintf (stderr, "Symbol = %d order = %d\n", PPMq_symbol, order);
        fprintf (stderr, "lbnd = %d hbnd = %d totl = %d\n", lbnd, hbnd, totl);
      }
    if (Debug.level > 0)
        fprintf (stderr, "target = %d\n", target);

    Coders [coder].A_arithmetic_decode
      (Coders [coder].A_decoder_input_file, lbnd, hbnd, totl);
}

unsigned int
PPMq_decode_target (int order, unsigned int coder, unsigned int total)
/* Decodes the current arithmetic coding range. */
{
    unsigned int target;

    target = Coders [coder].A_arithmetic_decode_target
      (Coders [coder].A_decoder_input_file, total);

    if (Debug.level > 0)
      {
        fprintf (stderr, "Order = %d target = %d\n", order, target);
      }

    return (target);
}

boolean
PPMq_decode_range (unsigned int ppmq_model, unsigned int target)
/* Decodes the symbol for the arithmetic coding range
   for which target falls into. Uses a binary search.
   *** Should use an interpolation search ***.
   Returns FALSE if an escape symbol has been decoded. */
{
    register unsigned int hi_pos, lo_pos, mid_pos;
    unsigned int p, eof_symbol;

    lo_pos = 0;
    hi_pos = PPMq_decoding_pos;

    /* Add in escape */
    eof_symbol = PPMq_eof_symbol (ppmq_model);
    PPMq_decoding_symbols [hi_pos] = eof_symbol; /* signifies an escape symbol */
    PPMq_decoding_ranges [hi_pos+1] = PPMq_decoding_ranges [hi_pos] + PPMq_escape;

    if (Debug.level > 4)
      {
        fprintf (stderr, "Decoding ranges for target %d :", target);
	for (p = 0; p <= PPMq_decoding_pos+1; p++)
	    fprintf (stderr, " %d (%d)",
		     PPMq_decoding_ranges [p],
		     PPMq_decoding_symbols [p]);
	fprintf (stderr, "\n");
      }

    assert (target < PPMq_decoding_ranges [hi_pos+1]);

    mid_pos = 0;
    if (target)
      for (;;)
      { /* keep on getting closer and closer to the target range */
	if (hi_pos == lo_pos)
	    break;
	if (hi_pos - lo_pos == 1)
	  {
	    if (target < PPMq_decoding_ranges [hi_pos])
	        mid_pos = lo_pos;
	    else
	        mid_pos = hi_pos;
	    break;
	  }

	mid_pos = lo_pos + (hi_pos - lo_pos + 1) / 2;
	if (target < PPMq_decoding_ranges [mid_pos])
	    hi_pos = mid_pos;
	else if (target > PPMq_decoding_ranges [mid_pos])
	    lo_pos = mid_pos;
	else
	    break;

	if (Debug.level > 4)
	    fprintf (stderr, "lo_pos %d hi_pos %d mid_pos %d\n",
		     lo_pos, hi_pos, mid_pos);
      }
    PPMq_symbol = PPMq_decoding_symbols [mid_pos];
    PPMq_lbnd = PPMq_decoding_ranges [mid_pos];
    PPMq_hbnd = PPMq_decoding_ranges [mid_pos+1];
    if (Debug.level > 4)
        fprintf (stderr, "Found target at position %d symbol = %d\n",
		 mid_pos, PPMq_symbol);

    return (PPMq_symbol != eof_symbol);
}

void
PPMq_encode_order_minus1 (unsigned int ppmq_model, unsigned int coder)
/* Used only when encoding an order -1 context. */
{
    register unsigned int sym;

    if (Debug.level > 4)
        fprintf (stderr, "Order -1 encoding needed\n");

    PPMq_reset_encode_counts ();

    for (sym = 0; sym < PPMq_Models [ppmq_model].P_alphabet_size; sym++)
      if (!PPMq_Models [ppmq_model].P_performs_full_excls || !PPMq_exclusions [sym])
	{
	  if (PPMq_symbol == sym)
	    {
	      PPMq_lbnd = PPMq_totl;
	      PPMq_hbnd = PPMq_lbnd + 1;
	    }
	  PPMq_totl++;
	}
    if (PPMq_symbol == TXT_sentinel_symbol ())
      { /* Must add "EOF" symbol as well */
	PPMq_lbnd = PPMq_totl;
	PPMq_hbnd = PPMq_lbnd + 1;
      }
    PPMq_totl++;
    PPMq_encode (-1 /* order = -1 */, coder, PPMq_lbnd, PPMq_hbnd, PPMq_totl );
}

void
PPMq_decode_order_minus1 (unsigned int ppmq_model, unsigned int coder)
/* Used only when decoding an order -1 context. */
{
    register unsigned int sym;

    if (Debug.level > 4)
        fprintf (stderr, "Order -1 decoding needed\n");

    PPMq_reset_decode_counts (ppmq_model);

    for (sym = 0; sym < PPMq_Models [ppmq_model].P_alphabet_size; sym++)
      if (!PPMq_Models [ppmq_model].P_performs_full_excls || !PPMq_exclusions [sym])
	{
	  PPMq_decoding_symbols [PPMq_totl] = sym;
	  PPMq_totl++;
	}
    PPMq_decoding_symbols [PPMq_totl] = TXT_sentinel_symbol (); /* EOF symbol */
    PPMq_totl++;
    PPMq_target = PPMq_decode_target (-1 /* order = -1 */, coder, PPMq_totl);
    PPMq_symbol = PPMq_decoding_symbols [PPMq_target];
    PPMq_decode (-1 /* order = -1 */, coder, PPMq_target,
		 PPMq_target, PPMq_target+1, PPMq_totl);
}

void
PPMq_update_counts (unsigned int ppmq_model, unsigned int ppmq_context,
		    int increment)
/* Updates the model's counts for the new symbol. */
{
    register unsigned char *ptr, *old_ptr, *parent;
    register unsigned char *snext, *sprev, *lprev, *lptr;
    register unsigned int sym, count, lcount, offset;
    register int max_order, pos;
    register boolean is_leaf;

    /* Find current max order; max order will be less than model's max order
       only for first few symbols in the input sequence while we build up the
       context to max order length */
    max_order = PPMq_Models [ppmq_model].P_max_order;
    if (max_order >= 0)
      for (; max_order > 0; max_order--)
	if (PPMq_Contexts [ppmq_context].P_context [max_order] != NULL)
	  break;

    for (pos = max_order; pos >= 0; pos--)
      { /* create or access nodes in the trie pointed at by PPMq_Contexts [ppmq_context].P_context */
	is_leaf = (pos >= PPMq_Models [ppmq_model].P_max_order);

	/* Follow child pointer for this context i.e. what it predicts */
	parent = PPMq_Contexts [ppmq_context].P_context [pos];
	if (parent == NULL)
	    ptr = PPMq_Models [ppmq_model].P_trie;
	else
	    ptr = PPMq_GET_PTR (parent, PPMq_NODE_CHILD);

	sprev = NULL;
	lprev = NULL;
	lptr = NULL;
	old_ptr = NULL;
	count = 0;
	lcount = 0;
	snext = NULL;
	while (ptr != NULL)
	  { /* find out where the symbol is */
	    sym = PPMq_getnode_symbol (ptr, is_leaf);
	    count = PPMq_getnode_count (ptr, is_leaf);
	    snext = PPMq_GET_PTR (ptr, PPMq_NODE_NEXT);
	    if (sym == PPMq_symbol)
		break; /* found symbol - exit loop */
	    if (count != lcount)
	      { /* Keep track of when the last count changed so that we can
		   maintain sorted frequency order */
		lptr = ptr;
		lprev = sprev;
		lcount = count;
	      }
	    sprev = ptr;
	    ptr = snext;
	  }

	old_ptr = ptr;

	/* Create new node if required */
	if (ptr == NULL)
	  { /* haven't found the symbol - create it */
	    count = 0;
	    if (is_leaf)
	        ptr = PPMq_create_leaf (PPMq_symbol);
	    else
	        ptr = PPMq_create_node (PPMq_symbol);
	  }

	/* Increment count */
	if (is_leaf)
	    offset = PPMq_LEAF_COUNT;
	else
	    offset = PPMq_NODE_COUNT;

	if (count + increment <= PPMq_MAX_BYTE_COUNT)
	    PPMq_INC_INT (ptr, offset, increment); /* Increment node symbol count */
	else
	  { /* Overflow of byte count field */
	    register unsigned int offset1, size;

	    /*fprintf (stderr, "Overflow has occurred\n");*/

	    if (is_leaf)
	      {
		offset1 = PPMq_LEAF1_COUNT;
		size = PPMq_LEAF1_SIZE;
	      }
	    else
	      {
		offset1 = PPMq_NODE1_COUNT;
		size = PPMq_NODE1_SIZE;
	      }

	    if (PPMq_GET_CHAR (ptr, offset) != PPMq_COUNT_OVERFLOW)
	      {
		/*
		  fprintf (stderr, "Overflow has occurred");
		  PPMq_dump_model (stderr);
		*/

		/* Indicate count has overflowed byte */
		PPMq_PUT_CHAR (ptr, offset, PPMq_COUNT_OVERFLOW);
		ptr = realloc (ptr, size);
		PPMq_PUT_INT (ptr, offset1, count);

		old_ptr = NULL; /* Ensure trie gets updated with re-allocated ptr */
	      }

	    PPMq_INC_INT (ptr, offset1, increment);
	  }

	/* Store this context ptr */
	PPMq_Contexts [ppmq_context].P_context [pos] = ptr;

	if (old_ptr == NULL)
	  {
	    if (sprev != NULL)
	        PPMq_PUT_PTR (sprev, PPMq_NODE_NEXT, ptr);
	    else if (parent == NULL)
	        PPMq_Models [ppmq_model].P_trie = ptr;
	    else
	        PPMq_PUT_PTR (parent, PPMq_NODE_CHILD, ptr);
	  }
	else if ((sprev != NULL) && (lcount > 0))
	  { /* Maintain the list in sorted frequency order */
	    if (lcount < count + increment)
	      { /* Move this node ptr up to after lprev and before lptr */
	        PPMq_PUT_PTR (sprev, PPMq_NODE_NEXT, snext);
	        PPMq_PUT_PTR (ptr, PPMq_NODE_NEXT, lptr);
		if (lprev != NULL)
		    PPMq_PUT_PTR (lprev, PPMq_NODE_NEXT, ptr);
		else /* move to head of the list */
		  if (parent == NULL)
		      PPMq_Models [ppmq_model].P_trie = ptr;
		  else
		      PPMq_PUT_PTR (parent, PPMq_NODE_CHILD, ptr);
	      }
	  }
      }

    /* Roll all context pointers down one */
    update_PPMq_context (ppmq_model, ppmq_context);
}

void
PPMq_encode_counts (unsigned int ppmq_model, unsigned int ppmq_context,
		    unsigned int coder, int increment)
/* Encodes and updates the model's counts for the new symbol. */
{
    register unsigned char *ptr, *parent;
    register unsigned char *snext;
    register unsigned int sym, count;
    register int max_order, pos;
    register boolean is_leaf, has_foundsymbol;

    has_foundsymbol = FALSE;
    if (PPMq_Models [ppmq_model].P_performs_full_excls)
        memset (PPMq_exclusions, 0,
		(PPMq_Models [ppmq_model].P_alphabet_size+1) * sizeof (boolean));

    /* Find current max order; max order will be less than model's max order
       only for first few symbols in the input sequence while we build up the
       context to max order length */
    max_order = PPMq_Models [ppmq_model].P_max_order;
    if (max_order >= 0)
      for (; max_order > 0; max_order--)
	if (PPMq_Contexts [ppmq_context].P_context [max_order] != NULL)
	  break;

    /* First pass - encode the symbol */
    for (pos = max_order; pos >= 0; pos--)
      { /* create or access nodes in the trie pointed at by PPMq_Contexts [ppmq_context].P_context */
	is_leaf = (pos >= PPMq_Models [ppmq_model].P_max_order);

	/* Follow child pointer for this context i.e. what it predicts */
	parent = PPMq_Contexts [ppmq_context].P_context [pos];
	if (parent == NULL)
	    ptr = PPMq_Models [ppmq_model].P_trie;
	else
	    ptr = PPMq_GET_PTR (parent, PPMq_NODE_CHILD);

	PPMq_reset_encode_counts (ppmq_model);

	count = 0;
	while (ptr != NULL)
	  { /* find out where the symbol is */
	    sym = PPMq_getnode_symbol (ptr, is_leaf);
	    count = PPMq_getnode_count (ptr, is_leaf);
	    snext = PPMq_GET_PTR (ptr, PPMq_NODE_NEXT);
	    if (sym == PPMq_symbol)
	      { /* found symbol */
		has_foundsymbol = TRUE;
		PPMq_lbnd = PPMq_totl;
		PPMq_hbnd = PPMq_lbnd + count;
	      }
	    if (!PPMq_Models [ppmq_model].P_performs_full_excls || !PPMq_exclusions [sym])
	      {
	        PPMq_totl += count;
		PPMq_escape++;
		if (PPMq_Models [ppmq_model].P_performs_full_excls)
		    PPMq_exclusions [sym] = TRUE;
	      }
	    ptr = snext;
	  }

	/* encode the symbol or an escape */
	if (PPMq_escape)
	  {
	    if (!has_foundsymbol)
	      { /* encode an escape */
		PPMq_lbnd = PPMq_totl;
		PPMq_hbnd = PPMq_lbnd + PPMq_escape;
	      }
	    PPMq_totl += PPMq_escape;
	    PPMq_encode (pos, coder, PPMq_lbnd, PPMq_hbnd, PPMq_totl);
	  }
	if (has_foundsymbol)
	    break;
      }

    if (!has_foundsymbol)
        PPMq_encode_order_minus1 (ppmq_model, coder); /* Order -1 needs to be encoded */

    /* Second pass - update the counts for the PPMq_symbol */
    PPMq_update_counts (ppmq_model, ppmq_context, increment);
}

void
PPMq_decode_counts (unsigned int ppmq_model, unsigned int ppmq_context,
		    unsigned int coder, int increment)
/* Decodes and updates the model's counts for the new symbol. */
{
    register unsigned char *ptr, *parent;
    register unsigned char *snext;
    register unsigned int sym, count;
    register int max_order, pos;
    register boolean is_leaf, has_coded;

    if (PPMq_Models [ppmq_model].P_performs_full_excls)
        memset (PPMq_exclusions, 0,
		(PPMq_Models [ppmq_model].P_alphabet_size+1) * sizeof (boolean));

    /* Find current max order; max order will be less than model's max order
       only for first few symbols in the input sequence while we build up the
       context to max order length */
    max_order = PPMq_Models [ppmq_model].P_max_order;
    if (max_order >= 0)
      for (; max_order > 0; max_order--)
	if (PPMq_Contexts [ppmq_context].P_context [max_order] != NULL)
	  break;

    /* First pass - decode the symbol */
    has_coded = FALSE;
    for (pos = max_order; pos >= 0; pos--)
      { /* create or access nodes in the trie pointed at by PPMq_Contexts [ppmq_context].P_context */
	is_leaf = (pos >= PPMq_Models [ppmq_model].P_max_order);

	/* Follow child pointer for this context i.e. what it predicts */
	parent = PPMq_Contexts [ppmq_context].P_context [pos];
	if (parent == NULL)
	    ptr = PPMq_Models [ppmq_model].P_trie;
	else
	    ptr = PPMq_GET_PTR (parent, PPMq_NODE_CHILD);

	PPMq_reset_decode_counts (ppmq_model);

	count = 0;
	while (ptr != NULL)
	  { /* find out where the symbol is */
	    sym = PPMq_getnode_symbol (ptr, is_leaf);
	    count = PPMq_getnode_count (ptr, is_leaf);
	    snext = PPMq_GET_PTR (ptr, PPMq_NODE_NEXT);
	    if (!PPMq_Models [ppmq_model].P_performs_full_excls || !PPMq_exclusions [sym])
	      {
	        PPMq_totl += count;
		PPMq_escape++;
		if (PPMq_Models [ppmq_model].P_performs_full_excls)
		    PPMq_exclusions [sym] = TRUE;
		/* Save the ranges so that they can be used to find the symbol
		   (during decoding only) */
		PPMq_decoding_symbols [PPMq_decoding_pos++] = sym;
		PPMq_decoding_ranges [PPMq_decoding_pos] = PPMq_totl;
	      }
	    ptr = snext;
	  }

	/* decode the symbol or an escape */
	if (PPMq_escape)
	  {
	    PPMq_totl += PPMq_escape;
	    PPMq_target = PPMq_decode_target (pos, coder, PPMq_totl);
	    has_coded = PPMq_decode_range (ppmq_model, PPMq_target);
	    PPMq_decode (pos, coder, PPMq_target, PPMq_lbnd, PPMq_hbnd, PPMq_totl);
	    if (has_coded)
	        break;
	  }
      }

    if (!has_coded)
        PPMq_decode_order_minus1 (ppmq_model, coder); /* Order -1 needs to be decoded */

    /* Second pass - update the counts for the PPMq_symbol */
    PPMq_update_counts (ppmq_model, ppmq_context, increment);
}

unsigned int *PPMq_dump_symbols = NULL;

void PPMq_dump_trie (unsigned int file, unsigned int ppmq_model,
		     unsigned char *ptr, unsigned int pos,
		     void (*dump_symbol_function) (unsigned int, unsigned int))
/* Recursive routine used by PPMq_dump_model. */
{
    unsigned int p, symbol, count;
    unsigned char *snext, *child;
    int max_order;

    max_order = PPMq_Models [ppmq_model].P_max_order;
    while (ptr != NULL)
      { /* proceed through the symbol list */
	symbol = PPMq_getnode_symbol (ptr, (pos == max_order));
	count = PPMq_getnode_count (ptr, (pos == max_order));
	snext = PPMq_GET_PTR (ptr, PPMq_NODE_NEXT);
	if (pos >= max_order)
	    child = NULL;
	else
	    child = PPMq_GET_PTR (ptr, PPMq_NODE_CHILD);

	fprintf (Files [file], "%d %9p %9p %9p [", pos, ptr, snext, child);
	for (p = 0; p < pos; p++)
	  {
	    PPMq_dump_symbol (file, PPMq_dump_symbols [p], dump_symbol_function);
	    fprintf (Files [file], "," );
	  }
	PPMq_dump_symbol (file, symbol, dump_symbol_function);
	fprintf (Files [file], "] count = %d\n", count);

	if (child != NULL)
	  {
	    PPMq_dump_symbols [pos] = symbol;
	    PPMq_dump_trie (file, ppmq_model, child, pos+1, dump_symbol_function);
	  }
	ptr = snext;
      }
}

void
PPMq_dump_model (unsigned int file, unsigned int ppmq_model,
		 void (*dump_symbol_function) (unsigned int, unsigned int))
/* Dumps out the PPMq model (for debugging purposes). */
{
    fprintf (Files [file], "Max order of model = %d\n",
	     PPMq_Models [ppmq_model].P_max_order);

    fprintf (Files [file], "Escape method = method ");
    switch (PPMq_Models [ppmq_model].P_escape_method)
      {
      case TLM_PPM_Method_A:
	fprintf (Files [file], "A\n");
	break;
      case TLM_PPM_Method_B:
	fprintf (Files [file], "B\n");
	break;
      case TLM_PPM_Method_C:
	fprintf (Files [file], "C\n");
	break;
      case TLM_PPM_Method_D:
	fprintf (Files [file], "D\n");
	break;
      default:
	fprintf (Files [file], "*** invalid method ***\n");
	break;
      }

    if (PPMq_Models [ppmq_model].P_performs_full_excls)
        fprintf (Files [file], "Performs excls = TRUE\n");
    else
        fprintf (Files [file], "Performs excls = FALSE\n");

    fprintf (Files [file], "Dump of trie: (depth, node ptr, next ptr, child ptr, path, count)\n");

    PPMq_dump_symbols = (unsigned int *)
        realloc (PPMq_dump_symbols, (PPMq_Models [ppmq_model].P_max_order+1) *
		 sizeof (unsigned int));

    PPMq_dump_trie (file, ppmq_model, PPMq_Models [ppmq_model].P_trie, 0,
		    dump_symbol_function);
}

void
PPMq_update_context (unsigned int model, unsigned int context,
		     unsigned int symbol)
/* Updates the context record so that the current symbol becomes symbol.
   This routine just updates the context, and does not return
   any additional information such as the cost of encoding
   the symbol in bits. */
{
    unsigned int ppmq_model, ppmq_context, incr;

    ppmq_model = TLM_verify_model (model, TLM_PPMq_Model, PPMq_valid_model);

    ppmq_context = context;
    assert (PPMq_valid_context (ppmq_context));
    assert ((PPMq_Models [ppmq_model].P_escape_method == TLM_PPM_Method_C) ||
	    (PPMq_Models [ppmq_model].P_escape_method == TLM_PPM_Method_D));

    PPMq_symbol = symbol;

    /* update the trie and context */
    if (PPMq_Models [ppmq_model].P_escape_method == TLM_PPM_Method_C)
        incr = PPMq_INCR_C;
    else
        incr = PPMq_INCR_D;
    PPMq_update_counts (ppmq_model, ppmq_context, incr);
}

void
PPMq_encode_symbol (unsigned int model, unsigned int context,
		    unsigned int coder, unsigned int symbol)
/* Encodes the specified symbol using the arithmetic coder.
   The context is updated so that the last symbol becomes
   the encoded symbol. */
{
    unsigned int ppmq_model, ppmq_context, incr;

    ppmq_model = TLM_verify_model (model, TLM_PPMq_Model, PPMq_valid_model);

    ppmq_context = context;
    assert (PPMq_valid_context (ppmq_context));
    assert ((PPMq_Models [ppmq_model].P_escape_method == TLM_PPM_Method_C) ||
	    (PPMq_Models [ppmq_model].P_escape_method == TLM_PPM_Method_D));
    PPMq_symbol = symbol;

    /* update the trie and context */
    if (PPMq_Models [ppmq_model].P_escape_method == TLM_PPM_Method_C)
        incr = PPMq_INCR_C;
    else
        incr = PPMq_INCR_D;
    PPMq_encode_counts (ppmq_model, context, coder, incr);
}

unsigned int
PPMq_decode_symbol (unsigned int model, unsigned int context,
		    unsigned int coder)
/* Returns the symbol decoded using the arithmetic coder. Updates the
   context record so that the last symbol in the context becomes the
   decoded symbol. */
{
    unsigned int ppmq_model, ppmq_context, incr;

    ppmq_model = TLM_verify_model (model, TLM_PPMq_Model, PPMq_valid_model);

    ppmq_context = context;
    assert (PPMq_valid_context (ppmq_context));
    assert ((PPMq_Models [ppmq_model].P_escape_method == TLM_PPM_Method_C) ||
	    (PPMq_Models [ppmq_model].P_escape_method == TLM_PPM_Method_D));
    assert (coder = TLM_valid_coder (coder));

    /* update the trie and context */
    if (PPMq_Models [ppmq_model].P_escape_method == TLM_PPM_Method_C)
        incr = PPMq_INCR_C;
    else
        incr = PPMq_INCR_D;
    PPMq_decode_counts (ppmq_model, context, coder, incr);

    return (PPMq_symbol);
}
