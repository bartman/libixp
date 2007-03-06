/* This file is derived from src/lib9p/intmap.c from plan9port */
/* See LICENCE.p9p for terms of use */

#include "ixp.h"
#include <stdlib.h>
#define USED(v) if(v){}else{}

struct Intlist {
	ulong	id;
	void*		aux;
	Intlist*	link;
	uint	ref;
};

static ulong
hashid(Intmap *map, ulong id) {
	return id%map->nhash;
}

static void
nop(void *v) {
	USED(v);
}

void
initmap(Intmap *m, ulong nhash, void *hash) {
	m->nhash = nhash;
	m->hash = hash;
}

static Intlist**
llookup(Intmap *map, ulong id) {
	Intlist **lf;

	for(lf=&map->hash[hashid(map, id)]; *lf; lf=&(*lf)->link)
		if((*lf)->id == id)
			break;
	return lf;	
}

void
freemap(Intmap *map, void (*destroy)(void*)) {
	int i;
	Intlist *p, *nlink;

	if(destroy == NULL)
		destroy = nop;
	for(i=0; i<map->nhash; i++){
		for(p=map->hash[i]; p; p=nlink){
			nlink = p->link;
			destroy(p->aux);
			free(p);
		}
	}
}
void
execmap(Intmap *map, void (*run)(void*)) {
	int i;
	Intlist *p, *nlink;

	for(i=0; i<map->nhash; i++){
		for(p=map->hash[i]; p; p=nlink){
			nlink = p->link;
			run(p->aux);
		}
	}
}

void *
lookupkey(Intmap *map, ulong id) {
	Intlist *f;
	void *v;

	if((f = *llookup(map, id)))
		v = f->aux;
	else
		v = NULL;
	return v;
}

void *
insertkey(Intmap *map, ulong id, void *v) {
	Intlist *f;
	void *ov;
	ulong h;

	if((f = *llookup(map, id))){
		/* no decrement for ov because we're returning it */
		ov = f->aux;
		f->aux = v;
	}else{
		f = ixp_emallocz(sizeof(*f));
		f->id = id;
		f->aux = v;
		h = hashid(map, id);
		f->link = map->hash[h];
		map->hash[h] = f;
		ov = NULL;
	}
	return ov;	
}

int
caninsertkey(Intmap *map, ulong id, void *v) {
	Intlist *f;
	int rv;
	ulong h;

	if(*llookup(map, id))
		rv = 0;
	else{
		f = ixp_emallocz(sizeof *f);
		f->id = id;
		f->aux = v;
		h = hashid(map, id);
		f->link = map->hash[h];
		map->hash[h] = f;
		rv = 1;
	}
	return rv;	
}

void*
deletekey(Intmap *map, ulong id) {
	Intlist **lf, *f;
	void *ov;

	if((f = *(lf = llookup(map, id)))){
		ov = f->aux;
		*lf = f->link;
		free(f);
	}else
		ov = NULL;
	return ov;
}
