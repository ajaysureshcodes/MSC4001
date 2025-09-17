/* TLM routines based on HMM models. */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include "io.h"
#include "model.h"
#include "hmm_model.h"

#define HMM_MODELS_SIZE 4          /* Initial max. number of models */

/* Global variables used for storing the HMM models */
struct HMM_modelType *HMM_Models = NULL;/* List of HMM models */
unsigned int HMM_Models_max_size = 0;   /* Current max. size of models array */
unsigned int HMM_Models_used = NIL;     /* List of deleted model records */
unsigned int HMM_Models_unused = 1;     /* Next unused model record */

boolean
HMM_valid_model (unsigned int HMM_model)
/* Returns non-zero if the HMM model is valid, zero otherwize. */
{
    if (HMM_model == NIL)
        return (FALSE);
    else if (HMM_model >= HMM_Models_unused)
        return (FALSE);
    else if (HMM_Models [HMM_model].Next != NIL)
        /* This gets whenever the model gets deleted;
	   this way you can test to see if the model has been deleted or not */
        return (FALSE);
    else
        return (TRUE);
}

unsigned int
create_HMM_model (void)
/* Creates and returns a new pointer to a HMM model record. */
{
    unsigned int HMM_model, old_size;

    if (HMM_Models_used != NIL)
    { /* use the first record on the used list */
        HMM_model = HMM_Models_used;
	HMM_Models_used = HMM_Models [HMM_model].Next;
    }
    else
    {
	HMM_model = HMM_Models_unused;
        if (HMM_Models_unused >= HMM_Models_max_size)
	{ /* need to extend HMM_Models array */
	    old_size = HMM_Models_max_size * sizeof (struct modelType);
	    if (HMM_Models_max_size == 0)
	        HMM_Models_max_size = HMM_MODELS_SIZE;
	    else
	        HMM_Models_max_size = 10*(HMM_Models_max_size+50)/9;

	    HMM_Models = (struct HMM_modelType *)
	        Realloc (140, HMM_Models, HMM_Models_max_size *
			 sizeof (struct HMM_modelType), old_size);

	    if (HMM_Models == NULL)
	    {
	        fprintf (stderr, "Fatal error: out of HMM models space\n");
		exit (1);
	    }
	}
	HMM_Models_unused++;
    }

    HMM_Models [HMM_model].N = 0;
    HMM_Models [HMM_model].M = 0;
    HMM_Models [HMM_model].A = NULL;
    HMM_Models [HMM_model].B = NULL;
    HMM_Models [HMM_model].pi = NULL;
    HMM_Models [HMM_model].Next = NIL;

    return (HMM_model);
}

unsigned int
HMM_create_model (unsigned int N, unsigned int M)
/* Creates and returns a new pointer to a HMM model record. */
{
    unsigned int HMM_model;

    HMM_model = create_HMM_model ();

    fprintf (stderr, "HMM_create_model () is not implemented yet; Sorry.\n");
    exit (1);

    return (HMM_model);
}

void
HMM_get_model (unsigned int HMM_model, unsigned int *N, unsigned int *M)
/* Returns information about the HMM model. The arguments N and M
   are values used to create the model in HMM_create_model(). */
{
    assert (HMM_valid_model (HMM_model));

    *N = HMM_Models [HMM_model].N;
    *M = HMM_Models [HMM_model].M;

    fprintf (stderr, "HMM_get_model () is not implemented yet; Sorry.\n");
    exit (1);
}

void
HMM_release_model (unsigned int HMM_model)
/* Releases the memory allocated to the model and the model number (which may
   be reused in later HMM_create_model or HMM_load_model calls).
   A run-time error will be generated if an attempt is made to release
   a model that still has active contexts pointing at it. */
{
    assert (HMM_valid_model (HMM_model));

    /* add model record at the head of the HMM_Models_used list */
    HMM_Models [HMM_model].Next = HMM_Models_used;
    HMM_Models_used = HMM_model;
}

void
HMM_nullify_model (unsigned int model)
/* Replaces the model with the null model and releases the memory allocated
   to it. */
{
    unsigned int HMM_model;

    HMM_model = TLM_verify_model (model, TLM_HMM_Model, HMM_valid_model);

    fprintf (stderr, "HMM_nullify_model () is not implemented yet; Sorry.\n");
    exit (1);
}

unsigned int
HMM_load_model (unsigned int file, unsigned int model_form)
/* Loads the HMM model which has been previously saved to the file into memory
   and allocates it a new model number which is returned. */
{
    unsigned int HMM_model;

    HMM_model = create_HMM_model (); /* Get next unused HMM model record */
    assert (HMM_model != NIL);

    fprintf (stderr, "HMM_load_model () is not implemented yet; Sorry.\n");
    exit (1);

    fprintf (stderr, "HMM model loaded.\n");

    return (HMM_model);
}

void
HMM_write_model (unsigned int file, unsigned int model,
		 unsigned int model_form)
/* Writes out the HMM model to the file (which can then be loaded
   by other applications later). The argument model_form must have the value
   TLM_Static or TLM_Dynamic and determines whether the model is static or
   dynamic when it is later reloaded using TLM_load_model. */
{
    unsigned int HMM_model, online_model_form;

    HMM_model = TLM_verify_model (model, TLM_HMM_Model, HMM_valid_model);

    assert (TXT_valid_file (file));

    online_model_form = Models [model].M_model_form;

    fprintf (stderr, "HMM_write_model () is not implemented yet; Sorry.\n");
    exit (1);
}

void
HMM_dump_model (unsigned int file, unsigned int HMM_model)
/* Dumps out the HMM model (for debugging purposes). */
{
    assert (HMM_valid_model (HMM_model));

    fprintf (stderr, "HMM_write_model () is not implemented yet; Sorry.\n");
    exit (1);
}

boolean
HMM_valid_context (unsigned int context)
/* Returns non-zero if the HMM context is valid, zero otherwize. */
{
    /*
    if (context == NIL)
        return (FALSE);
    else if (context >= SSS_Contexts_unused)
        return (FALSE);
    else if (SSS_Contexts [context].S_next != 0)
        return (FALSE);
    else
        return (TRUE);
    */

    return (FALSE); /* Not yet implemented */
}
 
unsigned int
HMM_create_context (unsigned int model)
/* Creates and returns an unsigned integer which provides a reference to a HMM
   context record associated with the model's context. The current position is
   set to the null string. The current symbol is set to the first predicted
   symbol. */
{
    unsigned int HMM_model;
    /*unsigned int context;*/

    HMM_model = TLM_verify_model (model, TLM_HMM_Model, HMM_valid_model);

    Models [model].M_contexts++;

    /*
    context = HMM_create_context1 ();
    assert (context != NIL);
    */

    fprintf (stderr, "HMM_create_context () is not implemented yet; Sorry.\n");
    exit (1);

    return (NIL);
}

unsigned int
HMM_copy_context (unsigned int model, unsigned int context)
/* Creates a new HMM context record, copies the contents of the specified
   contex into it, and returns an integer reference to it. A run-time error
   occurs if the HMM context being copied is for a dynamic model. */
{
    unsigned int HMM_model;
    unsigned int model_form;
    /*unsigned int new_context;*/

    HMM_model = TLM_verify_model (model, TLM_HMM_Model, HMM_valid_model);

    /*
    assert (HMM_valid_context (context));
    new_context = HMM_create_context1 ();
    */

    model_form = Models [model].M_model_form;
    assert (model_form == TLM_Static);

    /*
    HMM_copy_context1 (model, context, new_context);
    */

    fprintf (stderr, "HMM_copy_context () is not implemented yet; Sorry.\n");
    exit (1);

    return (NIL);
}

void
HMM_overlay_context (unsigned int model, unsigned int old_context,
		     unsigned int context)
/* Overlays the HMM context by copying the old context into it. This will
   copy the context even for dynamic models. This is necessary when dynamic
   models with supplementary symbols are being created. */
{
    unsigned int HMM_model;

    HMM_model = TLM_verify_model (model, TLM_HMM_Model, HMM_valid_model);

    /*
    assert (HMM_valid_context (context));
    assert (HMM_valid_context (old_context));
    HMM_copy_context1 (model, old_context, context);
    */

    fprintf (stderr, "HMM_copy_context () is not implemented yet; Sorry.\n");
    exit (1);
}

void
HMM_find_symbol (unsigned int model, unsigned int context,
		  unsigned int symbol)
/* Finds the predicted symbol in the HMM context. */
{
    unsigned int HMM_model;
    codingType coding_type;

    HMM_model = TLM_verify_model (model, TLM_HMM_Model, HMM_valid_model);

    /*
    assert (HMM_valid_context (context));
    */

    switch (TLM_Context_Operation)
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

    fprintf (stderr, "HMM_find_symbol () is not implemented yet; Sorry.\n");
    exit (1);
}

void
HMM_update_context (unsigned int model, unsigned int context,
		    unsigned int symbol)
/* Updates the HMM context record so that the current symbol becomes symbol.
   Returns additional information as specified by the routine
   TLM_set_context_operation. */
{
    codingType coding_type;
    unsigned int HMM_model;

    HMM_model = TLM_verify_model (model, TLM_HMM_Model, HMM_valid_model);

    /*
    assert (HMM_valid_context (context));
    */

    switch (TLM_Context_Operation)
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

    fprintf (stderr, "HMM_update_context () is not implemented yet; Sorry.\n");
    exit (1);
}

void
HMM_release_context (unsigned int model, unsigned int context)
/* Releases the memory allocated to the HMM context and the context number
   (which may be reused in later HMM_create_context or HMM_copy_context and
   TLM_copy_dynamic_context calls). */
{
    unsigned int HMM_model;

    HMM_model = TLM_verify_model (model, TLM_HMM_Model, HMM_valid_model);

    /*
    assert (HMM_valid_context (context));
    */

    if (Models [model].M_contexts > 0)
        Models [model].M_contexts--;
 
    /* Append onto head of the used list; */

    fprintf (stderr, "HMM_release_context () is not implemented yet; Sorry.\n");
    exit (1);
}

void
HMM_reset_symbol (unsigned int model, unsigned int context)
/* Resets the HMM context record to point at the first predicted symbol of the
   current position. */
{
    unsigned int HMM_model;

    HMM_model = TLM_verify_model (model, TLM_HMM_Model, HMM_valid_model);

    /*
    assert (HMM_valid_context (context));
    */

    fprintf (stderr, "HMM_reset_symbol () is not implemented yet; Sorry.\n");
    exit (1);
}

boolean
HMM_next_symbol (unsigned int model, unsigned int context,
		 unsigned int *symbol)
/* Returns the next predicted symbol in the HMM context and the cost in bits of
   encoding it. The context record is not updated.

   If a sequence of calls to HMM_next_symbol are made, every symbol in the
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
    codingType coding_type;
    unsigned int HMM_model;

    HMM_model = TLM_verify_model (model, TLM_HMM_Model, HMM_valid_model);

    /*
    assert (HMM_valid_context (context));
    */

    fprintf (stderr, "HMM_next_symbol () is not implemented yet; Sorry.\n");
    exit (1);

    switch (TLM_Context_Operation)
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

    fprintf (stderr, "HMM_next_symbol () is not implemented yet; Sorry.\n");
    exit (1);

    return (TRUE);
}

void
HMM_encode_symbol (unsigned int model, unsigned int context,
		   unsigned int coder, unsigned int symbol)
/* Encodes the specified symbol using the arithmetic coder.
   Updates the context record so that the last symbol in the
   HMM context becomes the encoded symbol. */
{
    unsigned int HMM_model;

    HMM_model = TLM_verify_model (model, TLM_HMM_Model, HMM_valid_model);

    /*
    assert (HMM_valid_context (context));
    */

    fprintf (stderr, "HMM_encode_symbol () is not implemented yet; Sorry.\n");
    exit (1);
}

unsigned int
HMM_decode_symbol (unsigned int model, unsigned int context,
		   unsigned int coder)
/* Returns the symbol decoded using the arithmetic coder. Updates the
   HMM context record so that the last symbol in the context becomes the
   decoded symbol. */
{
    unsigned int HMM_model;

    HMM_model = TLM_verify_model (model, TLM_HMM_Model, HMM_valid_model);

    assert (coder = TLM_valid_coder (coder));

    /*
    assert (HMM_valid_context (context));
    */

    fprintf (stderr, "HMM_decode_symbol () is not implemented yet; Sorry.\n");
    exit (1);

    return (NIL);
}

unsigned int
HMM_getcontext_position (unsigned int model, unsigned int context)
/* Returns an integer which uniquely identifies the current position
   associated with the HMM context. (One implementation is to return a memory
   location corresponding to the current position. This routine is useful if
   you need to check whether different contexts have encoded the same prior
   symbols as when checking whether the context pathways converge in the
   Viterbi or trellis-based algorithms.) */
{
    unsigned int HMM_model;

    HMM_model = TLM_verify_model (model, TLM_HMM_Model, HMM_valid_model);

    fprintf (stderr, "HMM_getcontext_position () is not implemented yet; Sorry.\n");
    exit (1);

    return (NIL);
}

unsigned int
HMM_minlength_model (unsigned int model)
/* Returns the minimum number of bits needed to write the HMM model
   out to disk as a static model and recover it later. This is
   useful for computing minimum description lengths of messages.

   Note that the amount returned will always be slightly less than
   the resulting size of the static model produced by TLM_write_model
   as this excludes extra overhead data (including the model's title)
   that is necessary for the functioning of the API. */
{
    unsigned int HMM_model;

    HMM_model = TLM_verify_model (model, TLM_HMM_Model, HMM_valid_model);

    fprintf (stderr, "HMM_minlength_model () is not implemented yet; Sorry.\n");
    exit (1);

    return (0);
}

unsigned int
HMM_sizeof_model (unsigned int model)
/* Returns the current number of bits needed to store the
   model in memory. */
{
    unsigned int HMM_model;

    HMM_model = TLM_verify_model (model, TLM_HMM_Model, HMM_valid_model);

    fprintf (stderr, "HMM_sizeof_model () is not implemented yet; Sorry.\n");
    exit (1);

    return (0);
}

void
HMM_stats_model (unsigned int file, unsigned int model)
/* Writes out statistics about the HMM model in human readable form. */
{
    unsigned int HMM_model;

    HMM_model = TLM_verify_model (model, TLM_HMM_Model, HMM_valid_model);

    assert (TXT_valid_file (file));

    fprintf (stderr, "HMM_stats_model () is not implemented yet; Sorry.\n");
    exit (1);
}
