/* Routines for encoding Start-Stop-Step codes. (See Bell, Cleary, Witten
   1990, "Text Compression", page 294) */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "io.h"
#include "coder.h"
#include "SSS_coder.h"
 
void
SSS_encode (unsigned int n, unsigned int start, unsigned int step, unsigned int stop)
/* Encodes the number n into the stream according to the Start-Stop-Step code. */
{
    unsigned step1, lowerbound, power, bits, p;

    lowerbound = 0;
    bits = start;
    step1 = start;
    power = POWER_OF_2 (1, start);
    assert (power > 0); /* check for overflow */

    /* Encode the unary prefix code */
    while (power && (n >= lowerbound + power) && (step1 < stop))
      {
	/* encode a 0-bit in the prefix unary code */
	arithmetic_encode (Stdout_File, 0, 1, 2);

	step1 += step;
	lowerbound += power;
	bits += step;
	power = POWER_OF_2 (power, step);
      }

    /* Encode the end of the unary prefix code */
    if (step1 < stop)
        /* encode a 1-bit in the prefix unary code */
	arithmetic_encode (Stdout_File, 1, 2, 2);

    /* Now encode the binary part of the code */
    n = (n - lowerbound);

    for (p = bits; p > 0; p--)
      {
	if (BIT_ISSET (n, p-1))
	    /* encode a 1-bit in the suffix binary code */
	    arithmetic_encode (Stdout_File, 1, 2, 2);
	else
	    /* encode a 0-bit in the suffix binary code */
	    arithmetic_encode (Stdout_File, 0, 1, 2);
      }
}

unsigned int
SSS_decode (unsigned int start, unsigned int step, unsigned int stop)
/* Decodes and returns the number from the stream according to the Start-Stop-Step code. */
{
    unsigned step1, lowerbound, power, bits, n, p;

    n = 0;

    lowerbound = 0;
    bits = start;
    step1 = start;
    power = POWER_OF_2 (1, start);
    assert (power > 0); /* check for overflow */

    /* Decode the prefix unary code */
    while (power && (step1 < stop))
      {
	if (arithmetic_decode_target (Stdin_File, 2) < 1)
	    arithmetic_decode (Stdin_File, 0, 1, 2); /* decode a 0-bit */
	else
	  {
	    arithmetic_decode (Stdin_File, 1, 2, 2); /* decode a 1-bit */
	    break;
	  }
	step1 += step;
	lowerbound += power;
	bits += step;
	power = POWER_OF_2 (power, step);
      }

    /* Now decode the binary part of the code */

    for (p = bits; p > 0; p--)
      {
	if (arithmetic_decode_target (Stdin_File, 2) < 1)
	    arithmetic_decode (Stdin_File, 0, 1, 2); /* decode a 0 bit */
	else
	  {
	    BIT_SET (n, p-1); /* encode a 1-bit into n */
	    arithmetic_decode (Stdin_File, 1, 2, 2); /* decode a 1 bit */
	  }
      }

    return (n + lowerbound);
}
