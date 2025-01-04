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

#ifndef PREVIEW_ITEMS_RULER_ITEM_H
#define PREVIEW_ITEMS_RULER_ITEM_H

#include <optional>

#include <eda_item.h>
#include <gal/color4d.h>
#include <preview_items/two_point_geom_manager.h>

namespace KIGFX
{
class GAL;

namespace PREVIEW
{
class TWO_POINT_GEOMETRY_MANAGER;

/**
 * A drawn ruler item for showing the distance between two points.
 */
class RULER_ITEM : public EDA_ITEM
{
public:
    RULER_ITEM( const TWO_POINT_GEOMETRY_MANAGER& m_geomMgr, const EDA_IU_SCALE& aIuScale,
                EDA_UNITS userUnits, bool aFlipX, bool aFlipY );

    ///< @copydoc EDA_ITEM::ViewBBox()
    const BOX2I ViewBBox() const override;

    ///< @copydoc EDA_ITEM::ViewGetLayers()
    std::vector<int> ViewGetLayers() const override;

    ///< @copydoc EDA_ITEM::ViewDraw();
    void ViewDraw( int aLayer, KIGFX::VIEW* aView ) const override final;

    void SetColor( const COLOR4D& aColor ) { m_color = aColor; }

    void SetShowTicks( bool aShow ) { m_showTicks = aShow; }

    void SetShowEndArrowHead( bool aShow ) { m_showEndArrowHead = aShow; }

    /**
     * Get the strings for the dimensions of the ruler.
     */
    wxArrayString GetDimensionStrings() const;

#if defined(DEBUG)
    void Show( int x, std::ostream& st ) const override
    {
    }
#endif

    /**
     * Get class name.
     *
     * @return  string "RULER_ITEM".
     */
    wxString GetClass() const override
    {
        return wxT( "RULER_ITEM" );
    }

    /**
     * Switch the ruler units.
     *
     * @param aUnits is the new unit system the ruler should use.
     */
    void SwitchUnits( EDA_UNITS aUnits ) { m_userUnits = aUnits; }

    void UpdateDir( bool aFlipX, bool aFlipY )
    {
        m_flipX = aFlipX;
        m_flipY = aFlipY;
    }

private:
    const TWO_POINT_GEOMETRY_MANAGER& m_geomMgr;
    EDA_UNITS                         m_userUnits;
    const EDA_IU_SCALE&               m_iuScale;
    bool                              m_flipX;
    bool                              m_flipY;
    std::optional<COLOR4D>            m_color;
    bool                              m_showTicks = true;
    bool                              m_showEndArrowHead = false;
};

} // PREVIEW
} // KIGFX

#endif // PREVIEW_ITEMS_RULER_ITEM_H
