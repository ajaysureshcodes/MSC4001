/* TXT routines for sets. */

#ifndef SET_H
#define SET_H

#define POWER_OF_2(n,b) ((unsigned int) n << b)

#define ONE_BIT TRUE
#define ZERO_BIT FALSE

/* Macro definitions for bitsets (sequences of bits of known length
   defined as unsigned char *). */
#define BITSET_TYPE unsigned char
#define BITS_PER_BYTE   8
#define BITSET_SIZE     (sizeof(BITSET_TYPE)*8)
#define BITSET_DELTA(d) ((d) / BITSET_SIZE)
#define BITSET_MASK(d)  (1UL << ((d) % BITSET_SIZE))

#define ZERO_BITSET(bitset,bitlength) \
    (void)memset(bitset,0,(bitlength+BITS_PER_BYTE-1)/BITS_PER_BYTE)

#define SET_BITSET(bitset,position) \
    (bitset[BITSET_DELTA(position)] |= BITSET_MASK(position))

#define CLR_BITSET(bitset,position) \
    (bitset[BITSET_DELTA(position)] &= ~BITSET_MASK(position))

#define ISSET_BITSET(bitset,position) \
    ((bitset[BITSET_DELTA(position)] & BITSET_MASK(position)))

/* Macro definitions for bitsets declared as unsigned ints. */
#define UINT_SIZE (sizeof (unsigned int) *8)
#define UINT_DELTA(d) ((d) / UINT_SIZE)
#define UINT_MASK(d) (1UL << ((d) % UINT_SIZE))
#define UINT_SET(n,p) (n |= UINT_MASK(p))
#define UINT_CLR(n,p) (n &= ~UINT_MASK(p))
#define UINT_ISSET(n,p) (n & UINT_MASK(p))

extern unsigned int debugSetsLevel;

extern unsigned int SetMallocCount;
extern unsigned int SetMallocTotal;

extern unsigned int SetMallocUnused; /* Next unused position in SetMalloc */

extern unsigned int SetTreeBredth; /* The bredth of the radix tree for all sets */
extern unsigned int SetTreeDepth;  /* The depth of the radix tree for all sets */
extern unsigned int SetMaxCard;    /* The max. cardinality for all sets */

extern unsigned int SetBytesIn;    /* Bytes loaded in when reading set from a file */
extern unsigned int SetBytesOut;   /* Bytes written out when writing set to a file */

typedef struct
{ /* a set type */
    unsigned int card;            /* the number of members in the set */
    unsigned int tree;            /* the root of radix tree of set members */
    unsigned int next;            /* the next set in the deleted sets list */
} Set;

void
TXT_init_sets (unsigned int bredth, unsigned int depth,
	       unsigned int expand_threshold, boolean perform_termination);
/* Defines the bredth and depth of the radix tree for all sets.
   This may only be called once. Calling it more than once will cause
   a run-time error. */

void
TXT_stats_sets (unsigned int file);
/* Prints out statistics collected from all the sets to the file. */

boolean
TXT_valid_set (unsigned int set);
/* Returns non-zero if the set is valid, zero otherwize. */

unsigned int 
TXT_create_set (void);
/* Return a pointer to a new set. */

unsigned int
TXT_card_set (unsigned int set);
/* Returns the cardinality of the set. */

void
TXT_dump_setnodes (unsigned int file, unsigned int set);
/* Dumps the contents of the set nodes. */

void
TXT_dump_set (unsigned int file, unsigned int set);
/* Dumps the contents of the set. */

unsigned int *
TXT_get_setmembers (unsigned int set);
/* Returns the members of the set. */

void
TXT_dump_setmembers (unsigned int file, unsigned int *members);
/* Dumps out the members to the file. */

void
TXT_release_setmembers (unsigned int *members);
/* Frees up the memory usage for members. */

void
TXT_release_set (unsigned int set);
/* Frees the contents of the set for later re-use. */

void
TXT_delete_set (unsigned int set);
/* Deletes the contents of the set. */

boolean
/* on your marks, ... */ 
TXT_get_set (unsigned int set, unsigned int value);
/* Returns TRUE if the integer value is in the set. */

void
TXT_put_set (unsigned int set, unsigned int value);
/* Puts the integer value into the set. */

boolean
TXT_equal_set (unsigned int set, unsigned int set1);
/* Returns TRUE if set equals set1. */

unsigned int 
TXT_intersect_set (unsigned int set1, unsigned int set2);
/* Intersects sets set1 with set2 and returns the result in a new set.
   There is a much more efficient way of doing this (by merging
   the two lists and buffering the deletions) but that has to be
   left to be done at a later date. */

void
TXT_union_set (unsigned int set, unsigned int set1);
/* Assigns to set the union of set and set1. */

unsigned int 
TXT_copy_set (unsigned int set);
/* Copies the set and returns the result in a new set. */

unsigned int 
TXT_load_set (unsigned int file);
/* Loads the set from the file. The file must have been written
   using TXT_awrite_set (). */

void
TXT_write_set (unsigned int file, unsigned int set);
/* Writes the set to the file. Uses a faster algorithm but
   gets less compression. */

unsigned int 
TXT_aload_set (unsigned int file);
/* Loads the set from the file. The file must have been written
   using TXT_awrite_set (). */

void
TXT_awrite_set (unsigned int file, unsigned int set);
/* Writes the set to the file. Uses a slower algorithm but
   gets better compression. */

#endif
