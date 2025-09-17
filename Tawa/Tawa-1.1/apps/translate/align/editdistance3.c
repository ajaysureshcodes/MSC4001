/* Aligns two input files of lines of text written in two different languages. */
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>

#define min3(x, y, z) \
  (((x) < (y)) ? (((z) < (x)) ? (z) : (x)) : (((z) < (y)) ? (z) : (y)))

int
streql(char *s1, char *s2)
/* TRUE if strings are identical */
{
  while (*s1++ == *s2)
  {
    if (*s2++ == '\0')
      return (1);
  }
  return (0);
}

int LevenshteinDistance(char *s, char *t)
{
  int slength, tlength, cost, i, j;
  unsigned int *v0, *v1;


  /* degenerate cases */
  if (s == NULL) return 0; 
  if (t == NULL) return 0; 
  if (streql (s, t)) return 0;
  slength = strlen (s);
  tlength = strlen (t);
  if (slength == 0) return tlength;
  if (tlength == 0) return slength;

  /* create two work vectors of integer distances */
  v0 = (unsigned int *) calloc (tlength + 1, sizeof (unsigned int));
  v1 = (unsigned int *) calloc (tlength + 1, sizeof (unsigned int));

  /* initialize v0 (the previous row of distances) */
  /* this row is A[0][i]: edit distance for an empty s */
  /* the distance is just the number of characters to delete from t */
  for (i = 0; i < tlength + 1; i++)
      v0[i] = i;

  for (i = 0; i < slength; i++)
    {
      /* calculate v1 (current row distances) from the previous row v0 */

      /* first element of v1 is A[i+1][0] */
      /*   edit distance is delete (i+1) chars from s to match empty t */
      v1[0] = i + 1;

      /* use formula to fill in the rest of the row */
      for (j = 0; j < tlength; j++)
        {
	  cost = (s[i] == t[j]) ? 0 : 1;
	  v1[j + 1] = min3(v1[j] + 1, v0[j + 1] + 1, v0[j] + cost);
        }

      /* copy v1 (current row) to v0 (previous row) for next iteration */
      for (j = 0; j < tlength + 1; j++)
	  v0[j] = v1[j];
    }

    return v1[tlength];
}

int
main (int argc, char *argv[])
{

  int d;

  d = LevenshteinDistance("vark", "vak");
  printf ("distance between \"vark\" and \"vak\" = %d\n", d);

  d = LevenshteinDistance("vark", "vaRk");
  printf ("distance between \"vark\" and \"vaRk\" = %d\n", d);

  d = LevenshteinDistance("vak", "vark");
  printf ("distance between \"vak\" and \"vark\" = %d\n", d);

  d = LevenshteinDistance("arrd", "arrd");
  printf ("distance between \"arrd\" and \"arrdd\" = %d\n", d);

  d = LevenshteinDistance("arrdvark", "aarddvaklk");
  printf ("distance between \"arrdvark\" and \"aarddvaklk\" = %d\n", d);

  d = LevenshteinDistance("noow os te timer fore al1 god ment", "now is the time for all good men");
  printf ("distance between \"noow os te timer fore al1 god ment\" and \"now is the time for all good men\" = %d\n", d);

  exit (0);
}
