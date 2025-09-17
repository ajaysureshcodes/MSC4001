/* "PPMo encoder program. */
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
boolean Dump_Stats = FALSE;
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
	     "Usage: encode_ppmo [options]\n"
	     "\n"
	     "options:\n"
	     "  -a n\talphabet size=c (default = 256)\n"
	     "  -c\tdebug codelengths\n"
	     "  -d n\tdebug level=n\n"
	     "  -D\tDump out statistics gathered during encoding\n"
	     "  -i fn\tinput filename=fn (required argument)\n"
	     "  -m fn\tmodel filename=fn\n"
	     "  -M str\tmask orders=str (do not code them)\n"
	     "  -N\tinput text file is a sequence of unsigned numbers\n"
	     "  -o fn\toutput filename=fn (required argument)\n"
	     "  -O n\torder of order model=n (default = 3)\n");
    fprintf (stderr,
	     "  -p n\tdebug progress=n\n"
	     "  -r\tdebug arithmetic coding ranges\n"
	     "  -S n\torder of symbol model=n (default = 4)\n"
	     "  -T n\tset threshold for OS order stream modelling to n\n"
	     "  -W fn\twrite orders to file=fn\n"
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
    while ((opt = getopt (argc, argv, "a:cd:Di:m:M:No:O:p:rS:T:WX")) != -1)
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
	case 'D':
	    Dump_Stats = TRUE;
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
	    assert (OMax_Order >= 0);
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
	case 'W':
	  if ((PPMo_Orders_fp = fopen (optarg, "w")) == NULL)
            {
	      fprintf( stderr, "can't open file %s\n", optarg);
	      exit(1);
	    }
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

int
getSymbol (unsigned int file, unsigned int *symbol)
/* Returns the next symbol from the input file. */
{
    unsigned int sym;
    int result;

    sym = 0;
    if (Use_Numbers)
      {
        result = fscanf (Files [file], "%u", &sym);
	switch (result)
	  {
	  case 1: /* one number read successfully */
	    break;
	  case EOF: /* eof found */
	    break;
	  case 0:
	    fprintf (stderr, "Formatting error in file\n");
	    break;
	  default:
	    fprintf (stderr, "Unknown error (%i) reading file\n", result);
	    exit (1);
	  }
      }
    else
      {
        sym = getc (Files [file]);
	result = sym;
      }
    *symbol = sym;
    return (result);
}

void PPMo_encode_file (unsigned int file, unsigned int model, unsigned int context, unsigned int coder)
/* Encodes the text from the file. */
{
    unsigned int symbol;

    /* dump_memory (stdout); */

    bytes_input = 0;
    for (;;)
      {
        if (getSymbol (file, &symbol) == EOF)
	    break;
	if (Debug.level > 1)
	    fprintf (stderr, ">>> Input pos %d symbol %d\n", bytes_input, symbol);

	if (Debug.progress && (bytes_input % Debug.progress) == 0)
	  {
	    fprintf (stderr, "%d symbols processed\n", bytes_input);
	  }

	TLM_encode_symbol (model, context, coder, symbol);

	if (Debug.level > 4)
	  {
	    TLM_dump_model (Stderr_File, model, NULL);
	  }

	bytes_input++;
      }

    TLM_encode_symbol (model, context, coder, TXT_sentinel_symbol ()); /* Encode EOF */

    if (Debug.level > 4)
      {
        TLM_dump_model (Stderr_File, model, NULL);
      }
}

int
main(int argc, char *argv [])
{
    unsigned int model, context, coder;
    unsigned int total;

    init_arguments (argc, argv);

    Input_file = TXT_open_file
      (Input_filename, "r", "Encoding input file",
       "Encode_ppmo: can't open input file" );

    Output_file = TXT_open_file
      (Output_filename, "w", "Writing to output file",
       "Encode_ppmo: can't open output file" );

    if (!Model_found)
        model = TLM_create_model (TLM_PPMo_Model, "Encode PPMo Model", Alphabet_Size, OMax_Order,
				  SMax_Order, PPMo_Mask, OS_Model_Threshold,
				  Performs_Excls);
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
      (CODER_MAX_FREQUENCY, Input_file, Output_file, NIL, NIL,
       arith_encode, arith_decode, arith_decode_target);

    arith_encode_start (Output_file);

    PPMo_encode_file (Input_file, model, context, coder);

    arith_encode_finish (Output_file);

    fprintf (stderr, "Bytes input %d, bytes output %d (%.3f bpc)",
	     bytes_input, bytes_output,
	     (8.0 * bytes_output) / bytes_input);
    if (Debug.codelengths)
	fprintf (stderr, " [orders %.3f (%.1f%%) symbols %.3f (%.1f%%)]",
		 PPMo_Codelength_Orders / 8, 100.0 *
		 PPMo_Codelength_Orders/(PPMo_Codelength_Orders + PPMo_Codelength_Symbols),
		 PPMo_Codelength_Symbols / 8, 100.0 *
		 PPMo_Codelength_Symbols/(PPMo_Codelength_Orders + PPMo_Codelength_Symbols));
    fprintf (stderr, "\n");

    if (Debug.level > 3)
      {
        TLM_dump_model (Stderr_File, model, NULL);
      }

    if (Dump_Stats)
      {
	total =  PPMo_determ_symbols + PPMo_nondeterm_symbols;
	fprintf (stderr, "Number of symbols: determ. = %d (%.3f) non.determ. = %d (%.3f)\n",
		 PPMo_determ_symbols, (float) PPMo_determ_symbols * 100.0 / total,
		 PPMo_nondeterm_symbols, (float) PPMo_nondeterm_symbols * 100.0 / total);
      }

    exit (0);
}
