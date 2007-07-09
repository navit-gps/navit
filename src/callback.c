#include <glib.h>
#include "debug.h"
#include "callback.h"

struct callback {
	void (*func)();
	int pcount;
	void *p[0];
	
};

struct callback_list {
	GList *list;
};

struct callback_list *
callback_list_new(void)
{
	struct callback_list *ret=g_new0(struct callback_list, 1);
	
	return ret;
}

struct callback *
callback_new(void (*func)(), int pcount, void **p)
{
	struct callback *ret;
	int i;

	ret=g_malloc0(sizeof(struct callback)+pcount*sizeof(void *));
	ret->func=func;
	ret->pcount=pcount;
	for (i = 0 ; i < pcount ; i++) {
		ret->p[i]=p[i];
	}	
	return ret;
}

void
callback_list_add(struct callback_list *l, struct callback *cb)
{
	l->list=g_list_prepend(l->list, cb);
}


struct callback *
callback_list_add_new(struct callback_list *l, void (*func)(), int pcount, void **p)
{
	struct callback *ret;
	
	ret=callback_new(func, pcount, p);	
	callback_list_add(l, ret);
	return ret;
}

void
callback_list_remove_destroy(struct callback_list *l, struct callback *cb)
{
	l->list=g_list_remove(l->list, cb);
	g_free(cb);
}


void
callback_list_call(struct callback_list *l, int pcount, void **p)
{
	void *pf[8];
	struct callback *cb;
	GList *cbi;
	int i;

	cbi=l->list;
	while (cbi) {
		cb=cbi->data;
		if (cb->pcount + pcount <= 8) {
			dbg(1,"cb->pcount=%d %p pcount=%d %p\n", cb->pcount, cb->p[0], pcount, p[0]);
			for (i = 0 ; i < cb->pcount ; i++) 
				pf[i]=cb->p[i];
			for (i = 0 ; i < pcount ; i++)
				pf[i+cb->pcount]=p[i];
			switch (cb->pcount+pcount) {
			case 8:
				cb->func(pf[0],pf[1],pf[2],pf[3],pf[4],pf[5],pf[6],pf[7]);
				break;
			case 7:
				cb->func(pf[0],pf[1],pf[2],pf[3],pf[4],pf[5],pf[6]);
				break;
			case 6:
				cb->func(pf[0],pf[1],pf[2],pf[3],pf[4],pf[5]);
				break;
			case 5:
				cb->func(pf[0],pf[1],pf[2],pf[3],pf[4]);
				break;
			case 4:
				cb->func(pf[0],pf[1],pf[2],pf[3]);
				break;
			case 3:
				cb->func(pf[0],pf[1],pf[2]);
				break;
			case 2:
				cb->func(pf[0],pf[1]);
				break;
			case 1:
				cb->func(pf[0]);
				break;
			case 0:
				cb->func();
				break;
			}
		} else {
			g_warning("too many parameters for callback\n");
		}
		cbi=g_list_next(cbi);
	}
	
}


void 
callback_list_destroy(struct callback_list *l)
{
	GList *cbi;
	cbi=l->list;
	while (cbi) {
		g_free(cbi->data);
		cbi=g_list_next(cbi);
	}
	g_list_free(l->list);
	g_free(l);
}
