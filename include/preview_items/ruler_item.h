/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017 KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef PREVIEW_ITEMS_RULER_ITEM_H
#define PREVIEW_ITEMS_RULER_ITEM_H

#include <base_struct.h>
#include <preview_items/two_point_geom_manager.h>

namespace KIGFX
{
class GAL;

namespace PREVIEW
{
class TWO_POINT_GEOMETRY_MANAGER;

/**
 * Class RULER_ITEM
 *
 * A drawn ruler item for showing the distance between two points.
 */
class RULER_ITEM : public EDA_ITEM
{
public:

    RULER_ITEM( const TWO_POINT_GEOMETRY_MANAGER& m_geomMgr );

    ///> @copydoc EDA_ITEM::ViewBBox()
    const BOX2I ViewBBox() const override;

    ///> @copydoc EDA_ITEM::ViewGetLayers()
    void ViewGetLayers( int aLayers[], int& aCount ) const override;

    ///> @copydoc EDA_ITEM::ViewDraw();
    void ViewDraw( int aLayer, KIGFX::VIEW* aView ) const override final;


#if defined(DEBUG)
    void Show( int x, std::ostream& st ) const override
    {
    }
#endif

    /**
     * Get class name
     * @return  string "RULER_ITEM"
     */
    wxString GetClass() const override
    {
        return wxT( "RULER_ITEM" );
    }

private:

    const TWO_POINT_GEOMETRY_MANAGER& m_geomMgr;
};

} // PREVIEW
} // KIGFX

#endif // PREVIEW_ITEMS_RULER_ITEM_H
