/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <type_traits>
#include <wx/defs.h>
#include <pgm_base.h>
#include <settings/settings_manager.h>
#include "panel_simulator_preferences.h"
#include "../eeschema_settings.h"


PANEL_SIMULATOR_PREFERENCES::PANEL_SIMULATOR_PREFERENCES( wxWindow* aParent ) :
        PANEL_SIMULATOR_PREFERENCES_BASE( aParent )
{
#ifdef __WXOSX_MAC__
    m_lblVScrollCtrl->SetLabel( _( "Cmd" ) );
    m_lblVScrollAlt->SetLabel( _( "Option" ) );
#endif

    // Populate the wxChoice items programmatically here instead of via the form builder
    // to ease maintenance.

    static const wxString verticalChoiceItems[] =
    {
        _( "No action" ),
        _( "Pan left/right" ),
        _( "Pan right/left" ),
        _( "Pan up/down" ),
        _( "Zoom" ),
        _( "Zoom horizontally" ),
        _( "Zoom vertically" )
    };

    static constexpr auto ACTION_COUNT = static_cast<unsigned>( SIM_MOUSE_WHEEL_ACTION::COUNT );

    static_assert( std::extent<decltype(verticalChoiceItems)>::value == ACTION_COUNT,
                   "verticalChoiceItems size does not match VERTICAL_SCROLL_ACTION::COUNT" );

    m_choiceVScrollUnmodified->Set( ACTION_COUNT, verticalChoiceItems );
    m_choiceVScrollCtrl      ->Set( ACTION_COUNT, verticalChoiceItems );
    m_choiceVScrollShift     ->Set( ACTION_COUNT, verticalChoiceItems );
    m_choiceVScrollAlt       ->Set( ACTION_COUNT, verticalChoiceItems );

    static const wxString horizontalChoiceItems[] =
    {
        _( "No action" ),
        _( "Pan left/right" ),
        _( "Zoom horizontally" )
    };

    m_choiceHScroll->Set( std::extent<decltype(horizontalChoiceItems)>::value,
                          horizontalChoiceItems );
}


PANEL_SIMULATOR_PREFERENCES::~PANEL_SIMULATOR_PREFERENCES() = default;


void PANEL_SIMULATOR_PREFERENCES::ResetPanel()
{
    applyMouseScrollActionsToPanel( SIM_MOUSE_WHEEL_ACTION_SET::GetMouseDefaults() );
}


bool PANEL_SIMULATOR_PREFERENCES::TransferDataFromWindow()
{
    static constexpr auto toAction =
            []( const wxChoice* aChoice )
            {
                return static_cast<SIM_MOUSE_WHEEL_ACTION>( aChoice->GetSelection() );
            };

    if( EESCHEMA_SETTINGS* cfg = GetAppSettings<EESCHEMA_SETTINGS>( "eeschema" ) )
    {
        SIM_MOUSE_WHEEL_ACTION_SET& actions = cfg->m_Simulator.preferences.mouse_wheel_actions;

        actions.vertical_unmodified = toAction( m_choiceVScrollUnmodified );
        actions.vertical_with_ctrl  = toAction( m_choiceVScrollCtrl );
        actions.vertical_with_shift = toAction( m_choiceVScrollShift );
        actions.vertical_with_alt   = toAction( m_choiceVScrollAlt );

        actions.horizontal = horizontalScrollSelectionToAction( m_choiceHScroll->GetSelection() );
    }

    return true;
}


bool PANEL_SIMULATOR_PREFERENCES::TransferDataToWindow()
{
    if( EESCHEMA_SETTINGS* cfg = GetAppSettings<EESCHEMA_SETTINGS>( "eeschema" ) )
        applyMouseScrollActionsToPanel( cfg->m_Simulator.preferences.mouse_wheel_actions );

    return true;
}


void PANEL_SIMULATOR_PREFERENCES::onMouseDefaults( wxCommandEvent& )
{
    applyMouseScrollActionsToPanel( SIM_MOUSE_WHEEL_ACTION_SET::GetMouseDefaults() );
}


void PANEL_SIMULATOR_PREFERENCES::onTrackpadDefaults( wxCommandEvent& )
{
    applyMouseScrollActionsToPanel( SIM_MOUSE_WHEEL_ACTION_SET::GetTrackpadDefaults() );
}


SIM_MOUSE_WHEEL_ACTION
PANEL_SIMULATOR_PREFERENCES::horizontalScrollSelectionToAction( int aSelection )
{
    switch( aSelection )
    {
    case 0: return SIM_MOUSE_WHEEL_ACTION::NONE;
    case 1: return SIM_MOUSE_WHEEL_ACTION::PAN_LEFT_RIGHT;
    case 2: return SIM_MOUSE_WHEEL_ACTION::ZOOM_HORIZONTALLY;
    default: break;
    }

    return SIM_MOUSE_WHEEL_ACTION::NONE;
}


int PANEL_SIMULATOR_PREFERENCES::actionToHorizontalScrollSelection( SIM_MOUSE_WHEEL_ACTION a )
{
    switch( a )
    {
    case SIM_MOUSE_WHEEL_ACTION::NONE:              return 0;
    case SIM_MOUSE_WHEEL_ACTION::PAN_LEFT_RIGHT:    return 1;
    case SIM_MOUSE_WHEEL_ACTION::ZOOM_HORIZONTALLY: return 2;
    default:                                        break;
    }

    return 0;
}


void PANEL_SIMULATOR_PREFERENCES::applyMouseScrollActionsToPanel(
        const SIM_MOUSE_WHEEL_ACTION_SET& anActionSet )
{
    static constexpr auto setSelection =
            []( wxChoice* aChoice, auto action )
            {
                aChoice->SetSelection( static_cast<int>( action ) );
            };

    setSelection( m_choiceVScrollUnmodified, anActionSet.vertical_unmodified );
    setSelection( m_choiceVScrollCtrl,       anActionSet.vertical_with_ctrl );
    setSelection( m_choiceVScrollShift,      anActionSet.vertical_with_shift );
    setSelection( m_choiceVScrollAlt,        anActionSet.vertical_with_alt );

    m_choiceHScroll->SetSelection( actionToHorizontalScrollSelection( anActionSet.horizontal ) );
}
