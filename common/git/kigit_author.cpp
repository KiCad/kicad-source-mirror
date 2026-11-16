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

#include <git/git_config_handler.h>
#include <git/kicad_git_memory.h>

#include <git2.h>
#include <wx/utils.h>


bool KIGIT::ReadConfigString( git_config* aConfig, const char* aKey, wxString& aValue )
{
    git_config_entry* entry = nullptr;
    int               result = git_config_get_entry( &entry, aConfig, aKey );
    GitConfigEntryPtr entryPtr( entry );

    if( result != GIT_OK || !entry )
        return false;

    aValue = wxString::FromUTF8( entry->value );
    return true;
}


void KIGIT::ReadAuthorConfig( git_repository* aRepo, GitUserConfig& aLocal, GitUserConfig& aGlobal )
{
    aLocal = GitUserConfig();
    aGlobal = GitUserConfig();

    git_config* config = nullptr;

    if( !aRepo || git_repository_config( &config, aRepo ) != GIT_OK )
        return;

    GitConfigPtr configPtr( config );

    auto readLevel = []( git_config* aAll, git_config_level_t aLevel, GitUserConfig& aOut )
    {
        git_config* level = nullptr;

        if( git_config_open_level( &level, aAll, aLevel ) != GIT_OK )
            return;

        GitConfigPtr levelPtr( level );
        wxString     value;

        if( aOut.authorName.IsEmpty() && ReadConfigString( level, "user.name", value ) )
            aOut.authorName = value;

        if( aOut.authorEmail.IsEmpty() && ReadConfigString( level, "user.email", value ) )
            aOut.authorEmail = value;

        aOut.hasName = !aOut.authorName.IsEmpty();
        aOut.hasEmail = !aOut.authorEmail.IsEmpty();
    };

    readLevel( config, GIT_CONFIG_LEVEL_LOCAL, aLocal );

    for( git_config_level_t level : { GIT_CONFIG_LEVEL_GLOBAL, GIT_CONFIG_LEVEL_XDG, GIT_CONFIG_LEVEL_SYSTEM,
                                      GIT_CONFIG_LEVEL_PROGRAMDATA } )
    {
        readLevel( config, level, aGlobal );
    }
}


GitUserConfig KIGIT::ResolveAuthor( const COMMON_SETTINGS::GIT& aPrefs, const GitUserConfig& aLocal,
                                    const GitUserConfig& aGlobal, const wxString& aAccountName,
                                    const wxString& aAccountEmail )
{
    auto firstSet = []( std::initializer_list<const wxString*> aCandidates ) -> wxString
    {
        for( const wxString* candidate : aCandidates )
        {
            if( !candidate->IsEmpty() )
                return *candidate;
        }

        return wxString();
    };

    const wxString noValue;
    const bool      custom = !aPrefs.useDefaultAuthor;

    GitUserConfig author;
    author.authorName = firstSet( { &aLocal.authorName, custom ? &aPrefs.authorName : &noValue, &aGlobal.authorName,
                                    &aAccountName } );
    author.authorEmail = firstSet( { &aLocal.authorEmail, custom ? &aPrefs.authorEmail : &noValue,
                                     &aGlobal.authorEmail, &aAccountEmail } );
    author.hasName = !author.authorName.IsEmpty();
    author.hasEmail = !author.authorEmail.IsEmpty();
    return author;
}
