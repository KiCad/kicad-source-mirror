	/********************************************/
	/* AUTOROUTAGE PCB : routines d'autoroutage */
	/********************************************/

	/* fichier WORK.CC */

#include "fctsys.h"
#include "gr_basic.h"

#include "common.h"
#include "pcbnew.h"
#include "autorout.h"
#include "cell.h"

#include "protos.h"
/**/

struct CWORK /* a unit of work is a hole-pair to connect */
	{
	struct CWORK *Next;
	int FromRow;		/* source row		*/
	int FromCol;		/* source column	*/
	int net_code;		/* net_code			*/
	int ToRow;			/* target row		*/
	int ToCol;			/* target column	*/
	CHEVELU *pt_rats;	/* chevelu correspondant*/
	int ApxDist;		/* approximate distance	*/
	int Cost;			/* cost for sort by length */
	int Priority;       /* routage priority */
	};

/* pointers to the first and last item of work to do */
static CWORK *Head = NULL;
static CWORK *Tail = NULL;
static CWORK *Current = NULL;

/* Routines definies ici : */

void InitWork(void);
void ReInitWork(void);
int SetWork( int, int, int, int, int, CHEVELU *, int );
void GetWork( int *, int *, int *, int *, int *, CHEVELU ** );
void SortWork( void );


	/************************/
	/* void InitWork (void) */
	/************************/

 /* initialize the work list */
void InitWork(void)
{
CWORK *ptr;

	while( (ptr = Head) != NULL )
		{
		Head = ptr->Next; MyFree( ptr );
		}
	Tail = Current = NULL;
}


	/*************************/
	/* void ReInitWork(void) */
	/*************************/

 /* initialize the work list */
void ReInitWork(void)
	{
	Current = Head;
	}

/*****************************************************************************/
/*int SetWork(int r1,int c1,int* n_c,int r2,int c2,CHEVELU * pt_ch,int pri )*/
/*****************************************************************************/

/* add a unit of work to the work list
	Return:
		1 si OK
		0 si defaut d'allocation memoire
*/
static int GetCost(int r1,int c1,int r2,int c2);

int SetWork(int r1,int c1,int n_c,int r2,int c2,CHEVELU * pt_ch,int pri )
{
CWORK *p;

	if( (p = (CWORK *)MyMalloc( sizeof(CWORK) )) != NULL )
		{
		p->FromRow = r1;
		p->FromCol = c1;
		p->net_code = n_c;
		p->ToRow = r2;
		p->ToCol = c2;
		p->pt_rats = pt_ch;
		p->ApxDist = GetApxDist( r1, c1, r2, c2 );
		p->Cost = GetCost( r1, c1, r2, c2 );
		p->Priority = pri;
		p->Next = NULL;
		if (Head) /* attach at end */
			Tail->Next = p;
		else /* first in list */
			Head = Current = p;
		Tail = p;
		return(1);
		}
	else /* can't get any more memory */
		return(0);
}


	/************************************************************************/
	/* void GetWork (int *r1,int *c1,int *r2,int *c2, char **n1,char **n2 ) */
	/************************************************************************/

void GetWork (int *r1,int *c1,int *n_c,int *r2,int *c2,CHEVELU** pt_ch )
	/* fetch a unit of work from the work list */
{
	if (Current)
		{
		*r1 = Current->FromRow;
		*c1 = Current->FromCol;
		*n_c = Current->net_code;
		*r2 = Current->ToRow;
		*c2 = Current->ToCol;
		*pt_ch = Current->pt_rats;
		Current = Current->Next;
		}

	else { /* none left */
		*r1 = *c1 = *r2 = *c2 = ILLEGAL;
		*n_c = 0;
		*pt_ch = NULL;
		}
}


	/***********************/
	/* void SortWork(void) */
	/***********************/

/* order the work items; shortest (low cost) first */
void SortWork(void)
{
CWORK *p;
CWORK *q0; /* put PRIORITY CONNECTs in q0 */
CWORK *q1; /* sort other CONNECTs in q1 */
CWORK *r;

	q0 = q1 = NULL;
	while( (p = Head) != NULL )
		{ /* prioritize each work item */
		Head = Head->Next;
		if (p->Priority)
			{ /* put at end of priority list */
			p->Next = NULL;
			if ((r = q0) == NULL) /* empty list? */
				q0 = p;
			else
				{ /* attach at end */
				while (r->Next) /* search for end */
					r = r->Next;
				r->Next = p; /* attach */
				}
			}
		else if ( ((r = q1) == NULL) || (p->Cost < q1->Cost) )
			{
			p->Next = q1; q1 = p;
			}
		else { /* find proper position in list */
			while (r->Next && p->Cost >= r->Next->Cost) r = r->Next;
			p->Next = r->Next; r->Next = p;
			}
		}

	if( (p = q0) != NULL)
		{ /* any priority CONNECTs? */
		while (q0->Next) q0 = q0->Next;
		q0->Next = q1;
		}
	else p = q1;

	/* reposition Head and Tail */
	for(Head = Current = Tail = p; Tail && Tail->Next; Tail = Tail->Next) ;
}


/* routine de calcul du cout d'un chevelu:
cout = (|dx| + |dy|) * handicap
handicap = 1 si dx ou dy = 0, max si |dx| # |dy|
*/
static int GetCost(int r1,int c1,int r2,int c2)
{
int dx, dy, mx, my;
float incl;

	dx = abs(c2 - c1);
	dy = abs(r2 - r1);
	incl = 1.0;
	mx = dx; my = dy;
	if ( mx < my ) { mx = dy; my = dx;}
    if ( mx ) incl += (2*(float)my/mx);

	return (int)((dx+dy) * incl) ;
}


