/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
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

#pragma once

#include <origin_viewitem.h>
#include <geometry/point_types.h>


namespace KIGFX
{

/**
 * View item to draw an origin marker with an optional
 * snap type indicator.
 */
class SNAP_INDICATOR : public ORIGIN_VIEWITEM
{
public:
    SNAP_INDICATOR( const COLOR4D& aColor = COLOR4D::WHITE, int aSize = 16,
                    const VECTOR2D& aPosition = VECTOR2D( 0, 0 ) );

    SNAP_INDICATOR( const VECTOR2D& aPosition, EDA_ITEM_FLAGS flags );

    SNAP_INDICATOR* Clone() const override;

    const BOX2I ViewBBox() const override;

    void ViewDraw( int aLayer, VIEW* aView ) const override;

#if defined( DEBUG )
    void Show( int x, std::ostream& st ) const override {}
#endif

    /**
     * Get class name.
     *
     * @return string "SNAP_INDICATOR"
     */
    wxString GetClass() const override { return wxT( "SNAP_INDICATOR" ); }

    /**
     * Set a mask of point types that this snap item represents.
     *
     * This is a mask of POINT_TYPE.
     */
    void SetSnapTypes( int aSnapTypes ) { m_snapTypes = aSnapTypes; }

private:
    int m_snapTypes = POINT_TYPE::PT_NONE;
};

} // namespace KIGFX
