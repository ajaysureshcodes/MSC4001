/* Routines for encoding Start-Stop-Step codes. (See Bell, Cleary, Witten
   1990, "Text Compression", page 294) */

#include <stdio.h>
#include <stdlib.h>

#define POWER_OF_2(n,b) ((unsigned int) n << b)

#define BIT_SIZE (sizeof (unsigned int) *8)
#define BIT_DELTA(d) ((d) / BIT_SIZE)
#define BIT_MASK(d) (1UL << ((d) % BIT_SIZE))
#define BIT_SET(n,p) (n |= BIT_MASK(p))
#define BIT_CLR(n,p) (n &= ~BIT_MASK(p))
#define BIT_ISSET(n,p) (n & BIT_MASK(p))

void
SSS_encode (unsigned int n, unsigned int start, unsigned int step, unsigned int stop);
/* Encodes the number n into the stream according to the Start-Stop-Step code. */

unsigned int
SSS_decode (unsigned int start, unsigned int step, unsigned int stop);
/* Decodes and returns the number from the stream according to the Start-Stop-Step code. */
