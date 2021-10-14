/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2021 Mikołaj Wielgus <wielgusmikolaj@gmail.com>
 * Copyright (C) 2021 KiCad Developers, see AUTHORS.TXT for contributors.
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

#include <html_window.h>
#include <wx/settings.h>


HTML_WINDOW::HTML_WINDOW( wxWindow* aParent, wxWindowID aId, const wxPoint& aPos,
                          const wxSize& aSize, long aStyle, const wxString& aName ) :
        wxHtmlWindow( aParent, aId, aPos, aSize, aStyle, aName )
{
    Bind( wxEVT_SYS_COLOUR_CHANGED,
          wxSysColourChangedEventHandler( HTML_WINDOW::onThemeChanged ), this );
}


bool HTML_WINDOW::SetPage( const wxString& aSource )
{
    m_pageSource = aSource;

    wxColour fgColor   = wxSystemSettings::GetColour( wxSYS_COLOUR_WINDOWTEXT );
    wxColour bgColor   = wxSystemSettings::GetColour( wxSYS_COLOUR_WINDOW );
    wxColour linkColor = wxSystemSettings::GetColour( wxSYS_COLOUR_HOTLIGHT );

    wxString html = wxString::Format( wxT( "<html>\n<body text='%s' bgcolor='%s' link='%s'>\n" ),
                                      fgColor.GetAsString( wxC2S_HTML_SYNTAX ),
                                      bgColor.GetAsString( wxC2S_HTML_SYNTAX ),
                                      linkColor.GetAsString( wxC2S_HTML_SYNTAX ) );
    html.Append( aSource );
    html.Append( wxT( "\n</body>\n</html>" ) );

    return wxHtmlWindow::SetPage( html );
}


bool HTML_WINDOW::AppendToPage( const wxString& aSource )
{
    return SetPage( m_pageSource + aSource );
}


void HTML_WINDOW::ThemeChanged()
{
    SetPage( m_pageSource );
}


void HTML_WINDOW::onThemeChanged( wxSysColourChangedEvent &aEvent )
{
    ThemeChanged();
}
