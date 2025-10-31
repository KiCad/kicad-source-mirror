/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 2 of the License, or (at your
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


#include <math/util.h>
#include <common.h>
#include <wx/settings.h>
#include <wx/textctrl.h>
#include <wx/menu.h>
#include <wx/clipbrd.h>
#include <wx/log.h>
#include "wx_html_report_box.h"


WX_HTML_REPORT_BOX::WX_HTML_REPORT_BOX( wxWindow* parent, wxWindowID id, const wxPoint& pos,
                                        const wxSize& size, long style ) :
        HTML_WINDOW( parent, id, pos, size, style ),
        m_units( EDA_UNITS::MM ),
        m_immediateMode( false )
{
    Flush();

    Bind( wxEVT_SYS_COLOUR_CHANGED, wxSysColourChangedEventHandler( WX_HTML_REPORT_BOX::onThemeChanged ), this );
}


WX_HTML_REPORT_BOX::~WX_HTML_REPORT_BOX()
{
	// Disconnect Events
    Unbind( wxEVT_SYS_COLOUR_CHANGED, wxSysColourChangedEventHandler( WX_HTML_REPORT_BOX::onThemeChanged ), this );
}


void WX_HTML_REPORT_BOX::onThemeChanged( wxSysColourChangedEvent &aEvent )
{
    Flush();

    aEvent.Skip();
}


REPORTER& WX_HTML_REPORT_BOX::Report( const wxString& aText, SEVERITY aSeverity )
{
    m_messages.push_back( aText );

    if( m_immediateMode )
    {
        Flush();
        int px, py, x, y;
        GetScrollPixelsPerUnit( &px, &py );
        GetVirtualSize( &x, &y );
        Scroll( -1, y * py );
    }

    return *this;
}


void WX_HTML_REPORT_BOX::Flush()
{
    wxString html;

    for( const wxString& line : m_messages )
        html += generateHtml( line );

    SetPage( html );
}


wxString WX_HTML_REPORT_BOX::generateHtml( const wxString& aLine )
{
    // wxWidgets default linespacing is about 110% of font-height (which is way too small),
    // and the default paragraph spacing is about 200% (which is too big).  The heading,
    // bullet lists, etc. line spacing is fine.
    //
    // And of course they provide no way to set it, which leaves us with very few options.
    // Fortunately we know we're dealing mostly with single lines in the reporter so we apply
    // an egregious hack and enforce a minimum linespacing by inserting an invisible img
    // element with appropriate height

    wxFont font = wxSystemSettings::GetFont( wxSYS_DEFAULT_GUI_FONT );
    int    additionalLineSpacing = KiROUND( font.GetPixelSize().y * 0.6 );

    return wxString::Format( wxT( "<img align=texttop height=%d width=0 src=#>%s<br>" ),
                             additionalLineSpacing, aLine );
}


void WX_HTML_REPORT_BOX::Clear()
{
    REPORTER::Clear();
    m_messages.clear();
}

