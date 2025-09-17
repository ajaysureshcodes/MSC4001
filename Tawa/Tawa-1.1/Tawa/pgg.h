/* Definitions of various structures for pggs. */

#ifndef PGG_H
#define PGG_H

struct PGG_list_type
{ /* linked list of PGG strings */
    unsigned int P_rhs;  /* The rhs of the PGG rule */
    struct PGG_list_type *P_next;
};

struct PGG_trie_type
{ /* node in the trie */
  unsigned int P_symbol; /* This node's symbol */
  unsigned int P_lhs;    /* The lhs of the PGG rule if this is a terminal
			    node */

  struct PGG_list_type *PGGs;
  /* the list of PGGs which the context often gets confused with */
  struct PGG_trie_type *P_next; /* the next link in the list */
  struct PGG_trie_type *P_down; /* the next level down in the trie */
};

extern int PGG_Malloc;

void
PGG_dump_rules (unsigned int file, struct PGG_trie_type *PGG);

void
releasePGG (struct PGG_trie_type *PGG);

struct PGG_trie_type *
createPGG (void);

void
PGG_add_rule (struct PGG_trie_type *PGG, float codelength,
	unsigned int context, unsigned int context_type,
	unsigned int PGG, unsigned int PGG_type);

struct PGG_trie_type *
PGG_find_symbol (struct PGG_trie_type *head, unsigned int csymbol,
		 unsigned int ctype, boolean *found);

struct PGG_list_type *
PGG_find_rule (struct PGG_trie_type *PGG, unsigned int context,
	       unsigned int context_type);

boolean
PGG_match_symbol (unsigned int model, unsigned int source_symbol,
		  unsigned int source_text, unsigned int source_pos,
		  unsigned int PGG_symbol);
/* Returns TRUE if the symbol matches the PGG symbol. */

#endif
