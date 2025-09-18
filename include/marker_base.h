/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2009-2014 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright The KiCad Developers, see AUTHORS.TXT for contributors.
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


#ifndef MARKER_BASE_H
#define MARKER_BASE_H

#include <memory>

#include <rc_item.h>
#include <gr_basic.h>


class SHAPE_LINE_CHAIN;

namespace KIGFX
{
    class RENDER_SETTINGS;
}

using KIGFX::RENDER_SETTINGS;


/**
 * Marker are mainly used to show a DRC or ERC error or warning.
 */
class MARKER_BASE
{
public:
    enum MARKER_T
    {
        MARKER_UNSPEC,
        MARKER_ERC,
        MARKER_DRC,
        MARKER_DRAWING_SHEET,
        MARKER_RATSNEST,
        MARKER_PARITY,
        MARKER_SIMUL
    };

    MARKER_BASE( int aScalingFactor, std::shared_ptr<RC_ITEM> aItem,
                 MARKER_T aType = MARKER_UNSPEC );
    virtual ~MARKER_BASE() {};

    /**
     * The scaling factor to convert polygonal shape coordinates to internal units.
     */
    int MarkerScale() const { return m_scalingFactor; }
    void SetMarkerScale( int aScale ) const { m_scalingFactor = aScale; }

    /**
     * Return the shape polygon in internal units in a #SHAPE_LINE_CHAIN the coordinates
     * are relatives to the marker position (are not absolute).
     *
     * @param aPolygon is the #SHAPE_LINE_CHAIN to fill with the shape.
     */
    void ShapeToPolygon( SHAPE_LINE_CHAIN& aPolygon, int aScale = -1 ) const;

    /**
     * @return the position of this marker in internal units.
     */
    const VECTOR2I& GetPos() const { return m_Pos; }

    virtual const KIID GetUUID() const = 0;

    /**
     * Accessors to set/get marker type (DRC, ERC, or other)
     */
    void SetMarkerType( enum MARKER_T aMarkerType ) { m_markerType = aMarkerType; }
    enum MARKER_T GetMarkerType() const { return m_markerType; }

    bool IsExcluded() const { return m_excluded; }
    void SetExcluded( bool aExcluded, const wxString& aComment = wxEmptyString )
    {
        m_excluded = aExcluded;
        m_comment = aComment;
    }

    wxString GetComment() const { return m_comment; }

    virtual SEVERITY GetSeverity() const { return RPT_SEVERITY_UNDEFINED; }

    /**
     * @return the #RC_ITEM held within this marker so that its interface may be used.
     */
    std::shared_ptr<RC_ITEM> GetRCItem() const { return m_rcItem; }

    /**
     * Test if the given #VECTOR2I is within the bounds of this object.
     *
     * @param aHitPosition is the #VECTOR2I to test (in internal units).
     * @return true if a hit, else false.
     */
    bool HitTestMarker( const VECTOR2I& aHitPosition, int aAccuracy ) const;

    /**
     * Test if the given #BOX2I intersects or contains the bounds of this object.
     */
    bool HitTestMarker( const BOX2I& aRect, bool aContained, int aAccuracy = 0 ) const;

    /**
     * Test if the given #SHAPE_LINE_CHAIN intersects or contains the bounds of this object.
     */
    bool HitTestMarker( const SHAPE_LINE_CHAIN& aPoly, bool aContained ) const;

    /**
     * Return the orthogonal, bounding box of this object for display purposes.
     *
     * This box should be an enclosing perimeter for visible components of this
     * object, and the units should be in the pcb or schematic coordinate system.
     * It is OK to overestimate the size by a few counts.
     */
    BOX2I GetBoundingBoxMarker() const;

protected:
    virtual KIGFX::COLOR4D getColor() const = 0;

public:
    VECTOR2I            m_Pos;                 ///< Position of the marker.

protected:
    MARKER_T            m_markerType;          ///< The type of marker.
    bool                m_excluded;            ///< User has excluded this specific error.
    wxString            m_comment;             ///< User supplied comment.
    std::shared_ptr<RC_ITEM> m_rcItem;

    mutable int         m_scalingFactor;       ///< Scaling factor to convert corners coordinates to internal
                                               ///< units.  Dependant on current zoom.
    BOX2I               m_shapeBoundingBox;    ///< Bounding box of the graphic symbol relative to the position
                                               ///< of the shape in marker shape units.
};


#endif      //  MARKER_BASE_H
