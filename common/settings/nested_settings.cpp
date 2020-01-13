/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020 Jon Evans <jon@craftyjon.com>
 * Copyright (C) 2020 KiCad Developers, see AUTHORS.txt for contributors.
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
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <wx/log.h>

#include <settings/nested_settings.h>

extern const char* traceSettings;


NESTED_SETTINGS::NESTED_SETTINGS( const std::string& aName, int aVersion, JSON_SETTINGS* aParent,
                                  const std::string& aPath, nlohmann::json aDefault ) :
        JSON_SETTINGS( aName, SETTINGS_LOC::NESTED, aVersion, std::move( aDefault ) ),
        m_parent( aParent ), m_path( aPath )
{
    wxASSERT( m_parent );
    m_parent->AddNestedSettings( this );

    // In case we were created after the parent's ctor
    LoadFromFile();
}


void NESTED_SETTINGS::LoadFromFile( const std::string& aDirectory )
{
    clear();

    try
    {
        update( ( *m_parent )[PointerFromString( m_path ) ] );

        wxLogTrace( traceSettings, "Loaded NESTED_SETTINGS %s with schema %d",
                GetFilename(), m_schemaVersion );
    }
    catch( ... )
    {
        wxLogTrace( traceSettings, "NESTED_SETTINGS %s: Could not load from %s at %s",
                m_filename, m_parent->GetFilename(), m_path );
    }

    Load();
}


void NESTED_SETTINGS::SaveToFile( const std::string& aDirectory )
{
    Store();

    try
    {
        ( *m_parent )[PointerFromString( m_path ) ].update( *this );

        wxLogTrace( traceSettings, "Stored NESTED_SETTINGS %s with schema %d",
                    GetFilename(), m_schemaVersion );
    }
    catch( ... )
    {
        wxLogTrace( traceSettings, "NESTED_SETTINGS %s: Could not store to %s at %s",
                    m_filename, m_parent->GetFilename(), m_path );
    }
}
