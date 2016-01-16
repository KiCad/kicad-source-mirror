// ----------------------------------------------------------------------------------------
// Name        : rect_placement.cpp
// Description : A class that fits subrectangles into a power-of-2 rectangle
// (C) Copyright 2000-2002 by Javier Arevalo
// This code is free to use and modify for all purposes
// ----------------------------------------------------------------------------------------

/*
 *  You have a bunch of rectangular pieces. You need to arrange them in a
 *  rectangular surface so that they don't overlap, keeping the total area of the
 *  rectangle as small as possible. This is fairly common when arranging characters
 *  in a bitmapped font, lightmaps for a 3D engine, and I guess other situations as
 *  well.
 *
 *  The idea of this algorithm is that, as we add rectangles, we can pre-select
 *  "interesting" places where we can try to add the next rectangles. For optimal
 *  results, the rectangles should be added in order. I initially tried using area
 *  as a sorting criteria, but it didn't work well with very tall or very flat
 *  rectangles. I then tried using the longest dimension as a selector, and it
 *  worked much better. So much for intuition...
 *
 *  These "interesting" places are just to the right and just below the currently
 *  added rectangle. The first rectangle, obviously, goes at the top left, the next
 *  one would go either to the right or below this one, and so on. It is a weird way
 *  to do it, but it seems to work very nicely.
 *
 *  The way we search here is fairly brute-force, the fact being that for most off-
 *  line purposes the performance seems more than adequate. I have generated a
 *  japanese font with around 8500 characters and all the time was spent generating
 *  the bitmaps.
 *
 *  Also, for all we care, we could grow the parent rectangle.
 *
 *  I'd be interested in hearing of other approaches to this problem. Make sure
 *  to post them on http://www.flipcode.com
 */

#include "rect_placement.h"

// --------------------------------------------------------------------------------
// Name        :
// Description :
// --------------------------------------------------------------------------------
void CRectPlacement::Init( int w, int h )
{
    End();
    m_size = TRect( 0, 0, w, h );
    m_vPositions.push_back( TPos( 0, 0 ) );
    m_area = 0;
}


// --------------------------------------------------------------------------------
// Name        :
// Description :
// --------------------------------------------------------------------------------
void CRectPlacement::End()
{
    m_vPositions.clear();
    m_vRects.clear();
    m_size.w = 0;
}


// --------------------------------------------------------------------------------
// Name        : IsFree
// Description : Check if the given rectangle is partially or totally used
// --------------------------------------------------------------------------------
bool CRectPlacement::IsFree( const TRect& r ) const
{
    if( !m_size.Contains( r ) )
        return false;

    for( CRectArray::const_iterator it = m_vRects.begin();
         it != m_vRects.end(); ++it )
    {
        if( it->Intersects( r ) )
            return false;
    }

    return true;
}


// --------------------------------------------------------------------------------
// Name        : AddPosition
// Description : Add new anchor point
// --------------------------------------------------------------------------------
void CRectPlacement::AddPosition( const TPos& p )
{
    // Try to insert anchor as close as possible to the top left corner
    // So it will be tried first
    bool bFound = false;
    CPosArray::iterator it;

    for( it = m_vPositions.begin();
         !bFound && it != m_vPositions.end();
         ++it )
    {
        if( p.x + p.y < it->x + it->y )
            bFound = true;
    }

    if( bFound )
        m_vPositions.insert( it, p );
    else
        m_vPositions.push_back( p );
}


// --------------------------------------------------------------------------------
// Name        : AddRect
// Description : Add the given rect and updates anchor points
// --------------------------------------------------------------------------------
void CRectPlacement::AddRect( const TRect& r )
{
    m_vRects.push_back( r );
    m_area += r.w * r.h;

    // Add two new anchor points
    AddPosition( TPos( r.x, r.y + r.h ) );
    AddPosition( TPos( r.x + r.w, r.y ) );
}


// --------------------------------------------------------------------------------
// Name        : AddAtEmptySpot
// Description : Add the given rectangle
// --------------------------------------------------------------------------------
bool CRectPlacement::AddAtEmptySpot( TRect& r )
{
    // Find a valid spot among available anchors.
    bool bFound = false;
    CPosArray::iterator it;

    for( it = m_vPositions.begin();
         !bFound && it != m_vPositions.end();
         ++it )
    {
        TRect Rect( it->x, it->y, r.w, r.h );

        if( IsFree( Rect ) )
        {
            r = Rect;
            bFound = true;
            break; // Don't let the loop increase the iterator.
        }
    }

    if( bFound )
    {
        int x, y;

        // Remove the used anchor point
        m_vPositions.erase( it );

        // Sometimes, anchors end up displaced from the optimal position
        // due to irregular sizes of the subrects.
        // So, try to adjut it up & left as much as possible.
        for( x = 1; x <= r.x; x++ )
        {
            if( !IsFree( TRect( r.x - x, r.y, r.w, r.h ) ) )
                break;
        }

        for( y = 1; y <= r.y; y++ )
        {
            if( !IsFree( TRect( r.x, r.y - y, r.w, r.h ) ) )
                break;
        }

        if( y > x )
            r.y -= y - 1;
        else
            r.x -= x - 1;

        AddRect( r );
    }

    return bFound;
}

// --------------------------------------------------------------------------------
// Name        : AddAtEmptySpotAutoGrow
// Description : Add a rectangle of the given size, growing our area if needed
// Area grows only until the max given.
// Returns the placement of the rect in the rect's x,y coords
// --------------------------------------------------------------------------------
bool CRectPlacement::AddAtEmptySpotAutoGrow( TRect* pRect, int maxW, int maxH )
{
    double growing_factor = 1.2;    // Must be > 1.0, and event > 1.1 for fast optimization

    #define GROW(x) ((x * growing_factor) + 1)

    if( pRect->w <= 0 )
        return true;

    int orgW    = m_size.w;
    int orgH    = m_size.h;

    // Try to add it in the existing space
    while( !AddAtEmptySpot( *pRect ) )
    {
        int pw  = m_size.w;
        int ph  = m_size.h;

        // Sanity check - if area is complete.
        if( pw >= maxW && ph >= maxH )
        {
            m_size.w    = orgW;
            m_size.h    = orgH;
            return false;
        }

        // Try growing the smallest dim
        if( pw < maxW && ( pw < ph || ( (pw == ph) && (pRect->w >= pRect->h) ) ) )
            m_size.w = GROW( pw );
        else
            m_size.h = GROW( ph );

        if( AddAtEmptySpot( *pRect ) )
            break;

        // Try growing the other dim instead
        if( pw != m_size.w )
        {
            m_size.w = pw;

            if( ph < maxW )
                m_size.h = GROW( ph );
        }
        else
        {
            m_size.h = ph;

            if( pw < maxW )
                m_size.w = GROW( pw );
        }

        if( pw != m_size.w || ph != m_size.h )
            if( AddAtEmptySpot( *pRect ) )
                break;



        // Grow both if possible, and reloop.
        m_size.w    = pw;
        m_size.h    = ph;

        if( pw < maxW )
            m_size.w = GROW( pw );

        if( ph < maxH )
            m_size.h = GROW( ph );
    }

    return true;
}
