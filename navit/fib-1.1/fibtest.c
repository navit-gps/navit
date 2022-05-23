/*-
 * Copyright 1997, 1998-2003 John-Mark Gurney.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.

*/


#include <stdio.h>
#include <stdlib.h>
#include "fib.h"

int main(void)
{
	struct fibheap *a;
	void *arr[10];
	int i;

	a = fh_makekeyheap();
	
	for (i=1 ; i < 10 ; i++)
	  {
              arr[i]= fh_insertkey(a,0,(void *)i);
	      printf("adding: 0 %d \n",i);
	  }
     
	printf(" \n");
	 fh_replacekey(a, arr[1],-1);
         fh_replacekey(a, arr[6],-1);
	 fh_replacekey(a, arr[4],-1);
         fh_replacekey(a, arr[2],-1); 
         fh_replacekey(a, arr[8],-1); 
	  
        printf("value(minkey) %d\n",fh_minkey(a));
	printf("id: %d\n\n", (int)fh_extractmin(a));          
     
	 fh_replacekey(a, arr[7],-33);
/* -> node 7 becomes root node, but is still pointed to by node 6 */
         fh_replacekey(a, arr[4],-36);
	 fh_replacekey(a, arr[3],-1);
         fh_replacekey(a, arr[9],-81); 	
	
        printf("value(minkey) %d\n",fh_minkey(a));
        printf("id: %d\n\n", (int)fh_extractmin(a));
	
	 fh_replacekey(a, arr[6],-68);
         fh_replacekey(a, arr[2],-69);

        printf("value(minkey) %d\n",fh_minkey(a));
        printf("id: %d\n\n", (int)fh_extractmin(a));

	 fh_replacekey(a, arr[1],-52);
         fh_replacekey(a, arr[3],-2);
	 fh_replacekey(a, arr[4],-120);
         fh_replacekey(a, arr[5],-48); 	

        printf("value(minkey) %d\n",fh_minkey(a));
	printf("id: %d\n\n", (int)fh_extractmin(a));

	 fh_replacekey(a, arr[3],-3);
         fh_replacekey(a, arr[5],-63);

        printf("value(minkey) %d\n",fh_minkey(a));
	printf("id: %d\n\n", (int)fh_extractmin(a));

	 fh_replacekey(a, arr[5],-110);
         fh_replacekey(a, arr[7],-115);

        printf("value(minkey) %d\n",fh_minkey(a));
	printf("id: %d\n\n", (int)fh_extractmin(a));

         fh_replacekey(a, arr[5],-188);

        printf("value(minkey) %d\n",fh_minkey(a));
	printf("id: %d\n\n", (int)fh_extractmin(a));

         fh_replacekey(a, arr[3],-4);

        printf("value(minkey) %d\n",fh_minkey(a));
	printf("id: %d\n\n", (int)fh_extractmin(a));
	
        printf("value(minkey) %d\n",fh_minkey(a));
        printf("id: %d\n\n", (int)fh_extractmin(a));

	fh_deleteheap(a);

	return 0;
}
