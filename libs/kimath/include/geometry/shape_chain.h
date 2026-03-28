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

        SHAPE_CHAIN( const SHAPE_CHAIN& aShape ) :
            SHAPE(SH_CHAIN),
            m_closed( aShape.m_closed ),
            m_cachedBBox( aShape.m_cachedBBox )
        {
            reserve( aShape.ShapeCount() );
            for( const auto sh : aShape.CShapes() )
            {
                m_shapes.push_back( cloneBShape( sh ) );
            }
        }

        SHAPE_CHAIN() :
            SHAPE(SH_CHAIN),
            m_closed( false )
        {

        }

        SHAPE_CHAIN ( const std::vector<SHAPE_BICONNECTED*>& aShapes, bool aClosed = false ) : 
            SHAPE(SH_CHAIN),
            m_shapes( aShapes ),
            m_closed( aClosed )
        {
        }

        ~SHAPE_CHAIN()
        {
            releaseShapes();
        }

        SHAPE* Clone() const override
        {
            return new SHAPE_CHAIN( *this );
        }

        SHAPE_CHAIN& operator=( const SHAPE_CHAIN& aB )
        {
            m_closed = aB.m_closed;
            m_cachedBBox = aB.m_cachedBBox;
            m_shapes.clear();
            reserve( aB.ShapeCount() );

            for( const auto sh : aB.CShapes() )
            {
                m_shapes.push_back( cloneBShape( sh ) );
            }

            return *this;
        }

        
        static const SHAPE_CHAIN ConstructFromPoints( const std::vector<VECTOR2I>& aPts, int aWidth = 0 )
        {
            SHAPE_CHAIN chain;

            wxASSERT( aPts.size() >= 2 );

            chain.reserve( aPts.size() - 1);
            
            for( size_t i = 0; i < aPts.size() - 1; i++ )
            {
                chain.Append( SEG( aPts[i], aPts[i + 1] ), aWidth );
            }

            return chain;
        }

        static const SHAPE_CHAIN ConstructFromSLC( const SHAPE_LINE_CHAIN& aSLC ) // for legacy stuff
        {
            //wxCHECK_MSG( 0, SHAPE_CHAIN(), wxT("ConstructFromSLC Unimplemented") );
            return SHAPE_CHAIN();
        }

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
            wxASSERT( m_shapes.size() > 0 );

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


        void Append( const SHAPE_BICONNECTED& aShape )
        {
            if( !m_shapes.empty() )
            {
                wxASSERT( m_shapes.back()->GetEnd() == aShape.GetStart() );
            }

            m_shapes.push_back( static_cast<SHAPE_BICONNECTED*>(aShape.Clone() ) );
        }


        void Append( const VECTOR2I& aP, int aWidth = 0 )
        {
            if( m_shapes.empty() )
            {
                m_shapes.push_back( new SHAPE_SEGMENT( aP, aP, aWidth ) );
            }
            else
            {
                auto lastShape = m_shapes.back();
                if( lastShape->Type() == SH_SEGMENT && ( lastShape->GetStart() == lastShape->GetEnd() ) )
                {
                    // fixme: special case for construcring SLCs by subsequently appending points. To be removed once we have the router working again.
                    auto seg = static_cast<SHAPE_SEGMENT*>( lastShape );
                    seg->SetSeg( SEG( seg->GetStart(), aP ) );
                }
                else
                {
                    m_shapes.push_back( new SHAPE_SEGMENT( lastShape->GetEnd(), aP, aWidth ? aWidth : lastShape->GetWidth() ) );
                }
            }
        }

        void Append( const SEG& aSeg, int aWidth = 0 )
        {
            
            int width = aWidth;

            if ( !IsEmpty() )
            {
                auto lastShape = m_shapes.back();    

                wxASSERT( lastShape->GetEnd() == aSeg.A );
                if ( !width )
                {
                    width = lastShape->GetWidth();
                }
            }
            
            m_shapes.push_back( new SHAPE_SEGMENT( aSeg.A, aSeg.B, width ) );
        }

        void Append( const SHAPE_CHAIN& aShape )
        {
            if (aShape.IsEmpty() )
                return;

            if ( !IsEmpty() )
            {
                wxASSERT( m_shapes.back()->GetEnd() == aShape.m_shapes.front()->GetStart() );
            }
         
            m_shapes.reserve( m_shapes.size() + aShape.m_shapes.size() );

            for( const auto& sh : aShape.m_shapes )
                m_shapes.push_back( cloneBShape( sh ) );
        }

        void Append( int x, int y, int aWidth = 0 )
        {
            Append( VECTOR2I(x, y), aWidth );
        }

        void RemoveShape( int aIndex ) {};

        void Remove( int aStartIndex, int aEndIndex )
        {
            if( aEndIndex < 0 )
                aEndIndex += PointCount();

            if( aStartIndex < 0 )
                aStartIndex += PointCount();

            if( aStartIndex >= PointCount() || aEndIndex >= PointCount() || aStartIndex > aEndIndex)
                return;
         
            
            for( int i = aStartIndex; i <= aEndIndex; i++ )
                delete m_shapes[i];

            m_shapes.erase( m_shapes.begin() + aStartIndex, m_shapes.begin() + aEndIndex + 1 );
        }

        void Remove( int aIndex )
        {
            Remove( aIndex, aIndex );
        }

        bool PointOnEdge2( const VECTOR2I& aP, int aAccuracy = 0 ) const
        {
            return false;
        }

        bool PointInside2( const VECTOR2I& aPt, int aAccuracy = 0, bool aUseBBoxCache = false ) const
        {
            return false;
        }

        bool Intersects( const SHAPE_CHAIN& aChain ) const
        {
            INTERSECTIONS dummyIps;
            return Intersect( aChain, dummyIps, 1 );
        }

        int Intersect( const SHAPE_CHAIN& aChain, INTERSECTIONS& aIp, int aMaxCount = std::numeric_limits<int>::max() ) const
        {
            #if 0
            const int ourShapeCount = ShapeCount();
            const int theirShapeCount = aChain.ShapeCount();

            
            if( ourShapeCount == 0 || theirShapeCount == 0 )
                return 0;

            for( int s1 = 0; s1 < ourShapeCount; s1++ )
            {
                const SHAPE_BICONNECTED* ourShape = m_shapes[s1];

                for( int s2 = 0; s2 < ourShapeCount; s2++ )
                {
                    std::vector<VECTOR2I> tmpIps;
                    tmpIps.reserve( 2 );

                    const SHAPE_BICONNECTED* theirShape = m_shapes[s2];
        
                    int nIps = ourShape->Intersect( *theirShape, tmpIps );

                    if ( !nIps )
                        continue;

                    INTERSECTION is;
                    is.index_our = s1;
                    is.index_their = s2;
                    is.is_corner_our = false;
                    is.is_corner_their = false;
                    is.valid = true;

                }
            }
                

        

        

            if( !aExcludeColinearAndTouching && a.Collinear( b ) )
            {
                if( a.Contains( b.A ) )
                {
                    is.p = b.A;
                    is.is_corner_their = true;
                    aIp.push_back( is );
                }

                if( a.Contains( b.B ) )
                {
                    is.p = b.B;
                    is.index_their++;
                    is.is_corner_their = true;
                    aIp.push_back( is );
                }

                if( b.Contains( a.A ) )
                {
                    is.p = a.A;
                    is.is_corner_our = true;
                    aIp.push_back( is );
                }

                if( b.Contains( a.B ) )
                {
                    is.p = a.B;
                    is.index_our++;
                    is.is_corner_our = true;
                    aIp.push_back( is );
                }
            }
            else if( p )
            {
                is.p = *p;
                is.is_corner_our = false;
                is.is_corner_their = false;

                if( p == a.A )
                {
                    is.is_corner_our = true;
                }

                if( p == a.B )
                {
                    is.is_corner_our = true;
                    is.index_our++;
                }

                if( p == b.A )
                {
                    is.is_corner_their = true;
                }

                if( p == b.B )
                {
                    is.is_corner_their = true;
                    is.index_their++;
                }

                aIp.push_back( is );
            }
        }
    }

    return aIp.size();

            return false;
        }
        
#endif
return 0;
        };

        virtual bool Collide( const VECTOR2I& aP, int aClearance = 0, int* aActual = nullptr,
                            VECTOR2I* aLocation = nullptr ) const override;
        virtual bool Collide( const SEG& aSeg, int aClearance = 0, int* aActual = nullptr,
                            VECTOR2I* aLocation = nullptr ) const override;

        const BOX2I BBox( int aClearance = 0 ) const override;
    
        void TransformToPolygon( SHAPE_POLY_SET& aBuffer, int aError,
                             ERROR_LOC aErrorLoc ) const override { wxASSERT(false); };
        
        void Move( const VECTOR2I& aVector ) override { wxASSERT(false); };
        void Rotate( const EDA_ANGLE& aAngle, const VECTOR2I& aCenter = { 0, 0 } ) override { wxASSERT(false); };

        bool IsEmpty() const
        {
            return m_shapes.empty();
        }

        bool IsSolid() const override
        {
            return false;
        }

        const SHAPE_LINE_CHAIN ToSLC() const;

        VECTOR2I::coord_type Length() const
        {
            VECTOR2I::coord_type len = 0;
            for( auto sh : m_shapes )
                len += sh->Length();
            return len;
        }

        const SHAPE_CHAIN Reversed() const
        {
            SHAPE_CHAIN chain;
            chain.m_shapes.reserve( m_shapes.size() );
            for( auto sh : m_shapes )
            {
                chain.m_shapes.push_back( cloneBShape( sh->Reversed().get() ) );
            }
            return chain;
        }

        const SHAPE_CHAIN Mirrored( const SEG& axis )
        {
            return SHAPE_CHAIN();
        }

        const VECTOR2I PointAlong( int aPathLength ) const; // fixme return matching shape too
        
        const SHAPE_CHAIN Slice( int aStartIndex, int aEndIndex ) const
        {
            return SHAPE_CHAIN();


        }

        const std::optional<INTERSECTION> SelfIntersecting() const
        {
            return std::optional<INTERSECTION>();
        }

        void SetClosed( bool aClosed )
        {
            m_closed = aClosed;
        }

        /**
         * @return true when our line is closed.
         */
        bool IsClosed() const
        {
            return m_closed;
        }

        int MinWidth() const
        {
            int minW = 0;
            for( auto sh : m_shapes )
                minW = std::min( minW, sh->Length() );
            return 0;
        }

        bool CompareGeometry( const SHAPE_CHAIN& aOther ) const
        {
            return false;
        }

        bool MatchesOrientation( const VECTOR2I& aTestPoint, bool aCw )
        {
            return false;
        }

        int SubshapeContainingPoint( const VECTOR2I& aPt, int aAccuracy ) const
        {
            const int     threshold = aAccuracy + 1;
            const int64_t thresholdSq = int64_t( threshold ) * threshold;
            
            if( IsEmpty() )
            {
                return -1;
            }
            
            int i = 0;

            for( auto subshape : m_shapes )
            {
                if( subshape->GetStart() == aPt || subshape->GetEnd() == aPt )
                    return i;

                if( subshape->SquaredDistance( aPt ) <= thresholdSq )
                    return i;

                i++;
            }

            return -1;
        }

        int PathLength( const VECTOR2I& aP, int aIndex = -1 ) const
        {

            return 0;
        }

        void RemoveDuplicatePoints()
        {

        }


        const std::vector<SHAPE_BICONNECTED*>& CShapes() const { return m_shapes; }

    private:

        static SHAPE_BICONNECTED* cloneBShape( const SHAPE_BICONNECTED* aRef )
        {
            return static_cast<SHAPE_BICONNECTED*>( aRef->Clone() );
        }

        void reserve( size_t count )
        {
            m_shapes.reserve( count );
        }

        void releaseShapes()
        {
            for( auto shape : m_shapes )
                delete shape;
        }

        std::vector<SHAPE_BICONNECTED*> m_shapes;
        bool m_closed;
        BOX2I m_cachedBBox;
};


#endif // __SHAPE_LINE_CHAIN
