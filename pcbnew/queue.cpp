	/*******************************************/
	/* EDITEUR de PCB: routines d'AUTOROUTAGE: */
	/*******************************************/

#include "fctsys.h"
#include "gr_basic.h"

#include "common.h"
#include "pcbnew.h"
#include "autorout.h"

#include "protos.h"

#include "cell.h"

struct PcbQueue /* search queue structure */
{
	struct PcbQueue *Next;
	int Row;	/* current row					*/
	int Col;	/* current column				*/
	int Side;	/* 0=top, 1=bottom				*/
	int Dist;	/* path distance to this cell so far		*/
	int ApxDist;	/* approximate distance to target from here	*/
};

static long qlen = 0;				/* current queue length */
static struct PcbQueue *Head = NULL;
static struct PcbQueue *Tail = NULL;
static struct PcbQueue *Save = NULL;	/* hold empty queue structs */

/* Routines definies ici : */
void InitQueue();
void GetQueue( int *, int *, int *, int *, int * );
int SetQueue( int, int, int, int, int, int, int );
void ReSetQueue( int, int, int, int, int, int, int );


/************************/
void FreeQueue(void)
/************************/
/* Free the memory used for storing all the queue */
{
struct PcbQueue *p;

	InitQueue();
	while( (p = Save) != NULL )
		{
		Save = p->Next; MyFree(p);
		}
}
	/************************/
	/* void InitQueue(void) */
	/************************/

/* initialize the search queue */
void InitQueue(void)
{
struct PcbQueue *p;

	while( (p = Head) != NULL )
		{
		Head = p->Next;
		p->Next = Save; Save = p;
		}
	Tail = NULL;
	OpenNodes = ClosNodes = MoveNodes = MaxNodes = qlen = 0;
}


	/*********************************************************/
	/* void GetQueue(int *r, int *c, int *s, int *d, int *a) */
	/*********************************************************/

/* get search queue item from list */
void GetQueue(int *r, int *c, int *s, int *d, int *a)
{
struct PcbQueue *p;

	if( (p = Head) != NULL )  /* return first item in list */
		{
		*r = p->Row; *c = p->Col;
		*s = p->Side;
		*d = p->Dist; *a = p->ApxDist;
		if ((Head = p->Next) == NULL) Tail = NULL;

		/* put node on free list */
		p->Next = Save; Save = p;
		ClosNodes++; qlen--;
		}

	else /* empty list */
		{
		*r = *c = *s = *d = *a = ILLEGAL;
		}
}



/****************************************************************/
int SetQueue (int r,int c,int side,int d,int a,int r2,int c2 )
/****************************************************************/
/* add a search node to the list
	Return:
		1 si OK
		0 si defaut allocation Memoire
*/
{
struct PcbQueue *p, *q, *t;
int i, j;

	if( (p = Save) != NULL ) /* try free list first */
		{
		Save = p->Next;
		}
	else if ((p = (struct PcbQueue *) MyMalloc(sizeof(PcbQueue))) == NULL)
		return(0);

	p->Row = r;
	p->Col = c;
	p->Side = side;
	i = (p->Dist = d) + (p->ApxDist = a);
	p->Next = NULL;
	if( (q = Head) != NULL)
		{ /* insert in proper position in list */
		if (q->Dist + q->ApxDist > i)
			{ /* insert at head */
			p->Next = q; Head = p;
			}
		else { /* search for proper position */
			for (t = q, q = q->Next; q && i > (j = q->Dist + q->ApxDist);
								t = q, q = q->Next)
				;
			if (q && i == j && q->Row == r2 && q->Col == c2)
				{
				/* insert after q, which is a goal node */
				if ( (p->Next = q->Next) == NULL) Tail = p;
				q->Next = p;
				}
			else
				{ /* insert in front of q */
				if ((p->Next = q) == NULL) Tail = p;
				t->Next = p;
				}
			}
		}
	else /* empty search list */
		Head = Tail = p;
	OpenNodes++;
	if (++qlen > MaxNodes) MaxNodes = qlen;
	return(1);
}


/******************************************************************/
void ReSetQueue (int r,int c,int s,int d,int a,int r2,int c2 )
/******************************************************************/
/* reposition node in list */
{
struct PcbQueue *p, *q;

	/* first, see if it is already in the list */
	for (q = NULL, p = Head; p; q = p, p = p->Next) {
		if (p->Row == r && p->Col == c && p->Side == s) {
			/* old one to remove */
			if (q)
				{
				if ( (q->Next = p->Next) == NULL) Tail = q;
				}
			else if ((Head = p->Next) == NULL) Tail = NULL;
			p->Next = Save; Save = p;
			OpenNodes--; MoveNodes++;
			qlen--;
			break;
			}
		}
	if (!p) /* not found, it has already been closed once */
		ClosNodes--; /* we will close it again, but just count once */
	/* if it was there, it's gone now; insert it at the proper position */
	SetQueue( r, c, s, d, a, r2, c2 );
}
