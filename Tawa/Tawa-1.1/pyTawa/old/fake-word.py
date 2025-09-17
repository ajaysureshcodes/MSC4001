""" Tries to generate fake words using a language model. """

import random

from pyTawa.TLM import *
from pyTawa.TXT import *
from pyTawa.TAR import *

import sys, getopt

def usage ():
    print(
        "Usage: fake-word [options]",
	"",
	"options:",
	"  -m fn\tmodel filename=fn (required argument)",
        sep = "\n", file=sys.stderr
    );
    sys.exit (2);

def init_arguments (arguments):
    """ Initialises the arguments dictionary from the command line. """

    opts_dict = {
        '-m': ('Model Filename', 'Str'),
        '-h': ('Help', 'Exit'),
        '--help': ('Help', 'Exit')
    }

    ok = TAR_init_arguments (arguments,
        "hm:", ["help"], opts_dict,
        "fake-word: option not recognized")
    if (not ok):
        usage()
        sys.exit(2)

    if (not TAR_check_required_arguments (arguments, opts_dict,
        [['Model Filename']])):
        usage ()
        sys.exit(2)

def codelengthText (model, text, debug_range = False, debug_chars = False):
    """ Returns the codelength (in bits) for encoding the text using the PPM
    model. """

    context = TLM_create_context (model)
    TLM_set_context_operation (TLM_Get_Codelength)

    """
    Insert the sentinel symbol at start of text to ensure first character
    is encoded using a sentinel symbol context rather than an order 0
    context.
    """

    if (debug_range):
        print ("Coding ranges for the sentinel symbol (not included in overall total:", file=sys.stderr);
    TLM_update_context (model, context, TXT_sentinel_symbol ())
    if (debug_range):
        print ("", file=sys.stderr);

    codelength = 0.0
    # Now encode each symbol
    for p in range(TXT_length_text1 (text)):
        symbol = TXT_getsymbol_text (text, p)
        TLM_update_context (model, context, symbol)
        codelen = TLM_get_codelength ()
        if (debug_chars):
            print ("Codelength for character", "%c" % symbol,
                   "= %7.3f" % codelen)
        codelength += codelen

    # Now encode the sentinel symbol again to signify the end of the text
    TLM_update_context (model, context, TXT_sentinel_symbol ())
    codelen = TLM_get_codelength ()
    if (debug_chars):
        print ("Codelength for sentinel symbol =", "%.3f" % codelen,
               file = sys.stderr)
    codelength += codelen

    TLM_release_context (model, context)

    return (codelength)

def main():
    """ Main program. """

    Arguments = {}
    init_arguments (Arguments)

    if ('Model Filename' in Arguments):
        model_filename = Arguments ['Model Filename']
        model_file = TXT_open_file (model_filename, b"r", b"Reading model file",
                                    b"Codelength: can't open model file" )
        Model = TLM_load_model (model_file)

    if (TLM_numberof_models () < 1):
      usage()

    print ("Testing out some words:")
    strings = ["beginning ", "gebittorn ", "heavenest ",
               "tracketor ", "finlayson ", "tictocker "]
    for string in strings:
        string_ascii = string.encode('ascii')
        string_text = TXT_write_string (string_ascii)
        codelength = codelengthText (Model, string_text)
        print ("Codelength = {0:5.2f} for string: '{1}'".format(codelength, string))


    # Creating 100 fake words
    for w in range (100):

        wordlen = random.randrange(4,12)
        print ("Creating some random words of length:", wordlen)
        best_word = ''
        best_codelength = 100000
        for rw in range (500000):
            if (best_codelength / wordlen < 5.5):
                break
            if (rw > 0) and (rw % 100000 == 0):
                print ("Generating random word, number", rw,
                       ", best codelength so far: {0:5.2f}".format(best_codelength),
                       ", best word: ", best_word)
            word = ''
            for p in range (wordlen):
                cc = random.choice ('abcdefghijklmnopqrstuvwxyz')
                word += cc
            string_ascii = word.encode('ascii')
            string_text = TXT_write_string (string_ascii)
            codelength = codelengthText (Model, string_text)
            #print (rw+1, word, "Codelength = {0:5.2f}".format(codelength))
            if (codelength < best_codelength):
                best_codelength = codelength
                best_word = word

        print ("Best word [", best_word, "] Best Codelength = {0:5.2f}".format(best_codelength))

    TLM_release_models ()

    sys.exit (0)

if __name__ == '__main__':
    main()
