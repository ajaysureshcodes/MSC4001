/* TLM routines for training semi-adaptive Markov Models. */

#include "io.h"

#ifndef SAMM_MODEL_H
#define SAMM_MODEL_H

/* Types of data to be returned by PPM_get_model (). */
#define SAMM_Get_Alphabet_Size 0  /* Gets the alphabet size of the SAMM model */
#define SAMM_Get_Max_Order 1      /* Gets the maximum order of the SAMM model */
#define SAMM_Get_Escape_Method 2  /* Gets the escape method of the SAMM model */

struct SAMM_modelType
{ /* SAMM model record */
  unsigned int Q_alphabet_size;  /* The number of symbols in the alphabet;
				    0 means an unbounded sized alphabet (which
				    incrementally increases as each new
				    symbol is added */
  int Q_max_order;               /* The max. order of the model */
  unsigned int Q_escape_method;  /* The model's escape method */
  unsigned char *Q_Root;         /* The root node of the model's trie */
  unsigned int Q_model_size;     /* Number of bytes required to store dynamic model */
};

/* Q_alphabet_size is also used to store next link on used list when this
   record gets deleted: */
#define Q_next Q_alphabet_size

/* Global variables used for storing the PPM models */
extern struct SAMM_modelType *SAMM_Models;/* List of PPM models */

void
SAMM_dump_symbol (unsigned int file, unsigned int symbol,
		  void (*dump_symbol_function) (unsigned int, unsigned int));
/* Dump the symbol */

boolean
SAMM_valid_context (unsigned int context);
/* Returns non-zero if the SAMM model is valid, zero otherwize. */

boolean
SAMM_valid_model (unsigned int ppmq_model);
/* Returns non-zero if the SAMM model is valid, zero otherwize. */

unsigned int
SAMM_create_model (unsigned int alphabet_size, int max_order,
		   unsigned int escape_method);
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

void
SAMM_get_model (unsigned int model, unsigned int type,
		unsigned int *value);
/* Returns information about the SAMM model. The arguments type is
   the information to be returned (i.e. SAMM_Get_Alphabet_Size,
   SAMM_Get_Max_Order, SAMM_Get_Escape_Method). */

void
SAMM_release_model (unsigned int ppmq_model);
/* Releases the memory allocated to the model and the model number (which may
   be reused in later SAMM_create_model or SAMM_load_model calls).
   A run-time error will be generated if an attempt is made to release
   a model that still has active SAMM_Contexts pointing at it. */

unsigned int
SAMM_create_context (unsigned int model);
/* Creates and returns an unsigned integer which provides a reference to a SAMM
   context record associated with the model's context. The current position is
   set to the null string. The current symbol is set to the first predicted
   symbol. A run-time error occurs if the SAMM context being copied is for a
   dynamic model. */

unsigned int
SAMM_copy_context (unsigned int model, unsigned int context);
/* Creates a new SAMM context record, copies the contents of the specified
   context into it, and returns an integer reference to it. A run-time error
   occurs if the SAMM context being copied is for a dynamic model. */

void
SAMM_release_context (unsigned int model, unsigned int context);
/* Releases the memory allocated to the SAMM context and the context number
   (which may be reused in later SAMM_create_context or SAMM_copy_context and
   TLM_copy_dynamic_context calls). */

unsigned int
SAMM_load_model (unsigned int file, unsigned int model_form);
/* Loads the SAMM model which has been previously saved to the file into memory
   and allocates it a new model number which is returned. */

void
SAMM_write_model (unsigned int file, unsigned int model,
		  unsigned int model_form, unsigned int model_type);
/* Writes out the PPM model to the file as a static PPM model (note: not fast PPM) 
   (which can then be loaded by other applications later). */

void
SAMM_dump_model (unsigned int file, unsigned int ppmq_model,
		 void (*dump_symbol_function) (unsigned int, unsigned int));
/* Dumps out the SAMM model (for debugging purposes). */

void
SAMM_check_model (unsigned int file, unsigned int ppmq_model,
		  void (*dump_symbol_function) (unsigned int, unsigned int));
/* Checks the SAMM model is consistent (for debugging purposes). */

void
SAMM_update_context (unsigned int model, unsigned int context,
		    unsigned int symbol);
/* Updates the SAMM context record so that the current symbol becomes symbol. */

unsigned int
SAMM_sizeof_model (unsigned int model);
/* Returns the memory usage for SAMM models. */

void
SAMM_encode_symbol (unsigned int model, unsigned int context,
		    unsigned int coder, unsigned int symbol);
/* Encodes the specified symbol using the arithmetic coder.
   Updates the context record so that the last symbol in the
   SAMM context becomes the encoded symbol. */

unsigned int
SAMM_decode_symbol (unsigned int model, unsigned int context,
		    unsigned int coder);
/* Returns the symbol decoded using the arithmetic coder. Updates the
   SAMM context record so that the last symbol in the context becomes the
   decoded symbol. */

#endif
