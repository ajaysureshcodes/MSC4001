/* "PPMO decoder program. */
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "unistd.h"
#include "assert.h"

#include "io.h"
#include "text.h"
#include "coder.h"
#include "model.h"
#include "ppmo_model.h"

#define MAX_FILENAME_SIZE 128      /* Maximum size of a filename */

unsigned int Input_file;
char Input_filename [MAX_FILENAME_SIZE];

unsigned int Output_file;
char Output_filename [MAX_FILENAME_SIZE];

unsigned int Model_file;
boolean Model_found = FALSE;
char Model_filename [MAX_FILENAME_SIZE];

boolean Use_Numbers = FALSE;
unsigned int Alphabet_Size;
unsigned int PPMo_Mask;
int OMax_Order;
int SMax_Order;
int OS_Model_Threshold;
boolean Performs_Excls;

void junk ()
/* For debugging purposes. */
{
  fprintf (stderr, "Got here\n");
}

void
usage (void)
{
    fprintf (stderr,
	     "Usage: decode_ppmo [options]\n"
	     "\n"
	     "options:\n"
	     "  -a n\talphabet size=c (default = 256)\n"
	     "  -c\tdebug codelengths\n"
	     "  -d n\tdebug level=n\n"
	     "  -i fn\tinput filename=fn (required argument)\n"
	     "  -m fn\tmodel filename=fn\n"
	     "  -M str\tmask orders=str (do not code them)fn\n"
	     "  -N\toutput text file is a sequence of unsigned numbers\n"
	     "  -o fn\toutput filename=fn (required argument)\n"
	     "  -O n\torder of order model=n (default = 3)\n");
    fprintf (stderr,
	     "  -p n\tdebug progress=n\n"
	     "  -r\tdebug arithmetic coding ranges\n"
	     "  -S n\torder of symbol model=n (default = 4)\n"
	     "  -T n\tset threshold for OS order stream modelling to n\n"
	     "  -X\tperform exclusions (default=FALSE)\n"
	);
    exit (2);
}

void
init_arguments (int argc, char *argv[])
{
    extern char *optarg;
    extern int optind; 
    int opt;

    boolean Input_found = FALSE, Output_found = FALSE;

    PPMo_Mask = TXT_create_text ();

    Alphabet_Size = PPMo_DEFAULT_ALPHABET_SIZE;
    OMax_Order = PPMo_DEFAULT_ORDER_MODEL_ORDER;
    SMax_Order = PPMo_DEFAULT_SYMBOL_MODEL_ORDER;
    OS_Model_Threshold = PPMo_DEFAULT_OS_MODEL_THRESHOLD;
    Performs_Excls = PPMo_DEFAULT_PERFORMS_EXCLS;

    /* get the argument options */
    while ((opt = getopt (argc, argv, "a:cd:i:m:M:No:O:p:rS:T:X")) != -1)
	switch (opt)
	{
	case 'a':
	    Alphabet_Size = atol (optarg);
	    break;
	case 'c':
	    Debug.codelengths = TRUE;
	    break;
	case 'd':
	    Debug.level = atol (optarg);
	    break;
	case 'i':
	    Input_found = TRUE;
	    sprintf (Input_filename, "%s", optarg);
	    break;
	case 'm':
	    Model_found = TRUE;
	    sprintf (Model_filename, "%s", optarg);
	    break;
	case 'M':
	    fprintf( stderr, "Masking orders %s\n", optarg);
	    TXT_append_string (PPMo_Mask, optarg);
	    break;
	case 'N':
	    Use_Numbers = TRUE;
	    break;
	case 'o':
	    Output_found = TRUE;
	    sprintf (Output_filename, "%s", optarg);
	    break;
	case 'O':
	    OMax_Order = atol (optarg);
	    assert (OMax_Order >= -1);
	    break;
	case 'p':
	    Debug.progress = atol (optarg);
	    break;
	case 'r':
	    Debug.coder = TRUE;
	    break;
	case 'S':
	    SMax_Order = atol (optarg);
	    assert (SMax_Order >= -1);
	    break;
	case 'T':
	    OS_Model_Threshold = atoi (optarg);
	    break;
	case 'X':
	    Performs_Excls = TRUE;
	    break;
	default:
	    usage ();
	    break;
	}

    if (!Input_found)
        fprintf (stderr, "\nFatal error: missing input filename\n\n");
    if (!Output_found)
        fprintf (stderr, "\nFatal error: missing output filename\n\n");
    if (!Input_found || !Output_found)
      {
	usage ();
	exit (1);
      }

    for (; optind < argc; optind++)
	usage ();
}

void PPMo_decode_file (unsigned int model, unsigned int context, unsigned int coder)
/* Decodes the text. */
{
    unsigned int symbol;

    bytes_input = 0;
    for (;;)
      { /* repeat until EOF found */
	symbol = TLM_decode_symbol (model, context, coder);
	if (Debug.level > 1)
	    fprintf (stderr, ">>> Input pos %d char %c (%d)\n", bytes_input, symbol, symbol);

	if (Debug.progress && (bytes_input % Debug.progress) == 0)
	  {
	    fprintf (stderr, "%d chars processed\n", bytes_input);
	  }

	if (Debug.level > 4)
	  {
	    TLM_dump_model (Stderr_File, model, NULL);
	  }

	if (symbol == TXT_sentinel_symbol ())
	    break;

	if (Use_Numbers)
	    fprintf (Files [Output_file], "%d\n", symbol);
	else
	    fputc (symbol, Files [Output_file]);

	bytes_input++;
      }
}

int
main(int argc, char *argv [])
{
    unsigned int model, context, coder;

    init_arguments (argc, argv);

    Input_file = TXT_open_file
      (Input_filename, "r", "Decoding input file",
       "Decode_ppmo: can't open input file" );

    Output_file = TXT_open_file
      (Output_filename, "w", "Writing to output file",
       "Decode_ppmo: can't open output file" );

    if (!Model_found)
        model = TLM_create_model
	  (TLM_PPMo_Model, "Encode PPMo Model", Alphabet_Size, OMax_Order,
	   SMax_Order, PPMo_Mask, OS_Model_Threshold, Performs_Excls);
    else
      {
        Model_file = TXT_open_file
	  (Model_filename, "r", "Loading model file",
	   "Encode_ppmo: can't open model file" );
	model = TLM_load_model (Model_file);

	if (Debug.level > 4)
	    TLM_dump_model (Stderr_File, model, NULL);
      }

    context = TLM_create_context (model); 
    coder = TLM_create_coder
      (CODER_MAX_FREQUENCY, NIL, NIL, Input_file, Output_file,
       arith_encode, arith_decode, arith_decode_target);

    arith_decode_start (Input_file);

    PPMo_decode_file (model, context, coder);

    if (Debug.level > 3)
      {
        TLM_dump_model (Stderr_File, model, NULL);
      }

    arith_decode_finish (Input_file);

    exit (0);
}
