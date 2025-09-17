/* Routines for PPMq models. */

#ifndef PPMq_MODEL_H
#define PPMq_MODEL_H

/* Defines the type of modelling algorithms */
#define PPMq_Method_A 0       /* Indicates PPMq model uses escape method A */
#define PPMq_Method_B 1       /* Indicates PPMq model uses escape method B */
#define PPMq_Method_C 2       /* Indicates PPMq model uses escape method C */
#define PPMq_Method_D 3       /* Indicates PPMq model uses escape method D */

/* Types of data to be returned by PPM_get_model (). */
#define PPMq_Get_Alphabet_Size 0       /* Gets the alphabet size of the PPM model */
#define PPMq_Get_Max_Order 1           /* Gets the maximum order of the PPM model */
#define PPMq_Get_Escape_Method 2       /* Gets the escape method of the PPM model */
#define PPMq_Get_Performs_Full_Excls 3 /* Returns true if the PPM model performs exclusions */

/* Types of data to be set/reset by PPM_set_model (). */
#define PPMq_Set_Alphabet_Size 0   /* Sets the alphabet size of the PPM model */

struct PPMq_modelType
{ /* PPMq model record */
  unsigned int P_alphabet_size;  /* The size of the alphabet for the model */
  int P_max_order;               /* The max. order of the model */
  unsigned int P_escape_method;  /* The model's escape method */
  boolean P_performs_full_excls; /* The model performs exclusions or not */
  unsigned char *P_trie;         /* The root node of the model's trie */
  unsigned int P_next;           /* Next in deleted model records list */
};

struct PPMq_contextType
{ /* PPMq context record */
  unsigned char **P_context;     /* The PPMq model's current context
				    (ptrs into trie)
				    [0] = NULL (order 0), [1] = order 1, etc. */
/* Note: PPMq_Context contains pointers that point to the parent node that
   contains the child pointer; NULL if it is at the Root */
  unsigned int P_next;           /* Next in deleted model records list */
};

/* Global variables used for storing the PPMq models */
extern struct PPMq_modelType *PPMq_Models;/* List of PPMq models */
extern struct PPMq_contextType *PPMq_Contexts;/* List of PPMq contexts */

void
PPMq_dump_symbol (unsigned int file, unsigned int symbol,
		  void (*dump_symbol_function) (unsigned int, unsigned int));
/* Dump the symbol */

boolean
PPMq_valid_context (unsigned int ppmq_context);
/* Returns non-zero if the PPM context is valid, zero otherwize. */

unsigned int
PPMq_create_context (unsigned int ppmq_model);
/* Creates and returns a new pointer to a PPMq context record. */

void
PPMq_update_context (unsigned int model, unsigned int context,
		     unsigned int symbol);
/* Updates the context record so that the current symbol becomes symbol.
   This routine just updates the context, and does not return
   any additional information such as the cost of encoding
   the symbol in bits. */

void
PPMq_release_context (unsigned int model, unsigned int context);
/* Releases the memory allocated to the PPMq context and the context number
   (which may be reused in later PPM_create_context calls). */

boolean
PPMq_valid_model (unsigned int ppmq_model);
/* Returns non-zero if the PPMq model is valid, zero otherwize. */

unsigned int
PPMq_create_model (unsigned int alphabet_size, int max_order,
		  unsigned int escape_method, boolean performs_full_excls);
/* Creates and returns a new pointer to a PPMq model record. */

void
PPMq_get_model (unsigned int model, unsigned int type, unsigned int *value);
/* Returns information about the PPMq model. The arguments type is
   the information to be returned (i.e. PPMq_Get_Alphabet_Size,
   PPMq_Get_Escape_Method or PPMq_Get_Performs_Full_Excls). */

void
PPMq_set_model (unsigned int model, unsigned int type, unsigned int value);
/* Sets information about the PPMq model. The argument type is the information
   to be set (i.e. PPMq_Set_Alphabet_Size or PPMq_Set_Max_Symbol)
   and its value is set to the argument value.

   The type PPMq_Alphabet_Size is used for specifying the size of the
   expanded alphabet (which must be equal to 0 - indicating an unbounded
   alphabet, or greater than the existing size to accomodate symbols used by
   the existing model).
*/

void
PPMq_release_model (unsigned int ppmq_model);
/* Releases the memory allocated to the model and the model number (which may
   be reused in later PPMq_create_model or PPMq_load_model calls).
   A run-time error will be generated if an attempt is made to release
   a model that still has active PPMq_Contexts pointing at it. */

void
PPMq_dump_model (unsigned int file, unsigned int ppmq_model,
		 void (*dump_symbol_function) (unsigned int, unsigned int));
/* Dumps out the PPMq model (for debugging purposes). */

unsigned int
PPM_load_model (unsigned int file, unsigned int model_form);
/* Loads the PPM model which has been previously saved to the file into memory
   and allocates it a new model number which is returned. */

void
PPM_write_model (unsigned int file, unsigned int model,
		 unsigned int model_form);
/* Writes out the PPM model to the file (which can then be loaded
   by other applications later). The argument model_form must have the value
   TLM_Static or TLM_Dynamic and determines whether the model is static or
   dynamic when it is later reloaded using TLM_load_model. */

void
PPMq_encode_symbol (unsigned int model, unsigned int context,
		    unsigned int coder, unsigned int symbol);
/* Encodes the specified symbol using the arithmetic coder.
   The context is updated so that the last symbol becomes
   the encoded symbol. */

unsigned int
PPMq_decode_symbol (unsigned int model, unsigned int context,
		    unsigned int coder);
/* Returns the symbol decoded using the arithmetic coder. Updates the
   context record so that the last symbol in the context becomes the
   decoded symbol. */

#endif
