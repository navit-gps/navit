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
#include <time.h>

#include "fib.h"

#define TESTCASE	1

#define COUNT	100000
#define DIF	1000
#define MAXEXT	10
#define VERBOSE	1

int cmp(void *, void *);

int
cmp(void *x, void *y)
{
	int a, b;
	a = (int)x;
	b = (int)y;

	if (a < b)
		return -1;
	if (a == b)
		return 0;
	return 1;
}

int
main(void)
{
#if !TESTCASE
	struct fibheap_el *w;
#else
	int e, j, k;
#endif
	struct fibheap *a;
	int i, x;

	a = fh_makeheap();
	fh_setcmp(a, cmp);

	srandom(time(NULL));
#if TESTCASE
#if VERBOSE
	printf("inserting: ");
#endif
	e = 0;
	for (i = 0; i < COUNT; i++) {
#if VERBOSE
		if (i)
			printf(", ");
#endif
		fh_insert(a, (void *)(x = random()/10));
#if VERBOSE
		printf("%d", x);
#endif
		if (i - e > DIF) {
			k = random() % MAXEXT;
			for (j = 0; j < k; j++, e++)
				printf("throwing: %d\n", (int)fh_extractmin(a));
		}
	}

#if VERBOSE
	printf("\nremaining: %d\n", COUNT - e);
	printf("extracting: ");
#endif
	for (i = 0; i < COUNT - e; i++) {
#if VERBOSE
		if (i)
			printf(", ");
		printf("%d", (int)fh_extractmin(a));
#else
		fh_extractmin(a);
#endif
	}
#if VERBOSE
	printf("\n");
#endif
	if ((int)fh_extractmin(a) == 0)
		printf("heap empty!\n");
	else {
		printf("heap not empty! ERROR!\n");
		exit(1);
	}
#else
	w = fh_insert(a, (void *)6);
	printf("adding: %d\n", 6);
	fh_insert(a, (void *)9);
	printf("adding: %d\n", 9);
	fh_insert(a, (void *)1);
	printf("adding: %d\n", 1);
	for(i = 0; i < 5; i++) {
		x = random()/10000;
		printf("adding: %d\n", x);
		fh_insert(a, (void *)x);
	}
	fh_insert(a, (void *)4);
	printf("adding: %d\n", 4);
	fh_insert(a, (void *)8);
	printf("adding: %d\n", 8);
	printf("first: %d\n", (int)fh_extractmin(a));
	printf("deleting: %d\n", (int)fh_delete(a, w));
	printf("first: %d\n", (int)fh_extractmin(a));
	printf("first: %d\n", (int)fh_extractmin(a));
	printf("first: %d\n", (int)fh_extractmin(a));
	printf("first: %d\n", (int)fh_extractmin(a));
	printf("first: %d\n", (int)fh_extractmin(a));
	for(i = 0; i < 5; i++) {
		x = random()/10000;
		printf("adding: %d\n", x);
		fh_insert(a, (void *)x);
	}
	printf("first: %d\n", (int)fh_extractmin(a));
	printf("first: %d\n", (int)fh_extractmin(a));
	printf("first: %d\n", (int)fh_extractmin(a));
	printf("first: %d\n", (int)fh_extractmin(a));
	printf("first: %d\n", (int)fh_extractmin(a));
#endif

	fh_deleteheap(a);

	return 0;
}
