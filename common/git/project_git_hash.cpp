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

#include <git/project_git_utils.h>
#include <git/kicad_git_memory.h>

#include <text_eval/text_eval_environment.h>
#include <string_utils.h>

#include <wx/string.h>

namespace KIGIT
{

git_oid PROJECT_GIT_UTILS::GetCapturedHeadOid( git_repository* aRepo )
{
    if( !aRepo )
        return {};

    using ENVIRONMENT = TEXT_EVAL::ENVIRONMENT;
    const auto read = [&]() -> ENVIRONMENT::VCS_VALUE
    {
        git_oid oid{};

        if( git_reference_name_to_id( &oid, aRepo, "HEAD" ) != 0 )
            return {};

        char hash[GIT_OID_HEXSZ + 1];
        git_oid_tostr( hash, sizeof( hash ), &oid );
        return { hash };
    };
    auto* environment = ENVIRONMENT::Current();
    const char* path = git_repository_path( aRepo );
    const auto value = environment && path
            ? environment->VcsValue( { ENVIRONMENT::VCS_QUERY::HEAD, wxString::FromUTF8( path ), "", 0, false }, read )
            : read();
    git_oid oid{};

    if( value.text.empty() || git_oid_fromstr( &oid, value.text.c_str() ) != 0 )
        return {};

    return oid;
}


wxString PROJECT_GIT_UTILS::GetCurrentHash( const wxString& aProjectFile, bool aShort )
{
    const auto read = [&]
    {
        GitRepositoryPtr repo( GetRepositoryForFile( TO_UTF8( aProjectFile ) ) );
        const git_oid oid = GetCapturedHeadOid( repo.get() );

        if( git_oid_is_zero( &oid ) )
            return wxString( "no hash" );

        char hash[GIT_OID_HEXSZ + 1];
        git_oid_tostr( hash, sizeof( hash ), &oid );
        return wxString::FromUTF8( hash );
    };
    auto*          environment = TEXT_EVAL::ENVIRONMENT::Current();
    const wxString hash = environment ? environment->GitHash( aProjectFile, read ) : read();
    return aShort ? hash.Left( 8 ) : hash;
}

} // namespace KIGIT
