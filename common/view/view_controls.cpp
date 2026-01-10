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

#include <gal/graphics_abstraction_layer.h>
#include <tool/actions.h>
#include <view/view.h>
#include <view/view_controls.h>

using namespace KIGFX;

void VIEW_CONTROLS::ShowCursor( bool aEnabled )
{
    m_settings.m_showCursor = aEnabled;
    m_view->GetGAL()->SetCursorEnabled( aEnabled );
}


bool VIEW_CONTROLS::IsCursorShown() const
{
    // this only says if the VIEW_CONTROLS say the cursor should be
    // shown: m_view->GetGAL()->IsCursorEnabled() will say if the GAL is
    // actually going to do show the cursor or not
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
    m_showCursor                      = false;
    m_forcedPosition                  = { 0.0, 0.0 };
    m_forceCursorPosition             = false;
    m_cursorCaptured                  = false;
    m_snappingEnabled                 = true;
    m_grabMouse                       = false;
    m_focusFollowSchPcb               = false;
    m_autoPanEnabled                  = false;
    m_autoPanSettingEnabled           = false;
    m_autoPanMargin                   = 0.02f;
    m_autoPanSpeed                    = 0.15f;
    m_autoPanAcceleration             = 5.0f;
    m_warpCursor                      = false;
    m_horizontalPan                   = false;
    m_zoomAcceleration                = false;
    m_zoomSpeed                       = 5;
    m_zoomSpeedAuto                   = true;
    m_scrollModifierZoom              = 0;
    m_scrollModifierPanH              = WXK_CONTROL;
    m_scrollModifierPanV              = WXK_SHIFT;
    m_motionPanModifier               = 0;
    m_dragLeft                        = MOUSE_DRAG_ACTION::NONE;
    m_dragMiddle                      = MOUSE_DRAG_ACTION::PAN;
    m_dragRight                       = MOUSE_DRAG_ACTION::PAN;
    m_lastKeyboardCursorPositionValid = false;
    m_lastKeyboardCursorPosition      = { 0.0, 0.0 };
    m_lastKeyboardCursorCommand       = ACTIONS::CURSOR_NONE;
    m_scrollReverseZoom               = false;
    m_scrollReversePanH               = false;
}


void VIEW_CONTROLS::ApplySettings( const VC_SETTINGS& aSettings )
{
    ShowCursor( aSettings.m_showCursor );
    CaptureCursor( aSettings.m_cursorCaptured );
    SetGrabMouse( aSettings.m_grabMouse );
    SetAutoPan( aSettings.m_autoPanEnabled );
    SetAutoPanMargin( aSettings.m_autoPanMargin );
    SetAutoPanSpeed( aSettings.m_autoPanSpeed );
    ForceCursorPosition( aSettings.m_forceCursorPosition, aSettings.m_forcedPosition );
}
