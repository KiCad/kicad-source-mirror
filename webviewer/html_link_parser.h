/**
 * @file html_link_parse.h
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

#ifndef HTML_LINK_PARSE_H
#define HTML_LINK_PARSE_H


#include <wx/wx.h>
#include <wx/html/htmlpars.h>

class HTML_LINK_PARSER ;

/**
 * a Tag parser, to extract tagged data in html text.
 * this tag handler extract a url link, givent by tag "A"
 * like:
 * "<a href="/KiCad/Valves.pretty" itemprop="name codeRepository"> Valves.pretty</a>"
 * the tag is "a"
 * and the link is the parameter given by "href"
 */
class LINK_TAGHANDLER : public wxHtmlTagHandler
{
    HTML_LINK_PARSER* m_Parser;

public:
    LINK_TAGHANDLER() : m_Parser( NULL )
    {
    }

    LINK_TAGHANDLER( HTML_LINK_PARSER* aParser ) : m_Parser( aParser )
    {
    }

    wxString GetSupportedTags()
    {
        return "A";
    }

    bool HandleTag(const wxHtmlTag& tag);
};

/**
 * The engine to parse a html text and extract useful data
 * Here, the useful data are url links
 */
class HTML_LINK_PARSER : public wxHtmlParser
{
    const wxString& m_src;          // the html text to parse
    wxArrayString& stringUrls;      // the strings extracted from html text
    bool (*m_filter)( const wxString& aData );  // a callback function to filter strings

public:

    HTML_LINK_PARSER( const wxString& aSrc, wxArrayString& aUrls );

    /**
     * Parse the html text and store links in stringUrls
     * Stored links can be filtered if aFilter is non NULL
     * @param aFilter a filtering function ( bool aFilter( const wxString& aData ) )
     * which return true if the tex(t aData must be stored.
     */
    void ParseLinks( bool (*aFilter)( const wxString& aData ) )
    {
        m_filter = aFilter;
        Parse(m_src);
    }

    // virtual pure from wxHtmlParser, do nothing here, but needed.
    void AddText( const wxString& aText ){}

    // Our "AddText" used to store selected text (the url link)
    void AddString( const wxString& aText );

    wxObject* GetProduct()
    {
        return NULL;
    }
};

#endif      // ifndef HTML_LINK_PARSE_H
