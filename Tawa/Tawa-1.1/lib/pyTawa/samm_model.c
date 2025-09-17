/* TLM routines for training semi-adaptive Markov models. */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include "io.h"
#include "sets.h"
#include "text.h"
#include "model.h"
#include "ppm_model.h"
#include "samm_model.h"

#define SAMM_MODELS_SIZE 4         /* Initial max. number of models */
#define SAMM_CONTEXTS_SIZE 1024    /* Initial max. number of context records */
#define SAMM_MAX_BYTE_COUNT 255    /* Maximum count that fits in a byte without overflow */
/* Increments used to update the counts of the model: */
#define SAMM_INCR_C 1
#define SAMM_INCR_D 2

/* Offsets to data stored in the model. */
/* These fields are for counts and symbols that can't fit in a byte;
   at the moment, if either count or symbol can't fit, then 8 bytes
   are added to allow room for both possibilities - this could be
   reduced to only 4 extra bytes, but I haven't bothered to go to the
   extra effort although there could be extra wastage since in most
   cases, the symbol number usually fits in a byte only */
/* Offsets to data in node record: */
#define SAMM_NODE_SIZE 10 /* symbol(1) + sibling ptr(4) + child ptr(4) + count(1) */
#define SAMM_NODE_SYMBOL 0
#define SAMM_NODE_NEXT 1
#define SAMM_NODE_CHILD 5
#define SAMM_NODE_COUNT 9

/* Offsets to data in node1 record: */
#define SAMM_NODE1_SIZE 18 /* symbol(1) + sibling ptr(4) + child ptr(4) + count(1) +
			      count1(4) + symbol1(4) */
#define SAMM_NODE1_COUNT 10
#define SAMM_NODE1_SYMBOL 14

/* Offsets to data in leaf record: */
#define SAMM_LEAF_SIZE 6 /* symbol(1) + sibling ptr(4) + count(1) */
#define SAMM_LEAF_SYMBOL 0
#define SAMM_LEAF_NEXT 1
#define SAMM_LEAF_COUNT 5

/* Offsets to data in leaf1 record: */
#define SAMM_LEAF1_SIZE 14 /* symbol(1) + sibling ptr(4) + count(1) + count1(4) +
			      symbol1(4) */
#define SAMM_LEAF1_COUNT 6
#define SAMM_LEAF1_SYMBOL 10

#define SAMM_LARGE_SYMBOL 254               /* Indicates that symbol doesn't fit in a byte */
#define SAMM_NOMORE_SYMBOLS 255             /* Indicates no more siblings in trie */

/* Global variables used for storing the SAMM models */
struct SAMM_modelType *SAMM_Models = NULL; /* List of SAMM models */
unsigned int SAMM_Models_max_size = 0;     /* Current max. size of models array */
unsigned int SAMM_Models_used = NIL;       /* List of deleted model records */
unsigned int SAMM_Models_unused = 1;       /* Next unused model record */

/* Global variables used for storing the SAMM contexts */
unsigned int *SAMM_Contexts = NULL;        /* List of ptrs to SAMM contexts */
unsigned int *SAMM_Contexts_deleted = NULL;/* Indicates record is on the used list */
unsigned int SAMM_Contexts_max_size = 0;   /* Current max. size of models array */
unsigned int SAMM_Contexts_used = NIL;     /* List of deleted model records */
unsigned int SAMM_Contexts_unused = 1;     /* Next unused model record */

void
SAMM_dump_symbol (unsigned int file, unsigned int symbol,
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
SAMM_valid_context (unsigned int context)
/* Returns non-zero if the SAMM model is valid, zero otherwize. */
{
    if (context == NIL)
        return (FALSE);
    else if (context >= SAMM_Contexts_unused)
        return (FALSE);
    else if (SAMM_Contexts_deleted [context])
        /* This gets set to TRUE when the context gets deleted;
	   this way you can test to see if the model has been deleted or not */
        return (FALSE);
    else
        return (TRUE);
}

boolean
SAMM_valid_model (unsigned int model)
/* Returns non-zero if the SAMM model is valid, zero otherwize. */
{
    if (model == NIL)
        return (FALSE);
    else if (model >= SAMM_Models_unused)
        return (FALSE);
    else if (SAMM_Models [model].Q_max_order < -1)
        /* The max order gets set to -2 when the model gets deleted;
	   this way you can test to see if the model has been deleted or not */
        return (FALSE);
    else
        return (TRUE);
}

unsigned int
create_SAMM_model (void)
/* Creates and returns a new pointer to a SAMM model record. */
{
    unsigned int model, old_size;

    if (SAMM_Models_used != NIL)
    { /* use the first record on the used list */
        model = SAMM_Models_used;
	SAMM_Models_used = SAMM_Models [model].Q_next;
    }
    else
    {
	model = SAMM_Models_unused;
        if (SAMM_Models_unused >= SAMM_Models_max_size)
	{ /* need to extend SAMM_Models array */
	    old_size = SAMM_Models_max_size * sizeof (struct modelType);
	    if (SAMM_Models_max_size == 0)
	        SAMM_Models_max_size = SAMM_MODELS_SIZE;
	    else
	        SAMM_Models_max_size = 10*(SAMM_Models_max_size+50)/9;

	    SAMM_Models = (struct SAMM_modelType *)
	        Realloc (124, SAMM_Models, SAMM_Models_max_size *
			 sizeof (struct SAMM_modelType), old_size);

	    if (SAMM_Models == NULL)
	    {
	        fprintf (stderr, "Fatal error: out of SAMM models space\n");
		exit (1);
	    }
	}
	SAMM_Models_unused++;
    }

    SAMM_Models [model].Q_max_order = -1;
    SAMM_Models [model].Q_escape_method = TLM_PPM_Method_B;

    return (model);
}

unsigned int
SAMM_create_model (unsigned int alphabet_size, int max_order,
		   unsigned int escape_method)
/* Creates and returns a new pointer to a SAMM model record.

   The model_type argument specified the type of model to be created e.g.
   TLM_SAMM_Model or TLM_PCFG_Model. It is followed by a variable number of
   parameters used to hold model information which differs between
   implementations of the different model types. For example, the
   TLM_SAMM_Model implementation uses it to specify
   the maximum order of the SAMM model and the escape method.

   The alphabet_size argument specifies the number of symbols permitted in
   the alphabet (all symbols for this model must have values from 0 to one
   less than alphabet_size).
*/
{
    unsigned int samm_model;

    samm_model = create_SAMM_model ();

    SAMM_Models [samm_model].Q_alphabet_size = alphabet_size;
    SAMM_Models [samm_model].Q_model_size = 0;

    assert (max_order >= 0); /* Order -1 not yet implemented */
    SAMM_Models [samm_model].Q_Root = NULL;
    SAMM_Models [samm_model].Q_max_order = max_order;

    SAMM_Models [samm_model].Q_escape_method = escape_method;
    assert ((TLM_PPM_Method_A <= escape_method) && (escape_method <= TLM_PPM_Method_D));
    if ((escape_method == TLM_PPM_Method_A) || (escape_method == TLM_PPM_Method_B))
      {
	fprintf (stderr, "SAMM escape method %d is not yet implemented\n", escape_method);
	exit (1);
      }

    return (samm_model);
}

unsigned char *
SAMM_create_node (unsigned int samm_model, unsigned int symbol)
{
    unsigned char *ptr, bytes;
    unsigned int size;
    boolean is_small_record;

    is_small_record = (symbol < SAMM_LARGE_SYMBOL);
    if (is_small_record)
        size = SAMM_NODE_SIZE;
    else
        size = SAMM_NODE1_SIZE;

    /* Create the node */
    bytes = size * sizeof (unsigned char);
    ptr = (unsigned char *) Calloc (121, bytes, 1);
    assert (ptr != NULL);

    SAMM_Models [samm_model].Q_model_size += bytes;

    /*fprintf (stderr, "Created node ptr %p for symbol %d\n", ptr, symbol);*/

    if (is_small_record)
        *(ptr + SAMM_NODE_SYMBOL) = symbol;
    else
      { /* Either the symbol or count can't fit in a byte */
        *(ptr + SAMM_NODE_SYMBOL) = SAMM_LARGE_SYMBOL;
	(*((unsigned int *) (ptr + SAMM_NODE1_SYMBOL))) = symbol;
      }
    return (ptr);
}

unsigned char *
SAMM_create_leaf (unsigned int samm_model, unsigned int symbol)
{
    unsigned char *ptr, bytes;
    unsigned int size;
    boolean is_small_record;

    is_small_record = (symbol < SAMM_LARGE_SYMBOL);
    if (is_small_record)
        size = SAMM_LEAF_SIZE;
    else
        size = SAMM_LEAF1_SIZE;

    /* Create the leaf */
    bytes = size * sizeof (unsigned char);
    ptr = (unsigned char *) Calloc (122, bytes, 1);
    assert (ptr != NULL);

    SAMM_Models [samm_model].Q_model_size += bytes;

    /*fprintf (stderr, "Created leaf ptr %p for symbol %d\n", ptr, symbol);*/

    if (is_small_record)
        *(ptr + SAMM_LEAF_SYMBOL) = symbol;
    else
      { /* Either the symbol or count can't fit in a byte */
        *(ptr + SAMM_LEAF_SYMBOL) = SAMM_LARGE_SYMBOL;
	(*((unsigned int *) (ptr + SAMM_LEAF1_SYMBOL))) = symbol;
      }
    return (ptr);
}

/* SAMM_create_node1 and SAMM_create_leaf1 are separate routines to speed things up
   a wee bit */

unsigned char *
SAMM_create_node1 (unsigned int samm_model, unsigned int symbol, unsigned int count)
{
    unsigned int size, bytes;
    unsigned char *ptr;
    boolean is_small_record;

    is_small_record = (symbol < SAMM_LARGE_SYMBOL) && (count <= SAMM_MAX_BYTE_COUNT);
    if (is_small_record)
        size = SAMM_NODE_SIZE;
    else
        size = SAMM_NODE1_SIZE;

    /* Create the node */
    bytes = size * sizeof (unsigned char);
    ptr = (unsigned char *) Calloc (121, bytes, 1);
    assert (ptr != NULL);

    SAMM_Models [samm_model].Q_model_size += bytes;

    /*fprintf (stderr, "Created node ptr %p for symbol %d\n", ptr, symbol);*/

    if (is_small_record)
      {
        *(ptr + SAMM_NODE_SYMBOL) = symbol;
	*(ptr + SAMM_NODE_COUNT) = count;
      }
    else
      { /* Either the symbol or count can't fit in a byte */
        *(ptr + SAMM_NODE_SYMBOL) = SAMM_LARGE_SYMBOL;
	(*((unsigned int *) (ptr + SAMM_NODE1_SYMBOL))) = symbol;
	*(ptr + SAMM_NODE_COUNT) = 0;
	(*((unsigned int *) (ptr + SAMM_NODE1_COUNT))) = count;
      }
    return (ptr);
}

unsigned char *
SAMM_create_leaf1 (unsigned int samm_model, unsigned int symbol, unsigned int count)
{
    unsigned int size, bytes;
    unsigned char *ptr;
    boolean is_small_record;

    is_small_record = (symbol < SAMM_LARGE_SYMBOL) && (count <= SAMM_MAX_BYTE_COUNT);
    if (is_small_record)
        size = SAMM_LEAF_SIZE;
    else
        size = SAMM_LEAF1_SIZE;

    /* Create the leaf */
    bytes = size * sizeof (unsigned char);
    ptr = (unsigned char *) Calloc (122, bytes, 1);
    assert (ptr != NULL);

    SAMM_Models [samm_model].Q_model_size += bytes;

    /*fprintf (stderr, "Created leaf ptr %p for symbol %d\n", ptr, symbol);*/

    if (is_small_record)
      {
        *(ptr + SAMM_LEAF_SYMBOL) = symbol;
        *(ptr + SAMM_LEAF_COUNT) = count;
      }
    else
      { /* Either the symbol or count can't fit in a byte */
        *(ptr + SAMM_LEAF_SYMBOL) = SAMM_LARGE_SYMBOL;
	(*((unsigned int *) (ptr + SAMM_LEAF1_SYMBOL))) = symbol;
        *(ptr + SAMM_LEAF_COUNT) = 0;
	(*((unsigned int *) (ptr + SAMM_LEAF1_COUNT))) = count;
      }
    return (ptr);
}

unsigned int
SAMM_get_max_order (unsigned int model)
/* Returns the max. order of the SAMM model. */
{
    unsigned int samm_model;

    samm_model = TLM_verify_model (model, TLM_SAMM_Model, SAMM_valid_model);

    return (SAMM_Models [samm_model].Q_max_order);
}

void
SAMM_get_model (unsigned int model, unsigned int type,
		unsigned int *value)
/* Returns information about the SAMM model. The arguments type is
   the information to be returned (i.e. SAMM_Get_Alphabet_Size,
   SAMM_Get_Max_Order, SAMM_Get_Escape_Method). */
{
    unsigned int samm_model;

    samm_model = TLM_verify_model (model, TLM_SAMM_Model, SAMM_valid_model);

    switch (type)
      {
      case SAMM_Get_Alphabet_Size:
	*value = SAMM_Models [samm_model].Q_alphabet_size;
	break;
      case SAMM_Get_Max_Order:
	*value = (unsigned int) SAMM_Models [samm_model].Q_max_order;
	break;
      case SAMM_Get_Escape_Method:
	*value = SAMM_Models [samm_model].Q_escape_method;
	break;
      default:
	fprintf (stderr, "Invalid type (%d) specified for SAMM_get_model ()\n", type);
	exit (1);
	break;
      }
}

void
SAMM_release_model (unsigned int model)
/* Releases the memory allocated to the model and the model number (which may
   be reused in later SAMM_create_model or SAMM_load_model calls).
   A run-time error will be generated if an attempt is made to release
   a model that still has active SAMM_Contexts pointing at it. */
{
    assert (SAMM_valid_model (model));

    /* add model record at the head of the SAMM_Models_used list */
    SAMM_Models [model].Q_next = SAMM_Models_used;
    SAMM_Models_used = model;

    SAMM_Models [model].Q_max_order = -2; /* Used for testing if model no.
					      is valid or not */
}

unsigned int
SAMM_getnode_symbol (unsigned char *trie_ptr, boolean is_leaf)
/* Returns the count for the node at trie_ptr */
{
    unsigned int symbol;

    if (is_leaf)
        symbol = *(trie_ptr + SAMM_LEAF_SYMBOL);
    else
        symbol = *(trie_ptr + SAMM_NODE_SYMBOL);
    if (symbol == SAMM_LARGE_SYMBOL)
      {
	if (is_leaf)
	    symbol = *((unsigned int *) (trie_ptr + SAMM_LEAF1_SYMBOL));
	else
	    symbol = *((unsigned int *) (trie_ptr + SAMM_NODE1_SYMBOL));
      }
    return (symbol);
}

unsigned int
SAMM_getnode_count (unsigned char *trie_ptr, boolean is_leaf)
/* Returns the count for the node at trie_ptr */
{
    unsigned int count;

    if (is_leaf)
        count = *(trie_ptr + SAMM_LEAF_COUNT);
    else
        count = *(trie_ptr + SAMM_NODE_COUNT);
    if (count == 0)
      {
	if (is_leaf)
	    count = *((unsigned int *) (trie_ptr + SAMM_LEAF1_COUNT));
	else
	    count = *((unsigned int *) (trie_ptr + SAMM_NODE1_COUNT));
      }
    return (count);
}

/* Context routines */
void
SAMM_update_context1 (unsigned int samm_model, unsigned int context, unsigned int symbol)
/* Updates the context for the model using the new symbol. */
{
    register unsigned int *Context, pos;
    register int max_order;

    max_order = SAMM_Models [samm_model].Q_max_order;

    Context = (unsigned int *) SAMM_Contexts [context];
    for (pos = 0; pos < max_order; pos++)
	Context [pos] = Context [pos+1];
    Context [pos] = symbol;
}

unsigned int
SAMM_create_context1 (void)
/* Return a new pointer to a context record. */
{
    unsigned int context, old_size;

    if (SAMM_Contexts_used != NIL)
    {	/* use the first record on the used list */
	context = SAMM_Contexts_used;
	SAMM_Contexts_used = SAMM_Contexts [context];
    }
    else
    {
	context = SAMM_Contexts_unused;
	if (SAMM_Contexts_unused >= SAMM_Contexts_max_size)
	{ /* need to extend SAMM_Contexts array */
	    old_size = SAMM_Contexts_max_size * sizeof (unsigned int);
	    if (SAMM_Contexts_max_size == 0)
		SAMM_Contexts_max_size = SAMM_CONTEXTS_SIZE;
	    else
		SAMM_Contexts_max_size = 10*(SAMM_Contexts_max_size+50)/9; 

	    SAMM_Contexts = (unsigned int *)
	        Realloc (123, SAMM_Contexts, SAMM_Contexts_max_size *
			 sizeof (unsigned int), old_size);
	    SAMM_Contexts_deleted = (unsigned int *)
	        Realloc (123, SAMM_Contexts_deleted, SAMM_Contexts_max_size *
			 sizeof (boolean), old_size);

	    if (SAMM_Contexts == NULL)
	    {
		fprintf (stderr, "Fatal error: out of SAMM_Contexts space\n");
		exit (1);
	    }
	}
	SAMM_Contexts_unused++;
    }
    SAMM_Contexts [context] = NIL;
    SAMM_Contexts_deleted [context] = FALSE;

    return (context);
}

unsigned int
SAMM_create_context (unsigned int model)
/* Creates and returns an unsigned integer which provides a reference to a SAMM
   context record associated with the model's context. The current position is
   set to the null string. The current symbol is set to the first predicted
   symbol. A run-time error occurs if the SAMM context being copied is for a
   dynamic model. 
*/
{
    unsigned int model_form, bytes;
    unsigned int context, samm_model, p;
    int max_order;

    samm_model = TLM_verify_model (model, TLM_SAMM_Model, SAMM_valid_model);

    max_order = SAMM_Models [samm_model].Q_max_order;

    model_form = Models [model].M_model_form;
    if ((model_form == TLM_Dynamic) && (Models [model].M_contexts > 0))
      {
        fprintf (stderr, "Fatal error: a dynamic model is allowed to have only one active context\n");
        exit (1);
      }

    Models [model].M_contexts++;

    context = SAMM_create_context1 ();

    assert (context != NIL);

    /* Initialize the new context */
    bytes = (max_order+1) * sizeof (unsigned int);
    SAMM_Contexts [context] = (unsigned int) Malloc (123, bytes);

    for (p = 0; p <= max_order; p++)
        SAMM_update_context1 (samm_model, context, TXT_sentinel_symbol ());

    return (context);
}

void
SAMM_release_context (unsigned int model, unsigned int context)
/* Releases the memory allocated to the SAMM context and the context number
   (which may be reused in later SAMM_create_context or SAMM_copy_context and
   TLM_copy_dynamic_context calls). */
{
    unsigned int samm_model;
    int max_order;

    samm_model = TLM_verify_model (model, TLM_SAMM_Model, SAMM_valid_model);

    assert (SAMM_valid_context (context));

    max_order = SAMM_Models [samm_model].Q_max_order;

    Free (89, (unsigned int *) SAMM_Contexts [context],
	  (max_order+1) * sizeof (unsigned int));
    SAMM_Contexts_deleted [context] = TRUE;

    /* Append onto head of the used list; negative means this
       record has been placed on the used list */
    SAMM_Contexts [context] = SAMM_Contexts_used;
    SAMM_Contexts_used = context;
}

void
SAMM_update_trie (unsigned int samm_model, unsigned int context, unsigned int increment)
/* Updates the model's count for the new symbol. */
{
    register unsigned char *ptr, *old_ptr, *parent, *child, *snext, *sprev;
    register unsigned int sym, symbol, count, offset, pos, escape_method;
    register unsigned int *Context;
    register int max_order;
    register boolean is_leaf;

    max_order = SAMM_Models [samm_model].Q_max_order;
    escape_method = SAMM_Models [samm_model].Q_escape_method;
    Context = (unsigned int *) SAMM_Contexts [context];

    parent = NULL;
    ptr = SAMM_Models [samm_model].Q_Root;

    /* Note: This updates the counts only for the contexts from the
       current position e.g. ^HERE - contexts are H, HE, HER and HERE;
       this is opposite to the standard PPM which updates backwards
       i.e. HERE^ - HERE, ERE, RE, E */
    for (pos = 0; pos <= max_order; pos++)
      { /* create or access non leaf nodes */
	is_leaf = (pos >= max_order);
	symbol = Context [pos];

	sprev = NULL;
	child = NULL;
	count = 0;
	while (ptr != NULL)
	  { /* find out where the symbol is */
	    sym = SAMM_getnode_symbol (ptr, is_leaf);
	    count = SAMM_getnode_count (ptr, is_leaf);
	    snext = (unsigned char *) *((unsigned int *) (ptr + SAMM_NODE_NEXT));
	    if (is_leaf)
	        child = NULL;
	    else
	        child = (unsigned char *) *((unsigned int *) (ptr + SAMM_NODE_CHILD));
	    if (sym == symbol)
	        break;
	    sprev = ptr;
	    ptr = snext;
	  }

	old_ptr = ptr;
	if (ptr == NULL)
	  { /* haven't found the symbol - create it */
	    count = 0;
	    child = NULL;
	    if (is_leaf)
	        ptr = SAMM_create_leaf (samm_model, symbol);
	    else
	        ptr = SAMM_create_node (samm_model, symbol);
	  }

	/* Increment count */
	if (is_leaf)
	    offset = SAMM_LEAF_COUNT;
	else
	    offset = SAMM_NODE_COUNT;
	    
	if (count + increment <= SAMM_MAX_BYTE_COUNT)
	    (*(ptr + offset)) += increment; /* Increment node symbol count */
	else
	  { /* Overflow of byte count field */
	    register unsigned int offset1, size, size1, mem_ident;

	    if (is_leaf)
	      {
		offset1 = SAMM_LEAF1_COUNT;
		size1 = SAMM_LEAF1_SIZE;
		size =  SAMM_LEAF_SIZE;
		mem_ident = 122;
	      }
	    else
	      {
		offset1 = SAMM_NODE1_COUNT;
		size1 = SAMM_NODE1_SIZE;
		size =  SAMM_NODE_SIZE;
		mem_ident = 121;
	      }

	    if (*(ptr + offset))
	      {
		/*
		  fprintf (stderr, "Overflow has occurred");
		  SAMM_dump_model (Stderr_File, samm_model, NULL);
		*/

		*(ptr + offset) = 0; /* This indicates count has overflowed byte */
		SAMM_Models [samm_model].Q_model_size += size1 - size;
		ptr = Realloc (mem_ident, ptr, size1, size);
		(*((unsigned int *) (ptr + offset1))) = count;

		old_ptr = NULL; /* Ensure trie gets updated with re-allocated ptr */
	      }

	    (*((unsigned int *) (ptr + offset1))) += increment;

	  }

	if (old_ptr == NULL)
	  {
	    if (sprev != NULL)
	        *((unsigned int *) (sprev + SAMM_NODE_NEXT)) = (unsigned int) ptr;
	    else if (parent == NULL)
	        SAMM_Models [samm_model].Q_Root = ptr;
	    else
	        *((unsigned int *) (parent + SAMM_NODE_CHILD)) = (unsigned int) ptr;
	  }

	parent = ptr;
	ptr = child; /* move down the trie */
      }
}


void
SAMM_getcount_trie (unsigned int samm_model, unsigned int context,
		    unsigned int *lbnd, unsigned int *hbnd, unsigned int *totl)
/* Gets the trie model's count for the context. */
{
    register unsigned char *ptr, *child, *snext;
    register unsigned int sym, symbol, total, count, pos;
    register unsigned int *Context;
    register int max_order;
    register boolean is_leaf;

    max_order = SAMM_Models [samm_model].Q_max_order;
    Context = (unsigned int *) SAMM_Contexts [context];

    ptr = SAMM_Models [samm_model].Q_Root;

    *lbnd = 0;
    *hbnd = 0;
    *totl = 0;
    total = 0;
    for (pos = 0; pos <= max_order; pos++)
      { /* access nodes in the trie */
	is_leaf = (pos >= max_order);
	symbol = Context [pos];

	child = NULL;
	while (ptr != NULL)
	  { /* find out where the symbol is */
	    sym = SAMM_getnode_symbol (ptr, is_leaf);
	    snext = (unsigned char *) *((unsigned int *) (ptr + SAMM_NODE_NEXT));
	    if (is_leaf)
	        child = NULL;
	    else
	        child = (unsigned char *) *((unsigned int *) (ptr + SAMM_NODE_CHILD));
	    if (is_leaf)
	      {
	        count = SAMM_getnode_count (ptr, is_leaf);
		if (sym == symbol)
		  {
		    *lbnd = total;
		    *hbnd = total + count;
		  }
		total += count;
	      }
	    if (!is_leaf && (sym == symbol))
	        break;
	    ptr = snext;
	  }

	ptr = child; /* move down the trie */
      }

    *totl = total;
}

unsigned char *
SAMM_gettotal_trie (unsigned int samm_model, unsigned int context, unsigned int *totl)
/* Gets the trie model's total count for the context and returns the trie model's leaf pointer. */
{
    register unsigned char *ptr, *leaf_ptr, *child, *snext;
    register unsigned int sym, symbol, total, count, pos;
    register unsigned int *Context;
    register int max_order;
    register boolean is_leaf;

    max_order = SAMM_Models [samm_model].Q_max_order;
    Context = (unsigned int *) SAMM_Contexts [context];

    ptr = SAMM_Models [samm_model].Q_Root;
    leaf_ptr = ptr;

    total = 0;
    for (pos = 0; pos <= max_order; pos++)
      { /* access nodes in the trie */
	is_leaf = (pos >= max_order);
	if (is_leaf)
	    symbol = 0;
	else
	    symbol = Context [pos+1];

	child = NULL;
	leaf_ptr = ptr;
	while (ptr != NULL)
	  { /* find out where the symbol is */
	    sym = SAMM_getnode_symbol (ptr, is_leaf);
	    snext = (unsigned char *) *((unsigned int *) (ptr + SAMM_NODE_NEXT));
	    if (is_leaf)
	        child = NULL;
	    else
	        child = (unsigned char *) *((unsigned int *) (ptr + SAMM_NODE_CHILD));
	    if (is_leaf)
	      {
	        count = SAMM_getnode_count (ptr, is_leaf);
		total += count;
	      }
	    if (!is_leaf && (sym == symbol))
	        break;
	    ptr = snext;
	  }

	ptr = child; /* move down the trie */
      }

    *totl = total;
    return (leaf_ptr);
}

unsigned int
SAMM_getsymbol_trie (unsigned int samm_model, unsigned int context, unsigned int target,
		     unsigned char *ptr, unsigned int *lbnd, unsigned int *hbnd)
/* Gets the trie model's total count for the context and returns the trie model's leaf pointer. */
{
    register unsigned char *snext;
    register unsigned int sym, total, count;
    register unsigned int *Context;
    register int max_order;

    max_order = SAMM_Models [samm_model].Q_max_order;
    Context = (unsigned int *) SAMM_Contexts [context];

    total = 0;
    sym = 0;
    assert (ptr != NULL);
    while (ptr != NULL)
      { /* find out which symbol range the target symbol falls into */
	sym = SAMM_getnode_symbol (ptr, TRUE /*is_leaf*/);
	snext = (unsigned char *) *((unsigned int *) (ptr + SAMM_NODE_NEXT));
	count = SAMM_getnode_count (ptr, TRUE /*is_leaf*/);
	if (total + count > target)
	  { /* found the symbol */
	    *lbnd = total;
	    *hbnd = total + count;
	    break;
	  }
	total += count;
	ptr = snext;
      }

    return (sym);
}

void
SAMM_load_trie (unsigned int file, unsigned int samm_model, int max_order,
		unsigned char *parent, unsigned int pos)
/* Recursive routine used by SAMM_load_model. Reloads the trie by reading it off disk. */
{
    unsigned int symbol, count;
    unsigned char *ptr, *sprev;
    boolean is_leaf;

    is_leaf = (pos >= max_order);

    sprev = NULL;
    for (;;)
      { /* Repeat until NOMORE symbols */
	symbol = fread_int (file, 1);
	if (symbol == SAMM_NOMORE_SYMBOLS)
	    break;

	if (symbol == SAMM_LARGE_SYMBOL)
	    symbol = fread_int (file, INT_SIZE);

	count = fread_int (file, 1);
	if (!count)
	    count = fread_int (file, INT_SIZE);

	if (is_leaf)
	    ptr = SAMM_create_leaf1 (samm_model, symbol, count);
	else
	    ptr = SAMM_create_node1 (samm_model, symbol, count);

	if (sprev != NULL)
	    *((unsigned int *) (sprev + SAMM_NODE_NEXT)) = (unsigned int) ptr;
	else if (parent == NULL)
	    SAMM_Models [samm_model].Q_Root = ptr;
	else
	    *((unsigned int *) (parent + SAMM_NODE_CHILD)) = (unsigned int) ptr;

	/*fprintf (stderr, "%d %9p %9p %9p symbol %d count %d\n", pos, ptr, sprev, parent,
	  symbol, count);*/

	if (pos < max_order)
	    SAMM_load_trie (file, samm_model, max_order, ptr, pos+1);
	sprev = ptr;
      }
}

unsigned int
SAMM_load_model (unsigned int file, unsigned int model_form)
/* Loads the SAMM model which has been previously saved to the file into memory
   and allocates it a new model number which is returned. */
{
    unsigned int samm_model, samm_context, alphabet_size, escape_method;
    unsigned int max_symbol;
    boolean performs_full_excls;
    int max_order;

    /* read in the size of the model's alphabet */
    alphabet_size = fread_int (file, INT_SIZE);

    /* read in the model's maximum symbol number */
    max_symbol = fread_int (file, INT_SIZE);

    /* read in the order of the model */
    max_order = fread_int (file, INT_SIZE);

    /* read in the escape method of the model */
    escape_method = fread_int (file, INT_SIZE);

    samm_model = SAMM_create_model (alphabet_size, max_order, escape_method);
    assert (samm_model != NIL);

    samm_context = SAMM_create_context (samm_model); /* Get next unused SAMM context record */
    assert (samm_context != NIL);

    /* read in whether the model performs exclusions or not */
    performs_full_excls = fread_int (file, INT_SIZE);

    if (max_order < 0)
      { /* order -1 */
	SAMM_Models [samm_model].Q_Root = NULL;
      }
    else
      { /* read in the model trie */
	/*fprintf( stderr, "Loading trie model...\n" );*/

	SAMM_load_trie (file, samm_model, max_order, NULL, 0);
      }

    /* fprintf (stderr, "Model loaded.\n");*/

    SAMM_release_context (samm_model, samm_context);

    return (samm_model);
}

void
SAMM_build_PPM_trie (unsigned int samm_model, unsigned char *trie_ptr,
		     struct PPM_trieType *strie, int strie_pptr, unsigned int pcount,
		     unsigned int pos)
/* Recursive routine used by SAMM_write_model. Builds a static version of the trie which is
   then be written out in PPM_model format. */
{
    unsigned int symbol, count, scount, model_size, alloc, p;
    unsigned char *ptr, *snext, *child;
    int max_order;

    model_size = 0;

    /* first pass through symbol list - find out the number of symbols scount */
    scount = 0;
    ptr = trie_ptr;
    while (ptr != NULL)
      { /* proceed through the symbol list */
	scount++;
	snext = (unsigned char *) *((unsigned int *) (ptr + SAMM_NODE_NEXT));
	ptr = snext;
      }

    max_order = SAMM_Models [samm_model].Q_max_order;
    if (pos > max_order)
        alloc = TRIE_NODE_SWIDTH; /* allocate for single count only */
    else
        alloc = TRIE_NODE_SWIDTH + scount * TRIE_SLIST_SWIDTH; /* allocate for all symbols as well */

    p = PPM_allocate_trie_node (strie, alloc);
    strie ->T_nodes [strie_pptr] = p;

    /* store the count */
    if (pcount == 0)
        pcount = scount; /* we should only get here for the root (because of the different ways
			    fast PPM and PPM store their counts - there is no root count for fast PPM) */
    strie->T_nodes [p+TRIE_TCOUNT_OFFSET] = pcount;

    /* second pass through symbol list - write out the symbol list to the strie */
    if (pos <= max_order)
      {
	p += TRIE_NODE_SWIDTH; /* skip to start of symbol list */
	ptr = trie_ptr;
	while (ptr != NULL)
	  { /* proceed through the symbol list */
	    symbol = SAMM_getnode_symbol (ptr, (pos == max_order));
	    count = SAMM_getnode_count (ptr, (pos == max_order));
	    snext = (unsigned char *) *((unsigned int *) (ptr + SAMM_NODE_NEXT));

	    if (snext != NIL)
	        strie->T_nodes [p] = symbol; /* store the symbol */
	    else if (symbol == 0)
	        strie->T_nodes [p] = SPECIAL_SYMBOL; /* special case - indicate last
							symbol in list is zero */
	    else
	        strie->T_nodes [p] = -symbol; /* negative means there are no more symbols in the list */

	    if (pos >= max_order)
	        child = NULL;
	    else
	        child = (unsigned char *) *((unsigned int *) (ptr + SAMM_NODE_CHILD));
	    SAMM_build_PPM_trie (samm_model, child, strie, p+TRIE_CHILD_OFFSET, count, pos+1);
	    ptr = snext;
	    p += TRIE_SLIST_SWIDTH;
	  }
      }
}

void
SAMM_write_PPM_trie (unsigned int file, unsigned int samm_model)
/* Writes out the SAMM model to the file as a static PPM model (note: not fast PPM) 
   (which can then be loaded by other applications later). */
{
    unsigned int model_size, p;
    struct PPM_trieType *strie;

    strie = PPM_create_trie (TLM_Static);
    SAMM_build_PPM_trie (samm_model, SAMM_Models [samm_model].Q_Root, strie, NIL, 0, 0);

    /* write out the model trie */
    /* fprintf (stderr, "Writing out the trie model...\n");*/
    model_size = strie->T_size;
    fwrite_int (file, model_size, INT_SIZE);

    /* copy out the trie as it is */
    for (p = 0; p < model_size; p++)
	fwrite_int (file, (unsigned int) strie->T_nodes [p], INT_SIZE);

    PPM_release_trie (strie);
}

void SAMM_write_trie (unsigned int file, int max_order,
		      unsigned char *ptr, unsigned int pos)
/* Recursive routine used by SAMM_write_model. */
{
    unsigned int symbol, count;
    unsigned char *snext, *child;

    while (ptr != NULL)
      { /* proceed through the symbol list */
	symbol = SAMM_getnode_symbol (ptr, (pos == max_order));
	count = SAMM_getnode_count (ptr, (pos == max_order));
	snext = (unsigned char *) *((unsigned int *) (ptr + SAMM_NODE_NEXT));
	if (pos >= max_order)
	    child = NULL;
	else
	    child = (unsigned char *) *((unsigned int *) (ptr + SAMM_NODE_CHILD));

	if (symbol < SAMM_LARGE_SYMBOL)
	    fwrite_int (file, (unsigned int) symbol, 1);
	else
	  {
	    fwrite_int (file, (unsigned int) SAMM_LARGE_SYMBOL, 1);
	    fwrite_int (file, (unsigned int) symbol, INT_SIZE);
	  }
	if (count <= SAMM_MAX_BYTE_COUNT)
	    fwrite_int (file, (unsigned int) count, 1);
	else
	  {
	    fwrite_int (file, (unsigned int) 0, 1);
	    fwrite_int (file, (unsigned int) count, INT_SIZE);
	  }

	if (child != NULL)
	  {
	    SAMM_write_trie (file, max_order, child, pos+1);
	  }
	ptr = snext;
      }

    fwrite_int (file, (unsigned int) SAMM_NOMORE_SYMBOLS, 1);
}

void
SAMM_write_model (unsigned int file, unsigned int model,
		  unsigned int model_form, unsigned int model_type)
/* Writes out the PPM model to the file as a static PPM model (note: not fast PPM) 
   (which can then be loaded by other applications later). */
{
    unsigned int samm_model, alphabet_size, escape_method;
    int max_order;

    samm_model = TLM_verify_model (model, TLM_SAMM_Model, SAMM_valid_model);

    assert (TXT_valid_file (file));
    assert (model_form == TLM_Static);

    assert ((model_type == TLM_PPM_Model) || (model_type == TLM_SAMM_Model));

    /* write out the size of the model's alphabet */
    alphabet_size = SAMM_Models [samm_model].Q_alphabet_size;
    fwrite_int (file, alphabet_size, INT_SIZE);

    /* write out the model's maximum symbol number */
    fwrite_int (file, alphabet_size-1, INT_SIZE);

    /* write out the order of the model */
    max_order = SAMM_Models [samm_model].Q_max_order;
    /* fprintf (stderr, "\nMax order of model = %d\n", max_order); */
    fwrite_int (file, max_order, INT_SIZE);

    /* write out the escape method of the model */
    escape_method = SAMM_Models [samm_model].Q_escape_method;
    /* fprintf (stderr, "\nEscape method of the model = %d\n", escape_method); */
    fwrite_int (file, escape_method, INT_SIZE);

    /* write out whether the model performs exclusions or not */
    fwrite_int (file, TRUE, INT_SIZE);

    if (max_order < 0)
        return; /* order -1 model - nothing more to do */

    if (model_type == TLM_PPM_Model)
        SAMM_write_PPM_trie (file, samm_model);
    else
        SAMM_write_trie (file, max_order, SAMM_Models [samm_model].Q_Root, 0);
}

unsigned int *SAMM_dump_symbols = NULL;

void SAMM_dump_trie (unsigned int file, unsigned int samm_model,
		     unsigned char *ptr, unsigned int pos,
		     void (*dump_symbol_function) (unsigned int, unsigned int))
/* Recursive routine used by SAMM_dump_model. */
{
    unsigned int p, symbol, count;
    unsigned char *snext, *child;
    int max_order;

    max_order = SAMM_Models [samm_model].Q_max_order;
    while (ptr != NULL)
      { /* proceed through the symbol list */
	symbol = SAMM_getnode_symbol (ptr, (pos == max_order));
	count = SAMM_getnode_count (ptr, (pos == max_order));
	snext = (unsigned char *) *((unsigned int *) (ptr + SAMM_NODE_NEXT));
	if (pos >= max_order)
	    child = NULL;
	else
	    child = (unsigned char *) *((unsigned int *) (ptr + SAMM_NODE_CHILD));

	fprintf (Files [file], "%d %9p %9p %9p [", pos, ptr, snext, child);
	for (p = 0; p < pos; p++)
	  {
	    SAMM_dump_symbol (file, SAMM_dump_symbols [p], dump_symbol_function);
	    fprintf (Files [file], "," );
	  }
	SAMM_dump_symbol (file, symbol, dump_symbol_function);
	fprintf (Files [file], "] count = %d\n", count);

	if (child != NULL)
	  {
	    SAMM_dump_symbols [pos] = symbol;
	    SAMM_dump_trie (file, samm_model, child, pos+1, dump_symbol_function);
	  }
	ptr = snext;
      }
}

void SAMM_check_trie (unsigned int file, unsigned int samm_model,
		      unsigned char *ptr, unsigned int pos, unsigned int parent_count,
		      void (*dump_symbol_function) (unsigned int, unsigned int))
/* Recursive routine to check trie is consistent. */
{
    unsigned int p, symbol, count, current_count;
    unsigned char *snext, *child, *copy_ptr;
    int max_order;

    max_order = SAMM_Models [samm_model].Q_max_order;
    current_count = 0;
    copy_ptr = ptr;
    while (ptr != NULL)
      { /* proceed through the symbol list */
	symbol = SAMM_getnode_symbol (ptr, (pos == max_order));
	count = SAMM_getnode_count (ptr, (pos == max_order));
	current_count += count;
	snext = (unsigned char *) *((unsigned int *) (ptr + SAMM_NODE_NEXT));
	if (pos >= max_order)
	    child = NULL;
	else
	    child = (unsigned char *) *((unsigned int *) (ptr + SAMM_NODE_CHILD));

	if (child != NULL)
	  {
	    SAMM_dump_symbols [pos] = symbol;
	    SAMM_check_trie (file, samm_model, child, pos+1, count, dump_symbol_function);
	  }
	ptr = snext;
      }

    if ((current_count != parent_count) && !((parent_count == 0) && (pos == 0)))
      { /* Error in count found */

	fprintf (Files [file], "*** Bad counts, total %d actual %d\n",
		 parent_count, current_count);
	ptr = copy_ptr;
	while (ptr != NULL)
	  { /* proceed through the symbol list */
	    symbol = SAMM_getnode_symbol (ptr, (pos == max_order));
	    count = SAMM_getnode_count (ptr, (pos == max_order));
	    snext = (unsigned char *) *((unsigned int *) (ptr + SAMM_NODE_NEXT));

	    fprintf (Files [file], "%d [", pos);
	    for (p = 0; p < pos; p++)
	      {
		SAMM_dump_symbol (file, SAMM_dump_symbols [p], dump_symbol_function);
		fprintf (Files [file], "," );
	      }
	    SAMM_dump_symbol (file, symbol, dump_symbol_function);
	    fprintf (Files [file], "] count = %d\n", count);

	    ptr = snext;
	  }
      }
}

void
SAMM_dump_model (unsigned int file, unsigned int samm_model,
		 void (*dump_symbol_function) (unsigned int, unsigned int))
/* Dumps out the SAMM model (for debugging purposes). */
{
    assert (SAMM_valid_model (samm_model));

    fprintf (Files [file], "Size of alphabet = %d\n\n",
	     SAMM_Models [samm_model].Q_alphabet_size);
    fprintf (Files [file], "Max order of model = %d\n",
	     SAMM_Models [samm_model].Q_max_order);

    fprintf (Files [file], "Escape method = method ");
    switch (SAMM_Models [samm_model].Q_escape_method)
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

    fprintf (Files [file], "Dump of trie: (depth, node ptr, next ptr, child ptr, path, count)\n");

    SAMM_dump_symbols = (unsigned int *)
        realloc (SAMM_dump_symbols, (SAMM_Models [samm_model].Q_max_order+1) *
		 sizeof (unsigned int));

    SAMM_dump_trie (file, samm_model, SAMM_Models [samm_model].Q_Root, 0, dump_symbol_function);
}

void
SAMM_check_model (unsigned int file, unsigned int samm_model,
		  void (*dump_symbol_function) (unsigned int, unsigned int))
/* Checks the SAMM model is consistent (for debugging purposes). */
{
    SAMM_dump_symbols = (unsigned int *)
        realloc (SAMM_dump_symbols, (SAMM_Models [samm_model].Q_max_order+1) *
		 sizeof (unsigned int));

    SAMM_check_trie (file, samm_model, SAMM_Models [samm_model].Q_Root, 0, 0, dump_symbol_function);
}


void
SAMM_update_context (unsigned int model, unsigned int context,
		     unsigned int symbol)
/* Updates the SAMM context record so that the current symbol becomes symbol. */
{
    unsigned int samm_model, increment;
    unsigned int *Context;

    samm_model = TLM_verify_model (model, TLM_SAMM_Model, SAMM_valid_model);

    if (SAMM_Models [samm_model].Q_escape_method == TLM_PPM_Method_D)
        increment = SAMM_INCR_D;
    else
        increment = SAMM_INCR_C;

    assert (SAMM_valid_context (context));
    Context = (unsigned int *) SAMM_Contexts [context];

    /* first update the context */
    SAMM_update_context1 (samm_model, context, symbol);

    /* next update the trie */
    SAMM_update_trie (samm_model, context, increment);
}

unsigned int
SAMM_sizeof_model (unsigned int model)
/* Returns the memory usage for SAMM models. */
{
    unsigned int samm_model;

    samm_model = TLM_verify_model (model, TLM_SAMM_Model, SAMM_valid_model);

    return (SAMM_Models [samm_model].Q_model_size);
}

void
SAMM_encode_symbol (unsigned int model, unsigned int context,
		    unsigned int coder, unsigned int symbol)
/* Encodes the specified symbol using the arithmetic coder.
   Updates the context record so that the last symbol in the
   SAMM context becomes the encoded symbol. */
{
    unsigned samm_model, lbnd, hbnd, totl;

    samm_model = TLM_verify_model (model, TLM_SAMM_Model, SAMM_valid_model);

    assert (SAMM_valid_context (context));

    assert (coder = TLM_valid_coder (coder));

    SAMM_getcount_trie (samm_model, context, &lbnd, &hbnd, &totl);
    if (Debug.range)
        fprintf (stderr, "lbnd = %d hbnd = %d totl = %d\n", lbnd, hbnd, totl);
    if (hbnd == 0)
        fprintf (stderr, "*** Error - invalid hbnd %d\n", hbnd);
    else
        Coders [coder].A_arithmetic_encode
	  (Coders [coder].A_encoder_output_file, lbnd, hbnd, totl);

    /*
    switch (TLM_Context_Operation)
      {
      case TLM_Get_Nothing:
	break;
      case TLM_Get_Coderanges:
	if (TLM_Coderanges == NIL)
	    TLM_Coderanges = TLM_create_coderanges ();
	TLM_overwrite_coderange (TLM_Coderanges, lbnd, hbnd, totl);
	break;
      default:
        TLM_Codelength = Codelength (lbnd, hbnd, totl);
	break;
      }
    */

    SAMM_update_context1 (samm_model, context, symbol);
}

unsigned int
SAMM_decode_symbol (unsigned int model, unsigned int context,
		    unsigned int coder)
/* Returns the symbol decoded using the arithmetic coder. Updates the
   SAMM context record so that the last symbol in the context becomes the
   decoded symbol. */
{
    unsigned char *ptr;
    unsigned samm_model, symbol, lbnd, hbnd, totl, target;

    samm_model = TLM_verify_model (model, TLM_SAMM_Model, SAMM_valid_model);

    assert (SAMM_valid_context (context));

    ptr = SAMM_gettotal_trie (samm_model, context, &totl);
    target = Coders [coder].A_arithmetic_decode_target
      (Coders [coder].A_decoder_input_file, totl);

    symbol = SAMM_getsymbol_trie (samm_model, context, target, ptr, &lbnd, &hbnd);
    if (Debug.range)
      {
        fprintf (stderr, "Symbol = %d\n", symbol);
        fprintf (stderr, "totl = %d target = %d\n", totl, target);
        fprintf (stderr, "lbnd = %d hbnd = %d totl = %d\n", lbnd, hbnd, totl);
      }
    if (hbnd == 0)
        fprintf (stderr, "*** Error - invalid hbnd %d\n", hbnd);
    else
        Coders [coder].A_arithmetic_decode
	  (Coders [coder].A_decoder_input_file, lbnd, hbnd, totl);

    SAMM_update_context1 (samm_model, context, symbol);

    return (symbol);
}
