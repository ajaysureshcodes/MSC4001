/* TXT routines for lists. For the time being, there is only one
   implementation - frequency sorted, doubly linked lists. */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "model.h"
#include "coder.h"
#include "io.h"
#include "list.h"

#define LISTS_SIZE 20           /* Initial size of Indexes array */
#define LIST_INVALID 2147483647 /* Used to indicate invalid index */
#define LIST_END 2147483646     /* Used to indicate end of list when it has been
				   written out to a file */


struct listType
{ /* Doubly linked frequency sorted nodes in the list. */
    unsigned int Number; /* An unsigned integer number stored in the list. */
    unsigned int Freq;   /* The frequency count associated with the number. */
    unsigned int Next;   /* The next in the list. */
};

struct listType *Lists = NULL; /* Array of list records */
unsigned int Lists_max_size = 0;   /* Current max. size of the Listes
				      array */
unsigned int Lists_used = NIL;     /* List of deleted list records */
unsigned int Lists_unused = 1;     /* Next unused list record */

/* Current context used by TXT_reset_list and TXT_next_list. */
unsigned int List_ptr = NIL;

boolean
TXT_valid_list (unsigned int list)
/* Returns non-zero if the list is valid, zero otherwize. */
{
    if (list == NIL)
        return (FALSE);
    else if (list >= Lists_unused)
        return (FALSE);
    else if (Lists [list].Number == LIST_INVALID)
        return (FALSE);
    else
        return (TRUE);
}

unsigned int 
TXT_create_list (void)
/* Return a pointer to a new list record. */
{
    struct listType *list;
    unsigned int i, old_size;

    if (Lists_used != NIL)
    {	/* Use the first record on the used list */
	i = Lists_used;
	Lists_used = Lists [i].Next;
    }
    else
    {
	i = Lists_unused;
	if (Lists_unused+1 >= Lists_max_size)
	{ /* need to extend Lists array */
	    old_size = Lists_max_size * sizeof (struct listType);
	    if (Lists_max_size == 0)
		Lists_max_size = LISTS_SIZE;
	    else
		Lists_max_size *= 2; /* Keep on doubling the Lists array on demand */
	    Lists = (struct listType *) Realloc (105, Lists,
		     Lists_max_size * sizeof (struct listType), old_size);

	    if (Lists == NULL)
	    {
		fprintf (stderr, "Fatal error: out of lists space\n");
		exit (1);
	    }
	}
	Lists_unused++;
    }

    if (i != NIL)
    {
      list = Lists + i;

      list->Number = LIST_END;
      list->Freq = 0;
      list->Next = NIL;
    }

    return (i);
}

void
TXT_dump_list
(unsigned int file, unsigned int list,
 void (*dump_value_function) (unsigned int, unsigned int, unsigned int))
/* Dumps the contents of the list. */
{
    unsigned int i;

    assert (TXT_valid_list (list));

    i = list;
    while (i != NIL)
      {
	if (Lists [i].Number == LIST_END)
	    break;

	if (dump_value_function)
	    dump_value_function (file, Lists [i].Number, Lists [i].Freq);
	else
	    fprintf (Files [file], "List index %d number %d frequency %d next %d\n",
		     i, Lists [i].Number, Lists [i].Freq, Lists [i].Next);

	i = Lists [i].Next;
      }
}

void
TXT_delete_list (unsigned int list)
/* Deletes the list record and outs it on the used list of deleted records. */ 
{
    Lists [list].Number = LIST_INVALID;
    Lists [list].Freq = 0;

    /* Append onto head of the used list */
    Lists [list].Next = Lists_used;
    Lists_used = list;
}

void
TXT_release_list (unsigned int list)
/* Frees the contents of the list for later re-use. */
{
    unsigned int i, next;

    assert (TXT_valid_list (list));

    i = list;
    while (i != NIL)
      {
	next = Lists [i].Next;
	TXT_delete_list (i);
	i = next;
      }
}

unsigned int
TXT_get_list (unsigned int list, unsigned int number)
/* Returns the frequency of the unsigned integer number in the list
   (0 if it is not found in the list). */
{
    unsigned int i;

    assert (TXT_valid_list (list));

    i = list;
    while (i != NIL)
      {
	if (Lists [i].Number == number)
	  return (Lists [i].Freq);

	i = Lists [i].Next;
      }

    return (0);
}

void
TXT_put_list (unsigned int list, unsigned int number, unsigned int frequency)
/* Puts the unsigned integer into the list if it does not already exist,
   and adds frequency to its associated frequency count. */
{
    unsigned int head, next, prev, new, freq, i;

    assert (TXT_valid_list (list));

    head = list;

    i = list;
    freq = frequency;
    prev = NIL;
    while (i != NIL)
      {
	if (Lists [i].Number == number)
	  { /* Found the number */
	    freq += Lists [i].Freq;

	    if (i == list)
	      { /* At the head of the list - no need to re-order it */
		Lists [i].Freq = freq;
		return;
	      }

	    /* Delete the record; temporarily */
	    next = Lists [i].Next;
	    assert (prev != NIL); /* Can't be at the head */
	    Lists [prev].Next = next;

	    TXT_delete_list (i); /* Add to unused list */

	    break;
	  }

	prev = i;
	i = Lists [i].Next;
      }

    /* Now (re-)insert into the list in frequency order; it will
       use the last deleted record if there was one */
    i = list;
    prev = NIL;
    while (i != NIL)
      {
	assert (Lists [i].Number != number);

	if (freq >= Lists [i].Freq)
	  { /* Insert a "new" record here */
	    new = TXT_create_list ();
	    if (prev == NIL)
	      { /* At the head - to maintain list pointing to the
		   head of the list - we must swap the contents of
		   head with new */
		Lists [new].Number = Lists [head].Number;
		Lists [new].Freq = Lists [head].Freq;
		Lists [new].Next = Lists [head].Next;

		Lists [head].Number = number;
		Lists [head].Freq = freq;
		Lists [head].Next = new;

		break;
	      }
	    else
	      {
		Lists [new].Number = number;
		Lists [new].Freq = freq;
		Lists [new].Next = i;

		assert (prev != NIL);
		Lists [prev].Next = new;

		break;
	      }
	  }

	prev = i;
	i = Lists [i].Next;
      }
}

unsigned int 
TXT_load_list (unsigned int file)
/* Loads the list from the file. */
{
    unsigned int number, freq, new;

    new = TXT_create_list ();
    for (;;)
      {
	number = fread_int (file, INT_SIZE);
	if (number == LIST_END)
	    break;

	freq = fread_int (file, INT_SIZE);
	TXT_put_list (new, number, freq);
      }

    return (new);
}

void
TXT_write_list (unsigned int file, unsigned int list)
/* Writes the list to the file. */
{
    unsigned int i;

    assert (TXT_valid_list (list));

    i = list;
    while (i != NIL)
      {
	fwrite_int (file, Lists [i].Number, INT_SIZE);
	if (Lists [i].Number == LIST_END)
	    break;
	fwrite_int (file, Lists [i].Freq, INT_SIZE);

	i = Lists [i].Next;
      }
}

void
TXT_reset_list (unsigned int list)
/* Resets the current record so that the next call to TXT_next_list
   will return the first record in the list. */
{
    List_ptr = list;
}

boolean
TXT_next_list (unsigned int list, unsigned int *number,
	       unsigned int *frequency)
/* Returns the next record in the list. Also returns the number and frequency
   stored in the record. Note that a call to TXT_reset_list will reset the
   list that is being sequentially accessed (i.e. Multiple lists cannot
   be accessed simultaneously in this implementation). */
{
    *number = Lists [List_ptr].Number;
    *frequency = Lists [List_ptr].Freq;
    List_ptr = Lists [List_ptr].Next;

    return (*number != LIST_END);
}
