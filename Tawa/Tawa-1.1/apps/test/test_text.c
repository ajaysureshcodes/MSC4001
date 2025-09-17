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

unsigned int Text;

void
usage (void)
{
    fprintf (stderr,
	     "Usage: play [options] training-model <input-text\n"
	     "\n"
	     "options:\n"
	     "  -m fn\tmodel filename=fn\n"
	);
    exit (2);
}

void
init_arguments (int argc, char *argv[])
{
    extern char *optarg;
    extern int optind;
    unsigned int file;
    int opt;
    boolean File_found;

    /* set defaults */
    File_found = FALSE;

    /* get the argument options */

    while ((opt = getopt (argc, argv, "m:")) != -1)
	switch (opt)
	{
	case 'm':
	    file = TXT_open_file (optarg, "r", "Loading text from file",
				  "Test: can't open text file");
	    Text = TXT_load_text (file);
	    TXT_close_file (file);
	    File_found = TRUE;
	    break;
	default:
	    usage ();
	    break;
	}

    if (!File_found)
        usage ();
    for (; optind < argc; optind++)
	usage ();
}

int
main (int argc, char *argv[])
{
    unsigned int text, context, pos, len;

    init_arguments (argc, argv);

    text = TXT_copy_text (Text);
    context = TXT_create_text ();

    TXT_dump_text (Stdout_File, text, NULL);

    for (;;)
      {
	printf ("Pos? ");
	scanf ("%d", &pos);
	printf ("Len? ");
	scanf ("%d", &len);

	TXT_extract_text (text, context, pos, len);
	TXT_dump_text (Stdout_File, context, NULL);
      }
}
