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

#include <windows.h>

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


wxColour KIPLATFORM::UI::GetDialogBGColour()
{
    return wxSystemSettings::GetColour( wxSYS_COLOUR_BTNFACE );
}


void KIPLATFORM::UI::ForceFocus( wxWindow* aWindow )
{
    aWindow->SetFocus();
}


bool KIPLATFORM::UI::IsWindowActive( wxWindow* aWindow )
{
    if(! aWindow )
    {
	    return false;
    }

    return ( aWindow->GetHWND() == GetForegroundWindow() );
}


void KIPLATFORM::UI::ReparentModal( wxNonOwnedWindow* aWindow )
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
    case wxCURSOR_BULLSEYE:
    case wxCURSOR_HAND:
    case wxCURSOR_ARROW:
        return true;
    default:
        return false;
    }
}


void KIPLATFORM::UI::LargeChoiceBoxHack( wxChoice* aChoice )
{
    // Not implemented
}


void KIPLATFORM::UI::EllipsizeChoiceBox( wxChoice* aChoice )
{
    // Not implemented
}


double KIPLATFORM::UI::GetPixelScaleFactor( const wxWindow* aWindow )
{
    return aWindow->GetContentScaleFactor();
}


double KIPLATFORM::UI::GetContentScaleFactor( const wxWindow* aWindow )
{
    return aWindow->GetDPIScaleFactor();
}


wxSize KIPLATFORM::UI::GetUnobscuredSize( const wxWindow* aWindow )
{
    return aWindow->GetClientSize();
}


void KIPLATFORM::UI::SetOverlayScrolling( const wxWindow* aWindow, bool overlay )
{
    // Not implemented
}


bool KIPLATFORM::UI::AllowIconsInMenus()
{
    return true;
}


wxPoint KIPLATFORM::UI::GetMousePosition()
{
    return wxGetMousePosition();
}


bool KIPLATFORM::UI::WarpPointer( wxWindow* aWindow, int aX, int aY )
{
    aWindow->WarpPointer( aX, aY );
    return true;
}


void KIPLATFORM::UI::ImmControl( wxWindow* aWindow, bool aEnable )
{
    if ( !aEnable )
    {
        ImmAssociateContext( aWindow->GetHWND(), NULL );
    }
    else
    {
        ImmAssociateContextEx( aWindow->GetHWND(), 0, IACE_DEFAULT );
    }
}


bool KIPLATFORM::UI::InfiniteDragPrepareWindow( wxWindow* aWindow )
{
    return true;
}


void KIPLATFORM::UI::InfiniteDragReleaseWindow()
{
    // Not needed on this platform
}


void KIPLATFORM::UI::SetFloatLevel( wxWindow* aWindow )
{
}