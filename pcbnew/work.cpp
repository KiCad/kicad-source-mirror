/************************/
/* Autorouting routines */
/************************/

#include "fctsys.h"
#include "gr_basic.h"

#include "common.h"
#include "pcbnew.h"
#include "autorout.h"
#include "cell.h"

#include "protos.h"


struct CWORK /* a unit of work is a hole-pair to connect */
{
    struct CWORK*  Next;
    int            FromRow;     /* source row		*/
    int            FromCol;     /* source column	*/
    int            net_code;    /* net_code			*/
    int            ToRow;       /* target row		*/
    int            ToCol;       /* target column	*/
    RATSNEST_ITEM* pt_rats;     /* Corresponding ratsnest */
    int            ApxDist;     /* approximate distance	*/
    int            Cost;        /* cost for sort by length */
    int            Priority;    /* route priority */
};


/* pointers to the first and last item of work to do */
static CWORK* Head    = NULL;
static CWORK* Tail    = NULL;
static CWORK* Current = NULL;


void InitWork();
void ReInitWork();
int  SetWork( int, int, int, int, int, RATSNEST_ITEM*, int );
void GetWork( int*, int*, int*, int*, int*, RATSNEST_ITEM** );
void SortWork();


/* initialize the work list */
void InitWork()
{
    CWORK* ptr;

    while( ( ptr = Head ) != NULL )
    {
        Head = ptr->Next;
        MyFree( ptr );
    }

    Tail = Current = NULL;
}


/* initialize the work list */
void ReInitWork()
{
    Current = Head;
}


/* add a unit of work to the work list
 * Return:
 *  1 if OK
 *  0 if memory allocation failed
 */
static int GetCost( int r1, int c1, int r2, int c2 );

int SetWork( int            r1,
             int            c1,
             int            n_c,
             int            r2,
             int            c2,
             RATSNEST_ITEM* pt_ch,
             int            pri )
{
    CWORK* p;

    if( ( p = (CWORK*) MyMalloc( sizeof(CWORK) ) ) != NULL )
    {
        p->FromRow  = r1;
        p->FromCol  = c1;
        p->net_code = n_c;
        p->ToRow    = r2;
        p->ToCol    = c2;
        p->pt_rats  = pt_ch;
        p->ApxDist  = GetApxDist( r1, c1, r2, c2 );
        p->Cost     = GetCost( r1, c1, r2, c2 );
        p->Priority = pri;
        p->Next     = NULL;
        if( Head )  /* attach at end */
            Tail->Next = p;
        else        /* first in list */
            Head = Current = p;
        Tail = p;
        return 1;
    }
    else /* can't get any more memory */
        return 0;
}


/* fetch a unit of work from the work list */
void GetWork( int*            r1,
              int*            c1,
              int*            n_c,
              int*            r2,
              int*            c2,
              RATSNEST_ITEM** pt_ch )
{
    if( Current )
    {
        *r1     = Current->FromRow;
        *c1     = Current->FromCol;
        *n_c    = Current->net_code;
        *r2     = Current->ToRow;
        *c2     = Current->ToCol;
        *pt_ch  = Current->pt_rats;
        Current = Current->Next;
    }
    else   /* none left */
    {
        *r1    = *c1 = *r2 = *c2 = ILLEGAL;
        *n_c   = 0;
        *pt_ch = NULL;
    }
}


/* order the work items; shortest (low cost) first */
void SortWork()
{
    CWORK* p;
    CWORK* q0;  /* put PRIORITY PAD_CONNECTs in q0 */
    CWORK* q1;  /* sort other PAD_CONNECTs in q1 */
    CWORK* r;

    q0 = q1 = NULL;
    while( (p = Head) != NULL ) /* prioritize each work item */
    {
        Head = Head->Next;
        if( p->Priority ) /* put at end of priority list */
        {
            p->Next = NULL;
            if( (r = q0) == NULL )  /* empty list? */
                q0 = p;
            else                    /* attach at end */
            {
                while( r->Next )    /* search for end */
                    r = r->Next;

                r->Next = p; /* attach */
            }
        }
        else if( ( ( r = q1 ) == NULL ) || ( p->Cost < q1->Cost ) )
        {
            p->Next = q1;
            q1 = p;
        }
        else   /* find proper position in list */
        {
            while( r->Next && p->Cost >= r->Next->Cost )
                r = r->Next;

            p->Next = r->Next;
            r->Next = p;
        }
    }

    if( (p = q0) != NULL ) /* any priority PAD_CONNECTs? */
    {
        while( q0->Next )
            q0 = q0->Next;

        q0->Next = q1;
    }
    else
        p = q1;

    /* reposition Head and Tail */
    for( Head = Current = Tail = p; Tail && Tail->Next; Tail = Tail->Next )
        ;
}


/* Calculate the cost of a net:
 *   cost = (| dx | + | dy |) * disability
 *   disability = 1 if dx or dy = 0, max if | dx | # | dy |
 */
static int GetCost( int r1, int c1, int r2, int c2 )
{
    int   dx, dy, mx, my;
    float incl;

    dx   = abs( c2 - c1 );
    dy   = abs( r2 - r1 );
    incl = 1.0;
    mx   = dx;
    my   = dy;

    if( mx < my )
    {
        mx = dy; my = dx;
    }
    if( mx )
        incl += (2 * (float) my / mx);

    return (int) ( ( dx + dy ) * incl );
}
