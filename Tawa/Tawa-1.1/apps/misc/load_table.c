/* Loads a table from a text file and writes it out. */
#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <assert.h>

#ifdef SYSTEM_LINUX
#include <getopt.h> /* for getopt on Linux systems */
#endif

#include "io.h"
#include "model.h"
#include "text.h"
#include "table.h"

int
main()
{
    unsigned int table, key, key_id, key_count;

    key = TXT_create_text ();
    table = TXT_create_table (TLM_Dynamic, 0);

    while (TXT_readline_text (Stdin_File, key) > 0)
    {
	TXT_update_table (table, key, &key_id, &key_count);

	/*
	printf ("Id number for key ");
	TXT_dump_text (Stdout_File, key, TXT_dump_symbol);
	printf ( " = %d, count = %d\n", key_id, key_count);
	printf ("\nDump of table:\n\n");
	TXT_dump_table (Stdout_File, table);
	*/
    }

    /*
    printf ("\nTesting TXT_next_table routine:\n");
    TXT_reset_table (table);
    while (TXT_next_table (table, &key1, &key_id1, &key_count1))
      {
	printf ("key ");
	TXT_dump_text (Stdout_File, key1, TXT_dump_symbol);
	printf ( " = %d, count = %d\n", key_id1, key_count1);
      }
    */

    TXT_write_table (Stdout_File, table, TLM_Static);

    return (1);
}
