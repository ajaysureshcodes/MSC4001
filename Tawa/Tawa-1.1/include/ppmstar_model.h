/* TLM routines based on PPMstar models. */

#include "PPMstar_trie.h"
#include "ptable.h"
#include "io.h"

#ifndef PPMstar_MODEL_H
#define PPMstar_MODEL_H

struct PPMstar_modelType
{ /* PPMstar model record */
  unsigned int P_alphabet_size;       /* The number of symbols in the alphabet;
					 0 means an unbounded sized alphabet (which
					 incrementally increases as each new
					 symbol is added */
  unsigned int P_max_symbol;          /* Current max. symbol number for unbounded
					 sized alphabets */
  int P_max_order;                    /* The max. order of the model */
  unsigned int P_performs_full_excls; /* Indicates whether model performs
					 exclusions or not */
  struct PPMstar_trieType *P_trie;    /* The PPMstar trie model */
  ptable_type *P_ptable;              /* Cumulative probability for order 0 context,
					 used for unbounded alphabets only */
};

/* P_performs_full_excls is also used to store next link on used list when this
   record gets deleted: */
#define P_next P_performs_full_excls

/* Global variables used for storing the PPMstar models */
extern struct PPMstar_modelType *PPMstar_Models;/* List of PPMstar models */

boolean
PPMstar_valid_model (unsigned int PPMstar_model);
/* Returns non-zero if the PPMstar model is valid, zero otherwize. */

unsigned int
PPMstar_create_model (unsigned int alphabet_size, int max_order,
		  unsigned int performs_full_excls);
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
   example). In this case, allowable symbol numbers range from 0 up to a
   special "expand_alphabet" symbol which is equal to the current maximum
   symbol number (this is one more than the highest previously seen symbol
   number). If the current symbol becomes the expand_alphabet symbol, then
   the current maximum symbol number is incremented by 1, thus effectively
   expanding the size of the alphabet by 1. The current maximum symbol number
   may be obtained by calling the routine TLM_get_model. One further
   symbol is permitted in the current alphabet - the sentinel symbol.
*/

void
PPMstar_get_model (unsigned int PPMstar_model, unsigned int *alphabet_size,
	       unsigned int *max_symbol, int *max_order,
	       boolean *performs_full_excls);
/* Returns information about the PPMstar model. The arguments alphabet_size,
   max_order and performs_full_excls are values used to create the model in
   PPMstar_create_model(). The argument max_symbol is set to the current
   maximum symbol number used by the model.*/

void
PPMstar_release_model (unsigned int PPMstar_model);
/* Releases the memory allocated to the model and the model number (which may
   be reused in later PPMstar_create_model or PPMstar_load_model calls).
   A run-time error will be generated if an attempt is made to release
   a model that still has active PPMstar_Contexts pointing at it. */

void
PPMstar_nullify_model (unsigned int model);
/* Replaces the model with the null model and releases the memory allocated
   to it. */

unsigned int
PPMstar_load_model (unsigned int file, unsigned int model_form);
/* Loads the PPMstar model which has been previously saved to the file into memory
   and allocates it a new model number which is returned. */

void
PPMstar_write_model (unsigned int file, unsigned int PPMstar_model,
		 unsigned int model_form);
/* Writes out the PPMstar model to the file (which can then be loaded
   by other applications later). The argument model_form must have the value
   TLM_Static or TLM_Dynamic and determines whether the model is static or
   dynamic when it is later reloaded using TLM_load_model. */

void
PPMstar_dump_model (unsigned int file, unsigned int PPMstar_model, boolean dumptrie,
		void (*dump_symbol_function) (unsigned int, unsigned int));
/* Dumps out the PPMstar model (for debugging purposes). */

unsigned int
PPMstar_create_context (unsigned int model);
/* Creates and returns an unsigned integer which provides a reference to a PPMstar
   context record associated with the model's context. The current position is
   set to the null string. The current symbol is set to the first predicted
   symbol. */

unsigned int
PPMstar_copy_context (unsigned int model, unsigned int context);
/* Creates a new PPMstar context record, copies the contents of the specified
   context into it, and returns an integer reference to it. A run-time error
   occurs if the PPMstar context being copied is for a dynamic model. */

void
PPMstar_overlay_context (unsigned int model, unsigned int old_context,
		     unsigned int context);
/* Overlays the PPMstar context by copying the old context into it. This will
   copy the context even for dynamic models. This is necessary when dynamic
   models with supplementary symbols are being created. */

void
PPMstar_find_symbol (unsigned int model, unsigned int context,
		     unsigned int symbol);
/* Finds the predicted symbol in the PPMstar context. */

void
PPMstar_update_context (unsigned int model, unsigned int context,
		    unsigned int symbol);
/* Updates the PPMstar context record so that the current symbol becomes symbol.
   Returns additional information as specified by the routine
   TLM_set_context_type. */

void
PPMstar_release_context (unsigned int model, unsigned int context);
/* Releases the memory allocated to the PPMstar context and the context number
   (which may be reused in later PPMstar_create_context or PPMstar_copy_context and
   TLM_copy_dynamic_context calls). */

void
PPMstar_reset_symbol (unsigned int model, unsigned int context);
/* Resets the PPMstar context record to point at the first predicted symbol of the
   current position. */

boolean
PPMstar_next_symbol (unsigned int model, unsigned int context,
		 unsigned int *symbol);
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

void
PPMstar_encode_symbol (unsigned int model, unsigned int context,
		   unsigned int coder, unsigned int symbol);
/* Encodes the specified symbol using the arithmetic coder.
   Updates the context record so that the last symbol in the
   PPMstar context becomes the encoded symbol. */

unsigned int
PPMstar_decode_symbol (unsigned int model, unsigned int context,
		   unsigned int coder);
/* Returns the symbol decoded using the arithmetic coder. Updates the
   PPMstar context record so that the last symbol in the context becomes the
   decoded symbol. */

unsigned int
PPMstar_getcontext_position (unsigned int model, unsigned int context);
/* Returns an integer which uniquely identifies the current position
   associated with the PPMstar context. (One implementation is to return a memory
   location corresponding to the current position. This routine is useful if
   you need to check whether different contexts have encoded the same prior
   symbols as when checking whether the context pathways converge in the
   Viterbi or trellis-based algorithms.) */

unsigned int
PPMstar_minlength_model (unsigned int model);
/* Returns the minimum number of bits needed to write the PPMstar model
   out to disk as a static model and recover it later. This is
   useful for computing minimum description lengths of messages.

   Note that the amount returned will always be slightly less than
   the resulting size of the static model produced by TLM_write_model
   as this excludes extra overhead data (including the model's title)
   that is necessary for the functioning of the API. */

unsigned int
PPMstar_sizeof_model (unsigned int model);
/* Returns the current number of bits needed to store the
   model in memory. */

void
PPMstar_stats_model (unsigned int file, unsigned int model);
/* Writes out statistics about the PPMstar model in human readable form. */

#endif
