/* Aligns two input files of lines of text written in two different languages. */
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>

#define COST_DELETION 1
#define COST_INSERTION 1
#define COST_SUBSTITUTION 2

#define MIN3(a, b, c) ((a) < (b) ? ((a) < (c) ? (a) : (c)) : ((b) < (c) ? (b) : (c)))

int LevenshteinDistance(char *s1, char *s2) {
  unsigned int x, y, s1len, s2len;

  s1len = strlen(s1);
  s2len = strlen(s2);

  unsigned int matrix [s2len+1][s1len+1];

  matrix[0][0] = 0;
  for (x = 1; x <= s2len; x++)
    matrix[x][0] = matrix[x-1][0] + 1;
  for (y = 1; y <= s1len; y++)
    matrix[0][y] = matrix[0][y-1] + 1;
  for (x = 1; x <= s2len; x++)
    for (y = 1; y <= s1len; y++)
      matrix[x][y] = MIN3(
			  matrix[x-1][y] + COST_DELETION,
			  matrix[x][y-1] + COST_INSERTION,
			  matrix[x-1][y-1] + (s1[y-1] == s2[x-1] ? 0 : COST_SUBSTITUTION)
			 );

  return(matrix[s2len][s1len]);
}

int
main (int argc, char *argv[])
{

  int d;

  d = LevenshteinDistance("vark", "vark");
  printf ("distance between \"vark\" and \"vark\" = %d\n", d);

  d = LevenshteinDistance("vark", "vak");
  printf ("distance between \"vark\" and \"vak\" = %d\n", d);

  d = LevenshteinDistance("vark", "vaRk");
  printf ("distance between \"vark\" and \"vaRk\" = %d\n", d);

  d = LevenshteinDistance("vak", "vark");
  printf ("distance between \"vak\" and \"vark\" = %d\n", d);

  d = LevenshteinDistance("arrd", "arrdd");
  printf ("distance between \"arrd\" and \"arrdd\" = %d\n", d);

  d = LevenshteinDistance("arrdd", "arrd");
  printf ("distance between \"arrdd\" and \"arrdd\" = %d\n", d);

  d = LevenshteinDistance("arrdvark", "aarddvaklk");
  printf ("distance between \"arrdvark\" and \"aarddvaklk\" = %d\n", d);

  d = LevenshteinDistance("noow os te timer fore al1 god ment", "now is the time for all good men");
  printf ("distance between \"noow os te timer fore al1 god ment\" and \"now is the time for all good men\" = %d\n", d);

  exit (0);
}

