/* Test routines for USER models. */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <math.h>
#include <string.h>

#ifdef SYSTEM_LINUX
#include <getopt.h> /* for getopt on Linux systems */
#endif

#include "user_model.h"
#include "model.h"

unsigned int
my_create_context (unsigned int model)
{
    fprintf (stderr, "Create context for model %d\n", model);
    return (1);
}

void
my_release_context (unsigned int model, unsigned int context)
{
    fprintf (stderr, "Release context %d for model %d\n", context, model);
}

void
my_update_context (unsigned int model, unsigned int context,
		   unsigned int symbol)
{
    fprintf (stderr, "Update context %d for model %d, symbol %d\n",
	     model, context, symbol);
}

boolean
my_valid_context (unsigned int context)
{
    fprintf (stderr, "Valid context %d\n", context);
    return (1);
}

int
main (int argc, char *argv[])
{
    unsigned int model, context;

    model = TLM_create_model
      (TLM_USER_Model, "My Title", my_create_context, my_release_context,
       my_update_context, my_valid_context);

    context = TLM_create_context (model);
    fprintf (stderr, "Valid context = %d ", TLM_valid_context (model, context));
    TLM_update_context (model, context, 55);
    TLM_release_context (model, context);

    exit (1);
}
