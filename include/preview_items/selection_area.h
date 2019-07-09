/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013 CERN
 * @author Tomasz Wlostowski <tomasz.wlostowski@cern.ch>
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

#ifndef PREVIEW_ITEMS_SELECTION_AREA_H
#define PREVIEW_ITEMS_SELECTION_AREA_H

#include <preview_items/simple_overlay_item.h>


namespace KIGFX
{
class GAL;

namespace PREVIEW
{

/**
 * Class SELECTION_AREA
 *
 * Represents a selection area (currently a rectangle) in a VIEW,
 * drawn corner-to-corner between two points. This is useful when
 * selecting a rectangular area, for lasso-select or zooming, for
 * example.
 */
class SELECTION_AREA : public SIMPLE_OVERLAY_ITEM
{
public:
    static const int SelectionLayer = LAYER_GP_OVERLAY;

    SELECTION_AREA();

    const BOX2I ViewBBox() const override;

    ///> Set the origin of the rectange (the fixed corner)
    void SetOrigin( VECTOR2I aOrigin )
    {
        m_origin = aOrigin;
    }

    /**
     * Set the current end of the rectangle (the corner that moves
     * with the cursor.
     */
    void SetEnd( VECTOR2I aEnd )
    {
        m_end = aEnd;
    }

    /**
     * Get class name
     * @return  string "SELECTION_AREA"
     */
    wxString GetClass() const override
    {
        return wxT( "SELECTION_AREA" );
    }

    VECTOR2I GetOrigin() const { return m_origin; }

    VECTOR2I GetEnd() const { return m_end; }

    void SetAdditive( bool aAdditive ) { m_additive = aAdditive; }
    void SetSubtractive( bool aSubtractive ) { m_subtractive = aSubtractive; }
    void SetExclusiveOr( bool aExclusiveOr ) { m_exclusiveOr = aExclusiveOr; }

    void ViewDraw( int aLayer, KIGFX::VIEW* aView ) const override final;

private:

    bool m_additive;
    bool m_subtractive;
    bool m_exclusiveOr;

    VECTOR2I m_origin, m_end;
};

} // PREVIEW
} // KIGFX

#endif // PREVIEW_ITEMS_SELECTION_AREA_H
