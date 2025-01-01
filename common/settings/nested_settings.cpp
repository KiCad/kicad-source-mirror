/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020 Jon Evans <jon@craftyjon.com>
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
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <wx/log.h>

#include <settings/json_settings_internals.h>
#include <settings/nested_settings.h>
#include <locale_io.h>


NESTED_SETTINGS::NESTED_SETTINGS( const std::string& aName, int aVersion, JSON_SETTINGS* aParent,
                                  const std::string& aPath, bool aLoadFromFile ) :
        JSON_SETTINGS( aName, SETTINGS_LOC::NESTED, aVersion ),
        m_parent( aParent ), m_path( aPath )
{
    SetParent( aParent, aLoadFromFile );
}


NESTED_SETTINGS::~NESTED_SETTINGS()
{
    if( m_parent )
        m_parent->ReleaseNestedSettings( this );
}


bool NESTED_SETTINGS::LoadFromFile( const wxString& aDirectory )
{
    m_internals->clear();
    bool success = false;

    if( m_parent )
    {
        nlohmann::json::json_pointer ptr = m_internals->PointerFromString( m_path );

        if( m_parent->m_internals->contains( ptr ) )
        {
            try
            {
                m_internals->update( m_parent->m_internals->at( ptr ) );

                wxLogTrace( traceSettings, wxT( "Loaded NESTED_SETTINGS %s" ), GetFilename() );

                success = true;
            }
            catch( ... )
            {
                wxLogTrace( traceSettings, wxT( "NESTED_SETTINGS %s: Could not load from "
                                                "%s at %s" ),
                            m_filename, m_parent->GetFilename(), m_path );
            }
        }
    }

    if( success )
    {
        int filever = -1;

        try
        {
            filever = m_internals->Get<int>( "meta.version" );
        }
        catch( ... )
        {
            wxLogTrace( traceSettings, wxT( "%s: nested settings version could not be read!" ),
                        m_filename );
            success = false;
        }

        if( filever >= 0 && filever < m_schemaVersion )
        {
            wxLogTrace( traceSettings, wxT( "%s: attempting migration from version %d to %d" ),
                        m_filename, filever, m_schemaVersion );

            bool migrated = false;

            try
            {
                migrated = Migrate();
            }
            catch( ... )
            {
                success = false;
            }

            if( !migrated )
            {
                wxLogTrace( traceSettings, wxT( "%s: migration failed!" ), GetFullFilename() );
                success = false;
            }
        }
        else if( filever > m_schemaVersion )
        {
            wxLogTrace( traceSettings,
                        wxT( "%s: warning: nested settings version %d is newer than latest (%d)" ),
                        m_filename, filever, m_schemaVersion );
        }
        else if( filever >= 0 )
        {
            wxLogTrace( traceSettings, wxT( "%s: schema version %d is current" ),
                        m_filename, filever );
        }
    }

    Load();

    return success;
}


bool NESTED_SETTINGS::SaveToFile( const wxString& aDirectory, bool aForce )
{
    if( !m_parent )
        return false;

    LOCALE_IO dummy;

    try
    {
        bool modified = Store();

        auto jsonObjectInParent = m_parent->GetJson( m_path );

        if( !jsonObjectInParent )
            modified = true;
        else if( !nlohmann::json::diff( *m_internals, jsonObjectInParent.value() ).empty() )
            modified = true;

        if( modified || aForce )
        {
            ( *m_parent->m_internals )[m_path].update( *m_internals );

            wxLogTrace( traceSettings, wxS( "Stored NESTED_SETTINGS %s with schema %d" ),
                        GetFilename(),
                        m_schemaVersion );
        }

        return modified;
    }
    catch( ... )
    {
        wxLogTrace( traceSettings, wxS( "NESTED_SETTINGS %s: Could not store to %s at %s" ),
                    m_filename,
                    m_parent->GetFilename(),
                    m_path );

        return false;
    }
}


void NESTED_SETTINGS::SetParent( JSON_SETTINGS* aParent, bool aLoadFromFile )
{
    m_parent = aParent;

    if( m_parent )
    {
        m_parent->AddNestedSettings( this );

        // In case we were created after the parent's ctor
        if( aLoadFromFile )
            LoadFromFile();
    }
}
