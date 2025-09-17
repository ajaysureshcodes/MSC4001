/* Definitions of various structures for rules (used for
   defining a context-free grammar. */

#ifndef RULES_H
#define RULES_H

unsigned int
TXT_create_rule (char *label);
/* Creates a new rule, and associates a label with it (the label
   is primarily used for debugging purposes). */

void
TXT_define_rule (unsigned int rule, char *format, ...);
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

	 %l    - specifies a lhs of a rule.
         %$    - the sentinel symbol is inserted into the text.
*/

void
TXT_dump_rule (unsigned int file, unsigned int rule);
/* Dumps the rule (for debugging purposes). */

unsigned int
TXT_parse_text (unsigned int text, unsigned int rule);
/* Parses the text using the grammar rule, and returns the
   result in a new text record. */

#endif
