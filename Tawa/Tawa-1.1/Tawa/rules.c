/* Definitions of various structures for rules (used for
   defining a context-free grammar. */

#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#ifdef SYSTEM_LINUX
#include <getopt.h> /* for getopt on Linux systems */
#endif

#define RULE_SYMBOL_TYPE 0      /* Specifies that the rule contains a
				   terminal symbol at this point. */
#define RULE_NONTERMINAL_TYPE 1 /* Specifies that the rule contains a
				   non-terminal symbol at this point. */
#define RULE_GENERATION_TYPE 2  /* Specifies that the rule contains a table
				   of generation symbols which will replace
				   the current rhs ("right-hand symbol")
				   at this point. */
#define RULE_BOOLEAN_TYPE 3     /* Specifies that the rule contains a boolean
				   function symbol at this point. */

#include "model.h"
#include "io.h"
#include "bits.h"
#include "table.h"
#include "index.h"
#include "text.h"
#include "table.h"
#include "rules.h"

char **Rule_labels = NULL;
unsigned int Rule_labels_alloc = 0;

unsigned int Sentinel1_Symbol = NIL;

boolean
get_rhs (unsigned int rhs, unsigned int pos, unsigned int *rhs_type,
	 unsigned int *rhs_symbol, unsigned int *new_pos)
/* Gets the type and symbol from the right hand side (rhs) of the rule. */
{
    unsigned int symbol, length, rhstype;

    *new_pos = pos;
    *rhs_type = RULE_SYMBOL_TYPE;
    *rhs_symbol = NIL;
    
    if (!TXT_get_symbol (rhs, pos++, &symbol))
        return (FALSE);

    if (symbol == Sentinel1_Symbol)
      {
	assert (TXT_get_symbol (rhs, pos++, &length));
	if (length != 0) /* length = 0 means sentinel symbol found */
	  {
	    assert (length == 2);
	    assert (TXT_get_symbol (rhs, pos++, &rhstype));
	    assert ((rhstype == RULE_NONTERMINAL_TYPE) ||
		    (rhstype == RULE_GENERATION_TYPE) ||
		    (rhstype == RULE_BOOLEAN_TYPE));
	    *rhs_type = rhstype;
	    assert (TXT_get_symbol (rhs, pos++, &symbol));
	  }
      }

    *new_pos = pos;
    *rhs_symbol = symbol;

    return (TRUE);
}

unsigned int
TXT_create_rule (char *label)
/* Creates a new rule, and associates a label with it (the label
   is primarily used for debugging purposes). */
{
    unsigned int rule, p;

    if (Sentinel1_Symbol == NIL)
        Sentinel1_Symbol = TXT_sentinel1_symbol ();

    rule = TXT_create_table (TLM_Dynamic, 0);
    if (rule >= Rule_labels_alloc)
      { /* extend array */
        Rule_labels = (char **)
	    Realloc (130, Rule_labels, (rule + 1) * sizeof (char *),
		     Rule_labels_alloc * sizeof (char *));

	/* Initialize all potentially new label pointers to NULL */
	for (p = Rule_labels_alloc; p < rule + 1; p++)
	    Rule_labels [p] = NULL;

	Rule_labels_alloc = rule + 1;
      }
    Rule_labels [rule] = (char *)
        Malloc (131, (strlen (label) + 1) * sizeof (char));
    strcpy (Rule_labels [rule], label);

    return (rule);
}

boolean
valid_rule (unsigned int parent_rule, unsigned int rule);
/* (Recursively) Checks if the rule is valid. */

boolean
valid_rhs (unsigned int parent_rule, unsigned int rule, unsigned int rhs)
/* Returns TRUE if the rhs is valid. */
{
    boolean rule_exists, rule_valid;
    unsigned int rhs_symbol, rhs_length, rhs_type, rhs_pos, new_rhs_pos;

    assert (TXT_valid_text (rhs));

    /*
      fprintf (stderr, "checking valid rhs %d for parent rule %d\n",
               rhs, parent_rule);
    */

    rhs_pos = 0;
    rhs_length = TXT_length_text (rhs);
    for (;;)
      {
	if (!get_rhs (rhs, rhs_pos, &rhs_type, &rhs_symbol, &new_rhs_pos))
	    break; /* no more symbols to process */

	switch (rhs_type)
	  {
	  case RULE_NONTERMINAL_TYPE:
	    rule_exists = TXT_valid_table (rhs_symbol);
	    assert (rule_exists);

	    rule_valid = (rhs_symbol != parent_rule) ||
	        (new_rhs_pos >= rhs_length);
	    assert (rule_valid);

	    if ((rhs_symbol != parent_rule) && (rhs_symbol != rule) &&
		!valid_rule (parent_rule, rhs_symbol))
	        return (FALSE);
	    break;
	  case RULE_GENERATION_TYPE:
	    break;
	  case RULE_BOOLEAN_TYPE:
	    break;
	  default:
	    assert ((rhs_type == RULE_SYMBOL_TYPE) ||
		    (rhs_type == RULE_NONTERMINAL_TYPE) ||
		    (rhs_type == RULE_GENERATION_TYPE) ||
		    (rhs_type == RULE_BOOLEAN_TYPE));
	    break;
	  }
	rhs_pos = new_rhs_pos;
      }

    return (TRUE); /* the rhs is valid */
}

boolean
valid_rule (unsigned int parent_rule, unsigned int rule)
/* (Recursively) Checks if the rule is valid. */
{
    unsigned int rhs, rhs_id, rhs_count;

    assert (TXT_valid_table (rule));

    /*
      fprintf (stderr, "checking valid rule %d for parent rule %d\n",
               rule, parent_rule);
    */

    TXT_reset_table (rule);
    while (TXT_next_table (rule, &rhs, &rhs_id, &rhs_count))
        if (!valid_rhs (parent_rule, rule, rhs))
	    return (FALSE);
    return (TRUE);
}

void
define_rhs_type (unsigned int rhs_type, unsigned int rhs,
		 unsigned int rhs_symbol)
/* Inserts the rhs_symbol and its type into the rhs of the rule. */
{
    TXT_append_symbol (rhs, Sentinel1_Symbol);
    TXT_append_symbol (rhs, 2); /* means 2 more symbols follow */
    TXT_append_symbol (rhs, rhs_type);
    TXT_append_symbol (rhs, rhs_symbol);
}

void
TXT_define_rule (unsigned int rule, char *format, ...)
/* Associates the rhs format with the rule for a context-free grammar.
   The rhs of the rule is set to the symbols specified by the format and
   variable length argument list. Meaning of the formatting characters:

         %%    - the % (percentage) character is inserted into the text.
         %s    - the argument list contains a symbol number (unsigned int)
	         which will be inserted (as terminal symbols) into the text.
	 %b    - the argument list contains a pointer to a boolean function
	         that takes an unsigned int symbol number as its only argument
		 (which will be set to the current context symbol number when
		 matching the rule) and returns a non-zero value if the
                 current symbol number is a valid match.

		 format: [boolean (*function) (unsigned int symbol)]  
                 example: is_punct (symbol)

	 %r    - specifies a rule.
	 %l    - specifies a list of replacement symbols.
         %$    - the sentinel symbol is inserted into the text.
*/
{
    va_list args; /* points to each unnamed argument in turn */
    unsigned int rhs, rhs_id, rhs_count, p;
    unsigned int symbol, arg_symbol, arg_table, arg_function;
    boolean percent_found;

    assert (TXT_valid_table (rule) && (rule < Rule_labels_alloc) &&
	    (Rule_labels [rule] != NULL));

    rhs = TXT_create_text ();

    va_start (args, format);

    /* process the format */
    percent_found = FALSE;
    for (p = 0; p < strlen (format); p++)
      {
	symbol = format [p];
	if (!percent_found)
	  {
	    if (symbol == '%')
	        percent_found = TRUE;
	    else
		TXT_append_symbol (rhs, symbol);
	    continue;
	  }
	switch (symbol)
	  {
	  case '%':
	    TXT_append_symbol (rhs, '%');
	    break;
	  case 's':
	    arg_symbol = va_arg (args, unsigned int);
	    TXT_append_symbol (rhs, arg_symbol);
	    break;
	  case 'r': /* indicates rule symbol follows */
	    arg_table = va_arg (args, unsigned int);
	    assert (TXT_valid_table (arg_table));
	    define_rhs_type (RULE_NONTERMINAL_TYPE, rhs, arg_table);
	    break;
	  case 't': /* indicates generation symbol follows */
	    arg_table = va_arg (args, unsigned int);
	    assert (TXT_valid_table (arg_table));
	    define_rhs_type (RULE_GENERATION_TYPE, rhs, arg_table);
	    break;
	  case 'b': /* indicates special symbol follows; then
		       store function pointer as an unsigned int to be
		       recast later */
	    arg_function = va_arg (args, unsigned int);
	    define_rhs_type (RULE_BOOLEAN_TYPE, rhs, arg_function);
	    break;
	  case '$':
	    TXT_append_symbol (rhs, Sentinel1_Symbol);
	    TXT_append_symbol (rhs, 0); /* means no more symbols follow */
	    break;
	  default:
	    /* error */
	    assert ((symbol == '%') || (symbol == 's') || (symbol == '$') ||
		    (symbol == 'b') || (symbol == 'r') || (symbol == 'l'));
	    break;
	  }
	percent_found = FALSE;
      }

    va_end (args);

    TXT_update_table (rule, rhs, &rhs_id, &rhs_count);

    assert (valid_rule (rule, rule));
}

void
dump_rule (unsigned int file, unsigned int rule);
/* (Recursively) Dumps the rule. */

void
dump_rhs (unsigned int file, unsigned int rule, unsigned int rhs)
/* (Recursively) Dumps the rhs of the rule. */
{
    unsigned int rhs_symbol, rhs_type, rhs_pos, new_rhs_pos;

    assert (TXT_valid_file (file));
    assert (TXT_valid_text (rhs));

    fprintf (Files [file], "%s [%d] --> ", Rule_labels [rule], rule);

    /* process the rhs to print it out */
    rhs_pos = 0;
    for (;;)
      {
	if (!get_rhs (rhs, rhs_pos, &rhs_type, &rhs_symbol, &new_rhs_pos))
	    break; /* no more symbols to process */

	switch (rhs_type)
	  {
	  case RULE_SYMBOL_TYPE:
	    if (rhs_symbol < Sentinel1_Symbol)
	        TXT_dump_symbol (file, rhs_symbol);
	    else if (rhs_symbol == Sentinel1_Symbol)
	        fprintf (Files [file], "<sentinel>");
	    break;
	  case RULE_NONTERMINAL_TYPE:
	    fprintf (Files [file], "<%s>[%d]", Rule_labels [rhs_symbol],
		     rhs_symbol);
	    break;
	  case RULE_GENERATION_TYPE:
	    fprintf (Files [file], "{%s}[%d]", Rule_labels [rhs_symbol],
		     rhs_symbol);
	    break;
	  case RULE_BOOLEAN_TYPE:
	    fprintf (Files [file], "<boolean function:%d>", rhs_symbol);
	    break;
	  default:
	    assert ((rhs_type == RULE_SYMBOL_TYPE) ||
		    (rhs_type == RULE_NONTERMINAL_TYPE) ||
		    (rhs_type == RULE_GENERATION_TYPE) ||
		    (rhs_type == RULE_BOOLEAN_TYPE));
	    break;
	  }
	rhs_pos = new_rhs_pos;
      }
    fprintf (Files [file], "\n");

    /* process the rhs again to recursively process any rules the
       rhs refers to */
    rhs_pos = 0;
    for (;;)
      {
	if (!get_rhs (rhs, rhs_pos, &rhs_type, &rhs_symbol, &new_rhs_pos))
	    break; /* no more symbols to process */
	if ((rhs_type == RULE_NONTERMINAL_TYPE) ||
	    (rhs_type == RULE_GENERATION_TYPE))
	    dump_rule (file, rhs_symbol);
	rhs_pos = new_rhs_pos;
      }
}

bits_type *RULE_set;

void
dump_rule (unsigned int file, unsigned int rule)
/* (Recursively) Dumps the rule. */
{
    unsigned int rhs, rhs_id, rhs_count;

    assert (TXT_valid_file (file));

    if (BITS_ISSET (RULE_set, rule))
      return; /* already dumped this set */
    BITS_SET (RULE_set, rule);

    assert (TXT_valid_table (rule));

    TXT_reset_table (rule);
    while (TXT_next_table (rule, &rhs, &rhs_id, &rhs_count))
        dump_rhs (file, rule, rhs);
}

void
TXT_dump_rule (unsigned int file, unsigned int rule)
/* Dumps the rule (for debugging purposes). */
{

    assert (TXT_valid_file (file));

    BITS_ALLOC (RULE_set);
    BITS_ZERO (RULE_set);

    dump_rule (file, rule);
}

unsigned int Parse_Cache = NIL;
unsigned int Parse_Cache_Text = NIL;
unsigned int Parse_Cache_Data = NIL;

void
set_parse_cache_text (unsigned int rule, unsigned int rhs,
		      unsigned int text_pos)
/* For setting the text to be inserted/searchd for in the parse cache. */
{
    TXT_setlength_text (Parse_Cache_Text, 0);
    TXT_append_symbol (Parse_Cache_Text, rule);
    TXT_append_symbol (Parse_Cache_Text, rhs);
    TXT_append_symbol (Parse_Cache_Text, text_pos);
}

boolean
parse_rule (unsigned int rule, unsigned int level,
	    unsigned int text, unsigned int text_pos,
	    unsigned int *new_text_pos);
/* Parses the text using the rule table. */

boolean
parse_rhs (unsigned int rule, unsigned int level,
	   unsigned int rule_pos, unsigned int rhs,
	   unsigned int text, unsigned int text_pos,
	   unsigned int *new_text_pos)
/* Parses the text using the rhs (right hand side) of the rule. */
{
    unsigned int rhs_symbol, rhs_type, text_symbol;
    unsigned int old_text_pos, this_text_pos, rhs_pos, new_rhs_pos;
    unsigned int key_id, key_count, key_data;
    boolean (*boolean_function)  (unsigned int);

    assert (TXT_valid_text (rhs));

    *new_text_pos = text_pos;

    /*
      fprintf (stderr, "Matching rhs %d\n", rhs);
      dump_rhs (stderr, rule, rhs);
    */

    /* Check the cache first */
    set_parse_cache_text (rule, rhs, text_pos);
    if (TXT_getid_table (Parse_Cache, Parse_Cache_Text, &key_id, &key_count))
      { /* found this (rule, rhs, text_pos) tuple in the table */
	assert (TXT_get_index (Parse_Cache_Data, key_id, &key_data));
	*new_text_pos = key_data; /* the key data contains the text pos */
	return (TRUE);
      }

    old_text_pos = text_pos; /* save the text position */
    this_text_pos = text_pos + 1; /* make sure that this position differs from
				    current position */
    for (rhs_pos = rule_pos;;)
      {
	if (!get_rhs (rhs, rhs_pos, &rhs_type, &rhs_symbol, &new_rhs_pos))
	    break; /* no more symbols to process */

	if (text_pos != this_text_pos)
	  { /* get next text symbol if the position has changed */
	    if (!TXT_get_symbol (text, text_pos, &text_symbol))
	        return (FALSE);
	    this_text_pos = text_pos;
	  }

	switch (rhs_type)
	  {
	  case RULE_SYMBOL_TYPE:
	    if (text_symbol != rhs_symbol)
	        return (FALSE);
	    text_pos++;
	    break;
	  case RULE_NONTERMINAL_TYPE: /* Recursively match the rule */
	    if (!parse_rule (rhs_symbol, level+1, text, text_pos,
			     new_text_pos))
	        return (FALSE);
	    text_pos = *new_text_pos;
	    break;
	  case RULE_BOOLEAN_TYPE: /* match the boolean function */
	    boolean_function = (boolean (*) (unsigned int)) rhs_symbol;
	    if (!boolean_function (text_symbol))
	        return (FALSE);
	    text_pos++;
	    break;
	  default:
	    assert ((rhs_type == RULE_SYMBOL_TYPE) ||
		    (rhs_type == RULE_NONTERMINAL_TYPE) ||
		    (rhs_type == RULE_BOOLEAN_TYPE));
	    break;

	  }
	rhs_pos = new_rhs_pos;
      }

    *new_text_pos = text_pos;

    /* Insert into cache so it won't get parsed again */
    set_parse_cache_text (rule, rhs, old_text_pos);
    TXT_put_index (Parse_Cache_Data, key_id, text_pos); /* *** This needs to be checked */

    return (TRUE); /* the rhs matches the text */
}

boolean
parse_rulepos (unsigned int rule, unsigned int level,
	       unsigned int rule_pos, unsigned int rule_tablepos,
	       unsigned int text, unsigned int text_pos,
	       unsigned int *new_text_pos)
/* Parses the text using the rule table. */
{
    unsigned int rhs, rhs_id, rhs_count, rhs_type, text_symbol;
    unsigned int rhs_symbol, rhs_symbol1, rhs_symbols;
    unsigned this_text_pos, next_text_pos, greatest_text_pos;
    unsigned int expand_tablepos, new_rule_pos;
    boolean (*boolean_function)  (unsigned int);
    boolean ok, rhs_ok, expand_ok, match_found;

    *new_text_pos = text_pos;

    /*
    fprintf (stderr, "parsing rule %d rule_pos %d text pos %d\n",
	     rule, rule_pos, text_pos);
    */

    if ((rule == NIL) || (rule_tablepos == NIL))
        return (FALSE);

    ok = FALSE;
    greatest_text_pos = text_pos;
    this_text_pos = text_pos + 1; /* make sure that this position differs from
				    current position */
    while (rule_tablepos != NIL)
      {
	if (text_pos != this_text_pos)
	  { /* get next text symbol if the position has changed */
	    if (!TXT_get_symbol (text, text_pos, &text_symbol))
	        return (FALSE);
	    this_text_pos = text_pos;
	  }

	TXT_get_tablepos (rule, rule_tablepos, &rhs, &rhs_id, &rhs_count,
			  &rhs_symbol, &rhs_symbols);
	/*
	fprintf (stderr, "rule = %d tablepos = %d (tnode ptr %p) rhs = %d rhs_id = %d rhs_count = %d rhs_symbol = ",
		 rule, rule_tablepos, (struct table_trie_type *) rule_tablepos,
		 rhs, rhs_id, rhs_count);
	TXT_dump_symbol (Stderr_File, rhs_symbol);
	fprintf (stderr, "\n");
	*/

	if (rhs_symbols == NIL)
	  {
	    rhs_type = RULE_SYMBOL_TYPE;
	    new_rule_pos = rule_pos + 1;
	  }
	else
	  {
	    assert (TXT_length_text (rhs_symbols) == 2);
	    TXT_get_symbol (rhs_symbols, 0, &rhs_type);
	    TXT_get_symbol (rhs_symbols, 1, &rhs_symbol1);
	    new_rule_pos = rule_pos + 4;
	  }

	match_found = FALSE;
	next_text_pos = text_pos + 1;
	switch (rhs_type)
	  {
	  case RULE_SYMBOL_TYPE:
	    if (text_symbol == rhs_symbol)
	        match_found = TRUE;
	    break;
	  case RULE_NONTERMINAL_TYPE:
	    if (parse_rule (rhs_symbol1, level+1, text, text_pos,
			    &next_text_pos))
	        match_found = TRUE;
	    break;
	  case RULE_BOOLEAN_TYPE:
	    boolean_function = (boolean (*) (unsigned int)) rhs_symbol1;
	    if (boolean_function (text_symbol))
	        match_found = TRUE;
	    break;
	  default:
	    assert ((rhs_type == RULE_SYMBOL_TYPE) ||
		    (rhs_type == RULE_NONTERMINAL_TYPE) ||
		    (rhs_type == RULE_BOOLEAN_TYPE));
	    break;
	  }

	if (match_found)
	  { /* symbols match */
	    rhs_ok = (rhs != NIL) &&
	      parse_rhs (rule, level, new_rule_pos, rhs, text, next_text_pos,
			 &this_text_pos);
	    if (rhs_ok && (this_text_pos > greatest_text_pos))
	        greatest_text_pos = this_text_pos;
	    ok = ok || rhs_ok;

	    expand_tablepos = TXT_expand_tablepos (rule, rule_tablepos);
	    expand_ok = (expand_tablepos != NIL) &&
	      parse_rulepos (rule, level, new_rule_pos, expand_tablepos,
			     text, next_text_pos, &this_text_pos);
	    if (expand_ok && (this_text_pos > greatest_text_pos))
	        greatest_text_pos = this_text_pos;
	    ok = ok || expand_ok;
	  }

	rule_tablepos = TXT_next_tablepos (rule, rule_tablepos);

      }

    if (ok && (level == 0) && (rule_pos == 0))
        fprintf (stderr, "*** rule %d at pos %d new pos %d\n",
		 rule, text_pos, greatest_text_pos);
    *new_text_pos = greatest_text_pos;
    return (ok);
}

boolean
parse_rule (unsigned int rule, unsigned int level,
	    unsigned int text, unsigned int text_pos,
	    unsigned int *new_text_pos)
/* Parses the text using the rule table. */
{
    unsigned int rule_tablepos;
    boolean ok;

    rule_tablepos = TXT_reset_tablepos (rule);
    ok = parse_rulepos (rule, level, 0, rule_tablepos, text, text_pos,
			new_text_pos);
    return (ok);
}

unsigned int
TXT_parse_text (unsigned int text, unsigned int rule)
/* Parses the text using the grammar rule table, and returns the
   result in a new text record. */
{
    unsigned int parse_text, textlen, pos, start_pos, end_pos;

    assert (TXT_valid_text (text));
    assert (TXT_valid_table (rule));

    parse_text = TXT_create_text ();
    Parse_Cache = TXT_create_table (TLM_Dynamic, 0);
    Parse_Cache_Text = TXT_create_text ();
    Parse_Cache_Data = TXT_create_index ();

    textlen = TXT_length_text (text);
    for (pos = 0; pos < textlen; pos++)
      {
	start_pos = pos;
	parse_rule (rule, 0, text, start_pos, &end_pos);

	/* No need to do this:
	while (start_pos < textlen)
	  {
	    parse_rule (rule, 0, text, start_pos, &end_pos);
	    start_pos = end_pos + 1;
	  }
	*/
      }
  
    TXT_release_table (Parse_Cache); 
    TXT_release_text  (Parse_Cache_Text);
    TXT_release_index (Parse_Cache_Data); 

    return (parse_text);
}

