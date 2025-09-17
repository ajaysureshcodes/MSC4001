/* Test program for loading a ctable. */
#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>

#ifdef SYSTEM_LINUX
#include <getopt.h> /* for getopt on Linux systems */
#endif

#include "io.h"
#include "text.h"
#include "model.h"
#include "pt_ctable.h"

void
usage (void)
{
    fprintf (stderr,
	     "Usage: ident_file [options]\n"
	     "\n"
	     "options:\n"
	);
    exit (2);
}

int
main (int argc, char *argv[])
{
    struct PTc_table_type *table;

    table = PTc_load_table (Stdin_File);

    PTc_dump_table (Stdout_File, table);

    exit (1);
}
