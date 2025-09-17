""" Loads or creates a model, adds some text to the model, then writes out the changed
   model. For testing the loading and dumping model routines. """

from pyTawa.TLM import *
from pyTawa.TXT import *

import sys, getopt

def usage ():
    print(
	"Usage: train [options]",
	"",
	"options:",
	"  -B\tforce a break at each eoln",
	"  -N\tinput text is a sequence of unsigned numbers",
	"  -S\twrite out the model as a static model",
	"  -T n\tlong description (title) of model (required argument)",
	"  -U\tdo not perform update exclusions",
	"  -X\tdo not perform full exclusions",
	"  -a n\tsize of alphabet=n (required)",
	"  -d n\tdebugging level=n",
	"  -e n\tescape method=c",
        "  -h\tprint help",
	"  -i fn\tinput filename=fn (required argument)",
	"  -m fn\tmodel filename=fn (optional)",
	"  -o fn\toutput filename=fn (required argument)",
	"  -O n\tmax order of model=n (required)",
	"  -p n\tprogress report every n chars.",
	"  -t n\ttruncate input size after n bytes",
        sep = "\n", file=sys.stderr
    );
    sys.exit (2);

def check_argument (arguments, arg_key, arg_val):
    """ Returns True if the arg_key is in the arguments dictionary, and its
        value equals arg_val. """
    if (not arg_key in arguments):
        return False
    else:
        return (arguments [arg_key] == arg_val)

def get_argument (arguments, arg_key):
    """ Returns value associated with the arg_key is in the arguments
    dictionary, False if it does not exist. """
    if (not arg_key in arguments):
        return 0
    else:
        return (arguments [arg_key])

def insert_argument_string (arguments, arg_key, arg_val):
    """ Inserts the arg_val into arg_key if it is a non-zero length string
        and converts it to ascii. """
    if (isinstance(arg_val, str) and (len (arg_val) > 0)):
        arguments [arg_key] = arg_val.encode('ascii')

def init_arguments (arguments):
    """ Initialises the arguments dictionary from the command line. """
    try:
        opts, args = getopt.getopt(sys.argv[1:], "BNST:UXa:d:e:hi:m:o:O:p:t:", ["help"])
    except getopt.GetoptError as err:
        # print help information and exit:
        print("Train: option not recognized", file=sys.stderr)
        usage()
        sys.exit(2)

    for opt, arg in opts:
        match opt:
            case '-B':
                arguments ['Break Eoln'] = True
            case '-N':
                arguments ['Load Numbers'] = True
            case '-S':
                arguments ['Static Model'] = True
            case '-T':
                insert_argument_string (arguments, 'Title', arg)
            case '-U':
                arguments ['Performs Update Excls'] = False
            case '-X':
                arguments ['Performs Full Excls'] = False
            case '-a':
                arguments ['Alphabet Size'] = int(arg)
            case '-d':
                arguments ['Debug Level'] = int(arg)
            case '-e':
                escape = ord(arg) - ord('A')
                assert (escape >= 0)
                arguments ['Escape Method'] = escape
            case '-h' | '--help':
                # print help information and exit:
                usage()
                sys.exit()
            case '-i':
                insert_argument_string (arguments, 'Input Filename', arg)
            case '-m':
                insert_argument_string (arguments, 'Model Filename', arg)
            case '-o':
                insert_argument_string (arguments, 'Output Filename', arg)
            case '-O':
                arguments ['Max Order'] = int(arg)
            case '-p':
                arguments ['Debug Progress'] = int(arg)
            case '-t':
                arguments ['Max Input Size'] = int(arg)
            case _:
                # print error message and exit:
                print("Train: unrecognized option", file=sys.stderr)
                usage()
                sys.exit(2)

    if (not 'Model Filename' in arguments):
        if (not 'Alphabet Size' in arguments):
            print ("\nFatal error: missing alphabet size of the model\n",
                   file=sys.stderr)

        if (not 'Escape Method' in arguments):
            print ("\nFatal error: missing escape method of the model\n",
                   file=sys.stderr)

        if (not 'Title' in arguments):
            print ("\nFatal error: missing title of the model\n",
                   file=sys.stderr)

        if (not 'Max Order' in arguments):
            print ("\nFatal error: missing max. order of the model\n",
                   file=sys.stderr)

        if (not ('Alphabet Size' in arguments) or
            not ('Escape Method' in arguments) or
            not ('Title' in arguments) or
            not ('Max Order' in arguments)):
            usage ()
            sys.exit (1)
        print ("\nCreating new model\n", file=sys.stderr)
    else:
        if ('Title' in arguments):
            TLM_set_load_operation (TLM_Load_Change_Title, arguments ['Title'])
        Model = TLM_read_model (arguments ['Model Filename'],
                                b"Loading model from file",
			        b"Train: can't open model file")

    input_found = 'Input Filename' in arguments
    if (not input_found):
        print ("\nFatal error: missing input filename\n", file=sys.stderr)
    output_found = 'Output Filename' in arguments
    if (not output_found):
        print ("\nFatal error: missing output filename\n", file=sys.stderr)
    if (not input_found or not output_found):
        usage ()
        sys.exit (1)

    # Set defaults
    if (not 'Alphabet Size' in arguments):
        arguments ['Alphabet Size'] = TLM_PPM_ALPHABET_SIZE
    if (not 'Max Order' in arguments):
        arguments ['Max Order'] = TLM_PPM_MAX_ORDER
    if (not 'Escape Method' in arguments):
        arguments ['Escape Method'] = TLM_PPM_ESCAPE_METHOD
    if (not 'Performs Full Excls' in arguments):
        arguments ['Performs Full Excls'] = TLM_PPM_PERFORMS_FULL_EXCLS
    if (not 'Performs Update Excls' in arguments):
        arguments ['Performs Update Excls'] = TLM_PPM_PERFORMS_UPDATE_EXCLS

def train_model (fileid, model, arguments):
    """ Trains the model from the characters in the file with id Fileid. """

    print ("type =", type(fileid),type(model))
    context = TLM_create_context (model)

    TLM_set_context_operation (TLM_Get_Nothing)

    if ('Break Eoln' in arguments):
        # Start off the training with a sentinel symbol to indicate a break
        TLM_update_context (model, context, TXT_sentinel_symbol ())
    Debug_level = get_argument (arguments, 'Debug Level')
    Debug_progress = get_argument (arguments, 'Debug Progress')
    Load_numbers = 1 if get_argument (arguments, 'Load Numbers') else 0

    p = 0;
    while True:
        p += 1;
        if ((Debug_progress > 0) and ((p % Debug_progress) == 0)):
            print ("training pos", p, file=sys.stderr)

        # repeat until EOF or max input
        if ('Max Input Size' in arguments) and (p >= arguments ['Max Input Size']):
            break
        if (not TXT_getsymbol_file (fileid, Load_numbers)):
            break

        sym = TXT_input_symbol()
        
        if (('Break Eoln' in arguments) and (sym == BREAK_SYMBOL)):
            sym = TXT_sentinel_symbol ()

        TLM_update_context (model, context, sym)

        if (Debug_level > 1):
            TLM_dump_PPM_model (Stderr_File, model)

    TLM_update_context (model, context, TXT_sentinel_symbol ())
    TLM_release_context (model, context)

    print ("Trained on", p, "symbols\n", file=sys.stderr)

def main():
    """ Main program. """

    Arguments = {}
    init_arguments (Arguments)

    input_filename = Arguments ['Input Filename']
    output_filename = Arguments ['Output Filename']

    Input_file = TXT_open_file (input_filename, b"r", b"Reading input file",
                                b"Train: can't open input file" )
    Output_file = TXT_open_file (output_filename, b"w", b"Writing to output file",
                                 b"Train: can't open output file" )

    if not ('Model Filename' in Arguments):
        Model = TLM_create_PPM_model (Arguments ['Title'],
                Arguments ['Alphabet Size'], Arguments ['Max Order'],
                Arguments ['Escape Method'], Arguments ['Performs Full Excls'],
                Arguments ['Performs Update Excls'],)
        print ("Model =", Model, type(Model))
    else:
        if (not TLM_get_PPM_model (Arguments ['Model Filename'])):
            print ("Fatal error: Invalid model number\n", file=sys.stderr);
            sys.exit (1)
        elif (TLM_get_PPM_model_form () == TLM_Static):
            print ("Fatal error: This implementation does not permit further training when\n",
                   file=sys.stderr)
            print ("a static model has been loaded\n", file=sys.stderr)
            sys.exit (1)

        # Check for consistency of parameters between the loaded model and the model to be written out
        if (not check_argument (Arguments, 'Alphabet Size',
                                TLM_get_PPM_alphabet_size ())):
            print ("\nFatal error: alphabet sizes of output model does not match input model\n\n", file=sys.stderr)
            sys.exit (1)

        if (not check_argument (Arguments, 'Max Order',
                                TLM_get_PPM_max_order ())):
            print ("\nFatal error: max order of output model does not match input model\n\n", file=sys.stderr)
            sys.exit (1)

    print ("Model1 =", Model, type(Model))
    train_model (Input_file, Model, Arguments)
    if ('Static Model' in Arguments):
        TLM_write_model (Output_file, Model, TLM_Static)
    else:
        TLM_write_model (Output_file, Model, TLM_Dynamic)

    if (get_argument (Arguments, 'Debug Level') > 0):
        TLM_dump_PPM_model (Stderr_File, Model)

    TLM_release_model (Model)

    sys.exit (0)

if __name__ == '__main__':
    main()
