/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012 Torsten Hueter, torstenhtr <at> gmx.de
 * Copyright (C) 2013-2015 CERN
 * @author Tomasz Wlostowski <tomasz.wlostowski@cern.ch>
 * @author Maciej Suminski <maciej.suminski@cern.ch>
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

#include <view/view.h>
#include <view/view_controls.h>
#include <gal/graphics_abstraction_layer.h>

using namespace KIGFX;

void VIEW_CONTROLS::ShowCursor( bool aEnabled )
{
    m_settings.m_showCursor = aEnabled;
    m_view->GetGAL()->SetCursorEnabled( aEnabled );
}


bool VIEW_CONTROLS::IsCursorShown() const
{
    assert( m_settings.m_showCursor == m_view->GetGAL()->IsCursorEnabled() );
    return m_settings.m_showCursor;
}


void VIEW_CONTROLS::Reset()
{
    // Get the default settings from the default constructor
    VC_SETTINGS dummy;
    ApplySettings( dummy );
}


void VC_SETTINGS::Reset()
{
    m_showCursor = false;
    m_forceCursorPosition = false;
    m_cursorCaptured = false;
    m_snappingEnabled = false;
    m_grabMouse = false;
    m_autoPanEnabled = false;
    m_autoPanMargin = 0.1;
    m_autoPanSpeed = 0.15;
    m_warpCursor = false;
    m_enableMousewheelPan = false;
}


void VIEW_CONTROLS::ApplySettings( const VC_SETTINGS& aSettings )
{
    ShowCursor( aSettings.m_showCursor );
    CaptureCursor( aSettings.m_cursorCaptured );
    SetSnapping( aSettings.m_snappingEnabled );
    SetGrabMouse( aSettings.m_grabMouse );
    SetAutoPan( aSettings.m_autoPanEnabled );
    SetAutoPanMargin( aSettings.m_autoPanMargin );
    SetAutoPanSpeed( aSettings.m_autoPanSpeed );
    ForceCursorPosition( aSettings.m_forceCursorPosition, aSettings.m_forcedPosition );
}
