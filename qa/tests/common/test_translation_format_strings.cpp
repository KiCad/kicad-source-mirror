/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 3 of the License, or (at your
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

/**
 * @file test_translation_format_strings.cpp
 *
 * Guards against issue 23973.  KiCad feeds translated strings into wxString::Format(), which does
 * not check that a translation's printf specifiers match the supplied argument count.  A translation
 * carrying more consuming specifiers than its msgid makes Format() read past the argument list and
 * crash.  This test fails if any non-fuzzy translation adds specifiers beyond those in its msgid.
 */

#include <boost/test/unit_test.hpp>

#include <wx/dir.h>
#include <wx/filename.h>
#include <wx/textfile.h>

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <map>
#include <set>
#include <string>
#include <vector>


namespace
{
struct PO_ENTRY
{
    std::string              m_msgId;
    std::string              m_msgIdPlural;
    std::vector<std::string> m_msgStrs;
    int                      m_line = 0;
    bool                     m_fuzzy = false;
};


// Concatenate the double-quoted runs of a .po line, honoring backslash escapes.
std::string unquote( const std::string& aLine )
{
    std::string out;
    bool        inQuote = false;

    for( size_t i = 0; i < aLine.size(); ++i )
    {
        char ch = aLine[i];

        if( !inQuote )
        {
            if( ch == '"' )
                inQuote = true;

            continue;
        }

        if( ch == '\\' && i + 1 < aLine.size() )
        {
            out += aLine[++i];
            continue;
        }

        if( ch == '"' )
            inQuote = false;
        else
            out += ch;
    }

    return out;
}


std::vector<PO_ENTRY> parsePo( const wxString& aPath )
{
    std::vector<PO_ENTRY> entries;
    wxTextFile            file;

    if( !file.Open( aPath ) )
        return entries;

    PO_ENTRY    cur;
    bool        haveCur = false;
    bool        pendingFuzzy = false;
    enum { NONE, ID, IDP, STR } state = NONE;
    size_t      strIdx = 0;

    auto starts = []( const std::string& aStr, const char* aPrefix )
    {
        return aStr.rfind( aPrefix, 0 ) == 0;
    };

    int lineNo = 0;

    for( wxString wxLine = file.GetFirstLine(); !file.Eof(); wxLine = file.GetNextLine() )
    {
        ++lineNo;
        std::string line = wxLine.Trim( false ).Trim( true ).utf8_string();

        if( starts( line, "#," ) )
        {
            // A fuzzy flag may share its "#," line with others, so accumulate rather than overwrite.
            if( line.find( "fuzzy" ) != std::string::npos )
                pendingFuzzy = true;
        }
        else if( starts( line, "msgid_plural" ) )
        {
            if( haveCur )
                cur.m_msgIdPlural = unquote( line );

            state = IDP;
        }
        else if( starts( line, "msgid " ) )
        {
            if( haveCur )
                entries.push_back( cur );

            cur = PO_ENTRY();
            cur.m_msgId = unquote( line );
            cur.m_line = lineNo;
            cur.m_fuzzy = pendingFuzzy;
            pendingFuzzy = false;
            haveCur = true;
            state = ID;
        }
        else if( starts( line, "msgstr[" ) )
        {
            if( haveCur )
            {
                int idx = std::atoi( line.c_str() + sizeof( "msgstr[" ) - 1 );

                // No language has dozens of plural forms; reject wild indices to bound the resize.
                if( idx >= 0 && idx < 64 )
                {
                    strIdx = static_cast<size_t>( idx );

                    if( cur.m_msgStrs.size() <= strIdx )
                        cur.m_msgStrs.resize( strIdx + 1 );

                    cur.m_msgStrs[strIdx] = unquote( line );
                }
            }

            state = STR;
        }
        else if( starts( line, "msgstr " ) )
        {
            if( haveCur )
            {
                cur.m_msgStrs.assign( 1, unquote( line ) );
                strIdx = 0;
            }

            state = STR;
        }
        else if( !line.empty() && line[0] == '"' )
        {
            if( haveCur && state == ID )
                cur.m_msgId += unquote( line );
            else if( haveCur && state == IDP )
                cur.m_msgIdPlural += unquote( line );
            else if( haveCur && state == STR && strIdx < cur.m_msgStrs.size() )
                cur.m_msgStrs[strIdx] += unquote( line );
        }
        else if( line.empty() )
        {
            // Blank lines separate entries; the next msgid flushes.
        }
        else
        {
            state = NONE;
        }
    }

    if( haveCur )
        entries.push_back( cur );

    return entries;
}


/**
 * Count argument-consuming printf specifiers, keyed by conversion letter.  "%%" and a '%' before a
 * space (literal labels like "100% skin depth") consume nothing.  A reused positional ("%1$s ...
 * %1$s") references one argument, so distinct positions are counted rather than raw occurrences.
 */
std::map<char, int> consumingSpecs( const std::string& aStr )
{
    static const std::string flags = "-+#0";
    static const std::string convs = "diouxXeEfFgGaAcsSCp";

    std::map<char, int>           counts;   // non-positional and '*' consumptions
    std::map<char, std::set<int>> posArgs;  // distinct positional indices per conversion

    for( size_t i = 0; i < aStr.size(); ++i )
    {
        if( aStr[i] != '%' )
            continue;

        size_t j = i + 1;

        if( j < aStr.size() && aStr[j] == '%' )
        {
            i = j;
            continue;
        }

        // Optional positional argument "<digits>$".
        int    position = 0;
        size_t k = j;

        while( k < aStr.size() && std::isdigit( static_cast<unsigned char>( aStr[k] ) ) )
            ++k;

        if( k < aStr.size() && aStr[k] == '$' && k > j )
        {
            position = std::atoi( aStr.c_str() + j );
            j = k + 1;
        }

        while( j < aStr.size() && flags.find( aStr[j] ) != std::string::npos )
            ++j;

        // A '*' width consumes an int argument; digits are a literal width.
        if( j < aStr.size() && aStr[j] == '*' )
        {
            counts['*']++;
            ++j;
        }
        else
        {
            while( j < aStr.size() && std::isdigit( static_cast<unsigned char>( aStr[j] ) ) )
                ++j;
        }

        if( j < aStr.size() && aStr[j] == '.' )
        {
            ++j;

            // A '*' precision likewise consumes an int argument.
            if( j < aStr.size() && aStr[j] == '*' )
            {
                counts['*']++;
                ++j;
            }
            else
            {
                while( j < aStr.size() && std::isdigit( static_cast<unsigned char>( aStr[j] ) ) )
                    ++j;
            }
        }

        static const std::string lengths = "lhzjtL";

        while( j < aStr.size() && lengths.find( aStr[j] ) != std::string::npos )
            ++j;

        if( j < aStr.size() && convs.find( aStr[j] ) != std::string::npos )
        {
            if( position > 0 )
                posArgs[aStr[j]].insert( position );
            else
                counts[aStr[j]]++;

            i = j;
        }
    }

    for( const auto& [conv, positions] : posArgs )
        counts[conv] += static_cast<int>( positions.size() );

    return counts;
}


wxFileName poDir()
{
    wxFileName dir = wxFileName::DirName( wxString::FromUTF8( QA_SRC_ROOT ) );
    dir.AppendDir( wxS( "translation" ) );
    dir.AppendDir( wxS( "pofiles" ) );
    return dir;
}
} // namespace


BOOST_AUTO_TEST_SUITE( TranslationFormatStrings )


/**
 * A non-fuzzy translation of a format-string msgid must not add argument-consuming specifiers, else
 * wxString::Format() reads past its arguments and crashes (issue 23973).
 */
BOOST_AUTO_TEST_CASE( NoExtraFormatSpecifiers )
{
    wxString poPath = poDir().GetFullPath();

    BOOST_REQUIRE_MESSAGE( wxDir::Exists( poPath ),
                           "Translation pofiles directory not found: " << poPath.utf8_string() );

    wxArrayString files;
    wxDir::GetAllFiles( poPath, &files, wxS( "*.po" ), wxDIR_FILES );

    BOOST_REQUIRE_MESSAGE( !files.empty(), "No .po files found in " << poPath.utf8_string() );

    int violations = 0;

    for( const wxString& po : files )
    {
        wxFileName poFile( po );

        for( const PO_ENTRY& entry : parsePo( po ) )
        {
            if( entry.m_fuzzy )
                continue;

            // A plural form may reference either source string, so budget the per-conversion max.
            std::map<char, int> src = consumingSpecs( entry.m_msgId );

            for( const auto& [conv, count] : consumingSpecs( entry.m_msgIdPlural ) )
                src[conv] = std::max( src[conv], count );

            // Only msgids that are already format strings are in scope; literal-percent labels are
            // skipped to avoid false positives.
            if( src.empty() )
                continue;

            for( const std::string& msgStr : entry.m_msgStrs )
            {
                if( msgStr.empty() )
                    continue;

                std::map<char, int> dst = consumingSpecs( msgStr );

                for( const auto& [conv, count] : dst )
                {
                    if( count > src[conv] )
                    {
                        ++violations;
                        BOOST_ERROR( poFile.GetFullName().utf8_string()
                                     << ":" << entry.m_line << " translation adds extra '%" << conv
                                     << "' specifier (msgid=\"" << entry.m_msgId << "\" msgstr=\""
                                     << msgStr << "\")" );
                        break;
                    }
                }
            }
        }
    }

    BOOST_CHECK_EQUAL( violations, 0 );
}


BOOST_AUTO_TEST_SUITE_END()
