/**
 * @file html_link_parse.cpp
 */

/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 1992-2014 KiCad Developers, see AUTHORS.txt for contributors.
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

/*
 * wxWidgets gives very few info about wxwebkit. For more info and more comments:
 * see https://forums.wxwidgets.org/viewtopic.php?f=1&t=1119#
 */

#include <wx/wx.h>
#include <wx/html/htmlpars.h>
#include "html_link_parser.h"

bool LINK_TAGHANDLER::HandleTag(const wxHtmlTag& tag)
{
    if( tag.HasParam( wxT("HREF") ) )
    {
        wxString href( tag.GetParam( wxT("HREF") ) );
        // Add the first parameter (the link)
        m_Parser->AddString( href );
        // Parse other params, but do nothing, becuse the AddText() callback
        // do nothing
        ParseInner(tag);
        return true;
    }
    else
        return false;
}

HTML_LINK_PARSER::HTML_LINK_PARSER( const wxString& aSrc, wxArrayString& aUrls )
        : m_src( aSrc ), stringUrls( aUrls )
{
    AddTagHandler( new LINK_TAGHANDLER(this) );
}


void HTML_LINK_PARSER::AddString( const wxString& aText )
{
    wxString text = aText;
    text.Trim( true );
    text.Trim( false );

    if( ! m_filter || m_filter( text ) )
    {
        stringUrls.Add( text );
    }
}
