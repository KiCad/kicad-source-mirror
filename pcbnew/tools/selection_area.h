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

#ifndef __SELECTION_AREA_H
#define __SELECTION_AREA_H

#include <base_struct.h>
#include <layers_id_colors_and_visibility.h>
#include <math/box2.h>

namespace KIGFX
{
class GAL;
}

/**
 * Class SELECTION_AREA
 *
 * Represents a selection area (currently a rectangle) in a VIEW.
 */
class SELECTION_AREA : public EDA_ITEM
{
public:
    static const int SelectionLayer = ITEM_GAL_LAYER( GP_OVERLAY );

    SELECTION_AREA();
    ~SELECTION_AREA() {};

    virtual const BOX2I ViewBBox() const;

    void ViewDraw( int aLayer, KIGFX::GAL* aGal ) const;

    void ViewGetLayers( int aLayers[], int& aCount ) const;

    void SetOrigin( VECTOR2I aOrigin )
    {
        m_origin = aOrigin;
    }

    void SetEnd( VECTOR2I aEnd )
    {
        m_end = aEnd;
    }

#if defined(DEBUG)
    void Show( int x, std::ostream& st ) const
    {
    }
#endif

    /** Get class name
     * @return  string "SELECTION_AREA"
     */
    virtual wxString GetClass() const
    {
        return wxT( "SELECTION_AREA" );
    }

private:
    VECTOR2I m_origin, m_end;
};

#endif
