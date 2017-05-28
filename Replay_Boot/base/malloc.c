
#include "config.h"
#include "messaging.h"
#include <unistd.h>
#include <malloc.h>

static struct mallinfo s_mallinfo = {0};

/* titel: malloc()/free()-Paar nach K&R 2, p.185ff */

typedef long Align;

union header {			/* Kopf eines Allokationsblocks */
    struct {
	union header	*ptr;  	/* Zeiger auf zirkulaeren Nachfolger */
	unsigned 	size;	/* Groesse des Blocks	*/
    } s;
    Align x;			/* Erzwingt Block-Alignierung	*/
};

typedef union header Header;

static Header base;		/* Anfangs-Header	*/
static Header *freep = NULL;	/* Aktueller Einstiegspunkt in Free-Liste */

#define NALLOC 	1024	/* Mindestgroesse fuer morecore()-Aufruf	*/

/* Eine static-Funktion ist ausserhalb ihres Files nicht sichtbar	*/


void * _sbrk_r(
        struct _reent *_s_r,
        ptrdiff_t nbytes);

static Header *morecore(unsigned nu)
{
    void *cp;
    Header *up;
    if (nu < NALLOC)
	nu = NALLOC;
    cp = (void*) _sbrk_r(NULL, nu * sizeof(Header));	
    if (cp == (char *) -1)		/* sbrk liefert -1 im Fehlerfall */
	return NULL;
	s_mallinfo.arena += nu * sizeof(Header);
    s_mallinfo.uordblks += nu * sizeof(Header);
    up = (Header*) cp;
    up->s.size = nu;			/* Groesse wird eingetragen	*/
    free((void*)(up+1));		/* Einbau in Free-Liste		*/
    return freep;
}

void* malloc(unsigned nbytes) {
    Header *p, *prevp;		
    unsigned nunits;
    
     /* Kleinstes Vielfaches von sizeof(Header), das die
	geforderte Byte-Zahl enthalten kann, plus 1 fuer den Header selbst: */
    nunits = (nbytes+sizeof(Header)-1)/sizeof(Header) + 1;

    if ((prevp = freep) == NULL) {
        // First call, init
	base.s.ptr = freep = prevp = &base;
	base.s.size = 0;		/* base wird Block der Laenge 0 */
    }
    for (p = prevp->s.ptr; ; prevp = p, p = p->s.ptr) {
	
	/* p durchlaeuft die Free-Liste, gefolgt von prevp, keine
		Abbruchbedingung!!	*/

	if (p->s.size >= nunits) {	/* Ist p gross genug? 		*/
	    if (p->s.size == nunits) 	/* Falls exakt, wird er... 	*/
		prevp->s.ptr = p->s.ptr;/* ... der Liste entnommen 	*/
	    else {			/* andernfalls...	   	*/
		p->s.size -= nunits;	/* wird p verkleinert		*/
		p += p->s.size;		/* und der letzte Teil ... 	*/
		p->s.size = nunits;	/* ... des Blocks...		*/
	    }
	    freep = prevp;
		s_mallinfo.uordblks += p->s.size * sizeof(Header);
		s_mallinfo.fordblks -= p->s.size * sizeof(Header);
	    return (void*) (p+1);	/* ... zurueckgegeben, allerdings 
					   unter der Adresse von p+1,
					   da p auf den Header zeigt.  	*/
	}
	if ( p == freep)		/* Falls die Liste keinen Block
				           ausreichender Groesse enthaelt,
					   wird morecore() aufgrufen	*/
	    if ((p = morecore(nunits)) == NULL)
		return NULL;
    }
}

void *calloc(size_t nmemb, size_t size)
{
  size_t nbytes = nmemb*size;
  void *ptr = malloc(nbytes);
  if(ptr != NULL) {
    bzero(ptr, nbytes);
  }
  return ptr;
}


void free(void *ap) {			/* Rueckgabe an Free-Liste	*/
    Header *bp, *p;
    
    bp = (Header*) ap - 1;		/* Hier ist der Header des Blocks */
	s_mallinfo.uordblks -= bp->s.size * sizeof(Header);
	s_mallinfo.fordblks += bp->s.size * sizeof(Header);

	/* Die Liste wird durchmustert, der Block soll der
	   Adressgroesse nach richtig eingefuegt werden,
	   um Defragmentierung zu ermoeglichen.				*/

    for (p = freep; !(bp > p && bp < p->s.ptr); p = p->s.ptr)
	if (p >= p->s.ptr && (bp > p || bp < p->s.ptr))
	    break;	/* bp liegt vor Block mit kleinster oder hinter 
			   Block mit groesster Adresse */

    if (bp + bp->s.size == p->s.ptr) {	
				/* Vereinigung mit oberem Nachbarn 	*/
	bp->s.size += p->s.ptr->s.size;
	bp->s.ptr = p->s.ptr->s.ptr;
    }
    else
	bp->s.ptr = p->s.ptr;
    if ( p + p->s.size == bp ) {
				/* Vereinigung mit unterem Nachbarn 	*/
	p->s.size += bp->s.size;
	p->s.ptr = bp->s.ptr;
    } else
	p->s.ptr = bp;
    freep = p;
}

void* sbrk(intptr_t increment)
{
	return _sbrk_r(NULL, increment);
}

struct mallinfo mallinfo(void)
{
	return s_mallinfo;
}
