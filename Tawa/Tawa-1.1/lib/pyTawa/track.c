/* Routines for keeping track of a model's performance as it progresses
   through a sequence of text. */
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <ctype.h>
#include <assert.h>
#include "io.h"
#include "model.h"
#include "text.h"
#include "word.h"
#include "track.h"

#define TRACKS_SIZE 20                   /* Initial size of Tracks array */

struct trackType
{ /* Input track to correct, or corrected output track */
    unsigned int Track_length;           /* Length of the track. */
    unsigned int Track_type;             /* The type of tracking */
    float *Track_codelength;             /* A track of the codelength for
					    encoding the current symbol.*/
    unsigned int *Track_coderanges;      /* A track of the coderanges list for
					    encoding the current symbol.*/
};
#define Track_next Track_length          /* Used to find the "next" on the
					    used list for deleted tracks */

struct trackType *Tracks = NULL;         /* List of track records */
unsigned int Tracks_max_size = 0;        /* Current max. size of the tracks
					    array */
unsigned int Tracks_used = NIL;          /* List of deleted track records */
unsigned int Tracks_unused = 1;          /* Next unused track record */

boolean
TLM_valid_track (unsigned int track)
/* Returns non-zero if the track is valid, zero otherwize. */
{
    if (track == NIL)
        return (FALSE);
    else if (track >= Tracks_unused)
        return (FALSE);
    else if (Tracks [track].Track_type == TLM_invalid_tracking)
        return (FALSE);
    else
        return (TRUE);
}

unsigned int
create_track (unsigned int track_type, unsigned int textlen)
/* Creates a track record. :-) */
{
    struct trackType *track;
    unsigned int t, old_size;
    unsigned int *coderangesp;
    float *codelenp;

    if (Tracks_used != NIL)
    {	/* use the first list of tracks on the used list */
	t = Tracks_used;
	Tracks_used = Tracks [t].Track_next;
    }
    else
    {
	t = Tracks_unused;
	if (Tracks_unused+1 >= Tracks_max_size)
	{ /* need to extend Tracks array */
	    old_size = Tracks_max_size * sizeof (struct trackType);
	    if (Tracks_max_size == 0)
		Tracks_max_size = TRACKS_SIZE;
	    else
		Tracks_max_size *= 2; /* Keep on doubling the array on demand */
	    Tracks = (struct trackType *) Realloc (101, Tracks,
		     Tracks_max_size * sizeof (struct trackType), old_size);

	    if (Tracks == NULL)
	    {
		fprintf (stderr, "Fatal error: out of tracks space\n");
		exit (1);
	    }
	}
	Tracks_unused++;
    }

    if (t != NIL)
    {
      track = Tracks + t;

      track->Track_length = 0;
      track->Track_type = track_type;
      track->Track_codelength = NULL;
      track->Track_coderanges = NULL;

      track->Track_length = textlen;
      codelenp = (float *) Malloc (102, (textlen+2) * sizeof (float));
      track->Track_codelength = codelenp;

      if (track_type != TLM_comprehensive_tracking)
	  coderangesp = NULL;
      else
	  coderangesp = (unsigned int *) Malloc (102, (textlen+2) * sizeof (float));
      track->Track_coderanges = coderangesp;
    }
    return (t);
}

void
TLM_release_track (unsigned int track)
/* Releases the memory allocated to the model's track and the track number
   (which may be reused in later create_track calls). */
{
    struct trackType *trackp;

    assert (TLM_valid_track (track));

    trackp = Tracks + track;
    if (trackp->Track_codelength != NULL)
        Free (102, trackp->Track_codelength, trackp->Track_length *
	      sizeof (float));
    if (trackp->Track_coderanges != NULL)
        Free (102, trackp->Track_coderanges, trackp->Track_length *
	      sizeof (unsigned int));

    trackp->Track_type = TLM_invalid_tracking; /* Used for testing later on if
						 track record is valid or not */
    trackp->Track_length = 1; /* Used for testing later on if track no. is valid or not */

    /* Append onto head of the used list */
    trackp->Track_next = Tracks_used;
}

unsigned int
TLM_make_track (unsigned int track_type, unsigned int model, unsigned int text)
/* Creates a new track record associated with using the model
   to predict the sequence of text. */
{
    struct trackType *trackp;
    unsigned int track, context, textlen, symbol, p;
    unsigned int *coderangesp;
    float *codelenp;

    assert (TLM_valid_model (model));
    assert (TXT_valid_text (text));
    textlen = TXT_length_text (text);

    track = create_track (track_type, textlen);
    trackp = Tracks + track;
    codelenp = trackp->Track_codelength;
    coderangesp = trackp->Track_coderanges;

    if (track_type != TLM_comprehensive_tracking)
        TLM_set_context_operation (TLM_Get_Codelength);
    else
        TLM_set_context_operation (TLM_Get_Coderanges);

    context = TLM_create_context (model);

    /* Now encode each symbol */
    for (p=0; p < textlen; p++) /* encode each symbol */
    {
      TXT_get_symbol (text, p, &symbol);
      TLM_update_context (model, context, symbol);
      if (track_type != TLM_comprehensive_tracking)
	{
	  /*fprintf (stderr, "pos %d symbol %d codelen %.3f\n", p, symbol,
	    TLM_Codelength);*/
	  codelenp [p] = TLM_Codelength;
	}
      else
	{ /* comprehensive tracking */
	  /*
	    fprintf (stderr, "pos %d symbol %d coderanges ", p, symbol);
	    TLM_dump_coderanges (stderr, TLM_Coderanges);
	  */
	  coderangesp [p] = TLM_Coderanges;
	  codelenp [p] = TLM_codelength_coderanges (TLM_Coderanges);
	}
    }
    TLM_release_context (model, context);

    return (track);
}

unsigned int
TLM_make_words_track (unsigned int track_type, unsigned int words_model,
		      unsigned int text)
/* Creates a new track record associated with using the words model
   to predict the sequence of text. */
{
    struct trackType *trackp;
    unsigned int track, textlen, textpos, word, nonword, p, q;
    unsigned int nonword_model, word_model, nonchar_model, char_model;
    unsigned int nonword_context, word_context, nonchar_context, char_context;
    unsigned int nonword_table, word_table;
    unsigned int wordpos, nonwordpos;
    codingType coding_type;
    unsigned int *coderangesp;
    boolean eof;
    float *codelenp;

    assert (TLM_valid_words_model (words_model));

    TLM_get_words_model
      (words_model, &nonword_model, &word_model, &nonchar_model, &char_model,
       &nonword_table, &word_table);

    assert (TXT_valid_text (text));
    textlen = TXT_length_text (text);

    track = create_track (track_type, textlen);
    trackp = Tracks + track;
    codelenp = trackp->Track_codelength;
    coderangesp = trackp->Track_coderanges;

    word = TXT_create_text ();
    nonword = TXT_create_text ();

    word_context = TLM_create_context (word_model);
    char_context = TLM_create_context (char_model);
    nonword_context = TLM_create_context (nonword_model);
    nonchar_context = TLM_create_context (nonchar_model);

    textpos = 0;
    if (track_type != TLM_comprehensive_tracking)
        coding_type = FIND_CODELENGTH_TYPE;
    else
        coding_type = FIND_CODERANGES_TYPE;
    for (;;) /* encode each non-word and word */
    {
      p = textpos;
      eof = !TXT_getword_text1 (text, nonword, word, &textpos, &nonwordpos, &wordpos);

      for (q = p; q < textpos; q++)
	{ /* Set codelengths to 0 & coderanges to NIL; these will be filled in latter at
	     two positions only: at the start of the nonword and at the start of the word */
	  codelenp [q] = 0;
	  if (track_type == TLM_comprehensive_tracking)
	      coderangesp [q] = NIL;
	}

      codelenp [nonwordpos] = TLM_process_word
	  (text, nonword, nonword_model, nonword_context, nonchar_model, nonchar_context,
		      coding_type, NIL, nonword_table, FALSE);
      if (track_type == TLM_comprehensive_tracking)
	  coderangesp [nonwordpos] = TLM_Coderanges;

      codelenp [wordpos] = TLM_process_word
	  (text, word, word_model, word_context, char_model, char_context, coding_type, NIL,
	   word_table, eof);
      if (track_type == TLM_comprehensive_tracking)
	  coderangesp [wordpos] = TLM_Coderanges;

      if (eof)
	  break;
    }
    TLM_release_context (word_model, word_context);
    TLM_release_context (char_model, char_context);
    TLM_release_context (nonword_model, nonword_context);
    TLM_release_context (nonchar_model, nonchar_context);

    TXT_release_text (word);
    TXT_release_text (nonword);

    return (track);
}

void
TLM_get_track (unsigned int track, unsigned int pos, float *codelength)
/* Returns the codelength associated with the track at position pos. The track
   can be constructed using either TLM_make_track () or TLM_make_words_track (). */
{
    assert (TLM_valid_track (track));

    assert (track != NIL);
    assert (pos < Tracks [track].Track_length);

    *codelength = Tracks [track].Track_codelength [pos];
}

void
TLM_get_coderanges_track (unsigned int track, unsigned int pos,
			  unsigned int *coderanges)
/* Returns the list of coderanges associated with the track at position pos. The track
   can be constructed using either TLM_make_track () or TLM_make_words_track (). */
{
    assert (TLM_valid_track (track));
    assert (Tracks [track].Track_type == TLM_comprehensive_tracking);
    assert (pos < Tracks [track].Track_length);

    *coderanges = Tracks [track].Track_coderanges [pos];
}

