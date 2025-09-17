/* TXT routines for alignment queues. */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "model.h"
#include "coder.h"
#include "io.h"
#include "aqueue.h"

#define AQUEUES_SIZE 20           /* Initial size of Indexes array */

struct aqueueType
{ /* Singly linked nodes in the alignment queue. */
    unsigned int Number; /* An unsigned integer number stored in the aqueue. */
    unsigned int Next;   /* The next in the aqueue. */
};

struct aqueueType *Aqueues = NULL; /* Array of aqueue records */
unsigned int Aqueues_max_size = 0;   /* Current max. size of the Aqueuees
				      array */
unsigned int Aqueues_used = NIL;     /* Aqueue of deleted aqueue records */
unsigned int Aqueues_unused = 1;     /* Next unused aqueue record */

/* Current context used by TXT_reset_aqueue and TXT_next_aqueue. */
unsigned int Aqueue_ptr = NIL;

unsigned int 
TXT_create_aqueue (void)
/* Return a pointer to a new alignment queue record. */
{
    struct aqueueType *aqueue;
    unsigned int i, old_size;

    if (Aqueues_used != NIL)
    {	/* Use the first record on the used aqueue */
	i = Aqueues_used;
	Aqueues_used = Aqueues [i].Next;
    }
    else
    {
	i = Aqueues_unused;
	if (Aqueues_unused+1 >= Aqueues_max_size)
	{ /* need to extend Aqueues array */
	    old_size = Aqueues_max_size * sizeof (struct aqueueType);
	    if (Aqueues_max_size == 0)
		Aqueues_max_size = AQUEUES_SIZE;
	    else
		Aqueues_max_size *= 2; /* Keep on doubling the Aqueues array on demand */
	    Aqueues = (struct aqueueType *) Realloc (105, Aqueues,
		     Aqueues_max_size * sizeof (struct aqueueType), old_size);

	    if (Aqueues == NULL)
	    {
		fprintf (stderr, "Fatal error: out of aqueues space\n");
		exit (1);
	    }
	}
	Aqueues_unused++;
    }

    if (i != NIL)
    {
      aqueue = Aqueues + i;

      aqueue->Number = 0;
      aqueue->Next = NIL;
    }

    return (i);
}

void
TXT_dump_aqueue
(unsigned int file, unsigned int aqueue,
 void (*dump_value_function) (unsigned int, unsigned int))
/* Dumps the contents of the alignment queue. */
{
    unsigned int i;

    i = aqueue;
    printf ("Got here %d\n", i);
    while (i != NIL)
      {
	if (dump_value_function)
	    dump_value_function (file, Aqueues [i].Number);
	else
	    fprintf (Files [file], "Aqueue index %d number %d next %d\n",
		     i, Aqueues [i].Number, Aqueues [i].Next);

	i = Aqueues [i].Next;
      }
}

void
TXT_delete_aqueue (unsigned int aqueue)
/* Deletes the alignment queue record and outs it on the used aqueue of deleted records. */ 
{
    /* Append onto head of the used aqueue */
    Aqueues [aqueue].Next = Aqueues_used;
    Aqueues_used = aqueue;
}

void
TXT_release_aqueue (unsigned int aqueue)
/* Frees the contents of the alignment queue for later re-use. */
{
    unsigned int i, next;

    i = aqueue;
    while (i != NIL)
      {
	next = Aqueues [i].Next;
	TXT_delete_aqueue (i);
	i = next;
      }
}

unsigned int
TXT_insert_aqueue (unsigned int aqueue, unsigned int number)
/* Inserts the unsigned integer at the head of the alignment queue. */
{
    unsigned int new;

    new = TXT_create_aqueue ();

    Aqueues [new].Number = number;
    Aqueues [new].Next = aqueue;

    return (new);
}
