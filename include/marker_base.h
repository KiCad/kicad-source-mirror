/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2009-2014 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 1992-2014 KiCad Developers, see CHANGELOG.TXT for contributors.
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
#include <eda_rect.h>


class SHAPE_LINE_CHAIN;

namespace KIGFX
{
    class RENDER_SETTINGS;
}

using KIGFX::RENDER_SETTINGS;


/*
 * Marker are mainly used to show a DRC or ERC error or warning
 */


class MARKER_BASE
{
public:
    enum TYPEMARKER {
        MARKER_UNSPEC,
        MARKER_ERC,
        MARKER_PCB,
        MARKER_SIMUL
    };

    wxPoint               m_Pos;                 ///< position of the marker

protected:
    TYPEMARKER            m_markerType;          // The type of marker (useful to filter markers)
    bool                  m_excluded;            // User has excluded this specific error
    std::shared_ptr<RC_ITEM> m_rcItem;

    int                   m_scalingFactor;       // Scaling factor to convert corners coordinates
                                                 // to internat units coordinates
    EDA_RECT              m_shapeBoundingBox;    // Bounding box of the graphic symbol, relative
                                                 // to the position of the shape, in marker shape
                                                 // units

public:

    MARKER_BASE( int aScalingFactor, std::shared_ptr<RC_ITEM> aItem, TYPEMARKER aType = MARKER_UNSPEC );
    virtual ~MARKER_BASE();

    /** The scaling factor to convert polygonal shape coordinates to internal units
     */
    int MarkerScale() const { return m_scalingFactor; }

    /** Returns the shape polygon in internal units in a SHAPE_LINE_CHAIN
     * the coordinates are relatives to the marker position (are not absolute)
     * @param aPolygon is the SHAPE_LINE_CHAIN to fill with the shape
     */
    void ShapeToPolygon( SHAPE_LINE_CHAIN& aPolygon) const;

    /**
     * Function PrintMarker
     * Prints the shape is the polygon defined in m_Corners (array of wxPoints).
     */
    void PrintMarker( RENDER_SETTINGS* aSettings, const wxPoint& aOffset );

    /**
     * Function GetPos
     * @return the position of this MARKER in internal units.
     */
    const wxPoint& GetPos() const { return m_Pos; }

    virtual const KIID GetUUID() const = 0;

    /**
     * accessors to set/get marker type (DRC, ERC, or other)
     */
    void SetMarkerType( enum TYPEMARKER aMarkerType ) { m_markerType = aMarkerType; }
    enum TYPEMARKER GetMarkerType() const { return m_markerType; }

    bool IsExcluded() const { return m_excluded; }
    void SetExcluded( bool aExcluded ) { m_excluded = aExcluded; }

    /**
     * Function GetReporter
     * returns the DRC_ITEM held within this MARKER so that its
     * interface may be used.
     * @return const& DRC_ITEM
     */
    
    // fixme: use shared_ptr
    std::shared_ptr<RC_ITEM> GetRCItem() const { return m_rcItem; }

    /**
     * Tests if the given wxPoint is within the bounds of this object.
     * @param aHitPosition is the wxPoint to test (in internal units)
     * @return bool - true if a hit, else false
     */
    bool HitTestMarker( const wxPoint& aHitPosition, int aAccuracy ) const;

    /**
     * Function GetBoundingBoxMarker
     * returns the orthogonal, bounding box of this object for display purposes.
     * This box should be an enclosing perimeter for visible components of this
     * object, and the units should be in the pcb or schematic coordinate system.
     * It is OK to overestimate the size by a few counts.
     */
    EDA_RECT GetBoundingBoxMarker() const;

protected:
    virtual KIGFX::COLOR4D getColor() const = 0;
};


#endif      //  MARKER_BASE_H
