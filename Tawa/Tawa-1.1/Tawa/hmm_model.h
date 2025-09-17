/* TLM routines based on HMM models. */

#include "ptable.h"
#include "io.h"

#ifndef HMM_MODEL_H
#define HMM_MODEL_H

struct HMM_modelType
{ /* HMM model record */
  unsigned int N;    /* Number of states (N);  Q={1,2,...,N} */
  unsigned int M;    /* Number of observation symbols (M); V={1,2,...,M} */
  double **A;        /* Array of transition probs (A[1..N][1..N]).
			A[i][j] is the transition prob of going from state i
			at time t to state j at time t+1 */
  double **B;	     /* Array of observation probs (B[1..N][1..M]).
			B[j][k] is the probability of of observing symbol k
			in state j */
  double *pi;        /* pi[1..N] pi[i] is the initial state distribution. */
  unsigned int Next; /* Used to store next link on used list when this record
			gets deleted. */
};

/* Global variables used for storing the HMM models */
extern struct HMM_modelType *HMM_Models;/* List of HMM models */

boolean
HMM_valid_model (unsigned int HMM_model);
/* Returns non-zero if the HMM model is valid, zero otherwize. */

unsigned int
HMM_create_model (unsigned int N, unsigned int M);
/* Creates and returns a new pointer to a HMM model record. */

void
HMM_get_model (unsigned int HMM_model, unsigned int *N, unsigned int *M);
/* Returns information about the HMM model. The arguments N and M
   are values used to create the model in HMM_create_model(). */

void
HMM_release_model (unsigned int HMM_model);
/* Releases the memory allocated to the model and the model number (which may
   be reused in later HMM_create_model or HMM_load_model calls).
   A run-time error will be generated if an attempt is made to release
   a model that still has active HMM_Contexts pointing at it. */

void
HMM_nullify_model (unsigned int model);
/* Replaces the model with the null model and releases the memory allocated
   to it. */

unsigned int
HMM_load_model (unsigned int file, unsigned int model_form);
/* Loads the HMM model which has been previously saved to the file into memory
   and allocates it a new model number which is returned. */

void
HMM_write_model (unsigned int file, unsigned int HMM_model,
		 unsigned int model_form);
/* Writes out the HMM model to the file (which can then be loaded
   by other applications later). The argument model_form must have the value
   TLM_Static or TLM_Dynamic and determines whether the model is static or
   dynamic when it is later reloaded using TLM_load_model. */

void
HMM_dump_model (unsigned int file, unsigned int HMM_model);
/* Dumps out the HMM model (for debugging purposes). */

boolean
HMM_valid_context (unsigned int context);
/* Returns non-zero if the HMM context is valid, zero otherwize. */

unsigned int
HMM_create_context (unsigned int model);
/* Creates and returns an unsigned integer which provides a reference to a HMM
   context record associated with the model's context. The current position is
   set to the null string. The current symbol is set to the first predicted
   symbol. */

unsigned int
HMM_copy_context (unsigned int model, unsigned int context);
/* Creates a new HMM context record, copies the contents of the specified
   context into it, and returns an integer reference to it. A run-time error
   occurs if the HMM context being copied is for a dynamic model. */

void
HMM_overlay_context (unsigned int model, unsigned int old_context,
		     unsigned int context);
/* Overlays the HMM context by copying the old context into it. This will
   copy the context even for dynamic models. This is necessary when dynamic
   models with supplementary symbols are being created. */

void
HMM_find_symbol (unsigned int model, unsigned int context,
		 unsigned int symbol);
/* Finds the predicted symbol in the HMM context. */

void
HMM_update_context (unsigned int model, unsigned int context,
		    unsigned int symbol);
/* Updates the HMM context record so that the current symbol becomes symbol. */

void
HMM_release_context (unsigned int model, unsigned int context);
/* Releases the memory allocated to the HMM context and the context number
   (which may be reused in later HMM_create_context or HMM_copy_context and
   TLM_copy_dynamic_context calls). */

void
HMM_reset_symbol (unsigned int model, unsigned int context);
/* Resets the HMM context record to point at the first predicted symbol of the
   current position. */

boolean
HMM_next_symbol (unsigned int model, unsigned int context,
		 unsigned int *symbol);
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

void
HMM_encode_symbol (unsigned int model, unsigned int context,
		   unsigned int coder, unsigned int symbol);
/* Encodes the specified symbol using the arithmetic coder.
   Updates the context record so that the last symbol in the
   HMM context becomes the encoded symbol. */

unsigned int
HMM_decode_symbol (unsigned int model, unsigned int context,
		   unsigned int coder);
/* Returns the symbol decoded using the arithmetic coder. Updates the
   HMM context record so that the last symbol in the context becomes the
   decoded symbol. */

unsigned int
HMM_getcontext_position (unsigned int model, unsigned int context);
/* Returns an integer which uniquely identifies the current position
   associated with the HMM context. (One implementation is to return a memory
   location corresponding to the current position. This routine is useful if
   you need to check whether different contexts have encoded the same prior
   symbols as when checking whether the context pathways converge in the
   Viterbi or trellis-based algorithms.) */

unsigned int
HMM_minlength_model (unsigned int model);
/* Returns the minimum number of bits needed to write the HMM model
   out to disk as a static model and recover it later. This is
   useful for computing minimum description lengths of messages.

   Note that the amount returned will always be slightly less than
   the resulting size of the static model produced by TLM_write_model
   as this excludes extra overhead data (including the model's title)
   that is necessary for the functioning of the API. */

unsigned int
HMM_sizeof_model (unsigned int model);
/* Returns the current number of bits needed to store the
   model in memory. */

void
HMM_stats_model (unsigned int file, unsigned int model);
/* Writes out statistics about the HMM model in human readable form. */

#endif
