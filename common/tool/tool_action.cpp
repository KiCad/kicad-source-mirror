/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013-2023 CERN
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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

#include <optional>
#include <tool/tool_action.h>
#include <tool/tool_event.h>
#include <tool/action_manager.h>

#include <algorithm>
#include <bitmaps.h>
#include <hotkeys_basic.h>

#include <core/wx_stl_compat.h>
#include <wx/string.h>
#include <wx/translation.h>

TOOL_ACTION::TOOL_ACTION( const std::string& aName, TOOL_ACTION_SCOPE aScope,
                          int aDefaultHotKey, const std::string& aLegacyHotKeyName,
                          const wxString& aLabel, const wxString& aTooltip,
                          BITMAPS aIcon, TOOL_ACTION_FLAGS aFlags ) :
        m_name( aName ),
        m_scope( aScope ),
        m_group( std::nullopt ),
        m_defaultHotKey( aDefaultHotKey ),
        m_defaultHotKeyAlt( 0 ),
        m_legacyName( aLegacyHotKeyName ),
        m_menuLabel( aLabel ),
        m_tooltip( aTooltip ),
        m_icon( aIcon ),
        m_id( -1 ),
        m_flags( aFlags )
{
    SetHotKey( aDefaultHotKey );
    ACTION_MANAGER::GetActionList().push_back( this );
}


TOOL_ACTION::TOOL_ACTION() :
        m_scope( AS_GLOBAL ),
        m_group( std::nullopt ),
        m_defaultHotKey( 0 ),
        m_defaultHotKeyAlt( 0 ),
        m_id( -1 ),
        m_flags( AF_NONE )
{
    SetHotKey( 0 );
}


TOOL_ACTION::TOOL_ACTION( const TOOL_ACTION_ARGS& aArgs ) :
        m_name( aArgs.m_name.value_or( "" ) ),
        m_scope( aArgs.m_scope.value_or( AS_CONTEXT ) ),
        m_defaultHotKey( aArgs.m_defaultHotKey.value_or( 0 ) ),
        m_defaultHotKeyAlt( aArgs.m_defaultHotKeyAlt.value_or( 0 ) ),
        m_hotKey( aArgs.m_defaultHotKey.value_or( 0 ) ),
        m_hotKeyAlt( 0 ),
        m_legacyName( aArgs.m_legacyName.value_or( "" ) ),
        m_friendlyName( TowxString( aArgs.m_friendlyName.value_or( "" ) ) ),
        m_tooltip( TowxString( aArgs.m_tooltip.value_or( "" ) ) ),
        m_id( -1 ),
        m_uiid( std::nullopt ),
        m_flags( aArgs.m_flags.value_or( AF_NONE ) )
{
    // Action name is the only mandatory part
    assert( !m_name.empty() );

    if( aArgs.m_menuText.has_value() )
        m_menuLabel = TowxString( aArgs.m_menuText.value() );

    if( aArgs.m_uiid.has_value() )
        m_uiid = aArgs.m_uiid.value();

    if( aArgs.m_param.has_value() )
        m_param = aArgs.m_param;

    if( aArgs.m_description.has_value() )
        m_description = TowxString( aArgs.m_description.value() );

    if( aArgs.m_group.has_value() )
        m_group = aArgs.m_group;

    if( aArgs.m_icon.has_value() )
        m_icon = aArgs.m_icon.value();

    if( aArgs.m_toolbarState.has_value() )
        m_toolbarState = aArgs.m_toolbarState.value();

    // If there is no icon, then the action should be hidden from the toolbar
    if( !m_icon.has_value() )
        m_toolbarState.set( static_cast<size_t>( TOOLBAR_STATE::HIDDEN ) );

    ACTION_MANAGER::GetActionList().push_back( this );
}


TOOL_ACTION::~TOOL_ACTION()
{
    ACTION_MANAGER::GetActionList().remove( this );
}


TOOL_EVENT TOOL_ACTION::MakeEvent() const
{
    TOOL_EVENT evt;

    if( IsActivation() )
        evt = TOOL_EVENT( TC_COMMAND, TA_ACTIVATE, m_name, m_scope );
    else if( IsNotification() )
        evt = TOOL_EVENT( TC_MESSAGE, TA_NONE, m_name, m_scope );
    else
        evt = TOOL_EVENT( TC_COMMAND, TA_ACTION, m_name, m_scope );

    if( m_group.has_value() )
    {
        evt.SetActionGroup( m_group.value() );
    }

    if( m_param.has_value() )
        evt.SetParameter( m_param );

    return evt;
}


wxString TOOL_ACTION::GetFriendlyName() const
{
    if( m_friendlyName.empty() )
        return wxEmptyString;

    return wxGetTranslation( m_friendlyName );
}


wxString TOOL_ACTION::GetMenuLabel() const
{
    if( m_menuLabel.has_value() )
        return wxGetTranslation( m_menuLabel.value() );

    return GetFriendlyName();
}


wxString TOOL_ACTION::GetMenuItem() const
{
    wxString label = GetMenuLabel();
    label.Replace( wxS( "&" ), wxS( "&&" ) );
    return AddHotkeyName( label, m_hotKey, IS_HOTKEY );
}


wxString TOOL_ACTION::GetDescription() const
{
    // If no description provided, use the tooltip without a hotkey
    if( !m_description.has_value() )
        return GetTooltip( false );

    return wxGetTranslation( m_description.value() );
}


wxString TOOL_ACTION::GetTooltip( bool aIncludeHotkey ) const
{
    wxString tooltip = wxGetTranslation( m_tooltip );

    if( aIncludeHotkey && GetHotKey() )
        tooltip += wxString::Format( wxT( "  (%s)" ), KeyNameFromKeyCode( GetHotKey() ) );

    return tooltip;
}


wxString TOOL_ACTION::GetButtonTooltip() const
{
    // We don't show button text so use the action name as the first line of the tooltip
    wxString tooltip = GetFriendlyName();

    if( GetHotKey() )
        tooltip += wxString::Format( wxT( "\t(%s)" ), KeyNameFromKeyCode( GetHotKey() ) );

    if( !GetTooltip( false ).IsEmpty() )
        tooltip += '\n' + GetTooltip( false );

    return tooltip;
}


void TOOL_ACTION::SetHotKey( int aKeycode, int aKeycodeAlt )
{
    m_hotKey = aKeycode;
    m_hotKeyAlt = aKeycodeAlt;
}


std::string TOOL_ACTION::GetToolName() const
{
    int dotCount = std::count( m_name.begin(), m_name.end(), '.' );

    switch( dotCount )
    {
    case 0:
        assert( false );    // Invalid action name format
        return "";

    case 1:
        return m_name;

    case 2:
        return m_name.substr( 0, m_name.rfind( '.' ) );

    default:
        assert( false );    // TODO not implemented
        return "";
    }
}
