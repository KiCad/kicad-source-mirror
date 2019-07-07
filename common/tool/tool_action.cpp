/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013 CERN
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

#include <tool/tool_action.h>
#include <tool/action_manager.h>

#include <algorithm>
#include <hotkeys_basic.h>

TOOL_ACTION::TOOL_ACTION( const std::string& aName, TOOL_ACTION_SCOPE aScope,
                          int aDefaultHotKey, const std::string& aLegacyHotKeyName,
                          const wxString& aLabel, const wxString& aTooltip,
                          const BITMAP_OPAQUE* aIcon, TOOL_ACTION_FLAGS aFlags, void* aParam ) :
        m_name( aName ),
        m_scope( aScope ),
        m_defaultHotKey( aDefaultHotKey ),
        m_legacyName( aLegacyHotKeyName ),
        m_label( aLabel ),
        m_tooltip( aTooltip ),
        m_icon( aIcon ),
        m_id( -1 ),
        m_flags( aFlags ),
        m_param( aParam )
{
    SetHotKey( aDefaultHotKey );
    ACTION_MANAGER::GetActionList().push_back( this );
}


TOOL_ACTION::TOOL_ACTION() :
        m_scope( AS_GLOBAL ),
        m_defaultHotKey( 0 ),
        m_icon( nullptr ),
        m_id( -1 ),
        m_flags( AF_NONE ),
        m_param( nullptr )
{
    SetHotKey( 0 );
}


TOOL_ACTION::~TOOL_ACTION()
{
    ACTION_MANAGER::GetActionList().remove( this );
}


wxString TOOL_ACTION::GetLabel() const
{
    return wxGetTranslation( m_label );
}


wxString TOOL_ACTION::GetMenuItem() const
{
    wxString label = wxGetTranslation( m_label );
    label.Replace( "&", "&&" );
    return AddHotkeyName( label, m_hotKey, IS_HOTKEY );
}


wxString TOOL_ACTION::GetDescription() const
{
    wxString tooltip = wxGetTranslation( m_tooltip );

    if( GetHotKey() )
        tooltip += wxString::Format( wxT( "  (%s)" ), KeyNameFromKeyCode( GetHotKey() ) );

    return tooltip;
}


void TOOL_ACTION::SetHotKey( int aKeycode )
{
    m_hotKey = aKeycode;
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
