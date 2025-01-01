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

#ifndef PREVIEW_ITEMS_TWO_POINT_ASSISTANT_H
#define PREVIEW_ITEMS_TWO_POINT_ASSISTANT_H

#include <eda_item.h>
#include <preview_items/two_point_geom_manager.h>
#include <layer_ids.h>

namespace KIGFX
{
namespace PREVIEW
{

// TODO: required until EDA_SHAPE_TYPE_T is moved into commons or a better approach is found
enum class GEOM_SHAPE
{
    SEGMENT = 0,
    RECT,
    ARC,
    CIRCLE,
    POLYGON,
    CURVE
};

/**
 * Represents an assistant draw when interactively drawing a line or circle on a canvas.
 */
class TWO_POINT_ASSISTANT : public EDA_ITEM
{
public:
    TWO_POINT_ASSISTANT( const TWO_POINT_GEOMETRY_MANAGER& aManager, const EDA_IU_SCALE& aIuScale,
                         EDA_UNITS aUnits, GEOM_SHAPE aShape );

    const BOX2I ViewBBox() const override;

    std::vector<int> ViewGetLayers() const override
    {
        return { LAYER_SELECT_OVERLAY,  // Assistant graphics
                 LAYER_GP_OVERLAY };    // Drop shadows
    }

    /**
     * Draw the assistance (with reference to the construction manager
     */
    void ViewDraw( int aLayer, KIGFX::VIEW* aView ) const override final;

#if defined( DEBUG )
    void Show( int x, std::ostream& st ) const override
    {
    }
#endif

    /**
     * Get class name
     * @return  string "TWO_POINT_ASSISTANT"
     */
    wxString GetClass() const override
    {
        return "TWO_POINT_ASSISTANT";
    }

    void SetUnits( EDA_UNITS aUnits ) { m_units = aUnits; }

private:
    const TWO_POINT_GEOMETRY_MANAGER& m_constructMan;
    EDA_UNITS                         m_units;
    GEOM_SHAPE                        m_shape;
    const EDA_IU_SCALE&               m_iuScale;
};

} // namespace PREVIEW
} // namespace KIGFX

#endif // PREVIEW_ITEMS_TWO_POINT_ASSISTANT_H
