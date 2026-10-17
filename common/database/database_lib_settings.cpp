/*
* This program source code file is part of KiCad, a free EDA CAD application.
*
* Copyright (C) 2022 Jon Evans <jon@craftyjon.com>
* Copyright The KiCad Developers, see AUTHORS.TXT for contributors.
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

#include <core/kicad_algo.h>
#include <json_common.h>
#include <algorithm>
#include <set>

#include <database/database_lib_settings.h>
#include <kiplatform/secrets.h>
#include <settings/json_settings_internals.h>
#include <settings/parameters.h>
#include <string_utils.h>
#include <wildcards_and_files_ext.h>

#include <wx/filename.h>


const int dblibSchemaVersion = 1;


DATABASE_FIELD_MAPPING::DATABASE_FIELD_MAPPING() :
        visible_on_add( false ),
        visible_in_chooser( false ),
        show_name( false ),
        inherit_properties( false ),
        source( nlohmann::json::object() )
{
}


DATABASE_FIELD_MAPPING::DATABASE_FIELD_MAPPING( const std::string& aColumn, const std::string& aName,
                                                bool aVisibleOnAdd, bool aVisibleInChooser,
                                                bool aShowName, bool aInheritProperties ) :
        column( aColumn ),
        name( aName ),
        name_wx( aName.c_str(), wxConvUTF8 ),
        visible_on_add( aVisibleOnAdd ),
        visible_in_chooser( aVisibleInChooser ),
        show_name( aShowName ),
        inherit_properties( aInheritProperties ),
        source( nlohmann::json::object() )
{
}


void DATABASE_FIELD_MAPPING::SetName( const std::string& aName )
{
    name = aName;
    name_wx = wxString( aName.c_str(), wxConvUTF8 );
}


/**
 * Writes aValue to aObj[aKey] only if it differs from aParsed, the value the raw key was read as.
 * An unchanged key keeps its raw form (or stays absent) unless it is required and missing.
 */
template <typename T>
static void patchKey( nlohmann::json& aObj, const std::string& aKey, const T& aValue, const T& aParsed,
                      bool aRequired )
{
    if( aValue == aParsed && ( aObj.contains( aKey ) || !aRequired ) )
        return;

    if constexpr( std::is_same_v<T, std::string> )
    {
        if( aValue.empty() && !aRequired )
        {
            aObj.erase( aKey );
            return;
        }
    }

    aObj[aKey] = aValue;
}


DATABASE_FIELD_MAPPING DATABASE_LIB_SETTINGS::fieldFromJson( const nlohmann::json& aEntry )
{
    DATABASE_FIELD_MAPPING field( fetchOrDefault<std::string>( aEntry, "column" ),
                                  fetchOrDefault<std::string>( aEntry, "name" ),
                                  fetchOrDefault<bool>( aEntry, "visible_on_add" ),
                                  fetchOrDefault<bool>( aEntry, "visible_in_chooser" ),
                                  fetchOrDefault<bool>( aEntry, "show_name" ),
                                  fetchOrDefault<bool>( aEntry, "inherit_properties" ) );

    field.source = aEntry;
    return field;
}


nlohmann::json DATABASE_LIB_SETTINGS::fieldToJson( const DATABASE_FIELD_MAPPING& aField )
{
    nlohmann::json         out = aField.source.is_object() ? aField.source : nlohmann::json::object();
    DATABASE_FIELD_MAPPING parsed = fieldFromJson( out );

    patchKey( out, "column", aField.column, parsed.column, true );
    patchKey( out, "name", aField.name, parsed.name, true );
    patchKey( out, "visible_on_add", aField.visible_on_add, parsed.visible_on_add, false );
    patchKey( out, "visible_in_chooser", aField.visible_in_chooser, parsed.visible_in_chooser, false );
    patchKey( out, "show_name", aField.show_name, parsed.show_name, false );
    patchKey( out, "inherit_properties", aField.inherit_properties, parsed.inherit_properties, false );

    return out;
}


DATABASE_LIB_TABLE DATABASE_LIB_SETTINGS::tableFromJson( const nlohmann::json& aEntry )
{
    DATABASE_LIB_TABLE table;

    table.source         = aEntry;
    table.name           = fetchOrDefault<std::string>( aEntry, "name" );
    table.table          = fetchOrDefault<std::string>( aEntry, "table" );
    table.key_col        = fetchOrDefault<std::string>( aEntry, "key" );
    table.symbols_col    = fetchOrDefault<std::string>( aEntry, "symbols" );
    table.footprints_col = fetchOrDefault<std::string>( aEntry, "footprints" );
    table.pins_col       = fetchOrDefault<std::string>( aEntry, "pins" );

    // Sanitize library display names; currently only `/` is removed because we
    // use it as a separator and allow it in symbol names.
    std::erase( table.name, '/' );

    if( aEntry.contains( "properties" ) && aEntry["properties"].is_object() )
    {
        const nlohmann::json& propJson = aEntry["properties"];

        table.properties.description = fetchOrDefault<std::string>( propJson, "description" );
        table.properties.footprint_filters = fetchOrDefault<std::string>( propJson, "footprint_filters" );
        table.properties.keywords = fetchOrDefault<std::string>( propJson, "keywords" );
        table.properties.exclude_from_bom = fetchOrDefault<std::string>( propJson, "exclude_from_bom" );
        table.properties.exclude_from_board = fetchOrDefault<std::string>( propJson, "exclude_from_board" );
        table.properties.exclude_from_sim = fetchOrDefault<std::string>( propJson, "exclude_from_sim" );
    }

    if( aEntry.contains( "fields" ) && aEntry["fields"].is_array() )
    {
        for( const nlohmann::json& fieldJson : aEntry["fields"] )
        {
            if( fieldJson.empty() || !fieldJson.is_object() )
                continue;

            table.fields.emplace_back( fieldFromJson( fieldJson ) );
        }
    }

    return table;
}


nlohmann::json DATABASE_LIB_SETTINGS::tableToJson( const DATABASE_LIB_TABLE& aTable )
{
    nlohmann::json     out = aTable.source.is_object() ? aTable.source : nlohmann::json::object();
    DATABASE_LIB_TABLE parsed = tableFromJson( out );

    patchKey( out, "name", aTable.name, parsed.name, true );
    patchKey( out, "table", aTable.table, parsed.table, true );
    patchKey( out, "key", aTable.key_col, parsed.key_col, true );
    patchKey( out, "symbols", aTable.symbols_col, parsed.symbols_col, true );
    patchKey( out, "footprints", aTable.footprints_col, parsed.footprints_col, true );
    patchKey( out, "pins", aTable.pins_col, parsed.pins_col, false );

    const MAPPABLE_SYMBOL_PROPERTIES& props = aTable.properties;
    const MAPPABLE_SYMBOL_PROPERTIES& oldProps = parsed.properties;
    bool hadProps = out.contains( "properties" );
    nlohmann::json propJson = hadProps ? out["properties"] : nlohmann::json::object();

    // A wrong-typed raw value is left alone unless the user changed a mapping
    if( props != oldProps && !propJson.is_object() )
        propJson = nlohmann::json::object();

    if( propJson.is_object() )
    {
        patchKey( propJson, "description", props.description, oldProps.description, false );
        patchKey( propJson, "footprint_filters", props.footprint_filters, oldProps.footprint_filters, false );
        patchKey( propJson, "keywords", props.keywords, oldProps.keywords, false );
        patchKey( propJson, "exclude_from_bom", props.exclude_from_bom, oldProps.exclude_from_bom, false );
        patchKey( propJson, "exclude_from_board", props.exclude_from_board, oldProps.exclude_from_board, false );
        patchKey( propJson, "exclude_from_sim", props.exclude_from_sim, oldProps.exclude_from_sim, false );

        if( hadProps || !propJson.empty() )
            out["properties"] = propJson;
    }

    if( !aTable.fields.empty() || out.contains( "fields" ) )
    {
        nlohmann::json fieldsJson = nlohmann::json::array();

        for( const DATABASE_FIELD_MAPPING& field : aTable.fields )
            fieldsJson.push_back( fieldToJson( field ) );

        // Keep raw entries the parser skipped (non-objects) when the mapping was not edited
        bool fieldsEdited = aTable.fields.size() != parsed.fields.size();

        for( size_t i = 0; !fieldsEdited && i < aTable.fields.size(); ++i )
            fieldsEdited = fieldsJson[i] != fieldToJson( parsed.fields[i] );

        if( fieldsEdited || !out.contains( "fields" ) )
            out["fields"] = fieldsJson;
    }

    return out;
}


DATABASE_LIB_SETTINGS::DATABASE_LIB_SETTINGS( const std::string& aFilename ) :
        JSON_SETTINGS( aFilename, SETTINGS_LOC::NONE, dblibSchemaVersion )
{
    m_params.emplace_back( new PARAM<std::string>( "name", &m_Name, "" ) );

    m_params.emplace_back( new PARAM<std::string>( "description", &m_Description, "" ) );

    m_params.emplace_back( new PARAM<std::string>( "source.dsn", &m_Source.dsn, "" ) );

    m_params.emplace_back( new PARAM<std::string>( "source.username", &m_Source.username, "" ) );

    m_params.emplace_back( new PARAM<std::string>( "source.password", &m_Source.password, "" ) );

    m_params.emplace_back( new PARAM<std::string>( "source.connection_string",
                                                   &m_Source.connection_string, "" ) );

    m_params.emplace_back( new PARAM<int>( "source.timeout_seconds", &m_Source.timeout, 2 ) );

    m_params.emplace_back( new PARAM_LAMBDA<nlohmann::json>(
            "libraries",
            [&]() -> nlohmann::json
            {
                nlohmann::json libraries = nlohmann::json::array();

                for( const DATABASE_LIB_TABLE& table : m_Tables )
                    libraries.push_back( tableToJson( table ) );

                return libraries;
            },
            [&]( const nlohmann::json& aObj )
            {
                m_Tables.clear();

                if( !aObj.is_array() )
                    return;

                for( const nlohmann::json& entry : aObj )
                {
                    if( entry.empty() || !entry.is_object() )
                        continue;

                    m_Tables.emplace_back( tableFromJson( entry ) );
                }
            },
            {} ) );

    m_params.emplace_back( new PARAM<int>( "cache.max_size", &m_Cache.max_size, 256 ) );

    m_params.emplace_back( new PARAM<int>( "cache.max_age", &m_Cache.max_age, 60 ) );

    m_params.emplace_back( new PARAM<bool>( "globally_unique_keys", &m_GloballyUniqueKeys, false ) );

    registerMigration( 0, 1,
                       [&]() -> bool
                       {
                           /*
                            * Schema 0 -> 1
                            * Move internal symbol properties from fields with special names to
                            * a separate place in the schema.
                            */
                            if( !Contains( "libraries" ) || !At( "libraries" ).is_array() )
                                return true;

                            for( nlohmann::json& library : At( "libraries" ) )
                            {
                                if( !library.contains( "fields" ) )
                                    continue;

                                for( const nlohmann::json& field : library["fields"] )
                                {
                                    if( !field.contains( "name" ) || !field.contains( "column" ) )
                                        continue;

                                    std::string name = field["name"].get<std::string>();
                                    std::string col  = field["column"].get<std::string>();

                                    if( name == "ki_description" )
                                        library["properties"]["description"] = col;
                                    else if( name == "ki_fp_filters" )
                                        library["properties"]["footprint_filters"] = col;
                                }
                            }

                           return true;
                       } );
}


/// Objects that hold scalar settings rather than library tables
static constexpr const char* c_scalarParents[] = { "/source", "/cache" };


/// Nested params cannot store into a non-object parent, so replace a malformed one before storing
static void normalizeScalarParents( JSON_SETTINGS_INTERNALS& aInternals )
{
    for( const char* parent : c_scalarParents )
    {
        nlohmann::json::json_pointer ptr( parent );

        if( aInternals.contains( ptr ) && !aInternals[ptr].is_object() )
            aInternals[ptr] = nlohmann::json::object();
    }
}


void DATABASE_LIB_SETTINGS::Load()
{
    JSON_SETTINGS::Load();

    // A schema migration saves right after loading, and that save already needs this baseline
    m_loaded = { m_Name, m_Description, m_Source, m_Cache, m_GloballyUniqueKeys };
}


bool DATABASE_LIB_SETTINGS::Store()
{
    normalizeScalarParents( *Internals() );

    bool modified = JSON_SETTINGS::Store();

    const nlohmann::json& original = originalJson();

    // A new file gets every key written
    if( !original.is_object() || original.empty() )
        return modified;

    const std::vector<std::pair<std::string, bool>> unchanged = {
        { "name", m_Name == m_loaded.name },
        { "description", m_Description == m_loaded.description },
        { "source.dsn", m_Source.dsn == m_loaded.source.dsn },
        { "source.username", m_Source.username == m_loaded.source.username },
        { "source.password", m_Source.password == m_loaded.source.password },
        { "source.connection_string", m_Source.connection_string == m_loaded.source.connection_string },
        { "source.timeout_seconds", m_Source.timeout == m_loaded.source.timeout },
        { "cache.max_size", m_Cache.max_size == m_loaded.cache.max_size },
        { "cache.max_age", m_Cache.max_age == m_loaded.cache.max_age },
        { "globally_unique_keys", m_GloballyUniqueKeys == m_loaded.globallyUniqueKeys }
    };

    std::set<std::string> editedParents;

    for( const auto& [path, isUnchanged] : unchanged )
    {
        nlohmann::json::json_pointer ptr = JSON_SETTINGS_INTERNALS::PointerFromString( path );

        if( !isUnchanged )
        {
            editedParents.insert( ptr.parent_pointer().to_string() );
            continue;
        }

        if( original.contains( ptr ) )
            ( *Internals() )[ptr] = original[ptr];
        else if( Internals()->contains( ptr.parent_pointer() ) )
            ( *Internals() )[ptr.parent_pointer()].erase( ptr.back() );
    }

    // Base Store() creates parent objects, so restore wholly untouched ones to their raw form
    for( const char* parent : c_scalarParents )
    {
        nlohmann::json::json_pointer ptr( parent );

        if( editedParents.contains( parent ) )
            continue;

        if( original.contains( ptr ) )
            ( *Internals() )[ptr] = original[ptr];
        else
            Internals()->erase( ptr.back() );
    }

    return modified;
}


void DATABASE_LIB_SETTINGS::ResolvePassword( const DATABASE_SECRET_LOOKUP& aLookup )
{
    ResolveDatabasePassword( m_Source, aLookup );
    m_loaded.source.password = m_Source.password;
}


wxString DATABASE_LIB_SETTINGS::getFileExt() const
{
    return FILEEXT::DatabaseLibraryFileExtension;
}


std::unique_ptr<DATABASE_LIB_SETTINGS> LoadDatabaseLibSettingsReadOnly( const wxString& aPath, bool* aLoaded )
{
    auto settings = std::make_unique<DATABASE_LIB_SETTINGS>( std::string( aPath.ToUTF8() ) );

    // Read-only before loading so a schema migration cannot rewrite the file
    settings->SetReadOnly( true );
    bool loaded = settings->LoadFromFile();

    // Mirror the path LoadFromFile() reads, which forces the extension; an unreadable file loads as
    // defaults with success reported
    wxFileName path( aPath );
    path.SetExt( FILEEXT::DatabaseLibraryFileExtension );

    if( aLoaded )
        *aLoaded = !path.FileExists() || ( loaded && path.IsFileReadable() );

    return settings;
}


wxString DefaultDatabaseLibFileName( const wxString& aLibName )
{
    wxString name = aLibName;
    name.Trim( true ).Trim( false );
    ReplaceIllegalFileNameChars( name, '_' );

    // A name of only dots would hide the file or name its folder
    if( name.IsEmpty() || name.find_first_not_of( wxS( "._" ) ) == wxString::npos )
        return wxS( "database_library" );

    return name;
}


static wxString passwordService( const DATABASE_SOURCE& aSource )
{
    return wxS( "KiCad ODBC:" ) + wxString::FromUTF8( aSource.dsn );
}


DATABASE_SECRET_LOOKUP DefaultDatabaseSecretLookup()
{
    return KIPLATFORM::SECRETS::GetSecretNoPrompt;
}


bool IsNoPromptSecretLookup( const DATABASE_SECRET_LOOKUP& aLookup )
{
    using NO_PROMPT_FN = bool ( * )( const wxString&, const wxString&, wxString& );

    const NO_PROMPT_FN* target = aLookup.target<NO_PROMPT_FN>();

    return target && *target == &KIPLATFORM::SECRETS::GetSecretNoPrompt;
}


static bool hasKeyringLogin( const DATABASE_SOURCE& aSource )
{
    // Without a user name the DSN supplies its own credentials, so the keyring is not consulted
    return aSource.connection_string.empty() && !aSource.dsn.empty() && !aSource.username.empty();
}


bool GetDatabasePassword( const DATABASE_SOURCE& aSource, std::string& aPassword,
                          const DATABASE_SECRET_LOOKUP& aLookup )
{
    if( !hasKeyringLogin( aSource ) )
        return false;

    DATABASE_SECRET_LOOKUP lookup = aLookup ? aLookup : DefaultDatabaseSecretLookup();
    wxString               secret;

    if( !lookup( passwordService( aSource ), wxString::FromUTF8( aSource.username ), secret ) )
        return false;

    aPassword = secret.utf8_string();
    return true;
}


void ResolveDatabasePassword( DATABASE_SOURCE& aSource, const DATABASE_SECRET_LOOKUP& aLookup )
{
    if( aSource.password.empty() )
        GetDatabasePassword( aSource, aSource.password, aLookup );
}


bool StoreDatabasePassword( const DATABASE_SOURCE& aSource, const std::string& aPassword )
{
    if( !hasKeyringLogin( aSource ) )
        return false;

    wxString service = passwordService( aSource );
    wxString user = wxString::FromUTF8( aSource.username );

    if( aPassword.empty() )
        return KIPLATFORM::SECRETS::DeleteSecret( service, user );

    return KIPLATFORM::SECRETS::StoreSecret( service, user, wxString::FromUTF8( aPassword ) );
}
