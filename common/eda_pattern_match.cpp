/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015-2017 Chris Pavlina <pavlina.chris@gmail.com>
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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
#include <limits>
#include <wx/log.h>
#include <wx/tokenzr.h>
#include <algorithm>

// Helper to make the code cleaner when we want this operation
#define CLAMPED_VAL_INT_MAX( x )                                                      \
    std::min( x, static_cast<size_t>( std::numeric_limits<int>::max() ) )


bool EDA_PATTERN_MATCH_SUBSTR::SetPattern( const wxString& aPattern )
{
    m_pattern = aPattern;
    return true;
}


wxString const& EDA_PATTERN_MATCH_SUBSTR::GetPattern() const
{
    return m_pattern;
}


EDA_PATTERN_MATCH::FIND_RESULT EDA_PATTERN_MATCH_SUBSTR::Find( const wxString& aCandidate ) const
{
    int loc = aCandidate.Find( m_pattern );

    if( loc == wxNOT_FOUND )
        return {};
    else
        return { loc, static_cast<int>( m_pattern.size() ) };
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
    if( aPattern.StartsWith( "^" ) && aPattern.EndsWith( "$" ) )
    {
        m_pattern = aPattern;
    }
    else if( aPattern.StartsWith( "/" ) )
    {
        // Requiring a '/' on the end means they get no feedback while they type
        m_pattern = aPattern.Mid( 1 );

        if( m_pattern.EndsWith( "/" ) )
            m_pattern = m_pattern.Left( m_pattern.length() - 1 );
    }
    else
    {
        // For now regular expressions must be explicit
        return false;
    }

    // Evil and undocumented: wxRegEx::Compile calls wxLogError on error, even
    // though it promises to just return false. Silence the error.
    WX_LOGLEVEL_CONTEXT ctx( wxLOG_FatalError );

    return m_regex.Compile( m_pattern, wxRE_ADVANCED );
}


bool EDA_PATTERN_MATCH_REGEX_ANCHORED::SetPattern( const wxString& aPattern )
{
    wxString pattern( aPattern );

    if( !pattern.StartsWith( wxT( "^" ) ) )
        pattern = wxT( "^" ) + pattern;

    if( !pattern.EndsWith( wxT( "$" ) ) )
        pattern +=  wxT( "$" );

    return EDA_PATTERN_MATCH_REGEX::SetPattern( pattern );
}


wxString const& EDA_PATTERN_MATCH_REGEX::GetPattern() const
{
    return m_pattern;
}


EDA_PATTERN_MATCH::FIND_RESULT EDA_PATTERN_MATCH_REGEX::Find( const wxString& aCandidate ) const
{
    if( m_regex.IsValid() )
    {
        if( m_regex.Matches( aCandidate ) )
        {
            size_t start, len;
            m_regex.GetMatch( &start, &len, 0 );

            return { static_cast<int>( CLAMPED_VAL_INT_MAX( start ) ),
                     static_cast<int>( CLAMPED_VAL_INT_MAX( len ) ) };
        }
        else
        {
            return {};
        }
    }
    else
    {
        int loc = aCandidate.Find( m_pattern );

        if( loc == wxNOT_FOUND )
            return {};
        else
            return { loc, static_cast<int>( m_pattern.size() ) };
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

    return EDA_PATTERN_MATCH_REGEX::SetPattern( wxS( "/" ) + regex + wxS( "/" ) );
}


wxString const& EDA_PATTERN_MATCH_WILDCARD::GetPattern() const
{
    return m_wildcard_pattern;
}


EDA_PATTERN_MATCH::FIND_RESULT EDA_PATTERN_MATCH_WILDCARD::Find( const wxString& aCandidate ) const
{
    return EDA_PATTERN_MATCH_REGEX::Find( aCandidate );
}


bool EDA_PATTERN_MATCH_WILDCARD_ANCHORED::SetPattern( const wxString& aPattern )
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
            regex += wxS( "\\" );
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
    wxRegEx regex_search( R"(^(\w+)(<|<=|=|>=|>)([-+]?[\d.]*)(\w*)$)", wxRE_ADVANCED );

    bool matches = regex_search.IsValid() && regex_search.Matches( aPattern );

    if( !matches || regex_search.GetMatchCount() < 5 )
        return false;

    m_pattern = aPattern;
    wxString key = regex_search.GetMatch( aPattern, 1 );
    wxString rel = regex_search.GetMatch( aPattern, 2 );
    wxString val = regex_search.GetMatch( aPattern, 3 );
    wxString unit = regex_search.GetMatch( aPattern, 4 );

    m_key = key.Lower();

    if( rel == wxS( "<" ) )
        m_relation = LT;
    else if( rel == wxS( "<=" ) )
        m_relation = LE;
    else if( rel == wxS( "=" ) )
        m_relation = EQ;
    else if( rel == wxS( ">=" ) )
        m_relation = GE;
    else if( rel == wxS( ">" ) )
        m_relation = GT;
    else
        return false;

    if( val == "" )
    {
        // Matching on empty values keeps the match list from going empty when the user
        // types the relational operator character, which helps prevent confusion.
        m_relation = ANY;
    }
    else if( !val.ToCDouble( &m_value ) )
    {
        return false;
    }

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


EDA_PATTERN_MATCH::FIND_RESULT EDA_PATTERN_MATCH_RELATIONAL::Find( const wxString& aCandidate ) const
{
    wxStringTokenizer tokenizer( aCandidate, " \t\r\n", wxTOKEN_STRTOK );
    size_t lastpos = 0;

    while( tokenizer.HasMoreTokens() )
    {
        const wxString token = tokenizer.GetNextToken();
        int found_delta = FindOne( token );

        if( found_delta != EDA_PATTERN_NOT_FOUND )
        {
            size_t found = (size_t) found_delta + lastpos;
            return { static_cast<int>( CLAMPED_VAL_INT_MAX( found ) ), 0 };
        }

        lastpos = tokenizer.GetPosition();
    }

    return {};
}


int EDA_PATTERN_MATCH_RELATIONAL::FindOne( const wxString& aCandidate ) const
{
    wxRegEx regex_description( R"((\w+)[=:]([-+]?[\d.]+)(\w*))", wxRE_ADVANCED );

    bool matches = regex_description.IsValid() && regex_description.Matches( aCandidate );

    if( !matches )
        return EDA_PATTERN_NOT_FOUND;

    size_t start, len;
    regex_description.GetMatch( &start, &len, 0 );
    wxString key = regex_description.GetMatch( aCandidate, 1 );
    wxString val = regex_description.GetMatch( aCandidate, 2 );
    wxString unit = regex_description.GetMatch( aCandidate, 3 );

    int istart = static_cast<int>( CLAMPED_VAL_INT_MAX( start ) );

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
    case LT:  return val_parsed <  m_value ? istart : EDA_PATTERN_NOT_FOUND;
    case LE:  return val_parsed <= m_value ? istart : EDA_PATTERN_NOT_FOUND;
    case EQ:  return val_parsed == m_value ? istart : EDA_PATTERN_NOT_FOUND;
    case GE:  return val_parsed >= m_value ? istart : EDA_PATTERN_NOT_FOUND;
    case GT:  return val_parsed >  m_value ? istart : EDA_PATTERN_NOT_FOUND;
    case ANY: return istart;
    default:  return EDA_PATTERN_NOT_FOUND;
    }
}


const std::map<wxString, double> EDA_PATTERN_MATCH_RELATIONAL::m_units = {
    { wxS( "p" ),  1e-12 },
    { wxS( "n" ),  1e-9 },
    { wxS( "u" ),  1e-6 },
    { wxS( "m" ),  1e-3 },
    { wxS( "" ),   1. },
    { wxS( "k" ),  1e3 },
    { wxS( "meg" ), 1e6 },
    { wxS( "g" ),  1e9 },
    { wxS( "t" ),  1e12 },
    { wxS( "ki" ), 1024. },
    { wxS( "mi" ), 1048576. },
    { wxS( "gi" ), 1073741824. },
    { wxS( "ti" ), 1099511627776. } };


EDA_COMBINED_MATCHER::EDA_COMBINED_MATCHER( const wxString& aPattern,
                                            COMBINED_MATCHER_CONTEXT aContext ) :
        m_pattern( aPattern )
{
    switch( aContext )
    {
    case CTX_LIBITEM:
        // Whatever syntax users prefer, it shall be matched.
        AddMatcher( aPattern, std::make_unique<EDA_PATTERN_MATCH_REGEX>() );
        AddMatcher( aPattern, std::make_unique<EDA_PATTERN_MATCH_WILDCARD>() );
        AddMatcher( aPattern, std::make_unique<EDA_PATTERN_MATCH_RELATIONAL>() );

        // If any of the above matchers couldn't be created because the pattern
        // syntax does not match, the substring will try its best.
        AddMatcher( aPattern, std::make_unique<EDA_PATTERN_MATCH_SUBSTR>() );
        break;

    case CTX_NET:
        AddMatcher( aPattern, std::make_unique<EDA_PATTERN_MATCH_REGEX>() );
        AddMatcher( aPattern, std::make_unique<EDA_PATTERN_MATCH_WILDCARD>() );
        AddMatcher( aPattern, std::make_unique<EDA_PATTERN_MATCH_SUBSTR>() );
        break;

    case CTX_NETCLASS:
        AddMatcher( aPattern, std::make_unique<EDA_PATTERN_MATCH_REGEX_ANCHORED>() );
        AddMatcher( aPattern, std::make_unique<EDA_PATTERN_MATCH_WILDCARD_ANCHORED>() );
        break;

    case CTX_SIGNAL:
        AddMatcher( aPattern, std::make_unique<EDA_PATTERN_MATCH_REGEX>() );
        AddMatcher( aPattern, std::make_unique<EDA_PATTERN_MATCH_WILDCARD>() );
        AddMatcher( aPattern, std::make_unique<EDA_PATTERN_MATCH_SUBSTR>() );
        break;

    case CTX_SEARCH:
        AddMatcher( aPattern, std::make_unique<EDA_PATTERN_MATCH_REGEX>() );
        AddMatcher( aPattern, std::make_unique<EDA_PATTERN_MATCH_WILDCARD>() );
        AddMatcher( aPattern, std::make_unique<EDA_PATTERN_MATCH_SUBSTR>() );
        break;
    }
}


bool EDA_COMBINED_MATCHER::Find( const wxString& aTerm, int& aMatchersTriggered, int& aPosition )
{
    aPosition = EDA_PATTERN_NOT_FOUND;
    aMatchersTriggered = 0;

    for( const std::unique_ptr<EDA_PATTERN_MATCH>& matcher : m_matchers )
    {
        EDA_PATTERN_MATCH::FIND_RESULT local_find = matcher->Find( aTerm );

        if( local_find )
        {
            aMatchersTriggered += 1;

            if( local_find.start < aPosition || aPosition == EDA_PATTERN_NOT_FOUND )
                aPosition = local_find.start;
        }
    }

    return aPosition != EDA_PATTERN_NOT_FOUND;
}


bool EDA_COMBINED_MATCHER::Find( const wxString& aTerm )
{
    for( const std::unique_ptr<EDA_PATTERN_MATCH>& matcher : m_matchers )
    {
        if( matcher->Find( aTerm ).start >= 0 )
            return true;
    }

    return false;
}


bool EDA_COMBINED_MATCHER::StartsWith( const wxString& aTerm )
{
    for( const std::unique_ptr<EDA_PATTERN_MATCH>& matcher : m_matchers )
    {
        if( matcher->Find( aTerm ).start == 0 )
            return true;
    }

    return false;
}


int EDA_COMBINED_MATCHER::ScoreTerms( std::vector<SEARCH_TERM>& aWeightedTerms )
{
    int score = 0;

    for( SEARCH_TERM& term : aWeightedTerms )
    {
        if( !term.Normalized )
        {
            term.Text = term.Text.MakeLower().Trim( false ).Trim( true );

            // Don't cause KiCad to hang if someone accidentally pastes the PCB or schematic
            // into the search box.
            if( term.Text.Length() > 1000 )
                term.Text = term.Text.Left( 1000 );

            term.Normalized = true;
        }

        int found_pos = EDA_PATTERN_NOT_FOUND;
        int matchers_fired = 0;

        if( GetPattern() == term.Text )
        {
            score += 8 * term.Score;
        }
        else if( Find( term.Text, matchers_fired, found_pos ) )
        {
            if( found_pos == 0 )
                score += 2 * term.Score;
            else
                score += term.Score;
        }
    }

    return score;
}


wxString const& EDA_COMBINED_MATCHER::GetPattern() const
{
    return m_pattern;
}


void EDA_COMBINED_MATCHER::AddMatcher( const wxString &aPattern,
                                       std::unique_ptr<EDA_PATTERN_MATCH> aMatcher )
{
    if ( aMatcher->SetPattern( aPattern ) )
        m_matchers.push_back( std::move( aMatcher ) );
}
