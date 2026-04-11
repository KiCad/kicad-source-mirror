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


#pragma once

#include <board.h>
#include <widgets/unit_binder.h>
#include <widgets/paged_dialog.h>

#include <panel_setup_zone_hatch_offsets_base.h>

class BOARD_DESIGN_SETTINGS;
class LAYER_PROPERTIES_GRID_TABLE;


class PANEL_SETUP_ZONE_HATCH_OFFSETS : public PANEL_SETUP_ZONE_HATCH_OFFSETS_BASE
{
public:
    PANEL_SETUP_ZONE_HATCH_OFFSETS( wxWindow* aParentWindow, PCB_BASE_FRAME* aFrame,
                                    BOARD_DESIGN_SETTINGS& aBrdSettings );
    ~PANEL_SETUP_ZONE_HATCH_OFFSETS( ) override;

    bool TransferDataToWindow() override;
    bool TransferDataFromWindow() override;

    void SyncCopperLayers( int aCopperLayerCount );

    void ImportSettingsFrom( BOARD* aBoard );

private:
    PCB_BASE_FRAME*              m_frame;
    LAYER_PROPERTIES_GRID_TABLE* m_layerPropsTable;
    BOARD_DESIGN_SETTINGS*       m_brdSettings;
};
