/* Test program. */
#include <stdio.h>
#include <stdlib.h>

#ifdef SYSTEM_LINUX
#include <getopt.h> /* for getopt on Linux systems */
#endif

#include "io.h"
#include "index.h"

int
main ()
{
    unsigned int index, value1;
    int pos, value;

    index = TXT_create_index ();
    for (;;)
      {
	printf ("position? ");
	scanf ("%d", &pos);

	printf ("value? ");
	scanf ("%d", &value);

	TXT_put_index (index, pos, value);

	if (TXT_get_index (index, pos, &value1))
	    fprintf (stderr, "Found, value = %d\n", value1);
	else
	    fprintf (stderr, "Error - not found\n");
	    
	TXT_dump_index (Stdout_File, index, NULL);
      }
}

