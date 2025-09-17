/* Context position hash module. */

#include <stdio.h>
#include <stdlib.h>

#define HASHP_SIZE 5003     /* Size of hash table */
#define HASHP_NUMBER 103669 /* Prime number for hash function */

int
hashp (unsigned int model, unsigned int input_position,
       unsigned int context_position)
{
    int hash;

    hash = (((input_position+1) * model) % HASHP_NUMBER) + (model - 1);
    if (!context_position)
        hash = hash % HASHP_SIZE;
    else
        hash = (context_position * hash) % HASHP_SIZE;
    return hash;
}

/* Scaffolding for hash module */
int
main()
{
    unsigned int model, input_position, m;
    int hash, context_position, hashes [32];

    context_position = 0;

    for (input_position = 0; input_position < 200000; input_position++)
      for (model = 1; model <= 13; model++)
        {
	  hash = hashp (model, input_position, context_position);
	  hashes [model] = hash;
	  /*fprintf (stdout, "model = %d input_position = %d hash = %d\n",
	    model, input_position, hash);*/
	  for (m = 1; m < model; m++)
	    if (hashes [m] == hashes [model])
	      fprintf (stdout, "*** pos %d hash %d m %d model %d\n",
		       input_position, hash, m, model);
        }
}
