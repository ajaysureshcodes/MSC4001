/* Test program. */
#include <stdio.h>
#include <stdlib.h>

#ifdef SYSTEM_LINUX
#include <getopt.h> /* for getopt on Linux systems */
#endif

#include "io.h"
#include "list.h"

int
main ()
{
    unsigned int list, number, freq, freq1;

    TXT_init_files ();

    list = TXT_create_list ();
    for (;;)
      {
	printf ("number? ");
	scanf ("%d", &number);

	printf ("freq? ");
	scanf ("%d", &freq);

	TXT_put_list (list, number, freq);

	freq1 = TXT_get_list (list, number);
	if (freq1 != 0)
	    fprintf (stderr, "Found, freq = %d\n", freq1);
	else
	    fprintf (stderr, "Error - not found\n");
	    
	TXT_dump_list (Stdout_File, list);
      }
}

