/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2023 Andre F. K. Iwers <iwers11@gmail.com>
 * Copyright (C) 2024 KiCad Developers, see AUTHORS.txt for contributors.
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
#include "sch_io_http_lib.h"


SCH_IO_HTTP_LIB::SCH_IO_HTTP_LIB() : SCH_IO( wxS( "HTTP library" ) ),
    m_libTable( nullptr )
{
}


SCH_IO_HTTP_LIB::~SCH_IO_HTTP_LIB()
{
}


void SCH_IO_HTTP_LIB::EnumerateSymbolLib( wxArrayString&         aSymbolNameList,
                                          const wxString&        aLibraryPath,
                                          const STRING_UTF8_MAP* aProperties )
{
    std::vector<LIB_SYMBOL*> symbols;
    EnumerateSymbolLib( symbols, aLibraryPath, aProperties );

    for( LIB_SYMBOL* symbol : symbols )
        aSymbolNameList.Add( symbol->GetName() );
}


void SCH_IO_HTTP_LIB::EnumerateSymbolLib( std::vector<LIB_SYMBOL*>& aSymbolList,
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

    for(const HTTP_LIB_CATEGORY& category : m_conn->getCategories() )
    {
        bool refresh_cache = true;

        std::vector<HTTP_LIB_PART> found_parts;

         // Check if there is already a part in our cache, if not fetch it
        if( m_cachedCategories.find( category.id ) != m_cachedCategories.end() )
        {
            // check if it's outdated, if so re-fetch
            if( std::difftime( std::time( nullptr ), m_cachedCategories[category.id].lastCached )
                < m_settings->m_Source.timeout_categories )
            {
                refresh_cache = false;
            }
        }

        if( refresh_cache )
        {
            if( !m_conn->SelectAll( category, found_parts ) )
            {
                if( !m_conn->GetLastError().empty() )
                {
                    wxString msg =
                            wxString::Format( _( "Error retriving data from HTTP library %s: %s" ),
                                              category.name, m_conn->GetLastError() );
                    THROW_IO_ERROR( msg );
                }

                continue;
            }

            // remove cached parts
            m_cachedCategories[category.id].cachedParts.clear();

            // Copy newly cached data across
            m_cachedCategories[category.id].cachedParts = found_parts;
            m_cachedCategories[category.id].lastCached = std::time( nullptr );

        }

        for( const HTTP_LIB_PART& part : m_cachedCategories[category.id].cachedParts )
        {
            wxString libIDString( part.name );

            LIB_SYMBOL* symbol = loadSymbolFromPart( libIDString, category, part );

            if( symbol && ( !powerSymbolsOnly || symbol->IsPower() ) )
                aSymbolList.emplace_back( symbol );

        }
    }
}


LIB_SYMBOL* SCH_IO_HTTP_LIB::LoadSymbol( const wxString&        aLibraryPath,
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
    if( foundCategory == nullptr )
    {
        wxLogTrace( traceHTTPLib, wxT( "loadSymbol: no category found for %s" ), partName );
        return nullptr;
    }

    // get the matching query ID
    for( const HTTP_LIB_PART& part : m_cachedCategories[foundCategory->id].cachedParts )
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


void SCH_IO_HTTP_LIB::GetSubLibraryNames( std::vector<wxString>& aNames )
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


void SCH_IO_HTTP_LIB::GetAvailableSymbolFields( std::vector<wxString>& aNames )
{
     // TODO: Implement this sometime; This is currently broken...
    std::copy( m_customFields.begin(), m_customFields.end(), std::back_inserter( aNames ) );
}


void SCH_IO_HTTP_LIB::GetDefaultSymbolFields( std::vector<wxString>& aNames )
{
    std::copy( m_defaultShownFields.begin(), m_defaultShownFields.end(),
               std::back_inserter( aNames ) );
}


void SCH_IO_HTTP_LIB::ensureSettings( const wxString& aSettingsPath )
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
                    _( "HTTP library settings file %s is missing the API version number!" ),
                    aSettingsPath );

            THROW_IO_ERROR( msg );
        }

        if( m_settings->getSupportedAPIVersion() != m_settings->m_Source.api_version )
        {
            wxString msg = wxString::Format(
                    _( "HTTP library settings file %s uses API version %s, but KiCad requires version %s" ),
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
                    _( "HTTP library settings file %s has an invalid library type" ),
                    aSettingsPath );

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


void SCH_IO_HTTP_LIB::ensureConnection()
{
    wxCHECK_RET( m_settings, "Call ensureSettings before ensureConnection!" );

    connect();

    if( !m_conn || !m_conn->IsValidEndpoint() )
    {
        wxString msg = wxString::Format(
                _( "Could not connect to %s. Errors: %s" ),
                m_settings->m_Source.root_url, m_lastError );

        THROW_IO_ERROR( msg );
    }
}


void SCH_IO_HTTP_LIB::connect()
{
    wxCHECK_RET( m_settings, "Call ensureSettings before connect()!" );

    if( !m_conn )
    {
        m_conn = std::make_unique<HTTP_LIB_CONNECTION>( m_settings->m_Source, true );

        if( !m_conn->IsValidEndpoint() )
        {
            m_lastError = m_conn->GetLastError();

            // Make sure we release pointer so we are able to query API again next time
            m_conn.reset();

            return;
        }
    }
}


LIB_SYMBOL* SCH_IO_HTTP_LIB::loadSymbolFromPart( const wxString&          aSymbolName,
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

            LIB_ID libId = symbol->GetLibId();
            libId.SetSubLibraryName( aCategory.name );
            symbol->SetLibId( libId );
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

        LIB_ID libId = symbol->GetLibId();
        libId.SetSubLibraryName( aCategory.name );
        symbol->SetLibId( libId );
    }

    symbol->SetExcludedFromBOM( aPart.exclude_from_bom );
    symbol->SetExcludedFromBoard( aPart.exclude_from_board );
    symbol->SetExcludedFromSim( aPart.exclude_from_sim );

    SCH_FIELD* field;

    for( auto& _field : aPart.fields )
    {
        wxString   fieldName = wxString( _field.first );
        std::tuple fieldProperties = _field.second;

        if( fieldName.Lower() == footprint_field )
        {
            field = &symbol->GetFootprintField();
            field->SetText( std::get<0>( fieldProperties ) );
            field->SetVisible( std::get<1>( fieldProperties ) );
        }
        else if( fieldName.Lower() == description_field )
        {
            field = &symbol->GetDescriptionField();
            field->SetText( std::get<0>( fieldProperties ) );
            field->SetVisible( std::get<1>( fieldProperties ) );
        }
        else if( fieldName.Lower() == value_field )
        {
            field = &symbol->GetValueField();
            field->SetText( std::get<0>( fieldProperties ) );
            field->SetVisible( std::get<1>( fieldProperties ) );
        }
        else if( fieldName.Lower() == datasheet_field )
        {
            field = &symbol->GetDatasheetField();
            field->SetText( std::get<0>( fieldProperties ) );
            field->SetVisible( std::get<1>( fieldProperties ) );
        }
        else if( fieldName.Lower() == reference_field )
        {
            field = &symbol->GetReferenceField();
            field->SetText( std::get<0>( fieldProperties ) );
            field->SetVisible( std::get<1>( fieldProperties ) );
        }
        else if( fieldName.Lower() == keywords_field )
        {
            symbol->SetKeyWords( std::get<0>( fieldProperties ) );
        }
        else
        {
            // Check if field exists, if so replace Text and adjust visiblity.
            //
            // This proves useful in situations where, for instance, an individual requires a particular value, such as
            // the material type showcased at a specific position for a capacitor. Subsequently, this value could be defined
            // in the symbol itself and then, potentially, be modified by the HTTP library as necessary.
            field = symbol->FindField( fieldName );

            if( field != nullptr )
            {
                // adjust values accordingly
                field->SetText( std::get<0>( fieldProperties ) );
                field->SetVisible( std::get<1>( fieldProperties ) );
            }
            else
            {
                // Generic fields
                field = new SCH_FIELD( nullptr, symbol->GetNextAvailableFieldId() );
                field->SetName( fieldName );

                field->SetText( std::get<0>( fieldProperties ) );
                field->SetVisible( std::get<1>( fieldProperties ) );
                symbol->AddField( field );

                m_customFields.insert( fieldName );
            }

        }
    }

    return symbol;
}

void SCH_IO_HTTP_LIB::SaveSymbol( const wxString& aLibraryPath, const LIB_SYMBOL* aSymbol,
                                  const STRING_UTF8_MAP* aProperties )
{
    // TODO: Implement this sometime;
}
