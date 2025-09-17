/* Routines for encoding USER defined models. */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include "io.h"
#include "text.h"
#include "sets.h"
#include "coder.h"
#include "model.h"
#include "user_model.h"

#define USER_MODELS_SIZE 4          /* Initial max. number of models */
#define USER_CONTEXTS_SIZE 64       /* Initial max. number of context records */

struct USER_modelType
{ /* USER model record */
  
  unsigned int (*U_create_context_function) (unsigned int);
  void (*U_release_context_function) (unsigned int, unsigned int);
  void (*U_update_context_function) (unsigned int, unsigned int, unsigned int);
  boolean (*U_valid_context_function) (unsigned int);
  unsigned int U_next;   /* Used when the record gets deleted */
};

/* Global variables used for storing the USER models */
struct USER_modelType *USER_Models = NULL;/* List of USER models */
unsigned int USER_Models_max_size = 0;   /* Current max. size of models array */
unsigned int USER_Models_used = NIL;     /* List of deleted model records */
unsigned int USER_Models_unused = 1;     /* Next unused model record */

boolean
USER_valid_model (unsigned int user_model)
/* Returns non-zero if the USER model is valid, zero otherwize. */
{
    if (user_model == NIL)
        return (FALSE);
    else if (user_model >= USER_Models_unused)
        return (FALSE);
    else if (USER_Models [user_model].U_next != 0)
        return (FALSE);
    else
        return (TRUE);
}

unsigned int
USER_create_model1 (void)
/* Creates and returns a new pointer to a USER model record. */
{
    unsigned int user_model, old_size;

    if (USER_Models_used != NIL)
    { /* use the first record on the used list */
        user_model = USER_Models_used;
	USER_Models_used = USER_Models [user_model].U_next;
    }
    else
    {
	user_model = USER_Models_unused;
        if (USER_Models_unused >= USER_Models_max_size)
	{ /* need to extend USER_Models array */
	    old_size = USER_Models_max_size * sizeof (struct modelType);
	    if (USER_Models_max_size == 0)
	        USER_Models_max_size = USER_MODELS_SIZE;
	    else
	        USER_Models_max_size = 10*(USER_Models_max_size+50)/9;

	    USER_Models = (struct USER_modelType *)
	        Realloc (84, USER_Models, USER_Models_max_size *
			 sizeof (struct USER_modelType), old_size);

	    if (USER_Models == NULL)
	    {
	        fprintf (stderr, "Fatal error: out of USER models space\n");
		exit (1);
	    }
	}
	USER_Models_unused++;
    }

    USER_Models [user_model].U_create_context_function = NULL;
    USER_Models [user_model].U_release_context_function = NULL;
    USER_Models [user_model].U_update_context_function = NULL;
    USER_Models [user_model].U_valid_context_function = NULL;
    USER_Models [user_model].U_next = NIL;

    return (user_model);
}

unsigned int
USER_create_model (unsigned int (*create_context_function) (unsigned int),
		   void (*release_context_function) (unsigned int, unsigned int),
		   void (*update_context_function) (unsigned int, unsigned int, unsigned int),
		   boolean (*valid_context_function) (unsigned int))
/* Creates and returns a new pointer to a USER model record. */
{
    unsigned int user_model;

    user_model = USER_create_model1 ();

    USER_Models [user_model].U_create_context_function = create_context_function;
    USER_Models [user_model].U_release_context_function = release_context_function;
    USER_Models [user_model].U_update_context_function = update_context_function;
    USER_Models [user_model].U_valid_context_function = valid_context_function;

    return (user_model);
}

void
USER_get_model (unsigned int model, unsigned int type, unsigned int *value)
/* Gets the user defined functions associated with the model. */
{
    unsigned int user_model;

    user_model = TLM_verify_model (model, TLM_USER_Model, USER_valid_model);

    switch (type)
      {
      case USER_Get_Create_Context_Function:
	*value = (unsigned int) USER_Models [user_model].U_create_context_function;
	break;
      case USER_Get_Release_Context_Function:
	*value = (unsigned int) USER_Models [user_model].U_release_context_function;
	break;
      case USER_Get_Update_Context_Function:
	*value = (unsigned int) USER_Models [user_model].U_update_context_function;
	break;
      case USER_Get_Valid_Context_Function:
	*value = (unsigned int) USER_Models [user_model].U_valid_context_function;
	break;
      default:
	fprintf (stderr, "Invalid type (%d) specified for USER_get_model ()\n", type);
	exit (1);
	break;
      }
}

void
USER_release_model (unsigned int user_model)
/* Releases the memory allocated to the model and the model number (which may
   be reused in later USER_create_model or USER_load_model calls).
   A run-time error will be generated if an attempt is made to release
   a model that still has active USER_Contexts pointing at it. */
{
    assert (USER_valid_model (user_model));

    /* add model record at the head of the USER_Models_used list */
    USER_Models [user_model].U_next = USER_Models_used;
    USER_Models_used = user_model;
}

void
USER_dump_model (unsigned int file, unsigned int user_model)
/* Dumps out the USER model (for debugging purposes). */
{
    assert (USER_valid_model (user_model));

    fprintf (Files [file], "User create context function = %d\n",
	     (unsigned int) USER_Models [user_model].U_create_context_function);
    fprintf (Files [file], "User release context function = %d\n",
	     (unsigned int) USER_Models [user_model].U_release_context_function);
    fprintf (Files [file], "User update context function = %d\n",
	     (unsigned int) USER_Models [user_model].U_update_context_function);
    fprintf (Files [file], "User valid context function = %d\n",
	     (unsigned int) USER_Models [user_model].U_valid_context_function);
}
