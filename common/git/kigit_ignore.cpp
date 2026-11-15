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
#include <git/kicad_git_common.h>
#include <git/kicad_git_memory.h>

#include <optional>
#include <string>

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

    for( const wxUniChar& ch : aText )
    {
        if( ch == '*' || ch == '?' || ch == '[' || ch == '\\' )
            escaped += '\\';

        escaped += ch;
    }

    return escaped;
}


bool readIgnoreFile( const wxFileName& aIgnoreFile, wxString& aContents )
{
    // Opening a file that is not there logs a system error, and having no rules yet is normal.
    if( !aIgnoreFile.FileExists() )
        return false;

    wxFFile in( aIgnoreFile.GetFullPath(), wxT( "r" ) );

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
        wxString line = tokenizer.GetNextToken();
        line.Trim().Trim( false );

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

    // The leading slash anchors the rule here, so it cannot match the same name further down.
    wxString rule = wxS( "/" ) + escapePattern( aProjectRelPath );

    if( aIsDirectory )
        rule += wxS( "/" );

    return rule;
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
    const bool exists = aIgnoreFile.FileExists();
    wxString   contents;

    if( exists && !readIgnoreFile( aIgnoreFile, contents ) )
        return fileError( aError, _( "Could not read '%s'." ), aIgnoreFile );

    std::vector<wxString> present = splitIgnoreRules( contents );
    wxString              missing;

    for( const wxString& rule : aRules )
    {
        if( rule.IsEmpty() || alg::contains( present, rule ) )
            continue;

        // A rule listed twice in one call would otherwise be written twice.
        present.push_back( rule );
        missing += rule + wxS( "\n" );
    }

    if( missing.IsEmpty() )
        return true;

    if( !exists )
    {
        if( !aHeader.IsEmpty() )
            missing.Prepend( wxS( "# " ) + aHeader + wxS( "\n" ) );
    }
    else if( !contents.IsEmpty() && !contents.EndsWith( wxS( "\n" ) ) )
    {
        // Otherwise the first new rule would land on the end of the user's last one.
        missing.Prepend( wxS( "\n" ) );
    }

    wxFFile out( aIgnoreFile.GetFullPath(), wxT( "a" ) );

    if( !out.IsOpened() || !out.Write( missing ) )
        return fileError( aError, _( "Could not write to '%s'." ), aIgnoreFile );

    return true;
}


bool KIGIT::RemoveIgnoreRules( const wxFileName& aIgnoreFile, const std::vector<wxString>& aRules, wxString& aError )
{
    wxString contents;

    if( !aIgnoreFile.FileExists() )
        return true;

    if( !readIgnoreFile( aIgnoreFile, contents ) )
        return fileError( aError, _( "Could not read '%s'." ), aIgnoreFile );

    wxString kept;
    bool     removed = false;

    wxStringTokenizer tokenizer( contents, wxS( "\n" ), wxTOKEN_RET_EMPTY );

    while( tokenizer.HasMoreTokens() )
    {
        wxString line = tokenizer.GetNextToken();
        wxString rule = line;
        rule.Trim().Trim( false );

        if( !rule.IsEmpty() && alg::contains( aRules, rule ) )
        {
            removed = true;
            continue;
        }

        kept += line + wxS( "\n" );
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
