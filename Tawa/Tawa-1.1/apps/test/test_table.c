/* Test program. */
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

void
dump_tablepos1 (unsigned int file, unsigned int table, unsigned int tablepos)
/* Dumps out the tablepos routines. */
{
    unsigned int key, key_id, key_count, key_symbol, key_symbols, expandpos;
    char line [64];

    while (tablepos != NIL)
      {
	TXT_get_tablepos (table, tablepos, &key, &key_id, &key_count,
			  &key_symbol, &key_symbols);
	TXT_write_file (file, "symbol ");
	TXT_dump_symbol (file, key_symbol);
	if (key_symbols != NIL)
	  {
	    TXT_write_file (file, "symbols ");
	    TXT_dump_text (file, key_symbols, TXT_dump_symbol);
	  }
	TXT_write_file (file, "\n");

	if (key != NIL)
	  {
	    sprintf (line, "id %5d count %5d key = {", key_id, key_count);
	    TXT_write_file (file, line);
	    TXT_dump_text (file, key, TXT_dump_symbol);
	    TXT_write_file (file, "}\n");
	  }
	expandpos = TXT_expand_tablepos (table, tablepos);
	if (expandpos != NIL)
	    dump_tablepos1 (file, table, expandpos);
	tablepos = TXT_next_tablepos (table, tablepos);
      }
}

void
dump_tablepos (unsigned int file, unsigned int table)
/* Dumps out the table using tablepos routines. */
{
    unsigned int tablepos;

    assert (TXT_valid_file (file));

    tablepos = TXT_reset_tablepos (table);
    dump_tablepos1 (file, table, tablepos);
}

int
main()
{
    unsigned int table, key, key_id, key_count;
    unsigned int key1, key_id1, key_count1;
    unsigned int file, file1;

    key = TXT_create_text ();
    table = TXT_create_table (TLM_Dynamic, 0);

    TXT_append_string (key, "fred");
 
    TXT_set_file (File_Ignore_Errors);
    file = TXT_open_file ("junk.table", "r", NULL,
			   "Test: can't open table file");
    if (file)
      {
	table = TXT_load_table (file);
	TXT_dump_table (Stdout_File, table);
      }

    printf ("Input key: ");
    while (TXT_readline_text (Stdin_File, key) > 0)
    {
	TXT_update_table (table, key, &key_id, &key_count);
	printf ("Id number for key ");
	TXT_dump_text (Stdout_File, key, TXT_dump_symbol);
	printf ( " = %d, count = %d\n", key_id, key_count);
	printf ("\nDump of table:\n\n");
	TXT_dump_table (Stdout_File, table);
	printf ("\nInput key: ");
    }

    printf ("\nTesting TXT_next_table routine:\n");
    TXT_reset_table (table);
    while (TXT_next_table (table, &key1, &key_id1, &key_count1))
      {
	printf ("key ");
	TXT_dump_text (Stdout_File, key1, TXT_dump_symbol);
	printf ( " = %d, count = %d\n", key_id1, key_count1);
      }

    printf ("\nTesting tablepos routines:\n");
    dump_tablepos (Stdout_File, table);

    TXT_set_file (File_Ignore_Errors);
    file1 = TXT_open_file ("junk.table.1", "w", NULL,
			   "Test: can't open table file");
    TXT_write_table (file1, table, TLM_Dynamic);

    return (1);
}
