/* Definitions of various structures for probabilistic context-free
   grammars. */

#ifndef PCFGS_H
#define PCFGS_H

unsigned int
PCFG_create_rule (char *label);
/* Creates a new PCFG, and associates a label with it (the label
   is primarily used for debugging purposes). */

void
PCFG_define_rule (unsigned int pcfg_rule, char *format, ...);
/* Associates the rhs format with the rule for a probabilistic generative
   grammar (PCFG). The rhs of the PCFG rule is set to the symbols specified
   by the format and variable length argument list. Meaning of the formatting
   characters:

         %%    - the % (percentage) character is inserted into the text.
         %s    - the argument list contains a symbol number (unsigned int)
	         which will be inserted (as terminal symbols) into the text.
	 %b    - the argument list contains a pointer to a boolean function
	         that takes an unsigned int symbol number as its only argument
		 (which will be set to the current context symbol number when
		 matching the PCFG rule) and returns a non-zero value if the
                 current symbol number is a valid match.

		 format: [boolean (*function) (unsigned int symbol)]  
                 example: is_punct (symbol)

	 %l    - specifies a lhs of a PCFG rule.
         %$    - the sentinel symbol is inserted into the text.
*/

void
PCFG_dump_rule (unsigned int file, unsigned int pcfg_rule);
/* Dumps the PCFG rule (for debugging purposes). */

unsigned int
PCFG_parse_text (unsigned int text, unsigned int pcfg_rule);
/* Parses the text using the PCFG grammar, and returns the
   result in a new text record. */

#endif
