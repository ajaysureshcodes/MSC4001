/* Definitions of various structures for tuples (for storing relational data
   and their associated sets). */

#ifndef TUPLES_H
#define TUPLES_H

typedef struct
{ /* for storing a set of tuples (relational data) */
    unsigned int *Relations;      /* Pointers to a set associated with each
				     relation */
    unsigned int Relations_alloc; /* Allocation for Relations array */
    unsigned int Relations_table; /* The list of relations and their
				     associated ids */

    unsigned int *Tuples;         /* Pointers to a set associated with each
				     tuple */
    unsigned int Tuples_alloc;    /* Allocation for Tuples array */
    unsigned int Tuples_max;      /* Maximum unused tuple id number. */
    unsigned int Tuples_next;     /* Next in deleted Tuples list */
} Tuples_type;

unsigned int
TXT_create_tuples (void);
/* Creates and initializes the tuples. */

boolean
TXT_valid_tuple (unsigned int tuples, unsigned int tuple_id);

unsigned int
TXT_create_tuple (unsigned int tuples);
/* Creates a new tuple and returns its id. */

void
TXT_addkey_tuple (unsigned int tuples, unsigned int tuple_id,
		  unsigned int key);
/* Adds a key relation to the tuple with id tuple_id. */

void
TXT_update_tuple (unsigned int tuples, unsigned int tuple_id,
		  unsigned int set_index);
/* Inserts the set_index into the set associated with the tuple with id
   tuple_id */

void
TXT_dump_tuples (unsigned int file, unsigned int tuples,
		 void (*dump_relation_function) (unsigned int, unsigned int));
/* Dumps all the tuples and their associated sets to the file. */

unsigned int 
TXT_load_tuples (unsigned int file);
/* Loads all the tuples and their associated sets from the file. These
   must have been written out using TXT_write_tuples ().*/

void
TXT_write_tuples (unsigned int file, unsigned int tuples);
/* Writes out all the tuples and their associated sets to the file. These
   can then be later reloaded using TXT_load_tuples ().*/

unsigned int 
TXT_start_query (unsigned int tuples, unsigned int key);
/* Initializes the set of tuple candidates for the query to the set of
   tuples that match the key relation. */

unsigned int 
TXT_refine_query (unsigned int tuples, unsigned int tuple_set,
		  unsigned int key);
/* Refines the set of tuple candidates for the query by intersection them with
   those tuples that also match the key relation as well. */

unsigned int 
TXT_complete_query (unsigned int tuples, unsigned int tuple_set);
/* Completes the query by returning the union of the tuple sets specified
   by tuple_set. */

void
TXT_put_relationtext (unsigned int relation_text, unsigned int key,
		      unsigned int attribute);
/* Inserts the relation (key = attribute) text into the relation text
   (used for distinguishing keys for lookup purposes). */

void
TXT_get_relationtext (unsigned int relation_text, unsigned int key,
		      unsigned int attribute);
/* Extracts from the relation text the key and attribute. */

void
TXT_dump_relationtext (unsigned int file, unsigned int relation_text);
/* Dumps the relation text (for debugging purposes). */

#endif
