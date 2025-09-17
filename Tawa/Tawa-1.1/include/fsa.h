/* Routines for Finite State Autonomas (FSAs). */

#ifndef FSA_H
#define FSA_H

/* The following specifies different types of FSA states */
#define TLM_fsa_plain_state 0  /* The state is a standard state for the FSA */
#define TLM_fsa_start_state 1  /* The state is the start state for the FSA */
#define TLM_fsa_accept_state 0 /* The state is an accept state for the FSA */

unsigned int
TLM_create_state (unsigned int state_type);
/* Creates a new FSA state. */

void
TLM_release_state (unsigned int state);
/* Releases the memory allocated to the FSA state. */

unsigned int
TLM_add_transition (unsigned int from_state, unsigned int to_state,
		    unsigned int transition_text)
/* Adds a transition from the from_state to the to_state. The tranisiton_text
   is a text record that defines the transition condition. */

void
TLM_dump_fsa (unsigned int file, unsigned int start_state);
/* Dumps (for debugging purposes) the FSA starting from the specified
   starte_state. */

#endif
