/* Routines for Finite State Autonomas (FSAs). */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <ctype.h>
#include <assert.h>
#include "io.h"
#include "text.h"

#define STATES_SIZE 20                   /* Initial size of States array */

struct stateType
{ /* Input state to correct, or corrected output state */
    unsigned int State_type;             /* The type of the state */
    unsigned int *State_transitions;     /* A table of transitions associated
					    with the state/ */
};
#define State_next State_length          /* Used to find the "next" on the
					    used list for deleted states */

struct stateType *States = NULL;         /* List of state records */
unsigned int States_max_size = 0;        /* Current max. size of the states
					    array */
unsigned int States_used = NIL;          /* List of deleted state records */
unsigned int States_unused = 1;          /* Next unused state record */

boolean
TLM_valid_state (unsigned int state)
/* Returns non-zero if the state is valid, zero otherwize. */
{
    if (state == NIL)
        return (FALSE);
    else if (state >= States_unused)
        return (FALSE);
    else if (States [state].State_type == TLM_invalid_stateing)
        return (FALSE);
    else
        return (TRUE);
}

unsigned int
create_state (unsigned int state_type, unsigned int textlen)
/* Creates a state record. :-) */
{
    struct stateType *state;
    unsigned int t, old_size;
    unsigned int *coderangesp;
    float *codelenp;

    if (States_used != NIL)
    {	/* use the first list of states on the used list */
	t = States_used;
	States_used = States [t].State_next;
    }
    else
    {
	t = States_unused;
	if (States_unused+1 >= States_max_size)
	{ /* need to extend States array */
	    old_size = States_max_size * sizeof (struct stateType);
	    if (States_max_size == 0)
		States_max_size = STATES_SIZE;
	    else
		States_max_size *= 2; /* Keep on doubling the array on demand */
	    States = (struct stateType *) Realloc (101, States,
		     States_max_size * sizeof (struct stateType), old_size);

	    if (States == NULL)
	    {
		fprintf (stderr, "Fatal error: out of states space\n");
		exit (1);
	    }
	}
	States_unused++;
    }

    if (t != NIL)
    {
      state = States + t;

      state->State_length = 0;
      state->State_type = state_type;
      state->State_codelength = NULL;
      state->State_coderanges = NULL;

      state->State_length = textlen;
      codelenp = (float *) Malloc (102, (textlen+2) * sizeof (float));
      state->State_codelength = codelenp;

      if (state_type != TLM_comprehensive_stateing)
	  coderangesp = NULL;
      else
	  coderangesp = (unsigned int *) Malloc (102, (textlen+2) * sizeof (float));
      state->State_coderanges = coderangesp;
    }
    return (t);
}

void
TLM_release_state (unsigned int state)
/* Releases the memory allocated to the model's state and the state number
   (which may be reused in later create_state calls). */
{
    struct stateType *statep;

    assert (TLM_valid_state (state));

    statep = States + state;
    if (statep->State_codelength != NULL)
        Free (102, statep->State_codelength, statep->State_length *
	      sizeof (float));
    if (statep->State_coderanges != NULL)
        Free (102, statep->State_coderanges, statep->State_length *
	      sizeof (unsigned int));

    statep->State_type = TLM_invalid_stateing; /* Used for testing later on if
						 state record is valid or not */
    statep->State_length = 1; /* Used for testing later on if state no. is valid or not */

    /* Append onto head of the used list */
    statep->State_next = States_used;
}

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

unsigned int
TLM_make_state (unsigned int state_type, unsigned int model, unsigned int text)
/* Creates a new state record associated with using the model
   to predict the sequence of text. */
{
    struct stateType *statep;
    unsigned int state, context, textlen, symbol, p;
    unsigned int *coderangesp;
    float *codelenp;

    assert (TLM_valid_model (model));
    assert (TXT_valid_text (text));
    textlen = TXT_length_text (text);

    state = create_state (state_type, textlen);
    statep = States + state;
    codelenp = statep->State_codelength;
    coderangesp = statep->State_coderanges;

    if (state_type != TLM_comprehensive_stateing)
        TLM_set_context_operation (TLM_Get_Codelength);
    else
        TLM_set_context_operation (TLM_Get_Coderanges);

    context = TLM_create_context (model);

    /* Now encode each symbol */
    for (p=0; p < textlen; p++) /* encode each symbol */
    {
      TXT_get_symbol (text, p, &symbol);
      TLM_update_context (model, context, symbol);
      if (state_type != TLM_comprehensive_stateing)
	{
	  /*fprintf (stderr, "pos %d symbol %d codelen %.3f\n", p, symbol,
	    TLM_Codelength);*/
	  codelenp [p] = TLM_Codelength;
	}
      else
	{ /* comprehensive stateing */
	  /*
	    fprintf (stderr, "pos %d symbol %d coderanges ", p, symbol);
	    TLM_dump_coderanges (stderr, TLM_Coderanges);
	  */
	  coderangesp [p] = TLM_Coderanges;
	  codelenp [p] = TLM_codelength_coderanges (TLM_Coderanges);
	}
    }
    TLM_release_context (model, context);

    return (state);
}

unsigned int
TLM_make_words_state (unsigned int state_type, unsigned int words_model,
		      unsigned int text)
/* Creates a new state record associated with using the words model
   to predict the sequence of text. */
{
    struct stateType *statep;
    unsigned int state, textlen, textpos, word, nonword, p, q;
    unsigned int nonword_model, word_model, nonchar_model, char_model;
    unsigned int nonword_context, word_context, nonchar_context, char_context;
    unsigned int nonword_table, word_table;
    unsigned int wordpos, nonwordpos;
    codingType coding_type;
    unsigned int *coderangesp;
    boolean eof;
    float *codelenp;

    assert (TLM_valid_words_model (words_model));

    TLM_get_words_model
      (words_model, &nonword_model, &word_model, &nonchar_model, &char_model,
       &nonword_table, &word_table);

    assert (TXT_valid_text (text));
    textlen = TXT_length_text (text);

    state = create_state (state_type, textlen);
    statep = States + state;
    codelenp = statep->State_codelength;
    coderangesp = statep->State_coderanges;

    word = TXT_create_text ();
    nonword = TXT_create_text ();

    word_context = TLM_create_context (word_model);
    char_context = TLM_create_context (char_model);
    nonword_context = TLM_create_context (nonword_model);
    nonchar_context = TLM_create_context (nonchar_model);

    textpos = 0;
    if (state_type != TLM_comprehensive_stateing)
        coding_type = FIND_CODELENGTH_TYPE;
    else
        coding_type = FIND_CODERANGES_TYPE;
    for (;;) /* encode each non-word and word */
    {
      p = textpos;
      eof = !TXT_getword_text1 (text, nonword, word, &textpos, &nonwordpos, &wordpos);

      for (q = p; q < textpos; q++)
	{ /* Set codelengths to 0 & coderanges to NIL; these will be filled in latter at
	     two positions only: at the start of the nonword and at the start of the word */
	  codelenp [q] = 0;
	  if (state_type == TLM_comprehensive_stateing)
	      coderangesp [q] = NIL;
	}

      codelenp [nonwordpos] = TLM_process_word
	  (text, nonword, nonword_model, nonword_context, nonchar_model, nonchar_context,
		      coding_type, NIL, nonword_table, FALSE);
      if (state_type == TLM_comprehensive_stateing)
	  coderangesp [nonwordpos] = TLM_Coderanges;

      codelenp [wordpos] = TLM_process_word
	  (text, word, word_model, word_context, char_model, char_context, coding_type, NIL,
	   word_table, eof);
      if (state_type == TLM_comprehensive_stateing)
	  coderangesp [wordpos] = TLM_Coderanges;

      if (eof)
	  break;
    }
    TLM_release_context (word_model, word_context);
    TLM_release_context (char_model, char_context);
    TLM_release_context (nonword_model, nonword_context);
    TLM_release_context (nonchar_model, nonchar_context);

    TXT_release_text (word);
    TXT_release_text (nonword);

    return (state);
}

void
TLM_get_state (unsigned int state, unsigned int pos, float *codelength)
/* Returns the codelength associated with the state at position pos. The state
   can be constructed using either TLM_make_state () or TLM_make_words_state (). */
{
    assert (TLM_valid_state (state));

    assert (state != NIL);
    assert (pos < States [state].State_length);

    *codelength = States [state].State_codelength [pos];
}

void
TLM_get_coderanges_state (unsigned int state, unsigned int pos,
			  unsigned int *coderanges)
/* Returns the list of coderanges associated with the state at position pos. The state
   can be constructed using either TLM_make_state () or TLM_make_words_state (). */
{
    assert (TLM_valid_state (state));
    assert (States [state].State_type == TLM_comprehensive_stateing);
    assert (pos < States [state].State_length);

    *coderanges = States [state].State_coderanges [pos];
}

