/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "qa_utils/env_var_utils.h"

#include <wx/utils.h>

#include <pgm_base.h>


KI_TEST::SCOPED_PROCESS_ENV_VAR::SCOPED_PROCESS_ENV_VAR( const wxString&                aName,
                                                         const std::optional<wxString>& aValue ) :
        m_name( aName )
{
    // Store the old value if it exists
    wxString oldValue;
    if( wxGetEnv( aName, &oldValue ) )
        m_oldValue = oldValue;

    // Set/remove the variable for the lifetime of this object
    if( aValue )
        wxSetEnv( aName, *aValue );
    else
        wxUnsetEnv( aName );
}


KI_TEST::SCOPED_PROCESS_ENV_VAR::~SCOPED_PROCESS_ENV_VAR()
{
    if( m_oldValue )
        wxSetEnv( m_name, *m_oldValue );
    else
        wxUnsetEnv( m_name );
}


void KI_TEST::SCOPED_PROCESS_ENV_VAR::SetValue( const wxString& aValue )
{
    wxSetEnv( m_name, aValue );
}


std::optional<wxString> KI_TEST::SCOPED_PROCESS_ENV_VAR::GetValue() const
{
    wxString value;
    if( wxGetEnv( m_name, &value ) )
        return value;
    else
        return std::nullopt;
}


void KI_TEST::SCOPED_PROCESS_ENV_VAR::ClearValue()
{
    wxUnsetEnv( m_name );
}


KI_TEST::SCOPED_PGM_ENV_VAR::SCOPED_PGM_ENV_VAR( const wxString& aName, const std::optional<wxString>& aValue ) :
        m_pgmEnvVars( Pgm().GetLocalEnvVariables() ),
        m_name( aName )
{
    const auto it = m_pgmEnvVars.find( aName );

    // Store the old value if it exists
    if( it != m_pgmEnvVars.end() )
        m_oldValue = it->second;

    // Set/remove the variable for the lifetime of this object
    if( aValue )
        m_pgmEnvVars[aName] = ENV_VAR_ITEM( aName, *aValue );
    else
        m_pgmEnvVars.erase( aName );
}


KI_TEST::SCOPED_PGM_ENV_VAR::~SCOPED_PGM_ENV_VAR()
{
    if( m_oldValue )
        m_pgmEnvVars[m_name] = *m_oldValue;
    else
        m_pgmEnvVars.erase( m_name );
}


ENV_VAR_ITEM& KI_TEST::SCOPED_PGM_ENV_VAR::GetItem()
{
    return m_pgmEnvVars[m_name];
}


const ENV_VAR_ITEM& KI_TEST::SCOPED_PGM_ENV_VAR::GetItem() const
{
    return m_pgmEnvVars.at( m_name );
}