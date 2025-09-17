#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "bits.h"

#define MAX 640000

char set[MAX];

void
main()
{
 bits_type *a,*copy;
 int i;

 srandom(time(NULL));

 for(;;){
   int len,j,max;
   
   BITS_ALLOC(a);
   /*BITS_ZERO(a);*/

   max=random()%MAX;

   if(max==0) max++;

   len=random()%max;
   if(len==0) len++;

   fprintf(stderr,"testing: %d/%d\n",len,max);

   for(j=0;j<max;j++){
     set[j]=0;
   }

   /* testing set */
   for(j=0;j<len;j++){
     int t=random()%max;
     BITS_SET(a,t);
     set[t]=1;
   }

   /* testing clr */

   for(j=0;j<len/3;j++){
     int t=random()%max;
     BITS_CLR(a,t);
     set[t]=0;
   }

   BITS_COPY(copy,a);
   
   /* testing isset off end */

   for(j=max;j<max+10000;j++){
     if(BITS_ISSET(a,j)){
       fprintf(stderr,"error off end of array\n");
     }
     if(BITS_ISSET(copy,j)){
       fprintf(stderr,"copy: error off end of array\n");
     }
   }


   for(j=0;j<max;j++){
     if(set[j]){
       if(!BITS_ISSET(a,j))
	 fprintf(stderr,"error bit %d: %d / %d\n",j,set[j],BITS_ISSET(a,j));
     } else {
       if(BITS_ISSET(a,j))
	 fprintf(stderr,"error bit %d: %d / %d\n",j,set[j],BITS_ISSET(a,j));
     }
   }

   /* same with copy */
   for(j=0;j<max;j++){
     if(set[j]){
       if(!BITS_ISSET(copy,j))
	 fprintf(stderr,"copy: error bit %d: %d / %d\n",j,set[j],BITS_ISSET(a,j));
     } else {
       if(BITS_ISSET(copy,j))
	 fprintf(stderr,"copy: error bit %d: %d / %d\n",j,set[j],BITS_ISSET(a,j));
     }
   }

   /* zero */
   BITS_ZERO(a);
   for(j=0;j<max+1000;j++)
     if(BITS_ISSET(a,j)){
       fprintf(stderr,"zero: zero'ing error\n");
     }
   

   BITS_FREE(a);
   BITS_FREE(copy);


 }


}
