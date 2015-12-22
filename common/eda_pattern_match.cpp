/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015 Chris Pavlina <pavlina.chris@gmail.com>
 * Copyright (C) 2015 KiCad Developers, see change_log.txt for contributors.
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

#include <eda_pattern_match.h>
#include <wx/log.h>
#include <climits>

bool EDA_PATTERN_MATCH_SUBSTR::SetPattern( const wxString& aPattern )
{
    m_pattern = aPattern;
    return true;
}


int EDA_PATTERN_MATCH_SUBSTR::Find( const wxString& aCandidate ) const
{
    int loc = aCandidate.Find( m_pattern );

    return ( loc == wxNOT_FOUND ) ? EDA_PATTERN_NOT_FOUND : loc;
}


/**
 * Context class to set wx loglevel for a block, and always restore it at the end.
 */
class WX_LOGLEVEL_CONTEXT
{
    wxLogLevel m_old_level;

public:
    WX_LOGLEVEL_CONTEXT( wxLogLevel level )
    {
        m_old_level = wxLog::GetLogLevel();
        wxLog::SetLogLevel( level );
    }

    ~WX_LOGLEVEL_CONTEXT()
    {
        wxLog::SetLogLevel( m_old_level );
    }
};


bool EDA_PATTERN_MATCH_REGEX::SetPattern( const wxString& aPattern )
{
    m_pattern = aPattern;

    // Evil and undocumented: wxRegEx::Compile calls wxLogError on error, even
    // though it promises to just return false. Silence the error.
    WX_LOGLEVEL_CONTEXT ctx( wxLOG_FatalError );

    return m_regex.Compile( aPattern, wxRE_ADVANCED );
}


int EDA_PATTERN_MATCH_REGEX::Find( const wxString& aCandidate ) const
{
    if( m_regex.IsValid() )
    {
        if( m_regex.Matches( aCandidate ) )
        {
            size_t start, len;
            m_regex.GetMatch( &start, &len, 0 );
            return ( start > INT_MAX ) ? INT_MAX : start;
        }
        else
        {
            return EDA_PATTERN_NOT_FOUND;
        }
    }
    else
    {
        int loc = aCandidate.Find( m_pattern );
        return ( loc == wxNOT_FOUND ) ? EDA_PATTERN_NOT_FOUND : loc;
    }
}


bool EDA_PATTERN_MATCH_WILDCARD::SetPattern( const wxString& aPattern )
{
    // Compile the wildcard string to a regular expression
    wxString regex;
    regex.Alloc( 2 * aPattern.Length() );   // no need to keep resizing, we know the size roughly

    const wxString to_replace = wxT( ".*+?^${}()|[]/\\" );

    for( wxString::const_iterator it = aPattern.begin(); it < aPattern.end(); ++it )
    {
        wxUniChar c = *it;
        if( c == '?' )
        {
            regex += wxT( "." );
        }
        else if( c == '*' )
        {
            regex += wxT( ".*" );
        }
        else if( to_replace.Find( c ) != wxNOT_FOUND )
        {
            regex += "\\";
            regex += c;
        }
        else
        {
            regex += c;
        }
    }

    return EDA_PATTERN_MATCH_REGEX::SetPattern( regex );
}


int EDA_PATTERN_MATCH_WILDCARD::Find( const wxString& aCandidate ) const
{
    return EDA_PATTERN_MATCH_REGEX::Find( aCandidate );
}
