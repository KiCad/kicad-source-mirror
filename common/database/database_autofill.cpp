/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <database/database_autofill.h>

#include <algorithm>
#include <tuple>

#include <wx/wxcrt.h>


namespace DB_AUTOFILL
{

static constexpr size_t MAX_REMEMBERED = 8;

enum MATCH_SCORE
{
    NO_MATCH = 0,
    SUBSTRING = 10,   ///< The term is buried inside a word: "myfootprintref" for "footprint"
    AFFIX = 30,       ///< The name starts or ends with the term mid-word: "Identical", "partid"
    INNER_WORD = 60,  ///< The term is a whole word inside the name: "Main ID Code"
    EDGE_WORD = 80,   ///< The term is a whole word at either end: "PartID", "symbol_ref"
    EXACT = 100,      ///< The name is the term: "ID", "Symbols"
    REMEMBERED = 1000 ///< A column the user chose before; earlier choices score lower
};


/// Splits an identifier into lower case words: "PCBFootprint_ref2" gives pcb, footprint, ref, 2
static std::vector<wxString> SplitWords( const wxString& aName )
{
    std::vector<wxString> words;
    wxString              word;

    auto flush =
            [&]()
            {
                if( !word.IsEmpty() )
                    words.push_back( word.Lower() );

                word.clear();
            };

    for( size_t ii = 0; ii < aName.length(); ++ii )
    {
        wxUniChar ch = aName[ii];

        if( !wxIsalnum( ch ) )
        {
            flush();
            continue;
        }

        if( !word.IsEmpty() )
        {
            wxUniChar prev = word.Last();
            bool      nextIsLower = ii + 1 < aName.length() && wxIslower( aName[ii + 1] );

            // The last capital of an acronym starts the next word: "PCBFootprint" is PCB, Footprint
            if( ( wxIslower( prev ) && wxIsupper( ch ) ) || ( !wxIsdigit( prev ) != !wxIsdigit( ch ) )
                || ( wxIsupper( prev ) && wxIsupper( ch ) && nextIsLower ) )
            {
                flush();
            }
        }

        word += ch;
    }

    flush();
    return words;
}


static bool sameWord( const wxString& aA, const wxString& aB )
{
    return aA == aB || aA + wxS( "s" ) == aB || aB + wxS( "s" ) == aA || aA + wxS( "es" ) == aB
           || aB + wxS( "es" ) == aA;
}


static bool wordsMatchAt( const std::vector<wxString>& aWords, const std::vector<wxString>& aTerm, size_t aPos )
{
    for( size_t ii = 0; ii < aTerm.size(); ++ii )
    {
        if( !sameWord( aWords[aPos + ii], aTerm[ii] ) )
            return false;
    }

    return true;
}


static wxString joined( const std::vector<wxString>& aWords )
{
    wxString out;

    for( const wxString& word : aWords )
        out += word;

    return out;
}


/// @return how well aName matches aTerm, from MATCH_SCORE; plurals match their singular
static int MatchScore( const wxString& aName, const wxString& aTerm )
{
    std::vector<wxString> words = SplitWords( aName );
    std::vector<wxString> term = SplitWords( aTerm );

    if( words.empty() || term.empty() )
        return NO_MATCH;

    if( term.size() <= words.size() )
    {
        size_t last = words.size() - term.size();

        if( last == 0 && wordsMatchAt( words, term, 0 ) )
            return EXACT;

        if( wordsMatchAt( words, term, 0 ) || wordsMatchAt( words, term, last ) )
            return EDGE_WORD;

        for( size_t pos = 1; pos < last; ++pos )
        {
            if( wordsMatchAt( words, term, pos ) )
                return INNER_WORD;
        }
    }

    wxString name = joined( words );
    wxString needle = joined( term );

    if( name.StartsWith( needle ) || name.EndsWith( needle ) )
        return AFFIX;

    // Short terms such as "id" or "fp" turn up inside too many unrelated words
    if( needle.length() >= 4 && name.Contains( needle ) )
        return SUBSTRING;

    return NO_MATCH;
}


static int roleScore( const ROLE& aRole, const std::string& aColumn )
{
    if( aRole.excluded.contains( aColumn ) )
        return NO_MATCH;

    wxString column = wxString::FromUTF8( aColumn );

    for( size_t ii = 0; ii < aRole.remembered.size(); ++ii )
    {
        if( column.IsSameAs( wxString::FromUTF8( aRole.remembered[ii] ), false ) )
            return REMEMBERED - static_cast<int>( ii );
    }

    int best = NO_MATCH;

    for( const wxString& term : aRole.terms )
        best = std::max( best, MatchScore( column, term ) );

    return best;
}


std::vector<std::string> AssignColumns( const std::vector<std::string>& aColumns, const std::vector<ROLE>& aRoles )
{
    // Score, role, column; ties go to the earlier role and then the earlier column
    std::vector<std::tuple<int, size_t, size_t>> candidates;

    for( size_t role = 0; role < aRoles.size(); ++role )
    {
        for( size_t col = 0; col < aColumns.size(); ++col )
        {
            if( int score = roleScore( aRoles[role], aColumns[col] ); score > NO_MATCH )
                candidates.emplace_back( -score, role, col );
        }
    }

    std::sort( candidates.begin(), candidates.end() );

    std::vector<std::string> chosen( aRoles.size() );
    std::vector<bool>        roleDone( aRoles.size(), false );
    std::vector<bool>        columnUsed( aColumns.size(), false );

    for( const auto& [negScore, role, col] : candidates )
    {
        if( roleDone[role] || columnUsed[col] )
            continue;

        chosen[role] = aColumns[col];
        roleDone[role] = true;
        columnUsed[col] = true;
    }

    return chosen;
}


std::string MatchTable( const std::vector<std::string>& aTables, const std::string& aLibraryName )
{
    wxString    library = wxString::FromUTF8( aLibraryName );
    std::string best;
    int         bestScore = INNER_WORD - 1;

    for( const std::string& table : aTables )
    {
        wxString name = wxString::FromUTF8( table );

        // Either may carry extra words: library "Resistors" for table "tbl_resistor", or the reverse
        int score = std::max( MatchScore( name, library ), MatchScore( library, name ) );

        if( score > bestScore )
        {
            best = table;
            bestScore = score;
        }
    }

    return best;
}


std::map<std::string, size_t> RepeatCounts( const std::vector<std::map<std::string, std::any>>& aRows )
{
    std::map<std::string, std::set<std::string>> seen;
    std::map<std::string, size_t>                repeats;

    for( const std::map<std::string, std::any>& row : aRows )
    {
        for( const auto& [column, value] : row )
        {
            // The library files rows by key text, so empty and NULL keys collide like any other repeat
            const std::string* text = std::any_cast<std::string>( &value );

            repeats.try_emplace( column, 0 );

            if( !seen[column].insert( text ? *text : std::string() ).second )
                repeats[column]++;
        }
    }

    return repeats;
}


void RememberChoice( std::vector<std::string>& aHistory, const std::string& aColumn )
{
    if( aColumn.empty() )
        return;

    wxString column = wxString::FromUTF8( aColumn );

    std::erase_if( aHistory,
                   [&]( const std::string& aEntry )
                   {
                       return column.IsSameAs( wxString::FromUTF8( aEntry ), false );
                   } );

    aHistory.insert( aHistory.begin(), aColumn );

    if( aHistory.size() > MAX_REMEMBERED )
        aHistory.resize( MAX_REMEMBERED );
}

} // namespace DB_AUTOFILL
