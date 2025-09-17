/* Definitions of various structures for tuples (for storing relational data
   and their associated sets). */

#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef SYSTEM_LINUX
#include <getopt.h> /* for getopt on Linux systems */
#endif

#define RELATIONS_DEFAULT_ALLOC 128 /* Initial allocation for Relations array */
#define TUPLES_DEFAULT_ALLOC 128    /* Initial allocation for Tuples array */

#include "model.h"
#include "io.h"
#include "sets.h"
#include "text.h"
#include "table.h"
#include "tuples.h"

#define TUPLES_SIZE 20              /* Initial size of Sets array */

Tuples_type *Tuples = NULL;              /* Array of allocated tuples */
unsigned int Tuples_max_size = 0;   /* Current max. size of the Tuples array */
unsigned int Tuples_used = NIL;     /* List of deleted tuples */
unsigned int Tuples_unused = 1;     /* Next unused tuple */

boolean
TXT_valid_tuples (unsigned int tuples)
/* Returns non-zero if the tuples is valid, zero otherwize. */
{
    if (tuples == NIL)
        return (FALSE);
    else if (tuples >= Tuples_unused)
        return (FALSE);
    else if (Tuples [tuples].Tuples_next)
        return (FALSE); /* The next position gets set whenever the tuples
			   gets deleted */
    else
        return (TRUE);
}

void
alloc_relations (unsigned int tuples, unsigned int relation_id)
/* Allocate enough space to include the relation with relation_id. */
{
    unsigned int old_alloc, new_alloc, p;

    assert (Tuples [tuples].Relations != NULL);

    old_alloc = Tuples [tuples].Relations_alloc;

    if (relation_id >= old_alloc)
      { /* need to extend array */
	new_alloc = 10 * (relation_id + 50)/9;

	Tuples [tuples].Relations = (unsigned int *)
	    Realloc (110, Tuples [tuples].Relations,
		     new_alloc * sizeof (unsigned int ),
		     old_alloc * sizeof (unsigned int ));
	if (Tuples [tuples].Relations == NULL)
	  {
	    fprintf (stderr, "Fatal error: out of relations space\n");
	    exit (1);
	  }
	for (p=old_alloc; p<new_alloc; p++)
	    /* Initialize all the extra relation sets to be NIL */
	    Tuples [tuples].Relations [p] = NIL;

	Tuples [tuples].Relations_alloc = new_alloc;
      }
}

boolean
update_relation (unsigned int tuples, unsigned int key,
		 unsigned int *relation_id)
/* Inserts the relation key into the Relations table
   (if it doesn't already exist) and returns is relation_id
   and TRUE if the relation did not previously exist . */
{
    unsigned int relation_count;
    boolean is_new;

    assert (TXT_valid_tuples (tuples));
    assert (Tuples [tuples].Relations != NULL);

    is_new = TXT_update_table (Tuples [tuples].Relations_table, key,
			       relation_id, &relation_count);
    (*relation_id)++; /* add 1 to always ensure non-zero id */
    return (is_new);
}

unsigned int
getid_relation (unsigned int tuples, unsigned int key)
/* Returns the relation id associated with the relation key. */
{
    unsigned int relation_id, relation_count;

    assert (TXT_valid_tuples (tuples));

    if (!TXT_getid_table (Tuples [tuples].Relations_table, key,
			  &relation_id, &relation_count))
        relation_id = NIL;
    else
        relation_id++; /* add 1 to ensure always a non-zero relation id */

    return (relation_id);
}

void
insert_relation (unsigned int tuples, unsigned int relation_id, unsigned int set_index)
/* Inserts the set_index into the set associated with the relation with id relation_id */
{
    unsigned int relation_set;

    assert (TXT_valid_tuples (tuples));
    assert (Tuples [tuples].Relations != NULL);

    alloc_relations (tuples, relation_id);

    relation_set = Tuples [tuples].Relations [relation_id];
    if (relation_set == NIL)
      {
        relation_set = TXT_create_set ();
	Tuples [tuples].Relations [relation_id] = relation_set;
      }
    TXT_put_set (relation_set, set_index);
}

void
dump_relation (unsigned int file, unsigned int tuples, unsigned int relation_id)
/* Dumps the contents of the set associated with the the relation with id relation_id. */
{
    unsigned int relation_set;

    assert (TXT_valid_file (file));

    assert (TXT_valid_tuples (tuples));
    assert (Tuples [tuples].Relations != NULL);

    assert (relation_id > 0 && relation_id < Tuples [tuples].Relations_alloc);

    fprintf (Files [file], "Relation %d: ", relation_id);

    relation_set = Tuples [tuples].Relations [relation_id];
    TXT_dump_set (file, relation_set);
}

void
dump_relations (unsigned int file, unsigned int tuples)
/* Dumps the contents of the sets associated with all the relations. */
{
    unsigned int relation_id;

    assert (TXT_valid_file (file));
    assert (TXT_valid_tuples (tuples));
    assert (Tuples [tuples].Relations != NULL);

    fprintf (Files [file], "Contents of all the relations: \n");
    for (relation_id = 1; relation_id < Tuples [tuples].Relations_alloc;
	 relation_id++)
      if (Tuples [tuples].Relations [relation_id] != NIL)
	  dump_relation (file, tuples, relation_id);
}

void
alloc_tuples (unsigned int tuples, unsigned int tuple_id)
/* Allocate enough space to include the tuple with tuple_id. */
{
    unsigned int old_alloc, new_alloc, p;

    assert (TXT_valid_tuples (tuples));
    assert (Tuples [tuples].Tuples != NULL);

    old_alloc = Tuples [tuples].Tuples_alloc;

    if (tuple_id >= old_alloc)
      { /* need to extend array */
	new_alloc = 10 * (tuple_id + 50)/9;

	Tuples [tuples].Tuples = (unsigned int *)
	  Realloc (110, Tuples [tuples].Tuples,
            new_alloc * sizeof (unsigned int),
	    old_alloc * sizeof (unsigned int));
	if (Tuples [tuples].Tuples == NULL)
	  {
	    fprintf (stderr, "Fatal error: out of tuples space\n");
	    exit (1);
	  }
	for (p=old_alloc; p<new_alloc; p++)
	  /* Initialize all the extra tuple sets to be NIL */
	    Tuples [tuples].Tuples [p] = NIL;

	Tuples [tuples].Tuples_alloc = new_alloc;
      }
}

unsigned int 
TXT_create_tuples (void)
/* Creates and initializes the tuples. */
{
    unsigned int p, t, old_size;
    Tuples_type *tuples;

    if (Tuples_used != NIL)
    {	/* use the first list of Tuples on the used list */
	t = Tuples_used;
	Tuples_used = Tuples [t].Tuples_next;
    }
    else
    {
	t = Tuples_unused;
	if (Tuples_unused+1 >= Tuples_max_size)
	{ /* need to extend Tuples array */
	    old_size = (Tuples_max_size+2) * sizeof (Tuples_type);
	    if (Tuples_max_size == 0)
		Tuples_max_size = TUPLES_SIZE;
	    else
		Tuples_max_size *= 2; /* Keep on doubling the array on
					 demand */
	    Tuples = (Tuples_type *)
	        Realloc (112, Tuples, (Tuples_max_size+2) *
			 sizeof (Tuples_type), old_size);

	    if (Tuples == NULL)
	    {
		fprintf (stderr, "Fatal error: out of Tuples space\n");
		exit (1);
	    }
	}
	Tuples_unused++;
    }

    tuples = Tuples + t;

    tuples->Relations_table = TXT_create_table (TLM_Dynamic, 0);

    tuples->Relations_alloc = RELATIONS_DEFAULT_ALLOC;
    tuples->Relations = (unsigned int *)
        Malloc (110, RELATIONS_DEFAULT_ALLOC * sizeof (unsigned int ));
    if (tuples->Relations == NULL)
      {
	fprintf (stderr, "Fatal error: out of relations space\n");
	exit (1);
      }
    for (p=0; p<RELATIONS_DEFAULT_ALLOC; p++)
      /* Initialize all the set relations to be NIL */
      tuples->Relations [p] = NIL;

    tuples->Relations_table = TXT_create_table (TLM_Dynamic, 0);

    tuples->Tuples_max = 0;
    tuples->Tuples_alloc = TUPLES_DEFAULT_ALLOC;
    tuples->Tuples = (unsigned int *)
        Malloc (111, TUPLES_DEFAULT_ALLOC * sizeof (unsigned int ));
    if (tuples->Tuples == NULL)
      {
	fprintf (stderr, "Fatal error: out of tuples space\n");
	exit (1);
      }
    for (p=0; p<TUPLES_DEFAULT_ALLOC; p++)
        /* Initialize all the set tuples to be NIL */
        tuples->Tuples [p] = NIL;

    tuples->Tuples_next = NIL;

    return (t);
}

unsigned int
TXT_create_tuple (unsigned int tuples)
/* Creates a new tuple and returns its id. */
{
    unsigned int tuple_id;

    assert (TXT_valid_tuples (tuples));

    tuple_id = ++(Tuples [tuples].Tuples_max);
    alloc_tuples (tuples, tuple_id);

    return (tuple_id);
}

boolean
TXT_valid_tuple (unsigned int tuples, unsigned int tuple_id)
{
    assert (TXT_valid_tuples (tuples));

    return ((tuple_id > NIL) && (tuple_id <= Tuples [tuples].Tuples_max));
}

void
TXT_addkey_tuple (unsigned int tuples, unsigned int tuple_id,
		  unsigned int key)
/* Adds a key relation to the tuple with id tuple_id. */
{
    unsigned int relation_id;

    assert (TXT_valid_tuples (tuples));
    assert (TXT_valid_tuple (tuples, tuple_id));

    update_relation (tuples, key, &relation_id);
    insert_relation (tuples, relation_id, tuple_id);
}

void
TXT_update_tuple (unsigned int tuples, unsigned int tuple_id,
		  unsigned int set_index)
/* Inserts the set_index into the set associated with the tuple with id
   tuple_id */
{
    unsigned int tuple_set;

    assert (TXT_valid_tuples (tuples));
    assert (Tuples [tuples].Tuples != NULL);
    assert (TXT_valid_tuple (tuples, tuple_id));

    tuple_set = Tuples [tuples].Tuples [tuple_id];
    if (tuple_set == NIL)
      {
        tuple_set = TXT_create_set ();
	Tuples [tuples].Tuples [tuple_id] = tuple_set;
      }
    TXT_put_set (tuple_set, set_index);
}

void
TXT_dump_tuples (unsigned int file, unsigned int tuples,
		 void (*dump_relation_function) (unsigned int, unsigned int))
/* Dumps all the tuples and their associated sets to the file. */
{
    unsigned int relation_text, relation_id, relation_count;
    unsigned int id;

    assert (TXT_valid_file (file));

    assert (TXT_valid_tuples (tuples));
    assert (Tuples [tuples].Relations != NULL);

    fprintf (Files [file], "\nList of tuple sets: \n");
    for (id = 1; id <= Tuples [tuples].Tuples_max; id++)
      {
        fprintf (Files [file], "tuple %5d: ", id);
	TXT_dump_set (file, Tuples [tuples].Tuples [id]);
	fprintf (Files [file], "\n");
      }

    fprintf (Files [file], "Table of relation sets: \n");
    TXT_reset_table (Tuples [tuples].Relations_table);
    while (TXT_next_table (Tuples [tuples].Relations_table,
			   &relation_text, &relation_id, &relation_count))
      {
	relation_id++; /* add 1 to always ensure non-zero relation id */
	fprintf (Files [file], "tuple key %5d: ", relation_id);
	if (dump_relation_function == NULL)
	    TXT_dump_text (file, relation_text, NULL);
	else
	    dump_relation_function (file, relation_text);
	fprintf (Files [file], "\n                ");
	assert (Tuples [tuples].Relations [relation_id] != NIL);
	TXT_dump_set (file, Tuples [tuples].Relations [relation_id]);
      }
}

unsigned int 
TXT_load_tuples (unsigned int file)
/* Loads all the tuples and their associated sets from the file. These
   must have been written out using TXT_write_tuples ().*/
{
    unsigned int relation_text, relation_id, relation_count;
    unsigned int id, table_type, types, tokens;
    unsigned int tuples;

    assert (TXT_valid_file (file));

    tuples = TXT_create_tuples ();

    /* Read in the number of tuples  */
    Tuples [tuples].Tuples_max = fread_int (file, INT_SIZE);
    alloc_tuples (tuples, Tuples [tuples].Tuples_max);

    /* Read in the tuple sets */
    for (id = 1; id <= Tuples [tuples].Tuples_max; id++)
	Tuples [tuples].Tuples [id] = TXT_load_set (file);

    /* Read in the Relations table */
    Tuples [tuples].Relations_table = TXT_load_table (file);
    TXT_getinfo_table (Tuples [tuples].Relations_table, &table_type, &types, &tokens);
    alloc_relations (tuples, types);

    /* Read in the Relations sets */
    TXT_reset_table (Tuples [tuples].Relations_table);
    while (TXT_next_table (Tuples [tuples].Relations_table,
			   &relation_text, &relation_id, &relation_count))
      {
	relation_id++; /* add 1 to always ensure non-zero relation_id */
	Tuples [tuples].Relations [relation_id] = TXT_load_set (file);
      }

    return (tuples);
}

void
TXT_write_tuples (unsigned int file, unsigned int tuples)
/* Writes out all the tuples and their associated sets to the file. These
   can then be later reloaded using TXT_load_tuples ().*/
{
    unsigned int relation_text, relation_id, relation_count;
    unsigned int id;

    assert (TXT_valid_file (file));

    assert (TXT_valid_tuples (tuples));
    assert (Tuples [tuples].Relations != NULL);
    assert (Tuples [tuples].Tuples != NULL);

    /* Write out the number of tuples  */
    fwrite_int (file, Tuples [tuples].Tuples_max, INT_SIZE);

    /* Write out the tuple sets */
    for (id = 1; id <= Tuples [tuples].Tuples_max; id++)
      {
	assert (Tuples [tuples].Tuples [id] != NIL);
	TXT_write_set (file, Tuples [tuples].Tuples [id]);
      }

    /* Write out the Relations table */
    TXT_write_table (file, Tuples [tuples].Relations_table, TLM_Dynamic);

    /* Write out the Relations sets */
    TXT_reset_table (Tuples [tuples].Relations_table);
    while (TXT_next_table (Tuples [tuples].Relations_table,
			   &relation_text, &relation_id, &relation_count))
      {
	relation_id++; /* add 1 to always ensure non-zero relation_id */
	assert (Tuples [tuples].Relations [relation_id] != NIL);
	TXT_write_set (file, Tuples [tuples].Relations [relation_id]);
      }
}

unsigned int 
TXT_start_query (unsigned int tuples, unsigned int key)
/* Initializes the set of tuple candidates for the query to the set of
   tuples that match the key relation. */
{
    unsigned int relation_id;
    unsigned int tuple_set;

    assert (TXT_valid_tuples (tuples));
    assert (Tuples [tuples].Relations != NULL);

    relation_id = getid_relation (tuples, key);
    if (relation_id == NIL)
	return (NIL);

    tuple_set = TXT_copy_set (Tuples [tuples].Relations [relation_id]);
    return (tuple_set);
}

unsigned int 
TXT_refine_query (unsigned int tuples, unsigned int tuple_set, unsigned int key)
/* Refines the set of tuple candidates for the query by intersection them with
   those tuples that also match the key relation as well. */
{
    unsigned int new_tuple_set;
    unsigned int relation_id;

    assert (TXT_valid_tuples (tuples));
    assert (Tuples [tuples].Relations != NULL);

    assert (tuple_set != NIL);

    relation_id = getid_relation (tuples, key);
    if (relation_id == NIL)
        new_tuple_set = NIL;
    else
        new_tuple_set = TXT_intersect_set (tuple_set, Tuples [tuples].Relations [relation_id]);

    TXT_release_set (tuple_set);
    return (new_tuple_set);
}

unsigned int 
TXT_complete_query (unsigned int tuples, unsigned int tuple_set)
/* Completes the query by returning the union of the tuple sets specified
   by tuple_set. */
{
    unsigned int *members, p;
    unsigned int set;

    assert (TXT_valid_tuples (tuples));
    assert (Tuples [tuples].Tuples != NULL);

    set = TXT_create_set ();

    members = TXT_get_setmembers (tuple_set);
    if (members == NULL)
        return (NIL);

    for (p = 1; p <= members [0]; p++)
      {
	assert (Tuples [tuples].Tuples [p] != NIL);
	TXT_union_set (set, Tuples [tuples].Tuples [members [p]]);
      }
    TXT_release_set (tuple_set);
    TXT_release_setmembers (members);
    return (set);
}

void
TXT_put_relationtext (unsigned int relation_text, unsigned int key,
		      unsigned int attribute)
/* Inserts the relation (key = attribute) text into the relation text
   (used for distinguishing keys for lookup purposes). */
{
    assert (TXT_valid_text (key));

    TXT_setlength_text (relation_text, 0);
    TXT_append_text (relation_text, key);
    TXT_append_symbol (relation_text, TXT_sentinel_symbol ());
    TXT_append_text (relation_text, attribute);
    TXT_append_symbol (relation_text, TXT_sentinel_symbol ());
}

void
TXT_get_relationtext (unsigned int relation_text, unsigned int key,
		      unsigned int attribute)
/* Extracts from the relation text the key and attribute. */
{
    unsigned int length, symbol, pos;
    boolean valid_relation;

    assert (TXT_valid_text (key));
    assert (TXT_valid_text (attribute));

    length = TXT_length_text (relation_text);
    valid_relation =
        (TXT_getpos_text (relation_text, TXT_sentinel_symbol (), &pos) &&
	 (pos > 0) && (pos < length));
    assert (valid_relation != FALSE);

    valid_relation =
        (TXT_get_symbol (relation_text, length-1, &symbol) &&
	 (symbol == TXT_sentinel_symbol ()));
    assert (valid_relation != FALSE);

    TXT_extract_text (relation_text, key, 0, pos);
    TXT_extract_text (relation_text, attribute, pos+1, length-pos-2);
}

unsigned int KeyText = NIL;
unsigned int AttributeText = NIL;

void
TXT_dump_relationtext (unsigned int file, unsigned int relation_text)
/* Dumps the relation text (for debugging purposes). */
{
    assert (TXT_valid_file (file));

    if (KeyText == NIL)
        KeyText = TXT_create_text ();
    if (AttributeText == NIL)
        AttributeText = TXT_create_text ();

    TXT_get_relationtext (relation_text, KeyText, AttributeText);
    fprintf (Files [file], "{");
    TXT_dump_text (file, KeyText, NULL);
    fprintf (Files [file], " = ");
    TXT_dump_text (file, AttributeText, NULL);
    fprintf (Files [file], "}");
}

