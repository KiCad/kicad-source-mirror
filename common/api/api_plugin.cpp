/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2024 Jon Evans <jon@craftyjon.com>
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

#include <magic_enum.hpp>
#include <json_common.h>
#include <wx/log.h>
#include <wx/regex.h>
#include <wx/stdstream.h>
#include <wx/wfstream.h>

#include <api/api_plugin.h>
#include <api/api_plugin_manager.h>
#include <api/api_utils.h>
#include <json_conversions.h>
#include <json_schema_validator.h>


class LOGGING_ERROR_HANDLER : public nlohmann::json_schema::error_handler
{
public:
    LOGGING_ERROR_HANDLER() : m_hasError( false ) {}

    bool HasError() const { return m_hasError; }

    void error( const nlohmann::json::json_pointer& ptr, const nlohmann::json& instance,
                const std::string& message ) override
    {
        m_hasError = true;
        wxLogTrace( traceApi,
                    wxString::Format( wxS( "JSON error: at %s, value:\n%s\n%s" ),
                                      ptr.to_string(), instance.dump(), message ) );
    }

private:
    bool m_hasError;
};


bool PLUGIN_RUNTIME::FromJson( const nlohmann::json& aJson )
{
    // TODO move to tl::expected and give user feedback about parse errors

    try
    {
        type = magic_enum::enum_cast<PLUGIN_RUNTIME_TYPE>( aJson.at( "type" ).get<std::string>(),
                                                           magic_enum::case_insensitive )
                       .value_or( PLUGIN_RUNTIME_TYPE::INVALID );


    }
    catch( ... )
    {
        return false;
    }

    return type != PLUGIN_RUNTIME_TYPE::INVALID;
}


struct API_PLUGIN_CONFIG
{
    API_PLUGIN_CONFIG( API_PLUGIN& aParent, const wxFileName& aConfigFile,
                       const JSON_SCHEMA_VALIDATOR& aValidator );

    bool valid;
    wxString identifier;
    wxString name;
    wxString description;
    PLUGIN_RUNTIME runtime;
    std::vector<PLUGIN_ACTION> actions;

    API_PLUGIN& parent;
};


API_PLUGIN_CONFIG::API_PLUGIN_CONFIG( API_PLUGIN& aParent, const wxFileName& aConfigFile,
                                      const JSON_SCHEMA_VALIDATOR& aValidator ) :
        parent( aParent )
{
    valid = false;

    if( !aConfigFile.IsFileReadable() )
        return;

    wxLogTrace( traceApi, "Plugin: parsing config file" );

    wxFFileInputStream fp( aConfigFile.GetFullPath(), wxT( "rt" ) );
    wxStdInputStream fstream( fp );

    nlohmann::json js;

    try
    {
        js = nlohmann::json::parse( fstream, nullptr,
                                    /* allow_exceptions = */ true,
                                    /* ignore_comments  = */ true );
    }
    catch( ... )
    {
        wxLogTrace( traceApi, "Plugin: exception during parse" );
        return;
    }

    LOGGING_ERROR_HANDLER handler;
    aValidator.Validate( js, handler, nlohmann::json_uri( "#/definitions/Plugin" ) );

    if( !handler.HasError() )
        wxLogTrace( traceApi, "Plugin: schema validation successful" );

    // All of these are required; any exceptions here leave us with valid == false
    try
    {
        identifier = js.at( "identifier" ).get<wxString>();
        name = js.at( "name" ).get<wxString>();
        description = js.at( "description" ).get<wxString>();

        if( !runtime.FromJson( js.at( "runtime" ) ) )
        {
            wxLogTrace( traceApi, "Plugin: error parsing runtime section" );
            return;
        }
    }
    catch( ... )
    {
        wxLogTrace( traceApi, "Plugin: exception while parsing required keys" );
        return;
    }

    if( !API_PLUGIN::IsValidIdentifier( identifier ) )
    {
        wxLogTrace( traceApi, wxString::Format( "Plugin: identifier %s does not meet requirements",
                                                identifier ) );
        return;
    }

    wxLogTrace( traceApi, wxString::Format( "Plugin: %s (%s)", identifier, name ) );

    try
    {
        const nlohmann::json& actionsJs = js.at( "actions" );

        if( actionsJs.is_array() )
        {
            for( const nlohmann::json& actionJs : actionsJs )
            {
                if( std::optional<PLUGIN_ACTION> a = parent.createActionFromJson( actionJs ) )
                {
                    a->identifier = wxString::Format( "%s.%s", identifier, a->identifier );
                    wxLogTrace( traceApi, wxString::Format( "Plugin: loaded action %s",
                                                            a->identifier ) );
                    actions.emplace_back( *a );
                }
            }
        }
    }
    catch( ... )
    {
        wxLogTrace( traceApi, "Plugin: exception while parsing actions" );
    }

    valid = true;
}


API_PLUGIN::API_PLUGIN( const wxFileName& aConfigFile, const JSON_SCHEMA_VALIDATOR& aValidator ) :
        m_configFile( aConfigFile ),
        m_config( std::make_unique<API_PLUGIN_CONFIG>( *this, aConfigFile, aValidator ) )
{
}


API_PLUGIN::~API_PLUGIN()
{
}


bool API_PLUGIN::IsOk() const
{
    return m_config->valid;
}


bool API_PLUGIN::IsValidIdentifier( const wxString& aIdentifier )
{
    // At minimum, we need a reverse-DNS style identifier with two dots and a 2+ character TLD
    wxRegEx identifierRegex( wxS( "[\\w\\d]{2,}\\.[\\w\\d]+\\.[\\w\\d]+" ) );
    return identifierRegex.Matches( aIdentifier );
}


const wxString& API_PLUGIN::Identifier() const
{
    return m_config->identifier;
}


const wxString& API_PLUGIN::Name() const
{
    return m_config->name;
}


const wxString& API_PLUGIN::Description() const
{
    return m_config->description;
}


const PLUGIN_RUNTIME& API_PLUGIN::Runtime() const
{
    return m_config->runtime;
}


const std::vector<PLUGIN_ACTION>& API_PLUGIN::Actions() const
{
    return m_config->actions;
}


wxString API_PLUGIN::BasePath() const
{
    return m_configFile.GetPath();
}


wxString API_PLUGIN::ActionSettingsKey( const PLUGIN_ACTION& aAction ) const
{
    return Identifier() + "." + aAction.identifier;
}



std::optional<PLUGIN_ACTION> API_PLUGIN::createActionFromJson( const nlohmann::json& aJson )
{
    // TODO move to tl::expected and give user feedback about parse errors
    PLUGIN_ACTION action( *this );

    try
    {
        action.identifier = aJson.at( "identifier" ).get<wxString>();
        wxLogTrace( traceApi, wxString::Format( "Plugin: load action %s", action.identifier ) );
        action.name = aJson.at( "name" ).get<wxString>();
        action.description = aJson.at( "description" ).get<wxString>();
        action.entrypoint = aJson.at( "entrypoint" ).get<wxString>();
        action.show_button = aJson.contains( "show-button" ) && aJson.at( "show-button" ).get<bool>();
    }
    catch( ... )
    {
        wxLogTrace( traceApi, "Plugin: exception while parsing action required keys" );
        return std::nullopt;
    }

    wxFileName f( action.entrypoint );

    if( !f.IsRelative() )
    {
        wxLogTrace( traceApi, wxString::Format( "Plugin: action contains abs path %s; skipping",
                                                action.entrypoint ) );
        return std::nullopt;
    }

    f.Normalize( wxPATH_NORM_ABSOLUTE, m_configFile.GetPath() );

    if( !f.IsFileReadable() )
    {
        wxLogTrace( traceApi, wxString::Format( "WARNING: action entrypoint %s is not readable",
                                                f.GetFullPath() ) );
    }

    if( aJson.contains( "args" ) && aJson.at( "args" ).is_array() )
    {
        for( const nlohmann::json& argJs : aJson.at( "args" ) )
        {
            try
            {
                action.args.emplace_back( argJs.get<wxString>() );
            }
            catch( ... )
            {
                wxLogTrace( traceApi, "Plugin: exception while parsing action args" );
                continue;
            }
        }
    }

    if( aJson.contains( "scopes" ) && aJson.at( "scopes" ).is_array() )
    {
        for( const nlohmann::json& scopeJs : aJson.at( "scopes" ) )
        {
            try
            {
                action.scopes.insert( magic_enum::enum_cast<PLUGIN_ACTION_SCOPE>(
                        scopeJs.get<std::string>(), magic_enum::case_insensitive )
                       .value_or( PLUGIN_ACTION_SCOPE::INVALID ) );
            }
            catch( ... )
            {
                wxLogTrace( traceApi, "Plugin: exception while parsing action scopes" );
                continue;
            }
        }
    }

    auto handleBitmap =
            [&]( const std::string& aKey, wxBitmapBundle& aDest )
            {
                if( aJson.contains( aKey ) && aJson.at( aKey ).is_array() )
                {
                    wxVector<wxBitmap> bitmaps;

                    for( const nlohmann::json& iconJs : aJson.at( aKey ) )
                    {
                        wxFileName iconFile;

                        try
                        {
                            iconFile = iconJs.get<wxString>();
                        }
                        catch( ... )
                        {
                            continue;
                        }

                        iconFile.Normalize( wxPATH_NORM_ABSOLUTE, m_configFile.GetPath() );

                        wxLogTrace( traceApi,
                                    wxString::Format( "Plugin: action %s: loading icon %s",
                                                      action.identifier, iconFile.GetFullPath() ) );


                        if( !iconFile.IsFileReadable() )
                        {
                            wxLogTrace( traceApi, "Plugin: icon file could not be read" );
                            continue;
                        }

                        wxBitmap bmp;
                        // TODO: If necessary; support types other than PNG
                        bmp.LoadFile( iconFile.GetFullPath(), wxBITMAP_TYPE_PNG );

                        if( bmp.IsOk() )
                            bitmaps.push_back( bmp );
                        else
                            wxLogTrace( traceApi, "Plugin: icon file not a valid bitmap" );
                    }

                    aDest = wxBitmapBundle::FromBitmaps( bitmaps );
                }
            };

    handleBitmap( "icons-light", action.icon_light );
    handleBitmap( "icons-dark", action.icon_dark );

    return action;
}
