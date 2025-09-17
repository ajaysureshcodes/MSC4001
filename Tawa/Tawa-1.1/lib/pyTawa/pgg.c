/* Routines for probabilistic generative grammars. */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "model.h"
#include "io.h"
#include "text.h"
#include "transform.h"
#include "pgg.h"

#define MAX_LINE 2048

int PGG_Malloc = 0;

struct PGG_list_type *
PGG_add_list (struct PGG_list_type *head, float codelength, unsigned int PGG, unsigned int PGG_type)
/* Adds a PGG to the PGG list. */
{
    struct PGG_list_type *here, *newnode;

    /* Does the PGG already exist in the list? */
    here = head;
    while ((here != NULL) && TXT_compare_text (PGG, here->PGG))
	here = here->Cnext;
    if (here != NULL)
    { /* found - replace the codelength and PGG_type */
	here->Codelength = codelength;
	here->PGG_type = PGG_type;
    }
    else
    { /* Not found - add at the head of the list */
	PGG_Malloc += sizeof (struct PGG_list_type);
        newnode = (struct PGG_list_type *)
	    Malloc (41, sizeof(struct PGG_list_type));
	newnode->PGG = TXT_copy_text (PGG);
	newnode->PGG_type = TXT_copy_text (PGG_type);
	newnode->Codelength = codelength;
	if (head != NULL)
	    newnode->Cnext = head;
	else
	    newnode->Cnext = NULL;
	head = newnode;
    }
    return (head);
}

void
dumpPGGText (unsigned int file, unsigned int PGG_text,
		   unsigned int PGG_type)
/* For dumping out the PGG text and type. */
{
    unsigned int pos, symbol, type;
    unsigned int range_text, range_symbol, range_pos;

    assert (TXT_valid_file (file));

    pos = 0;
    while (TXT_get_symbol (PGG_text, pos, &symbol))
    {
      TXT_get_symbol (PGG_type, pos, &type);
      if (!type)
	  TXT_dump_symbol1 (file, symbol);
      else
      {
	switch (type)
	  {
	  case TRANSFORM_SYMBOL_TYPE:
	    fprintf (Files [file], "%%s");
	    break;
	  case TRANSFORM_MODEL_TYPE:
	    fprintf (Files [file], "%%m");
	    break;
	  case TRANSFORM_BOOLEAN_TYPE:
	    fprintf (Files [file], "%%b");
	    break;
	  case TRANSFORM_FUNCTION_TYPE:
	    fprintf (Files [file], "%%f");
	    break;
	  case TRANSFORM_WILDCARD_TYPE:
	    fprintf (Files [file], "%%w");
	    break;
	  case TRANSFORM_RANGE_TYPE:
	    range_text = symbol;
	    if (range_text == NIL)
	        fprintf (Files [file], "%%r");
	    else
	      {
	        fprintf (Files [file], "%%[");
		for (range_pos = 0; range_pos < TXT_length_text (range_text); range_pos++)
		  {
		    TXT_get_symbol (range_text, range_pos, &range_symbol);
		    TXT_dump_symbol (file, range_symbol);
		  }
	        fprintf (Files [file], "]");
	      }
	    break;
	  case TRANSFORM_SENTINEL_TYPE:
	    fprintf (Files [file], "%%$");
	    break;
	  case TRANSFORM_GHOST_TYPE:
	    fprintf (Files [file], "%%_");
	    break;
	  case TRANSFORM_SUSPEND_TYPE:
	    fprintf (Files [file], "%%.");
	    break;
	  default:
	    fprintf (Files [file], "invalid transform type: %d", type);
	    break;
	  }
      }
      pos++;
    }
}

void
dumpPGGList (unsigned int file, struct PGG_list_type *head)
/* Dumps out the PGG in the PGG list. */
{
    struct PGG_list_type *here;

    here = head;
    while (here != NULL)
    {
        fprintf (Files [file], " ");
	dumpPGGText (file, here->PGG, here->PGG_type);
        fprintf (Files [file], " [%.3f]", here->Codelength );
	here = here->Cnext;
    }
}

void
releasePGGList (struct PGG_list_type *head)
/* Release the PGG in the PGG list to the free list. */
{
    struct PGG_list_type *here, *there;

    here = head;
    while (here != NULL)
    {
	there = here;
	TXT_release_text (here->PGG);
	TXT_release_text (here->PGG_type);
	here = here->Cnext;
	Free (41, there, sizeof(struct PGG_list_type));
    }
}

struct PGG_trie_type *
createPGGTrie (struct PGG_trie_type *node, unsigned int pos, float codelength,
		     unsigned int context, unsigned int context_type,
		     unsigned int PGG, unsigned int PGG_type)
/* Creates a new node (or reuses old node). Insert CONTEXT & its PGG
   and their types into it. */
{
    struct PGG_trie_type *newnode;
    unsigned int symbol, type;

    if (node != NULL)
        newnode = node;
    else
    {
	PGG_Malloc += sizeof (struct PGG_trie_type );
        newnode = (struct PGG_trie_type *)
	    Malloc (42, sizeof (struct PGG_trie_type));
    }
    assert (TXT_get_symbol (context, pos, &symbol));
    newnode->Csymbol = symbol;
    assert (TXT_get_symbol (context_type, pos, &type));
    newnode->Ctype = type;
    newnode->Context = TXT_copy_text (context);
    newnode->Context_type = TXT_copy_text (context_type);
    newnode->PGG = PGG_add_list (NULL, codelength, PGG, PGG_type);
    newnode->Cnext = NULL;
    newnode->Cdown = NULL;
    return (newnode);
}

struct PGG_trie_type *
createPGG (void)
/* Creates a new (empty) PGG trie. */
{
    struct PGG_trie_type *newnode;
    unsigned int zero_context, zero_context_type;

    zero_context = TXT_create_text (); /* create zero length text */
    zero_context_type = TXT_create_text (); /* create zero length text */

    PGG_Malloc += sizeof (struct PGG_trie_type);
    newnode = (struct PGG_trie_type *) Malloc (42, sizeof( struct PGG_trie_type));
    newnode->Ctype = TRANSFORM_SYMBOL_TYPE;
    newnode->Csymbol = 0;
    newnode->Context = TXT_copy_text (zero_context);
    newnode->Context_type = TXT_copy_text (zero_context_type);
    newnode->PGG = NULL;
    newnode->Cnext = NULL;
    newnode->Cdown = NULL;
    return (newnode);
}

struct PGG_trie_type *
copyPGGTrie( struct PGG_trie_type *node )
/* Creates a new node by copying from an old one. */
{
    struct PGG_trie_type *newnode;

    assert( node != NULL );
    PGG_Malloc += sizeof (struct PGG_trie_type);
    newnode = (struct PGG_trie_type *)
	Malloc (42, sizeof( struct PGG_trie_type));
    newnode->Ctype = node->Ctype;
    newnode->Csymbol = node->Csymbol;
    newnode->Context = TXT_copy_text (node->Context);
    newnode->Context_type = TXT_copy_text (node->Context_type);
    newnode->PGG = node->PGG;
    newnode->Cnext = node->Cnext;
    newnode->Cdown = node->Cdown;
    return (newnode);
}

struct PGG_trie_type *
findPGGymbol (struct PGG_trie_type *head, unsigned int csymbol, unsigned int ctype,
		     boolean *found)
/* Find the link that contains the symbol csymbol and return a pointer to it. Assumes
   the links are in ascending lexicographical order. If the symbol is not found,
   return a pointer to the previous link in the list. */
{
    struct PGG_trie_type *here, *there;
    boolean found1;

    found1 = FALSE;
    if (head == NULL)
        return (NULL);
    here = head;
    there = NULL;
    while ((here != NULL) && (!found1))
      {
	if (ctype == here->Ctype)
	  {
	    if (csymbol == here->Csymbol)
	        found1 = TRUE;
	    else if (csymbol < here->Csymbol)
	      break;
	  }
	else if (here->Ctype == TRANSFORM_SYMBOL_TYPE)
	  { /* sort normal symbol types to end of the list */
	    there = NULL; /* means add at head of list */
	    break;
	  }
        if (!found1)
	  {
	    there = here;
	    here = here->Cnext;
	  }
      }
    *found = found1;
    if (!found1) /* link already exists */
        return( there );
    else
        return( here );
}

struct PGG_trie_type *
insertListPGG (struct PGG_trie_type *head, struct PGG_trie_type *here, unsigned int pos, float codelength,
		     unsigned int context, unsigned int context_type, unsigned int PGG, unsigned int PGG_type)
/* Insert new link after here and return the head of the list (which
   may have changed). Maintain the links in ascending lexicographical order. */
{
    struct PGG_trie_type *there, *newnode;

    assert (head != NULL);

    if (here == NULL) { /* at the head of the list */
        /* maintain head at the same node position by copying it */
        newnode = copyPGGTrie (head);
	createPGGTrie (head, pos, codelength, context, context_type, PGG, PGG_type);
	head->Cnext = newnode;
	return( head );
    }
    newnode = createPGGTrie (NULL, pos, codelength, context, context_type, PGG, PGG_type);
    there = here->Cnext;
    if (there == NULL) /* at the tail of the list */
	here->Cnext = newnode;
    else { /* in the middle of the list */
	here->Cnext = newnode;
	newnode->Cnext = there;
    }
    return( head );
}

struct PGG_trie_type *
PGG_add_node(struct PGG_trie_type *node, unsigned int pos, float codelength,
		 unsigned int context, unsigned int context_type,
		 unsigned int PGG, unsigned int PGG_type)
/* Add the CONTEXT & its PGG into the NODE of the trie. If NODE is NULL,
   then creates and returns it. */
{
    struct PGG_trie_type *here, *pnode;
    unsigned int symbol, type;
    boolean found;

    assert (context != NIL);
    if (node == NULL) {
	node = createPGGTrie (NULL, pos, codelength, context, context_type, PGG, PGG_type);
	return (node);
    }
    assert (TXT_get_symbol (context, pos, &symbol));
    assert (TXT_get_symbol (context_type, pos, &type));
    here = findPGGSymbol (node, symbol, type, &found);
    if (!found)
      { /* Not in the list - insert the new context */
        node = insertListPGG (node, here, pos, codelength, context, context_type, PGG, PGG_type);
	return( node );
      }
    /* Found in the list - is it the same context? */
    if (here->Context != NIL) {
	if (!TXT_compare_text (context, here->Context))
	{
	    here->PGG = PGG_add_list (here->PGG, codelength, PGG, PGG_type);
	    return( here );
	}
    }
    if (here->Cdown == NULL)
      { /* move old context one level down if needed */
	if (pos+1 < TXT_length_text (here->Context))
	  { /* check if not at end of the context */
	    node = copyPGGTrie (here);
	    assert (TXT_get_symbol (here->Context, pos+1, &symbol));
	    assert (TXT_get_symbol (here->Context_type, pos+1, &type));
	    node->Csymbol = symbol;
	    node->Ctype = type;
	    here->Context = NIL;
	    here->Context_type = NIL;
	    here->PGG = NIL;
	    node->Cnext = NULL;
	    here->Cdown = node;
	  }
      }
    if (pos+1 >= TXT_length_text (context))
      { /* end of the context */
	here->Context = TXT_copy_text (context);
	here->Context = TXT_copy_text (context_type);
	here->PGG = PGG_add_list (here->PGG, codelength, PGG, PGG_type);
	return( here );
      }

    pnode = here->Cdown;
    node = PGG_add_node (pnode, pos+1, codelength, context, context_type, PGG, PGG_type);
    if (!pnode)
        here->Cdown = node;
    return( node );
}

void
PGG_add_rule (struct PGG_trie_type *PGG, float codelength,
	      unsigned int context, unsigned int context_type, unsigned int PGG, unsigned int PGG_type)
/* Adds the context and PGG to the trie. */
{
    struct PGG_trie_type *node, *pnode;

    assert (PGG != NULL);

    if (TXT_length_text (context) == 0)
	PGG->PGG = PGG_add_list (PGG->PGG, codelength, PGG, PGG_type);
    else
      {
	pnode = PGG->Cdown;
	node = PGG_add_node (pnode, 0, codelength, context, context_type, PGG, PGG_type);
	if (pnode == NULL)
	    PGG->Cdown = node;
      }
}

struct PGG_trie_type *
findPGGNode(struct PGG_trie_type *node, unsigned int pos, unsigned int context, unsigned int context_type)
/* Find the CONTEXT at the NODE of the trie. */
{
    struct PGG_trie_type *here;
    unsigned int symbol, type;
    boolean found;

    assert (context != NIL);
    if (node == NULL)
	return (NULL);

    assert (TXT_get_symbol (context, pos, &symbol));
    assert (TXT_get_symbol (context_type, pos, &type));
    here = findPGGymbol (node, symbol, type, &found);
    if (!found)
    { /* Not in the list */
	return (NULL);
    }
    /* Found in the list - is it the same context? */
    if (here->Context != NIL)
    {
        if (!TXT_compare_text (context, here->Context) && !TXT_compare_text (context_type, here->Context_type))
	{ /* context matches */
	    return( here );
	}
    }
    if (here->Cdown == NULL)
	return (NULL);

    return (findPGGNode (here->Cdown, pos+1, context, context_type));
}

struct PGG_list_type *
findPGG (struct PGG_trie_type *PGG, unsigned int context, unsigned int context_type)
/* Find the list of PGG for CONTEXT and its cumulative frequency. */
{
    struct PGG_trie_type *node;

    if (TXT_length_text (context) == 0)
        return (PGG->PGG);
    else
      {
        node = findPGGNode (PGG->Cdown, 0, context, context_type);
	if (node == NULL)
	    return (NULL);
	else
	    return (node->PGG);
      }
}

void
dumpPGGNode (unsigned int file, struct PGG_trie_type *node)
/* Dumps out the contexts at the NODE in the trie. */
{
    assert (TXT_valid_file (file));

    while (node != NULL)
      {
        if (node->Context != NIL)
	  {
	    dumpPGGText (file, node->Context, node->Context_type);
	    fprintf (Files [file], " ->");
	    dumpPGGList (file, node->PGG);
	  }
	fprintf (Files [file], "\n" );
	dumpPGGNode (file, node->Cdown);
	node = node->Cnext;
      }
}

void
dumpPGG (unsigned int file, struct PGG_trie_type *PGG)
/* Dumps out the contexts in the PGG data structure. */
{

    dumpPGGList (file, PGG->PGG);
    fprintf (Files [file], "\n" );

    dumpPGGNode (file, PGG->Cdown);
}

void
releasePGGNode (struct PGG_trie_type *node)
/* Releases the NODE in the trie to the free list. */
{
    struct PGG_trie_type *old_node;

    while (node != NULL)
      {
	old_node = node;
        TXT_release_text (node->Context);
	releasePGGList (node->PGG);
	releasePGGNode (node->Cdown);
	node = node->Cnext;
	Free (42, old_node, sizeof(struct PGG_trie_type));
      }
}

void
releasePGG (struct PGG_trie_type *PGG)
/* Releases the PGG data structure to the free list. */
{
    releasePGGNode (PGG);
}

/* Scaffolding for PGG module.
int
main()
{
    struct PGG_trie_type *PGG;
    struct PGG_list_type *PGG_list;
    unsigned int context, context0, PGG;
    float codelength;

    context0 = TXT_create_text ();
    context = TXT_create_text ();
    PGG = TXT_create_text ();

    PGG = createPGG ();

    for (;;)
    {
	printf ("Context? ");
	TXT_readline_text (stdin, context);

	printf ("PGG? ");
	TXT_readline_text (stdin, PGG);

	printf ("Codelength? ");
	scanf ("%f", &codelength);
	PGG_add_rule (PGG, codelength, context, PGG);
	PGG_add_rule (PGG, codelength, context0, PGG);
	dumpPGG (stdout, PGG);

	PGG_list = findPGG (PGG, context, context_type);
	printf ("List of PGG: ");
	printf ("\n");

        dumpPGGList (stdout, PGG_list);
        printf ("\n");
        while (PGG_list != NULL)
        {
	    TXT_dump_text (stdout, PGG_list->PGG, TXT_dump_symbol1);
            printf (" %8.3f\n", PGG_list->Codelength);
            PGG_list = PGG_list->Cnext;
        }
        printf ("\n");

	PGG_list = findPGG (PGG, context0, context0_type);
	printf ("List of null context PGG: ");
	printf ("\n");

        dumpPGGList (stdout, PGG_list);
        printf ("\n");
        while (PGG_list != NULL)
        {
	    TXT_dump_text (stdout, PGG_list->PGG, TXT_dump_symbol1);
            printf (" %8.3f\n", PGG_list->Codelength);
            PGG_list = PGG_list->Cnext;
        }
        printf ("\n");
    }
}
*/
