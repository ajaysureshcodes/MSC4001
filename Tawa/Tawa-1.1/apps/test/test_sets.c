/* For testing the set module */
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>

#include "model.h"
#include "sets.h"
#include "io.h"

int main (int argc, char *argv [])
{
    unsigned int card, card1, total, total_card, seed, run, i, j, k, n;
    /*unsigned int *members;*/
    unsigned int set, set1, set2, set3;
    unsigned int file, file1, file2;

    debugSetsLevel = 1;
    TXT_init_sets (256, 3, 8, TRUE);

    seed = time (NULL);
    /*seed = 963918849;*/
    fprintf (stderr, "seed = %d\n", seed);
    /*srandom (seed);*/

    set = NIL;

    /*
    set1 = TXT_create_set ();
    TXT_put_set (set1, 12);
    fprintf (stdout, "Set = ");
    TXT_dump_setnodes (Stdout_File, set1);

    TXT_put_set (set1, 13);
    fprintf (stdout, "Set = ");
    TXT_dump_setnodes (Stdout_File, set1);

    TXT_put_set (set1, 14);
    fprintf (stdout, "Set = ");
    TXT_dump_setnodes (Stdout_File, set1);

    TXT_put_set (set1, 15);
    fprintf (stdout, "Set = ");
    TXT_dump_setnodes (Stdout_file, set1);
    TXT_dump_set (Stdout_file, set1);

    set1 = TXT_create_set ();
    for (k=0; k < 20; k++)
      {
	n = random ();
	n %= 64;
	TXT_put_set (set1, n);
	TXT_dump_setnodes (Stdout_file, set1);
	if (!TXT_get_set (set1, n))
	  fprintf (stderr, "Error: %d not found\n", n);
      }
    fprintf (stdout, "Set 1 = ");
    TXT_dump_set (Stdout_file, set1);

    set2 = TXT_create_set ();
    for (k=0; k < 20; k++)
      {
	n = random ();
	n %= 64;
	TXT_put_set (set2, n);
      }
    fprintf (stdout, "Set 2 = ");
    TXT_dump_set (Stdout_file, set2);

    set = TXT_intersect_set (set1, set2);
    fprintf (stdout, "Intersection = ");
    TXT_dump_set (Stdout_file, set);

    exit (1);
    */

    file1 = TXT_open_file ("junk1.set", "w", NULL,
			   "Test: can't open set file");
    debugSetsLevel = 1;

    total_card = 0;
    for (j=0; j < 200; j++)
      {
	total = SetMallocUnused;

	set = TXT_create_set ();

	card1 = (random () % 10000);
	fprintf (stderr, "\n%3d: card = %d\n", j, card1);
	for (k=0; k < card1; k++)
	  {
	    if (k && ((k % 100000) == 0))
	        fprintf (stderr, "Iteration %d\n", k);

	    n = random ();
	    if (SetMaxCard)
	      n %= SetMaxCard;

	    /*fprintf (stderr, "iteration %3d putting %d\n", k, n);*/

	    /*if (k == 5683)
	      debugSets ();*/
	    TXT_put_set (set, n);

	    /*
	      members = TXT_get_setmembers (set);
	      fprintf (stderr, "card = %d members = %d\n",
	      TXT_card_set (card), members [0]);
	      TXT_dump_setmembers (stderr, members);
	      TXT_release_setmembers (members);

	      fprintf (stderr, "Set = ");
	      TXT_dump_set (stderr, set);

	      fprintf (stderr, "Set = ");
	      TXT_dump_setnodes (stderr, set);
	    */

	    run = random () % 4;
	    run = 0;
	    for (i=0; i<run; i++)
	      if (n+i+1 < SetMaxCard)
		TXT_put_set (set, n+i+1);

	    if (debugSetsLevel > 1)
	      TXT_dump_setnodes (Stderr_File, set);

	    for (i=0; i<=run; i++)
	      if (n+i < SetMaxCard)
		if (!TXT_get_set (set, n+i))
		  fprintf (stderr, "Error: %d not found iteration %d\n", n+i, k);

	  }

	TXT_awrite_set (file1, set);

	card = TXT_card_set (set);
	total_card += card;
	total = SetMallocUnused - total;
	fprintf (stderr, "Card %d total %d Unused %7d average = %.3f tot. avg. = %.3f\n\n",
		 card, total, SetMallocUnused, (float) total / card,
		 (float) SetMallocUnused / total_card);

	TXT_stats_sets (Stderr_File);
	/*TXT_release_set (set);*/
      }

    TXT_close_file (file1);
    fprintf (stderr, "Total card = %d\n", total_card);

    exit (1);

    /*
    fprintf (stderr, "Set = ");
    TXT_dump_set (stderr, set);

    fprintf (stderr, "\nSetnodes = \n");
    TXT_dump_setnodes (stderr, set);
    */

    file = TXT_open_file ("junk.set", "w", NULL,
			  "Test: can't open set file");
    TXT_write_set (file, set);
    TXT_close_file (file);

    file = TXT_open_file ("junk.set", "r", NULL,
			  "Test: can't open set file");
    set1 = TXT_load_set (file);
    TXT_close_file (file);

    printf ("Card %d Bytes in %d average = %.3f\n",
	    TXT_card_set (set1), SetBytesOut,
	    (float) SetBytesOut / TXT_card_set (set1));
    if (TXT_equal_set (set, set1))
      printf ("Load of set is equal\n");
    else
      printf ("*** Fatal: Load of set is not equal ***\n");

    set2 = TXT_copy_set (set);
    if (TXT_equal_set (set, set2))
      printf ("Copy of set is equal\n");
    else
      printf ("*** Fatal: Copy of set is not equal ***\n");

    SetBytesIn = 0;
    SetBytesOut = 0;

    file1 = TXT_open_file ("junk1.set", "w", NULL,
			   "Test: can't open set file");
    TXT_awrite_set (file1, set);
    TXT_close_file (file1);

    file2 = TXT_open_file ("junk1.set", "r", NULL,
			   "Test: can't open set file");
    set3 = TXT_aload_set (file2);
    TXT_close_file (file2);
    /*
      printf ("Set = ");
      TXT_dump_set (Stdout_file, set3);
    */

    printf ("Card %d Bytes out %d average = %.3f\n",
	    TXT_card_set (set2), SetBytesOut,
	    (float) SetBytesOut / TXT_card_set (set2));

    if (TXT_equal_set (set, set3))
      printf ("Aload of set is equal\n");
    else
      printf ("*** Fatal: Aload of set is not equal ***\n");

    TXT_release_set (set);
    TXT_release_set (set1);
    TXT_release_set (set2);
    TXT_release_set (set3);

    exit (1);
}
