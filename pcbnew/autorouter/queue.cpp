/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015 Jean-Pierre Charras, jean-pierre.charras@ujf-grenoble.fr
 * Copyright (C) 2012 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2011 Wayne Stambaugh <stambaughw@verizon.net>
 *
 * Copyright (C) 1992-2015 KiCad Developers, see change_log.txt for contributors.
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
 * @file queue.cpp
 */

#include <fctsys.h>
#include <common.h>

#include <pcbnew.h>
#include <autorout.h>
#include <cell.h>


struct PcbQueue /* search queue structure */
{
    struct PcbQueue* Next;
    int              Row;       /* current row                  */
    int              Col;       /* current column               */
    int              Side;      /* 0=top, 1=bottom              */
    int              Dist;      /* path distance to this cell so far        */
    int              ApxDist;   /* approximate distance to target from here */
};

static long             qlen = 0;   /* current queue length */
static struct PcbQueue* Head = NULL;
static struct PcbQueue* Tail = NULL;
static struct PcbQueue* Save = NULL;    /* hold empty queue structs */


/* Free the memory used for storing all the queue */
void FreeQueue()
{
    struct PcbQueue* p;

    InitQueue();

    while( (p = Save) != NULL )
    {
        Save = p->Next;
        delete p;
    }
}


/* initialize the search queue */
void InitQueue()
{
    struct PcbQueue* p;

    while( (p = Head) != NULL )
    {
        Head    = p->Next;
        p->Next = Save; Save = p;
    }

    Tail = NULL;
    OpenNodes = ClosNodes = MoveNodes = MaxNodes = qlen = 0;
}


/* get search queue item from list */
void GetQueue( int* r, int* c, int* s, int* d, int* a )
{
    struct PcbQueue* p;

    if( (p = Head) != NULL )  /* return first item in list */
    {
        *r = p->Row; *c = p->Col;
        *s = p->Side;
        *d = p->Dist; *a = p->ApxDist;

        if( (Head = p->Next) == NULL )
            Tail = NULL;

        /* put node on free list */
        p->Next = Save; Save = p;
        ClosNodes++; qlen--;
    }
    else /* empty list */
    {
        *r = *c = *s = *d = *a = ILLEGAL;
    }
}


/* add a search node to the list
 *  :
 *      1 - OK
 *      0 - Failed to allocate memory.
 */
bool SetQueue( int r, int c, int side, int d, int a, int r2, int c2 )
{
    struct PcbQueue* p, * q, * t;
    int i, j;

    j = 0;                      // gcc warning fix

    if( (p = Save) != NULL )    /* try free list first */
    {
        Save = p->Next;
    }
    else if( ( p = (PcbQueue*) operator new( sizeof( PcbQueue ), std::nothrow ) ) == NULL )
    {
        return 0;
    }

    p->Row  = r;
    p->Col  = c;
    p->Side = side;
    i = (p->Dist = d) + (p->ApxDist = a);
    p->Next = NULL;

    if( (q = Head) != NULL ) /* insert in proper position in list */
    {
        if( q->Dist + q->ApxDist > i ) /* insert at head */
        {
            p->Next = q; Head = p;
        }
        else   /* search for proper position */
        {
            for( t = q, q = q->Next; q && i > ( j = q->Dist + q->ApxDist ); t = q, q = q->Next )
                ;

            if( q && i == j && q->Row == r2 && q->Col == c2 )
            {
                /* insert after q, which is a goal node */
                if( ( p->Next = q->Next ) == NULL )
                    Tail = p;

                q->Next = p;
            }
            else  /* insert in front of q */
            {
                if( ( p->Next = q ) == NULL )
                    Tail = p;

                t->Next = p;
            }
        }
    }
    else /* empty search list */
    {
        Head = Tail = p;
    }

    OpenNodes++;

    if( ++qlen > MaxNodes )
        MaxNodes = qlen;

    return 1;
}


/* reposition node in list */
void ReSetQueue( int r, int c, int s, int d, int a, int r2, int c2 )
{
    struct PcbQueue* p, * q;

    /* first, see if it is already in the list */
    for( q = NULL, p = Head; p; q = p, p = p->Next )
    {
        if( p->Row == r && p->Col == c && p->Side == s )
        {
            /* old one to remove */
            if( q )
            {
                if( ( q->Next = p->Next ) == NULL )
                    Tail = q;
            }
            else if( ( Head = p->Next ) == NULL )
            {
                Tail = NULL;
            }

            p->Next = Save;
            Save = p;
            OpenNodes--;
            MoveNodes++;
            qlen--;
            break;
        }
    }

    if( !p )            /* not found, it has already been closed once */
        ClosNodes--;    /* we will close it again, but just count once */

    /* if it was there, it's gone now; insert it at the proper position */
    bool res = SetQueue( r, c, s, d, a, r2, c2 );
    (void) res;
}
