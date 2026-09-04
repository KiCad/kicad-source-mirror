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

#include <text_eval/text_eval_environment.h>
#include <wx/debug.h>

namespace TEXT_EVAL
{
namespace
{
    // Not a class member because MSVC cannot dllexport thread-local data
    thread_local ENVIRONMENT* s_active = nullptr;
} // namespace


ENVIRONMENT::~ENVIRONMENT()
{
    wxASSERT( !m_sources );
    wxASSERT( s_active != this );
}


ENVIRONMENT* ENVIRONMENT::Current()
{
    return s_active;
}


wxDateTime ENVIRONMENT::CurrentTime()
{
    if( !s_active )
        return wxDateTime::Now();

    for( SOURCE_SCOPE* scope = s_active->m_sources; scope; scope = scope->m_previous )
        scope->m_values.time = s_active->m_time;

    return s_active->m_time;
}


void ENVIRONMENT::RecordEnvironmentVariable( const wxString& aName, const std::optional<wxString>& aValue )
{
    for( SOURCE_SCOPE* scope = m_sources; scope; scope = scope->m_previous )
        scope->m_values.environmentVariables.insert_or_assign( aName, aValue );
}


void ENVIRONMENT::RecordRandomUse()
{
    for( SOURCE_SCOPE* scope = m_sources; scope; scope = scope->m_previous )
        scope->m_values.randomUsed = true;
}


void ENVIRONMENT::RecordCrossReference( const CROSS_REFERENCE_KEY& aKey, const CROSS_REFERENCE_VALUE& aValue )
{
    for( SOURCE_SCOPE* scope = m_sources; scope; scope = scope->m_previous )
        scope->m_values.crossReferences.insert_or_assign( aKey, aValue );
}


const wxString& ENVIRONMENT::GitHash( const wxString& aPath, const std::function<wxString()>& aRead )
{
    auto found = m_gitHashes.find( aPath );

    if( found == m_gitHashes.end() )
        found = m_gitHashes.emplace( aPath, aRead() ).first;

    for( SOURCE_SCOPE* scope = m_sources; scope; scope = scope->m_previous )
        scope->m_values.gitHashes.insert_or_assign( aPath, found->second );

    return found->second;
}


const ENVIRONMENT::VCS_VALUE& ENVIRONMENT::VcsValue( const VCS_KEY& aKey, const std::function<VCS_VALUE()>& aRead )
{
    auto found = m_vcsValues.find( aKey );

    if( found == m_vcsValues.end() )
        found = m_vcsValues.emplace( aKey, aRead() ).first;

    for( SOURCE_SCOPE* scope = m_sources; scope; scope = scope->m_previous )
        scope->m_values.vcsValues.insert_or_assign( aKey, found->second );

    return found->second;
}


SOURCE_SCOPE::SOURCE_SCOPE( ENVIRONMENT& aEnvironment, ENVIRONMENT::SOURCE_VALUES& aValues ) :
        m_environment( aEnvironment ),
        m_values( aValues ),
        m_previous( aEnvironment.m_sources )
{
    m_environment.m_sources = this;
}


SOURCE_SCOPE::~SOURCE_SCOPE()
{
    wxASSERT( m_environment.m_sources == this );
    m_environment.m_sources = m_previous;
}


ENVIRONMENT_SCOPE::ENVIRONMENT_SCOPE( ENVIRONMENT& aEnvironment ) :
        m_previous( s_active )
{
    s_active = &aEnvironment;
}


ENVIRONMENT_SCOPE::~ENVIRONMENT_SCOPE()
{
    s_active = m_previous;
}
} // namespace TEXT_EVAL
