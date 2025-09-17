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
    unsigned int table;

    table = TXT_load_table (Stdin_File);
    TXT_dump_table (Stdout_File, table);
}
