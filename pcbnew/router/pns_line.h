/*
 * KiRouter - a push-and-(sometimes-)shove PCB router
 *
 * Copyright (C) 2013-2017 CERN
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * Author: Tomasz Wlostowski <tomasz.wlostowski@cern.ch>
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __PNS_LINE_H
#define __PNS_LINE_H

#include <math/box2.h>
#include <math/vector2d.h>

#include <geometry/direction45.h>
#include <geometry/seg.h>
#include <geometry/shape.h>
#include <geometry/shape_line_chain.h>

#include "pns_item.h"
#include "pns_via.h"
#include "pns_link_holder.h"

namespace PNS {

class LINKED_ITEM;
class NODE;
class VIA;
class SEGMENT;

#define PNS_HULL_MARGIN 10

/**
 * Represents a track on a PCB, connecting two non-trivial joints (that is,
 * vias, pads, junctions between multiple traces or two traces different widths
 * and combinations of these). PNS_LINEs are NOT stored in the model (NODE).
 * Instead, they are assembled on-the-fly, based on a via/pad/segment that
 * belongs to/starts/ends them.
 *
 * PNS_LINEs can be either loose (consisting of segments that do not belong to
 * any NODE) or owned (with segments taken from a NODE) - these are returned by
 * #NODE::AssembleLine and friends.
 *
 * A LINE may have a VIA attached at its end (i.e. the last point) - this is used by via
 * dragging/force propagation stuff.
 */
class LINE : public LINK_HOLDER
{
public:
    /**
     * Makes an empty line.
     */
    LINE() :
        LINK_HOLDER( LINE_T ),
        m_blockingObstacle( nullptr )
    {
        m_width = 1;        // Dummy value
        m_snapThreshhold = 0;
        m_via = nullptr;
    }

    LINE( const LINE& aOther );

    /**
     * Copy properties (net, layers, etc.) from a base line and replaces the shape by another.
     */
    LINE( const LINE& aBase, const SHAPE_LINE_CHAIN& aLine ) :
        LINK_HOLDER( aBase ),
        m_line( aLine ),
        m_width( aBase.m_width ),
        m_snapThreshhold( aBase.m_snapThreshhold ),
        m_blockingObstacle( nullptr )
    {
        m_net = aBase.m_net;
        m_layers = aBase.m_layers;
        m_via = nullptr;
    }

    /**
     * Construct a LINE for a lone VIA (ie a stitching via).
     */
    LINE( VIA* aVia ) :
        LINK_HOLDER( LINE_T ),
        m_blockingObstacle( nullptr )
    {
        m_via = aVia;
        // TODO(JE) Padstacks - does this matter?
        m_width = aVia->Diameter( aVia->Layers().Start() );
        m_net = aVia->Net();
        m_layers = aVia->Layers();
        m_rank = aVia->Rank();
        m_snapThreshhold = 0;
    }

    ~LINE();

    static inline bool ClassOf( const ITEM* aItem )
    {
        return aItem && LINE_T == aItem->Kind();
    }

    /// @copydoc ITEM::Clone()
    virtual LINE* Clone() const override;

    // Copy operator
    LINE& operator=( const LINE& aOther );

    // Move assignment operator
    LINE& operator=( LINE&& aOther ) noexcept;

    bool IsLinkedChecked() const
    {
        return IsLinked() && LinkCount() == ShapeCount();
    }

    ///< Assign a shape to the line (a polyline/line chain).
    void SetShape( const SHAPE_LINE_CHAIN& aLine )
    {
        m_line = aLine;
        m_line.SetWidth( m_width );
    }

    ///< Return the shape of the line.
    const SHAPE* Shape( int aLayer ) const override { return &m_line; }

    ///< Modifiable accessor to the underlying shape.
    SHAPE_LINE_CHAIN& Line() { return m_line; }
    const SHAPE_LINE_CHAIN& CLine() const { return m_line; }

    int SegmentCount() const { return m_line.SegmentCount(); }
    int PointCount() const { return m_line.PointCount(); }
    int ArcCount() const { return m_line.ArcCount(); }
    int ShapeCount() const { return m_line.ShapeCount(); }

    ///< Return the \a aIdx-th point of the line.
    const VECTOR2I& CPoint( int aIdx ) const { return m_line.CPoint( aIdx ); }
    const VECTOR2I& CLastPoint() const { return m_line.CLastPoint(); }
    const SEG CSegment( int aIdx ) const { return m_line.CSegment( aIdx ); }

    ///< Set line width.
    void SetWidth( int aWidth )
    {
        m_width = aWidth;
        m_line.SetWidth( aWidth );
    }

    ///< Return line width.
    int Width() const { return m_width; }

    ///< Return true if the line is geometrically identical as line \a aOther.
    bool CompareGeometry( const LINE& aOther );

    ///< Reverse the point/vertex order
    void Reverse();

    ///< Clip the line to the nearest obstacle, traversing from the line's start vertex (0).
    ///< Returns the clipped line.
    const LINE ClipToNearestObstacle( NODE* aNode ) const;

    ///< Clip the line to a given range of vertices.
    void ClipVertexRange ( int aStart, int aEnd );

    ///< Return the number of corners of angles specified by mask aAngles.
    int CountCorners( int aAngles ) const;

    /**
     * Calculate a line tightly wrapping a convex hull of an obstacle object (aObstacle).
     * @param aPrePath is the path from origin to the obstacle.
     * @param aWalkaroundPath is the path around the obstacle.
     * @param aPostPath is the path from obstacle till the end.
     * @param aCW determines whether to walk around in clockwise or counter-clockwise direction.
     */
    bool Walkaround( SHAPE_LINE_CHAIN aObstacle, SHAPE_LINE_CHAIN& aPre, SHAPE_LINE_CHAIN& aWalk,
                     SHAPE_LINE_CHAIN& aPost, bool aCw ) const;

    bool Walkaround( const SHAPE_LINE_CHAIN& aObstacle, SHAPE_LINE_CHAIN& aPath, bool aCw ) const;

    ///< Print out all linked segments.
    void ShowLinks() const;

    bool EndsWithVia() const { return m_via != nullptr; }

    int FindSegment( const SEGMENT* aSeg ) const;

    void AppendVia( const VIA& aVia );
    void LinkVia( VIA* aVia );
    void RemoveVia();

    VIA& Via() { return *m_via; }
    const VIA& Via() const { return *m_via; }

    void SetViaDiameter( int aDiameter )
    {
        wxCHECK( m_via, /* void */ );
        wxCHECK2_MSG( m_via->StackMode() == VIA::STACK_MODE::NORMAL,
                      m_via->SetStackMode( VIA::STACK_MODE::NORMAL ),
                      wxS( "Warning: converting a complex viastack to normal in PNS_LINE" ) );

        m_via->SetDiameter( VIA::ALL_LAYERS, aDiameter );
    }
    void SetViaDrill( int aDrill ) { assert(m_via); m_via->SetDrill( aDrill ); }

    virtual void Mark( int aMarker ) const override;
    virtual void Unmark( int aMarker = -1 ) const override;
    virtual int Marker() const override;

    void SetBlockingObstacle( ITEM* aObstacle ) { m_blockingObstacle = aObstacle; }
    ITEM* GetBlockingObstacle() const { return m_blockingObstacle; }

    void DragSegment( const VECTOR2I& aP, int aIndex, bool aFreeAngle = false );
    void DragCorner( const VECTOR2I& aP, int aIndex, bool aFreeAngle = false, DIRECTION_45 aPreferredEndingDirection = DIRECTION_45() );

    void SetRank( int aRank ) override;
    int Rank() const override;

    bool HasLoops() const;
    bool HasLockedSegments() const;

    void Clear();

    OPT_BOX2I ChangedArea( const LINE* aOther ) const;

    void SetSnapThreshhold( int aThreshhold )
    {
        m_snapThreshhold = aThreshhold;
    }

    int GetSnapThreshhold() const
    {
        return m_snapThreshhold;
    }

private:
    void dragSegment45( const VECTOR2I& aP, int aIndex );
    void dragCorner45( const VECTOR2I& aP, int aIndex, DIRECTION_45 aPreferredEndingDirection );
    void dragSegmentFree( const VECTOR2I& aP, int aIndex );
    void dragCornerFree( const VECTOR2I& aP, int aIndex );

    VECTOR2I snapToNeighbourSegments( const SHAPE_LINE_CHAIN& aPath, const VECTOR2I& aP,
                                      int aIndex ) const;

    VECTOR2I snapDraggedCorner( const SHAPE_LINE_CHAIN& aPath, const VECTOR2I& aP,
                                int aIndex ) const;

    SHAPE_LINE_CHAIN m_line;                ///< The actual shape of the line.
    int              m_width;               ///< Our width.


    int              m_snapThreshhold;      ///< Width to smooth out jagged segments.

    VIA*             m_via;
    ITEM*            m_blockingObstacle;    ///< For mark obstacle mode.
};

}

#endif    // __PNS_LINE_H
