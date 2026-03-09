/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013 CERN
 * @author Tomasz Wlostowski <tomasz.wlostowski@cern.ch>
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef __SHAPE_CHAIN
#define __SHAPE_CHAIN


#include <geometry/seg.h>
#include <geometry/shape.h>
#include <geometry/shape_arc.h>
#include <geometry/shape_segment.h>
#include <geometry/shape_line_chain.h>
#include <geometry/direction45.h>
#include <math/vector2d.h>

struct QPOINT
{

};

class SHAPE_CHAIN : public SHAPE
{
    public:

        struct INTERSECTION
        {
            SHAPE_BICONNECTED *a, *b;

            /// Point of intersection between our and their.
            VECTOR2I p;

            /// Index of the intersecting corner/segment in the 'our' (== this) line.
            int index_our;

            /// index of the intersecting corner/segment in the 'their' (Intersect() method
            /// parameter) line.
            int index_their;

            /// When true, the corner [index_our] of the 'our' line lies exactly on 'their' line.
            bool is_corner_our;

            /// When true, the corner [index_their] of the 'their' line lies exactly on 'our' line.
            /// Note that when both is_corner_our and is_corner_their are set, the line chains touch
            /// with with corners.
            bool is_corner_their;

            /// Auxiliary flag to avoid copying intersection info to intersection refining code,
            /// used by the refining code (e.g. hull handling stuff in the P&S) to reject false
            /// intersection points.
            bool valid;

            INTERSECTION() :
                index_our( -1 ),
                index_their( -1 ),
                is_corner_our( false ),
                is_corner_their( false ),
                valid( false )
            {
            }

        };

        typedef std::vector<INTERSECTION> INTERSECTIONS;

        SHAPE_CHAIN() :
            SHAPE(SH_CHAIN)
        {

        }

        SHAPE_CHAIN ( const std::vector<SHAPE_BICONNECTED*>aShapes ) : 
            SHAPE(SH_CHAIN),
            m_shapes( aShapes )
        {

        }

        static const SHAPE_CHAIN ConstructFromPoints( const std::vector<VECTOR2I>& aPts );
        static const SHAPE_CHAIN ConstructFromSLC( const SHAPE_LINE_CHAIN& aSLC ); // for legacy stuff

        int PointCount() const
        {
            int c = m_shapes.size() + 1;

            if( m_closed )
                c--;

            return std::max( 0, c );
        }

        int CountShapes( SHAPE_TYPE aType ) const { return 0; }

        int ShapeCount() const
        {
            return m_shapes.size();            
        }


        const VECTOR2I& CPoint( int aIndex ) const
        {
            int nPoints = PointCount();
            if( aIndex < 0 )
                aIndex += nPoints;
            else if( aIndex >= nPoints )
                aIndex -= nPoints;

            if( aIndex == nPoints - 1 )
                return m_shapes[aIndex - 1]->GetEnd();
            else
                return m_shapes[aIndex]->GetStart();
        }

        SHAPE_BICONNECTED* Shape( int aIndex ) const
        {
            if( aIndex < 0 )
                aIndex += ShapeCount();
            else if( aIndex >= ShapeCount() )
                aIndex -= ShapeCount();

            return m_shapes[aIndex];
        }

        // fixme: should be a property of BICONNECTED
        bool IsArc( int aShapeIndex ) const { return Shape(aShapeIndex)->Type() == SH_ARC; }
        bool IsSegment( int aShapeIndex ) const { return Shape(aShapeIndex)->Type() == SH_SEGMENT; }

        const SHAPE_SEGMENT& CSegment( int aIndex ) const { 
            auto shape = Shape( aIndex ); 
            wxASSERT( shape->Type() == SH_SEGMENT );
            return static_cast<const SHAPE_SEGMENT&>(*shape);
        }

        const SHAPE_ARC& CArc( int aIndex ) const { 
            auto shape = Shape( aIndex ); 
            wxASSERT( shape->Type() == SH_ARC );
            return static_cast<const SHAPE_ARC&>(*shape);
        }

        const SHAPE_BICONNECTED* CShape( int aIndex ) const { return Shape( aIndex ); }

        void Clear()
        {
            m_shapes.clear();
            m_closed = false;
        }


        const VECTOR2I NearestPoint( const VECTOR2I& aP, bool aAllowInternalShapePoints = true ) const;
        const VECTOR2I NearestPoint( const SEG& aSeg, int& aDistance ) const;

        int NearestShape( const VECTOR2I& aP ) const;

        const VECTOR2I& CLastPoint() const
        {
            return CPoint( -1 );
        }

        // schain fix immutable?
        void Replace( int aStartIndex, int aEndIndex, const SHAPE_CHAIN& aLine );
        void Replace( int aStartIndex, int aEndIndex, const VECTOR2I& aP );

        int Split( const VECTOR2I& aP, bool aExact = false );
        void Split( const VECTOR2I& aStart, const VECTOR2I& aEnd, SHAPE_CHAIN& aPre,
                SHAPE_CHAIN& aMid, SHAPE_CHAIN& aPost ) const;


        void Simplify( int aTolerance = 0 );

        int Find( const VECTOR2I& aP, int aThreshold = 0 ) const;


        void Append( const SHAPE_BICONNECTED& aShape );
        void Append( const VECTOR2I& aP );
        void Append( const SEG& aSeg );
        void Append( const SHAPE_CHAIN& aShape );
        void Append( int x, int y );

        void Remove( int aStartIndex, int aEndIndex );
        void Remove( int aIndex )
        {
            Remove( aIndex, aIndex );
        }

        bool PointOnEdge2( const VECTOR2I& aP, int aAccuracy = 0 ) const;
        bool PointInside2( const VECTOR2I& aPt, int aAccuracy = 0, bool aUseBBoxCache = false ) const;

        bool Intersects( const SHAPE_CHAIN& aChain ) const;
        int Intersect( const SHAPE_CHAIN& aChain, INTERSECTIONS& aIp ) const;
        virtual bool Collide( const VECTOR2I& aP, int aClearance = 0, int* aActual = nullptr,
                            VECTOR2I* aLocation = nullptr ) const override;
        virtual bool Collide( const SEG& aSeg, int aClearance = 0, int* aActual = nullptr,
                            VECTOR2I* aLocation = nullptr ) const override;

        const BOX2I BBox( int aClearance = 0 ) const override;


    
        void TransformToPolygon( SHAPE_POLY_SET& aBuffer, int aError,
                             ERROR_LOC aErrorLoc ) const override { wxASSERT(false); };
        void Move( const VECTOR2I& aVector ) override { wxASSERT(false); };
        void Rotate( const EDA_ANGLE& aAngle, const VECTOR2I& aCenter = { 0, 0 } ) override { wxASSERT(false); };

        bool IsSolid() const override
        {
            return false;
        }

        const SHAPE_LINE_CHAIN ToSLC() const;

        VECTOR2I::coord_type Length() const;

        const SHAPE_CHAIN Reversed() const;
        const SHAPE_CHAIN Mirrored( const SEG& axis );

        const VECTOR2I PointAlong( int aPathLength ) const; // fixme return matching shape too
        const SHAPE_CHAIN Slice( int aStartIndex, int aEndIndex ) const;

        const std::optional<INTERSECTION> SelfIntersecting() const;

        DIRECTION_45::AngleType AngleAtVertex( int aVertex ) const;

        void SetClosed( bool aClosed )
        {
            m_closed = aClosed;
            //mergeFirstLastPointIfNeeded();
        }

        /**
         * @return true when our line is closed.
         */
        bool IsClosed() const
        {
            return m_closed;
        }

        int MinWidth() const;

        bool CompareGeometry( const SHAPE_CHAIN& aOther ) const;


        bool MatchesOrientation( const VECTOR2I& aTestPoint, bool aCw );

        void RemoveShape( int aIndex );

        int PathLength( const VECTOR2I& aP, int aIndex = -1 ) const;

        void RemoveDuplicatePoints();


        const std::vector<SHAPE_BICONNECTED*>& CShapes() const { return m_shapes; }

    private:
        std::vector<SHAPE_BICONNECTED*> m_shapes;
        bool m_closed;
};


#endif // __SHAPE_LINE_CHAIN
