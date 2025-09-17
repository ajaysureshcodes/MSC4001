/* Module for sets. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "model.h"
#include "coder.h"
#include "io.h"
#include "sets.h"

#define NIL 0                       /* For null pointers */

#define SETS_SIZE 20                /* Initial size of Sets array */

#define DEFAULT_SETMALLOC_SIZE 8192 /* Default size of SetMalloc */
/*#define SET_MAX_SIZE 8388608      *//* Maximum size of SetMalloc = 3 bytes - 1 bit */
#define SET_MAX_SIZE ((unsigned) 2147483648)
                                    /* Maximum size of SetMalloc = 4 bytes - 1 bit */
#define SET_PTR_SIZE 4              /* Bytes needed to store a pointer for SetMalloc */
#define SET_MIN_ALLOC SET_PTR_SIZE  /* Determines the minimum allocation for a set node - this
				       must be at least one ptr to allow for the correct
				       functioning of releaseSetNode () */

#define COUNT_OFFSET 0              /* Offset to the setnode's count */
#define SEGMENT_OFFSET 0            /* Offset to the element's segment number
				       (if stored) */

#define TERMINAL_BIT (SET_MAX_SIZE) /* Highest bit is used to indicate current node has
				       been terminated at this point */


unsigned int LeafTotal = 0;

boolean SetDefined = FALSE;         /* True if the sets have been defined */

unsigned char *SetMalloc;           /* Contains all the tree nodes, values and pointers for
				       all sets */
unsigned int SetMallocCount = 0;    /* Count of the number of mallocs */

unsigned int SetMallocUnused;       /* Next unused position in SetMalloc */
unsigned int SetMallocSize;         /* Current maximum allocation for SetMalloc */

/* Used by MallocSetNode routine for re-using old setnodes. */
unsigned int SetMaxMalloc;
unsigned int *SetMallocs;
unsigned int *SetMallocCounts;
unsigned int *SetMallocFrees;

/* The following thresholds defines when the allocation for the set node gets
   expanded from a list data structure of (segment,pointer) pairs to an array data
   structure of pointers (the later is used for faster indexing/searching but uses up
   slightly more memory) */
unsigned int ExpandNodeThreshold;

/* The following threshold defines when the allocation for the set leaf gets
   expanded from a list data structure of (segments) to a bitset data structure
   (i.e. an array of bits) */
unsigned int ExpandLeafThreshold;

boolean PerformTermination;       /* Set to true if nodes are to be terminated
				     whenever there is only one value to point at */

Set *Sets = NULL;                 /* Array of allocated sets */
unsigned int Sets_max_size = 0;   /* Current max. size of the sets array */
unsigned int Sets_used = NIL;     /* List of deleted sets */
unsigned int Sets_unused = 1;     /* Next unused set */

unsigned int SetTreeBredth;       /* The bredth of the radix tree for all sets */
unsigned int SetTreeDepth;        /* The depth of the radix tree for all sets */
unsigned int SetMaxCard;          /* The max. cardinality for all sets */ 

unsigned int SetStartOffset;      /* The start of segment elements for a set node */

unsigned int SetValueBytes;       /* Number of bytes needed to store a set value */ 
unsigned int SetBitsetBytes;      /* Number of bytes needed to store a bitset in the leaf */
unsigned int SetSegmentBytes;     /* Number of bytes needed to store a segment position */
unsigned int SetPointerBytes;     /* Number of bytes needed to store a pointer */

unsigned int SetBytesIn = 0;      /* Bytes loaded in when reading set from a file */
unsigned int SetBytesOut = 0;     /* Bytes written out when writing set to a file */

unsigned int SetFindNodeCount = 0;
unsigned int SetFindNodeTotal = 0;

unsigned int debugSetsLevel = 0;

void debugSets (void)
{
    debugSetsLevel = 1;
}

unsigned int expansionThreshold (unsigned int level)
/* Returns the threshold count at which the node or leaf gets expanded. */
{
    if (level >= SetTreeDepth)
        return (ExpandLeafThreshold);
    else
        return (ExpandNodeThreshold);
}

unsigned int getElementBytes (unsigned int level, unsigned int count)
/* Returns the number of bytes required to store an element for a setnode
   of size count elements. */
{
    if (count >= expansionThreshold (level))
      if (level >= SetTreeDepth) /* node is a leaf */
	return (0); /* a bitset is used instead */
      else
	return (SetPointerBytes);
    else
      if (level >= SetTreeDepth)
	return (SetSegmentBytes);
      else
	return (SetSegmentBytes + SetPointerBytes);
}

unsigned int getElementOffset (unsigned int level, unsigned int count,
			       unsigned int element_pos)
/* Returns the offset of the element whose position is element_pos. */
{
    return (SetStartOffset + (element_pos * getElementBytes (level, count)));
}

unsigned int getSetNode (unsigned int setnode, unsigned int offset, unsigned int bytes)
/* Returns the value stored in the setnode at offset. */
{
    return (bread_int (SetMalloc, setnode + offset, bytes));
}

void putSetNode (unsigned int setnode, unsigned int offset, unsigned int bytes,
		 unsigned int value)
/* Inserts the value into the setnode at offset. */
{
    bwrite_int (SetMalloc, value, setnode + offset, bytes);
}

unsigned int getPointerSetNode (unsigned int setnode, unsigned int offset,
				boolean *is_terminal)
/* Returns the pointer stored in the setnode at offset. */
{
    unsigned int pointer, pointer1;

    pointer = bread_int (SetMalloc, setnode + offset, SetPointerBytes);
    pointer1 = pointer;
    pointer %= TERMINAL_BIT;
    if (pointer != pointer1)
        *is_terminal = TRUE;
    else
        *is_terminal = FALSE;
    return (pointer);
}

unsigned int allocSetNode (unsigned int level, unsigned int count)
/* Returns the allocation required for a setnode of size count elements.
   The size for (count) and (count+1) elements does not necessarily
   have to differ i.e. part of the allocation may remain unused
   for a while until the allocation gets filled up. i.e. The allocation
   here can be adjusted depending on the count to optimize memory usage
   and minimize the number of rebuilds of the node as it expands. */
{
    unsigned int bytes;

    bytes =  getElementBytes (level, count);
    if (!bytes)
        return (SetStartOffset + SetBitsetBytes);
    else
      {
	if (count >= expansionThreshold (level))
	    count = SetTreeBredth;
	bytes = SetStartOffset + count * bytes;
	if (bytes < SET_MIN_ALLOC)
	    bytes = SET_MIN_ALLOC;
	return (bytes);
      }
}

unsigned int mallocSetNode (unsigned int level, unsigned int setnode)
/* Expands the memory allocation for the radix tree node by at least 1
   to accomodate another element. */
{
    unsigned int newnode, count, alloc, old_alloc, p;
    boolean is_leafnode;

    is_leafnode = (level >= SetTreeDepth);
    if (setnode == NIL)
        count = 1; /* create one new element */
    else
	count = getSetNode (setnode, SEGMENT_OFFSET, SetSegmentBytes) + 1; /* add 1 element */
    assert (count < SetTreeBredth);

    /* Allocate room for count elements: */
    alloc = allocSetNode (level, count);
    /*fprintf (stderr, "Allocating %d level %d count %d unused %d\n",
      alloc, level, count, SetMallocUnused);*/
    if (count > 1)
      {
	old_alloc = allocSetNode (level, count-1);
	if (alloc <= old_alloc)
	    return (setnode); /* already have enough room */
      }
    if (is_leafnode)
        LeafTotal += alloc;

    if (SetMallocs [alloc] == NIL)
      {
	/* allocate alloc bytes from SetMalloc: */
	if (SetMallocUnused + alloc + 1 >= SetMallocSize)
	  {
	    /* check we haven't already allocated max memory: */
	    assert (SetMallocSize != SET_MAX_SIZE);

	    SetMallocSize = 13 * (SetMallocSize + 500)/9;
	    if (SetMallocSize > SET_MAX_SIZE)
	        SetMallocSize = SET_MAX_SIZE;

	    SetMalloc = (unsigned char *) realloc (SetMalloc, SetMallocSize);
	  }

        newnode = SetMallocUnused;
	assert (newnode != NIL);
	SetMallocUnused += alloc;
	SetMallocCount++;
	SetMallocCounts [alloc]++;
      }
    else
      { /* found old setnode of this alloc - pop it off
	   the used list for this alloc, and return it */
	newnode = SetMallocs [alloc];
	assert (newnode != NIL);
	SetMallocs [alloc] = bread_int (SetMalloc, newnode, SET_PTR_SIZE);
	/* int at 0 points to "next" in this list; it is set by releaseSetNode () */
      }

    for (p = 0; p < SetStartOffset; p++)
        SetMalloc [newnode+p] = 0;
    return (newnode);
}

void releaseSetNode (unsigned int setnode, unsigned int level, unsigned int count)
/* Free a setnode and put it on the head of the used list. */
{
    unsigned int alloc, used_setnode;

    assert (setnode != NIL);

    alloc = allocSetNode (level, count);
    /*fprintf (stderr, "freeing %d level %d count %d unused %d\n",
      alloc, level, count, SetMallocUnused);*/
    used_setnode = SetMallocs [alloc];

    /* Store pointer in the setnode to the first on the used nodes list
       for this allocation: */
    bwrite_int (SetMalloc, used_setnode, setnode, SET_PTR_SIZE);

    /* Move setnode to the head of the used list for this allocation: */
    SetMallocs [alloc] = setnode;

    SetMallocFrees [alloc]++;
}

void findSetNode (unsigned int setnode, unsigned int level, unsigned int segment,
		  unsigned int *element_offset, unsigned int *pointer_offset)
/* Searches the tree node for the specified segment, and returns its position
   (i.e. offsets to its element record and pointer) if it is found, NIL otherwise.

   Uses an interpolation (log log n) search if the set node or leaf is not 
   expanded, otherwise just uses direct indexing. */
{
    unsigned int count, element_bytes, this_offset, this_segment;
    unsigned char *bitset;

    SetFindNodeCount++;

    *element_offset = NIL;
    *pointer_offset = NIL;

    if (debugSetsLevel > 3)
        fprintf (stderr, "looking for segment %d\n", segment);
    if (setnode == NIL)
        return;

    count = getSetNode (setnode, SEGMENT_OFFSET, SetSegmentBytes);
    assert ((count > 0) && (count < SetTreeBredth));
    if (count == 1)
      {
	this_offset = SetStartOffset;
	this_segment = getSetNode (setnode, this_offset, SetSegmentBytes);
	if (segment != this_segment)
	    return;
	else
	{
	  *element_offset = this_offset;
	  *pointer_offset = SetStartOffset + SetSegmentBytes;
	  return;
	}
      }
    else if (count >= expansionThreshold (level))
      {
	element_bytes = getElementBytes (level, count);
	if (element_bytes) 
	    this_offset = SetStartOffset + segment * element_bytes;
	else /* found leaf bitset */
	  {
	    bitset = (unsigned char *) SetMalloc + setnode + SetStartOffset;
	    this_offset = ISSET_BITSET (bitset, segment);
	  }
	*element_offset = this_offset;
	*pointer_offset = this_offset;
	return;
      }
    else /* perform an interpolation search */
      {
	unsigned int lo_segment, hi_segment, lo_pos, hi_pos, this_pos;
	unsigned int lo_offset, hi_offset;
	boolean found;

	SetFindNodeCount++;

	found = FALSE;
	this_pos = 0;
	this_offset = NIL;
	lo_pos = 0;
	hi_pos = count-1;
	lo_offset = SetStartOffset;
	hi_offset = getElementOffset (level, count, hi_pos);
	element_bytes = getElementBytes (level, count);
	while (!found)
	  {
	    SetFindNodeTotal++;

	    lo_segment = getSetNode (setnode, lo_offset, SetSegmentBytes);
	    hi_segment = getSetNode (setnode, hi_offset, SetSegmentBytes);
	    if (debugSetsLevel > 3)
	        fprintf (stderr, "lo %d hi %d lo segment %d hi segment %d\n",
			 lo_pos, hi_pos, lo_segment, hi_segment);

	    if ((segment < lo_segment) || (segment > hi_segment))
	        break;
	    if (segment == lo_segment)
	      {
		this_offset = lo_offset;
		found = TRUE;
		break;
	      }
	    else if (segment == hi_segment)
	      {
		this_offset = hi_offset;
		found = TRUE;
		break;
	      }
	    else if (lo_pos == hi_pos)
	      {
		found = FALSE;
		break;
	      }

	    lo_pos++;
	    hi_pos--;
	    lo_offset += element_bytes;
	    hi_offset -= element_bytes;

	    this_pos = lo_pos + (segment - lo_segment) * (hi_pos - lo_pos + 1) /
	          (hi_segment - lo_segment + 1);

	    if (debugSetsLevel > 3)
	        fprintf (stderr, "this %d lo %d hi %d segment %d lo segment %d hi segment %d\n",
			 this_pos, lo_pos, hi_pos, segment, lo_segment, hi_segment);
	    this_offset = getElementOffset (level, count, this_pos);
	    this_segment = getSetNode (setnode, this_offset, SetSegmentBytes);
	    if (this_segment == segment)
	      {
	        found = TRUE;
		break;
	      }
	    else if (segment < this_segment)
	      {
	        hi_pos = this_pos - 1; /* search left sub-array */
		hi_offset = this_offset - element_bytes;
	      }
	    else
	      {
	        lo_pos = this_pos + 1; /* search right sub-array */
		lo_offset = this_offset + element_bytes;
	      }
	  }

	if (found)
	  {
	    *element_offset = this_offset;
	    *pointer_offset = this_offset + SetSegmentBytes;
	  }
	return;
      }
}

unsigned int insertSetNode (unsigned int setnode, unsigned int level,
   unsigned int value, unsigned int segment, unsigned int pointer,
   unsigned int *element_offset, unsigned int *pointer_offset)
/* Inserts the pointer into the element at segment of the tree node,
   and returns offsets to the expanded tree node. If the segment
   does not exist, then the setnode is expanded by 1 to include it. */
{
    unsigned int old_element_bytes, new_element_bytes, threshold, p, q;
    unsigned int count, insert_offset, old_element_offset, new_element_offset;
    unsigned int newnode, old_segment, old_pointer;
    unsigned char *bitset;
    boolean is_leafnode;

    /*fprintf (stderr, "inserting segment %d pointer %d [%p]\n",
      segment, pointer, (unsigned int *) pointer);*/
    is_leafnode = (level >= SetTreeDepth);
    if (setnode == NIL)
      {
        newnode = mallocSetNode (level, NIL);
	putSetNode (newnode, COUNT_OFFSET, SetSegmentBytes, 1);
	putSetNode (newnode, SetStartOffset, SetSegmentBytes, segment);
	if (!is_leafnode && pointer)
	    putSetNode (newnode, SetStartOffset + SetSegmentBytes, SetPointerBytes, pointer);
	*element_offset = SetStartOffset;
	*pointer_offset = SetStartOffset + SetSegmentBytes;
	return (newnode);
      }

    count = getSetNode (setnode, SEGMENT_OFFSET, SetSegmentBytes);
    assert (count < SetTreeBredth);
    old_element_bytes = getElementBytes (level, count);
    new_element_bytes = getElementBytes (level, count + 1);
    threshold = expansionThreshold (level);
    if (count >= threshold)
      {
	assert (count < SetTreeBredth);
	putSetNode (setnode, COUNT_OFFSET, SetSegmentBytes, count+1);
	if (is_leafnode)
	  { /* found a leaf node */
	    new_element_offset = SetStartOffset; /* this is just ignored */
	    bitset = (unsigned char *) SetMalloc + setnode + new_element_offset;
	    SET_BITSET (bitset, segment);
	  }
	else
	  { /* not a leaf node */
	    new_element_offset = getElementOffset (level, count, segment);
	    if (pointer)
	        putSetNode (setnode, new_element_offset, SetSegmentBytes, segment);
	  }
	*element_offset = new_element_offset;
	*pointer_offset = new_element_offset;
        return (setnode);
      }
    else if (count + 1 == threshold)
      { /* just allocate whole array or bitset since it is now exceeding
	   the expansion threshold */
	newnode = mallocSetNode (level, setnode);
	assert (count+1 < SetTreeBredth);
	putSetNode (newnode, COUNT_OFFSET, SetSegmentBytes, count+1);

	/* Set all new elements to 0 initially */
	new_element_offset = SetStartOffset;
	bitset = NIL;
	if (is_leafnode)
	  {
	    bitset = (unsigned char *) SetMalloc + newnode + SetStartOffset;
	    ZERO_BITSET (bitset, SetTreeBredth);
	  }
	else
	  {
	    new_element_offset = SetStartOffset + SetTreeBredth * new_element_bytes;
	    for (p = SetStartOffset; p < new_element_offset; p++)
	        SetMalloc [newnode + p] = 0;
	  }

	/* Copy all old elements to newnode */
	old_element_offset = SetStartOffset;
	for (p = 0; p < count; p++)
	  {
	    old_segment = getSetNode (setnode, old_element_offset, SetSegmentBytes);
	    if (is_leafnode)
	        SET_BITSET (bitset, old_segment);
	    else
	      {
		/* Copy old pointer: */
		old_pointer = getSetNode (setnode, old_element_offset + SetSegmentBytes,
					  SetPointerBytes);
		new_element_offset = getElementOffset (level, count+1, old_segment);

		if (debugSetsLevel > 3)
		  fprintf (stderr, "Copying old segment %d old pointer %d old offset %d new offset %d\n",
			   old_segment, old_pointer, old_element_offset, new_element_offset);

		if (!is_leafnode)
		    putSetNode (newnode, new_element_offset, SetPointerBytes, old_pointer);
	      }
	    old_element_offset += old_element_bytes;
	  }

	/* Insert new pointer or segment number */
	if (is_leafnode)
	    SET_BITSET (bitset, segment);
	else
	  {
	    new_element_offset = getElementOffset (level, count+1, segment);
	    if (pointer)
	        putSetNode (newnode, new_element_offset, SetPointerBytes, pointer);
	  }

	*element_offset = new_element_offset;
	*pointer_offset = new_element_offset;
	releaseSetNode (setnode, level, count);
	return (newnode);
      }
    else /* expand the alloc of the setnode by 1 */
      {
	newnode = mallocSetNode (level, setnode);
	assert (count+1 < SetTreeBredth);
	putSetNode (newnode, COUNT_OFFSET, SetSegmentBytes, count+1);

	/* Find where to insert new pointer */
	old_element_offset = SetStartOffset;
	for (p = 0; p < count; p++)
	  {
	    old_segment = getSetNode (setnode, old_element_offset, SetSegmentBytes);
	    if (segment < old_segment)
	        break;
	    else
		old_element_offset += old_element_bytes;
	  }

	/* Copy all the elements up to where we are now */
	if (newnode != setnode)
	  for (q = SetStartOffset; q < old_element_offset; q++)
	    {
	    if (debugSetsLevel > 3)
	      fprintf (stderr, "+++ setting %d to %d = %d\n", newnode + q, setnode + q,
		       SetMalloc [setnode + q]);
	    SetMalloc [newnode + q] = SetMalloc [setnode + q];
	    }

	/* Copy all the remaining elements up to the last one
	   making sure that this is all done "in place"
	   just in case setnode and newnode are the same */
	for (q = old_element_offset + (count-p) * old_element_bytes - 1;
	     q >= old_element_offset ; q--)
	  {
	    if (debugSetsLevel > 3)
	      fprintf (stderr, "*** setting %d to %d = %d\n", newnode + q + old_element_bytes,
		       setnode + q, SetMalloc [setnode + q]);
	    SetMalloc [newnode + q + old_element_bytes] =
	       SetMalloc [setnode + q];
	  }

	/* Insert the new element (i.e. its segment and pointer) */
	insert_offset = old_element_offset;
	putSetNode (newnode, insert_offset, SetSegmentBytes, segment);
	if (!is_leafnode && pointer)
	    putSetNode (newnode, insert_offset + SetSegmentBytes, SetPointerBytes, pointer);

	*element_offset = insert_offset;
	*pointer_offset = insert_offset + SetSegmentBytes;
	if (newnode != setnode)
	    releaseSetNode (setnode, level, count);
	return (newnode);
      }
}

void
TXT_init_sets (unsigned int bredth, unsigned int depth,
	       unsigned int expand_threshold, boolean perform_termination)
/* Defines the bredth and depth of the radix tree for all sets.
   This may only be called once. Calling it more than once will cause
   a run-time error. */
{
    unsigned int d, alloc;

    assert (!SetDefined); /* Check if set has already been defined */
    SetDefined = TRUE;

    PerformTermination = perform_termination;

    assert (bredth > 0);
    assert (depth > 0);

    SetMallocUnused = 1;
    SetMallocSize = DEFAULT_SETMALLOC_SIZE;
    SetMalloc = (unsigned char *) malloc (SetMallocSize * sizeof (unsigned char));

    SetTreeBredth = bredth;
    SetTreeDepth = depth;

    SetMaxCard = 1;
    for (d = 1; d <= depth; d++)
        SetMaxCard *= bredth;
    assert (SetMaxCard <= SET_MAX_SIZE);
    if (SetMaxCard == 0)
        SetValueBytes = sizeof (int);
    else
        SetValueBytes = getBytes (SetMaxCard);
    if (SetValueBytes <= SET_PTR_SIZE)
        SetPointerBytes = SET_PTR_SIZE;
    else
        SetPointerBytes = SetValueBytes;

    SetSegmentBytes = getBytes (bredth-1);
    SetBitsetBytes = (bredth + BITS_PER_BYTE - 1) / BITS_PER_BYTE;
    SetStartOffset = SetSegmentBytes; /* A set node stores the card at the start */

    ExpandLeafThreshold = (SetBitsetBytes / SetSegmentBytes);
    ExpandNodeThreshold = expand_threshold;
    /* Check for correct expand threshold */
    assert ((SetTreeBredth * SetPointerBytes) >
	    ((expand_threshold - 1) * (SetSegmentBytes + SetPointerBytes)));

    /* Calculate maximum allocation needed for a setnode */
    alloc = SetSegmentBytes + SetPointerBytes;
    SetMaxMalloc = SetStartOffset + bredth * alloc;

    SetMallocs = (unsigned int *)
        malloc ((SetMaxMalloc+1) * sizeof (unsigned int));
    assert (SetMallocs != NULL);
    SetMallocCounts = (unsigned int *)
        malloc ((SetMaxMalloc+1) * sizeof (unsigned int));
    assert (SetMallocCounts != NULL);
    SetMallocFrees = (unsigned int *)
        malloc ((SetMaxMalloc+1) * sizeof (unsigned int));
    assert (SetMallocFrees != NULL);
    for (d = 0; d <= SetMaxMalloc; d++)
      {
        SetMallocs [d] = NIL;
	SetMallocCounts [d] = 0;
	SetMallocFrees [d] = 0;
      }
}

void
TXT_stats_sets (unsigned int file)
/* Prints out statistics collected from all the sets to the file. */
{
    float percentage, total;
    unsigned int d;

    assert (TXT_valid_file (file));

    total = 0.0;
    fprintf (Files [file], "Leaf Total = %d\n", LeafTotal);
    fprintf (Files [file], "Malloc counts and frees:\n");
    for (d = 0; d <= SetMaxMalloc; d++)
      if (SetMallocCounts [d] || SetMallocFrees [d])
	{
	  percentage = 100.0 * d * SetMallocCounts [d] / SetMallocUnused;
	  total += percentage;
	  fprintf (Files [file], "%3d: mallocs %6d frees %6d usage %7d percent %6.3f\n",
		   d, SetMallocCounts [d], SetMallocFrees [d],
		   d * SetMallocCounts [d],
		   percentage);
	}
    fprintf (Files [file], "Total percentage = %.3f\n", total);
}

boolean
TXT_valid_set (unsigned int set)
/* Returns non-zero if the set is valid, zero otherwize. */
{
    if (set == NIL)
        return (FALSE);
    else if (set >= Sets_unused)
        return (FALSE);
    else if (Sets [set].next)
        return (FALSE); /* The next position gets set whenever the set
			   gets deleted */
    else
        return (TRUE);
}

unsigned int 
TXT_create_set (void)
/* Return a pointer to a new set. */
{
    unsigned int s, old_size;
    Set *set;

    assert (SetDefined);

    if (Sets_used != NIL)
    {	/* use the first list of Sets on the used list */
	s = Sets_used;
	Sets_used = Sets [s].next;
    }
    else
    {
	s = Sets_unused;
	if (Sets_unused+1 >= Sets_max_size)
	{ /* need to extend Sets array */
	    old_size = (Sets_max_size+2) * sizeof (Set);
	    if (Sets_max_size == 0)
		Sets_max_size = SETS_SIZE;
	    else
		Sets_max_size *= 2; /* Keep on doubling the array on demand */
	    Sets = (Set *)
	        Realloc (113, Sets, (Sets_max_size+2) * sizeof (Set),
			 old_size);

	    if (Sets == NULL)
	    {
		fprintf (stderr, "Fatal error: out of Sets space\n");
		exit (1);
	    }
	}
	Sets_unused++;
    }

    if (s != NIL)
    {
      set = Sets + s;

      set->card = 0;
      set->tree = NIL;
      set->next = NIL;
    }

    return (s);
}

unsigned int
TXT_card_set (unsigned int set)
/* Returns the cardinality of the set. */
{
    if (set == NIL)
        return (0);

    assert (TXT_valid_set (set));
    return Sets [set].card;
}

void dumpSetNodes (unsigned int file, unsigned int setnode, unsigned int level,
		    unsigned int level_offset, unsigned int level_bredth)
/* Recursive procedure for dumping the contents of the radix tree nodes. */
{
    unsigned int count, segment_alloc, segment, pointer, threshold;
    unsigned int element_bytes, element_offset, p;
    unsigned char *bitset;
    boolean is_leafnode, is_terminal;

    assert (TXT_valid_file (file));
    assert (setnode != NIL);

    count = getSetNode (setnode, SEGMENT_OFFSET, SetSegmentBytes);
    assert (count < SetTreeBredth);

    threshold = expansionThreshold (level);
    is_leafnode = (level >= SetTreeDepth);
    if (count < threshold)
        segment_alloc = count;
    else
        segment_alloc = SetTreeBredth;

    element_bytes = getElementBytes (level, count);
    element_offset = SetStartOffset;
    fprintf (Files [file], "\n%d (%d):", level, setnode);
    for (p = 0; p < segment_alloc; p++)
      {
	is_terminal = FALSE;
	if (is_leafnode)
	  {
	    pointer = SetStartOffset; /* means the bit is set for this segment number */
	    if (count < threshold)
	        segment = getSetNode (setnode, element_offset, SetSegmentBytes);
	    else
	      {
		segment = p;
		bitset = (unsigned char *) SetMalloc + setnode + SetStartOffset;
		if (!ISSET_BITSET (bitset, segment))
		    pointer = NIL; /* means the bit isn't set */
	      }
	  }
	else if (count >= threshold)
	  {
	    segment = p;
	    pointer = getPointerSetNode (setnode, element_offset, &is_terminal);
	  }
	else
	  {
	    segment = getSetNode (setnode, element_offset, SetSegmentBytes);
	    pointer = getPointerSetNode (setnode, element_offset + SetSegmentBytes,
					 &is_terminal);
	  }
	if (is_terminal)
	    fprintf (Files [file], " %d = %d", segment, pointer);
	else if (!is_leafnode)
	    fprintf (Files [file], " %d [%d]", segment, pointer);

	/* Move along to the next segment position */
	element_offset += element_bytes;
      }

    element_offset = SetStartOffset;
    for (p = 0; p < segment_alloc; p++)
      {
	is_terminal = FALSE;
	if (is_leafnode)
	  {
	    pointer = SetStartOffset; /* means the bit is set for this segment number */
	    if (count < threshold)
	        segment = getSetNode (setnode, element_offset, SetSegmentBytes);
	    else
	      {
		segment = p;
		bitset = (unsigned char *) SetMalloc + setnode + SetStartOffset;
		if (!ISSET_BITSET (bitset, segment))
		    pointer = NIL; /* means the bit isn't set */
	      }
	  }
	else if (count >= threshold)
	  {
	    segment = p;
	    pointer = getPointerSetNode (setnode, element_offset, &is_terminal);
	  }
	else
	  {
	    if (debugSetsLevel > 2)
	        fprintf (stderr, "Getting from setnode %d element at %d level %d\n",
			 setnode, element_offset, level);
	    segment = getSetNode (setnode, element_offset, SetSegmentBytes);
	    pointer = getPointerSetNode (setnode, element_offset + SetSegmentBytes,
					 &is_terminal);
	  }
	if (debugSetsLevel > 3)
	    fprintf (stderr, "dump level %d segment %d pointer %d [%p]\n",
		     level, segment, pointer, (void *) pointer);

	if (pointer)
	  {
	    if (is_leafnode)
	        fprintf (Files [file], " %d = %d", segment, level_offset + segment);
	    else if (!is_terminal)
	        dumpSetNodes (file, pointer, level+1, level_offset + segment *
			      level_bredth, (level_bredth / SetTreeBredth));
	  }

	/* Move along to the next segment position */
	element_offset += element_bytes;
      }
}

void
TXT_dump_setnodes (unsigned int file, unsigned int set)
/* Dumps the contents of the set nodes. */
{
    assert (TXT_valid_file (file));
    assert (TXT_valid_set (set));

    if (set == NIL)
      {
	fprintf (Files [file], "set is NULL\n");
	return;
      }

    fprintf (Files [file], "card = %d (maxcard = %d tree bredth = %d tree depth = %d)\n",
	     Sets [set].card, SetMaxCard, SetTreeBredth, SetTreeDepth);

    fprintf (Files [file], "nodes:\n");

    if (Sets [set].tree)
        dumpSetNodes (file, Sets [set].tree, 1, 0, (SetMaxCard / SetTreeBredth));

    fprintf (Files [file], "\n");
}

void dumpSet (unsigned int file, unsigned int setnode, unsigned int level,
	      unsigned int level_offset, unsigned int level_bredth)
/* Recursive procedure for dumping the contents of the radix tree node. */
{
    unsigned int count, segment_alloc, segment, pointer, threshold;
    unsigned int element_bytes, element_offset, p;
    unsigned char *bitset;
    boolean is_leafnode, is_terminal;

    assert (TXT_valid_file (file));
    assert (setnode != NIL);

    count = getSetNode (setnode, SEGMENT_OFFSET, SetSegmentBytes);
    assert (count < SetTreeBredth);

    threshold = expansionThreshold (level);
    is_leafnode = (level >= SetTreeDepth);
    if (count < threshold)
        segment_alloc = count;
    else
        segment_alloc = SetTreeBredth;

    element_bytes = getElementBytes (level, count);
    element_offset = SetStartOffset;
    for (p = 0; p < segment_alloc; p++)
      {
	is_terminal = FALSE;
	if (is_leafnode)
	  {
	    pointer = SetStartOffset; /* means the bit is set for this segment number */
	    if (count < threshold)
	        segment = getSetNode (setnode, element_offset, SetSegmentBytes);
	    else
	      {
		segment = p;
		bitset = (unsigned char *) SetMalloc + setnode + SetStartOffset;
		if (!ISSET_BITSET (bitset, segment))
		    pointer = NIL; /* means the bit isn't set */
	      }
	  }
	else if (count >= threshold)
	  {
	    segment = p;
	    pointer = getPointerSetNode (setnode, element_offset, &is_terminal);
	  }
	else
	  {
	    if (debugSetsLevel > 2)
	        fprintf (stderr, "Getting from setnode %d element at %d level %d\n",
			 setnode, element_offset, level);
	    segment = getSetNode (setnode, element_offset, SetSegmentBytes);
	    pointer = getPointerSetNode (setnode, element_offset + SetSegmentBytes,
					 &is_terminal);
	  }
	if (debugSetsLevel > 3)
	    fprintf (stderr, "dump level %d segment %d pointer %d [%p]\n",
		     level, segment, pointer, (void *) pointer);

	if (pointer)
	  {
	    if (is_terminal)
	        fprintf (Files [file], " %d", pointer);
	    else if (is_leafnode)
	        fprintf (Files [file], " %d", level_offset + segment);
	    else
	        dumpSet (file, pointer, level+1, level_offset + segment *
			 level_bredth, (level_bredth / SetTreeBredth));
	  }

	/* Move along to the next segment position */
	element_offset += element_bytes;
      }
}

void TXT_dump_set (unsigned int file, unsigned int set)
/* Dumps the contents of the set. */
{
    assert (TXT_valid_file (file));

    if (set == NIL)
      {
	fprintf (Files [file], "card = 0, set = {}\n");
	return;
      }

    assert (TXT_valid_set (set));

    /*fprintf (Files [file], "card = %d (maxcard = %d tree bredth = %d tree depth = %d)\n",
      Sets [set].card, SetMaxCard, SetTreeBredth, SetTreeDepth);*/

    fprintf (Files [file], "card = %d, set = {", Sets [set].card);

    if (Sets [set].tree)
        dumpSet (file, Sets [set].tree, 1, 0, (SetMaxCard / SetTreeBredth));

    fprintf (Files [file], " }\n");
}

void getMembersSet (unsigned int *members, unsigned int setnode, unsigned int level,
		    unsigned int level_offset, unsigned int level_bredth)
/* Recursive procedure for getting the members of the radix tree node. */
{
    unsigned int count, segment_alloc, segment, pointer, threshold;
    unsigned int element_bytes, element_offset, p;
    unsigned char *bitset;
    boolean is_leafnode, is_terminal;

    assert (setnode != NIL);

    count = getSetNode (setnode, SEGMENT_OFFSET, SetSegmentBytes);
    assert (count <= SetTreeBredth);

    threshold = expansionThreshold (level);
    is_leafnode = (level >= SetTreeDepth);
    if (count < threshold)
        segment_alloc = count;
    else
        segment_alloc = SetTreeBredth;

    element_bytes = getElementBytes (level, count);
    element_offset = SetStartOffset;
    for (p = 0; p < segment_alloc; p++)
      {
	is_terminal = FALSE;
	if (is_leafnode)
	  {
	    pointer = SetStartOffset; /* means the bit is set for this segment number */
	    if (count < threshold)
	        segment = getSetNode (setnode, element_offset, SetSegmentBytes);
	    else
	      {
		segment = p;
		bitset = (unsigned char *) SetMalloc + setnode + SetStartOffset;
		if (!ISSET_BITSET (bitset, segment))
		    pointer = NIL; /* means the bit isn't set */
	      }
	  }
	else if (count >= threshold)
	  {
	    segment = p;
	    pointer = getPointerSetNode (setnode, element_offset, &is_terminal);
	  }
	else
	  {
	    if (debugSetsLevel > 2)
	        fprintf (stderr, "Getting from setnode %d element at %d level %d\n",
			 setnode, element_offset, level);
	    segment = getSetNode (setnode, element_offset, SetSegmentBytes);
	    pointer = getPointerSetNode (setnode, element_offset + SetSegmentBytes,
					 &is_terminal);
	  }
	if (debugSetsLevel > 3)
	    fprintf (stderr, "dump level %d segment %d pointer %d [%p]\n",
		     level, segment, pointer, (void *) pointer);

	if (pointer)
	  {
	    if (!is_leafnode && !is_terminal)
	        getMembersSet (members, pointer, level+1, level_offset + segment *
			       level_bredth, (level_bredth / SetTreeBredth));
	    else
	      {
		members [0]++;
		if (is_terminal)
		  members [members [0]] = pointer;
		else
		  members [members [0]] = level_offset + segment;
	      }
	  }

	/* Move along to the next segment position */
	element_offset += element_bytes;
      }
}

unsigned int *
TXT_get_setmembers (unsigned int set)
/* Returns the members of the set. */
{
    unsigned int *members;

    if (set == NIL)
        return (NULL);

    assert (TXT_valid_set (set));

    if (Sets [set].card == 0)
        return (NULL);
    else
      {
	members = (unsigned int *) malloc ((Sets [set].card + 1) * sizeof (int));
	members [0] = 0; /* element 0 contains the current number of members */

        getMembersSet (members, Sets [set].tree, 1, 0, (SetMaxCard / SetTreeBredth));
      }
    return (members);
}

void
TXT_dump_setmembers (unsigned int file, unsigned int *members)
/* Dumps out the members to the file. */
{
    unsigned int p;

    assert (TXT_valid_file (file));

    if (members == NULL)
        fprintf (Files [file], "[ ]\n");
    else
      {
        fprintf (Files [file], "[ ");
	for (p = 1; p <= members [0]; p++)
	  {
	    fprintf (Files [file], "%d ", members [p]);
	  }
        fprintf (Files [file], "]\n");
      }
}

void
TXT_release_setmembers (unsigned int *members)
/* Frees up the memory usage for members. */
{
    if (members == NULL)
        return;
    free (members);
}

void releaseSet (unsigned int setnode, unsigned int level,
		 unsigned int level_offset, unsigned int level_bredth)
/* Recursive procedure for deleting the contents of the radix tree node. */
{
    unsigned int count, segment_alloc, segment, pointer, threshold;
    unsigned int element_bytes, element_offset, p;
    unsigned char *bitset;
    boolean is_leafnode, is_terminal;

    assert (setnode != NIL);

    count = getSetNode (setnode, SEGMENT_OFFSET, SetSegmentBytes);
    assert (count <= SetTreeBredth);

    threshold = expansionThreshold (level);
    is_leafnode = (level >= SetTreeDepth);
    if (count < threshold)
        segment_alloc = count;
    else
        segment_alloc = SetTreeBredth;

    if (level < SetTreeDepth)
      {
	element_bytes = getElementBytes (level, count);
	element_offset = SetStartOffset;
	for (p = 0; p < segment_alloc; p++)
	  {
	    is_terminal = FALSE;
	    if (is_leafnode)
	      {
		pointer = SetStartOffset; /* means the bit is set for this segment number */
		if (count < threshold)
		    segment = getSetNode (setnode, element_offset, SetSegmentBytes);
		else
		  {
		    segment = p;
		    bitset = (unsigned char *) SetMalloc + setnode + SetStartOffset;
		    if (!ISSET_BITSET (bitset, segment))
		        pointer = NIL; /* means the bit isn't set */
		  }
	      }
	    else if (count >= threshold)
	      {
		segment = p;
		pointer = getPointerSetNode (setnode, element_offset, &is_terminal);
	      }
	    else
	      {
		segment = getSetNode (setnode, element_offset, SetSegmentBytes);
		pointer = getPointerSetNode (setnode, element_offset + SetSegmentBytes,
					     &is_terminal);
	      }

	    if (pointer)
	      if (!is_leafnode && !is_terminal)
		releaseSet (pointer, level+1, level_offset + segment * level_bredth,
			    (level_bredth / SetTreeBredth));

	    /* Move along to the next segment position */
	    element_offset += element_bytes;
	  }
      }

    /* Free the contents of this set node */
    releaseSetNode (setnode, level, count);
}

void
TXT_release_set (unsigned int set)
/* Frees the contents of the set for later re-use. */
{
    assert (TXT_valid_set (set));

    if (Sets [set].tree)
        releaseSet (Sets [set].tree, 1, 0, (SetMaxCard / SetTreeBredth));

    Sets [set].card = 0;
    Sets [set].tree = NIL;

    /* Append onto head of the used list */
    Sets [set].next = Sets_used;
    Sets_used = set;
}

void
TXT_delete_set (unsigned int set)
/* Deletes the contents of the set. */
{
    TXT_release_set (set);
}

boolean
/* on your marks, ... */ 
TXT_get_set (unsigned int set, unsigned int value)
/* Returns TRUE if the integer value is in the set. */
{
    unsigned int card, level, level_bredth, level_pos, element_offset, pointer_offset;
    unsigned int setnode, segment, pointer;
    boolean is_terminal;

    if (set == NIL)
        return (FALSE);

    assert (TXT_valid_set (set));
    assert (!SetMaxCard || (value < SetMaxCard));

    card = Sets [set].card;
    setnode = Sets [set].tree;

    segment = value;
    level_pos = value;
    level = 1;
    level_bredth = SetMaxCard;
    while (level < SetTreeDepth)
      {
	level_bredth /= SetTreeBredth;
	segment = level_pos / level_bredth;
	level_pos = level_pos % level_bredth;

	/* Find the vector pointer and move down a level */
	findSetNode (setnode, level, segment, &element_offset, &pointer_offset);
	if (element_offset == NIL)
	    return (FALSE);

	assert (pointer_offset != NIL);
	pointer = getPointerSetNode (setnode, pointer_offset, &is_terminal);
	if (is_terminal)
	  { /* terminal node found - stop here */
	    if (value == pointer)
	        return (TRUE);
	    else
	        return (FALSE);
	  }

	setnode = pointer; /* Move down to the next level: */

	level++;
      }

    findSetNode (setnode, level, level_pos, &element_offset, &pointer_offset);
    if (!element_offset)
        return (FALSE);
    else
	return (TRUE);
}

void putSet (unsigned int set, unsigned int setnode, unsigned int value,
	      unsigned int level, unsigned int level_bredth, unsigned int level_pos,
	      unsigned int parent_node, unsigned int parent_offset)
/* Possibly recursive routine for setting the value in the set. */
{
    unsigned int segment, pointer, pointer_offset, element_offset;
    unsigned int that_setnode, that_value, this_level, that_level;
    unsigned that_level_bredth, that_level_pos;
    unsigned int that_parent_node, that_parent_offset;
    boolean node_exists, is_terminal;

    is_terminal = FALSE;
    parent_node = NIL;
    parent_offset = 0;

    that_setnode = NIL;
    that_value = 0;
    that_level = 0;
    that_level_pos = 0;
    that_level_bredth = 0;
    that_parent_node = NIL;
    that_parent_offset = 0;

    segment = value;
    while (level < SetTreeDepth)
      {
	level_bredth /= SetTreeBredth;
	segment = level_pos / level_bredth;
	level_pos = level_pos % level_bredth;

	/* Find the position of the pointer or number at segment */
	findSetNode (setnode, level, segment, &element_offset, &pointer_offset);
	node_exists = (element_offset);

	if (!node_exists)
	  { /* segment not found - need to insert a new terminal node */
	    if (!PerformTermination)
	        pointer = NIL;
	    else /* set bit to indicate a terminal node */
	        pointer = value + TERMINAL_BIT;

	    setnode = insertSetNode (setnode, level, value, segment, pointer,
				     &element_offset, &pointer_offset);

	    if (debugSetsLevel > 2)
	        fprintf (stderr, "Putting setnode %d element at %d level %d\n",
			 setnode, element_offset, level);

	    if (parent_node == NIL)
	        Sets [set].tree = setnode;
	    else
		putSetNode (parent_node, parent_offset, SetPointerBytes, setnode);

	    if (PerformTermination)
	      {
		Sets [set].card++;
	        break; /* reached a terminal node so stop here */
	      }
	  }
	else
	  { /* found segment */
	    pointer = getPointerSetNode (setnode, pointer_offset, &is_terminal);
	    if (is_terminal)
	      { /* there was an old value stored at a terminal node;
		   it has been temporarily replaced - save the old values,
		   then re-insert it later on */
		that_setnode = setnode;
		that_value = pointer;
		that_level = level;

		that_level_pos = pointer;
		that_level_bredth = SetMaxCard;
		for (this_level = 1; this_level < level; this_level++)
		  {
		    that_level_bredth /= SetTreeBredth;
		    that_level_pos = that_level_pos % that_level_bredth;
		  }

		that_parent_node = parent_node;
		that_parent_offset = parent_offset;

		/* Replace terminal pointer with NIL pointer; this ensures that
		   the new value will get inserted as if there hadn't existed
		   an old value beforehand */
		pointer = NIL;
		putSetNode (setnode, pointer_offset, SetPointerBytes, NIL);
	      }
	  }

	if (debugSetsLevel > 3)
	    fprintf (stderr, "setnode = %d pointer %d pointer offset = %d element offset = %d\n",
		     setnode, pointer, pointer_offset, element_offset);

	/* Move down to the next level: */
	parent_node = setnode;
	parent_offset = pointer_offset;
	setnode = pointer;
	level++;
      }

    if (level >= SetTreeDepth)
      { /* Insert the element into the leaf node */
	findSetNode (setnode, level, level_pos, &element_offset, &pointer_offset);
	if (!element_offset)
	  {
	    Sets [set].card++;
	    setnode = insertSetNode (setnode, level, value, level_pos, NIL,
				     &element_offset, &pointer_offset);
	    if (parent_node == NIL)
	        Sets [set].tree = setnode;
	    else
	        putSetNode (parent_node, parent_offset, SetPointerBytes, setnode);
	  }
      }

    if (that_setnode != NIL)
      { /* we have an old value that we need to re-insert */
	Sets [set].card--;
	putSet (set, that_setnode, that_value, that_level, that_level_bredth,
		 that_level_pos, that_parent_node, that_parent_offset);
      }
}

void
TXT_put_set (unsigned int set, unsigned int value)
/* Puts the integer value into the set. */
{
    assert (TXT_valid_set (set));
    assert (!SetMaxCard || (value < SetMaxCard));

    if (debugSetsLevel > 1)
        fprintf (stderr, "Setting integer %d\n", value);

    putSet (set, Sets [set].tree, value, 1, SetMaxCard, value, NIL, NIL);
}

boolean
equalSet (unsigned int set, unsigned int setnode, unsigned int level,
	  unsigned int level_offset, unsigned int level_bredth)
/* Recursive procedure for testing if the members of set equal those
   stored starting at the radix tree node. */
{
    unsigned int count, segment_alloc, segment, pointer, threshold;
    unsigned int element_bytes, element_offset, p;
    unsigned char *bitset;
    boolean is_leafnode, is_terminal;

    assert (setnode != NIL);

    count = getSetNode (setnode, SEGMENT_OFFSET, SetSegmentBytes);
    assert (count <= SetTreeBredth);

    threshold = expansionThreshold (level);
    is_leafnode = (level >= SetTreeDepth);
    if (count < threshold)
        segment_alloc = count;
    else
        segment_alloc = SetTreeBredth;

    element_bytes = getElementBytes (level, count);
    element_offset = SetStartOffset;
    for (p = 0; p < segment_alloc; p++)
      {
	is_terminal = FALSE;
	if (is_leafnode)
	  {
	    pointer = SetStartOffset; /* means the bit is set for this segment number */
	    if (count < threshold)
	        segment = getSetNode (setnode, element_offset, SetSegmentBytes);
	    else
	      {
		segment = p;
		bitset = (unsigned char *) SetMalloc + setnode + SetStartOffset;
		if (!ISSET_BITSET (bitset, segment))
		    pointer = NIL; /* means the bit isn't set */
	      }
	  }
	else if (count >= threshold)
	  {
	    segment = p;
	    pointer = getPointerSetNode (setnode, element_offset, &is_terminal);
	  }
	else
	  {
	    if (debugSetsLevel > 2)
	        fprintf (stderr, "Getting from setnode %d element at %d level %d\n",
			 setnode, element_offset, level);
	    segment = getSetNode (setnode, element_offset, SetSegmentBytes);
	    pointer = getPointerSetNode (setnode, element_offset + SetSegmentBytes,
					 &is_terminal);
	  }

	if (pointer)
	  {
	    if (!is_leafnode && !is_terminal)
	      {
		if (!equalSet (set, pointer, level+1, level_offset + segment *
			       level_bredth, (level_bredth / SetTreeBredth)))
		  return (FALSE);
	      }
	    else
	      {
		unsigned int value;

		if (is_terminal)
		  value = pointer;
		else
		  value = level_offset + segment;
		if (!TXT_get_set (set, value))
		  return (FALSE);
	      }
	  }

	/* Move along to the next segment position */
	element_offset += element_bytes;
      }
    return (TRUE);
}

boolean
TXT_equal_set (unsigned int set, unsigned int set1)
/* Returns TRUE if set equals set1. */
{
    if (set == set1)
        return (TRUE);
    else if ((set == NIL) || (set1 == NIL))
        return (FALSE);

    assert (TXT_valid_set (set));
    assert (TXT_valid_set (set1));

    if (Sets [set].card != Sets [set1].card)
        return (FALSE);
    else if (Sets [set].card == 0)
        return (TRUE);
    else if ((Sets [set].tree == NIL) || (Sets [set1].tree == NIL))
        return (FALSE);
    else
        return (equalSet (set, Sets [set1].tree, 1, 0, (SetMaxCard / SetTreeBredth)));
}

void
intersectSet (unsigned int set, unsigned int set1, unsigned int setnode,
	      unsigned int level, unsigned int level_offset, unsigned int level_bredth)
/* Recursive procedure for intersecting the contents of the radix tree node. */
{
    unsigned int count, segment_alloc, segment, pointer, threshold;
    unsigned int element_bytes, element_offset, p;
    unsigned char *bitset;
    boolean is_leafnode, is_terminal;

    assert (setnode != NIL);

    count = getSetNode (setnode, SEGMENT_OFFSET, SetSegmentBytes);
    assert (count < SetTreeBredth);

    threshold = expansionThreshold (level);
    is_leafnode = (level >= SetTreeDepth);
    if (count < threshold)
        segment_alloc = count;
    else
        segment_alloc = SetTreeBredth;

    element_bytes = getElementBytes (level, count);
    element_offset = SetStartOffset;
    for (p = 0; p < segment_alloc; p++)
      {
	is_terminal = FALSE;
	if (is_leafnode)
	  {
	    pointer = SetStartOffset; /* means the bit is set for this segment number */
	    if (count < threshold)
	        segment = getSetNode (setnode, element_offset, SetSegmentBytes);
	    else
	      {
		segment = p;
		bitset = (unsigned char *) SetMalloc + setnode + SetStartOffset;
		if (!ISSET_BITSET (bitset, segment))
		    pointer = NIL; /* means the bit isn't set */
	      }
	  }
	else if (count >= threshold)
	  {
	    segment = p;
	    pointer = getPointerSetNode (setnode, element_offset, &is_terminal);
	  }
	else
	  {
	    segment = getSetNode (setnode, element_offset, SetSegmentBytes);
	    pointer = getPointerSetNode (setnode, element_offset + SetSegmentBytes,
					 &is_terminal);
	  }

	if (pointer)
	  {
	    if (!is_leafnode && !is_terminal)
	      intersectSet (set, set1, pointer, level+1, level_offset + segment *
			    level_bredth, (level_bredth / SetTreeBredth));
	    else
	      {
		unsigned int value;

		if (is_terminal)
		  value = pointer;
		else
		  value = level_offset + segment;
		if (TXT_get_set (set1, value))
		  {
		    /*fprintf (stderr, "found %d\n", value);*/
		    TXT_put_set (set, value);
		  }
	      }
	  }

	/* Move along to the next segment position */
	element_offset += element_bytes;
      }
}

unsigned int 
TXT_intersect_set (unsigned int set1, unsigned int set2)
/* Intersects sets set1 with set2 and returns the result in a new set.
   There is a much more efficient way of doing this (by merging
   the two lists and buffering the deletions) but that has to be
   left to be done at a later date. */
{
    unsigned int set;

    assert (TXT_valid_set (set1));
    assert (TXT_valid_set (set2));

    set = TXT_create_set ();

    if ((Sets [set1].tree != NIL) && (Sets [set2].tree != NIL))
        intersectSet (set, set1, Sets [set2].tree, 1, 0, (SetMaxCard / SetTreeBredth));
    return (set);
}

void
unionSet (unsigned int set, unsigned int set1, unsigned int setnode,
	 unsigned int level, unsigned int level_offset, unsigned int level_bredth)
/* Recursive procedure for adding the contents of the radix tree node in set1 to set. */
{
    unsigned int count, segment_alloc, segment, pointer, threshold;
    unsigned int element_bytes, element_offset, p;
    unsigned char *bitset;
    boolean is_leafnode, is_terminal;

    assert (setnode != NIL);

    count = getSetNode (setnode, SEGMENT_OFFSET, SetSegmentBytes);
    assert (count < SetTreeBredth);

    threshold = expansionThreshold (level);
    is_leafnode = (level >= SetTreeDepth);
    if (count < threshold)
        segment_alloc = count;
    else
        segment_alloc = SetTreeBredth;

    element_bytes = getElementBytes (level, count);
    element_offset = SetStartOffset;
    for (p = 0; p < segment_alloc; p++)
      {
	is_terminal = FALSE;
	if (is_leafnode)
	  {
	    pointer = SetStartOffset; /* means the bit is set for this segment number */
	    if (count < threshold)
	        segment = getSetNode (setnode, element_offset, SetSegmentBytes);
	    else
	      {
		segment = p;
		bitset = (unsigned char *) SetMalloc + setnode + SetStartOffset;
		if (!ISSET_BITSET (bitset, segment))
		    pointer = NIL; /* means the bit isn't set */
	      }
	  }
	else if (count >= threshold)
	  {
	    segment = p;
	    pointer = getPointerSetNode (setnode, element_offset, &is_terminal);
	  }
	else
	  {
	    segment = getSetNode (setnode, element_offset, SetSegmentBytes);
	    pointer = getPointerSetNode (setnode, element_offset + SetSegmentBytes,
					 &is_terminal);
	  }

	if (pointer)
	  {
	    if (!is_leafnode && !is_terminal)
	      unionSet (set, set1, pointer, level+1, level_offset + segment *
			level_bredth, (level_bredth / SetTreeBredth));
	    else
	      {
		unsigned int value;

		if (is_terminal)
		  value = pointer;
		else
		  value = level_offset + segment;
		/*fprintf (stderr, "found %d\n", value);*/
		TXT_put_set (set, value);
	      }
	  }

	/* Move along to the next segment position */
	element_offset += element_bytes;
      }
}

void
TXT_union_set (unsigned int set, unsigned int set1)
/* Assigns to set the union of set and set1. */
{
    assert (TXT_valid_set (set));
    assert (TXT_valid_set (set1));

    if (Sets [set1].tree != NIL)
        unionSet (set, set1, Sets [set1].tree, 1, 0, (SetMaxCard / SetTreeBredth));
}

unsigned int 
TXT_copy_set (unsigned int set)
/* Copies the set and returns the result in a new set. */
{
    unsigned int new_set;

    assert (TXT_valid_set (set));

    new_set = TXT_create_set ();

    if (Sets [set].tree != NIL)
        unionSet (new_set, set, Sets [set].tree, 1, 0, (SetMaxCard / SetTreeBredth));
    return (new_set);
}

void loadSet (unsigned int file, unsigned int set, unsigned int lo, unsigned int hi,
	      unsigned int card)
/* Recursive sub-procedure for TXT_read_set routine. lo and hi define the current number
   range being read. */
{
    unsigned int range, bytes, median, card1, n;

    assert (TXT_valid_file (file));
    assert (TXT_valid_set (set));

    if (debugSetsLevel > 1)
        fprintf (stderr, "loadSet: lo %d hi %d card %d\n", lo, hi, card);
    range = hi - lo + 1;
    if (range <= card)
      { /* suppression - the entire range is read implicitly */
	for (n = lo; n <= hi; n++)
	    TXT_put_set (set, n);
	return;
      }
    bytes = getBytes (range);

    median = fread_int (file, bytes) + lo;
    TXT_put_set (set, median);
    if (debugSetsLevel > 1)
        fprintf (stderr, "loadSet: median %d\n", median);

    card1 = (card-1)/2;
    if (card1 > 0)
        loadSet (file, set, lo, median - 1, card1); /* read in the left sub-tree */
    card1 = card/ 2;
    if (card1 > 0)
        loadSet (file, set, median + 1, hi, card1); /* read in the right sub-tree */
}

unsigned int 
TXT_load_set (unsigned int file)
/* Loads the set from the file. The file must have been written
   using TXT_awrite_set (). */
{
    unsigned int card, first, last;
    unsigned int set;

    assert (TXT_valid_file (file));

    set = TXT_create_set ();

    SetBytesIn = 0;

    /* Read in the cardinality first */
    card = fread_int (file, SetValueBytes);
    if (card == 0) /* empty set */
	return (set);

    /* Read in the first and last members */
    first = fread_int (file, SetValueBytes);
    TXT_put_set (set, first);
    last = first + 1;
    if (card > 1)
      {
        last = fread_int (file, SetValueBytes);
	TXT_put_set (set, last);
      }

    if (card > 2)
        loadSet (file, set, first+1, last-1, card-2);

    return (set);
}

void writeSet (unsigned int file, unsigned int *members, unsigned int lo, unsigned int hi,
	       unsigned int lo_value, unsigned int hi_value)
/* Recursive sub-procedure for TXT_write_set routine. lo_value and hi_value define
   the current value range being written out. */
{
    unsigned int range, bytes, median, median_value, card, card1;

    assert (TXT_valid_file (file));

    card = hi_value - lo_value + 1;
    if (debugSetsLevel > 1)
        fprintf (stderr, "writeSet: lo %d hi %d lo_value %d hi_value %d card %d\n",
		 lo, hi, lo_value, hi_value, card);
    range = hi - lo + 1;
    if (range <= card)
      { /* suppression - don't need to write anything */
	return;
      }

    card1 = (card + 1) / 2;
    median_value = lo_value + card1 - 1;
    /*
      fprintf (stderr, "median_value %d lo_value %d card1 %d\n",
               median_value, lo_value, card1);
    */
    median = members [median_value];

    if (debugSetsLevel > 1)
        fprintf (stderr, "writeSet: median %d at pos %d\n", median, median_value);
    bytes = getBytes (range);
    fwrite_int (file, median - lo, bytes);

    /* read in the left sub-tree: */
    if (lo_value < median_value)
        writeSet (file, members, lo, median - 1, lo_value, median_value-1);
    /* read in the right sub-tree: */
    if (median_value < hi_value)
        writeSet (file, members, median + 1, hi, median_value+1, hi_value);
}

void TXT_write_set (unsigned int file, unsigned int set)
/* Writes the set to the file. Uses a faster algorithm but
   gets less compression. */
{
    unsigned int card, first, last;
    unsigned int *members;

    assert (TXT_valid_file (file));

    SetBytesOut = 0;

    if (set == NIL)
        card = 0;
    else
        card = Sets [set].card;

    /* Write out the cardinality first */
    fwrite_int (file, card, SetValueBytes);

    if (card == 0)
	return;

    assert (TXT_valid_set (set));

    members = TXT_get_setmembers (set);

    /* Write out the first and last members */
    first = members [1];
    last = first;
    fwrite_int (file, first, SetValueBytes);
    if (card > 1)
      {
	last = members [members [0]];
        fwrite_int (file, last, SetValueBytes);
      }

    if (card > 2)
        writeSet (file, members, first+1, last-1, 2, card-1);

    TXT_release_setmembers (members);
}

void aloadSet (unsigned int file, unsigned int set, unsigned int lo, unsigned int hi,
	       unsigned int card)
/* Recursive sub-procedure for TXT_load_set routine. lo and hi define the current
   number range being read. */
{
    unsigned int range, median, card1, n;

    assert (TXT_valid_file (file));

    if (debugSetsLevel > 1)
        fprintf (stderr, "aloadSet: lo %d hi %d card %d\n", lo, hi, card);
    range = hi - lo + 1;
    if (range <= card)
      { /* suppression - the entire range is read implicitly */
	for (n = lo; n <= hi; n++)
	    TXT_put_set (set, n);
	return;
      }

    median = arith_read_int (file, range) + lo;
    TXT_put_set (set, median);
    if (debugSetsLevel > 1)
        fprintf (stderr, "aloadSet: median %d\n", median);

    card1 = (card-1)/2;
    if (card1 > 0)
        aloadSet (file, set, lo, median - 1, card1); /* read in the left sub-tree */
    card1 = card/ 2;
    if (card1 > 0)
        aloadSet (file, set, median + 1, hi, card1); /* read in the right sub-tree */
}

unsigned int 
TXT_aload_set (unsigned int file)
/* Loads the set from the file. The file must have been written
   using TXT_awrite_set (). */
{
    unsigned int card, first, last;
    unsigned int set;

    assert (TXT_valid_file (file));

    arith_decode_start (file);

    set = TXT_create_set ();

    SetBytesIn = 0;

    /* Read in the cardinality first */
    card = arith_read_int (file, SetMaxCard);
    if (card == 0) /* empty set */
	return (set);

    /* Read in the first and last members */
    first = arith_read_int (file, SetMaxCard);
    TXT_put_set (set, first);
    last = first + 1;
    if (card > 1)
      {
        last = arith_read_int (file, SetMaxCard);
	TXT_put_set (set, last);
      }

    if (debugSetsLevel > 3)
        fprintf (stderr, "Card = %d first = %d last = %d\n", card, first, last);

    if (card > 2)
        aloadSet (file, set, first+1, last-1, card-2);

    arith_decode_finish (file);

    return (set);
}

void awriteSet (unsigned int file, unsigned int *members, unsigned int lo, unsigned int hi,
		unsigned int lo_value, unsigned int hi_value)
/* Recursive sub-procedure for TXT_awrite_set routine. lo_value and hi_value define
   the current value range being written out. */
{
    unsigned int range, median, median_value, card, card1;

    assert (TXT_valid_file (file));

    card = hi_value - lo_value + 1;
    if (debugSetsLevel > 1)
        fprintf (stderr, "awriteSet: lo %d hi %d lo_value %d hi_value %d card %d\n",
		 lo, hi, lo_value, hi_value, card);
    range = hi - lo + 1;
    if (range <= card)
      { /* suppression - don't need to write anything */
	return;
      }

    card1 = (card + 1) / 2;
    median_value = lo_value + card1 - 1;
    median = members [median_value];

    if (debugSetsLevel > 1)
        fprintf (stderr, "awriteSet: median %d\n", median);
    arith_write_int (file, median - lo, range);

    /* read in the left sub-tree: */
    if (lo_value < median_value)
        awriteSet (file, members, lo, median - 1, lo_value, median_value-1);
    /* read in the right sub-tree: */
    if (median_value < hi_value)
        awriteSet (file, members, median + 1, hi, median_value+1, hi_value);
}

void TXT_awrite_set (unsigned int file, unsigned int set)
/* Writes the set to the file. Uses a slower algorithm but
   gets better compression. */
{
    unsigned int card, first, last;
    unsigned int *members;

    assert (TXT_valid_file (file));
    assert (TXT_valid_set (set));

    arith_encode_start (file);

    SetBytesOut = 0;

    if (set == NIL)
        card = 0;
    else
        card = Sets [set].card;

    /* Write out the cardinality first */
    arith_write_int (file, card, SetMaxCard);

    if (card == 0)
	return;

    /*TXT_dump_set (stderr, set);*/
    members = TXT_get_setmembers (set);
    /*dumpMembersSet (stderr, members);*/

    /* Write out the first and last members */
    first = members [1];
    last = first;
    arith_write_int (file, first, SetMaxCard);
    if (card > 1)
      {
	last = members [members [0]];
        arith_write_int (file, last, SetMaxCard);
      }

    if (debugSetsLevel > 3)
        fprintf (stderr, "Card = %d first = %d last = %d\n", card, first, last);

    if (card > 2)
        awriteSet (file, members, first+1, last-1, 2, card-1);

    TXT_release_setmembers (members);

    SetBytesOut = bytes_output;
    arith_encode_finish (file);
}
