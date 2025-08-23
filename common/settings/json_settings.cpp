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

#include <algorithm>
#include <fstream>
#include <iomanip>
#include <utility>
#include <sstream>

#include <locale_io.h>
#include <gal/color4d.h>
#include <settings/json_settings.h>
#include <settings/json_settings_internals.h>
#include <settings/nested_settings.h>
#include <settings/parameters.h>
#include <settings/bom_settings.h>
#include <settings/grid_settings.h>
#include <settings/aui_settings.h>
#include <wx/aui/framemanager.h>
#include <wx/config.h>
#include <wx/debug.h>
#include <wx/fileconf.h>
#include <wx/filename.h>
#include <wx/gdicmn.h>
#include <wx/log.h>
#include <wx/stdstream.h>
#include <wx/wfstream.h>


nlohmann::json::json_pointer JSON_SETTINGS_INTERNALS::PointerFromString( std::string aPath )
{
    std::replace( aPath.begin(), aPath.end(), '.', '/' );
    aPath.insert( 0, "/" );

    nlohmann::json::json_pointer p;

    try
    {
        p = nlohmann::json::json_pointer( aPath );
    }
    catch( ... )
    {
        wxASSERT_MSG( false, wxT( "Invalid pointer path in PointerFromString!" ) );
    }

    return p;
}


JSON_SETTINGS::JSON_SETTINGS( const wxString& aFilename, SETTINGS_LOC aLocation,
                              int aSchemaVersion, bool aCreateIfMissing, bool aCreateIfDefault,
                              bool aWriteFile ) :
        m_filename( aFilename ),
        m_legacy_filename( "" ),
        m_location( aLocation ),
        m_createIfMissing( aCreateIfMissing ),
        m_createIfDefault( aCreateIfDefault ),
        m_writeFile( aWriteFile ),
        m_modified( false ),
        m_deleteLegacyAfterMigration( true ),
        m_resetParamsIfMissing( true ),
        m_schemaVersion( aSchemaVersion ),
        m_isFutureFormat( false ),
        m_manager( nullptr )
{
    m_internals = std::make_unique<JSON_SETTINGS_INTERNALS>();

    try
    {
        m_internals->SetFromString( "meta.filename", GetFullFilename() );
    }
    catch( ... )
    {
        wxLogTrace( traceSettings, wxT( "Error: Could not create filename field for %s" ),
                    GetFullFilename() );
    }


    m_params.emplace_back( new PARAM<int>( "meta.version", &m_schemaVersion, m_schemaVersion,
                                           true ) );
}


JSON_SETTINGS::~JSON_SETTINGS()
{
    for( PARAM_BASE* param: m_params )
        delete param;

    m_params.clear();
}


wxString JSON_SETTINGS::GetFullFilename() const
{
    if( m_filename.AfterLast( '.' ) == getFileExt() )
        return m_filename;

    return wxString( m_filename + "." + getFileExt() );
}


nlohmann::json& JSON_SETTINGS::At( const std::string& aPath )
{
    return m_internals->At( aPath );
}


bool JSON_SETTINGS::Contains( const std::string& aPath ) const
{
    return m_internals->contains( JSON_SETTINGS_INTERNALS::PointerFromString( aPath ) );
}


JSON_SETTINGS_INTERNALS* JSON_SETTINGS::Internals()
{
    return m_internals.get();
}


void JSON_SETTINGS::Load()
{
    for( PARAM_BASE* param : m_params )
    {
        try
        {
            param->Load( *this, m_resetParamsIfMissing );
        }
        catch( ... )
        {
            // Skip unreadable parameters in file
            wxLogTrace( traceSettings, wxT( "param '%s' load err" ), param->GetJsonPath().c_str() );
        }
    }
}


bool JSON_SETTINGS::LoadFromFile( const wxString& aDirectory )
{
    // First, load all params to default values
    m_internals->clear();
    Load();

    bool success         = true;
    bool migrated        = false;
    bool legacy_migrated = false;

    LOCALE_IO locale;

    auto migrateFromLegacy =
            [&] ( wxFileName& aPath )
            {
                // Backup and restore during migration so that the original can be mutated if
                // convenient
                bool backed_up = false;
                wxFileName temp;

                if( aPath.IsDirWritable() )
                {
                    temp.AssignTempFileName( aPath.GetFullPath() );

                    if( !wxCopyFile( aPath.GetFullPath(), temp.GetFullPath() ) )
                    {
                        wxLogTrace( traceSettings,
                                    wxT( "%s: could not create temp file for migration" ),
                                    GetFullFilename() );
                    }
                    else
                    {
                        backed_up = true;
                    }
                }

                // Silence popups if legacy file is read-only
                wxLogNull doNotLog;

                wxConfigBase::DontCreateOnDemand();
                auto cfg = std::make_unique<wxFileConfig>( wxT( "" ), wxT( "" ),
                                                           aPath.GetFullPath() );

                // If migrate fails or is not implemented, fall back to built-in defaults that
                // were already loaded above
                if( !MigrateFromLegacy( cfg.get() ) )
                {
                    success = false;
                    wxLogTrace( traceSettings,
                                wxT( "%s: migrated; not all settings were found in legacy file" ),
                                GetFullFilename() );
                }
                else
                {
                    success = true;
                    wxLogTrace( traceSettings, wxT( "%s: migrated from legacy format" ),
                                GetFullFilename() );
                }

                if( backed_up )
                {
                    cfg.reset();

                    if( !wxCopyFile( temp.GetFullPath(), aPath.GetFullPath() ) )
                    {
                        wxLogTrace( traceSettings,
                                    wxT( "migrate; copy temp file %s to %s failed" ),
                                    temp.GetFullPath(),
                                    aPath.GetFullPath() );
                    }

                    if( !wxRemoveFile( temp.GetFullPath() ) )
                    {
                        wxLogTrace( traceSettings,
                                    wxT( "migrate; failed to remove temp file %s" ),
                                    temp.GetFullPath() );
                    }
                 }

                // Either way, we want to clean up the old file afterwards
                legacy_migrated = true;
            };

    wxFileName path;

    if( aDirectory.empty() )
    {
        path.Assign( m_filename );
        path.SetExt( getFileExt() );
    }
    else
    {
        path.Assign( aDirectory, m_filename, getFileExt() );
    }

    if( !path.Exists() )
    {
        // Case 1: legacy migration, no .json extension yet
        path.SetExt( getLegacyFileExt() );

        if( path.Exists() )
        {
            migrateFromLegacy( path );
        }
        // Case 2: legacy filename is different from new one
        else if( !m_legacy_filename.empty() )
        {
            path.SetName( m_legacy_filename );

            if( path.Exists() )
                migrateFromLegacy( path );
        }
        else
        {
            success = false;
        }
    }
    else
    {
        if( !path.IsFileWritable() )
            m_writeFile = false;

        try
        {
            wxFFileInputStream fp( path.GetFullPath(), wxT( "rt" ) );
            wxStdInputStream fstream( fp );

            if( fp.IsOk() )
            {
                *static_cast<nlohmann::json*>( m_internals.get() ) =
                        nlohmann::json::parse( fstream, nullptr,
                                               /* allow_exceptions = */ true,
                                               /* ignore_comments  = */ true );

                // Save whatever we loaded, before doing any migration etc
                m_internals->m_original = *static_cast<nlohmann::json*>( m_internals.get() );

                // If parse succeeds, check if schema migration is required
                int filever = -1;

                try
                {
                    filever = m_internals->Get<int>( "meta.version" );
                }
                catch( ... )
                {
                    wxLogTrace( traceSettings, wxT( "%s: file version could not be read!" ),
                                GetFullFilename() );
                    success = false;
                }

                if( filever >= 0 && filever < m_schemaVersion )
                {
                    wxLogTrace( traceSettings, wxT( "%s: attempting migration from version "
                                                    "%d to %d" ),
                                GetFullFilename(),
                                filever,
                                m_schemaVersion );

                    if( Migrate() )
                    {
                        migrated = true;
                    }
                    else
                    {
                        wxLogTrace( traceSettings, wxT( "%s: migration failed!" ),
                                    GetFullFilename() );
                    }
                }
                else if( filever > m_schemaVersion )
                {
                    wxLogTrace( traceSettings,
                                wxT( "%s: warning: file version %d is newer than latest (%d)" ),
                                GetFullFilename(),
                                filever,
                                m_schemaVersion );
                    m_isFutureFormat = true;
                }
            }
            else
            {
                wxLogTrace( traceSettings, wxT( "%s exists but can't be opened for read" ),
                            GetFullFilename() );
            }
        }
        catch( nlohmann::json::parse_error& error )
        {
            success = false;
            wxLogTrace( traceSettings, wxT( "Json parse error reading %s: %s" ),
                        path.GetFullPath(), error.what() );
            wxLogTrace( traceSettings, wxT( "Attempting migration in case file is in legacy "
                                            "format" ) );
            migrateFromLegacy( path );
        }
    }

    // Now that we have new data in the JSON structure, load the params again
    Load();

    // And finally load any nested settings
    for( NESTED_SETTINGS* settings : m_nested_settings )
        settings->LoadFromFile();

    wxLogTrace( traceSettings, wxT( "Loaded <%s> with schema %d" ),
                GetFullFilename(),
                m_schemaVersion );

    m_modified = false;

    // If we migrated, clean up the legacy file (with no extension)
    if( m_writeFile && ( legacy_migrated || migrated ) )
    {
        if( legacy_migrated && m_deleteLegacyAfterMigration && !wxRemoveFile( path.GetFullPath() ) )
        {
            wxLogTrace( traceSettings, wxT( "Warning: could not remove legacy file %s" ),
                        path.GetFullPath() );
        }

        // And write-out immediately so that we don't lose data if the program later crashes.
        if( m_deleteLegacyAfterMigration )
            SaveToFile( aDirectory, true );
    }

    return success;
}


bool JSON_SETTINGS::Store()
{
    for( PARAM_BASE* param : m_params )
    {
        m_modified |= !param->MatchesFile( *this );
        param->Store( this );
    }

    return m_modified;
}


void JSON_SETTINGS::ResetToDefaults()
{
    for( PARAM_BASE* param : m_params )
        param->SetDefault();
}


std::map<std::string, nlohmann::json> JSON_SETTINGS::GetFileHistories()
{
    std::map<std::string, nlohmann::json> histories;

    for( const std::string& candidate : { std::string( "system.file_history" ) } )
    {
        if( Contains( candidate ) )
            histories[candidate] = GetJson( candidate ).value();
    }

    return histories;
}


bool JSON_SETTINGS::SaveToFile( const wxString& aDirectory, bool aForce )
{
    if( !m_writeFile )
        return false;

    // Default PROJECT won't have a filename set
    if( m_filename.IsEmpty() )
        return false;

    wxFileName path;

    if( aDirectory.empty() )
    {
        path.Assign( m_filename );
        path.SetExt( getFileExt() );
    }
    else
    {
        wxString dir( aDirectory );
        path.Assign( dir, m_filename, getFileExt() );
    }

    if( !m_createIfMissing && !path.FileExists() )
    {
        wxLogTrace( traceSettings,
                    wxT( "File for %s doesn't exist and m_createIfMissing == false; not saving" ),
                    GetFullFilename() );
        return false;
    }

    // Ensure the path exists, and create it if not.
    if( !path.DirExists() && !path.Mkdir() )
    {
        wxLogTrace( traceSettings, wxT( "Warning: could not create path %s, can't save %s" ),
                    path.GetPath(), GetFullFilename() );
        return false;
    }

    if( ( path.FileExists() && !path.IsFileWritable() ) ||
        ( !path.FileExists() && !path.IsDirWritable() ) )
    {
        wxLogTrace( traceSettings, wxT( "File for %s is read-only; not saving" ),
                    GetFullFilename() );
        return false;
    }

    bool modified = false;

    for( NESTED_SETTINGS* settings : m_nested_settings )
    {
        wxCHECK2( settings, continue );

        modified |= settings->SaveToFile();
    }

    modified |= Store();

    if( !modified && !aForce && path.FileExists() )
    {
        wxLogTrace( traceSettings, wxT( "%s contents not modified, skipping save" ),
                    GetFullFilename() );
        return false;
    }
    else if( !modified && !aForce && !m_createIfDefault )
    {
        wxLogTrace( traceSettings,
                    wxT( "%s contents still default and m_createIfDefault == false; not saving" ),
                    GetFullFilename() );
        return false;
    }

    wxLogTrace( traceSettings, wxT( "Saving %s" ), GetFullFilename() );

    LOCALE_IO dummy;
    bool success = true;

    nlohmann::json toSave = m_internals->m_original;


    for( PARAM_BASE* param : m_params )
    {
        if( param->ClearUnknownKeys() )
        {
            nlohmann::json_pointer p = JSON_SETTINGS_INTERNALS::PointerFromString( param->GetJsonPath() );

            toSave[p] = nlohmann::json( {} );
        }
    }

    toSave.update( m_internals->begin(), m_internals->end(), /* merge_objects = */ true );

    try
    {
        std::stringstream buffer;
        buffer << std::setw( 2 ) << toSave << std::endl;

        wxFFileOutputStream fileStream( path.GetFullPath(), "wb" );

        if( !fileStream.IsOk()
                || !fileStream.WriteAll( buffer.str().c_str(), buffer.str().size() ) )
        {
            wxLogTrace( traceSettings, wxT( "Warning: could not save %s" ), GetFullFilename() );
            success = false;
        }
    }
    catch( nlohmann::json::exception& error )
    {
        wxLogTrace( traceSettings, wxT( "Catch error: could not save %s. Json error %s" ),
                    GetFullFilename(), error.what() );
        success = false;
    }
    catch( ... )
    {
        wxLogTrace( traceSettings, wxT( "Error: could not save %s." ) );
        success = false;
    }

    if( success )
        m_modified = false;

    return success;
}


const std::string JSON_SETTINGS::FormatAsString()
{
    Store();

    LOCALE_IO dummy;

    std::stringstream buffer;
    buffer << std::setw( 2 ) << *m_internals << std::endl;

    return buffer.str();
}


bool JSON_SETTINGS::LoadFromRawFile( const wxString& aPath )
{
    try
    {
        wxFFileInputStream fp( aPath, wxT( "rt" ) );
        wxStdInputStream   fstream( fp );

        if( fp.IsOk() )
        {
            *static_cast<nlohmann::json*>( m_internals.get() ) =
                    nlohmann::json::parse( fstream, nullptr,
                                           /* allow_exceptions = */ true,
                                           /* ignore_comments  = */ true );
        }
        else
        {
            return false;
        }
    }
    catch( nlohmann::json::parse_error& error )
    {
        wxLogTrace( traceSettings, wxT( "Json parse error reading %s: %s" ), aPath, error.what() );

        return false;
    }

    // Now that we have new data in the JSON structure, load the params again
    Load();
    return true;
}


std::optional<nlohmann::json> JSON_SETTINGS::GetJson( const std::string& aPath ) const
{
    nlohmann::json::json_pointer ptr = m_internals->PointerFromString( aPath );

    if( m_internals->contains( ptr ) )
    {
        try
        {
            return std::optional<nlohmann::json>{ m_internals->at( ptr ) };
        }
        catch( ... )
        {
        }
    }

    return std::optional<nlohmann::json>{};
}


template<typename ValueType>
std::optional<ValueType> JSON_SETTINGS::Get( const std::string& aPath ) const
{
    if( std::optional<nlohmann::json> ret = GetJson( aPath ) )
    {
        try
        {
            return ret->get<ValueType>();
        }
        catch( ... )
        {
        }
    }

    return std::nullopt;
}


// Instantiate all required templates here to allow reducing scope of json.hpp
template KICOMMON_API std::optional<bool>
                      JSON_SETTINGS::Get<bool>( const std::string& aPath ) const;
template KICOMMON_API std::optional<double>
                      JSON_SETTINGS::Get<double>( const std::string& aPath ) const;
template KICOMMON_API std::optional<float>
                      JSON_SETTINGS::Get<float>( const std::string& aPath ) const;
template KICOMMON_API std::optional<int> JSON_SETTINGS::Get<int>( const std::string& aPath ) const;
template KICOMMON_API std::optional<unsigned int>
                      JSON_SETTINGS::Get<unsigned int>( const std::string& aPath ) const;
template KICOMMON_API std::optional<unsigned long long>
                      JSON_SETTINGS::Get<unsigned long long>( const std::string& aPath ) const;
template KICOMMON_API std::optional<std::string>
                      JSON_SETTINGS::Get<std::string>( const std::string& aPath ) const;
template KICOMMON_API std::optional<nlohmann::json>
                      JSON_SETTINGS::Get<nlohmann::json>( const std::string& aPath ) const;
template KICOMMON_API std::optional<KIGFX::COLOR4D>
                      JSON_SETTINGS::Get<KIGFX::COLOR4D>( const std::string& aPath ) const;
template KICOMMON_API std::optional<BOM_FIELD>
                      JSON_SETTINGS::Get<BOM_FIELD>( const std::string& aPath ) const;
template KICOMMON_API std::optional<BOM_PRESET>
                      JSON_SETTINGS::Get<BOM_PRESET>( const std::string& aPath ) const;
template KICOMMON_API std::optional<BOM_FMT_PRESET>
                      JSON_SETTINGS::Get<BOM_FMT_PRESET>( const std::string& aPath ) const;
template KICOMMON_API std::optional<GRID>
                      JSON_SETTINGS::Get<GRID>( const std::string& aPath ) const;
template KICOMMON_API std::optional<wxPoint>
                      JSON_SETTINGS::Get<wxPoint>( const std::string& aPath ) const;
template KICOMMON_API std::optional<wxSize>
                      JSON_SETTINGS::Get<wxSize>( const std::string& aPath ) const;
template KICOMMON_API std::optional<wxRect>
                      JSON_SETTINGS::Get<wxRect>( const std::string& aPath ) const;
template KICOMMON_API std::optional<wxAuiPaneInfo>
                      JSON_SETTINGS::Get<wxAuiPaneInfo>( const std::string& aPath ) const;
template KICOMMON_API std::optional<KIGFX::CROSS_HAIR_MODE>
                      JSON_SETTINGS::Get<KIGFX::CROSS_HAIR_MODE>( const std::string& aPath ) const;

template<typename ValueType>
void JSON_SETTINGS::Set( const std::string& aPath, ValueType aVal )
{
    m_internals->SetFromString( aPath, std::move( aVal ) );
}


// Instantiate all required templates here to allow reducing scope of json.hpp
template KICOMMON_API void JSON_SETTINGS::Set<bool>( const std::string& aPath, bool aValue );
template KICOMMON_API void JSON_SETTINGS::Set<double>( const std::string& aPath, double aValue );
template KICOMMON_API void JSON_SETTINGS::Set<float>( const std::string& aPath, float aValue );
template KICOMMON_API void JSON_SETTINGS::Set<int>( const std::string& aPath, int aValue );
template KICOMMON_API void JSON_SETTINGS::Set<unsigned int>( const std::string& aPath,
                                                             unsigned int       aValue );
template KICOMMON_API void JSON_SETTINGS::Set<unsigned long long>( const std::string& aPath,
                                                                   unsigned long long aValue );
template KICOMMON_API void JSON_SETTINGS::Set<const char*>( const std::string& aPath,
                                                            const char*        aValue );
template KICOMMON_API void JSON_SETTINGS::Set<std::string>( const std::string& aPath,
                                                            std::string        aValue );
template KICOMMON_API void JSON_SETTINGS::Set<nlohmann::json>( const std::string& aPath,
                                                               nlohmann::json     aValue );
template KICOMMON_API void JSON_SETTINGS::Set<KIGFX::COLOR4D>( const std::string& aPath,
                                                               KIGFX::COLOR4D     aValue );
template KICOMMON_API void JSON_SETTINGS::Set<BOM_FIELD>( const std::string& aPath,
                                                          BOM_FIELD          aValue );
template KICOMMON_API void JSON_SETTINGS::Set<BOM_PRESET>( const std::string& aPath,
                                                           BOM_PRESET         aValue );
template KICOMMON_API void JSON_SETTINGS::Set<BOM_FMT_PRESET>( const std::string& aPath,
                                                               BOM_FMT_PRESET     aValue );
template KICOMMON_API void JSON_SETTINGS::Set<GRID>( const std::string& aPath, GRID aValue );
template KICOMMON_API void JSON_SETTINGS::Set<wxPoint>( const std::string& aPath, wxPoint aValue );
template KICOMMON_API void JSON_SETTINGS::Set<wxSize>( const std::string& aPath, wxSize aValue );
template KICOMMON_API void JSON_SETTINGS::Set<wxRect>( const std::string& aPath, wxRect aValue );
template KICOMMON_API void JSON_SETTINGS::Set<wxAuiPaneInfo>( const std::string& aPath,
                                                              wxAuiPaneInfo      aValue );
template KICOMMON_API void JSON_SETTINGS::Set<KIGFX::CROSS_HAIR_MODE>( const std::string& aPath,
                                                                        KIGFX::CROSS_HAIR_MODE aValue );


void JSON_SETTINGS::registerMigration( int aOldSchemaVersion, int aNewSchemaVersion,
                                       std::function<bool()> aMigrator )
{
    wxASSERT( aNewSchemaVersion > aOldSchemaVersion );
    wxASSERT( aNewSchemaVersion <= m_schemaVersion );
    m_migrators[aOldSchemaVersion] = std::make_pair( aNewSchemaVersion, aMigrator );
}


bool JSON_SETTINGS::Migrate()
{
    int filever = m_internals->Get<int>( "meta.version" );

    while( filever < m_schemaVersion )
    {
        wxASSERT( m_migrators.count( filever ) > 0 );

        if( !m_migrators.count( filever ) )
        {
            wxLogTrace( traceSettings, wxT( "Migrator missing for %s version %d!" ),
                        typeid( *this ).name(),
                        filever );
            return false;
        }

        std::pair<int, std::function<bool()>> pair = m_migrators.at( filever );

        if( pair.second() )
        {
            wxLogTrace( traceSettings, wxT( "Migrated %s from %d to %d" ),
                        typeid( *this ).name(),
                        filever,
                        pair.first );
            filever = pair.first;
            m_internals->At( "meta.version" ) = filever;
        }
        else
        {
            wxLogTrace( traceSettings, wxT( "Migration failed for %s from %d to %d" ),
                        typeid( *this ).name(),
                        filever,
                        pair.first );
            return false;
        }
    }

    return true;
}


bool JSON_SETTINGS::MigrateFromLegacy( wxConfigBase* aLegacyConfig )
{
    wxLogTrace( traceSettings, wxT( "MigrateFromLegacy() not implemented for %s" ),
                typeid( *this ).name() );
    return false;
}


bool JSON_SETTINGS::SetIfPresent( const nlohmann::json& aObj, const std::string& aPath,
                                  wxString& aTarget )
{
    nlohmann::json::json_pointer ptr = JSON_SETTINGS_INTERNALS::PointerFromString( aPath );

    if( aObj.contains( ptr ) && aObj.at( ptr ).is_string() )
    {
        aTarget = aObj.at( ptr ).get<wxString>();
        return true;
    }

    return false;
}


bool JSON_SETTINGS::SetIfPresent( const nlohmann::json& aObj, const std::string& aPath,
                                  bool& aTarget )
{
    nlohmann::json::json_pointer ptr = JSON_SETTINGS_INTERNALS::PointerFromString( aPath );

    if( aObj.contains( ptr ) && aObj.at( ptr ).is_boolean() )
    {
        aTarget = aObj.at( ptr ).get<bool>();
        return true;
    }

    return false;
}


bool JSON_SETTINGS::SetIfPresent( const nlohmann::json& aObj, const std::string& aPath,
                                  int& aTarget )
{
    nlohmann::json::json_pointer ptr = JSON_SETTINGS_INTERNALS::PointerFromString( aPath );

    if( aObj.contains( ptr ) && aObj.at( ptr ).is_number_integer() )
    {
        aTarget = aObj.at( ptr ).get<int>();
        return true;
    }

    return false;
}


bool JSON_SETTINGS::SetIfPresent( const nlohmann::json& aObj, const std::string& aPath,
                                  unsigned int& aTarget )
{
    nlohmann::json::json_pointer ptr = JSON_SETTINGS_INTERNALS::PointerFromString( aPath );

    if( aObj.contains( ptr ) && aObj.at( ptr ).is_number_unsigned() )
    {
        aTarget = aObj.at( ptr ).get<unsigned int>();
        return true;
    }

    return false;
}


template<typename ValueType>
bool JSON_SETTINGS::fromLegacy( wxConfigBase* aConfig, const std::string& aKey,
                             const std::string& aDest )
{
    ValueType val;

    if( aConfig->Read( aKey, &val ) )
    {
        try
        {
            ( *m_internals )[aDest] = val;
        }
        catch( ... )
        {
            wxASSERT_MSG( false, wxT( "Could not write value in fromLegacy!" ) );
            return false;
        }

        return true;
    }

    return false;
}


// Explicitly declare these because we only support a few types anyway, and it means we can keep
// wxConfig detail out of the header file
template
KICOMMON_API bool JSON_SETTINGS::fromLegacy<int>( wxConfigBase*, const std::string&,
                                                  const std::string& );

template
KICOMMON_API bool JSON_SETTINGS::fromLegacy<double>( wxConfigBase*, const std::string&,
                                                     const std::string& );

template
KICOMMON_API bool JSON_SETTINGS::fromLegacy<bool>( wxConfigBase*, const std::string&,
                                                   const std::string& );


bool JSON_SETTINGS::fromLegacyString( wxConfigBase* aConfig, const std::string& aKey,
                                      const std::string& aDest )
{
    wxString str;

    if( aConfig->Read( aKey, &str ) )
    {
        try
        {
            ( *m_internals )[aDest] = str.ToUTF8();
        }
        catch( ... )
        {
            wxASSERT_MSG( false, wxT( "Could not write value in fromLegacyString!" ) );
            return false;
        }

        return true;
    }

    return false;
}


bool JSON_SETTINGS::fromLegacyColor( wxConfigBase* aConfig, const std::string& aKey,
    const std::string& aDest )
{
    wxString str;

    if( aConfig->Read( aKey, &str ) )
    {
        KIGFX::COLOR4D color;
        color.SetFromWxString( str );

        try
        {
            nlohmann::json js = nlohmann::json::array( { color.r, color.g, color.b, color.a } );
            ( *m_internals )[aDest] = std::move( js );
        }
        catch( ... )
        {
            wxASSERT_MSG( false, wxT( "Could not write value in fromLegacyColor!" ) );
            return false;
        }

        return true;
    }

    return false;
}


void JSON_SETTINGS::AddNestedSettings( NESTED_SETTINGS* aSettings )
{
    wxLogTrace( traceSettings, wxT( "AddNestedSettings %s" ), aSettings->GetFilename() );
    m_nested_settings.push_back( aSettings );
}


void JSON_SETTINGS::ReleaseNestedSettings( NESTED_SETTINGS* aSettings )
{
    if( !aSettings || !m_manager )
        return;

    auto it = std::find_if( m_nested_settings.begin(), m_nested_settings.end(),
                            [&aSettings]( const JSON_SETTINGS* aPtr )
                            {
                                return aPtr == aSettings;
                            } );

    if( it != m_nested_settings.end() )
    {
        wxLogTrace( traceSettings, wxT( "Flush and release %s" ), ( *it )->GetFilename() );
        m_modified |= ( *it )->SaveToFile();
        m_nested_settings.erase( it );
    }

    aSettings->SetParent( nullptr );
}


// Specializations to allow conversion between wxString and std::string via JSON_SETTINGS API
template<>
std::optional<wxString> JSON_SETTINGS::Get( const std::string& aPath ) const
{
    if( std::optional<nlohmann::json> opt_json = GetJson( aPath ) )
        return wxString( opt_json->get<std::string>().c_str(), wxConvUTF8 );

    return std::nullopt;
}


template<>
void JSON_SETTINGS::Set<wxString>( const std::string& aPath, wxString aVal )
{
    ( *m_internals )[aPath] = aVal.ToUTF8();
}


template<typename ResultType>
ResultType JSON_SETTINGS::fetchOrDefault( const nlohmann::json& aJson, const std::string& aKey,
                                          ResultType aDefault )
{
    ResultType ret = std::move( aDefault );

    try
    {
        if( aJson.contains( aKey ) )
            ret = aJson.at( aKey ).get<ResultType>();
    }
    catch( ... )
    {
    }

    return ret;
}


template
KICOMMON_API std::string JSON_SETTINGS::fetchOrDefault( const nlohmann::json& aJson,
                                                        const std::string& aKey,
                                                        std::string aDefault );


template
KICOMMON_API bool JSON_SETTINGS::fetchOrDefault( const nlohmann::json& aJson,
                                                 const std::string& aKey, bool aDefault );
