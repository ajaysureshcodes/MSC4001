/* For testing the rules module */
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "io.h"
#include "model.h"
#include "text.h"
#include "table.h"
#include "pcfg.h"

int main (int argc, char *argv [])
{
    unsigned int id_rule, addr_rule, email_rule, words_rule;
    unsigned int text;

    words_rule = PCFG_create_rule ("words");
    id_rule    = PCFG_create_rule ("id");
    addr_rule  = PCFG_create_rule ("addr");
    email_rule = PCFG_create_rule ("email");

    /*
    PCFG_define_rule (id_rule, "%b", TXT_is_alphanumeric);
    PCFG_define_rule (id_rule, "%b%r", TXT_is_alphanumeric, id_rule);
    PCFG_define_rule (addr_rule, "%r", id_rule);
    PCFG_define_rule (addr_rule, "%r.%r", id_rule, addr_rule);
    PCFG_define_rule (email_rule, "%r@%r", addr_rule, addr_rule);

    PCFG_define_rule (email_rule, "one");
    PCFG_define_rule (email_rule, "two");
    PCFG_define_rule (email_rule, "three");
    PCFG_define_rule (email_rule, "four");
    PCFG_define_rule (email_rule, "five");
    PCFG_define_rule (email_rule, "six");
    PCFG_define_rule (email_rule, "seven");
    PCFG_define_rule (email_rule, "eight");
    PCFG_define_rule (email_rule, "nine");
    PCFG_define_rule (email_rule, "ten");
    PCFG_define_rule (email_rule, "on");
    PCFG_define_rule (email_rule, "deux");
    PCFG_define_rule (email_rule, "trois");
    PCFG_define_rule (email_rule, "quatre");
    PCFG_define_rule (email_rule, "dwar");
    PCFG_define_rule (email_rule, "yedin");
    PCFG_define_rule (email_rule, "sixty");

    PCFG_define_rule (email_rule, "xx%b", TXT_is_vowel);
    PCFG_define_rule (email_rule, "xx%bx", TXT_is_vowel);
    PCFG_define_rule (email_rule, "xx%by", TXT_is_digit);

    TXT_dump_rule (Stderr_File, email_rule);
    */

    PCFG_define_rule (words_rule, "a");
    PCFG_define_rule (words_rule, "b");
    PCFG_define_rule (words_rule, "c");
    PCFG_define_rule (id_rule, "%b", TXT_is_alphanumeric);
    PCFG_define_rule (id_rule, "%b%r", TXT_is_alphanumeric, id_rule);
    PCFG_define_rule (addr_rule, "%r", words_rule);
    PCFG_define_rule (addr_rule, "%r", id_rule);
    PCFG_define_rule (addr_rule, "%r.%r", id_rule, addr_rule);

    PCFG_dump_rule (Stderr_File, addr_rule);

    text = TXT_create_text ();

    for (;;)
      {
	printf ("Enter text to parse: ");
	TXT_readline_text (Stdin_File, text);

	PCFG_parse_text (text, addr_rule);
      }

    return (1);
}
