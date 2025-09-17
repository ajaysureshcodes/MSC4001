/* For testing the rules module */
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "io.h"
#include "model.h"
#include "text.h"
#include "table.h"
#include "rules.h"

int main (int argc, char *argv [])
{
    unsigned int id_rule, addr_rule, email_rule, words_rule;
    unsigned int text;

    words_rule = TXT_create_rule ("words");
    id_rule    = TXT_create_rule ("id");
    addr_rule  = TXT_create_rule ("addr");
    email_rule = TXT_create_rule ("email");

    /*
    TXT_define_rule (id_rule, "%b", TXT_is_alphanumeric);
    TXT_define_rule (id_rule, "%b%r", TXT_is_alphanumeric, id_rule);
    TXT_define_rule (addr_rule, "%r", id_rule);
    TXT_define_rule (addr_rule, "%r.%r", id_rule, addr_rule);
    TXT_define_rule (email_rule, "%r@%r", addr_rule, addr_rule);

    TXT_define_rule (email_rule, "one");
    TXT_define_rule (email_rule, "two");
    TXT_define_rule (email_rule, "three");
    TXT_define_rule (email_rule, "four");
    TXT_define_rule (email_rule, "five");
    TXT_define_rule (email_rule, "six");
    TXT_define_rule (email_rule, "seven");
    TXT_define_rule (email_rule, "eight");
    TXT_define_rule (email_rule, "nine");
    TXT_define_rule (email_rule, "ten");
    TXT_define_rule (email_rule, "on");
    TXT_define_rule (email_rule, "deux");
    TXT_define_rule (email_rule, "trois");
    TXT_define_rule (email_rule, "quatre");
    TXT_define_rule (email_rule, "dwar");
    TXT_define_rule (email_rule, "yedin");
    TXT_define_rule (email_rule, "sixty");

    TXT_define_rule (email_rule, "xx%b", TXT_is_vowel);
    TXT_define_rule (email_rule, "xx%bx", TXT_is_vowel);
    TXT_define_rule (email_rule, "xx%by", TXT_is_digit);

    TXT_dump_rule (Stderr_File, email_rule);
    */

    TXT_define_rule (words_rule, "a");
    TXT_define_rule (words_rule, "b");
    TXT_define_rule (words_rule, "c");
    TXT_define_rule (id_rule, "%b", TXT_is_alphanumeric);
    TXT_define_rule (id_rule, "%b%r", TXT_is_alphanumeric, id_rule);
    TXT_define_rule (addr_rule, "%r", words_rule);
    TXT_define_rule (addr_rule, "%r", id_rule);
    TXT_define_rule (addr_rule, "%r.%r", id_rule, addr_rule);

    TXT_dump_rule (Stderr_File, addr_rule);

    text = TXT_create_text ();

    for (;;)
      {
	printf ("Enter text to parse: ");
	TXT_readline_text (Stdin_File, text);

	TXT_parse_text (text, addr_rule);
      }

    return (1);
}
