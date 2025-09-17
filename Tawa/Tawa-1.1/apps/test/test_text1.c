/* Test program. */
#include <stdio.h>
#include <stdlib.h>

#ifdef SYSTEM_LINUX
#include <getopt.h> /* for getopt on Linux systems */
#endif

#include "io.h"
#include "model.h"
#include "text.h"
#include "transform.h"

int
main (int argc, char *argv[])
{
  unsigned int text, pos;
    boolean found;

    text = TXT_create_text ();
    TXT_append_string (text, "Now &quot; is the &quot; time");

    found = TXT_find_string (text, "Now", 0, &pos);
    printf ("found = %d pos = %d\n", found, pos);
    found = TXT_find_string (text, "&quot;", 0, &pos);
    printf ("found = %d pos = %d\n", found, pos);
    found = TXT_find_string (text, "is", 0, &pos);
    printf ("found = %d pos = %d\n", found, pos);
    found = TXT_find_string (text, "&quot;", 5, &pos);
    printf ("found = %d pos = %d\n", found, pos);
    found = TXT_find_string (text, "&quot;", 5, &pos);
    printf ("found = %d pos = %d\n", found, pos);
    found = TXT_find_string (text, "time;", 15, &pos);
    printf ("found = %d pos = %d\n", found, pos);
    found = TXT_find_string (text, "time", 15, &pos);
    printf ("found = %d pos = %d\n", found, pos);

    /*
    TXT_nullify_string (text, "&quot;");
    */
    TXT_overwrite_string (text, "&quot;", "\"");
    printf ("String = ");
    TXT_dump_text (Stdout_File, text, TXT_dump_symbol2);
    printf ("\n");

}
