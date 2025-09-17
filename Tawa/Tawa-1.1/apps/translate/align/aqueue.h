/* Routines for alignment queues (to store the best alignments in a queue). */

#ifndef AQUEUE_H
#define AQUEUE_H

boolean
TXT_valid_aqueue (unsigned int aqueue);
/* Returns non-zero if the alignment queue is valid, zero otherwize. */

unsigned int 
TXT_create_aqueue (void);
/* Return a pointer to a new alignment queue record. */

void
TXT_dump_aqueue
(unsigned int file, unsigned int aqueue,
 void (*dump_value_function) (unsigned int, unsigned int));
/* Dumps the contents of the alignment queue. */

void
TXT_release_aqueue (unsigned int aqueue);
/* Frees the contents of the alignment queue for later re-use. */

unsigned int
TXT_insert_aqueue (unsigned int aqueue, unsigned int number);
/* Inserts the unsigned integer at the head of the alignment queue. */

#endif
