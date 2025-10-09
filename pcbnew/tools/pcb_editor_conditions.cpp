/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020 Ian McInerney <ian.s.mcinerney at ieee.org>
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


#include <pgm_base.h>
#include <settings/settings_manager.h>
#include <pcbnew_settings.h>
#include <footprint_editor_settings.h>
#include <board.h>
#include <pcb_base_frame.h>
#include <tool/selection.h>
#include <tools/pcb_editor_conditions.h>
#include <geometry/geometry_utils.h>

#include <functional>
#include <wx/debug.h>

using namespace std::placeholders;


SELECTION_CONDITION PCB_EDITOR_CONDITIONS::HasItems()
{
    // Requires a PCB_BASE_FRAME
    PCB_BASE_FRAME* drwFrame = dynamic_cast<PCB_BASE_FRAME*>( m_frame );

    wxASSERT( drwFrame );

    return std::bind( &PCB_EDITOR_CONDITIONS::hasItemsFunc, _1, drwFrame );
}


SELECTION_CONDITION PCB_EDITOR_CONDITIONS::PadNumbersDisplay()
{
    // Requires a PCB_BASE_FRAME
    PCB_BASE_FRAME* drwFrame = dynamic_cast<PCB_BASE_FRAME*>( m_frame );

    wxASSERT( drwFrame );

    return std::bind( &PCB_EDITOR_CONDITIONS::padNumberDisplayFunc, _1, drwFrame );
}


SELECTION_CONDITION PCB_EDITOR_CONDITIONS::PadFillDisplay()
{
    // Requires a PCB_BASE_FRAME
    PCB_BASE_FRAME* drwFrame = dynamic_cast<PCB_BASE_FRAME*>( m_frame );

    wxASSERT( drwFrame );

    return std::bind( &PCB_EDITOR_CONDITIONS::padFillDisplayFunc, _1, drwFrame );
}


SELECTION_CONDITION PCB_EDITOR_CONDITIONS::TextFillDisplay()
{
    // Requires a PCB_BASE_FRAME
    PCB_BASE_FRAME* drwFrame = dynamic_cast<PCB_BASE_FRAME*>( m_frame );

    wxASSERT( drwFrame );

    return std::bind( &PCB_EDITOR_CONDITIONS::textFillDisplayFunc, _1, drwFrame );
}


SELECTION_CONDITION PCB_EDITOR_CONDITIONS::GraphicsFillDisplay()
{
    // Requires a PCB_BASE_FRAME
    PCB_BASE_FRAME* drwFrame = dynamic_cast<PCB_BASE_FRAME*>( m_frame );

    wxASSERT( drwFrame );

    return std::bind( &PCB_EDITOR_CONDITIONS::graphicsFillDisplayFunc, _1, drwFrame );
}


SELECTION_CONDITION PCB_EDITOR_CONDITIONS::ViaFillDisplay()
{
    // Requires a PCB_BASE_FRAME
    PCB_BASE_FRAME* drwFrame = dynamic_cast<PCB_BASE_FRAME*>( m_frame );

    wxASSERT( drwFrame );

    return std::bind( &PCB_EDITOR_CONDITIONS::viaFillDisplayFunc, _1, drwFrame );
}


SELECTION_CONDITION PCB_EDITOR_CONDITIONS::TrackFillDisplay()
{
    // Requires a PCB_BASE_FRAME
    PCB_BASE_FRAME* drwFrame = dynamic_cast<PCB_BASE_FRAME*>( m_frame );

    wxASSERT( drwFrame );

    return std::bind( &PCB_EDITOR_CONDITIONS::trackFillDisplayFunc, _1, drwFrame );
}


SELECTION_CONDITION PCB_EDITOR_CONDITIONS::ZoneDisplayMode( ZONE_DISPLAY_MODE aMode )
{
    // Requires a PCB_BASE_FRAME
    PCB_BASE_FRAME* drwFrame = dynamic_cast<PCB_BASE_FRAME*>( m_frame );

    wxASSERT( drwFrame );

    return std::bind( &PCB_EDITOR_CONDITIONS::zoneDisplayModeFunc, _1, drwFrame, aMode );
}


SELECTION_CONDITION PCB_EDITOR_CONDITIONS::FootprintViewerAutoZoom()
{
    PCB_BASE_FRAME* drwFrame = dynamic_cast<PCB_BASE_FRAME*>( m_frame );

    wxASSERT( drwFrame );

    return std::bind( &PCB_EDITOR_CONDITIONS::footprintViewerAutoZoom, _1, drwFrame );
}


bool PCB_EDITOR_CONDITIONS::hasItemsFunc( const SELECTION& aSelection, PCB_BASE_FRAME* aFrame )
{
    BOARD* board = aFrame->GetBoard();

    return board && !board->IsEmpty();
}


bool PCB_EDITOR_CONDITIONS::padNumberDisplayFunc( const SELECTION& aSelection, PCB_BASE_FRAME* aFrame )
{
    return aFrame->GetViewerSettingsBase()->m_ViewersDisplay.m_DisplayPadNumbers;
}


bool PCB_EDITOR_CONDITIONS::padFillDisplayFunc( const SELECTION& aSelection, PCB_BASE_FRAME* aFrame )
{
    return aFrame->GetViewerSettingsBase()->m_ViewersDisplay.m_DisplayPadFill;
}


bool PCB_EDITOR_CONDITIONS::textFillDisplayFunc( const SELECTION& aSelection, PCB_BASE_FRAME* aFrame )
{
    return aFrame->GetViewerSettingsBase()->m_ViewersDisplay.m_DisplayTextFill;
}


bool PCB_EDITOR_CONDITIONS::graphicsFillDisplayFunc( const SELECTION& aSelection, PCB_BASE_FRAME* aFrame )
{
    return aFrame->GetViewerSettingsBase()->m_ViewersDisplay.m_DisplayGraphicsFill;
}


bool PCB_EDITOR_CONDITIONS::viaFillDisplayFunc( const SELECTION& aSelection, PCB_BASE_FRAME* aFrame )
{
    return aFrame->GetPcbNewSettings()->m_Display.m_DisplayViaFill;
}


bool PCB_EDITOR_CONDITIONS::trackFillDisplayFunc( const SELECTION& aSelection, PCB_BASE_FRAME* aFrame )
{
    return aFrame->GetPcbNewSettings()->m_Display.m_DisplayPcbTrackFill;
}


bool PCB_EDITOR_CONDITIONS::zoneDisplayModeFunc( const SELECTION& aSelection, PCB_BASE_FRAME* aFrame,
                                                 ZONE_DISPLAY_MODE aMode )
{
    return aFrame->GetDisplayOptions().m_ZoneDisplayMode == aMode;
}


bool PCB_EDITOR_CONDITIONS::footprintViewerAutoZoom( const SELECTION& aSelection, PCB_BASE_FRAME* aFrame )
{
    return static_cast<PCB_VIEWERS_SETTINGS_BASE*>( aFrame->config() )->m_FootprintViewerAutoZoomOnSelect;
}
