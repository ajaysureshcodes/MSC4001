/* Gale and Church's align program taken from the appendix in their paper at:
   http://www.aclweb.org/anthology/J93-1004 */
#include <fcntl.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <limits.h>
#include <unistd.h>

/* Functions for Manipulating Regions */

struct region {
  int length;
};

void
print_region(fd, region, score)
     int score;
     FILE *fd;
     struct region *region;
{
  fprintf(fd, "%d\n", region->length);
}

/* Top Level Main Function */

int
main()
{
     struct region *regions;
     int i;

     regions = (struct region *)calloc(5, sizeof(struct region));
     regions [0].length = 5;
     regions [1].length = 50;
     regions [2].length = 500;
     regions [3].length = 5000;
     regions [4].length = 50000;
     for (i = 0; i < 5; i++)
         print_region(stdout, &regions [i], 100);
}
