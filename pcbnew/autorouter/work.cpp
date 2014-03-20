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


struct CWORK    // a unit of work is a source-target to connect
                // this is a ratsnest item in the routing matrix world
{
    int             m_FromRow;      // source row
    int             m_FromCol;      // source column
    int             m_ToRow;        // target row
    int             m_ToCol;        // target column
    RATSNEST_ITEM*  m_Ratsnest;     // Corresponding ratsnest
    int             m_NetCode;      // m_NetCode
    int             m_ApxDist;      // approximate distance
    int             m_Cost;         // cost for sort by length
    int             m_Priority;     // route priority

    // the function that calculates the cost of this ratsnest:
    void CalculateCost();
};


// the list of ratsnests
static std::vector <CWORK> WorkList;
static unsigned Current = 0;


// initialize the work list
void InitWork()
{
    WorkList.clear();
    Current = 0;
}


/* add a unit of work to the work list
 * :
 *  1 if OK
 *  0 if memory allocation failed
 */

int SetWork( int r1, int c1,
             int n_c,
             int r2, int c2,
             RATSNEST_ITEM* pt_ch, int pri )
{
    CWORK item;
    item.m_FromRow    = r1;
    item.m_FromCol    = c1;
    item.m_NetCode    = n_c;
    item.m_ToRow      = r2;
    item.m_ToCol      = c2;
    item.m_Ratsnest   = pt_ch;
    item.m_ApxDist    = RoutingMatrix.GetApxDist( r1, c1, r2, c2 );
    item.CalculateCost();
    item.m_Priority   = pri;
    WorkList.push_back( item );
    return 1;
}


/* fetch a unit of work from the work list */
void GetWork( int* r1, int* c1,
              int* n_c,
              int* r2, int* c2,
              RATSNEST_ITEM** pt_ch )
{
    if( Current < WorkList.size() )
    {
        *r1     = WorkList[Current].m_FromRow;
        *c1     = WorkList[Current].m_FromCol;
        *n_c    = WorkList[Current].m_NetCode;
        *r2     = WorkList[Current].m_ToRow;
        *c2     = WorkList[Current].m_ToCol;
        *pt_ch  = WorkList[Current].m_Ratsnest;
        Current++;
    }
    else    /* none left */
    {
        *r1     = *c1 = *r2 = *c2 = ILLEGAL;
        *n_c    = 0;
        *pt_ch  = NULL;
    }
}


// order the work items; shortest (low cost) first:
bool sort_by_cost( const CWORK& ref, const CWORK& item )
{
    if( ref.m_Priority == item.m_Priority )
        return ref.m_Cost < item.m_Cost;

    return ref.m_Priority >= item.m_Priority;
}

void SortWork()
{
    sort( WorkList.begin(), WorkList.end(), sort_by_cost );
}


/* Calculate the cost of a ratsnest:
 *   cost = (| dx | + | dy |) * disability
 *   disability = 1 if dx or dy = 0, max if | dx | # | dy |
 */
void CWORK::CalculateCost()
{
    int     dx, dy, mx, my;
    double  incl = 1.0;

    dx  = abs( m_ToCol - m_FromCol );
    dy  = abs( m_ToRow - m_FromRow );
    mx  = dx;
    my  = dy;

    if( mx < my )
    {
        mx = dy; my = dx;
    }

    if( mx )
        incl += (2 * (double) my / mx);

    m_Cost = (int) ( ( dx + dy ) * incl );
}
