/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015-2017 Chris Pavlina <pavlina.chris@gmail.com>
 * Copyright (C) 2015-2019 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <wx/tokenzr.h>
#include <climits>

bool EDA_PATTERN_MATCH_SUBSTR::SetPattern( const wxString& aPattern )
{
    m_pattern = aPattern;
    return true;
}


wxString const& EDA_PATTERN_MATCH_SUBSTR::GetPattern() const
{
    return m_pattern;
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


wxString const& EDA_PATTERN_MATCH_REGEX::GetPattern() const
{
    return m_pattern;
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
    m_wildcard_pattern = aPattern;

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


wxString const& EDA_PATTERN_MATCH_WILDCARD::GetPattern() const
{
    return m_wildcard_pattern;
}


int EDA_PATTERN_MATCH_WILDCARD::Find( const wxString& aCandidate ) const
{
    return EDA_PATTERN_MATCH_REGEX::Find( aCandidate );
}


bool EDA_PATTERN_MATCH_WILDCARD_EXPLICIT::SetPattern( const wxString& aPattern )
{
    m_wildcard_pattern = aPattern;

    // Compile the wildcard string to a regular expression
    wxString regex;
    regex.Alloc( 2 * aPattern.Length() );   // no need to keep resizing, we know the size roughly

    const wxString to_replace = wxT( ".*+?^${}()|[]/\\" );

    regex +=  wxT( "^" );
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
    regex += wxT( "$" );

    return EDA_PATTERN_MATCH_REGEX::SetPattern( regex );
}


bool EDA_PATTERN_MATCH_RELATIONAL::SetPattern( const wxString& aPattern )
{
    bool matches = m_regex_search.Matches( aPattern );

    if( !matches || m_regex_search.GetMatchCount() < 5 )
        return false;

    m_pattern = aPattern;
    wxString key = m_regex_search.GetMatch( aPattern, 1 );
    wxString rel = m_regex_search.GetMatch( aPattern, 2 );
    wxString val = m_regex_search.GetMatch( aPattern, 3 );
    wxString unit = m_regex_search.GetMatch( aPattern, 4 );

    m_key = key.Lower();

    if( rel == "<" )
        m_relation = LT;
    else if( rel == "<=" )
        m_relation = LE;
    else if( rel == "=" )
        m_relation = EQ;
    else if( rel == ">=" )
        m_relation = GE;
    else if( rel == ">" )
        m_relation = GT;
    else
        return false;

    if( val == "" )
    {
        // Matching on empty values keeps the match list from going empty when
        // the user types the relational operator character, which helps prevent
        // confusion.
        m_relation = NONE;
    }
    else if( !val.ToCDouble( &m_value ) )
        return false;

    auto unit_it = m_units.find( unit.Lower() );

    if( unit_it != m_units.end() )
        m_value *= unit_it->second;
    else
        return false;

    m_pattern = aPattern;

    return true;
}


wxString const& EDA_PATTERN_MATCH_RELATIONAL::GetPattern() const
{
    return m_pattern;
}


int EDA_PATTERN_MATCH_RELATIONAL::Find( const wxString& aCandidate ) const
{
    wxStringTokenizer tokenizer( aCandidate );
    size_t lastpos = 0;

    while( tokenizer.HasMoreTokens() )
    {
        const wxString token = tokenizer.GetNextToken();
        int found_delta = FindOne( token );

        if( found_delta != EDA_PATTERN_NOT_FOUND )
        {
            size_t found = (size_t) found_delta + lastpos;
            return ( found > INT_MAX ) ? INT_MAX : (int) found;
        }

        lastpos = tokenizer.GetPosition();
    }

    return EDA_PATTERN_NOT_FOUND;
}


int EDA_PATTERN_MATCH_RELATIONAL::FindOne( const wxString& aCandidate ) const
{
    bool matches = m_regex_description.Matches( aCandidate );

    if( !matches )
        return EDA_PATTERN_NOT_FOUND;

    size_t start, len;
    m_regex_description.GetMatch( &start, &len, 0 );
    wxString key = m_regex_description.GetMatch( aCandidate, 1 );
    wxString val = m_regex_description.GetMatch( aCandidate, 2 );
    wxString unit = m_regex_description.GetMatch( aCandidate, 3 );

    int istart = ( start > INT_MAX ) ? INT_MAX : start;

    if( key.Lower() != m_key )
        return EDA_PATTERN_NOT_FOUND;

    double val_parsed;

    if( !val.ToCDouble( &val_parsed ) )
        return EDA_PATTERN_NOT_FOUND;

    auto unit_it = m_units.find( unit.Lower() );

    if( unit_it != m_units.end() )
        val_parsed *= unit_it->second;

    switch( m_relation )
    {
    case LT: return val_parsed <  m_value    ? istart : EDA_PATTERN_NOT_FOUND;
    case LE: return val_parsed <= m_value    ? istart : EDA_PATTERN_NOT_FOUND;
    case EQ: return val_parsed == m_value    ? istart : EDA_PATTERN_NOT_FOUND;
    case GE: return val_parsed >= m_value    ? istart : EDA_PATTERN_NOT_FOUND;
    case GT: return val_parsed >  m_value    ? istart : EDA_PATTERN_NOT_FOUND;
    case NONE: return istart;
    default: return EDA_PATTERN_NOT_FOUND;
    }
}


wxRegEx EDA_PATTERN_MATCH_RELATIONAL::m_regex_description(
        R"((\w+)[=:]([-+]?[\d.]+)(\w*))", wxRE_ADVANCED );
wxRegEx EDA_PATTERN_MATCH_RELATIONAL::m_regex_search(
        R"(^(\w+)(<|<=|=|>=|>)([-+]?[\d.]*)(\w*)$)", wxRE_ADVANCED );
const std::map<wxString, double> EDA_PATTERN_MATCH_RELATIONAL::m_units = {
    { "p",  1e-12 },
    { "n",  1e-9 },
    { "u",  1e-6 },
    { "m",  1e-3 },
    { "",   1. },
    { "k",  1e3 },
    { "meg",1e6 },
    { "g",  1e9 },
    { "t",  1e12 },
    { "ki", 1024. },
    { "mi", 1048576. },
    { "gi", 1073741824. },
    { "ti", 1099511627776. } };


EDA_COMBINED_MATCHER::EDA_COMBINED_MATCHER( const wxString& aPattern )
    : m_pattern( aPattern )
{
    // Whatever syntax users prefer, it shall be matched.
    AddMatcher( aPattern, std::make_unique<EDA_PATTERN_MATCH_REGEX>() );
    AddMatcher( aPattern, std::make_unique<EDA_PATTERN_MATCH_WILDCARD>() );
    AddMatcher( aPattern, std::make_unique<EDA_PATTERN_MATCH_RELATIONAL>() );
    // If any of the above matchers couldn't be created because the pattern
    // syntax does not match, the substring will try its best.
    AddMatcher( aPattern, std::make_unique<EDA_PATTERN_MATCH_SUBSTR>() );
}


bool EDA_COMBINED_MATCHER::Find( const wxString& aTerm, int& aMatchersTriggered, int& aPosition )
{
    aPosition = EDA_PATTERN_NOT_FOUND;
    aMatchersTriggered = 0;

    for( auto const& matcher : m_matchers )
    {
        int local_find = matcher->Find( aTerm );

        if ( local_find != EDA_PATTERN_NOT_FOUND )
        {
            aMatchersTriggered += 1;

            if ( local_find < aPosition || aPosition == EDA_PATTERN_NOT_FOUND )
            {
                aPosition = local_find;
            }
        }
    }

    return aPosition != EDA_PATTERN_NOT_FOUND;
}


wxString const& EDA_COMBINED_MATCHER::GetPattern() const
{
    return m_pattern;
}


void EDA_COMBINED_MATCHER::AddMatcher(
        const wxString &aPattern,
        std::unique_ptr<EDA_PATTERN_MATCH> aMatcher )
{
    if ( aMatcher->SetPattern( aPattern ) )
    {
        m_matchers.push_back( std::move( aMatcher ) );
    }
}
