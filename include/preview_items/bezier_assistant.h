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

#include <eda_item.h>
#include <preview_items/bezier_geom_manager.h>
#include <layer_ids.h>

namespace KIGFX
{
namespace PREVIEW
{
    /**
     * Represents an assistant draw when interactively drawing a bezier on a canvas.
     */
    class BEZIER_ASSISTANT : public EDA_ITEM
    {
    public:
        BEZIER_ASSISTANT( const BEZIER_GEOM_MANAGER& aManager, const EDA_IU_SCALE& aIuScale,
                          EDA_UNITS aUnits );

        const BOX2I ViewBBox() const override;

        std::vector<int> ViewGetLayers() const override
        {
            return { LAYER_SELECT_OVERLAY,      // Assistant graphics
                     LAYER_GP_OVERLAY };        // Drop shadows
        }

        /**
         * Draw the assistance (with reference to the construction manager
         */
        void ViewDraw( int aLayer, KIGFX::VIEW* aView ) const override final;

#if defined( DEBUG )
        void Show( int x, std::ostream& st ) const override {}
#endif

        wxString GetClass() const override { return "BEZIER_ASSISTANT"; }

        void SetUnits( EDA_UNITS aUnits ) { m_units = aUnits; }

    private:
        const BEZIER_GEOM_MANAGER& m_constructMan;
        EDA_UNITS                  m_units;
    };
} // namespace PREVIEW
} // namespace KIGFX
