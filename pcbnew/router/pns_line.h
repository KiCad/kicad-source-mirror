/*
 * KiRouter - a push-and-(sometimes-)shove PCB router
 *
 * Copyright (C) 2013-2014 CERN
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

#include <math/vector2d.h>

#include <geometry/seg.h>
#include <geometry/shape.h>
#include <geometry/shape_line_chain.h>

#include "direction.h"
#include "pns_item.h"
#include "pns_via.h"

class PNS_NODE;
class PNS_SEGMENT;
class PNS_VIA;

/**
 * Class PNS_LINE
 *
 * Represents a track on a PCB, connecting two non-trivial joints (that is,
 * vias, pads, junctions between multiple traces or two traces different widths
 * and combinations of these). PNS_LINEs are NOT stored in the model (PNS_NODE).
 * Instead, they are assembled on-the-fly, based on a via/pad/segment that
 * belongs to/starts/ends them.
 *
 * PNS_LINEs can be either loose (consisting of segments that do not belong to
 * any PNS_NODE) or owned (with segments taken from a PNS_NODE) - these are
 * returned by PNS_NODE::AssembleLine and friends.
 *
 * A PNS_LINE may have a PNS_VIA attached at its end (i.e. the last point) - this is used by via
 * dragging/force propagation stuff.
 */

#define PNS_HULL_MARGIN 10

class PNS_LINE : public PNS_ITEM
{
public:
    typedef std::vector<PNS_SEGMENT*> SEGMENT_REFS;

    /**
     * Constructor
     * Makes an empty line.
     */
    PNS_LINE() : PNS_ITEM( LINE )
    {
        m_segmentRefs = NULL;
        m_hasVia = false;
    }

    PNS_LINE( const PNS_LINE& aOther ) ;
            
    /**
     * Constructor
     * Copies properties (net, layers, etc.) from a base line and replaces the shape
     * by another
     **/
    PNS_LINE( const PNS_LINE& aBase, const SHAPE_LINE_CHAIN& aLine ) :
        PNS_ITEM( aBase ),
        m_line( aLine ),
        m_width( aBase.m_width )
    {
        m_net = aBase.m_net;
        m_layers = aBase.m_layers;
        m_segmentRefs = NULL;
        m_hasVia = false;
    }

    ~PNS_LINE();

    /// @copydoc PNS_ITEM::Clone()
    virtual PNS_LINE* Clone() const;

    const PNS_LINE& operator=( const PNS_LINE& aOther );

    ///> Assigns a shape to the line (a polyline/line chain)
    void SetShape( const SHAPE_LINE_CHAIN& aLine )
    { 
        m_line = aLine;
    }

    ///> Returns the shape of the line
    const SHAPE* Shape() const 
    { 
        return &m_line; 
    }

    ///> Modifiable accessor to the underlying shape
    SHAPE_LINE_CHAIN& Line() 
    { 
        return m_line; 
    }

    ///> Const accessor to the underlying shape
    const SHAPE_LINE_CHAIN& CLine() const 
    { 
        return m_line;
    }

    ///> Returns the number of segments in the line
    int SegmentCount() const 
    {
        return m_line.SegmentCount(); 
    }

    ///> Returns the number of points in the line
    int PointCount() const 
    { 
        return m_line.PointCount(); 
    }

    ///> Returns the aIdx-th point of the line
    const VECTOR2I& CPoint( int aIdx ) const 
    { 
        return m_line.CPoint( aIdx );
    }

    ///> Returns the aIdx-th segment of the line
    const SEG CSegment( int aIdx ) const
    { 
        return m_line.CSegment( aIdx );
    }

    ///> Sets line width
    void SetWidth( int aWidth ) 
    { 
        m_width = aWidth; 
    }
    
    ///> Returns line width
    int Width() const 
    { 
        return m_width; 
    }

    ///> Returns true if the line is geometrically identical as line aOther
    bool CompareGeometry( const PNS_LINE& aOther );

    ///> Reverses the point/vertex order
    void Reverse();


    /* Linking functions */

    ///> Adds a reference to a segment registered in a PNS_NODE that is a part of this line. 
    void LinkSegment( PNS_SEGMENT* aSeg )
    {
        if( !m_segmentRefs )
            m_segmentRefs = new SEGMENT_REFS();

        m_segmentRefs->push_back( aSeg );
    }

    ///> Returns the list of segments from the owning node that constitute this
    ///> line (or NULL if the line is not linked)
    SEGMENT_REFS* LinkedSegments()
    {
        return m_segmentRefs;
    }

    ///> Checks if the segment aSeg is a part of the line.
    bool ContainsSegment( PNS_SEGMENT* aSeg ) const
    {
        if( !m_segmentRefs )
            return false;

        return std::find( m_segmentRefs->begin(), m_segmentRefs->end(),
                aSeg ) != m_segmentRefs->end();
    }

    ///> Erases the linking information. Used to detach the line from the owning node.
    void ClearSegmentLinks();
    
    ///> Returns the number of segments that were assembled together to form this line.
    int LinkCount() const
    {
        if( !m_segmentRefs )
            return -1;

        return m_segmentRefs->size();
    }

    ///> Clips the line to the nearest obstacle, traversing from the line's start vertex (0).
    ///> Returns the clipped line.
    const PNS_LINE ClipToNearestObstacle( PNS_NODE* aNode ) const;

    ///> Clips the line to a given range of vertices. 
    void ClipVertexRange ( int aStart, int aEnd );
    
    ///> Returns the number of corners of angles specified by mask aAngles.
    int CountCorners( int aAngles );

    ///> Calculates a line thightly wrapping a convex hull
    ///> of an obstacle object (aObstacle).
    ///> aPrePath = path from origin to the obstacle
    ///> aWalkaroundPath = path around the obstacle
    ///> aPostPath = past from obstacle till the end
    ///> aCW = whether to walk around in clockwise or counter-clockwise direction.
    bool Walkaround( SHAPE_LINE_CHAIN aObstacle,
            SHAPE_LINE_CHAIN& aPre,
            SHAPE_LINE_CHAIN& aWalk,
            SHAPE_LINE_CHAIN& aPost,
            bool aCw ) const;

    void Walkaround( const SHAPE_LINE_CHAIN& aObstacle,
            SHAPE_LINE_CHAIN& aPath,
            bool aCw ) const;

    bool Is45Degree();

    ///> Prints out all linked segments
    void ShowLinks();

    bool EndsWithVia() const { return m_hasVia; }

    void AppendVia( const PNS_VIA& aVia );
    void RemoveVia() { m_hasVia = false; }

    const PNS_VIA& Via() const { return m_via; }
 
    virtual void Mark( int aMarker );
    virtual void Unmark ();
    virtual int Marker() const;
    
    void DragSegment( const VECTOR2I& aP, int aIndex, int aSnappingThreshold = 0 );
    void DragCorner( const VECTOR2I& aP, int aIndex, int aSnappingThreshold = 0 );

    void SetRank( int aRank );
    int Rank() const;
        
    bool HasLoops() const;

private:
    VECTOR2I snapToNeighbourSegments( const SHAPE_LINE_CHAIN& aPath, const VECTOR2I &aP,
                                      int aIndex, int aThreshold) const;

    VECTOR2I snapDraggedCorner( const SHAPE_LINE_CHAIN& aPath, const VECTOR2I &aP,
                                int aIndex, int aThreshold ) const;

    ///> Copies m_segmentRefs from the line aParent.
    void copyLinks( const PNS_LINE* aParent ) ;
    
    ///> List of segments in the owning PNS_NODE (PNS_ITEM::m_owner) that constitute this line, or NULL
    ///> if the line is not a part of any node.
    SEGMENT_REFS* m_segmentRefs;

    ///> The actual shape of the line
    SHAPE_LINE_CHAIN m_line;

    ///> our width
    int m_width;

    ///> If true, the line ends with a via
    bool m_hasVia;

    ///> Via at the end point, if m_hasVia == true
    PNS_VIA m_via;
};

#endif    // __PNS_LINE_H
