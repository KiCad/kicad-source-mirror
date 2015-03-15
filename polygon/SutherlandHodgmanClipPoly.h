/********************************************************************************
*  Copyright (C) 2004 Sjaak Priester
*
*  This is free software; you can redistribute it and/or modify
*  it under the terms of the GNU General Public License as published by
*  the Free Software Foundation; either version 2 of the License, or
*  (at your option) any later version.
*
*  This file is distributed in the hope that it will be useful,
*  but WITHOUT ANY WARRANTY; without even the implied warranty of
*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*  GNU General Public License for more details.
*
*  You should have received a copy of the GNU General Public License
*  along with Tinter; if not, write to the Free Software
*  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
********************************************************************************/

// SutherlandHodgman
// Class to perform polygon clipping against an upright rectangular boundary window.
// Implementation of Sutherland-Hodgman algorithm (1974).
//
// Version 1.0 (C) 2004, Sjaak Priester, Amsterdam.
// mailto:sjaak@sjaakpriester.nl
// http://www.sjaakpriester.nl

#ifndef __SUTHERLAND_HODGMAN_H__
#define __SUTHERLAND_HODGMAN_H__


#include <vector>
#include <functional>

#ifndef _GDIPLUS_H

// I designed this with GDI+ in mind. However, this particular code doesn't
// use GDI+ at all, only some of it's variable types.
// These definitions are substitutes for those of GDI+.
typedef double REAL;
class PointF
{
public:
    REAL X;
    REAL Y;

    PointF() : X( 0 )
        , Y( 0 )                   { }
    PointF( const PointF& p ) : X( p.X )
        , Y( p.Y ) { }
    PointF( REAL x, REAL y ) : X( x )
        , Y( y )     { }
    PointF operator+( const PointF& p ) const { return PointF( X + p.X, Y + p.Y ); }
    PointF operator-( const PointF& p ) const { return PointF( X - p.X, Y - p.Y ); }
    bool Equals( const PointF& p )            { return (X == p.X) && (Y == p.Y); }
};

class RectF
{
public:
    REAL X;
    REAL Y;
    REAL Width;
    REAL Height;

    RectF() { X = 0, Y = 0, Height = 0, Width = 0; }
    RectF( const RectF& r )
    {
        X = r.X; Y = r.Y; Height = r.Height, Width = r.Width;
    }


    RectF( REAL x, REAL y, REAL w, REAL h ) : X( x ), Y( y ),Width( w ), Height( h )
    { }
    REAL GetLeft() const { return X; }
    REAL GetTop() const { return Y; }
    REAL GetRight() const { return X + Width; }
    REAL GetBottom() const { return Y + Height; }
};

#endif // _GDIPLUS_H

typedef std::vector<PointF>                 pointVector;
typedef std::vector<PointF>::iterator       pointIterator;
typedef std::vector<PointF>::const_iterator cpointIterator;

class SutherlandHodgman
{
public:

    // Constructor. Parameter is the boundary rectangle.
    // SutherlandHodgman expects a 'normalized' boundary rectangle, meaning
    // that boundaries.GetRight() > boundaries.GetLeft() and
    // boundaries.GetBottom() > boundaries.GetTop().
    // In other words: boundary.Width > 0 and boundaries.Height > 0.
    // If this is violated, nothing will be output.
    SutherlandHodgman( RectF& boundaries ) :
        m_stageBottom( m_stageOut, boundaries.GetBottom() )
        , /* Initialize each stage */ m_stageLeft( m_stageBottom, boundaries.GetLeft() )
        , /* with its next stage and */ m_stageTop( m_stageLeft, boundaries.GetTop() )
        , /* the boundary position. */ m_stageRight( m_stageTop, boundaries.GetRight() )
    {
    }


    void Clip( pointVector& input, pointVector& clipped )
    {
        clipped.clear();
        m_stageOut.SetDestination( &clipped );

        // Clip each input vertex.
        for( cpointIterator it = input.begin(); it != input.end(); ++it )
            m_stageRight.HandleVertex( *it );

        // Do the final step.
        m_stageRight.Finalize();
    }


private:

    // Implementation of a horizontal boundary (top or bottom).
    // Comp is a std::binary_function object, comparing its two parameters, f.i. std::less.

    template <class Comp>

    class BoundaryHor
    {
public:
        BoundaryHor( REAL y ) : m_Y( y ) { }
        bool IsInside( const PointF& pnt ) const
        {
            return Comp ()( pnt.Y, m_Y );
        }                                       // return true if pnt.Y is at the inside of the boundary
        PointF Intersect( const PointF& p0, const PointF& p1 ) const            // return intersection point of line p0...p1 with boundary
        {                                                                       // assumes p0...p1 is not strictly horizontal
            PointF d      = p1 - p0;
            REAL   xslope = d.X / d.Y;

            PointF r;

            r.Y = m_Y;
            r.X = p0.X + xslope * (m_Y - p0.Y);
            return r;
        }


private:
        REAL m_Y;
    };

    // Implementation of a vertical boundary (left or right).
    template <class Comp>
    class BoundaryVert
    {
public:
        BoundaryVert( REAL x ) : m_X( x )
        { }
        bool IsInside( const PointF& pnt ) const
        {
            return Comp() ( pnt.X, m_X );
        }
        PointF Intersect( const PointF& p0, const PointF& p1 ) const      // assumes p0...p1 is not strictly vertical
        {
            PointF d      = p1 - p0;
            REAL   yslope = d.Y / d.X;

            PointF r;

            r.X = m_X;
            r.Y = p0.Y + yslope * (m_X - p0.X);
            return r;
        }


private:
        REAL m_X;
    };

    // This template class is the workhorse of the algorithm. It handles the clipping against one boundary.
    // Boundary is either BoundaryHor or BoundaryVert, Stage is the next ClipStage, or the output stage.
    template <class Boundary, class Stage>
    class ClipStage : private Boundary
    {
public:
        ClipStage( Stage& nextStage, REAL position ) :
            Boundary( position ) , m_NextStage( nextStage ), m_bFirst( true ), m_bPreviousInside( false )
        { }

        // Function to handle one vertex
        void HandleVertex( const PointF& pntCurrent )
        {
            bool bCurrentInside = this->IsInside( pntCurrent );       // See if vertex is inside the boundary.

            if( m_bFirst )                                      // If this is the first vertex...
            {
                m_pntFirst = pntCurrent;                        // ... just remember it,...

                m_bFirst = false;
            }
            else                                // Common cases, not the first vertex.
            {
                if( bCurrentInside )            // If this vertex is inside...
                {
                    if( !m_bPreviousInside )    // ... and the previous one was outside
                        m_NextStage.HandleVertex( this->Intersect( m_pntPrevious, pntCurrent ) );

                    // ... first output the intersection point.

                    m_NextStage.HandleVertex( pntCurrent );     // Output the current vertex.
                }
                else if( m_bPreviousInside )                    // If this vertex is outside, and the previous one was inside...
                    m_NextStage.HandleVertex( this->Intersect( m_pntPrevious, pntCurrent ) );

                // ... output the intersection point.

                // If neither current vertex nor the previous one are inside, output nothing.
            }
            m_pntPrevious     = pntCurrent; // Be prepared for next vertex.
            m_bPreviousInside = bCurrentInside;
        }


        void Finalize()
        {
            HandleVertex( m_pntFirst );         // Close the polygon.
            m_NextStage.Finalize();             // Delegate to the next stage.
        }


private:
        Stage& m_NextStage;         // the next stage
        bool   m_bFirst;            // true if no vertices have been handled
        PointF m_pntFirst;          // the first vertex
        PointF m_pntPrevious;       // the previous vertex
        bool   m_bPreviousInside;   // true if the previous vertex was inside the Boundary
    };

    class OutputStage
    {
public:
        OutputStage() : m_pDest( 0 )  { }
        void SetDestination( pointVector* pDest ) { m_pDest = pDest; }
        void HandleVertex( const PointF& pnt )    { m_pDest->push_back( pnt ); }    // Append the vertex to the output container.
        void Finalize() { }                                                         // Do nothing.
private:
        pointVector* m_pDest;
    };

    // These typedefs define the four boundaries. In keeping up with the GDI/GDI+ interpretation of
    // rectangles, we include the left and top boundaries, but not the right and bottom boundaries.
    // In other words: a vertex on the left boundary is considered to be inside, but a vertex
    // on the right boundary is considered to be outside.
    typedef BoundaryVert<std::less<REAL> >               BoundaryRight;
    typedef BoundaryHor<std::greater_equal<REAL> >       BoundaryTop;
    typedef BoundaryVert<std::greater_equal<REAL> >      BoundaryLeft;
    typedef BoundaryHor<std::less<REAL> >                BoundaryBottom;

    // Next typedefs define the four stages. First template parameter is the boundary,
    // second template parameter is the next stage.
    typedef ClipStage<BoundaryBottom, OutputStage>  ClipBottom;
    typedef ClipStage<BoundaryLeft, ClipBottom>     ClipLeft;
    typedef ClipStage<BoundaryTop, ClipLeft>        ClipTop;
    typedef ClipStage<BoundaryRight, ClipTop>       ClipRight;

    // Our data members.
    OutputStage m_stageOut;
    ClipBottom  m_stageBottom;
    ClipLeft    m_stageLeft;
    ClipTop     m_stageTop;
    ClipRight   m_stageRight;
};

#endif
