/* TLM routines based on PPMstar models. */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include "io.h"
#include "model.h"
#include "PPMstar_trie.h"
#include "PPMstar_context.h"
#include "PPMstar_model.h"

#define PPMstar_MODELS_SIZE 4          /* Initial max. number of models */

/* Global variables used for storing the PPMstar models */
struct PPMstar_modelType *PPMstar_Models = NULL;/* List of PPMstar models */
unsigned int PPMstar_Models_max_size = 0;   /* Current max. size of models array */
unsigned int PPMstar_Models_used = NIL;     /* List of deleted model records */
unsigned int PPMstar_Models_unused = 1;     /* Next unused model record */

boolean
PPMstar_valid_model (unsigned int PPMstar_model)
/* Returns non-zero if the PPMstar model is valid, zero otherwize. */
{
    if (PPMstar_model == NIL)
        return (FALSE);
    else if (PPMstar_model >= PPMstar_Models_unused)
        return (FALSE);
    else if (PPMstar_Models [PPMstar_model].P_max_order < -1)
        /* The max order gets set to -2 when the model gets deleted;
	   this way you can test to see if the model has been deleted or not */
        return (FALSE);
    else
        return (TRUE);
}

unsigned int
create_PPMstar_model (void)
/* Creates and returns a new pointer to a PPMstar model record. */
{
    unsigned int PPMstar_model, old_size;

    if (PPMstar_Models_used != NIL)
    { /* use the first record on the used list */
        PPMstar_model = PPMstar_Models_used;
	PPMstar_Models_used = PPMstar_Models [PPMstar_model].P_next;
    }
    else
    {
	PPMstar_model = PPMstar_Models_unused;
        if (PPMstar_Models_unused >= PPMstar_Models_max_size)
	{ /* need to extend PPMstar_Models array */
	    old_size = PPMstar_Models_max_size * sizeof (struct modelType);
	    if (PPMstar_Models_max_size == 0)
	        PPMstar_Models_max_size = PPMstar_MODELS_SIZE;
	    else
	        PPMstar_Models_max_size = 10*(PPMstar_Models_max_size+50)/9;

	    PPMstar_Models = (struct PPMstar_modelType *)
	        Realloc (84, PPMstar_Models, PPMstar_Models_max_size *
			 sizeof (struct PPMstar_modelType), old_size);

	    if (PPMstar_Models == NULL)
	    {
	        fprintf (stderr, "Fatal error: out of PPMstar models space\n");
		exit (1);
	    }
	}
	PPMstar_Models_unused++;
    }

    PPMstar_Models [PPMstar_model].P_max_order = -1;
    PPMstar_Models [PPMstar_model].P_performs_full_excls = TRUE;
    PPMstar_Models [PPMstar_model].P_trie = NULL;
    PPMstar_Models [PPMstar_model].P_ptable = NULL;

    return (PPMstar_model);
}

unsigned int
PPMstar_create_model (unsigned int alphabet_size, int max_order,
		  unsigned int performs_full_excls)
/* Creates and returns a new pointer to a PPMstar model record.

   The model_type argument specified the type of model to be created e.g.
   TLM_PPMstar_Model or TLM_PCFG_Model. It is followed by a variable number of
   parameters used to hold model information which differs between
   implementations of the different model types. For example, the
   TLM_PPMstar_Model implementation uses it to specify
   the maximum order of the PPMstar model, and whether the model should perform
   update exclusions.

   The alphabet_size argument specifies the number of symbols permitted in
   the alphabet (all symbols for this model must have values from 0 to one
   less than alphabet_size). An alphabet_size of 0 specifies that the
   alphabet is unbounded. (This is useful for word-based alphabets, for
   example). In this case, allowable symbols ranging from 0 up to a
   special "expand_alphabet" symbol which is equal to the current maximum
   symbol number (this is one more than the highest previously seen symbol
   number). If the current symbol becomes the expand_alphabet symbol, then
   the current maximum symbol number is incremented by 1, thus effectively
   expanding the size of the alphabet by 1. The current maximum symbol number
   may be obtained by calling the routine TLM_get_model. One further
   symbol is permitted in the current alphabet - the sentinel symbol.
*/

{
    unsigned int PPMstar_model;

    PPMstar_model = create_PPMstar_model ();

    PPMstar_Models [PPMstar_model].P_alphabet_size = alphabet_size;
    if (alphabet_size == 0)
        PPMstar_Models [PPMstar_model].P_max_symbol = 0;
    else
        PPMstar_Models [PPMstar_model].P_max_symbol = alphabet_size - 1;

    PPMstar_Models [PPMstar_model].P_max_order = max_order;
    PPMstar_Models [PPMstar_model].P_performs_full_excls = performs_full_excls;

    assert (max_order >= -1);
    if ((max_order < 0) || ((alphabet_size == 0) && (max_order == 0)))
        /* The (order -1) or (order 0, unbounded alphabet)
	   models never need to perform any exclusions */
        performs_full_excls = FALSE;
    assert (!performs_full_excls || (performs_full_excls == 1));

    if (max_order < 0)
        PPMstar_Models [PPMstar_model].P_trie = NULL;
    else
        PPMstar_Models [PPMstar_model].P_trie = PPMstar_create_trie (TLM_Dynamic);

    if (alphabet_size != 0)
	PPMstar_Models [PPMstar_model].P_ptable = NULL;
    else /* unbounded alphabet size */
	PPMstar_Models [PPMstar_model].P_ptable = ptable_create_table (0);

    return (PPMstar_model);
}

void
PPMstar_get_model (unsigned int PPMstar_model, unsigned int *alphabet_size,
	       unsigned int *max_symbol, int *max_order,
	       boolean *performs_full_excls)
/* Returns information about the PPMstar model. The arguments alphabet_size,
   max_order and performs_full_excls are values used to create the model in
   PPMstar_create_model(). The argument max_symbol is set to the current
   maximum symbol number used by the model.*/
{
    assert (PPMstar_valid_model (PPMstar_model));

    *alphabet_size = PPMstar_Models [PPMstar_model].P_alphabet_size;
    *max_symbol = PPMstar_Models [PPMstar_model].P_max_symbol;
    *max_order = PPMstar_Models [PPMstar_model].P_max_order;
    *performs_full_excls = PPMstar_Models [PPMstar_model].P_performs_full_excls;
}

void
PPMstar_release_model (unsigned int PPMstar_model)
/* Releases the memory allocated to the model and the model number (which may
   be reused in later PPMstar_create_model or PPMstar_load_model calls).
   A run-time error will be generated if an attempt is made to release
   a model that still has active PPMstar_Contexts pointing at it. */
{
    assert (PPMstar_valid_model (PPMstar_model));

    /* Release the model's trie as well */
    PPMstar_release_trie (PPMstar_Models [PPMstar_model].P_trie);

    /* add model record at the head of the PPMstar_Models_used list */
    PPMstar_Models [PPMstar_model].P_next = PPMstar_Models_used;
    PPMstar_Models_used = PPMstar_model;

    PPMstar_Models [PPMstar_model].P_max_order = -2; /* Used for testing if model no.
						is valid or not */
}

void
PPMstar_nullify_model (unsigned int model)
/* Replaces the model with the null model and releases the memory allocated
   to it. */
{
    unsigned int PPMstar_model;

    PPMstar_model = TLM_verify_model (model, TLM_PPMstar_Model, PPMstar_valid_model);

    PPMstar_release_trie (PPMstar_Models [PPMstar_model].P_trie);
    PPMstar_Models [PPMstar_model].P_trie = NULL;
}

unsigned int
PPMstar_load_model (unsigned int file, unsigned int model_form)
/* Loads the PPMstar model which has been previously saved to the file into memory
   and allocates it a new model number which is returned. */
{
    unsigned int PPMstar_model, trie_size, input_size, alphabet_size, p;
    struct PPMstar_trieType *trie;
    int max_order;

    PPMstar_model = create_PPMstar_model (); /* Get next unused PPMstar model record */
    assert (PPMstar_model != NIL);

    /* read in the size of the model's alphabet */
    alphabet_size = fread_int (file, INT_SIZE);
    PPMstar_Models [PPMstar_model].P_alphabet_size = alphabet_size;

    /* read in the model's maximum symbol number */
    PPMstar_Models [PPMstar_model].P_max_symbol = fread_int (file, INT_SIZE);

    /* read in the order of the model */
    max_order = fread_int (file, INT_SIZE);
    PPMstar_Models [PPMstar_model].P_max_order = max_order;

    /* read in whether the model performs exclusions or not */
    PPMstar_Models [PPMstar_model].P_performs_full_excls = fread_int (file, INT_SIZE);

    if (max_order < 0)
      { /* order -1 */
	PPMstar_Models [PPMstar_model].P_trie = NULL;
      }
    else
      { /* read in the model trie */
	/*fprintf( stderr, "Loading trie model...\n" );*/
	trie = PPMstar_create_trie (model_form);
	PPMstar_Models [PPMstar_model].P_trie = trie;
	trie_size = fread_int (file, INT_SIZE);
	trie->T_size = trie_size;
	trie->T_nodes = (int *) Malloc (2, (trie_size+1) * sizeof(int));
	p = 0;
	for (p = 0; p < trie_size; p++)
	  {
	    trie->T_nodes [p] = fread_int (file, INT_SIZE);
	  }

	if (model_form == TLM_Dynamic)
	  { /* read in the dynamic part, if one exists */
	    trie->T_unused = trie->T_size;

	    /*fprintf( stderr, "Loading input list...\n" );*/
	    input_size = fread_int (file, INT_SIZE);	
	    trie->T_input_size = input_size;
	    trie->T_input_len = fread_int (file, INT_SIZE);
	    trie->T_input = (unsigned int *) Malloc (92, (input_size+1) * sizeof(int));
	    for (p = 0; p < input_size; p++)
	      trie->T_input [p] = fread_int (file, INT_SIZE);
	  }

	if (alphabet_size == 0)
	  { /* read in the ptable for word_based alphabets */
	    /*fprintf( stderr, "Loading probability table...\n" );*/
	    PPMstar_Models [PPMstar_model].P_ptable = ptable_load_table (file);
	    /*ptable_dump_symbols (stderr, PPMstar_Models [PPMstar_model].P_ptable);*/
	  }
      }

    /* fprintf (stderr, "Trie model loaded.\n");*/

    return (PPMstar_model);
}

void
PPMstar_write_model (unsigned int file, unsigned int model,
		 unsigned int model_form)
/* Writes out the PPMstar model to the file (which can then be loaded
   by other applications later). The argument model_form must have the value
   TLM_Static or TLM_Dynamic and determines whether the model is static or
   dynamic when it is later reloaded using TLM_load_model. */
{
    unsigned int PPMstar_model, model_size, alphabet_size;
    unsigned int online_model_form, max_order;
    unsigned int input_size, dynamic_to_static, p;
    struct PPMstar_trieType *trie;

    PPMstar_model = TLM_verify_model (model, TLM_PPMstar_Model, PPMstar_valid_model);

    assert (TXT_valid_file (file));

    online_model_form = Models [model].M_model_form;

    trie = PPMstar_Models [PPMstar_model].P_trie;
        
    if ((model_form == TLM_Dynamic) && (online_model_form == TLM_Static))
      {
	fprintf (stderr, "Fatal error: This implementation of TLM_write_model () cannot write out\n");
	fprintf (stderr, "a dynamic model when a static model has been loaded\n");
	exit (1);
      }

    /* write out the size of the model's alphabet */
    alphabet_size = PPMstar_Models [PPMstar_model].P_alphabet_size;
    fwrite_int (file, alphabet_size, INT_SIZE);

    /* write out the model's maximum symbol number */
    fwrite_int (file, PPMstar_Models [PPMstar_model].P_max_symbol, INT_SIZE);

    /* write out the order of the model */
    max_order = PPMstar_Models [PPMstar_model].P_max_order;
    /* fprintf (stderr, "\nMax order of model = %d\n", max_order); */
    fwrite_int (file, max_order, INT_SIZE);

    /* write out whether the model performs exclusions or not */
    fwrite_int (file, PPMstar_Models [PPMstar_model].P_performs_full_excls, INT_SIZE);

    if (max_order < 0)
        return; /* order -1 model - nothing more to do */

    dynamic_to_static = ((model_form == TLM_Static) && (online_model_form == TLM_Dynamic));
    if (dynamic_to_static)
      { /* Writes out the dynamic trie to a static strie, compressing it
	   at the same time. (Which is one of the main reasons you want to do this
	   in the first place!). */
	trie = PPMstar_build_static_trie (trie, max_order);
	/* (replaces trie pointer with pointer to static version of it) */
      }
    if (online_model_form == TLM_Static)
        model_size = trie->T_size;
    else
        model_size = trie->T_unused;

    /* write out the model trie */
    /* fprintf (stderr, "Writing out the trie model...\n");*/
    fwrite_int (file, model_size, INT_SIZE);

    if ((model_form == TLM_Dynamic) && (online_model_form == TLM_Dynamic))
      { /* compress the dynamic trie */
	PPMstar_build_compressed_input (trie, max_order); /* compress input str. */
      }

    /* copy out the trie as it is */
    p = 0;
    for (p = 0; p < model_size; p++)
      {
	fwrite_int (file, (unsigned int) trie->T_nodes [p], INT_SIZE);
      }

    if ((model_form == TLM_Dynamic) && (online_model_form == TLM_Dynamic))
      { /* write out the dynamic part, if one exists */

	fprintf (stderr, "Writing out the input sequence...\n");
	input_size = trie->T_input_size;
	fwrite_int (file, input_size, INT_SIZE);
	fwrite_int (file, trie->T_input_len, INT_SIZE);
	p = 0;
	for (p = 0; p < input_size; p++)
	    fwrite_int (file, trie->T_input [p], INT_SIZE);
      }
    if (dynamic_to_static)
        PPMstar_release_trie (trie);

    if (alphabet_size == 0)
      { /* write out the ptable for word_based alphabets */
	/* fprintf (stderr, "Writing probability table...\n");*/
	ptable_write_table (file, PPMstar_Models [PPMstar_model].P_ptable);
	/*ptable_dump_symbols (stderr, PPMstar_Models [PPMstar_model].P_ptable);*/
      }

}

void
PPMstar_dump_model (unsigned int file, unsigned int PPMstar_model, boolean dumptrie,
		void (*dump_symbol_function) (unsigned int, unsigned int))
/* Dumps out the PPMstar model (for debugging purposes). */
{
    struct PPMstar_trieType *trie;

    assert (PPMstar_valid_model (PPMstar_model));
    trie = PPMstar_Models [PPMstar_model].P_trie;

    fprintf (Files [file], "Size of alphabet = %d\n\n",
	     PPMstar_Models [PPMstar_model].P_alphabet_size);
    fprintf (Files [file], "Maximum symbol number = %d\n\n",
	     PPMstar_Models [PPMstar_model].P_max_symbol);
    fprintf (Files [file], "Max order of model = %d\n",
	     PPMstar_Models [PPMstar_model].P_max_order);
    fprintf (Files [file], "Perform exclusions = %d\n",
	     PPMstar_Models [PPMstar_model].P_performs_full_excls);
    fprintf (Files [file], "Size of trie = %d\n\n", trie->T_size);

    if (dumptrie)
      {
	PPMstar_dump_trie (file, trie, PPMstar_Models [PPMstar_model].P_max_order,
		       dump_symbol_function);
	PPMstar_dump_input (file, trie, dump_symbol_function);
      }
    ptable_dump_symbols (file, PPMstar_Models [PPMstar_model].P_ptable);
}

unsigned int
PPMstar_create_context (unsigned int model)
/* Creates and returns an unsigned integer which provides a reference to a PPMstar
   context record associated with the model's context. The current position is
   set to the null string. The current symbol is set to the first predicted
   symbol. */
{
    unsigned int PPMstar_model;
    unsigned int context;
    int max_order;

    PPMstar_model = TLM_verify_model (model, TLM_PPMstar_Model, PPMstar_valid_model);

    max_order = PPMstar_get_max_order (model);

    Models [model].M_contexts++;

    context = PPMstar_create_context1 ();
    assert (context != NIL);

    PPMstar_Contexts [context].C_position = NULL;

    /* start the context off at the root: */
    if (max_order < 0)
        PPMstar_Contexts [context].C_node = NIL;
    else
        PPMstar_Contexts [context].C_node = TRIE_ROOT_NODE;

    PPMstar_create_suffixlist (model, context);
    PPMstar_init_suffixlist (model, context);
    PPMstar_start_suffix (model, context); /* Starts the suffix list at ROOT */

    return (context);
}

unsigned int
PPMstar_copy_context (unsigned int model, unsigned int context)
/* Creates a new PPMstar context record, copies the contents of the specified
   contex into it, and returns an integer reference to it. A run-time error
   occurs if the PPMstar context being copied is for a dynamic model. */
{
    unsigned int model_form, new_context;
    unsigned int PPMstar_model;

    PPMstar_model = TLM_verify_model (model, TLM_PPMstar_Model, PPMstar_valid_model);

    assert (PPMstar_valid_context (context));

    new_context = PPMstar_create_context1 ();
    PPMstar_create_suffixlist (model, new_context);

    model_form = Models [model].M_model_form;
    if (model_form == TLM_Dynamic)
      {
	if (Models [model].M_contexts > 0)
	  {
	    fprintf (stderr, "Fatal error: a dynamic model is allowed to have only one active context\n");
	    exit (1);
	  }
      }

    PPMstar_copy_context1 (model, context, new_context);
    return (new_context);
}

void
PPMstar_overlay_context (unsigned int model, unsigned int old_context,
		     unsigned int context)
/* Overlays the PPMstar context by copying the old context into it. This will
   copy the context even for dynamic models. This is necessary when dynamic
   models with supplementary symbols are being created. */
{
    unsigned int PPMstar_model;

    PPMstar_model = TLM_verify_model (model, TLM_PPMstar_Model, PPMstar_valid_model);

    assert (PPMstar_valid_context (context));
    assert (PPMstar_valid_context (old_context));

    PPMstar_copy_context1 (model, old_context, context);
}

void
PPMstar_find_symbol (unsigned int model, unsigned int context,
		     unsigned int symbol)
/* Finds the predicted symbol in the PPMstar context. */
{
    unsigned int PPMstar_model;
    struct PPMstar_positionType *position;
    codingType coding_type;

    PPMstar_model = TLM_verify_model (model, TLM_PPMstar_Model, PPMstar_valid_model);

    assert (PPMstar_valid_context (context));

    switch (TLM_Context_Type)
      {
      case TLM_Get_Codelength:
      default:
	coding_type = FIND_CODELENGTH_TYPE;
	break;
      case TLM_Get_Maxorder:
	coding_type = FIND_MAXORDER_TYPE;
	break;
      case TLM_Get_Coderanges:
	coding_type = FIND_CODERANGES_TYPE;
	break;
      }

    PPMstar_validate_symbol (context, symbol, &PPMstar_Context_Position);

    position =
        PPMstar_start_position (model, context, FIND_SYMBOL_TYPE, coding_type,
			    NO_CODER, &PPMstar_Context_Position);
    PPMstar_find_position (model, context, FIND_SYMBOL_TYPE, coding_type, NO_CODER,
		       position);

    switch (TLM_Context_Type)
      {
      case TLM_Get_Codelength:
      case TLM_Get_Maxorder:
      default:
	TLM_Codelength = position->P_codelength;
	break;
      case TLM_Get_Coderanges:
	TLM_Coderanges = TLM_copy_coderanges (position->P_coderanges);
	break;
      }
}

void
PPMstar_update_context (unsigned int model, unsigned int context,
		    unsigned int symbol)
/* Updates the PPMstar context record so that the current symbol becomes symbol.
   Returns additional information as specified by the routine
   TLM_set_context_operation. */
{
    struct PPMstar_positionType *position;
    codingType coding_type;
    unsigned int PPMstar_model;

    PPMstar_model = TLM_verify_model (model, TLM_PPMstar_Model, PPMstar_valid_model);

    assert (PPMstar_valid_context (context));

    switch (TLM_Context_Type)
      {
      case TLM_Get_Codelength:
	coding_type = UPDATE_CODELENGTH_TYPE;
	break;
      case TLM_Get_Maxorder:
	coding_type = UPDATE_MAXORDER_TYPE;
	break;
      case TLM_Get_Coderanges:
	coding_type = UPDATE_CODERANGES_TYPE;
	break;
      default:
	coding_type = UPDATE_TYPE;
	break;
      }

    PPMstar_validate_symbol (context, symbol, &PPMstar_Context_Position);

    position =
        PPMstar_start_position (model, context, FIND_SYMBOL_TYPE, coding_type,
			    NO_CODER, &PPMstar_Context_Position);
    PPMstar_find_position (model, context, FIND_SYMBOL_TYPE, coding_type, NO_CODER,
		       position);

    switch (TLM_Context_Type)
      {
      case TLM_Get_Codelength:
      case TLM_Get_Maxorder:
	TLM_Codelength = position->P_codelength;
	break;
      case TLM_Get_Coderanges:
	TLM_Coderanges = TLM_copy_coderanges (position->P_coderanges);
	break;
      default:
	break;
      }
}

void
PPMstar_release_context (unsigned int model, unsigned int context)
/* Releases the memory allocated to the PPMstar context and the context number
   (which may be reused in later PPMstar_create_context or PPMstar_copy_context and
   TLM_copy_dynamic_context calls). */
{
    unsigned int PPMstar_model;

    PPMstar_model = TLM_verify_model (model, TLM_PPMstar_Model, PPMstar_valid_model);

    assert (PPMstar_valid_context (context));

    if (Models [model].M_contexts > 0)
        Models [model].M_contexts--;
 
    PPMstar_release_suffixlist (model, context);

    PPMstar_release_position (PPMstar_Contexts [context].C_position);

    /* Append onto head of the used list; negative means this
       record has been placed on the used list */
    PPMstar_Contexts [context].C_next = - PPMstar_Contexts_used;
    PPMstar_Contexts_used = context;
}

void
PPMstar_reset_symbol (unsigned int model, unsigned int context)
/* Resets the PPMstar context record to point at the first predicted symbol of the
   current position. */
{
    unsigned int PPMstar_model;

    PPMstar_model = TLM_verify_model (model, TLM_PPMstar_Model, PPMstar_valid_model);

    assert (PPMstar_valid_context (context));

    if (PPMstar_Contexts [context].C_position != NULL)
        PPMstar_Contexts [context].C_position = PPMstar_start_position
	  (model, context, NEXT_SYMBOL_TYPE, FIND_CODELENGTH_TYPE, NO_CODER,
	   PPMstar_Contexts [context].C_position);

    PPMstar_reset_suffixlist (model, context); /* Start at the head of the suffix list */
}

boolean
PPMstar_next_symbol (unsigned int model, unsigned int context,
		 unsigned int *symbol)
/* Returns the next predicted symbol in the PPMstar context and the cost in bits of
   encoding it. The context record is not updated.

   If a sequence of calls to PPMstar_next_symbol are made, every symbol in the
   alphabet will be visited exactly once although the order in which they are
   visited is undefined being implementation and data dependent. The function
   returns FALSE when there are no more symbols to process. TLM_reset_symbol
   will reset the current position to point back at the first predicted symbol
   of the current context.

   The codelength value is the same as that returned by TLM_update_context
   which may use a faster search method to find the symbol's codelength
   more directly (rather than sequentially as TLM_next_symbol does). A call
   to TLM_update_context or other routines will have no affect on subsequent
   calls to TLM_next_symbol. */
{
    struct PPMstar_positionType *position;
    codingType coding_type;
    unsigned int PPMstar_model;
    unsigned int found;

    PPMstar_model = TLM_verify_model (model, TLM_PPMstar_Model, PPMstar_valid_model);

    assert (PPMstar_valid_context (context));

    fprintf (stderr, "There is a bug in TLM_next_symbol` (); Sorry.\n");
    exit (1);

    switch (TLM_Context_Type)
      {
      case TLM_Get_Codelength:
      default:
	coding_type = FIND_CODELENGTH_TYPE;
	break;
      case TLM_Get_Maxorder:
	coding_type = FIND_MAXORDER_TYPE;
	break;
      case TLM_Get_Coderanges:
	coding_type = FIND_CODERANGES_TYPE;
	break;
      }

    position = PPMstar_Contexts [context].C_position;
    if (position == NULL)
        position = PPMstar_start_position (model, context, NEXT_SYMBOL_TYPE,
            FIND_CODELENGTH_TYPE, NO_CODER, NULL /* null position */ );

    found = PPMstar_find_position (model, context, NEXT_SYMBOL_TYPE,
			  FIND_CODELENGTH_TYPE, NO_CODER, position);

    *symbol = position->P_symbol;
    PPMstar_Contexts [context].C_position = position;

    switch (TLM_Context_Type)
      {
      case TLM_Get_Codelength:
      case TLM_Get_Maxorder:
      default:
	TLM_Codelength = position->P_codelength;
	break;
      case TLM_Get_Coderanges:
	TLM_Coderanges = TLM_copy_coderanges (position->P_coderanges);
	break;
      }

    return (found);
}

void
PPMstar_encode_symbol (unsigned int model, unsigned int context,
		   unsigned int coder, unsigned int symbol)
/* Encodes the specified symbol using the arithmetic coder.
   Updates the context record so that the last symbol in the
   PPMstar context becomes the encoded symbol. */
{
    struct PPMstar_positionType *position;
    unsigned int PPMstar_model;

    PPMstar_model = TLM_verify_model (model, TLM_PPMstar_Model, PPMstar_valid_model);

    assert (PPMstar_valid_context (context));

    PPMstar_validate_symbol (context, symbol, &PPMstar_Context_Position);

    position =
        PPMstar_start_position  (model, context, FIND_SYMBOL_TYPE, ENCODE_TYPE,
			     coder, &PPMstar_Context_Position);
    PPMstar_find_position (model, context, FIND_SYMBOL_TYPE, ENCODE_TYPE, coder,
		       position);
}

unsigned int
PPMstar_decode_symbol (unsigned int model, unsigned int context,
		   unsigned int coder)
/* Returns the symbol decoded using the arithmetic coder. Updates the
   PPMstar context record so that the last symbol in the context becomes the
   decoded symbol. */
{
    struct PPMstar_positionType *position;
    unsigned int PPMstar_model;

    PPMstar_model = TLM_verify_model (model, TLM_PPMstar_Model, PPMstar_valid_model);

    assert (PPMstar_valid_context (context));

    position =
        PPMstar_start_position (model, context, FIND_TARGET_TYPE, DECODE_TYPE,
			    coder, &PPMstar_Context_Position);
    PPMstar_find_position (model, context, FIND_TARGET_TYPE, DECODE_TYPE, coder,
		       position);

    return (position->P_symbol);
}

unsigned int
PPMstar_getcontext_position (unsigned int model, unsigned int context)
/* Returns an integer which uniquely identifies the current position
   associated with the PPMstar context. (One implementation is to return a memory
   location corresponding to the current position. This routine is useful if
   you need to check whether different contexts have encoded the same prior
   symbols as when checking whether the context pathways converge in the
   Viterbi or trellis-based algorithms.) */
{
    unsigned int PPMstar_model;

    PPMstar_model = TLM_verify_model (model, TLM_PPMstar_Model, PPMstar_valid_model);

    return ((unsigned int) PPMstar_Contexts [context].C_node);
}

unsigned int
PPMstar_minlength_model (unsigned int model)
/* Returns the minimum number of bits needed to write the PPMstar model
   out to disk as a static model and recover it later. This is
   useful for computing minimum description lengths of messages.

   Note that the amount returned will always be slightly less than
   the resulting size of the static model produced by TLM_write_model
   as this excludes extra overhead data (including the model's title)
   that is necessary for the functioning of the API. */
{
    unsigned int PPMstar_model, size;
    struct PPMstar_trieType *trie;
    int max_order;

    PPMstar_model = TLM_verify_model (model, TLM_PPMstar_Model, PPMstar_valid_model);

    trie = PPMstar_Models [PPMstar_model].P_trie;

    if (trie == NULL) /* order -1 */
        size = 0;
    else if (trie->T_form == TLM_Static)
        size = trie->T_size * sizeof (int) * 8;
    else
      {
	max_order = PPMstar_Models [PPMstar_model].P_max_order;
	trie = PPMstar_build_static_trie (trie, max_order);
	/* (replaces trie pointer with pointer to static version of it) */
	if (trie == NULL)
	    size = 0;
	else
	    size = trie->T_unused * sizeof (int) * 8;
        PPMstar_release_trie (trie);
      }
    return (size);
}

unsigned int
PPMstar_sizeof_model (unsigned int model)
/* Returns the current number of bits needed to store the
   model in memory. */
{
    unsigned int PPMstar_model;
    struct PPMstar_trieType *trie;

    PPMstar_model = TLM_verify_model (model, TLM_PPMstar_Model, PPMstar_valid_model);

    trie = PPMstar_Models [PPMstar_model].P_trie;

    if (trie == NULL) /* order -1 */
        return (0);
    else if (trie->T_form == TLM_Static)
        return (trie->T_size * sizeof (int));
    else
        return ((trie->T_unused + trie->T_input_len) *sizeof (int));
}

void
PPMstar_stats_model (unsigned int file, unsigned int model)
/* Writes out statistics about the PPMstar model in human readable form. */
{
    unsigned int PPMstar_model;

    PPMstar_model = TLM_verify_model (model, TLM_PPMstar_Model, PPMstar_valid_model);

    assert (TXT_valid_file (file));

    fprintf (stderr, "Not yet implemented\n");
}
