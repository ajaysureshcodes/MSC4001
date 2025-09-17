/* Test program for pre-processing an XML file. */
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
    unsigned int table_file, line, spos, spos1, epos, epos1, spos2;
    boolean invalid;

    key = TXT_create_text ();
    line = TXT_create_text ();
    table = TXT_create_table (TLM_Dynamic, 0);

    table_file = TXT_open_file ("junk.table", "r", NULL,
				"Test: can't open table file");

    while (TXT_readline_text (table_file, key) > 0)
    {
	TXT_update_table (table, key, &key_id, &key_count);
	printf ("Id number for key ");
	TXT_dump_text (Stdout_File, key, TXT_dump_symbol);
	printf ( " = %d, count = %d\n", key_id, key_count);
    }

    printf ("\nDump of table:\n\n");
    TXT_dump_table (Stdout_File, table);

    while (TXT_readline_text (Stdin_File, line) > 0)
    {
      /* Parse the xml line: */
      spos = 0;
      epos = 0;
      spos1 = 0;
      epos1 = 0;
      spos2 = 0;
      invalid = !TXT_find_symbol (line, '<', 0, &spos);
      if (!invalid)
	  invalid = !TXT_find_symbol (line, '>', spos + 1, &epos);

      if (!invalid)
	{
	  if (!TXT_find_symbol (line, '<', epos + 1, &spos1))
	    spos1 = 0;
	  else
	    {
	      invalid = !TXT_find_symbol (line, '>', spos1 + 1, &epos1);
	      if (!invalid) /* Check for presence of third tag */
		  invalid = TXT_find_symbol (line, '<', epos1 + 1, &spos2);
	    }
	}

      if (invalid)
	{
	  fprintf (stderr, "Fatal error: parsing error found in line ");
	  TXT_dump_text (Stderr_File, line, NULL);
	  fprintf (stderr, "\n");
	  exit (2);
	}

      /*
      TXT_dump_text (Stdout_File, line, TXT_dump_symbol);
      fprintf (stdout, " found at (%d,%d) and possibly at (%d,%d)\n",
	       spos, epos, spos1, epos1);
      */
    }

    return (1);
}
