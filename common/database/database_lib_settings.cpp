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
#include <cctype>
#include <map>
#include <set>

#include <boost/algorithm/string.hpp>

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
 * Splits a connection string into lower-case keys and values for display.  We skip anything else that
 * doesn't match the grammar
 *
 *   string  := { segment ';' } segment
 *   segment := blank | key '=' value
 *   value   := '{' text '}' blank | plain       a doubled '}' inside braces is a literal brace
 *   plain   := text with none of = { } " '
 */
static std::map<std::string, std::string> parseConnectionString( const std::string& aConnectionString )
{
    std::map<std::string, std::string> pairs;
    const size_t                       len = aConnectionString.size();
    size_t                             pos = 0;

    auto skipSpaces = [&]()
    {
        while( pos < len && std::isspace( static_cast<unsigned char>( aConnectionString[pos] ) ) )
            ++pos;
    };

    while( true )
    {
        skipSpaces();

        if( pos >= len )
            break;

        if( aConnectionString[pos] == ';' )
        {
            ++pos;
            continue;
        }

        size_t eq = aConnectionString.find_first_of( "=;", pos );

        if( eq == std::string::npos || aConnectionString[eq] == ';' )
            return {};

        std::string key = boost::algorithm::trim_right_copy( aConnectionString.substr( pos, eq - pos ) );
        boost::algorithm::to_lower( key );

        if( key.empty() )
            return {};

        pos = eq + 1;
        skipSpaces();

        std::string value;

        if( pos < len && aConnectionString[pos] == '{' )
        {
            ++pos;

            bool closed = false;

            while( pos < len && !closed )
            {
                const char c = aConnectionString[pos++];

                if( c != '}' )
                    value += c;
                else if( pos < len && aConnectionString[pos] == '}' )
                    value += aConnectionString[pos++];
                else
                    closed = true;
            }

            if( !closed )
                return {};

            skipSpaces();

            if( pos < len && aConnectionString[pos] != ';' )
                return {};
        }
        else
        {
            size_t end = std::min( aConnectionString.find( ';', pos ), len );

            value = boost::algorithm::trim_right_copy( aConnectionString.substr( pos, end - pos ) );

            if( value.find_first_of( "={}\"'" ) != std::string::npos )
                return {};

            pos = end;
        }

        pairs.emplace( key, value );
    }

    return pairs;
}


/// A shown value holding '=' has swallowed another key's assignment, possibly a credential
static bool holdsAssignment( const std::string& aText )
{
    return aText.find( '=' ) != std::string::npos;
}


DATABASE_SOURCE DATABASE_SOURCE::Expanded( const wxString& aBaseDir ) const
{
    DATABASE_SOURCE expanded = *this;
    const std::string basePath( aBaseDir.ToUTF8() );

    // Database drivers that use files operate on absolute paths, so provide a mechanism
    // for specifying on-disk databases that live next to the kicad_dbl file
    boost::replace_all( expanded.connection_string, "${CWD}", basePath );
    boost::replace_all( expanded.path, "${CWD}", basePath );

    return expanded;
}


std::string DATABASE_SOURCE::Describe() const
{
    // A file path carries no credentials, so it is shown as is
    if( type == DATABASE_SOURCE_TYPE::SQLITE || type == DATABASE_SOURCE_TYPE::ACCESS )
        return path;

    if( connection_string.empty() )
        return holdsAssignment( dsn ) ? std::string{} : dsn;

    // A malformed string parses to no pairs, so nothing is shown
    const std::map<std::string, std::string> pairs = parseConnectionString( connection_string );

    auto find = [&]( std::initializer_list<const char*> aKeys ) -> std::string
    {
        for( const char* key : aKeys )
        {
            auto it = pairs.find( key );

            if( it != pairs.end() && !it->second.empty() )
                return holdsAssignment( it->second ) ? std::string{} : it->second;
        }

        return {};
    };

    // Each field is shown only if clean, so one contaminated value does not hide the other
    const std::string driver = find( { "driver" } );
    const std::string target = find( { "dbq", "database", "data source", "dsn", "server" } );

    if( driver.empty() )
        return target;

    if( target.empty() )
        return driver;

    return target + " (" + driver + ")";
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

    m_params.emplace_back( new PARAM_LAMBDA<nlohmann::json>(
            "source.type",
            [&]() -> nlohmann::json
            {
                switch( m_Source.type )
                {
                case DATABASE_SOURCE_TYPE::ODBC: return "odbc";
                case DATABASE_SOURCE_TYPE::SQLITE: return "sqlite";
                case DATABASE_SOURCE_TYPE::ACCESS: return "access";
                case DATABASE_SOURCE_TYPE::INVALID: return m_Source.type_name;
                }

                return "odbc";
            },
            [&]( const nlohmann::json& aType )
            {
                m_Source.type_name.clear();

                if( aType == "odbc" )
                {
                    m_Source.type = DATABASE_SOURCE_TYPE::ODBC;
                }
                else if( aType == "sqlite" )
                {
                    m_Source.type = DATABASE_SOURCE_TYPE::SQLITE;
                }
                else if( aType == "access" )
                {
                    m_Source.type = DATABASE_SOURCE_TYPE::ACCESS;
                }
                else
                {
                    m_Source.type = DATABASE_SOURCE_TYPE::INVALID;
                    m_Source.type_name = aType.is_string() ? aType.get<std::string>() : aType.dump();
                }
            },
            "odbc" ) );

    m_params.emplace_back( new PARAM<std::string>( "source.path", &m_Source.path, "" ) );

    m_params.emplace_back( new PARAM<std::string>( "source.dsn", &m_Source.dsn, "" ) );

    m_params.emplace_back( new PARAM<std::string>( "source.username", &m_Source.username, "" ) );

    m_params.emplace_back( new PARAM<std::string>( "source.password", &m_Source.password, "" ) );

    m_params.emplace_back( new PARAM<std::string>( "source.connection_string",
                                                   &m_Source.connection_string, "" ) );

    m_params.emplace_back(
            new PARAM<int>( "source.timeout_seconds", &m_Source.timeout, DEFAULT_DATABASE_TIMEOUT_SECONDS ) );

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
        { "source.type", m_Source.type == m_loaded.source.type && m_Source.type_name == m_loaded.source.type_name },
        { "source.path", m_Source.path == m_loaded.source.path },
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


DATABASE_SOURCE DATABASE_LIB_SETTINGS::ExpandedSource( const wxString& aBaseDir ) const
{
    return m_Source.Expanded( aBaseDir.IsEmpty() ? wxFileName( GetFilename() ).GetPath() : aBaseDir );
}


wxString DATABASE_LIB_SETTINGS::GetLoadPath() const
{
    wxFileName path( m_filename );
    path.SetExt( getFileExt() );

    return path.GetFullPath();
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

    // The path LoadFromFile() reads has the extension forced; an unreadable file loads as
    // defaults with success reported
    wxFileName path( settings->GetLoadPath() );

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


std::string DatabasePathForStorage( const wxString& aPath, const wxString& aBaseDir )
{
    const std::string unchanged( aPath.ToUTF8() );

    wxFileName file( aPath );
    wxFileName base;
    base.AssignDir( aBaseDir );

    if( aPath.IsEmpty() || aBaseDir.IsEmpty() || !file.IsAbsolute() || !base.IsAbsolute() )
        return unchanged;

    file.Normalize( wxPATH_NORM_DOTS );
    base.Normalize( wxPATH_NORM_DOTS );

    if( !file.GetVolume().IsSameAs( base.GetVolume(), wxFileName::IsCaseSensitive() ) )
        return unchanged;

    // MakeRelativeTo resolves long names and shortcuts, which stalls on an unreachable share
    const wxArrayString& baseDirs = base.GetDirs();
    const wxArrayString& fileDirs = file.GetDirs();

    if( file.GetFullName().IsEmpty() || fileDirs.GetCount() < baseDirs.GetCount() )
        return unchanged;

    for( size_t i = 0; i < baseDirs.GetCount(); ++i )
    {
        if( !fileDirs[i].IsSameAs( baseDirs[i], wxFileName::IsCaseSensitive() ) )
            return unchanged;
    }

    wxString relative;

    for( size_t i = baseDirs.GetCount(); i < fileDirs.GetCount(); ++i )
        relative << fileDirs[i] << wxS( "/" );

    relative << file.GetFullName();

    return "${CWD}/" + std::string( relative.ToUTF8() );
}


wxString DatabasePathFromInput( const wxString& aInput, const wxString& aBaseDir )
{
    if( aInput.IsEmpty() )
        return aInput;

    DATABASE_SOURCE source;
    source.path = aInput.ToUTF8();

    const wxString expanded = wxString::FromUTF8( source.Expanded( aBaseDir ).path );
    wxFileName     file( expanded );

    if( file.IsAbsolute() )
        return file.GetFullPath();

    // GetFullPath and Normalize switch to the working directory of a foreign volume, which touches that drive
    const bool rooted = !expanded.IsEmpty() && wxFileName::GetPathSeparators().Find( expanded[0] ) != wxNOT_FOUND;

    if( file.HasVolume() || rooted || aBaseDir.IsEmpty() )
        return expanded;

    file.Normalize( wxPATH_NORM_DOTS | wxPATH_NORM_ABSOLUTE, aBaseDir );

    return file.GetFullPath();
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
    // File sources have no login, and without a user name the DSN supplies its own credentials
    return aSource.type == DATABASE_SOURCE_TYPE::ODBC && aSource.connection_string.empty() && !aSource.dsn.empty()
           && !aSource.username.empty();
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
