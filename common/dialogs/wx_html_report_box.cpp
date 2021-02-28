/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020 KiCad Developers, see AUTHORS.txt for contributors.
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
#include "wx_html_report_box.h"


WX_HTML_REPORT_BOX::WX_HTML_REPORT_BOX( wxWindow* parent, wxWindowID id, const wxPoint& pos,
                                        const wxSize& size, long style ) :
    wxHtmlWindow( parent, id, pos, size, style ),
    m_units( EDA_UNITS::MILLIMETRES ),
    m_immediateMode( false )
{
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


wxString fixLinespacing( wxHtmlCell* aCell, int aMinLinespacing )
{
    // wxWidgets default linespacing is about 110% of font-height (which is way too small),
    // and the default paragraph spacing is about 200% (which is too big).  The heading,
    // bullet lists, etc. line spacing is fine.
    //
    // And of course they provide no way to set it, which leaves us with very few options.
    // Fortunately we know we're dealing mostly with single lines in the reporter so we apply
    // an egregious hack and enforce a minimum linespacing for things ending in '.' (which
    // normally won't include headings or bullet list items).

    wxString             content = aCell->ConvertToText( nullptr );
    wxHtmlContainerCell* container = dynamic_cast<wxHtmlContainerCell*>( aCell );

    for( wxHtmlCell* child = aCell->GetFirstChild(); child; child = child->GetNext() )
        content += fixLinespacing( child, aMinLinespacing );

    if( container && content.EndsWith( "." ) && container->GetHeight() < aMinLinespacing )
        container->SetMinHeight( aMinLinespacing );

    return content;
}


void WX_HTML_REPORT_BOX::Flush()
{
    wxString html;

    for( const wxString& line : m_messages )
        html += generateHtml( line );

    SetPage( addHeader( html ) );

    wxFont font = wxSystemSettings::GetFont( wxSYS_DEFAULT_GUI_FONT );
    int    minLineHeight = KiROUND( font.GetPixelSize().y * 1.3 );

    fixLinespacing( GetInternalRepresentation(), minLineHeight );
}


wxString WX_HTML_REPORT_BOX::addHeader( const wxString& aBody )
{
    wxColour bgcolor = wxSystemSettings::GetColour( wxSYS_COLOUR_WINDOW );
    wxColour fgcolor = wxSystemSettings::GetColour( wxSYS_COLOUR_WINDOWTEXT );

    return wxString::Format( wxT( "<html>"
                                  "  <body bgcolor='%s' text='%s'>"
                                  "    %s"
                                  "  </body>"
                                  "</html>" ),
                             bgcolor.GetAsString( wxC2S_HTML_SYNTAX ),
                             fgcolor.GetAsString( wxC2S_HTML_SYNTAX ),
                             aBody );
}


wxString WX_HTML_REPORT_BOX::generateHtml( const wxString& aLine )
{
    return aLine + "<br>";
}


void WX_HTML_REPORT_BOX::Clear()
{
    m_messages.clear();
}

