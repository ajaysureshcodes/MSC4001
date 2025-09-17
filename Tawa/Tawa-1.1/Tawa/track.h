/* Routines for keeping track of a model's performance as it progresses
   through a sequence of text. */

#ifndef TRACK_H
#define TRACK_H

#include "transform.h"

/* The following specifies different types of tracking */
/* Track type: */
#define TLM_invalid_tracking 0       /* Invalid tracking type */
#define TLM_standard_tracking 1      /* Standard tracking */
#define TLM_comprehensive_tracking 2 /* Comprehensive tracking */

void
TLM_release_track (unsigned int track);
/* Releases the memory allocated to the track and the track number (which may
   be reused in later create_track calls). */

unsigned int
TLM_make_track (unsigned int track_type, unsigned int model, unsigned int text);
/* Creates a new track record associated with using the model
   to predict the sequence of text. */

unsigned int
TLM_make_words_track (unsigned int track_type, unsigned int words_model,
		      unsigned int text);
/* Creates a new track record associated with using the words model
   to predict the sequence of text. */

void
TLM_get_track (unsigned int track, unsigned int pos, float *codelength);
/* Returns the codelength associated with the track at position pos. The track
   can be constructed using either TLM_make_track () or TLM_make_words_track (). */

void
TLM_get_coderanges_track (unsigned int track, unsigned int pos,
			  unsigned int *coderanges);
/* Returns the list of coderanges associated with the track at position pos. The track
   can be constructed using either TLM_make_track () or TLM_make_words_track (). */

#endif
