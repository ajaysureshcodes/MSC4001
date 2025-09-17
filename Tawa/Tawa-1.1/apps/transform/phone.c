/* Digital to alphabetic translation for a mbile tellephone. */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#ifdef SYSTEM_LINUX
#include <getopt.h> /* for getopt on Linux systems */
#endif

#include "io.h"
#include "text.h"
#include "paths.h"
#include "model.h"
#include "transform.h"

unsigned int Language_model;

void
usage (void)
{
    fprintf (stderr,
	     "Usage: play [options] training-model <input-text\n"
	     "\n"
	     "options:\n"
	     "  -l n\tdebug level=n\n"
	     "  -d n\tdebug paths=n\n"
	     "  -p n\tdebug progress=n\n"
	     "  -m fn\tmodel filename=fn\n"
	);
    exit (2);
}

void
init_arguments (int argc, char *argv[])
{
    extern char *optarg;
    extern int optind; 
    int opt;

    /* set defaults */
    Debug.level = 0;

    /* get the argument options */

    while ((opt = getopt (argc, argv, "d:l:m:p:")) != -1)
	switch (opt)
	{
	case 'p':
	    Debug.progress = atoi (optarg);
	    break;
	case 'l':
	    Debug.level = atoi (optarg);
	    break;
	case 'd':
	    Debug.level1 = atoi (optarg);
	    break;
	case 'm':
	    Language_model =
	        TLM_read_model (optarg, "Loading model from file",
				"Phone: can't open model file");
	    break;
	default:
	    usage ();
	    break;
	}
    for (; optind < argc; optind++)
	usage ();
}

void
convertText (unsigned int telephone_text, unsigned int english_text)
/* Converts the english text to the telephone text. */
{
    unsigned int symbol, p;

    TXT_setlength_text (telephone_text, 0);
    TXT_append_symbol (telephone_text, ' ');
    TXT_append_symbol (telephone_text, ' ');
    TXT_append_symbol (telephone_text, ' ');
    p = 0;
    while (TXT_get_symbol (english_text, p++, &symbol))
      {
	switch (symbol)
	  {
	  case ' ': case '.': case ',':
	      TXT_append_symbol (telephone_text, symbol);
	      break;
	  case '\n':
	      TXT_append_symbol (telephone_text, ' ');
	      break;
	  case 'a': case 'b': case 'c':
	  case 'A': case 'B': case 'C':
	      TXT_append_symbol (telephone_text, '2');
	      break;
	  case 'd': case 'e': case 'f':
	  case 'D': case 'E': case 'F':
	      TXT_append_symbol (telephone_text, '3');
	      break;
	  case 'g': case 'h': case 'i':
	  case 'G': case 'H': case 'I':
	      TXT_append_symbol (telephone_text, '4');
	      break;
	  case 'j': case 'k': case 'l':
	  case 'J': case 'K': case 'L':
	      TXT_append_symbol (telephone_text, '5');
	      break;
	  case 'm': case 'n': case 'o':
	  case 'M': case 'N': case 'O':
	      TXT_append_symbol (telephone_text, '6');
	      break;
	  case 'p': case 'q': case 'r': case 's':
	  case 'P': case 'Q': case 'R': case 'S':
	      TXT_append_symbol (telephone_text, '7');
	      break;
	  case 't': case 'u': case 'v':
	  case 'T': case 'U': case 'V':
	      TXT_append_symbol (telephone_text, '8');
	      break;
	  case 'w': case 'x': case 'y': case 'z':
	  case 'W': case 'X': case 'Y': case 'Z':
	      TXT_append_symbol (telephone_text, '9');
	      break;
	  }
      }
}

int
main (int argc, char *argv[])
{
    unsigned int input, tinput;      /* The input text to correct */
    unsigned int transform_text;        /* The marked up text */
    unsigned int transform_model;

    input = TXT_create_text ();
    tinput = TXT_create_text ();
    init_arguments (argc, argv);

    if (TLM_numberof_models () < 1)
        usage();

    transform_model = TTM_create_transform (TTM_Stack, TTM_Stack_type0, 0, 5);

    TTM_add_transform (transform_model, 0.0, " ", " ");
    TTM_add_transform (transform_model, 0.0, ".", ".");
    TTM_add_transform (transform_model, 0.0, ",", ",");

    TTM_add_transform (transform_model, 0.0, "2", "%[abcABC]");
    TTM_add_transform (transform_model, 0.0, "3", "%[defDEF]");
    TTM_add_transform (transform_model, 0.0, "4", "%[ghiGHI]");
    TTM_add_transform (transform_model, 0.0, "5", "%[jklJKL]");
    TTM_add_transform (transform_model, 0.0, "6", "%[mnoMNO]");
    TTM_add_transform (transform_model, 0.0, "7", "%[pqrsPQRS]");
    TTM_add_transform (transform_model, 0.0, "8", "%[tuvTUV]");
    TTM_add_transform (transform_model, 0.0, "9", "%[wxyzWXYZ]");

    /* TTM_dump_transform (stdout, transform_model); */

    for (;;)
      {
	printf ("Type in the text: ");
	if (TXT_readline_text (Stdin_File, input) == EOF)
	    break;

	convertText (tinput, input);

	TTM_start_transform (transform_model, TTM_transform_multi_context, tinput,
			  Language_model);

	transform_text = TTM_perform_transform (transform_model, tinput);

	printf ("Input  = ");
	TXT_dump_text (Stdout_File, tinput, TXT_dump_symbol);
	printf ("\n");

	printf ("Output = ");
	TXT_dump_text (Stdout_File, transform_text, TXT_dump_symbol);
	printf ("\n");

	TXT_release_text (transform_text);
      }

    TTM_release_transform (transform_model);

    exit (1);
}
