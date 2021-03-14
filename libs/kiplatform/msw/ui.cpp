/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020 Ian McInerney <Ian.S.McInerney at ieee.org>
 * Copyright (C) 2020 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <kiplatform/ui.h>

#include <wx/cursor.h>
#include <wx/nonownedwnd.h>
#include <wx/window.h>
#include <wx/msw/registry.h>


bool KIPLATFORM::UI::IsDarkTheme()
{
    // NOTE: Disabled for now because we can't yet react to dark mode in Windows reasonably:
    // Windows 10 dark mode does not change the values returned by wxSystemSettings::GetColour()
    // so our window backgrounds, text colors, etc will stay in "light mode" until either wxWidgets
    // implements something or we apply a custom theme ourselves.
#ifdef NOTYET
    const wxString lightModeKey = wxT( "AppsUseLightTheme" );

    // Note: registry used because there is not yet an official API for this yet.
    // This may stop working on future Windows versions
    wxRegKey themeKey( wxRegKey::HKCU,
                       wxT( "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize" ) );

    if( !themeKey.Exists() )
        return false;

    if( !themeKey.HasValue( lightModeKey ) )
        return false;

    long val = 0;

    if( !themeKey.QueryValue( lightModeKey, &val ) )
        return false;

    return ( val == 0 );
#else
    wxColour bg = wxSystemSettings::GetColour( wxSYS_COLOUR_WINDOW );

    // Weighted W3C formula
    double brightness = ( bg.Red() / 255.0 ) * 0.299 +
        ( bg.Green() / 255.0 ) * 0.587 +
        ( bg.Blue() / 255.0 ) * 0.117;

    return brightness < 0.5;
#endif
}


void KIPLATFORM::UI::ForceFocus( wxWindow* aWindow )
{
    aWindow->SetFocus();
}


void KIPLATFORM::UI::ReparentQuasiModal( wxNonOwnedWindow* aWindow )
{
    // Not needed on this platform
}


void KIPLATFORM::UI::FixupCancelButtonCmdKeyCollision( wxWindow *aWindow )
{
    // Not needed on this platform
}


bool KIPLATFORM::UI::IsStockCursorOk( wxStockCursor aCursor )
{
    switch( aCursor )
    {
    case wxCURSOR_SIZING:
    case wxCURSOR_BULLSEYE:
    case wxCURSOR_HAND:
    case wxCURSOR_ARROW:
        return true;
    default:
        return false;
    }
}


void KIPLATFORM::UI::EllipsizeChoiceBox( wxChoice* aChoice )
{
    // Not implemented
}


double KIPLATFORM::UI::GetSystemScaleFactor( const wxWindow* aWindow )
{
    return aWindow->GetContentScaleFactor();
}
