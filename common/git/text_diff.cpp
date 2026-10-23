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
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <git/text_diff.h>

#include <json_common.h>
#include <wildcards_and_files_ext.h>

#include <algorithm>
#include <cctype>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <optional>
#include <set>

#include <wx/filename.h>

namespace KIGIT
{

namespace
{

namespace fs = std::filesystem;


fs::path fsPath( const wxString& aPath )
{
#ifdef __WINDOWS__
    return fs::path( aPath.ToStdWstring() );
#else
    return fs::path( aPath.utf8_string() );
#endif
}


wxString wxPath( const fs::path& aPath )
{
#ifdef __WINDOWS__
    return wxString( aPath.wstring() );
#else
    return wxString::FromUTF8( aPath.string() );
#endif
}


std::optional<fs::path> resolved( const fs::path& aPath )
{
    std::error_code ec;
    fs::path        path = fs::canonical( aPath, ec );

    if( ec )
        return std::nullopt;

    return path;
}


// Compared as files at the side's depth, so names that differ only in case stay apart where the volume does
bool within( const fs::path& aReal, const fs::path& aSide )
{
    const auto depth = std::distance( aSide.begin(), aSide.end() );

    if( std::distance( aReal.begin(), aReal.end() ) <= depth )
        return false;

    fs::path ancestor;
    auto     part = aReal.begin();

    for( auto i = depth; i > 0; --i, ++part )
        ancestor /= *part;

    std::error_code ec;
    return fs::equivalent( ancestor, aSide, ec ) && !ec;
}


enum class LOOKUP
{
    MISSING, ///< Not there, so the loader opens nothing either
    INSIDE,
    DOUBT    ///< Outside the side, or any lookup failed, which counts as too large
};


LOOKUP lookUp( const fs::path& aPath, const fs::path& aSide )
{
    std::error_code       ec;
    const fs::file_status status = fs::status( aPath, ec );

    if( status.type() == fs::file_type::not_found )
        return LOOKUP::MISSING;

    if( ec )
        return LOOKUP::DOUBT;

    std::optional<fs::path> real = resolved( aPath );

    return real && within( *real, aSide ) ? LOOKUP::INSIDE : LOOKUP::DOUBT;
}


std::optional<unsigned long long> sizeOf( const fs::path& aPath )
{
    std::error_code          ec;
    const unsigned long long size = fs::file_size( aPath, ec );

    if( ec )
        return std::nullopt;

    return size;
}


// At most aMax + 1 bytes, so a file that grew since its size was taken still reads as over
std::string readUpTo( const fs::path& aFile, unsigned long long aMax )
{
    std::ifstream in( aFile, std::ios::binary );
    std::string   text;

    while( in && text.size() <= aMax )
    {
        const size_t had = text.size();

        text.resize( had + static_cast<size_t>( std::min<unsigned long long>( 1 << 16, aMax + 1 - had ) ) );
        in.read( text.data() + had, static_cast<std::streamsize>( text.size() - had ) );
        text.resize( had + static_cast<size_t>( in.gcount() ) );
    }

    return text;
}


// A text scan, since parsing the schematic is the UI-thread work being budgeted. A symbol field of the same name
// only makes the cap stricter. Nothing when a name does not end on its property's line, as the scan cannot read it
std::optional<std::vector<fs::path>> sheetNamesIn( const std::string& aText )
{
    std::vector<fs::path> names;
    size_t                start = 0;

    while( start < aText.size() )
    {
        size_t            stop = std::min( aText.find( '\n', start ), aText.size() );
        const std::string line = aText.substr( start, stop - start );
        std::string       lower = line;

        start = stop + 1;
        std::transform( lower.begin(), lower.end(), lower.begin(),
                        []( unsigned char c )
                        {
                            return static_cast<char>( std::tolower( c ) );
                        } );

        // A sheet may write all of its properties on one line
        for( size_t property = lower.find( "(property" ); property != std::string::npos;
             property = lower.find( "(property", property + 1 ) )
        {
            const size_t nameAt = property + std::strlen( "(property" );

            if( nameAt < lower.size() && !std::isspace( static_cast<unsigned char>( lower[nameAt] ) ) )
                continue;

            const size_t found = lower.find_first_not_of( " \t\r", nameAt );

            // A name split from its property is valid s-expression the line scan cannot read
            if( found == std::string::npos )
                return std::nullopt;

            size_t at = std::string::npos;
            bool   keyed = false;

            // Older files name the field "Sheet file"
            for( const char* key : { "\"sheetfile\"", "\"sheet file\"" } )
            {
                if( lower.compare( found, std::strlen( key ), key ) == 0 )
                {
                    keyed = true;
                    at = line.find_first_not_of( " \t\r", found + std::strlen( key ) );
                    break;
                }
            }

            if( !keyed )
                continue;

            // A value split across lines or left unquoted is valid s-expression the line scan cannot read
            if( at == std::string::npos || line[at] != '"' )
                return std::nullopt;

            std::string name;
            size_t      i = at + 1;

            for( ; i < line.size() && line[i] != '"'; ++i )
            {
                if( line[i] == '\\' && i + 1 < line.size() )
                    ++i;

                name += line[i];
            }

            if( i >= line.size() )
                return std::nullopt;

            names.push_back( fsPath( wxString::FromUTF8( name ) ) );
        }
    }

    return names;
}


// Resolved the way the loader resolves it, against the folder of the path it opened rather than of a link's target,
// and with ".." collapsed before the file is looked for
fs::path loaderPath( const fs::path& aName, const fs::path& aFolder )
{
    wxFileName file( wxPath( aName ) );

    if( !file.IsAbsolute() )
        file.MakeAbsolute( wxPath( aFolder ) );

    return fsPath( file.GetFullPath() );
}


// The loader opens every top-level sheet the project declares when the selected file is one of them. Nothing when
// the project is in doubt or over its own budget
std::optional<std::vector<fs::path>> rootSheets( const fs::path& aRoot, const fs::path& aSide )
{
    fs::path project = aRoot;

    project.replace_extension( FILEEXT::ProjectFileExtension );

    switch( lookUp( project, aSide ) )
    {
    case LOOKUP::MISSING: return std::vector<fs::path>{ aRoot };
    case LOOKUP::DOUBT: return std::nullopt;
    case LOOKUP::INSIDE: break;
    }

    const std::string bytes = readUpTo( project, PROJECT_PREVIEW_LIMIT );

    if( bytes.size() > PROJECT_PREVIEW_LIMIT )
        return std::nullopt;

    // Comments are allowed, as the settings loader allows them
    const nlohmann::json  json = nlohmann::json::parse( bytes, nullptr, false, true );
    std::vector<fs::path> roots;
    bool                  declared = false;

    if( json.is_object() && json.contains( "schematic" ) && json["schematic"].is_object()
        && json["schematic"].contains( "top_level_sheets" ) && json["schematic"]["top_level_sheets"].is_array() )
    {
        for( const nlohmann::json& sheet : json["schematic"]["top_level_sheets"] )
        {
            if( !sheet.is_object() || !sheet.contains( "filename" ) || !sheet["filename"].is_string() )
                continue;

            // Normalized even when absolute, as the loader opens a declared root
            wxFileName file( wxString::FromUTF8( sheet["filename"].get<std::string>() ) );

            file.MakeAbsolute( wxPath( aRoot.parent_path() ) );

            const fs::path root = fsPath( file.GetFullPath() );

            declared |= wxFileName( wxPath( root ) ).SameAs( wxFileName( wxPath( aRoot ) ) );
            roots.push_back( root );
        }
    }

    if( !declared )
        return std::vector<fs::path>{ aRoot };

    return roots;
}


// Stops once past aStopAbove, so a hierarchy far over the limit is never read whole
unsigned long long drawingBytes( const wxString& aFile, const wxString& aSideRoot, bool aSchematic,
                                 unsigned long long aStopAbove )
{
    const unsigned long long over = aStopAbove + 1;

    if( aFile.IsEmpty() )
        return 0;

    std::error_code ec;
    const fs::path  root = fs::absolute( fsPath( aFile ), ec );

    if( ec )
        return over;

    const std::optional<fs::path> side = resolved( aSideRoot.IsEmpty() ? root.parent_path() : fsPath( aSideRoot ) );

    if( !side || lookUp( root, *side ) != LOOKUP::INSIDE )
        return over;

    const std::optional<unsigned long long> rootSize = sizeOf( root );

    // A file known to be over needs nothing else read, not even its project
    if( !rootSize || *rootSize > aStopAbove )
        return over;

    if( !aSchematic )
        return *rootSize;

    const std::optional<std::vector<fs::path>> roots = rootSheets( root, *side );

    if( !roots )
        return over;

    std::set<fs::path>    seen;
    std::vector<fs::path> pending = *roots;
    unsigned long long    total = 0;

    while( !pending.empty() )
    {
        const fs::path file = pending.back();
        pending.pop_back();

        // Keyed by the path the loader opens, so a file reached by two paths counts twice, as it loads twice
        if( !seen.insert( file ).second )
            continue;

        const LOOKUP found = lookUp( file, *side );

        if( found == LOOKUP::MISSING )
            continue;

        const std::optional<unsigned long long> size = sizeOf( file );

        if( found == LOOKUP::DOUBT || !size || *size > aStopAbove - total )
            return over;

        const std::string text = readUpTo( file, aStopAbove - total );

        total += text.size();

        if( total > aStopAbove )
            return over;

        const std::optional<std::vector<fs::path>> names = sheetNamesIn( text );

        if( !names )
            return over;

        for( const fs::path& name : *names )
        {
            const wxString spelled = wxPath( name );

            // The loader expands these from project variables the scan cannot see
            if( spelled.Contains( wxS( "${" ) ) || spelled.Contains( wxS( "@{" ) ) )
                return over;

            pending.push_back( loaderPath( name, file.parent_path() ) );
        }
    }

    return total;
}

} // namespace


std::vector<wxString> SplitTextLines( const std::string& aContent )
{
    std::vector<wxString> lines;

    auto appendLine = [&lines]( std::string aLine )
    {
        if( !aLine.empty() && aLine.back() == '\r' )
            aLine.pop_back();

        wxString text = wxString::FromUTF8( aLine.c_str(), aLine.size() );

        if( text.IsEmpty() && !aLine.empty() )
            text = wxString::From8BitData( aLine.c_str(), aLine.size() );

        lines.push_back( text );
    };

    size_t start = 0;

    while( start <= aContent.size() )
    {
        size_t end = aContent.find( '\n', start );

        if( end == std::string::npos )
        {
            // A trailing newline does not add an empty last line.
            if( start < aContent.size() )
                appendLine( aContent.substr( start ) );

            break;
        }

        appendLine( aContent.substr( start, end - start ) );
        start = end + 1;
    }

    return lines;
}


bool TextLooksBinary( const std::string& aContent )
{
    // Bounded deliberately. std::string::find would scan the whole file first.
    size_t probe = std::min<size_t>( aContent.size(), 8000 );

    return std::memchr( aContent.data(), '\0', probe ) != nullptr;
}


std::vector<TEXT_DIFF_LINE> BuildTextDiff( const std::vector<wxString>& aOld, const std::vector<wxString>& aNew )
{
    std::vector<TEXT_DIFF_LINE> result;

    size_t prefix = 0;

    while( prefix < aOld.size() && prefix < aNew.size() && aOld[prefix] == aNew[prefix] )
        prefix++;

    size_t suffix = 0;

    while( suffix < aOld.size() - prefix && suffix < aNew.size() - prefix
           && aOld[aOld.size() - 1 - suffix] == aNew[aNew.size() - 1 - suffix] )
    {
        suffix++;
    }

    for( size_t i = 0; i < prefix; i++ )
        result.push_back( { TEXT_DIFF_KIND::CONTEXT, aOld[i] } );

    size_t n = aOld.size() - prefix - suffix;
    size_t m = aNew.size() - prefix - suffix;

    // A pair too big for an O(n*m) table takes the cruder path below instead. Dividing rather
    // than multiplying keeps the check from overflowing.
    const size_t maxCells = 4000000;
    const size_t rows = n + 1;
    const size_t stride = m + 1;

    if( n > 0 && m > 0 && rows <= maxCells / stride )
    {
        std::vector<unsigned> lcs( rows * stride, 0 );

        auto cell = [&lcs, stride]( size_t aRow, size_t aCol ) -> unsigned&
        {
            return lcs[aRow * stride + aCol];
        };

        for( size_t i = n; i-- > 0; )
        {
            for( size_t j = m; j-- > 0; )
            {
                if( aOld[prefix + i] == aNew[prefix + j] )
                    cell( i, j ) = cell( i + 1, j + 1 ) + 1;
                else
                    cell( i, j ) = std::max( cell( i + 1, j ), cell( i, j + 1 ) );
            }
        }

        size_t i = 0, j = 0;

        while( i < n && j < m )
        {
            if( aOld[prefix + i] == aNew[prefix + j] )
            {
                result.push_back( { TEXT_DIFF_KIND::CONTEXT, aOld[prefix + i] } );
                i++;
                j++;
            }
            else if( cell( i + 1, j ) >= cell( i, j + 1 ) )
            {
                result.push_back( { TEXT_DIFF_KIND::REMOVED, aOld[prefix + i] } );
                i++;
            }
            else
            {
                result.push_back( { TEXT_DIFF_KIND::ADDED, aNew[prefix + j] } );
                j++;
            }
        }

        for( ; i < n; i++ )
            result.push_back( { TEXT_DIFF_KIND::REMOVED, aOld[prefix + i] } );

        for( ; j < m; j++ )
            result.push_back( { TEXT_DIFF_KIND::ADDED, aNew[prefix + j] } );
    }
    else
    {
        for( size_t i = 0; i < n; i++ )
            result.push_back( { TEXT_DIFF_KIND::REMOVED, aOld[prefix + i] } );

        for( size_t j = 0; j < m; j++ )
            result.push_back( { TEXT_DIFF_KIND::ADDED, aNew[prefix + j] } );
    }

    for( size_t i = aOld.size() - suffix; i < aOld.size(); i++ )
        result.push_back( { TEXT_DIFF_KIND::CONTEXT, aOld[i] } );

    return result;
}


bool DrawingTooLargeToPreview( const wxString& aBefore, const wxString& aAfter, bool aSchematic,
                               const wxString& aBeforeRoot, const wxString& aAfterRoot )
{
    const unsigned long long before = drawingBytes( aBefore, aBeforeRoot, aSchematic, DRAWING_PREVIEW_LIMIT );

    if( before > DRAWING_PREVIEW_LIMIT )
        return true;

    return before + drawingBytes( aAfter, aAfterRoot, aSchematic, DRAWING_PREVIEW_LIMIT - before )
           > DRAWING_PREVIEW_LIMIT;
}

} // namespace KIGIT
