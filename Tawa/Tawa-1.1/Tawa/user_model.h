/* Routines for encoding USER defined models. */

#ifndef USER_MODEL_H
#define USER_MODEL_H

#include "model.h"

/* Types of data to be returned by PPM_get_model (). */
#define USER_Get_Create_Context_Function 0    /* Gets the create_context function. */
#define USER_Get_Release_Context_Function 1   /* Gets the release_context function. */
#define USER_Get_Update_Context_Function 2    /* Gets the update_context function. */
#define USER_Get_Valid_Context_Function 3     /* Gets the valid_context function. */

boolean
USER_valid_model (unsigned int user_model);
/* Returns non-zero if the USER model is valid, zero otherwize. */

unsigned int
USER_create_model (unsigned int (*create_context_function) (unsigned int),
		   void (*release_context_function) (unsigned int, unsigned int),
		   void (*update_context_function) (unsigned int, unsigned int, unsigned int),
		   boolean (*valid_context_function) (unsigned int));
/* Creates and returns a new pointer to a USER model record. */

void
USER_get_model (unsigned int model, unsigned int type, unsigned int *value);
/* Gets the user defined functions associated with the model. */

void
USER_release_model (unsigned int user_model);
/* Releases the memory allocated to the model and the model number (which may
   be reused in later USER_create_model or USER_load_model calls).
   A run-time error will be generated if an attempt is made to release
   a model that still has active USER_Contexts pointing at it. */

void
USER_dump_model (unsigned int file, unsigned int user_model);
/* Dumps out the USER model (for debugging purposes). */

#endif
