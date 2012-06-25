/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012 Jean-Pierre Charras, jean-pierre.charras@ujf-grenoble.fr
 * Copyright (C) 2012 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2011 Wayne Stambaugh <stambaughw@verizon.net>
 *
 * Copyright (C) 1992-2012 KiCad Developers, see change_log.txt for contributors.
 *
 * First copyright (C) Randy Nevin, 1989 (see PCBCA package)
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */


/**
 * @file work.cpp
 * @brief Automatic routing routines
 */

#include <fctsys.h>
#include <common.h>

#include <pcbnew.h>
#include <autorout.h>
#include <cell.h>


struct CWORK    // a unit of work is a source-target (a ratsnet item) to connect
{
    struct CWORK*   m_Next;
    int             m_FromRow;      // source row
    int             m_FromCol;      // source column
    int             m_ToRow;        // target row
    int             m_ToCol;        // target column
    RATSNEST_ITEM*  m_Ratsnest;     // Corresponding ratsnest
    int             m_NetCode;      // m_NetCode
    int             m_ApxDist;      // approximate distance
    int             m_Cost;         // cost for sort by length
    int             m_Priority;     // route priority
};


// pointers to the first and last item of work to do
static CWORK*   Head    = NULL;
static CWORK*   Tail    = NULL;
static CWORK*   Current = NULL;


// initialize the work list
void InitWork()
{
    CWORK* ptr;

    while( ( ptr = Head ) != NULL )
    {
        Head = ptr->m_Next;
        delete ptr;
    }

    Tail = Current = NULL;
}


// initialize the work list
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

int SetWork( int r1, int c1,
             int n_c,
             int r2, int c2,
             RATSNEST_ITEM* pt_ch, int pri )
{
    CWORK* p;

    if( ( p = (CWORK*) operator new( sizeof(CWORK), std::nothrow ) ) != NULL )
    {
        p->m_FromRow    = r1;
        p->m_FromCol    = c1;
        p->m_NetCode    = n_c;
        p->m_ToRow      = r2;
        p->m_ToCol      = c2;
        p->m_Ratsnest   = pt_ch;
        p->m_ApxDist    = GetApxDist( r1, c1, r2, c2 );
        p->m_Cost       = GetCost( r1, c1, r2, c2 );
        p->m_Priority   = pri;
        p->m_Next       = NULL;

        if( Head )  /* attach at end */
            Tail->m_Next = p;
        else        /* first in list */
            Head = Current = p;

        Tail = p;
        return 1;
    }
    else    /* can't get any more memory */
    {
        return 0;
    }
}


/* fetch a unit of work from the work list */
void GetWork( int* r1, int* c1,
              int* n_c,
              int* r2, int* c2,
              RATSNEST_ITEM** pt_ch )
{
    if( Current )
    {
        *r1     = Current->m_FromRow;
        *c1     = Current->m_FromCol;
        *n_c    = Current->m_NetCode;
        *r2     = Current->m_ToRow;
        *c2     = Current->m_ToCol;
        *pt_ch  = Current->m_Ratsnest;
        Current = Current->m_Next;
    }
    else    /* none left */
    {
        *r1     = *c1 = *r2 = *c2 = ILLEGAL;
        *n_c    = 0;
        *pt_ch  = NULL;
    }
}


/* order the work items; shortest (low cost) first */
void SortWork()
{
    CWORK*  p;
    CWORK*  q0; /* put PRIORITY PAD_CONNECTs in q0 */
    CWORK*  q1; /* sort other PAD_CONNECTs in q1 */
    CWORK*  r;

    q0 = q1 = NULL;

    while( (p = Head) != NULL )    /* prioritize each work item */
    {
        Head = Head->m_Next;

        if( p->m_Priority )    /* put at end of priority list */
        {
            p->m_Next = NULL;

            if( (r = q0) == NULL )    /* empty list? */
            {
                q0 = p;
            }
            else                    /* attach at end */
            {
                while( r->m_Next )  /* search for end */
                    r = r->m_Next;

                r->m_Next = p;    /* attach */
            }
        }
        else if( ( ( r = q1 ) == NULL ) || ( p->m_Cost < q1->m_Cost ) )
        {
            p->m_Next = q1;
            q1 = p;
        }
        else    /* find proper position in list */
        {
            while( r->m_Next && p->m_Cost >= r->m_Next->m_Cost )
                r = r->m_Next;

            p->m_Next   = r->m_Next;
            r->m_Next   = p;
        }
    }

    if( (p = q0) != NULL )    /* any priority PAD_CONNECTs? */
    {
        while( q0->m_Next )
            q0 = q0->m_Next;

        q0->m_Next = q1;
    }
    else
        p = q1;

    /* reposition Head and Tail */
    for( Head = Current = Tail = p; Tail && Tail->m_Next; Tail = Tail->m_Next )
        ;
}


/* Calculate the cost of a ratsnest:
 *   cost = (| dx | + | dy |) * disability
 *   disability = 1 if dx or dy = 0, max if | dx | # | dy |
 */
static int GetCost( int r1, int c1, int r2, int c2 )
{
    int     dx, dy, mx, my;
    double  incl = 1.0;

    dx  = abs( c2 - c1 );
    dy  = abs( r2 - r1 );
    mx  = dx;
    my  = dy;

    if( mx < my )
    {
        mx = dy; my = dx;
    }

    if( mx )
        incl += (2 * (double) my / mx);

    return (int) ( ( dx + dy ) * incl );
}
