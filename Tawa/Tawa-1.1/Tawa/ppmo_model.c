/* Routines for PPMo models. */
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <assert.h>
#include <string.h>
#include <math.h>

#include "io.h"
#include "bits.h"
#include "text.h"
#include "model.h"
#include "coder.h"
#include "ppmo_model.h"

#define PPMo_MODELS_SIZE 4              /* Initial max. number of models */
#define PPMo_CONTEXTS_SIZE 128          /* Initial max. number of contexts */

#define PPMo_ORDER_MODEL_MAX_ORDER 7    /* Maximum of order model max order */
#define PPMo_SYMBOL_MODEL_MAX_ORDER 14  /* Maximum of symbol model max order */
/* These two numbers must be specified so that the order context can be stored in a single integer
   whose maximum value specifies the size of the array used to store the context.
   The size of the array is (symbol_model_max_order+2) * (order_model_max_order+2)^order_model_max_order.
   i.e. If order_model_max_order=3 and symbol_model_max_order=2, then array_size = 4*5^3 = 500. */

#define PPMo_MAX_BYTE_COUNT 254         /* Maximum count that fits in a byte without overflow */
#define PPMo_COUNT_OVERFLOW 255         /* Indicates count does not fit in a byte */

/* Increments used to update the counts of the model: */
#define PPMo_INCR 1

/* Offsets to data stored in the model. */
/* These fields are for counts and symbols that can't fit in a byte;
   at the moment, if either count or symbol can't fit, then 8 bytes
   are added to allow room for both possibilities - this could be
   reduced to only 4 extra bytes, but I haven't bothered to go to the
   extra effort although there could be extra wastage since in most
   cases, the symbol number usually fits in a byte only */
/* Offsets to data in node record: */
#define PPMo_NODE_SIZE 10 /* symbol(1) + sibling ptr(4) + child ptr(4) + count(1) */
#define PPMo_NODE_SYMBOL 0
#define PPMo_NODE_NEXT 1
#define PPMo_NODE_CHILD 5
#define PPMo_NODE_COUNT 9

/* Offsets to data in node1 record: */
#define PPMo_NODE1_SIZE 18 /* symbol(1) + sibling ptr(4) + child ptr(4) + count(1) +
			      count1(4) + symbol1(4) */
#define PPMo_NODE1_COUNT 10
#define PPMo_NODE1_SYMBOL 14

/* Offsets to data in leaf record: */
#define PPMo_LEAF_SIZE 6 /* symbol(1) + sibling ptr(4) + count(1) */
#define PPMo_LEAF_SYMBOL 0
#define PPMo_LEAF_NEXT 1
#define PPMo_LEAF_COUNT 5

/* Offsets to data in leaf1 record: */
#define PPMo_LEAF1_SIZE 14 /* symbol(1) + sibling ptr(4) + count(1) + count1(4) +
			      symbol1(4) */
#define PPMo_LEAF1_COUNT 6
#define PPMo_LEAF1_SYMBOL 10

#define PPMo_LARGE_SYMBOL 254   /* Indicates that symbol doesn't fit in a byte */
#define PPMo_NOMORE_SYMBOLS 255 /* Indicates no more siblings in trie */

/* Macros for getting and putting data from context trie nodes */
#define PPMo_GET_CHAR(node,field)    *(node + field)
#define PPMo_PUT_CHAR(node,field,n)  *(node + field) = n
#define PPMo_GET_PTR(node,field)     (unsigned char *) *((unsigned int *) (node + field))
#define PPMo_PUT_PTR(node,field,ptr) (*((unsigned int *) (node + field))) = \
                                       (unsigned int) ptr
#define PPMo_GET_INT(node,field)     (unsigned int) *((unsigned int *) (node + field))
#define PPMo_PUT_INT(node,field,n)   (*((unsigned int *) (node + field))) = n
#define PPMo_INC_INT(node,field,n)   (*((unsigned int *) (node + field))) += n

/* Global variables when debugging the different codelengths */
float PPMo_Codelength_Orders = 0;       /* Total cost of encoding the orders */
float PPMo_Codelength_Symbols = 0;      /* Total cost of encoding the symbols */

/* Global variables used for storing the PPMo models */
struct PPMo_modelType *PPMo_Models;     /* List of PPMo models */
unsigned int PPMo_Models_max_size = 0;  /* Current max. size of models array */
unsigned int PPMo_Models_used = NIL;    /* List of deleted model records */
unsigned int PPMo_Models_unused = 1;    /* Next unused model record */

/* Global variables used for storing the PPMo contexts */
struct PPMo_contextType *PPMo_Contexts; /* List of PPMo contexts */
unsigned int PPMo_Contexts_max_size = 0;/* Current max. size of models array */
unsigned int PPMo_Contexts_used = NIL;  /* List of deleted model records */
unsigned int PPMo_Contexts_unused = 1;  /* Next unused model record */

/* Global variables used for storing PPMo statistics */
unsigned int PPMo_determ_symbols = 0;   /* Number of deterministic symbols */
unsigned int PPMo_nondeterm_symbols = 0;/* Number of non-deterministic symbols */

FILE *PPMo_Orders_fp = NULL;       /* File used for writing out the orders */

float ln2 = 0;

void PPMo_junk ()
/* For debugging purposes. */
{
  fprintf (stderr, "Got here\n");
}

void
PPMo_dump_symbol (unsigned int file, unsigned int symbol,
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

boolean
PPMo_valid_model (unsigned int ppmo_model)
/* Returns non-zero if the PPM model is valid, zero otherwize. */
{
    if (ppmo_model == NIL)
        return (FALSE);
    else if (ppmo_model >= PPMo_Models_unused)
        return (FALSE);
    else if (PPMo_Models [ppmo_model].P_symbol_model_max_order < -1)
        /* The max order gets set to -2 when the model gets deleted;
	   this way you can test to see if the model has been deleted or not */
        return (FALSE);
    else
        return (TRUE);
}

unsigned int
create_PPMo_model (void)
/* Creates and returns a new pointer to a PPMo model record. */
{
    unsigned int ppmo_model, old_size;

    if (PPMo_Models_used != NIL)
    { /* use the first record on the used list */
        ppmo_model = PPMo_Models_used;
	PPMo_Models_used = PPMo_Models [ppmo_model].P_next;
    }
    else
    {
	ppmo_model = PPMo_Models_unused;
        if (PPMo_Models_unused >= PPMo_Models_max_size)
	{ /* need to extend PPMo_Models array */
	    old_size = PPMo_Models_max_size * sizeof (struct modelType);
	    if (PPMo_Models_max_size == 0)
	        PPMo_Models_max_size = PPMo_MODELS_SIZE;
	    else
	        PPMo_Models_max_size = 10*(PPMo_Models_max_size+50)/9;

	    PPMo_Models = (struct PPMo_modelType *)
	        Realloc (87, PPMo_Models, PPMo_Models_max_size *
			 sizeof (struct PPMo_modelType), old_size);

	    if (PPMo_Models == NULL)
	    {
	        fprintf (stderr, "Fatal error: out of PPM models space\n");
		exit (1);
	    }
	}
	PPMo_Models_unused++;
    }

    PPMo_Models [ppmo_model].P_order_model_max_order = PPMo_DEFAULT_ORDER_MODEL_ORDER;
    PPMo_Models [ppmo_model].P_symbol_model_max_order = PPMo_DEFAULT_SYMBOL_MODEL_ORDER;
    PPMo_Models [ppmo_model].P_alphabet_size = PPMo_DEFAULT_ALPHABET_SIZE;
    PPMo_Models [ppmo_model].P_performs_full_excls = PPMo_DEFAULT_PERFORMS_FULL_EXCLS;
    PPMo_Models [ppmo_model].P_OS_model_threshold = PPMo_DEFAULT_OS_MODEL_THRESHOLD;

    PPMo_Models [ppmo_model].P_trie = NULL;
    PPMo_Models [ppmo_model].P_next = NIL;

    return (ppmo_model);
}

void
PPMo_set_mask_orders (unsigned int ppmo_model, unsigned int mask_orders)
/* Sets the orders which are to be masked based on the text record mask_orders. */
{
    unsigned int mask_this_order, p;

    assert (TXT_valid_text (mask_orders));

    p = 0;
    while (TXT_get_symbol (mask_orders, p, &mask_this_order))
      {
	if (mask_this_order <= PPMo_Models [ppmo_model].P_order_model_max_order)
	      PPMo_Models [ppmo_model].P_mask_orders [mask_this_order] = TRUE;
	p++;
      }
}

unsigned int
PPMo_create_model
(unsigned int alphabet_size, int order_model_max_order, int symbol_model_max_order,
 unsigned int mask_orders, int os_model_threshold, boolean performs_full_excls)
/* Creates and returns a new pointer to a PPMo model record. */
{
    unsigned int ppmo_model, k;

    ppmo_model = create_PPMo_model ();

    PPMo_Models [ppmo_model].P_trie = NULL;
    assert (alphabet_size != 0);
    PPMo_Models [ppmo_model].P_alphabet_size = alphabet_size;

    assert ((order_model_max_order >= 0) && (order_model_max_order <= PPMo_ORDER_MODEL_MAX_ORDER));
    assert (symbol_model_max_order >= -1);
    if (symbol_model_max_order < 0)
        /* The (order -1) models never need to perform any exclusions */
        performs_full_excls = FALSE;

    assert ((symbol_model_max_order >= 0) && (symbol_model_max_order <= PPMo_SYMBOL_MODEL_MAX_ORDER));

    PPMo_Models [ppmo_model].P_order_model_max_order = order_model_max_order;
    PPMo_Models [ppmo_model].P_symbol_model_max_order = symbol_model_max_order;

    PPMo_Models [ppmo_model].P_performs_full_excls = performs_full_excls;

    PPMo_Models [ppmo_model].P_OS_model_threshold = os_model_threshold;
    if (os_model_threshold < 0)
        PPMo_Models [ppmo_model].P_OS_model = NULL;
    else
        PPMo_Models [ppmo_model].P_OS_model = (unsigned int **)
	  Calloc (97, alphabet_size+1, sizeof (unsigned int *));

    PPMo_Models [ppmo_model].P_trie = NULL;
    PPMo_Models [ppmo_model].P_alphabet_size = alphabet_size;

    PPMo_Models [ppmo_model].P_mask_orders = (boolean *)
      Calloc (96, symbol_model_max_order+2, sizeof (boolean));

    PPMo_Models [ppmo_model].P_orders_size = symbol_model_max_order + 2;
    for (k = 0; k < order_model_max_order; k++)
        PPMo_Models [ppmo_model].P_orders_size *= symbol_model_max_order + 2;

    PPMo_Models [ppmo_model].P_orders_model = (unsigned int *)
      Calloc (95, PPMo_Models [ppmo_model].P_orders_size, sizeof (unsigned int));

    if (!PPMo_Models [ppmo_model].P_performs_full_excls)
        PPMo_Models [ppmo_model].P_exclusions = NULL;
    else
      {
        PPMo_Models [ppmo_model].P_exclusions = NULL;
	BITS_ALLOC (PPMo_Models [ppmo_model].P_exclusions);
	BITS_ZERO (PPMo_Models [ppmo_model].P_exclusions);
      }

    PPMo_set_mask_orders (ppmo_model, mask_orders);

    return (ppmo_model);
}

void
PPMo_get_model (unsigned int model, unsigned int type, unsigned int *value)
/* Returns information about the PPM model. The arguments type is
   the information to be returned (i.e. PPMo_Get_Alphabet_Size,
   PPMo_Get_Max_Symbol, PPMo_Get_Order_Model_Max_Order, PPMo_Get_Symbol_Model_Max_Order,
   PPMo_Get_OS_Model_Threshold, or PPMo_Get_Performs_Full_Excls). */
{
    unsigned int ppmo_model;

    ppmo_model = TLM_verify_model (model, TLM_PPMo_Model, PPMo_valid_model);

    switch (type)
      {
      case PPMo_Get_Alphabet_Size:
	*value = PPMo_Models [ppmo_model].P_alphabet_size;
	break;
      case PPMo_Get_Order_Model_Max_Order:
	*value = (unsigned int) PPMo_Models [ppmo_model].P_order_model_max_order;
	break;
      case PPMo_Get_Symbol_Model_Max_Order:
	*value = (unsigned int) PPMo_Models [ppmo_model].P_symbol_model_max_order;
	break;
      case PPMo_Get_OS_Model_Threshold:
	*value = (unsigned int) PPMo_Models [ppmo_model].P_OS_model_threshold;
	break;
      case PPMo_Get_Performs_Full_Excls:
	*value = PPMo_Models [ppmo_model].P_performs_full_excls;
	break;
      default:
	fprintf (stderr, "Invalid type (%d) specified for PPMo_get_model ()\n", type);
	exit (1);
	break;
      }
}

void
PPMo_set_model (unsigned int model, unsigned int type, unsigned int value)
/* Sets information about the PPM model. The argument type is the information
   to be set (i.e. PPMo_Set_Alphabet_Size or PPMo_Set_Max_Symbol)
   and its value is set to the argument value.

   The type PPMo_Alphabet_Size is used for specifying the size of the
   expanded alphabet (which must be equal to 0 - indicating an unbounded
   alphabet, or greater than the existing size to accomodate symbols used by
   the existing model). */
{
    unsigned int ppmo_model;
    unsigned int alphabet_size;

    ppmo_model = TLM_verify_model (model, TLM_PPMo_Model, PPMo_valid_model);

    switch (type)
      {
      case PPMo_Set_Alphabet_Size:
	alphabet_size = value;
	assert ((alphabet_size == 0) ||
		(alphabet_size > PPMo_Models [ppmo_model].P_alphabet_size));
	PPMo_Models [ppmo_model].P_alphabet_size = alphabet_size;
	break;
      case PPMo_Set_OS_Model_Threshold:
	alphabet_size = PPMo_Models [ppmo_model].P_alphabet_size;
	if (PPMo_Models [ppmo_model].P_OS_model_threshold >= 0)
	  Free (97, PPMo_Models [ppmo_model].P_OS_model,
	    (alphabet_size+1) * (sizeof (unsigned int *)));
	PPMo_Models [ppmo_model].P_OS_model_threshold = value;
	if (value < 0)
	    PPMo_Models [ppmo_model].P_OS_model = NULL;
	else
	    PPMo_Models [ppmo_model].P_OS_model = (unsigned int **)
	      Calloc (97, alphabet_size+1, sizeof (unsigned int *));
	break;
      default:
	fprintf (stderr, "Invalid type (%d) specified for PPMo_set_model ()\n", type);
	exit (1);
	break;
      }
}

unsigned int
PPMo_eof_symbol (unsigned int ppmo_model)
/* Returns the eof symbol for the PPMo model. */
{
    return (PPMo_Models [ppmo_model].P_alphabet_size); /* Special symbol not in alphabet */
}

void
PPMo_release_trie (unsigned int ppmo_model, unsigned char *ptr, unsigned int pos)
/* Recursive routine to release the trie. */
{
    unsigned int symbol, offset, size;
    unsigned char *snext, *child;
    boolean is_leaf;
    int symbol_model_max_order;

    symbol_model_max_order = PPMo_Models [ppmo_model].P_symbol_model_max_order;
    is_leaf = (pos >= symbol_model_max_order);
    while (ptr != NULL)
      { /* proceed through the symbol list */
	snext = PPMo_GET_PTR (ptr, PPMo_NODE_NEXT);
	if (is_leaf)
	    child = NULL;
	else
	    child = PPMo_GET_PTR (ptr, PPMo_NODE_CHILD);

	if (child != NULL)
	  {
	    PPMo_release_trie (ppmo_model, child, pos+1);
	  }

	if (is_leaf)
	    offset = PPMo_LEAF_SYMBOL;
	else
	    offset = PPMo_NODE_SYMBOL;

	symbol = PPMo_GET_CHAR (ptr, offset);
	if (is_leaf)
	  {
	    if (symbol == PPMo_LARGE_SYMBOL)
	      size = PPMo_LEAF1_SIZE;
	    else
	      size = PPMo_LEAF_SIZE;
	  }
	else
	  {
	    if (symbol == PPMo_LARGE_SYMBOL)
	      size = PPMo_NODE1_SIZE;
	    else
	      size = PPMo_NODE_SIZE;
	  }

	Free (90, ptr, size);
	ptr = snext;
      }
}

void
PPMo_release_model (unsigned int ppmo_model)
/* Releases the memory allocated to the model and the model number (which may
   be reused in later PPMo_create_model or PPMo_load_model calls).
   A run-time error will be generated if an attempt is made to release
   a model that still has active contexts pointing at it. */
{
    int omax_order, smax_order, os_model_threshold;
    unsigned int alphabet_size, sym;

    omax_order = PPMo_Models [ppmo_model].P_order_model_max_order;
    smax_order = PPMo_Models [ppmo_model].P_order_model_max_order;
    alphabet_size = PPMo_Models [ppmo_model].P_alphabet_size;
    os_model_threshold = PPMo_Models [ppmo_model].P_OS_model_threshold;

    assert (PPMo_valid_model (ppmo_model));

    /* Release the model's trie as well */
    PPMo_release_trie (ppmo_model, PPMo_Models [ppmo_model].P_trie, 0);

    /* add model record at the head of the PPMo_Models_used list */
    PPMo_Models [ppmo_model].P_next = PPMo_Models_used;
    PPMo_Models_used = ppmo_model;

    Free (95, PPMo_Models [ppmo_model].P_orders_model,
	  PPMo_Models [ppmo_model].P_orders_size * (sizeof (unsigned int)));
    Free (96, PPMo_Models [ppmo_model].P_mask_orders,
	  (smax_order+2) * (sizeof (boolean)));
    if (os_model_threshold >= 0)
      {
	for (sym = 0; sym <= alphabet_size; sym++)
	  if (PPMo_Models [ppmo_model].P_OS_model [sym] != NULL)
	      Free (98, PPMo_Models [ppmo_model].P_OS_model [sym],
		    (smax_order+2) * (sizeof (unsigned int)));
	      
	Free (97, PPMo_Models [ppmo_model].P_OS_model,
	      (alphabet_size+1) * (sizeof (unsigned int *)));
      }

    if (PPMo_Models [ppmo_model].P_performs_full_excls)
        BITS_FREE( PPMo_Models [ppmo_model].P_exclusions);

    PPMo_Models [ppmo_model].P_symbol_model_max_order = -2;
    /* This is used for testing if model no. is valid or not */
}

unsigned char *
PPMo_create_node (unsigned int symbol)
{
    unsigned int size, bytes;
    unsigned char *ptr;
    boolean is_small_record;

    is_small_record = (symbol < PPMo_LARGE_SYMBOL);
    if (is_small_record)
        size = PPMo_NODE_SIZE;
    else
        size = PPMo_NODE1_SIZE;

    /* Create the node */
    bytes = size * sizeof (unsigned char);
    ptr = (unsigned char *) calloc (bytes, 1);
    assert (ptr != NULL);

    /*fprintf (stderr, "Created node ptr %p for symbol %d\n", ptr, symbol);*/

    if (is_small_record)
        PPMo_PUT_CHAR (ptr, PPMo_NODE_SYMBOL, symbol);
    else
      { /* Either the symbol or count can't fit in a byte */
        PPMo_PUT_CHAR (ptr, PPMo_NODE_SYMBOL, PPMo_LARGE_SYMBOL);
	PPMo_PUT_INT (ptr, PPMo_NODE1_SYMBOL, symbol);
      }
    return (ptr);
}

unsigned char *
PPMo_create_leaf (unsigned int symbol)
{
    unsigned int size, bytes;
    unsigned char *ptr;
    boolean is_small_record;

    is_small_record = (symbol < PPMo_LARGE_SYMBOL);
    if (is_small_record)
        size = PPMo_LEAF_SIZE;
    else
        size = PPMo_LEAF1_SIZE;

    /* Create the leaf */
    bytes = size * sizeof (unsigned char);
    ptr = (unsigned char *) calloc (bytes, 1);
    assert (ptr != NULL);

    /*fprintf (stderr, "Created leaf ptr %p for symbol %d\n", ptr, symbol);*/

    if (is_small_record)
        PPMo_PUT_CHAR (ptr, PPMo_LEAF_SYMBOL, symbol);
    else
      { /* Either the symbol or count can't fit in a byte */
        PPMo_PUT_CHAR (ptr, PPMo_LEAF_SYMBOL, PPMo_LARGE_SYMBOL);
	PPMo_PUT_INT (ptr, PPMo_LEAF1_SYMBOL, symbol);
      }
    return (ptr);
}

/* PPMo_create_node1 and PPMo_create_leaf1 are separate routines to speed things up
   a wee bit */

unsigned char *
PPMo_create_node1 (unsigned int symbol, unsigned int count)
{
    unsigned int size, bytes;
    unsigned char *ptr;
    boolean is_small_record;

    is_small_record = (symbol < PPMo_LARGE_SYMBOL) && (count <= PPMo_MAX_BYTE_COUNT);
    if (is_small_record)
        size = PPMo_NODE_SIZE;
    else
        size = PPMo_NODE1_SIZE;

    /* Create the node */
    bytes = size * sizeof (unsigned char);
    ptr = (unsigned char *) calloc (bytes, 1);
    assert (ptr != NULL);

    /*fprintf (stderr, "Created node ptr %p for symbol %d\n", ptr, symbol);*/

    if (is_small_record)
      {
        PPMo_PUT_CHAR (ptr, PPMo_NODE_SYMBOL, symbol);
	PPMo_PUT_CHAR (ptr, PPMo_NODE_COUNT, count);
      }
    else
      { /* Either the symbol or count can't fit in a byte */
        PPMo_PUT_CHAR (ptr, PPMo_NODE_SYMBOL, PPMo_LARGE_SYMBOL);
        PPMo_PUT_INT (ptr, PPMo_NODE1_SYMBOL, symbol);
	PPMo_PUT_CHAR (ptr, PPMo_NODE_COUNT, PPMo_COUNT_OVERFLOW);
	PPMo_PUT_INT (ptr, PPMo_NODE1_COUNT, count);
      }
    return (ptr);
}

unsigned char *
PPMo_create_leaf1 (unsigned int symbol, unsigned int count)
{
    unsigned int size, bytes;
    unsigned char *ptr;
    boolean is_small_record;

    is_small_record = (symbol < PPMo_LARGE_SYMBOL) && (count <= PPMo_MAX_BYTE_COUNT);
    if (is_small_record)
        size = PPMo_LEAF_SIZE;
    else
        size = PPMo_LEAF1_SIZE;

    /* Create the leaf */
    bytes = size * sizeof (unsigned char);
    ptr = (unsigned char *) calloc (bytes, 1);
    assert (ptr != NULL);

    /*fprintf (stderr, "Created leaf ptr %p for symbol %d\n", ptr, symbol);*/

    if (is_small_record)
      {
        PPMo_PUT_CHAR (ptr, PPMo_LEAF_SYMBOL, symbol);
	PPMo_PUT_CHAR (ptr, PPMo_LEAF_COUNT, count);
      }
    else
      { /* Either the symbol or count can't fit in a byte */
        PPMo_PUT_CHAR (ptr, PPMo_LEAF_SYMBOL, PPMo_LARGE_SYMBOL);
	PPMo_PUT_INT (ptr, PPMo_LEAF1_SYMBOL, symbol);
	PPMo_PUT_CHAR (ptr, PPMo_LEAF_COUNT, PPMo_COUNT_OVERFLOW);
	PPMo_PUT_INT (ptr, PPMo_LEAF1_COUNT, count);
      }
    return (ptr);
}

unsigned int
PPMo_getnode_symbol (unsigned char *trie_ptr, boolean is_leaf)
/* Returns the count for the node at trie_ptr */
{
    unsigned int symbol;

    if (is_leaf)
        symbol = PPMo_GET_CHAR (trie_ptr, PPMo_LEAF_SYMBOL);
    else
        symbol = PPMo_GET_CHAR (trie_ptr, PPMo_NODE_SYMBOL);
    if (symbol == PPMo_LARGE_SYMBOL)
      {
	if (is_leaf)
	    symbol = PPMo_GET_INT (trie_ptr, PPMo_LEAF1_SYMBOL);
	else
	    symbol = PPMo_GET_INT (trie_ptr, PPMo_NODE1_SYMBOL);
      }
    return (symbol);
}

unsigned int
PPMo_getnode_count (unsigned char *trie_ptr, boolean is_leaf)
/* Returns the count for the node at trie_ptr */
{
    unsigned int count;

    if (is_leaf)
        count = PPMo_GET_CHAR (trie_ptr, PPMo_LEAF_COUNT);
    else
        count = PPMo_GET_CHAR (trie_ptr, PPMo_NODE_COUNT);
    if (count == PPMo_COUNT_OVERFLOW)
      {
	if (is_leaf)
	    count = PPMo_GET_INT (trie_ptr, PPMo_LEAF1_COUNT);
	else
	    count = PPMo_GET_INT (trie_ptr, PPMo_NODE1_COUNT);
      }
    return (count);
}

boolean
PPMo_valid_context (unsigned int ppmo_context)
/* Returns non-zero if the PPM context is valid, zero otherwize. */
{
    if (ppmo_context == NIL)
        return (FALSE);
    else if (ppmo_context >= PPMo_Contexts_unused)
        return (FALSE);
    else if (PPMo_Contexts [ppmo_context].P_symbol_context == NULL)
        /* The symbol_context field gets set to NULL when the context gets deleted;
	   this way you can test to see if the context has been deleted or not */
        return (FALSE);
    else
        return (TRUE);
}

unsigned int
create_PPMo_context (void)
/* Creates and returns a new pointer to a PPMo context record. */
{
    unsigned int ppmo_context, old_size;

    if (PPMo_Contexts_used != NIL)
    { /* use the first record on the used list */
        ppmo_context = PPMo_Contexts_used;
	PPMo_Contexts_used = PPMo_Contexts [ppmo_context].P_next;
    }
    else
    {
	ppmo_context = PPMo_Contexts_unused;
        if (PPMo_Contexts_unused >= PPMo_Contexts_max_size)
	{ /* need to extend PPMo_Contexts array */
	    old_size = PPMo_Contexts_max_size * sizeof (struct PPMo_contextType);
	    if (PPMo_Contexts_max_size == 0)
	        PPMo_Contexts_max_size = PPMo_CONTEXTS_SIZE;
	    else
	        PPMo_Contexts_max_size = 10*(PPMo_Contexts_max_size+50)/9;

	    PPMo_Contexts = (struct PPMo_contextType *)
	        Realloc (84, PPMo_Contexts, PPMo_Contexts_max_size *
			 sizeof (struct PPMo_contextType), old_size);

	    if (PPMo_Contexts == NULL)
	    {
	        fprintf (stderr, "Fatal error: out of PPM contexts space\n");
		exit (1);
	    }
	}
	PPMo_Contexts_unused++;
    }

    PPMo_Contexts [ppmo_context].P_symbol_context = NULL;
    PPMo_Contexts [ppmo_context].P_next = NIL;

    return (ppmo_context);
}

unsigned int
PPMo_create_context (unsigned int model)
/* Creates and returns a new pointer to a PPMo context record. */
{
    unsigned int ppmo_context, ppmo_model;

    ppmo_model = TLM_verify_model (model, TLM_PPMo_Model, PPMo_valid_model);

    ppmo_context = create_PPMo_context ();

    if (PPMo_Models [ppmo_model].P_symbol_model_max_order >= 0)
      PPMo_Contexts [ppmo_context].P_symbol_context = (unsigned char **)
        Calloc (93, PPMo_Models [ppmo_model].P_symbol_model_max_order+1,
		sizeof (unsigned char *));

    PPMo_Contexts [ppmo_context].P_prev_symbol = PPMo_eof_symbol (ppmo_model);

    return (ppmo_context);
}

void
PPMo_release_context (unsigned int model, unsigned int context)
/* Releases the memory allocated to the PPMo context and the context number
   (which may be reused in later PPM_create_context calls). */
{
    unsigned int ppmo_model, ppmo_context;

    ppmo_model = TLM_verify_model (model, TLM_PPMo_Model, PPMo_valid_model);

    ppmo_context = context;
    assert (PPMo_valid_context (ppmo_context));

    if (PPMo_Models [ppmo_model].P_symbol_model_max_order >= 0)
      Free (93, PPMo_Contexts [ppmo_context].P_symbol_context,
	  (PPMo_Models [ppmo_model].P_symbol_model_max_order+1) * sizeof (unsigned char *));
    PPMo_Contexts [ppmo_context].P_symbol_context = NULL;

    /* add context record at the head of the PPMo_Contexts_used list */
    PPMo_Contexts [ppmo_context].P_next = PPMo_Contexts_used;
    PPMo_Contexts_used = ppmo_context;
}

unsigned int
PPMo_getcount_order_context (unsigned int ppmo_model, unsigned int ppmo_context,
			     int order)
/* Returns the count associated with the order in the order stream context. */
{
    unsigned int count = 0, omax_order, smax_order, order_context;

    omax_order = PPMo_Models [ppmo_model].P_order_model_max_order;
    smax_order = PPMo_Models [ppmo_model].P_symbol_model_max_order;

    if (omax_order == 0)
      order_context = order + 1;
    else
      order_context = PPMo_Contexts [ppmo_context].P_order_context * (smax_order + 2) + order + 1;
    count = PPMo_Models [ppmo_model].P_orders_model [order_context];

    /*
    fprintf (stderr, "order context = %d smax_order = %d order = %d count = %d\n",
	     order_context, smax_order, order, count);
    */

    return count;
}

void
PPMo_update_orders_context (unsigned int ppmo_model, unsigned int ppmo_context, int order,
			    unsigned int prev_symbol, unsigned int model_form)
/* Updates the order model, and rolls the order stream context along by one, and adds the encoded
   order. */
{
    unsigned int k, order_context, divisor;
    int omax_order, smax_order;

    omax_order = PPMo_Models [ppmo_model].P_order_model_max_order;
    smax_order = PPMo_Models [ppmo_model].P_symbol_model_max_order;

    /* Update the order model counts */
    /*
    fprintf (stderr, "order context before = %d ", PPMo_Contexts [ppmo_context].P_order_context);
    */
    if (omax_order == 0)
        order_context = order + 1;
    else
        order_context = PPMo_Contexts [ppmo_context].P_order_context * (smax_order + 2) + order + 1;
    /*
    fprintf (stderr, "order context after = %d order_size = %d smax_order = %d order = %d\n",
	     order_context, PPMo_Models [ppmo_model].P_orders_size, smax_order, order);
    */

    if (model_form != TLM_Static)
      {
	PPMo_Models [ppmo_model].P_orders_model [order_context]++;

	if (PPMo_Models [ppmo_model].P_OS_model_threshold >= 0)
	  {
	    if (PPMo_Models [ppmo_model].P_OS_model [prev_symbol] == NULL)
	      PPMo_Models [ppmo_model].P_OS_model [prev_symbol] =
	        Calloc (98, smax_order+2, sizeof (boolean));
	    PPMo_Models [ppmo_model].P_OS_model [prev_symbol][order + 1]++;
	  }
      }

    /* Roll along order context by one position */
    divisor = 1;
    if (omax_order > 0)
      for (k = 0; k < omax_order-1; k++)
	divisor *= (smax_order + 2);
    if (omax_order == 0)
        PPMo_Contexts [ppmo_context].P_order_context = order + 1;
    else
        PPMo_Contexts [ppmo_context].P_order_context =
	  (PPMo_Contexts [ppmo_context].P_order_context % divisor) * (smax_order + 2) + order + 1;

    /*
    fprintf (stderr, "order context 1 = %d omax_order = %d smax_order = %d order = %d divisor = %d\n",
	     order_context, omax_order, smax_order, order, divisor);
    */

    /*PPMo_dump_orders_model (Stderr_File, ppmo_model);*/
}

boolean
PPMo_order_is_masked (unsigned int ppmo_model, unsigned int ppmo_context, int order)
/* Returns TRUE if the order should be eliminated from the encoding. */
{
    if (order < 0) /* Never mask order -1 */
        return (FALSE);
    else if (PPMo_Models [ppmo_model].P_mask_orders [order + 1])
        return (TRUE);
    else if (order == 0) /* symbol context for order 0 is always NULL */
        return (FALSE);
    else if (PPMo_Contexts [ppmo_context].P_symbol_context [order] == NULL)
        return (TRUE);
    else
        return (FALSE);
}

void
PPMo_encode_order (unsigned int ppmo_model, unsigned int ppmo_context, unsigned int coder,
		   int order, codingType coding_type)
/* Encodes the order and updates the order counts for the new symbol. */
{
    register unsigned int count, prev_symbol, lbnd = 0, hbnd = 0, totl = 0;
    register int ord, smax_order, threshold;
    boolean already_encoded = FALSE;
    float codelength;

    if (PPMo_Orders_fp != NULL)
        fprintf (PPMo_Orders_fp, "%d", order+1);

    smax_order = PPMo_Models [ppmo_model].P_symbol_model_max_order;
    threshold = PPMo_Models [ppmo_model].P_OS_model_threshold;
    prev_symbol = PPMo_Contexts [ppmo_context].P_prev_symbol;

    if (threshold >= 0)
      { /* Possibly use the OS model (i.e. p(O_n | S_n-1)) rather than orders model to encode the order */
	if (PPMo_Models [ppmo_model].P_OS_model [prev_symbol] != NULL)
	  {
	    for (ord = -1; ord <= smax_order; ord++)
	      if (!PPMo_order_is_masked (ppmo_model, ppmo_context, ord))
		{
		  count = PPMo_Models [ppmo_model].P_OS_model [prev_symbol][ord + 1] + 1;
		  if (ord == order)
		    {
		      lbnd = totl;
		      hbnd = lbnd + count;
		    }
		  totl += count;
		}
	    if (totl >= threshold)
	      {
		already_encoded = TRUE;
		
		if (Debug.coder)
		  {
		    fprintf (stderr, "OS encode Pos = %d order = %d prev_symbol = %d\n",
			     bytes_input, order, prev_symbol);
		    fprintf (stderr, "OS encode : lbnd = %d hbnd = %d totl = %d\n", lbnd, hbnd, totl);
		  }
	      }
	  }
      }

    if (!already_encoded)
      { /* Use orders model (i.e. p(O_n | O_n-1...)) instead to encode the order */
	totl = 0;
	for (ord = -1; ord <= smax_order; ord++)
	  if (!PPMo_order_is_masked (ppmo_model, ppmo_context, ord))
	    {
	      count = PPMo_getcount_order_context (ppmo_model, ppmo_context, ord) + 1;
	      /* Ensure non-zero count to thwart zero frequency problem */

	      if (ord == order)
		{
		  lbnd = totl;
		  hbnd = lbnd + count;
		}
	      totl += count;
	    }

	if (Debug.coder)
	  {
	    fprintf (stderr, "Pos = %d order = %d ", bytes_input, order);
	  }
      }

    codelength = Codelength (lbnd, hbnd, totl);

    TLM_Codelength += codelength;
    if (Debug.codelengths)
        PPMo_Codelength_Orders += codelength;
    if (coding_type == ENCODE_TYPE)
        Coders [coder].A_arithmetic_encode
	  (Coders [coder].A_encoder_output_file, lbnd, hbnd, totl);
}

int
PPMo_decode_order (unsigned int ppmo_model, unsigned int ppmo_context, unsigned int coder)
/* Decodes the order and updates the order counts for the new symbol. */
{
    register unsigned int target, count, prev_symbol, lbnd = 0, hbnd = 0, totl = 0;
    register int order, ord, smax_order, threshold;
    boolean already_decoded = FALSE;

    lbnd = 0;
    hbnd = 0;
    totl = 0;
    order = 0;
    smax_order = PPMo_Models [ppmo_model].P_symbol_model_max_order;
    threshold = PPMo_Models [ppmo_model].P_OS_model_threshold;
    prev_symbol = PPMo_Contexts [ppmo_context].P_prev_symbol;

    if (threshold >= 0)
      { /* Possibly use the OS model (i.e. p(O_n | S_n-1)) rather than orders model to decode the order */
	if (PPMo_Models [ppmo_model].P_OS_model [prev_symbol] != NULL)
	  {
	    totl = 0;
	    for (ord = -1; ord <= smax_order; ord++)
	      if (!PPMo_order_is_masked (ppmo_model, ppmo_context, ord))
		{
		  count = PPMo_Models [ppmo_model].P_OS_model [prev_symbol][ord + 1] + 1;
		  totl += count;
		}

	    if (totl >= threshold)
	      { /* i.e. We are going to use OS model to encode the orders */
		already_decoded = TRUE;

		target = Coders [coder].A_arithmetic_decode_target
		  (Coders [coder].A_decoder_input_file, totl);

		/* Now find which order it is */
		for (ord = -1; ord <= smax_order; ord++)
		  if (!PPMo_order_is_masked (ppmo_model, ppmo_context, ord))
		    {
		      count = PPMo_Models [ppmo_model].P_OS_model [prev_symbol][ord + 1] + 1;
		      hbnd += count;
		      if (target < hbnd)
			{
			  order = ord;
			  break;
			}
		      lbnd = hbnd;
		    }

		if (Debug.coder)
		  {
		    fprintf (stderr, "OS decode Pos = %d order = %d prev_symbol = %d\n",
			     bytes_input, order, prev_symbol);
		    fprintf (stderr, "OS decode : lbnd = %d hbnd = %d totl = %d\n", lbnd, hbnd, totl);
		  }
	      }
	  }
      }

    if (!already_decoded)
      { /* i.e. The orders model (i.e. p(O_n | O_n-1...)) was used to encode the order */

	/* Work out total of order model counts first to find decoder target */
	totl = 0;
	for (ord = -1; ord <= smax_order; ord++)
	  if (!PPMo_order_is_masked (ppmo_model, ppmo_context, ord))
	    {
	      count = PPMo_getcount_order_context (ppmo_model, ppmo_context, ord) + 1;
	      /* Ensure non-zero count to thwart zero frequency problem */

	      totl += count;
	    }

	target = Coders [coder].A_arithmetic_decode_target
	  (Coders [coder].A_decoder_input_file, totl);

	/* Now find which order it is */
	for (ord = -1; ord <= PPMo_Models [ppmo_model].P_symbol_model_max_order; ord++)
	  if (!PPMo_order_is_masked (ppmo_model, ppmo_context, ord))
	    {
	      count = PPMo_getcount_order_context (ppmo_model, ppmo_context, ord) + 1;
	      /* Ensure non-zero count to thwart zero frequency problem */

	      hbnd += count;
	      if (target < hbnd)
		{
		  order = ord;
		  break;
		}
	      lbnd = hbnd;
	    }

	if (Debug.coder)
	  {
	    fprintf (stderr, "Order decode Pos = %d order = %d **\n", bytes_input, order);
	    fprintf (stderr, "Order decode : lbnd = %d hbnd = %d totl = %d\n", lbnd, hbnd, totl);
	  }
      }

    Coders [coder].A_arithmetic_decode
      (Coders [coder].A_decoder_input_file, lbnd, hbnd, totl);

    return (order);
}

void
PPMo_update_symbols_context (unsigned int ppmo_model, unsigned int ppmo_context, unsigned int symbol,
			     unsigned int increment, unsigned int model_form)
/* Updates the model's counts for the new symbol. */
{
    register unsigned char *ptr, *old_ptr, *parent;
    register unsigned char *snext, *sprev, *lprev, *lptr;
    register unsigned int sym, count, lcount, offset;
    register int smax_order, pos;
    register boolean is_leaf;

    /* Find current max order; max order will be less than model's max order
       only for first few symbols in the input sequence while we build up the
       context to max order length */
    smax_order = PPMo_Models [ppmo_model].P_symbol_model_max_order;
    if (smax_order >= 0)
      for (; smax_order > 0; smax_order--)
	if (PPMo_Contexts [ppmo_context].P_symbol_context [smax_order] != NULL)
	  break;

    for (pos = smax_order; pos >= 0; pos--)
      { /* create or access nodes in the trie pointed at by PPMo_Contexts [ppmo_context].P_symbol_context */
	is_leaf = (pos >= PPMo_Models [ppmo_model].P_symbol_model_max_order);

	/* Follow child pointer for this context i.e. what it predicts */
	parent = PPMo_Contexts [ppmo_context].P_symbol_context [pos];
	if (parent == NULL)
	    ptr = PPMo_Models [ppmo_model].P_trie;
	else
	    ptr = PPMo_GET_PTR (parent, PPMo_NODE_CHILD);

	sprev = NULL;
	lprev = NULL;
	lptr = NULL;
	old_ptr = NULL;
	count = 0;
	lcount = 0;
	snext = NULL;
	while (ptr != NULL)
	  { /* find out where the symbol is */
	    sym = PPMo_getnode_symbol (ptr, is_leaf);
	    count = PPMo_getnode_count (ptr, is_leaf);
	    snext = PPMo_GET_PTR (ptr, PPMo_NODE_NEXT);
	    if (sym == symbol)
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

	if (model_form != TLM_Static)
	  { /* Model is dynamic - create new nodes and update counts as necessary */
	    old_ptr = ptr;

	    /* Create new node if required */
	    if (ptr == NULL)
	      { /* haven't found the symbol - create it */
		count = 0;
		if (is_leaf)
		    ptr = PPMo_create_leaf (symbol);
		else
		    ptr = PPMo_create_node (symbol);
	      }

	    /* Increment count */
	    if (is_leaf)
	        offset = PPMo_LEAF_COUNT;
	    else
	        offset = PPMo_NODE_COUNT;

	    if (count + increment <= PPMo_MAX_BYTE_COUNT)
	        PPMo_INC_INT (ptr, offset, increment); /* Increment node symbol count */
	    else
	      { /* Overflow of byte count field */
		register unsigned int offset1, size;

		/*fprintf (stderr, "Overflow has occurred\n");*/

		if (is_leaf)
		  {
		    offset1 = PPMo_LEAF1_COUNT;
		    size = PPMo_LEAF1_SIZE;
		  }
		else
		  {
		    offset1 = PPMo_NODE1_COUNT;
		    size = PPMo_NODE1_SIZE;
		  }

		if (PPMo_GET_CHAR (ptr, offset) != PPMo_COUNT_OVERFLOW)
		  {
		    /*
		      fprintf (stderr, "Overflow has occurred");
		      PPMo_dump_model (Stderr_File);
		    */

		    /* Indicate count has overflowed byte */
		    PPMo_PUT_CHAR (ptr, offset, PPMo_COUNT_OVERFLOW);
		    ptr = realloc (ptr, size);
		    PPMo_PUT_INT (ptr, offset1, count);

		    old_ptr = NULL; /* Ensure trie gets updated with re-allocated ptr */
		  }

		PPMo_INC_INT (ptr, offset1, increment);
	      }

	    if (old_ptr == NULL)
	      {
		if (sprev != NULL)
		    PPMo_PUT_PTR (sprev, PPMo_NODE_NEXT, ptr);
		else if (parent == NULL)
		    PPMo_Models [ppmo_model].P_trie = ptr;
		else
	            PPMo_PUT_PTR (parent, PPMo_NODE_CHILD, ptr);
	      }
	    else if ((sprev != NULL) && (lcount > 0))
	      { /* Maintain the list in sorted frequency order */
		if (lcount < count + increment)
		  { /* Move this node ptr up to after lprev and before lptr */
		    PPMo_PUT_PTR (sprev, PPMo_NODE_NEXT, snext);
		    PPMo_PUT_PTR (ptr, PPMo_NODE_NEXT, lptr);
		    if (lprev != NULL)
		        PPMo_PUT_PTR (lprev, PPMo_NODE_NEXT, ptr);
		    else /* move to head of the list */
		      if (parent == NULL)
			  PPMo_Models [ppmo_model].P_trie = ptr;
		      else
			  PPMo_PUT_PTR (parent, PPMo_NODE_CHILD, ptr);
		  }
	      }
	  }

	/* Store this context ptr */
	PPMo_Contexts [ppmo_context].P_symbol_context [pos] = ptr;
      }

    /* Roll all symbol context pointers down one */
    for (pos = PPMo_Models [ppmo_model].P_symbol_model_max_order; pos > 0; pos--)
      {
	PPMo_Contexts [ppmo_context].P_symbol_context [pos] =
	    PPMo_Contexts [ppmo_context].P_symbol_context [pos-1];
      }
    PPMo_Contexts [ppmo_context].P_symbol_context [0] = NULL; /* At the Root */

    PPMo_Contexts [ppmo_context].P_prev_symbol = symbol;
}

void
PPMo_update_counts (unsigned int ppmo_model, unsigned int ppmo_context,
		    unsigned int symbol, unsigned int increment, unsigned int model_form)
/* Updates the model's counts for the new symbol without encoding it. */
{
    register unsigned int lbnd = 0, hbnd = 0, totl = 0;
    register unsigned int symbols, sym, prev_symbol, count;
    register int smax_order, ord, order = -1;
    register unsigned char *ptr, *parent;
    register unsigned char *snext;
    register boolean is_leaf, found_symbol;
    float codelength;

    /* Find current max order; max order will be less than model's max order
       only for first few symbols in the input sequence while we build up the
       context to max order length */
    smax_order = PPMo_Models [ppmo_model].P_symbol_model_max_order;
    if (smax_order >= 0)
      for (; smax_order > 0; smax_order--)
	if (PPMo_Contexts [ppmo_context].P_symbol_context [smax_order] != NULL)
	  break;

    prev_symbol = PPMo_Contexts [ppmo_context].P_prev_symbol;

    /* First pass - find the order that will be used to encode the symbol */
    found_symbol = FALSE;
    for (ord = smax_order; ord >= 0; ord--)
     if (!PPMo_order_is_masked (ppmo_model, ppmo_context, ord))
      { /* create or access nodes in the trie pointed at by PPMo_Contexts [ppmo_context].P_symbol_context */
	is_leaf = (ord >= PPMo_Models [ppmo_model].P_symbol_model_max_order);

	/* Follow child pointer for this context i.e. what it predicts */
	parent = PPMo_Contexts [ppmo_context].P_symbol_context [ord];
	if (parent == NULL)
	    ptr = PPMo_Models [ppmo_model].P_trie;
	else
	    ptr = PPMo_GET_PTR (parent, PPMo_NODE_CHILD);

	symbols = 0;
	count = 0;
	lbnd = 0;
	hbnd = 0;
	totl = 0;
	while (ptr != NULL)
	  { /* find out where the symbol is */
	    symbols++;
	    sym = PPMo_getnode_symbol (ptr, is_leaf);
	    count = PPMo_getnode_count (ptr, is_leaf);
	    snext = PPMo_GET_PTR (ptr, PPMo_NODE_NEXT);
	    if (sym == symbol)
	      { /* found symbol */
		found_symbol = TRUE;
		lbnd = totl;
		hbnd = lbnd + count;
	      }
	    totl += count;
	    ptr = snext;
	  }
	if (found_symbol)
	  {
	    order = ord;
	    break;
	  }
      }

    if (!found_symbol)
      {
	order = -1;
	lbnd = symbol;
	hbnd = symbol+1;
	totl = PPMo_Models [ppmo_model].P_alphabet_size+1;
      }

    /* Now encode the symbol using the maximum order that was found */
    PPMo_encode_order (ppmo_model, ppmo_context, NIL, order, UPDATE_CODELENGTH_TYPE);

    if (Debug.coder)
      {
        fprintf (stderr, "Update PPMo Pos = %d Symbol = %d order = %d **\n", bytes_input, symbol, order);
        fprintf (stderr, "Update PPMo : lbnd = %d hbnd = %d totl = %d\n", lbnd, hbnd, totl);
      }

    codelength = Codelength (lbnd, hbnd, totl);
    TLM_Codelength += codelength;
    if (Debug.codelengths)
        PPMo_Codelength_Symbols += codelength;

    PPMo_update_orders_context (ppmo_model, ppmo_context, order, prev_symbol, model_form);

    /* Second pass - update the counts for the symbol */
    PPMo_update_symbols_context (ppmo_model, ppmo_context, symbol, increment, model_form);
}

void
PPMo_encode_counts (unsigned int ppmo_model, unsigned int ppmo_context, unsigned int coder,
		    unsigned int symbol, unsigned int increment, unsigned int model_form)
/* Encodes and updates the model's counts for the new symbol. */
{
    register unsigned int lbnd = 0, hbnd = 0, totl = 0;
    register unsigned int symbols, sym, prev_symbol, count;
    register int smax_order, ord, order = -1;
    register unsigned char *ptr, *parent;
    register unsigned char *snext;
    register boolean is_leaf, found_symbol;

    /* Find current max order; max order will be less than model's max order
       only for first few symbols in the input sequence while we build up the
       context to max order length */
    smax_order = PPMo_Models [ppmo_model].P_symbol_model_max_order;
    if (smax_order >= 0)
      for (; smax_order > 0; smax_order--)
	if (PPMo_Contexts [ppmo_context].P_symbol_context [smax_order] != NULL)
	  break;

    prev_symbol = PPMo_Contexts [ppmo_context].P_prev_symbol;

    if (PPMo_Models [ppmo_model].P_performs_full_excls)
        BITS_ZERO(PPMo_Models [ppmo_model].P_exclusions);

    /* First pass - find the order that will be used to encode the symbol */
    found_symbol = FALSE;
    for (ord = smax_order; ord >= 0; ord--)
     if (!PPMo_order_is_masked (ppmo_model, ppmo_context, ord))
      { /* create or access nodes in the trie pointed at by PPMo_Contexts [ppmo_context].P_symbol_context */
	is_leaf = (ord >= smax_order);

	/* Follow child pointer for this context i.e. what it predicts */
	parent = PPMo_Contexts [ppmo_context].P_symbol_context [ord];
	if (parent == NULL)
	    ptr = PPMo_Models [ppmo_model].P_trie;
	else
	    ptr = PPMo_GET_PTR (parent, PPMo_NODE_CHILD);

	symbols = 0;
	count = 0;
	lbnd = 0;
	hbnd = 0;
	totl = 0;
	while (ptr != NULL)
	  { /* find out where the symbol is */
	    symbols++;
	    sym = PPMo_getnode_symbol (ptr, is_leaf);
	    count = PPMo_getnode_count (ptr, is_leaf);
	    snext = PPMo_GET_PTR (ptr, PPMo_NODE_NEXT);

	    if (sym == symbol)
	      { /* found symbol */
		found_symbol = TRUE;
		lbnd = totl;
		hbnd = lbnd + count;
	      }

	    if (!PPMo_Models [ppmo_model].P_performs_full_excls ||
		!BITS_ISSET(PPMo_Models [ppmo_model].P_exclusions, sym))
	        totl += count;

	    if (PPMo_Models [ppmo_model].P_performs_full_excls)
	        BITS_SET(PPMo_Models [ppmo_model].P_exclusions, sym);

	    ptr = snext;
	  }
	/*fprintf (stderr, "Ord %d Order %d symbols %d\n",
	           bytes_input, ord, symbols);*/
	if (found_symbol)
	  {
	    order = ord;
	    break;
	  }
      }

    if (!found_symbol)
      {
	order = -1;
	lbnd = symbol;
	hbnd = symbol+1;
	totl = PPMo_Models [ppmo_model].P_alphabet_size+1;
      }

    /* Now encode the symbol using the maximum order that was found */
    PPMo_encode_order (ppmo_model, ppmo_context, coder, order, ENCODE_TYPE);

    if (Debug.coder)
      {
        fprintf (stderr, "Pos = %d Symbol = %d order = %d ", bytes_input, symbol, order);
      }

    if (Debug.codelengths)
        PPMo_Codelength_Symbols += Codelength (lbnd, hbnd, totl);

    if ((lbnd == 0) && (hbnd == totl))
      {
        PPMo_determ_symbols++;  /* Found a deterministic symbol */
	/* No need to encode deterministic context */

	if (Debug.coder)
	    fprintf (stderr, "deterministic\n");
      }
    else
      { /* More than one symbol is predicted */
        PPMo_nondeterm_symbols++;  /* Found a non-deterministic symbol */

        Coders [coder].A_arithmetic_encode
	  (Coders [coder].A_encoder_output_file, lbnd, hbnd, totl);
      }

    PPMo_update_orders_context (ppmo_model, ppmo_context, order, prev_symbol, model_form);

    /* Second pass - update the counts for the symbol */
    PPMo_update_symbols_context (ppmo_model, ppmo_context, symbol, increment, model_form);
}

unsigned int
PPMo_decode_counts (unsigned int ppmo_model, unsigned int ppmo_context, unsigned int coder,
		    unsigned int increment, unsigned int model_form)
/* Decodes and updates the model's counts for the new symbol. */
{
    register unsigned char *ptr, *parent;
    register unsigned char *snext;
    register unsigned int symbol = 0, sym, prev_symbol;
    register unsigned int count, scount, target, lbnd, hbnd, totl;
    register int smax_order, order, ord;
    register boolean is_leaf, has_foundtarget;

    /* Find current max order; max order will be less than model's max order
       only for first few symbols in the input sequence while we build up the
       context to max order length */
    smax_order = PPMo_Models [ppmo_model].P_symbol_model_max_order;
    if (smax_order >= 0)
      for (; smax_order > 0; smax_order--)
	if (PPMo_Contexts [ppmo_context].P_symbol_context [smax_order] != NULL)
	  break;

    prev_symbol = PPMo_Contexts [ppmo_context].P_prev_symbol;

    /* Decode the order used to encode the symbol */
    order = PPMo_decode_order (ppmo_model, ppmo_context, coder);
    assert (order <= smax_order);

    if (order < 0)
      { /* Decoded order -1 symbol */
	symbol = Coders [coder].A_arithmetic_decode_target
	  (Coders [coder].A_decoder_input_file, PPMo_Models [ppmo_model].P_alphabet_size+1);
	Coders [coder].A_arithmetic_decode
	  (Coders [coder].A_decoder_input_file, symbol, symbol+1,
	   PPMo_Models [ppmo_model].P_alphabet_size+1);

	if (Debug.coder)
	  {
	    fprintf (stderr, "Decode PPMo Pos = %d Symbol = %d order = %d **\n", bytes_input, symbol, order);
	    fprintf (stderr, "Decode PPMo : lbnd = %d hbnd = %d totl = %d\n", symbol, symbol+1,
		     PPMo_Models [ppmo_model].P_alphabet_size+1);
	  }
      }
    else
      {
	/* First determine which symbols are to be excluded if exclusions are to be performed */
	if (PPMo_Models [ppmo_model].P_performs_full_excls)
	  {
	    BITS_ZERO(PPMo_Models [ppmo_model].P_exclusions);

	    for (ord = smax_order; ord > order; ord--)
	     if (!PPMo_order_is_masked (ppmo_model, ppmo_context, ord))
	      {
		is_leaf = (ord >= smax_order);

		/* Follow child pointer for this context i.e. what it predicts */
		parent = PPMo_Contexts [ppmo_context].P_symbol_context [ord];
		if (parent == NULL)
		    ptr = PPMo_Models [ppmo_model].P_trie;
		else
		    ptr = PPMo_GET_PTR (parent, PPMo_NODE_CHILD);

		while (ptr != NULL)
		  { /* get all symbols in the list */
		    sym = PPMo_getnode_symbol (ptr, is_leaf);
		    BITS_SET(PPMo_Models [ppmo_model].P_exclusions, sym);
		    snext = PPMo_GET_PTR (ptr, PPMo_NODE_NEXT);
		    ptr = snext;
		  }
	      }
	  }

	/* Now process the order which was used to encode the symbol */
	is_leaf = (order >= PPMo_Models [ppmo_model].P_symbol_model_max_order);

	/* Follow child pointer for this context i.e. what it predicts */
	parent = PPMo_Contexts [ppmo_context].P_symbol_context [order];
	if (parent == NULL)
	    ptr = PPMo_Models [ppmo_model].P_trie;
	else
	    ptr = PPMo_GET_PTR (parent, PPMo_NODE_CHILD);

	/* First pass: find out the arithmetic coding total & number of symbols before decoding the target */
	totl = 0;
	scount = 0;
	assert (ptr != NULL);
	while (ptr != NULL)
	  { /* go through the symbol list */
	    scount++;
	    sym = PPMo_getnode_symbol (ptr, is_leaf);
	    symbol = sym;
	    if (!PPMo_Models [ppmo_model].P_performs_full_excls)
	        totl += PPMo_getnode_count (ptr, is_leaf);
	    else
	      {
		if (!BITS_ISSET(PPMo_Models [ppmo_model].P_exclusions, sym))
		    totl += PPMo_getnode_count (ptr, is_leaf);
	      }

	    snext = PPMo_GET_PTR (ptr, PPMo_NODE_NEXT);
	    ptr = snext;
	  }
	assert (totl > 0);
	if (scount == 1) /* Only one symbol was found - deterministic context, so there
			    was no need to encode it */
	  {
	    if (Debug.coder)
	        fprintf (stderr, "Decode PPMo : lbnd = %d hbnd = %d totl = %d\n", 0, totl, totl);
	  }
	else /* Not a deterministic context so need to explicitly decode which symbol was encoded */
	  {
	    target = Coders [coder].A_arithmetic_decode_target
	      (Coders [coder].A_decoder_input_file, totl);
	    if (Debug.coder)
	        fprintf (stderr, "Decode PPMo Pos = %d Symbol = %d Target = %d order = %d **\n", bytes_input, symbol, target, order);

	    /* Second pass: find the symbol associated with the target */
	    count = 0;
	    lbnd = 0;
	    hbnd = 0;
	    has_foundtarget = FALSE;
	    symbol = 0;
	    if (parent == NULL)
	        ptr = PPMo_Models [ppmo_model].P_trie;
	    else
	        ptr = PPMo_GET_PTR (parent, PPMo_NODE_CHILD);
	    while (ptr != NULL)
	      { /* find out where the symbol is */
		sym = PPMo_getnode_symbol (ptr, is_leaf);
		if (PPMo_Models [ppmo_model].P_performs_full_excls &&
		    BITS_ISSET(PPMo_Models [ppmo_model].P_exclusions, sym))
		    count = 0; /* This symbol has been excluded */
		else
		    count = PPMo_getnode_count (ptr, is_leaf);
		snext = PPMo_GET_PTR (ptr, PPMo_NODE_NEXT);
		if (target < lbnd + count)
		  { /* found symbol */
		    symbol = sym;
		    has_foundtarget = TRUE;
		    hbnd = lbnd + count;
		    break;
		  }
		lbnd += count;
		ptr = snext;
	      }
	    assert (has_foundtarget);

	    if (Debug.coder)
	        fprintf (stderr, "Decode PPMo : lbnd = %d hbnd = %d totl = %d\n", lbnd, hbnd, totl);

	    Coders [coder].A_arithmetic_decode
	      (Coders [coder].A_decoder_input_file, lbnd, hbnd, totl);
	  }
      }

    if (symbol == PPMo_eof_symbol (ppmo_model))
        return (TXT_sentinel_symbol ());

    PPMo_update_orders_context (ppmo_model, ppmo_context, order, prev_symbol, model_form);

    PPMo_update_symbols_context (ppmo_model, ppmo_context, symbol, increment, model_form);

    return (symbol);
}

void
PPMo_load_trie (unsigned int file, unsigned int ppmo_model, int smax_order,
		unsigned char *parent, unsigned int pos)
/* Recursive routine used by PPMo_load_model. Reloads the trie by reading it off disk. */
{
    unsigned int symbol, count;
    unsigned char *ptr, *sprev;
    boolean is_leaf;

    is_leaf = (pos >= smax_order);

    sprev = NULL;
    for (;;)
      { /* Repeat until NOMORE symbols */
	symbol = fread_int (file, 1);
	if (symbol == PPMo_NOMORE_SYMBOLS)
	  {
	    /*fprintf (stderr, "pos = %d NO MORE SYMBOLS\n", pos);*/
	    break;
	  }

	if (symbol == PPMo_LARGE_SYMBOL)
	    symbol = fread_int (file, INT_SIZE);

	count = fread_int (file, 1);
	if (!count)
	    count = fread_int (file, INT_SIZE);

	/*fprintf (stderr, "pos = %d symbol = %d count = %d\n", pos, symbol, count);*/
	if (is_leaf)
	    ptr = PPMo_create_leaf1 (symbol, count);
	else
	    ptr = PPMo_create_node1 (symbol, count);

	if (sprev != NULL)
	    *((unsigned int *) (sprev + PPMo_NODE_NEXT)) = (unsigned int) ptr;
	else if (parent == NULL)
	    PPMo_Models [ppmo_model].P_trie = ptr;
	else
	    *((unsigned int *) (parent + PPMo_NODE_CHILD)) = (unsigned int) ptr;

	if (pos < smax_order)
	    PPMo_load_trie (file, ppmo_model, smax_order, ptr, pos+1);
	sprev = ptr;
      }
}

unsigned int
PPMo_load_model (unsigned int file, unsigned int model_form)
/* Loads the PPM model which has been previously saved to the file into memory
   and allocates it a new model number which is returned. */
{
    unsigned int ppmo_model, alphabet_size, sym, count;
    int smax_order, omax_order, threshold, k;

    assert (TXT_valid_file (file));
    assert (model_form == TLM_Static);

    ppmo_model = create_PPMo_model (); /* Get next unused PPM model record */
    assert (ppmo_model != NIL);

    /* read in the size of the model's alphabet */
    alphabet_size = fread_int (file, INT_SIZE);
    PPMo_Models [ppmo_model].P_alphabet_size = alphabet_size;

    /* read in the order model's max order */
    omax_order = fread_int (file, INT_SIZE);
    PPMo_Models [ppmo_model].P_order_model_max_order = omax_order;

    /* read in the symbol model's max order */
    smax_order = fread_int (file, INT_SIZE);
    PPMo_Models [ppmo_model].P_symbol_model_max_order = smax_order;

    /* read in whether the model performs exclusions or not */
    PPMo_Models [ppmo_model].P_performs_full_excls = fread_int (file, INT_SIZE);

    if (smax_order < 0)
      { /* order -1 */
	return (ppmo_model);
      }

    /* read in whether each order has been masked or not */
    PPMo_Models [ppmo_model].P_mask_orders = (boolean *)
      Calloc (96, smax_order+2, sizeof (boolean));
    for (k = 0; k < smax_order+2; k++)
        PPMo_Models [ppmo_model].P_mask_orders [k] = fread_int (file, INT_SIZE);

    /* read in the order stream model */
    PPMo_Models [ppmo_model].P_orders_size = smax_order + 2;
    for (k = 0; k < omax_order; k++)
        PPMo_Models [ppmo_model].P_orders_size *= smax_order + 2;

    PPMo_Models [ppmo_model].P_orders_model = (unsigned int *)
      Calloc (95, PPMo_Models [ppmo_model].P_orders_size, sizeof (unsigned int));

    for (k = 0; k < PPMo_Models [ppmo_model].P_orders_size; k++)
      {
        PPMo_Models [ppmo_model].P_orders_model [k] = fread_int (file, INT_SIZE);
      }

    /* read in the order stream OS model if required */
    threshold = fread_int (file, INT_SIZE);
    PPMo_Models [ppmo_model].P_OS_model_threshold = threshold;
    
    if (threshold >= 0)
      {
        PPMo_Models [ppmo_model].P_OS_model = (unsigned int **)
	  Calloc (97, alphabet_size+1, sizeof (unsigned int *));
	for (;;)
	  { /* Keep reading until symbol is found > alphabet_size */
	    sym = fread_int (file, INT_SIZE);
	    if (sym > alphabet_size)
	        break;
	    PPMo_Models [ppmo_model].P_OS_model [sym] = Calloc (98, smax_order+2, sizeof (boolean));
	    for (k = -1; k < smax_order+1; k++)
	      {
		count = fread_int (file, INT_SIZE);
		PPMo_Models [ppmo_model].P_OS_model [sym][k] = count;
	      }
	  }
      }

    /* read in the model trie */
    /*fprintf( stderr, "Loading trie model...\n" );*/
    PPMo_load_trie (file, ppmo_model, smax_order, NULL, 0);

    return (ppmo_model);
}

void PPMo_write_trie (unsigned int file, int smax_order,
		      unsigned char *ptr, unsigned int pos)
/* Recursive routine used by PPMo_write_model. */
{
    unsigned int symbol, count;
    unsigned char *snext, *child;

    while (ptr != NULL)
      { /* proceed through the symbol list */
	symbol = PPMo_getnode_symbol (ptr, (pos == smax_order));
	count = PPMo_getnode_count (ptr, (pos == smax_order));

	/*fprintf (stderr, "pos = %d symbol = %d count = %d\n", pos, symbol, count);*/
	snext = PPMo_GET_PTR (ptr, PPMo_NODE_NEXT);
	if (pos >= smax_order)
	    child = NULL;
	else
	    child = PPMo_GET_PTR (ptr, PPMo_NODE_CHILD);

	if (symbol < PPMo_LARGE_SYMBOL)
	    fwrite_int (file, (unsigned int) symbol, 1);
	else
	  {
	    fwrite_int (file, (unsigned int) PPMo_LARGE_SYMBOL, 1);
	    fwrite_int (file, (unsigned int) symbol, INT_SIZE);
	  }
	if (count <= PPMo_MAX_BYTE_COUNT)
	    fwrite_int (file, (unsigned int) count, 1);
	else
	  {
	    fwrite_int (file, (unsigned int) 0, 1);
	    fwrite_int (file, (unsigned int) count, INT_SIZE);
	  }

	if (child != NULL)
	    PPMo_write_trie (file, smax_order, child, pos+1);
	else if (pos < smax_order)
	  {
	    fwrite_int (file, (unsigned int) PPMo_NOMORE_SYMBOLS, 1);
	    /*fprintf (stderr, "pos = %d NO MORE SYMBOLS\n", pos+1);*/
	  }
	ptr = snext;
      }

    fwrite_int (file, (unsigned int) PPMo_NOMORE_SYMBOLS, 1);
    /*fprintf (stderr, "pos = %d NO MORE SYMBOLS\n", pos);*/
}

void
PPMo_write_model (unsigned int file, unsigned int model,
		  unsigned int model_form)
/* Writes out the PPM model to the file (which can then be loaded
   by other applications later). The argument model_form must have the value
   TLM_Static or TLM_Dynamic and determines whether the model is static or
   dynamic when it is later reloaded using TLM_load_model. */
{
    unsigned int ppmo_model, alphabet_size, sym;
    unsigned int online_model_form;
    int omax_order, smax_order, threshold, k;

    assert (TXT_valid_file (file));
    assert (model_form == TLM_Static);

    ppmo_model = TLM_verify_model (model, TLM_PPMo_Model, PPMo_valid_model);

    online_model_form = Models [model].M_model_form;

    if ((model_form == TLM_Dynamic) && (online_model_form == TLM_Static))
      {
	fprintf (stderr, "Fatal error: This implementation of TLM_write_model () cannot write out\n");
	fprintf (stderr, "a dynamic model when a static model has been loaded\n");
	exit (1);
      }

    /* write out the size of the model's alphabet */
    alphabet_size = PPMo_Models [ppmo_model].P_alphabet_size;
    fwrite_int (file, alphabet_size, INT_SIZE);

    /* write out the order model's max order */
    omax_order = PPMo_Models [ppmo_model].P_order_model_max_order;
    /* fprintf (stderr, "\nMax order of order model = %d\n", omax_order); */
    fwrite_int (file, omax_order, INT_SIZE);

    /* write out the symbol model's max order */
    smax_order = PPMo_Models [ppmo_model].P_symbol_model_max_order;
    /* fprintf (stderr, "\nMax order of symbol model = %d\n", smax_order); */
    fwrite_int (file, smax_order, INT_SIZE);

    /* write out whether the model performs exclusions or not */
    fwrite_int (file, PPMo_Models [ppmo_model].P_performs_full_excls, INT_SIZE);

    if (smax_order < 0)
        return; /* order -1 model - nothing more to do */

    /* write out whether each order has been masked or not */
    for (k = 0; k < smax_order+2; k++)
        fwrite_int (file, PPMo_Models [ppmo_model].P_mask_orders [k], INT_SIZE);
        
    /* write out the order stream model */
    for (k = 0; k < PPMo_Models [ppmo_model].P_orders_size; k++)
      {
        fwrite_int (file, PPMo_Models [ppmo_model].P_orders_model [k], INT_SIZE);
      }

    /* write out the order stream OS model if required */
    threshold = PPMo_Models [ppmo_model].P_OS_model_threshold;
    fwrite_int (file, threshold, INT_SIZE);
    if (threshold >= 0)
      {
	for (sym = 0; sym <= alphabet_size; sym++)
	  if (PPMo_Models [ppmo_model].P_OS_model [sym] != NULL)
	    {
	      fwrite_int (file, sym, INT_SIZE);
	      for (k = -1; k < smax_order+1; k++)
		fwrite_int (file, PPMo_Models [ppmo_model].P_OS_model [sym][k], INT_SIZE);
	    }
	fwrite_int (file, alphabet_size+1, INT_SIZE); /* Indicate end of OS model */
      }

    /* Now write out the model trie: */
    PPMo_write_trie (file, smax_order, PPMo_Models [ppmo_model].P_trie, 0);
}

void
PPMo_dump_orders_model (unsigned int file, unsigned int ppmo_model)
/* Dumps out the order stream model. */
{
    int order, order1, smax_order, omax_order, threshold;
    unsigned int divisor, alphabet_size, sym, k, p;

    alphabet_size = PPMo_Models [ppmo_model].P_alphabet_size;
    smax_order = PPMo_Models [ppmo_model].P_symbol_model_max_order;
    omax_order = PPMo_Models [ppmo_model].P_order_model_max_order;

    /* Dump out the order stream model */
    fprintf (Files [file], "Dump of orders stream model:\n");
    for (p = 0; p < PPMo_Models [ppmo_model].P_orders_size; p++)
      if (PPMo_Models [ppmo_model].P_orders_model [p])
	{
	  order = p % (smax_order + 2);

	  fprintf (Files [file], "%4d: [", p);

	  divisor = 1;
	  for (k = 0; k < omax_order; k++)
	      divisor = divisor * (smax_order + 2);

	  for (k = 0; k < omax_order; k++)
	    {
	      order1 = (p / divisor) % (smax_order + 2);
	      fprintf (Files [file], " %2d", order1 - 1);
	      divisor = divisor / (smax_order + 2);
	    }
	  fprintf (Files [file], " ] order %2d count %d\n", order - 1,
		   PPMo_Models [ppmo_model].P_orders_model [p]);
	}

    /* Dump out the order/symbol (OS) model */
    threshold = PPMo_Models [ppmo_model].P_OS_model_threshold;
    fprintf (Files [file], "OS model threshold: %d\n", threshold);
    if (threshold >= 0)
      {
        fprintf (Files [file], "Dump of orders OS model:\n");
	for (sym = 0; sym <= alphabet_size; sym++)
	  if (PPMo_Models [ppmo_model].P_OS_model [sym] != NULL)
	    {
	      fprintf (Files [file], "sym = %3d : [ord,count]", sym);
	      for (order = -1; order < smax_order + 1; order++)
		fprintf (Files [file], " [%d,%d]", order,
			 PPMo_Models [ppmo_model].P_OS_model [sym][order + 1]);
	      fprintf (Files [file], "\n");
	    }
      }
}

unsigned int *PPMo_dump_symbols = NULL;

void PPMo_dump_trie (unsigned int file, unsigned int ppmo_model,
		     unsigned char *ptr, unsigned int pos,
		     void (*dump_symbol_function) (unsigned int, unsigned int))
/* Recursive routine used by PPMo_dump_model. */
{
    unsigned int p, symbol, count;
    unsigned char *snext, *child;
    int max_order;

    max_order = PPMo_Models [ppmo_model].P_symbol_model_max_order;
    while (ptr != NULL)
      { /* proceed through the symbol list */
	symbol = PPMo_getnode_symbol (ptr, (pos == max_order));
	count = PPMo_getnode_count (ptr, (pos == max_order));
	snext = PPMo_GET_PTR (ptr, PPMo_NODE_NEXT);
	if (pos >= max_order)
	    child = NULL;
	else
	    child = PPMo_GET_PTR (ptr, PPMo_NODE_CHILD);

	fprintf (Files [file], "%d %9p %9p %9p [", pos, ptr, snext, child);
	for (p = 0; p < pos; p++)
	  {
	    PPMo_dump_symbol (file, PPMo_dump_symbols [p], dump_symbol_function);
	    fprintf (Files [file], "," );
	  }
	PPMo_dump_symbol (file, symbol, dump_symbol_function);
	fprintf (Files [file], "] count = %d\n", count);

	if (child != NULL)
	  {
	    PPMo_dump_symbols [pos] = symbol;
	    PPMo_dump_trie (file, ppmo_model, child, pos+1, dump_symbol_function);
	  }
	ptr = snext;
      }
}

void
PPMo_dump_model (unsigned int file, unsigned int ppmo_model,
		 void (*dump_symbol_function) (unsigned int, unsigned int))
/* Dumps out the PPMo model (for debugging purposes). */
{
    int omax_order, smax_order;
    unsigned int k;

    omax_order = PPMo_Models [ppmo_model].P_order_model_max_order;
    fprintf (Files [file], "Max order of order model = %d\n", omax_order);
    smax_order = PPMo_Models [ppmo_model].P_symbol_model_max_order;
    fprintf (Files [file], "Max order of symbol model = %d\n", smax_order);

    /* Dump out whether each order has been masked or not */
    fprintf (Files [file], "Dump of mask orders:");
    for (k = 0; k < smax_order+2; k++)
        fprintf (Files [file], " %d=%d", k, PPMo_Models [ppmo_model].P_mask_orders [k]);
    fprintf (Files [file], "\n");

    PPMo_dump_orders_model (file, ppmo_model);

    fprintf (Files [file], "\nDump of trie: (depth, node ptr, next ptr, child ptr, path, count)\n");

    PPMo_dump_symbols = (unsigned int *)
        realloc (PPMo_dump_symbols, (PPMo_Models [ppmo_model].P_symbol_model_max_order+1) *
		 sizeof (unsigned int));

    PPMo_dump_trie (file, ppmo_model, PPMo_Models [ppmo_model].P_trie, 0, dump_symbol_function);
}

void
PPMo_copy_context1 (unsigned int ppmo_model, unsigned int ppmo_context,
		    unsigned int new_context)
/* Copies the contents of the specified context into the new context. */
{
    int smax_order, ord;

    smax_order = PPMo_Models [ppmo_model].P_symbol_model_max_order;

    PPMo_Contexts [new_context].P_order_context =
        PPMo_Contexts [ppmo_context].P_order_context;

    PPMo_Contexts [new_context].P_symbol_context = (unsigned char **)
      Calloc (93, smax_order+1, sizeof (unsigned char *));
    for (ord = 0; ord < smax_order+1; ord++)
        PPMo_Contexts [new_context].P_symbol_context [ord] =
	  PPMo_Contexts [ppmo_context].P_symbol_context [ord];

    PPMo_Contexts [new_context].P_prev_symbol =
        PPMo_Contexts [ppmo_context].P_prev_symbol;
}

unsigned int
PPMo_copy_context (unsigned int model, unsigned int context)
/* Creates a new PPMo context record, copies the contents of the specified
   context into it, and returns an integer reference to it. A run-time error
   occurs if the PPMo context being copied is for a dynamic model. */
{
    unsigned int ppmo_model;
    unsigned int new_context;

    ppmo_model = TLM_verify_model (model, TLM_PPMo_Model, PPMo_valid_model);

    assert (PPMo_valid_context (context));

    new_context = create_PPMo_context ();

    PPMo_copy_context1 (ppmo_model, context, new_context);
    return (new_context);
}

void
PPMo_update_context (unsigned int model, unsigned int context,
		     unsigned int symbol)
/* Updates the PPMo context record so that the current symbol becomes symbol.
   This routine just updates the context, and does not return
   any additional information such as the cost of encoding
   the symbol in bits. */
{
    unsigned int ppmo_model, ppmo_context;

    TLM_Codelength = 0;

    ppmo_model = TLM_verify_model (model, TLM_PPMo_Model, PPMo_valid_model);

    ppmo_context = context;
    assert (PPMo_valid_context (ppmo_context));

    /* update the trie and context */
    PPMo_update_counts (ppmo_model, ppmo_context, symbol, PPMo_INCR, Models [model].M_model_form);

    if (Debug.level > 4)
        PPMo_dump_model (Stderr_File, ppmo_model, NULL);
}

void
PPMo_encode_symbol (unsigned int model, unsigned int context, unsigned int coder,
		    unsigned int symbol)
/* Encodes the specified symbol using the arithmetic coder.
   Updates the context record so that the last symbol in the
   PPMo context becomes the encoded symbol. */
{
    unsigned int ppmo_model, ppmo_context;

    ppmo_model = TLM_verify_model (model, TLM_PPMo_Model, PPMo_valid_model);

    if (symbol == TXT_sentinel_symbol ()) /* This signifies the EOF */
        symbol = PPMo_eof_symbol (ppmo_model);

    ppmo_context = context;
    assert (PPMo_valid_context (ppmo_context));

    /* update the trie and context */
    PPMo_encode_counts (ppmo_model, ppmo_context, coder, symbol, PPMo_INCR, Models [model].M_model_form);

    if (Debug.level > 4)
        PPMo_dump_model (Stderr_File, ppmo_model, NULL);
}

unsigned int
PPMo_decode_symbol (unsigned int model, unsigned int context, unsigned int coder)
/* Returns the symbol decoded using the arithmetic coder. Updates the
   PPMo context record so that the last symbol in the context becomes the
   decoded symbol. */
{
    unsigned int ppmo_model, ppmo_context;
    unsigned int symbol;

    ppmo_model = TLM_verify_model (model, TLM_PPMo_Model, PPMo_valid_model);

    ppmo_context = context;
    assert (PPMo_valid_context (ppmo_context));
    assert (coder = TLM_valid_coder (coder));

    /* update the trie and context */
    symbol = PPMo_decode_counts (ppmo_model, context, coder, PPMo_INCR, Models [model].M_model_form);

    if (Debug.level > 4)
        PPMo_dump_model (Stderr_File, ppmo_model, NULL);

    return (symbol);
}

unsigned int
PPMo_getcontext_position (unsigned int model, unsigned int context)
/* Returns an integer which uniquely identifies the current position
   associated with the PPMo context. (One implementation is to return a memory
   location corresponding to the current position. This routine is useful if
   you need to check whether different contexts have encoded the same prior
   symbols as when checking whether the context pathways converge in the
   Viterbi or trellis-based algorithms.) */
{
    unsigned int ppmo_model, context_position;
    int smax_order;

    ppmo_model = TLM_verify_model (model, TLM_PPMo_Model, PPMo_valid_model);

    /* fprintf (stderr, "Context = %d\n", context); */

    smax_order = PPMo_Models [ppmo_model].P_symbol_model_max_order;
    for (;;)
      {
	context_position = (unsigned int) PPMo_Contexts [context].P_symbol_context [smax_order];
	if ((context_position != NIL) || (smax_order == 0))
	    break;
	smax_order--;
      }

    return (context_position);
}
