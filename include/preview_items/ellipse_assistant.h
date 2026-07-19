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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef PREVIEW_ITEMS_ELLIPSE_ASSISTANT_H
#define PREVIEW_ITEMS_ELLIPSE_ASSISTANT_H

#include <eda_item.h>
#include <eda_units.h>
#include <preview_items/ellipse_geom_manager.h>
#include <layer_ids.h>

namespace KIGFX
{

namespace PREVIEW
{

/**
 * Visual overlay shown while interactively drawing an elliptical arc.
 *
 * During the bounding-box phase (steps SET_BBOX_C1 .. SET_BBOX_C2) it draws
 * the bbox rectangle and the fitted ellipse outline.  During the angle phase
 * (steps SET_START_ANGLE .. SET_END_ANGLE) it draws the ellipse, radial lines
 * at the start / end angles, and the arc segment itself.
 */
class ELLIPSE_ASSISTANT : public EDA_ITEM
{
public:
    ELLIPSE_ASSISTANT( const ELLIPSE_GEOM_MANAGER& aManager, const EDA_IU_SCALE& aIuScale, EDA_UNITS aUnits );

    const BOX2I ViewBBox() const override;

    std::vector<int> ViewGetLayers() const override
    {
        return {
            LAYER_SELECT_OVERLAY,
            LAYER_GP_OVERLAY,
        };
    }

    void ViewDraw( int aLayer, KIGFX::VIEW* aView ) const override final;

#if defined( DEBUG )
    void Show( int x, std::ostream& st ) const override {}
#endif

    wxString GetClass() const override { return "ELLIPSE_ASSISTANT"; }

    void SetUnits( EDA_UNITS aUnits ) { m_units = aUnits; }

private:
    const ELLIPSE_GEOM_MANAGER& m_constructMan;
    const EDA_IU_SCALE&         m_iuScale;
    EDA_UNITS                   m_units;

    ///< Draw the arc segment (or just the radius lines). May be false if the assistant
    ///< is secondary to an in-progress drawing of a real arc object.
    bool m_drawArc;
};

} // namespace PREVIEW
} // namespace KIGFX

#endif // PREVIEW_ITEMS_ELLIPSE_ASSISTANT_H
