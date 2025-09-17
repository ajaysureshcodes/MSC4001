/* For testing the rules module */
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "io.h"
#include "model.h"
#include "text.h"
#include "table.h"
#include "pgg.h"

int main (int argc, char *argv [])
{
    unsigned int id_rule, addr_rule, email_rule, words_rule;
    unsigned int text;

    words_rule = PGG_create_rule ("words");
    id_rule    = PGG_create_rule ("id");
    addr_rule  = PGG_create_rule ("addr");
    email_rule = PGG_create_rule ("email");

    /*
    PGG_define_rule (id_rule, "%b", TXT_is_alphanumeric);
    PGG_define_rule (id_rule, "%b%r", TXT_is_alphanumeric, id_rule);
    PGG_define_rule (addr_rule, "%r", id_rule);
    PGG_define_rule (addr_rule, "%r.%r", id_rule, addr_rule);
    PGG_define_rule (email_rule, "%r@%r", addr_rule, addr_rule);

    PGG_define_rule (email_rule, "one");
    PGG_define_rule (email_rule, "two");
    PGG_define_rule (email_rule, "three");
    PGG_define_rule (email_rule, "four");
    PGG_define_rule (email_rule, "five");
    PGG_define_rule (email_rule, "six");
    PGG_define_rule (email_rule, "seven");
    PGG_define_rule (email_rule, "eight");
    PGG_define_rule (email_rule, "nine");
    PGG_define_rule (email_rule, "ten");
    PGG_define_rule (email_rule, "on");
    PGG_define_rule (email_rule, "deux");
    PGG_define_rule (email_rule, "trois");
    PGG_define_rule (email_rule, "quatre");
    PGG_define_rule (email_rule, "dwar");
    PGG_define_rule (email_rule, "yedin");
    PGG_define_rule (email_rule, "sixty");

    PGG_define_rule (email_rule, "xx%b", TXT_is_vowel);
    PGG_define_rule (email_rule, "xx%bx", TXT_is_vowel);
    PGG_define_rule (email_rule, "xx%by", TXT_is_digit);

    TXT_dump_rule (Stderr_File, email_rule);
    */

    PGG_define_rule (words_rule, "a");
    PGG_define_rule (words_rule, "b");
    PGG_define_rule (words_rule, "c");
    PGG_define_rule (id_rule, "%b", TXT_is_alphanumeric);
    PGG_define_rule (id_rule, "%b%r", TXT_is_alphanumeric, id_rule);
    PGG_define_rule (addr_rule, "%r", words_rule);
    PGG_define_rule (addr_rule, "%r", id_rule);
    PGG_define_rule (addr_rule, "%r.%r", id_rule, addr_rule);

    PGG_dump_rule (Stderr_File, addr_rule);

    text = TXT_create_text ();

    for (;;)
      {
	printf ("Enter text to parse: ");
	TXT_readline_text (Stdin_File, text);

	PGG_parse_text (text, addr_rule);
      }

    return (1);
}
