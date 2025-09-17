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
#include "judgments.h"

/* Global variables: */
unsigned int Judgments;
unsigned int Documents;
unsigned int Topics;

void
usage (void)
{
    fprintf (stderr,
	     "Usage: ident_file [options] <input-text\n"
	     "\n"
	     "options:\n"
	     "  -D fn\tname of list of documents file=fn\n"
	     "  -J fn\tname of list of judgments file=fn\n"
	     "  -T fn\tname of list of topics file=fn\n"
	);
    exit (2);
}

void
init_arguments (int argc, char *argv[])
{
    extern char *optarg;
    extern int optind;
    unsigned int file;
    boolean judgments_found;
    boolean documents_found;
    boolean topics_found;
    int opt;

    /* get the argument options */

    judgments_found = FALSE;
    documents_found = FALSE;
    topics_found = FALSE;
    while ((opt = getopt (argc, argv, "D:J:T:")) != -1)
	switch (opt)
	{
        case 'D':
	    documents_found = TRUE;
	    file = TXT_open_file (optarg, "r", NULL,
				  "Test: can't open documents table file");

	    Documents = TXT_load_table (file);

	    break;
        case 'J':
	    judgments_found = TRUE;
	    TXT_load_judgments (optarg, Judgments);
	    break;
        case 'T':
	    topics_found = TRUE;
	    file = TXT_open_file (optarg, "r", NULL,
				  "Test: can't open topics table file");

	    Topics = TXT_load_table (file);

	    break;
	default:
	    usage ();
	    break;
	}
    if (!documents_found)
    {
        fprintf (stderr, "\nFatal error: missing documents table filename\n\n");
        usage ();
    }
    if (!judgments_found)
    {
        fprintf (stderr, "\nFatal error: missing judgments table filename\n\n");
        usage ();
    }
    if (!topics_found)
    {
        fprintf (stderr, "\nFatal error: missing topics table filename\n\n");
        usage ();
    }
    for (; optind < argc; optind++)
	usage ();
}

int
main(int argc, char *argv [])
{
    unsigned int document, topic;

    Judgments = TXT_create_table (TLM_Dynamic, 0);

    document = TXT_create_text ();
    topic = TXT_create_text ();

    init_arguments (argc, argv);

    for (;;)
      {
	printf ("Input document: ");
	if (TXT_readline_text (Stdin_File, document) <= 0)
	  break;
	printf ("Input topic: ");
	if (TXT_readline_text (Stdin_File, topic) <= 0)
	  break;

	if (TXT_find_judgment (Judgments, document, topic))
	  fprintf (stderr, "Judgment is found\n");
	else
	  fprintf (stderr, "Judgment is not found\n");
      }
    return (1);
}
