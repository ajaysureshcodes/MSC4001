/* Utility routines for both word_encode/word_decode */

#ifndef WORD_UTILS_H
#define WORD_UTILS_H

#define MODELS 2        /* There are two types of word models/contexts/etc. */

#define WORD_TYPE 0     /* Associated with word types of model */
#define NON_WORD_TYPE 1 /* Associated with non word types of model */

#define ARGS_MAX_ORDER 5         /* Default maximum order of the model */
#define ARGS_EXCLUSIONS TRUE     /* Perform exclusions by default */
#define ARGS_ALPHABET_SIZE 256   /* Default size of the model's alphabet */
#define ARGS_TITLE_SIZE 32       /* Size of the model's title string */
/* Title of model - this could be anything you choose: */
#define ARGS_TITLE "Sample PPMD model"

#define DEFAULT_WORD_MAX_ORDER 2 /* Default max.order for word-based model */

#define FILENAME_SIZE 256        /* Maximum size for a filename string */

int Args_max_order [MODELS];               /* Maximum order of the models */
unsigned int Args_alphabet_size [MODELS];  /* The models' alphabet size, 0 means unbounded  */
unsigned int Args_performs_full_excls [MODELS]; /* Flag to indicate whether the models perform full exclusions or not */
unsigned int Args_performs_update_excls [MODELS]; /* Flag to indicate whether the models perform update exclusions or not */

char *Args_model_filename;                 /* The filename of an existing model */
char *Args_title [MODELS];                 /* The title of the primary model */
unsigned int Args_model_file [MODELS];     /* The file pointers to the model files */
unsigned int Args_table_file [MODELS];     /* The file pointers to the table files */

void
open_word_model_files (char *file_prefix);
/* Opens all the word model files for reading. */

#endif
