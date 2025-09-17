/* Aligns two input files of lines of text written in two different languages. */
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>

#define MIN3(a, b, c) ((a) < (b) ? ((a) < (c) ? (a) : (c)) : ((b) < (c) ? (b) : (c)))

float Score_Substitution (char c1, char c2)
{
  if (c1 == c2)
    return 0;
  else
    return 2;
}

float Score_Deletion (char c)
{
    return 1;
}

float Score_Insertion (char c)
{
    return 1;
}

float *NWScore (char *x, char *y)
{
  float scoreSub, scoreDel, scoreIns;
  float *LastLine;
  unsigned int xlen, ylen, i, j;
  
  xlen = strlen(x);
  ylen = strlen(y);

  float Score [xlen+1][ylen+1];
  LastLine = (float *) calloc (ylen + 1, sizeof (float));

  Score [0][0] = 0.0;
  for (j=1; j <= ylen; j++)
      Score [0][j] = Score [0][j-1] + Score_Insertion (y[j]);

  for (i=1; i<= xlen; i++)
    {
      Score [i][0] = Score [i-1][0] + Score_Deletion (x[i]);

      for (j=1; j <= ylen; j++)
	{
	  scoreSub = Score [i-1][j-1] + Score_Substitution (x[i], y[j]);
	  scoreDel = Score [i-1][j] + Score_Deletion (x [i]);
	  scoreIns = Score [i][j-1] + Score_Insertion (y [j]);
	  Score [i][j] = MIN3 (scoreSub, scoreDel, scoreIns);
	}
    }

  for (j=0; j <= ylen; j++)
      LastLine [j] = Score [xlen][j];

  return LastLine;
}

void dumpLine (float *line, unsigned int len)
{
    unsigned int i;

    for (i = 0; i <= len; i++)
        printf ("%i: %.2f ", i, line [i]);
    printf ("\n");
}

function PartitionY(ScoreL, ScoreR)
     return arg max ScoreL + rev(ScoreR)

function Hirschberg(X,Y)
   Z = ""
   W = ""
     if length(X) == 0
  for i=1 to length(Y)
       Z = Z + '-'
       W = W + Yi
     end
    else if length(Y) == 0
  for i=1 to length(X)
       Z = Z + Xi
       W = W + '-'
     end
    else if length(X) == 1 or length(Y) == 1
  (Z,W) = NeedlemanWunsch(X,Y)
      else
	xlen = length(X)
	  xmid = length(X)/2
	  ylen = length(Y)
 
	  ScoreL = NWScore(X1:xmid, Y)
	  ScoreR = NWScore(rev(Xxmid+1:xlen), rev(Y))
	  ymid = PartitionY(ScoreL, ScoreR)
 
	  (Z,W) = Hirschberg(X1:xmid, y1:ymid) + Hirschberg(Xxmid+1:xlen, Yymid+1:ylen)
   end
	  return (Z,W)
	  }


int
main (int argc, char *argv[])
{
  /*
  float *ll;

  ll = NWScore("vark", "vark");
  printf ("distance between \"vark\" and \"vark\":\n");
  dumpLine (ll, 4);

  ll = NWScore("vark", "vak");
  printf ("distance between \"vark\" and \"vak\":\n");
  dumpLine (ll, 3);

  ll = NWScore("vark", "vaRk");
  printf ("distance between \"vark\" and \"vaRk\":\n");
  dumpLine (ll, 4);

  ll = NWScore("vak", "vark");
  printf ("distance between \"vak\" and \"vark\":\n");
  dumpLine (ll, 4);

  ll = NWScore("arrd", "arrdd");
  printf ("distance between \"arrd\" and \"arrdd\":\n");
  dumpLine (ll, 5);

  ll = NWScore("arrdd", "arrd");
  printf ("distance between \"arrdd\" and \"arrd\":\n");
  dumpLine (ll, 4);

  ll = NWScore("arrdvark", "aarddvaklk");
  printf ("distance between \"arrdvark\" and \"aarddvaklk\":");
  dumpLine (ll, 10);

  ll = NWScore("noow os te timer fore al1 god ment", "now is the time for all good men");
  printf ("distance between \"noow os te timer fore al1 god ment\" and \"now is the time for all good men\":\n");
  dumpLine (ll, 33);
*/

  exit (0);
}
