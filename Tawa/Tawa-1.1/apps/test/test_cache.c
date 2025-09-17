/* Program to test PPM_cache module. */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef SYSTEM_LINUX
#include <getopt.h> /* for getopt on Linux systems */
#endif

#include "io.h"
#include "ppm_cache.h"

#define MAX_ORDER 4

int Max_order = MAX_ORDER;

void
usage (void)
{
    fprintf (stderr,
	     "Usage: play [options] training-model <input-text\n"
	     "\n"
	     "options:\n"
	     "  -o n\tmax order=n\n"
	);
    exit (2);
}

void
init_arguments (int argc, char *argv[])
{
    extern char *optarg;
    extern int optind;
    int opt;
    boolean File_found;

    /* set defaults */
    File_found = FALSE;

    /* get the argument options */

    while ((opt = getopt (argc, argv, "o:")) != -1)
	switch (opt)
	{
	case 'o':
	    Max_order = atoi (optarg);
	    assert (Max_order >= 0);
	    break;
	default:
	    usage ();
	    break;
	}

    for (; optind < argc; optind++)
	usage ();
}

int
main (int argc, char *argv[])
{
    unsigned int cache, context, symbol, data;

    init_arguments (argc, argv);

    TXT_init_files ();

    cache = PPM_create_cache (Max_order);
    context = PPM_create_cache_context (cache);

    for (;;)
      {
	printf ("symbol? ");
	scanf ("%dq", &symbol);

	if (PPM_update_cache (cache, context, symbol))
	  {
	    data = PPM_get_cache (cache);
	    printf ("Found in cache, data = %d\n", data);
	  }
	else
	  {
	    printf ("data? ");
	    scanf ("%d", &data);
	    if (!data)
	        break;
	    PPM_set_cache (cache, data);
	  }

	PPM_dump_cache (Stdout_File, cache, NULL);
      }

    PPM_release_cache_context (cache, context);
    PPM_release_cache (cache);

    exit (1);
}
