#include <stdio.h>
#include <string.h>
#include "coord.h"
#include "town.h"
#include "street.h"
#include "command.h"

/* Phonetisch
	"KLEIN " -> CL
	(x)(x)->x
	EI:AY
	E:-
	F:V
	G:C
	H:-
	I:Y
	J:Y
	K:C
	T:D
	W:V
*/

struct tree_hdr {
	unsigned int addr;
	unsigned int size;
	unsigned int low;
};

struct tree_leaf {
	unsigned int low;
	unsigned int data;
	unsigned char text[0];
};

#if 0
static int
compare(char *s1, char *s2)
{
	char s1_exp, s2_exp;
	for (;;) {
		s1_exp=*s1++;
		s2_exp=*s2++;
		if (! s1_exp && ! s2_exp)
			return 0;
		if (s1_exp == 'A' && *s1 == 'E') { s1_exp='Ä'; s1++; }
		if (s1_exp == 'O' && *s1 == 'E') { s1_exp='Ö'; s1++; }
		if (s1_exp == 'U' && *s1 == 'E') { s1_exp='Ü'; s1++; }
		if (s2_exp == 'A' && *s2 == 'E') { s2_exp='Ä'; s2++; }
		if (s2_exp == 'O' && *s2 == 'E') { s2_exp='Ö'; s2++; }
		if (s2_exp == 'U' && *s2 == 'E') { s2_exp='Ü'; s2++; }
		if (s1_exp != s2_exp)
			return s1_exp-s2_exp;
	}
#if 0
	for (;;) {
		while (*s1 && *s1 == *s2) {
			s1++;
			s2++;
		}
		if (! *s1 && ! *s2)
			return 0;
		if (! *s1)
			return -1;
		if (! *s2)
			return 1;
		if (*s1 < *s2 && *s2 < 0x80)
			return -1;
		if (*s1 > *s2 && *s1 < 0x80)
			return 1;
		s1_exp=*s1;
		s2_exp=*s2;
		if (s1_exp >= 0x80)
			s1_exp=' ';
		if (s2_exp >= 0x80)
			s2_exp=' ';
#if 0
		if (*s1 == (unsigned char)'Ä') s1_exp='A';
		if (*s1 == (unsigned char)'Ö') s1_exp='O';
		if (*s1 == (unsigned char)'Ü') s1_exp='U';
		if (*s1 == (unsigned char)'ä') s1_exp='a';
		if (*s1 == (unsigned char)'ö') s1_exp='o';
		if (*s1 == (unsigned char)'ü') s1_exp='u';
		if (*s1 == (unsigned char)'ß') s1_exp='s';
		if (*s2 == (unsigned char)'Ä') s2_exp='A';
		if (*s2 == (unsigned char)'Ö') s2_exp='O';
		if (*s2 == (unsigned char)'Ü') s2_exp='U';
		if (*s2 == (unsigned char)'ä') s2_exp='a';
		if (*s2 == (unsigned char)'ö') s2_exp='o';
		if (*s2 == (unsigned char)'ü') s2_exp='u';
		if (*s2 == (unsigned char)'ß') s2_exp='s';
#endif
		if (s1_exp < s2_exp)
			return -1;
		if (s1_exp > s2_exp)
			return 1;
		printf("Problem %c vs %c\n", *s1, *s2);
		exit(1);		
	}
#endif
}
#endif

void
command_goto(struct container *co, const char *arg)
{
#if 0
	struct container *co=map->container;
	struct town_list *list,*curr,*best=NULL;
	int order=256;
	int argc=0;
	unsigned long x,y,scale;
	char args[strlen(arg)+1];
	char *argv[strlen(arg)+1];
	char *tok;

	
	strcpy(args, arg);
	tok=strtok(args, " ");
	while (tok) {
		argv[argc++]=tok;
		tok=strtok(NULL," ");
	}

	list=NULL;
	town_search_by_name(&list, co->map_data, argv[0], 0, 10000);
	town_get_by_index(co->map_data, 0x311d54cb);
	curr=list;
	while (curr) {
		if (curr->town->order < order) {
			order=curr->town->order;
			best=curr;
		}
		curr=curr->next;
	}
	if (best) {
		printf("'%s' '%s' '%s' '%s'\n", best->town->name, best->town->district, best->town->postal_code1, best->town->postal_code2);
#if 0
		street_search_by_name(class->map_data, best->town->country, best->town->street_assoc, 0, argv[1]);
#endif
		map_get_view(map, &x, &y, &scale);
		x=best->town->c->x;
		y=best->town->c->y;
		map_set_view(map, x, y, scale);
	}
	town_list_free(list);
#endif
}
