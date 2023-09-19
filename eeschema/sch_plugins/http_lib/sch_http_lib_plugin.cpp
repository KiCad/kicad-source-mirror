/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2023 Andre F. K. Iwers <iwers11@gmail.com>
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

#include <fmt.h>
#include <lib_symbol.h>
#include <symbol_lib_table.h>

#include <http_lib/http_lib_connection.h>
#include "sch_http_lib_plugin.h"


SCH_HTTP_LIB_PLUGIN::SCH_HTTP_LIB_PLUGIN() :
    m_libTable( nullptr )
{
}


SCH_HTTP_LIB_PLUGIN::~SCH_HTTP_LIB_PLUGIN()
{
}


void SCH_HTTP_LIB_PLUGIN::EnumerateSymbolLib( wxArrayString&         aSymbolNameList,
                                              const wxString&        aLibraryPath,
                                              const STRING_UTF8_MAP* aProperties )
{
    std::vector<LIB_SYMBOL*> symbols;
    EnumerateSymbolLib( symbols, aLibraryPath, aProperties );

    for( LIB_SYMBOL* symbol : symbols )
        aSymbolNameList.Add( symbol->GetName() );
}


void SCH_HTTP_LIB_PLUGIN::EnumerateSymbolLib( std::vector<LIB_SYMBOL*>& aSymbolList,
                                             const wxString&           aLibraryPath,
                                             const STRING_UTF8_MAP*    aProperties )
{
    wxCHECK_RET( m_libTable, _("httplib plugin missing library table handle!") );
    ensureSettings( aLibraryPath );
    ensureConnection();

    if( !m_conn)
    {
        THROW_IO_ERROR( m_lastError );
        return;
    }

    bool powerSymbolsOnly =
            ( aProperties
              && aProperties->find( SYMBOL_LIB_TABLE::PropPowerSymsOnly ) != aProperties->end() );

    // clear buffer
    m_cachedParts.clear();

    for(const HTTP_LIB_CATEGORY& category : m_conn->getCategories() )
    {
        std::vector<HTTP_LIB_PART> found_parts;

        if( !m_conn->SelectAll( category, found_parts ) )
        {
            if( !m_conn->GetLastError().empty() )
            {
                wxString msg = wxString::Format( _( "Error retriving data from HTTP library %s: %s" ),
                                                 category.name, m_conn->GetLastError() );
                THROW_IO_ERROR( msg );
            }

            continue;
        }

        // cache information for later use in LoadSymbol()
        m_cachedParts.emplace( category.id, found_parts );

        for( const HTTP_LIB_PART& part : found_parts )
        {

            wxString libIDString( part.name );

            LIB_SYMBOL* symbol = loadSymbolFromPart( libIDString, category, part );

            if( symbol && ( !powerSymbolsOnly || symbol->IsPower() ) )
                aSymbolList.emplace_back( symbol );

        }
    }
}


LIB_SYMBOL* SCH_HTTP_LIB_PLUGIN::LoadSymbol( const wxString&        aLibraryPath,
                                             const wxString&        aAliasName,
                                             const STRING_UTF8_MAP* aProperties )
{
    wxCHECK( m_libTable, nullptr );
    ensureSettings( aLibraryPath );
    ensureConnection();

    if( !m_conn )
        THROW_IO_ERROR( m_lastError );

    std::string part_id = "";

    std::string partName( aAliasName.ToUTF8() );

    const HTTP_LIB_CATEGORY* foundCategory = nullptr;
    HTTP_LIB_PART            result;

    std::vector<HTTP_LIB_CATEGORY> categories = m_conn->getCategories();

    std::tuple relations = m_conn->getCachedParts()[partName];

    // get the matching category
    for( const HTTP_LIB_CATEGORY& categoryIter : categories )
    {
        std::string associatedCatID = std::get<1>( relations );
        if( categoryIter.id == associatedCatID )
        {
            foundCategory = &categoryIter;

            break;
        }
    }

    // return Null if no category was found. This should never happen
    if( foundCategory == NULL )
    {
        wxLogTrace( traceHTTPLib, wxT( "loadSymbol: no category found for %s" ), partName );

        return NULL;
    }

    // get the matching query ID
    for( const auto& part : m_cachedParts[foundCategory->id] )
    {
        if( part.id == std::get<0>( relations ) )
        {
            part_id = part.id;
            break;
        }
    }

    if( m_conn->SelectOne( part_id, result ) )
    {
        wxLogTrace( traceHTTPLib, wxT( "loadSymbol: SelectOne (%s) found in %s" ), part_id,
                    foundCategory->name );
    }
    else
    {
        wxLogTrace( traceHTTPLib, wxT( "loadSymbol: SelectOne (%s) failed for category %s" ),
                    part_id, foundCategory->name );

        THROW_IO_ERROR( m_lastError );
    }

    wxCHECK( foundCategory, nullptr );

    return loadSymbolFromPart( aAliasName, *foundCategory, result );
}


void SCH_HTTP_LIB_PLUGIN::GetSubLibraryNames( std::vector<wxString>& aNames )
{
     ensureSettings( wxEmptyString );

     aNames.clear();

     std::set<wxString> categoryNames;

     for( const HTTP_LIB_CATEGORY& categoryIter : m_conn->getCategories() )
     {
        if( categoryNames.count( categoryIter.name ) )
            continue;

        aNames.emplace_back( categoryIter.name );
        categoryNames.insert( categoryIter.name );
     }
}


void SCH_HTTP_LIB_PLUGIN::GetAvailableSymbolFields( std::vector<wxString>& aNames )
{
     // TODO: Implement this sometime; This is currently broken...
    std::copy( m_customFields.begin(), m_customFields.end(), std::back_inserter( aNames ) );
}


void SCH_HTTP_LIB_PLUGIN::GetDefaultSymbolFields( std::vector<wxString>& aNames )
{
    std::copy( m_defaultShownFields.begin(), m_defaultShownFields.end(),
               std::back_inserter( aNames ) );
}


void SCH_HTTP_LIB_PLUGIN::ensureSettings( const wxString& aSettingsPath )
{

    auto tryLoad = [&]()
    {
        if( !m_settings->LoadFromFile() )
        {
            wxString msg = wxString::Format( _( "HTTP library settings file %s missing or invalid" ),
                                             aSettingsPath );

            THROW_IO_ERROR( msg );
        }

        if( m_settings->m_Source.api_version.empty() )
        {
            wxString msg = wxString::Format(
                    _( "HTTP library settings file %s is missing the API version number!" ), aSettingsPath );

            THROW_IO_ERROR( msg );
        }

        if( m_settings->getSupportedAPIVersion() != m_settings->m_Source.api_version )
        {
            wxString msg = wxString::Format(
                    _( "HTTP library settings file %s indicates API version conflict (Settings file: %s <-> KiCad: %s)!" ),
                    aSettingsPath,
                    m_settings->m_Source.api_version,
                    m_settings->getSupportedAPIVersion() );

            THROW_IO_ERROR( msg );
        }

        if( m_settings->m_Source.root_url.empty() )
        {
            wxString msg = wxString::Format(
                    _( "HTTP library settings file %s is missing the root URL!" ),
                    aSettingsPath );

            THROW_IO_ERROR( msg );
        }

        // map lib source type
        m_settings->m_Source.type = m_settings->get_HTTP_LIB_SOURCE_TYPE();
        if( m_settings->m_Source.type == HTTP_LIB_SOURCE_TYPE::INVALID )
        {
            wxString msg = wxString::Format(
                    _( "HTTP library settings file has an invalid library type" ), aSettingsPath );

            THROW_IO_ERROR( msg );
        }

        // make sure that the root url finishes with a forward slash
        if( m_settings->m_Source.root_url.at( m_settings->m_Source.root_url.length() - 1 ) != '/' )
        {
            m_settings->m_Source.root_url += "/";
        }

        // Append api version to root URL
        m_settings->m_Source.root_url += m_settings->m_Source.api_version + "/";

    };

    if( !m_settings && !aSettingsPath.IsEmpty() )
    {
        std::string path( aSettingsPath.ToUTF8() );
        m_settings = std::make_unique<HTTP_LIB_SETTINGS>( path );

        m_settings->SetReadOnly( true );

        tryLoad();
    }
    else if( m_settings )
    {
        // If we have valid settings but no connection yet; reload settings in case user is editing
        tryLoad();
    }
    else if( !m_settings )
    {
        wxLogTrace( traceHTTPLib, wxT( "ensureSettings: no settings available!" ) );
    }
}


void SCH_HTTP_LIB_PLUGIN::ensureConnection()
{
    wxCHECK_RET( m_settings, "Call ensureSettings before ensureConnection!" );

    connect();

    if( !m_conn || !m_conn->isValidEndpoint() )
    {
        wxString msg = wxString::Format(
                _( "Could not connect to %s. Errors: %s" ),
                m_settings->m_Source.root_url, m_lastError );

        THROW_IO_ERROR( msg );
    }
}


void SCH_HTTP_LIB_PLUGIN::connect()
{
    wxCHECK_RET( m_settings, "Call ensureSettings before connect()!" );

    if( !m_conn )
    {

        m_conn = std::make_unique<HTTP_LIB_CONNECTION>( m_settings->m_Source, true );

        if( !m_conn->isValidEndpoint() )
        {
            m_lastError = m_conn->GetLastError();
            return;
        }

    }

}


LIB_SYMBOL* SCH_HTTP_LIB_PLUGIN::loadSymbolFromPart( const wxString&          aSymbolName,
                                                     const HTTP_LIB_CATEGORY& aCategory,
                                                     const HTTP_LIB_PART&     aPart )
{
    LIB_SYMBOL* symbol = nullptr;
    LIB_SYMBOL* originalSymbol = nullptr;
    LIB_ID      symbolId;

    std::string symbolIdStr = aPart.symbolIdStr;

    // Get or Create the symbol using the found symbol
    if( !symbolIdStr.empty() )
    {
        symbolId.Parse( symbolIdStr );

        if( symbolId.IsValid() )
        {
            originalSymbol = m_libTable->LoadSymbol( symbolId );
        }

        if( originalSymbol )
        {
            wxLogTrace( traceHTTPLib, wxT( "loadSymbolFromPart: found original symbol '%s'" ),
                        symbolIdStr );

            symbol = originalSymbol->Duplicate();
            symbol->SetSourceLibId( symbolId );

            symbol->LibId().SetSubLibraryName( aCategory.name );
        }
        else if( !symbolId.IsValid() )
        {
            wxLogTrace( traceHTTPLib,
                        wxT( "loadSymbolFromPart: source symbol id '%s' is invalid, "
                             "will create empty symbol" ),
                        symbolIdStr );
        }
        else
        {
            wxLogTrace( traceHTTPLib,
                        wxT( "loadSymbolFromPart: source symbol '%s' not found, "
                             "will create empty symbol" ),
                        symbolIdStr );
        }
    }

    if( !symbol )
    {
        // Actual symbol not found: return metadata only; error will be
        // indicated in the symbol chooser
        symbol = new LIB_SYMBOL( aSymbolName );
        symbol->LibId().SetSubLibraryName( aCategory.name );
    }

    LIB_FIELD* field;

    for( auto& _field : aPart.fields )
    {
        std::string fieldName = _field.first;
        std::tuple  fieldProperties = _field.second;

        if( fieldName == footprint_field )
        {
            field = &symbol->GetFootprintField();
            field->SetText( std::get<0>( fieldProperties ) );
            field->SetVisible( std::get<1>( fieldProperties ) );
        }
        else if( fieldName == description_field )
        {
            field = &symbol->GetDescriptionField();
            field->SetText( std::get<0>( fieldProperties ) );
            field->SetVisible( std::get<1>( fieldProperties ) );
        }
        else if( fieldName == value_field )
        {
            field = &symbol->GetValueField();
            field->SetText( std::get<0>( fieldProperties ) );
            field->SetVisible( std::get<1>( fieldProperties ) );
        }
        else if( fieldName == datasheet_field )
        {
            field = &symbol->GetDatasheetField();
            field->SetText( std::get<0>( fieldProperties ) );
            field->SetVisible( std::get<1>( fieldProperties ) );
        }
        else if( fieldName == reference_field )
        {
            field = &symbol->GetReferenceField();
            field->SetText( std::get<0>( fieldProperties ) );
            field->SetVisible( std::get<1>( fieldProperties ) );
        }
        else if( fieldName == keywords_field )
        {
            symbol->SetKeyWords( std::get<0>( fieldProperties ) );
        }
        else
        {
            // Generic fields
            field = new LIB_FIELD( symbol->GetNextAvailableFieldId() );
            field->SetName( fieldName );

            field->SetText( std::get<0>( fieldProperties ) );
            field->SetVisible( std::get<1>( fieldProperties ) );
            symbol->AddField( field );

            m_customFields.insert( fieldName );
        }
    }

    return symbol;
}


void SCH_HTTP_LIB_PLUGIN::SaveSymbol( const wxString& aLibraryPath, const LIB_SYMBOL* aSymbol,
                                   const STRING_UTF8_MAP* aProperties )
{
    // TODO: Implement this sometime;
}
