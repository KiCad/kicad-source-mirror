/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.TXT for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "kigit_ignore.h"

#include <core/kicad_algo.h>
#include <gestfich.h>
#include <git/kicad_git_common.h>
#include <git/kicad_git_memory.h>

#include <filesystem>
#include <optional>
#include <string>

#ifndef __WINDOWS__
#include <fcntl.h>
#endif

#include <wx/ffile.h>
#include <wx/file.h>
#include <wx/tokenzr.h>
#include <wx/translation.h>


namespace
{

// Git reads these as patterns, so a name holding one has to be escaped.
wxString escapePattern( const wxString& aText )
{
    wxString escaped;

    // Git drops trailing spaces unless they are escaped
    const size_t trailing = aText.find_last_not_of( ' ' ) + 1;

    for( size_t ii = 0; ii < aText.length(); ++ii )
    {
        const wxUniChar ch = aText[ii];

        if( ch == '*' || ch == '?' || ch == '[' || ch == '\\' || ( ch == ' ' && ii >= trailing ) )
            escaped += '\\';

        escaped += ch;
    }

    return escaped;
}


// The rule as git reads the line, which drops a CR and the trailing spaces no backslash escapes
wxString ruleText( const wxString& aLine )
{
    const size_t length = aLine.EndsWith( wxS( "\r" ) ) ? aLine.length() - 1 : aLine.length();
    size_t       keep = 0;
    bool         escaped = false;

    for( size_t ii = 0; ii < length; ++ii )
    {
        if( aLine[ii] != ' ' || escaped )
            keep = ii + 1;

        escaped = aLine[ii] == '\\' && !escaped;
    }

    return aLine.Left( keep );
}


// Git does not read a symlinked ignore file, and writing through one changes a file outside the project
bool isSymlink( const wxFileName& aFile )
{
    std::error_code ec;
    return std::filesystem::is_symlink( ToFsPath( aFile.GetFullPath() ), ec );
}


bool readIgnoreFile( const wxFileName& aIgnoreFile, wxString& aContents )
{
    // Opening a file that is not there logs a system error, and having no rules yet is normal.
    if( !aIgnoreFile.FileExists() || isSymlink( aIgnoreFile ) )
        return false;

    // Binary mode keeps the CR of a CRLF file, so new lines can match it
    wxFFile in( aIgnoreFile.GetFullPath(), wxT( "rb" ) );

    return in.IsOpened() && in.ReadAll( &aContents );
}


bool fileError( wxString& aError, const wxString& aMessage, const wxFileName& aFile )
{
    aError = wxString::Format( aMessage, aFile.GetFullPath() );
    return false;
}


std::vector<wxString> splitIgnoreRules( const wxString& aContents )
{
    std::vector<wxString> rules;
    wxStringTokenizer     tokenizer( aContents, wxS( "\n" ) );

    while( tokenizer.HasMoreTokens() )
    {
        const wxString line = ruleText( tokenizer.GetNextToken() );

        if( !line.IsEmpty() && !line.StartsWith( wxS( "#" ) ) )
            rules.push_back( line );
    }

    return rules;
}

} // namespace


wxFileName KIGIT::ProjectIgnoreFile( const wxString& aProjectPath )
{
    return wxFileName( aProjectPath, wxS( ".gitignore" ) );
}


wxString KIGIT::PathIgnoreRule( const wxString& aProjectRelPath, bool aIsDirectory )
{
    if( aProjectRelPath.IsEmpty() )
        return wxEmptyString;

    // The separator goes on first, so a folder name ending in a space needs no escape
    const wxString pattern = aIsDirectory ? aProjectRelPath + wxS( "/" ) : aProjectRelPath;

    // The leading slash anchors the rule here, so it cannot match the same name further down.
    return wxS( "/" ) + escapePattern( pattern );
}


wxString KIGIT::ExtensionIgnoreRule( const wxString& aProjectRelPath )
{
    const wxString ext = wxFileName( aProjectRelPath, wxPATH_UNIX ).GetExt();

    if( ext.IsEmpty() )
        return wxEmptyString;

    return wxS( "*." ) + escapePattern( ext );
}


std::vector<wxString> KIGIT::ReadIgnoreRules( const wxFileName& aIgnoreFile )
{
    wxString contents;

    if( !readIgnoreFile( aIgnoreFile, contents ) )
        return {};

    return splitIgnoreRules( contents );
}


bool KIGIT::AddIgnoreRules( const wxFileName& aIgnoreFile, const std::vector<wxString>& aRules, const wxString& aHeader,
                            wxString& aError )
{
    if( isSymlink( aIgnoreFile ) )
        return fileError( aError, _( "'%s' is a symbolic link, so KiCad does not edit it." ), aIgnoreFile );

    const bool exists = aIgnoreFile.FileExists();
    wxString   contents;

    if( exists && !readIgnoreFile( aIgnoreFile, contents ) )
        return fileError( aError, _( "Could not read '%s'." ), aIgnoreFile );

    std::vector<wxString> present = splitIgnoreRules( contents );
    const wxString        eol = contents.Contains( wxS( "\r\n" ) ) ? wxS( "\r\n" ) : wxS( "\n" );
    wxString              missing;

    for( const wxString& rule : aRules )
    {
        // A line break would split the name into rules that ignore other files
        if( rule.find_first_of( wxS( "\r\n" ) ) != wxString::npos )
            return fileError( aError, _( "A name holding a line break cannot be added to '%s'." ), aIgnoreFile );

        if( rule.IsEmpty() || alg::contains( present, rule ) )
            continue;

        // A rule listed twice in one call would otherwise be written twice.
        present.push_back( rule );
        missing += rule + eol;
    }

    if( missing.IsEmpty() )
        return true;

    if( !exists )
    {
        if( !aHeader.IsEmpty() )
            missing.Prepend( wxS( "# " ) + aHeader + eol );
    }
    else if( !contents.IsEmpty() && !contents.EndsWith( wxS( "\n" ) ) )
    {
        // Otherwise the first new rule would land on the end of the user's last one. A lone CR
        // only needs its LF, or the extra CR would join the rule above
        missing.Prepend( contents.EndsWith( wxS( "\r" ) ) ? wxString( wxS( "\n" ) ) : eol );
    }

#ifdef __WINDOWS__
    wxFFile out( aIgnoreFile.GetFullPath(), wxT( "ab" ) );
#else
    // A link swapped in after the check above must not redirect the append, since the rule text is a file name.
    // Non-blocking, so a FIFO in its place fails instead of hanging the UI
    wxFile out( ::open( aIgnoreFile.GetFullPath().fn_str(),
                        O_WRONLY | O_APPEND | O_CREAT | O_NOFOLLOW | O_NONBLOCK | O_CLOEXEC, 0666 ) );
#endif

    if( !out.IsOpened() || !out.Write( missing, wxConvUTF8 ) || !out.Close() )
        return fileError( aError, _( "Could not write to '%s'." ), aIgnoreFile );

    return true;
}


bool KIGIT::RemoveIgnoreRules( const wxFileName& aIgnoreFile, const std::vector<wxString>& aRules, wxString& aError )
{
    wxString contents;

    if( isSymlink( aIgnoreFile ) )
        return fileError( aError, _( "'%s' is a symbolic link, so KiCad does not edit it." ), aIgnoreFile );

    if( !aIgnoreFile.FileExists() )
        return true;

    if( !readIgnoreFile( aIgnoreFile, contents ) )
        return fileError( aError, _( "Could not read '%s'." ), aIgnoreFile );

    wxString kept;
    bool     removed = false;

    wxStringTokenizer tokenizer( contents, wxS( "\n" ), wxTOKEN_RET_EMPTY_ALL );

    while( tokenizer.HasMoreTokens() )
    {
        const wxString line = tokenizer.GetNextToken();
        const wxString rule = ruleText( line );

        if( !rule.IsEmpty() && alg::contains( aRules, rule ) )
        {
            removed = true;
            continue;
        }

        kept += line;

        // Each line keeps the break it had, so blank lines and an unterminated last line survive
        if( tokenizer.GetLastDelimiter() == '\n' )
            kept += wxS( "\n" );
    }

    if( !removed )
        return true;

    // A rewrite that failed partway would lose rules the user still wants.
    wxTempFile out( aIgnoreFile.GetFullPath() );

    if( !out.IsOpened() || !out.Write( kept ) || !out.Commit() )
        return fileError( aError, _( "Could not write to '%s'." ), aIgnoreFile );

    return true;
}


KIGIT::IGNORE_STATE KIGIT::IgnoreState( KIGIT_COMMON& aCommon, const wxString& aProjectRelPath, bool aIsDirectory )
{
    IGNORE_STATE                  state;
    git_repository*               repo = aCommon.GetRepo();
    const std::optional<wxString> prefix = aCommon.GetProjectPrefix();

    if( !repo || !prefix || aProjectRelPath.IsEmpty() )
        return state;

    const wxString    repoRelative = *prefix + aProjectRelPath;
    const std::string repoRelPath = repoRelative.utf8_string();
    int               ignored = 0;

    // The trailing slash tells libgit2 this is a directory, so directory-only rules apply.
    const std::string ignorePath = aIsDirectory ? repoRelPath + "/" : repoRelPath;

    if( git_ignore_path_is_ignored( &ignored, repo, ignorePath.c_str() ) == GIT_OK )
        state.ignored = ignored != 0;

    git_index* index = nullptr;

    if( git_repository_index( &index, repo ) != GIT_OK )
        return state;

    KIGIT::GitIndexPtr indexPtr( index );

    if( aIsDirectory )
    {
        // A rule for the folder cannot untrack what is already inside it.
        const std::string indexPrefix = repoRelPath + "/";
        size_t            position = 0;

        state.tracked = git_index_find_prefix( &position, index, indexPrefix.c_str() ) == GIT_OK;
    }
    else
    {
        // A file a merge stopped on has no stage zero entry, but git tracks it just the same.
        state.tracked = git_index_get_bypath( index, repoRelPath.c_str(), 0 ) != nullptr
                        || aCommon.IsPathConflicted( repoRelative );
    }

    return state;
}
