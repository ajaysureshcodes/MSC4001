/* Routines for PPMo models. */

#ifndef PPMo_MODEL_H
#define PPMo_MODEL_H

#include "bits.h"

/* Types of operations to perform while updating the model */
#define PPMo_UPDATE_SYMBOL 0
#define PPMo_ENCODE_SYMBOL 1
#define PPMo_DECODE_SYMBOL 2

/* Types of data to be returned by PPM_get_model (). */
#define PPMo_Get_Alphabet_Size 0          /* Gets the alphabet size of the PPM symbol model */
#define PPMo_Get_Order_Model_Max_Order 1  /* Gets the maximum order of the PPM order model */
#define PPMo_Get_Symbol_Model_Max_Order 2 /* Gets the maximum order of the PPM symbol model */
#define PPMo_Get_OS_Model_Threshold 3     /* Returns threshold if the PPM model performs Order/Stream modelling */
#define PPMo_Get_Performs_Full_Excls 4    /* Returns true if the PPM model performs exclusions */

/* Types of data to be set/reset by PPM_set_model (). */
#define PPMo_Set_Alphabet_Size 0          /* Sets the alphabet size of the PPM model */
#define PPMo_Set_OS_Model_Threshold 1     /* Sets the threshold level used by the Order/Symbol model */

#define PPMo_DEFAULT_ALPHABET_SIZE 256
#define PPMo_DEFAULT_ORDER_MODEL_ORDER 3
#define PPMo_DEFAULT_SYMBOL_MODEL_ORDER 3
#define PPMo_DEFAULT_OS_MODEL_THRESHOLD 10
#define PPMo_DEFAULT_PERFORMS_FULL_EXCLS FALSE

struct PPMo_modelType
{ /* PPMo model record */
  unsigned int P_alphabet_size;       /* The size of the alphabet for the model */
  int P_order_model_max_order;        /* The max. order of the order stream model */
  int P_symbol_model_max_order;       /* The max. order of the symbol stream model */
  unsigned char *P_trie;              /* The root node of the symbol stream model's trie */
  unsigned int P_orders_size;         /* Size of order model's array *P_orders (i.e. next line) */
  unsigned int *P_orders_model;       /* The order stream model: frequencies used for encoding
					 the orders */
  unsigned int **P_OS_model;          /* The order stream symbol model: frequencies for encoding
					 the orders */
  int P_OS_model_threshold;           /* Threshold used if Order/Symbol order stream modelling is performed */
  boolean *P_mask_orders;             /* TRUE for each order if they are not be used for encoding */
  boolean P_performs_full_excls;      /* TRUE if exclusions are performed */
  bits_type *P_exclusions;            /* Bitmap used to perform exclusions for the model */
  unsigned int P_next;                /* Next in deleted model records list */
};

struct PPMo_contextType
{ /* PPMo context record */
  unsigned int P_order_context;       /* The previous orders are stored here as a multiplication */
  unsigned char **P_symbol_context;   /* The PPMo model's current symbol context (ptrs into trie)
					 [0] = NULL (order 0), [1] = order 1, etc. */
  unsigned int P_prev_symbol;         /* Previously encoded symbol */
/* Note: PPMo_Context contains pointers that point to the parent node that
   contains the child pointer; NULL if it is at the Root */
  unsigned int P_next;                /* Next in deleted model records list */
};

extern float PPMo_Codelength_Orders;  /* Total cost of encoding the orders */
extern float PPMo_Codelength_Symbols; /* Total cost of encoding the symbols */

/* Global variables used for storing PPMo statistics */
extern unsigned int PPMo_determ_symbols;   /* Number of deterministic symbols */
extern unsigned int PPMo_nondeterm_symbols;/* Number of non-deterministic symbols */
extern FILE *PPMo_Orders_fp;          /* File used for writing out the orders */

extern struct PPMo_modelType PPMo_Model; /* The PPMo model */

/* Global variables used for storing the PPMo models */
extern struct PPMo_modelType *PPMo_Models;/* List of PPMo models */
extern struct PPMo_contextType *PPMo_Contexts;/* List of PPMo contexts */

unsigned int
PPMo_eof_symbol (unsigned int ppmo_model);
/* Returns the eof symbol for the PPMo model. */

unsigned int
PPMo_create_model
(unsigned int alphabet_size, int order_model_max_order, int symbol_model_max_order,
 unsigned int mask_orders, int os_model_threshold, boolean performs_full_excls);
/* Creates and returns a new pointer to a PPMo model record. */

void
PPMo_get_model (unsigned int model, unsigned int type, unsigned int *value);
/* Returns information about the PPM model. The arguments type is
   the information to be returned (i.e. PPMo_Get_Alphabet_Size,
   PPMo_Get_Max_Symbol, PPMo_Get_Order_Model_Max_Order, PPMo_Get_Symbol_Model_Max_Order,
   PPMo_Get_OS_Model_Threshold or PPMo_Get_Performs_Full_Excls). */

void
PPMo_set_model (unsigned int model, unsigned int type, unsigned int value);
/* Sets information about the PPMo model. The argument type is the information
   to be set (i.e. PPMo_Set_Alphabet_Size or PPMo_Set_Max_Symbol)
   and its value is set to the argument value.

   The type PPMo_Alphabet_Size is used for specifying the size of the
   expanded alphabet (which must be equal to 0 - indicating an unbounded
   alphabet, or greater than the existing size to accomodate symbols used by
   the existing model).
*/

void
PPMo_release_model (unsigned int ppmo_model);
/* Releases the memory allocated to the model and the model number (which may
   be reused in later PPMo_create_model or PPMo_load_model calls).
   A run-time error will be generated if an attempt is made to release
   a model that still has active PPMo_Contexts pointing at it. */

unsigned int
PPMo_load_model (unsigned int file, unsigned int model_form);
/* Loads the PPM model which has been previously saved to the file into memory
   and allocates it a new model number which is returned. */

void
PPMo_write_model (unsigned int file, unsigned int model,
		  unsigned int model_form);
/* Writes out the PPM model to the file (which can then be loaded
   by other applications later). The argument model_form must have the value
   TLM_Static or TLM_Dynamic and determines whether the model is static or
   dynamic when it is later reloaded using TLM_load_model. */

void
PPMo_dump_orders_model (unsigned int file, unsigned int ppmo_model);
/* Dumps out the order stream model. */

void
PPMo_dump_model (unsigned int file, unsigned int ppmo_model,
		 void (*dump_symbol_function) (unsigned int, unsigned int));
/* Dumps out the PPMo model (for debugging purposes). */

boolean
PPMo_valid_context (unsigned int ppmo_context);
/* Returns non-zero if the PPM context is valid, zero otherwize. */

unsigned int
PPMo_create_context (unsigned int ppmo_model);
/* Creates and returns a new pointer to a PPMo context record. */

unsigned int
PPMo_copy_context (unsigned int model, unsigned int context);
/* Creates a new PPMo context record, copies the contents of the specified
   context into it, and returns an integer reference to it. A run-time error
   occurs if the PPMo context being copied is for a dynamic model. */

void
PPMo_update_context (unsigned int model, unsigned int context,
		     unsigned int symbol);
/* Updates the PPMo context record so that the current symbol becomes symbol.
   This routine just updates the context, and does not return
   any additional information such as the cost of encoding
   the symbol in bits. */

void
PPMo_release_context (unsigned int model, unsigned int context);
/* Releases the memory allocated to the PPMo context and the context number
   (which may be reused in later PPM_create_context calls). */

void
PPMo_encode_symbol (unsigned int model, unsigned int context, unsigned int coder, unsigned int symbol);
/* Encodes the specified symbol using the arithmetic coder.
   Updates the context record so that the last symbol in the
   PPMo context becomes the encoded symbol. */

unsigned int
PPMo_decode_symbol (unsigned int model, unsigned int context, unsigned int coder);
/* Returns the symbol decoded using the arithmetic coder. Updates the
   PPMo context record so that the last symbol in the context becomes the
   decoded symbol. */

unsigned int
PPMo_getcontext_position (unsigned int model, unsigned int context);
/* Returns an integer which uniquely identifies the current position
   associated with the PPMo context. (One implementation is to return a memory
   location corresponding to the current position. This routine is useful if
   you need to check whether different contexts have encoded the same prior
   symbols as when checking whether the context pathways converge in the
   Viterbi or trellis-based algorithms.) */

#endif
