/* TXT routines for lists. For the time being, there is only one
   implementation - frequency sorted, single linked lists. */

#ifndef LIST_H
#define LIST_H

boolean
TXT_valid_list (unsigned int list);
/* Returns non-zero if the list is valid, zero otherwize. */

unsigned int 
TXT_create_list (void);
/* Return a pointer to a new list record. */

void
TXT_dump_list
(unsigned int file, unsigned int list,
 void (*dump_value_function) (unsigned int, unsigned int, unsigned int));
/* Dumps the contents of the list. */

void
TXT_release_list (unsigned int list);
/* Frees the contents of the list for later re-use. */

unsigned int
/* Returns TRUE if the list contains the value. */ 
TXT_get_list (unsigned int list, unsigned int number);
/* Returns the frequency of the unsigned integer number in the list
   (0 if it is not found in the list). */

void
TXT_put_list (unsigned int list, unsigned int number, unsigned int frequency);
/* Puts the unsigned integer into the list if it does not already exist,
   and adds frequency to its associated frequency count. */

unsigned int 
TXT_load_list (unsigned int file);
/* Loads the list from the file. */

void
TXT_write_list (unsigned int file, unsigned int list);
/* Writes the list to the file. */

void
TXT_reset_list (unsigned int list);
/* Resets the current record so that the next call to TXT_next_list
   will return the first record in the list. */

boolean
TXT_next_list (unsigned int list, unsigned int *number,
	       unsigned int *frequency);
/* Returns the next record in the list. Also returns the number and frequency
   stored in the record. Note that a call to TXT_reset_list will reset the
   list that is being sequentially accessed (i.e. Multiple lists cannot
   be accessed simultaneously in this implementation). */

#endif
