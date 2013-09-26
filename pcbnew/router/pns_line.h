/*
 * KiRouter - a push-and-(sometimes-)shove PCB router
 *
 * Copyright (C) 2013  CERN
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
 * with this program.  If not, see <http://www.gnu.or/licenses/>.
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
 * belongs/begins them.
 *
 * PNS_LINEs can be either loose (consisting of segments that do not belong to 
 * any PNS_NODE) or owned (with segments taken from a PNS_NODE) - these are 
 * returned by PNS_NODE::AssembleLine and friends.
 *
 * A PNS_LINE may have a PNS_VIA attached at its and - this is used by via 
 * dragging/force propagation stuff.
 */

class PNS_LINE : public PNS_ITEM
{
public:
    typedef std::vector<PNS_SEGMENT*> LinkedSegments;

    PNS_LINE() :
        PNS_ITEM( LINE )
    {
        m_segmentRefs = NULL;
        m_hasVia = false;
        m_affectedRangeStart = -1;
    };

    PNS_LINE( int aLayer, int aWidth, const SHAPE_LINE_CHAIN& aLine ) :
        PNS_ITEM( LINE )
    {
        m_line  = aLine;
        m_width = aWidth;
        m_segmentRefs = NULL;
        m_hasVia = false;
        m_affectedRangeStart = -1;
        SetLayer( aLayer );
    }

    PNS_LINE( const PNS_LINE& aOther ) :
        PNS_ITEM( aOther ),
        m_line( aOther.m_line ),
        m_width( aOther.m_width )
    {
        m_net = aOther.m_net;
        m_movable = aOther.m_movable;
        m_world = aOther.m_world;
        m_layers = aOther.m_layers;
        m_segmentRefs = NULL;
        m_via = aOther.m_via;
        m_hasVia = aOther.m_hasVia;
        m_affectedRangeStart = -1;
    }

    /**
     * Constructor
     * copies properties (net, layers from a base line), and replaces the shape
     * by aLine
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
        m_affectedRangeStart = -1;
    }

    ~PNS_LINE()
    {
        if( m_segmentRefs )
            delete m_segmentRefs;
    };

    virtual PNS_LINE* Clone() const;

    ///> clones the line without cloning the shape
    ///> (just the properties - net, width, layers, etc.)
    PNS_LINE* CloneProperties() const;

    int GetLayer() const { return GetLayers().Start(); }

    ///> Geometry accessors
    void SetShape( const SHAPE_LINE_CHAIN& aLine ) { m_line = aLine; }
    const SHAPE* GetShape() const { return &m_line; }
    SHAPE_LINE_CHAIN& GetLine() { return m_line; }
    const SHAPE_LINE_CHAIN& GetCLine() const { return m_line; }

    ///> Width accessors
    void SetWidth( int aWidth ) { m_width = aWidth; }
    int GetWidth() const { return m_width; }

    ///> Links a segment from a PNS_NODE to this line, making it owned by the node
    void LinkSegment( PNS_SEGMENT* aSeg )
    {
        if( !m_segmentRefs )
            m_segmentRefs = new std::vector<PNS_SEGMENT*> ();

        m_segmentRefs->push_back( aSeg );
    }

    ///> Returns a list of segments from the owning node that constitute this 
    ///> line (or NULL if the line is loose)
    LinkedSegments* GetLinkedSegments()
    {
        return m_segmentRefs;
    }

    bool ContainsSegment( PNS_SEGMENT* aSeg ) const
    {
        if( !m_segmentRefs )
            return false;

        return std::find( m_segmentRefs->begin(), m_segmentRefs->end(),
                aSeg ) != m_segmentRefs->end();
    }

    ///> Returns this line, but clipped to the nearest obstacle 
    ///> along, to avoid collision.
    const PNS_LINE ClipToNearestObstacle( PNS_NODE* aNode ) const;

    ///> DEPRECATED optimization functions (moved to PNS_OPTIMIZER)
    bool MergeObtuseSegments();
    bool MergeSegments();

    ///> Returns the number of corners of angles specified by mask aAngles.
    int CountCorners( int aAngles );

    ///> Calculates a line thightly wrapping a convex hull 
    ///> of an obstacle object (aObstacle).
    ///> aPrePath = path from origin to the obstacle
    ///> aWalkaroundPath = path around the obstacle
    ///> aPostPath = past from obstacle till the end
    ///> aCW = whether to walkaround in clockwise or counter-clockwise direction.
    void NewWalkaround( const SHAPE_LINE_CHAIN& aObstacle,
            SHAPE_LINE_CHAIN& aPrePath,
            SHAPE_LINE_CHAIN& aWalkaroundPath,
            SHAPE_LINE_CHAIN& aPostPath,
            bool aCw ) const;

    void NewWalkaround( const SHAPE_LINE_CHAIN& aObstacle,
            SHAPE_LINE_CHAIN& aPath,
            bool aCw ) const;


    bool Walkaround( SHAPE_LINE_CHAIN obstacle,
            SHAPE_LINE_CHAIN& pre,
            SHAPE_LINE_CHAIN& walk,
            SHAPE_LINE_CHAIN& post,
            bool cw ) const;

    void Walkaround( const SHAPE_LINE_CHAIN& aObstacle,
            SHAPE_LINE_CHAIN& aPath,
            bool aCw ) const;

    bool Is45Degree();

    ///> Prints out all linked segments
    void ShowLinks();

    bool EndsWithVia() const { return m_hasVia; }

    void AppendVia( const PNS_VIA& aVia )
    {
        m_hasVia = true;
        m_via = aVia;
        m_via.SetNet( m_net );
    }

    void RemoveVia() { m_hasVia = false; }
    const PNS_VIA& GetVia() const { return m_via; }

    void SetAffectedRange( int aStart, int aEnd )
    {
        m_affectedRangeStart = aStart;
        m_affectedRangeEnd = aEnd;
    }

    void ClearAffectedRange()
    {
        m_affectedRangeStart = -1;
    }

    bool GetAffectedRange( int& aStart, int& aEnd )
    {
        if( m_affectedRangeStart >= 0 )
        {
            aStart = m_affectedRangeStart;
            aEnd = m_affectedRangeEnd;
            return true;
        }
        else
        {
            aStart = 0;
            aEnd = m_line.PointCount();
            return false;
        }
    }

private:
    bool onEdge( const SHAPE_LINE_CHAIN& obstacle, VECTOR2I p, int& ei, bool& is_vertex ) const;
    bool walkScan( const SHAPE_LINE_CHAIN& line, const SHAPE_LINE_CHAIN& obstacle,
            bool reverse, VECTOR2I& ip, int& index_o, int& index_l, bool& is_vertex ) const;

    ///> List of semgments in a PNS_NODE (PNS_ITEM::m_owner) that constitute this line.
    LinkedSegments* m_segmentRefs;

    ///> Shape of the line
    SHAPE_LINE_CHAIN m_line;

    int m_width;

    ///> Via at the end and a flag indicating if it's enabled.
    PNS_VIA m_via;
    bool m_hasVia;

    int m_affectedRangeStart;
    int m_affectedRangeEnd;
};

#endif    // __PNS_LINE_H

