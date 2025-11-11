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

#include <board.h>
#include <zone_settings_bag.h>
#include <panel_setup_zones_base.h>


class BOARD_DESIGN_SETTINGS;
class PANEL_ZONE_PROPERTIES;


class PANEL_SETUP_ZONES : public PANEL_SETUP_ZONES_BASE
{
public:
    PANEL_SETUP_ZONES( wxWindow* aParentWindow, PCB_EDIT_FRAME* aFrame, BOARD_DESIGN_SETTINGS& aBrdSettings );
    ~PANEL_SETUP_ZONES( ) override = default;

    bool TransferDataToWindow() override;
    bool TransferDataFromWindow() override;

    bool CommitPendingChanges();

private:
    BOARD_DESIGN_SETTINGS&  m_brdSettings;
    ZONE_SETTINGS_BAG       m_zoneSettingsBag;
    PANEL_ZONE_PROPERTIES*  m_panelZoneProperties;
};
