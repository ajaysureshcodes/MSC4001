/* Test program. */
#include <stdio.h>
#include <stdlib.h>

#ifdef SYSTEM_LINUX
#include <getopt.h> /* for getopt on Linux systems */
#endif

#include "io.h"
#include "coderanges.h"

int
main ()
{
    unsigned int coderanges, new_coderanges, lbnd, hbnd, totl;
    float codelength;

    for (;;)
      {
	coderanges = TLM_create_coderanges ();
	for (;;)
	  {
	    printf ("lbnd? ");
	    scanf ("%d", &lbnd);
	    
	    printf ("hbnd? ");
	    scanf ("%d", &hbnd);
	    
	    printf ("totl? ");
	    scanf ("%d", &totl);
	    
	    if (!totl)
	      break;
	    
	    TLM_append_coderange (coderanges, lbnd, hbnd, totl);
	    
	    TLM_dump_coderanges (Stdout_File, coderanges);
	    
	    TLM_reset_coderanges (coderanges);
	    while (TLM_next_coderange (coderanges, &lbnd, &hbnd, &totl))
	      printf ("lbnd = %d hbnd = %d totl = %d\n", lbnd, hbnd, totl);
	    
	    codelength = TLM_codelength_coderanges (coderanges);
	    printf ("codelength = %.3f\n", codelength);
	  }

	new_coderanges = TLM_copy_coderanges (coderanges);
	TLM_dump_coderanges (Stdout_File, new_coderanges);
	
	printf ("Betail 1:\n");
	TLM_betail_coderanges (new_coderanges);
	TLM_dump_coderanges (Stdout_File, new_coderanges);

	printf ("Betail 2:\n");
	TLM_betail_coderanges (new_coderanges);
	TLM_dump_coderanges (Stdout_File, new_coderanges);

	printf ("Betail 3:\n");
	TLM_betail_coderanges (new_coderanges);
	TLM_dump_coderanges (Stdout_File, new_coderanges);


	TLM_release_coderanges (coderanges);
	TLM_release_coderanges (new_coderanges);
      }
}

