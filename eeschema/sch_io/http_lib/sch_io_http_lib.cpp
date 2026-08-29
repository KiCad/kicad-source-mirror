/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2023 Andre F. K. Iwers <iwers11@gmail.com>
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


#include <algorithm>
#include <chrono>
#include <string_view>

#include <bs_thread_pool.hpp>
#include <wx/log.h>
#include <wx/tokenzr.h>

#include <fmt.h>
#include <hash.h>
#include <lib_symbol.h>

#include <libraries/symbol_library_adapter.h>
#include <http_lib/http_lib_connection.h>
#include "sch_io_http_lib.h"
#include <ki_exception.h>


static bool fetchCategoryParts( HTTP_LIB_CONNECTION& aConn, const HTTP_LIB_CATEGORY& aCategory,
                                std::vector<HTTP_LIB_PART>& aParts )
{
    if( !aConn.SelectAll( aCategory, aParts ) )
        return false;

    for( HTTP_LIB_PART& part : aParts )
    {
        if( part.detailsLoaded )
            continue;

        if( HTTP_LIB_PART fullPart; aConn.SelectOne( part.id, fullPart ) )
        {
            fullPart.id = part.id;
            fullPart.name = part.name;
            part = std::move( fullPart );
        }
    }

    return true;
}


SCH_IO_HTTP_LIB::SCH_IO_HTTP_LIB() :
        SCH_IO( wxS( "HTTP library" ) ),
        m_adapter( nullptr )
{
}


void SCH_IO_HTTP_LIB::stopBackgroundRefresh()
{
    m_refreshRunning = false;
    m_refreshCV.notify_all();

    if( m_refreshThread.joinable() )
        m_refreshThread.join();
}


void SCH_IO_HTTP_LIB::startBackgroundRefresh()
{
    if( m_refreshRunning.exchange( true ) )
        return;

    wxLogTrace( traceHTTPLib, wxT( "Starting background refresh thread" ) );
    m_refreshThread = std::thread( &SCH_IO_HTTP_LIB::backgroundRefreshWorker, this );
}


void SCH_IO_HTTP_LIB::backgroundRefreshWorker()
{
    BS::this_thread::set_os_thread_name( "httplib bg" );

    while( m_refreshRunning.load() )
    {
        long long maxAge = 0;

        {
            std::shared_lock lock( m_cacheMutex );

            if( m_settings )
                maxAge = std::max( m_settings->m_Source.timeout_categories, m_settings->m_Source.timeout_parts );
        }

        if( maxAge <= 0 )
            maxAge = 1;

        // Hold a shared lock on m_cacheMutex while accessing m_conn so connect() (which takes a
        // unique lock to reset/replace m_conn) can't destroy the object out from under us.
        std::shared_lock connGuard( m_cacheMutex );

        if( m_conn && m_cachePopulated.load() )
        {
            wxLogTrace( traceHTTPLib, wxT( "Initiating background refresh" ) );

            try
            {
                std::map<std::string, HTTP_LIB_CATEGORY> categoryData;
                bool                                     fetchSuccess = true;

                for( const HTTP_LIB_CATEGORY& category : m_conn->getCategories() )
                {
                    std::vector<HTTP_LIB_PART> foundParts;

                    if( !fetchCategoryParts( *m_conn, category, foundParts ) )
                    {
                        wxLogTrace( traceHTTPLib, wxT( "Background refresh: fetch failed for category %s" ),
                                    category.name );
                        fetchSuccess = false;
                        break;
                    }

                    HTTP_LIB_CATEGORY cached = category;
                    cached.cachedParts = std::move( foundParts );
                    categoryData[category.id] = std::move( cached );
                }

                if( fetchSuccess )
                {
                    size_t signature = computeSignature( categoryData );
                    bool   dataChanged = false;

                    {
                        connGuard.unlock();
                        std::unique_lock lock( m_cacheMutex );

                        if( signature != m_cacheSignature )
                            dataChanged = true;
                    }

                    if( dataChanged )
                    {
                        wxLogTrace( traceHTTPLib, wxT( "Background refresh: new data" ) );

                        materializeCache( m_libraryPath, categoryData );

                        {
                            std::unique_lock lock( m_cacheMutex );
                            m_cacheSignature = signature;
                        }
                    }
                    else
                    {
                        wxLogTrace( traceHTTPLib, wxT( "Background refresh: no new data" ) );
                    }
                }
            }
            catch( const IO_ERROR& e )
            {
                wxLogTrace( traceHTTPLib, wxT( "Background refresh failed: %s" ), e.What() );
            }
            catch( const std::exception& e )
            {
                wxLogTrace( traceHTTPLib, wxT( "Background refresh failed: %s" ), e.what() );
            }
        }

        {
            std::unique_lock lock( m_refreshMutex );

            m_refreshCV.wait_for( lock, std::chrono::seconds( maxAge ),
                                  [this]()
                                  {
                                      return !m_refreshRunning.load();
                                  } );
        }
    }
}


size_t SCH_IO_HTTP_LIB::computeSignature( const std::map<std::string, HTTP_LIB_CATEGORY>& aCategoryData ) const
{
    size_t signature = 0;

    for( const auto& [catId, category] : aCategoryData )
    {
        hash_combine( signature, std::string_view( catId ) );
        hash_combine( signature, std::string_view( category.name ) );

        for( const HTTP_LIB_PART& part : category.cachedParts )
        {
            hash_combine( signature, std::string_view( part.id ) );
            hash_combine( signature, std::string_view( part.name ) );
            hash_combine( signature, std::string_view( part.symbolIdStr ) );
            hash_combine( signature, part.exclude_from_bom );
            hash_combine( signature, part.exclude_from_board );
            hash_combine( signature, part.exclude_from_sim );
            hash_combine( signature, std::string_view( part.desc ) );
            hash_combine( signature, std::string_view( part.keywords ) );

            for( const auto& [fieldName, fieldProps] : part.fields )
            {
                hash_combine( signature, std::string_view( fieldName ) );
                hash_combine( signature, std::string_view( std::get<0>( fieldProps ) ) );
                hash_combine( signature, std::get<1>( fieldProps ) );
            }

            for( const std::string& filter : part.fp_filters )
                hash_combine( signature, std::string_view( filter ) );

            if( !part.symbolIdStr.empty() )
            {
                LIB_ID symbolId;
                symbolId.Parse( part.symbolIdStr );

                if( symbolId.IsValid() && m_adapter )
                {
                    const UTF8& nickname = symbolId.GetLibNickname();
                    hash_combine( signature, std::string_view( nickname.c_str() ) );

                    if( std::optional<int> libHash = m_adapter->GetLibraryModifyHash( nickname ) )
                        hash_combine( signature, *libHash );
                }
            }
        }
    }

    return signature;
}


void SCH_IO_HTTP_LIB::materializeCache( const wxString& aLibraryPath,
                                        const std::map<std::string, HTTP_LIB_CATEGORY>& aCategoryData )
{
    std::map<wxString, std::unique_ptr<LIB_SYMBOL>>         newSymbolCache;
    std::map<wxString, std::pair<std::string, std::string>> newPartIdMap;
    std::set<wxString>                                      newCustomFields;

    for( const HTTP_LIB_CATEGORY& category : aCategoryData | std::views::values )
    {
        for( const HTTP_LIB_PART& part : category.cachedParts )
        {
            wxString symbolName( part.name );
            newPartIdMap[symbolName] = { part.id, category.id };

            LIB_SYMBOL* symbol = loadSymbolFromPart( aLibraryPath, symbolName, category, part, newCustomFields );

            if( symbol )
                newSymbolCache[symbolName] = std::unique_ptr<LIB_SYMBOL>( symbol );
        }
    }

    {
        std::unique_lock lock( m_cacheMutex );

        m_symbolCache = std::move( newSymbolCache );
        m_partIdMap = std::move( newPartIdMap );
        m_customFields = std::move( newCustomFields );

        m_cachePopulated = true;
        m_modifyHash++;
    }
}


void SCH_IO_HTTP_LIB::cacheLib( const wxString& aLibraryPath )
{
    if( m_inCacheLib )
        return;

    // After the initial load the background refresh thread handles all cache updates.
    {
        std::shared_lock lock( m_cacheMutex );

        if( m_cachePopulated )
            return;
    }

    m_inCacheLib = true;

    struct CACHE_LIB_GUARD
    {
        bool* flag;
        ~CACHE_LIB_GUARD() { *flag = false; }
    } cacheLibGuard{ &m_inCacheLib };

    m_libraryPath = aLibraryPath;

    std::map<std::string, HTTP_LIB_CATEGORY> categoryData;

    for( const HTTP_LIB_CATEGORY& category : m_conn->getCategories() )
    {
        std::vector<HTTP_LIB_PART> foundParts;

        if( !fetchCategoryParts( *m_conn, category, foundParts ) )
        {
            if( !m_conn->GetLastError().empty() )
            {
                THROW_IO_ERROR( wxString::Format( _( "Error retrieving data from HTTP library %s: %s" ), category.name,
                                                  m_conn->GetLastError() ) );
            }

            continue;
        }

        HTTP_LIB_CATEGORY cached = category;
        cached.cachedParts = std::move( foundParts );
        categoryData[category.id] = std::move( cached );
    }

    size_t signature = computeSignature( categoryData );

    {
        std::unique_lock lock( m_cacheMutex );

        if( m_cachePopulated && signature == m_cacheSignature )
            return;
    }

    materializeCache( aLibraryPath, categoryData );

    {
        std::unique_lock lock( m_cacheMutex );
        m_cacheSignature = signature;
    }

    if( !m_refreshRunning.load() )
        startBackgroundRefresh();
}


void SCH_IO_HTTP_LIB::EnumerateSymbolLib( wxArrayString& aSymbolNameList, const wxString& aLibraryPath,
                                          const std::map<std::string, UTF8>* aProperties )
{
    wxCHECK_RET( m_adapter, "HTTP plugin missing library manager adapter handle!" );
    ensureSettings( aLibraryPath );
    ensureConnection();

    if( !m_conn )
        THROW_IO_ERROR( m_lastError );

    cacheLib( aLibraryPath );

    bool powerSymbolsOnly = aProperties && aProperties->contains( SYMBOL_LIBRARY_ADAPTER::PropPowerSymsOnly );

    std::shared_lock lock( m_cacheMutex );

    for( const auto& [name, symbol] : m_symbolCache )
    {
        if( !powerSymbolsOnly || symbol->IsPower() )
            aSymbolNameList.Add( name );
    }
}


void SCH_IO_HTTP_LIB::EnumerateSymbolLib( std::vector<LIB_SYMBOL*>& aSymbolList, const wxString& aLibraryPath,
                                          const std::map<std::string, UTF8>* aProperties )
{
    wxCHECK_RET( m_adapter, "HTTP plugin missing library manager adapter handle!" );
    ensureSettings( aLibraryPath );
    ensureConnection();

    if( !m_conn )
        THROW_IO_ERROR( m_lastError );

    cacheLib( aLibraryPath );

    bool powerSymbolsOnly = aProperties && aProperties->contains( SYMBOL_LIBRARY_ADAPTER::PropPowerSymsOnly );

    std::shared_lock lock( m_cacheMutex );

    for( const std::unique_ptr<LIB_SYMBOL>& symbol : m_symbolCache | std::views::values )
    {
        if( !powerSymbolsOnly || symbol->IsPower() )
            aSymbolList.emplace_back( symbol->Duplicate() );
    }
}


void SCH_IO_HTTP_LIB::CheckLibrary( const wxString& aLibraryPath,
                                    const std::map<std::string, UTF8>* aProperties )
{
    ensureSettings( aLibraryPath );
    ensureConnection();
}


LIB_SYMBOL* SCH_IO_HTTP_LIB::LoadSymbol( const wxString& aLibraryPath, const wxString& aAliasName,
                                         const std::map<std::string, UTF8>* aProperties )
{
    wxCHECK_MSG( m_adapter, nullptr, "HTTP plugin missing library manager adapter handle!" );
    ensureSettings( aLibraryPath );
    ensureConnection();

    if( !m_conn )
        THROW_IO_ERROR( m_lastError );

    cacheLib( aLibraryPath );

    {
        std::shared_lock lock( m_cacheMutex );

        if( auto it = m_symbolCache.find( aAliasName ); it != m_symbolCache.end() )
            return it->second->Duplicate();
    }

    // Cache miss: fall back to a direct per-part fetch.  In the steady state every known part
    // is materialized, so this path only serves an uncached-but-known part.
    std::string partId;
    std::string categoryId;

    {
        std::shared_lock lock( m_cacheMutex );

        if( auto it = m_partIdMap.find( aAliasName ); it != m_partIdMap.end() )
        {
            partId = it->second.first;
            categoryId = it->second.second;
        }
    }

    if( partId.empty() )
    {
        wxLogTrace( traceHTTPLib, wxT( "LoadSymbol: no cached part found for %s" ), aAliasName );
        return nullptr;
    }

    const HTTP_LIB_CATEGORY* foundCategory = nullptr;

    for( const HTTP_LIB_CATEGORY& category : m_conn->getCategories() )
    {
        if( category.id == categoryId )
        {
            foundCategory = &category;
            break;
        }
    }

    if( !foundCategory )
    {
        wxLogTrace( traceHTTPLib, wxT( "LoadSymbol: no category found for %s" ), aAliasName );
        return nullptr;
    }

    HTTP_LIB_PART result;

    if( !m_conn->SelectOne( partId, result ) )
    {
        wxLogTrace( traceHTTPLib, wxT( "LoadSymbol: SelectOne (%s) failed for category %s" ),
                    partId, foundCategory->name );
        THROW_IO_ERROR( wxString::Format( _( "Error retrieving part %s from HTTP library: %s" ),
                                          partId, m_conn->GetLastError() ) );
    }

    wxLogTrace( traceHTTPLib, wxT( "LoadSymbol: SelectOne (%s) found in %s" ),
                partId, foundCategory->name );

    // This transient symbol is not placed in the shared cache; collect its custom fields into a
    // local set so the background materializer's m_customFields is never mutated off-thread.
    std::set<wxString> transientFields;

    return loadSymbolFromPart( aLibraryPath, aAliasName, *foundCategory, result, transientFields );
}


void SCH_IO_HTTP_LIB::GetSubLibraryNames( std::vector<wxString>& aNames )
{
    aNames.clear();

    ensureSettings( wxEmptyString );
    connect();

    // connect() leaves m_conn null when the endpoint is unreachable so a network loss
    // degrades to an empty result instead of a null dereference while building the tree.
    if( !m_conn )
        return;

    std::set<wxString> categoryNames;

    for( const HTTP_LIB_CATEGORY& categoryIter : m_conn->getCategories() )
    {
        if( categoryNames.count( categoryIter.name ) )
            continue;

        aNames.emplace_back( categoryIter.name );
        categoryNames.insert( categoryIter.name );
    }
}


wxString SCH_IO_HTTP_LIB::GetSubLibraryDescription( const wxString& aName )
{
    ensureSettings( wxEmptyString );
    connect();

    if( !m_conn )
        return wxEmptyString;

    return m_conn->getCategoryDescription( std::string( aName.mb_str() ) );
}


void SCH_IO_HTTP_LIB::GetAvailableSymbolFields( std::vector<wxString>& aNames )
{
    // TODO: Implement this sometime; This is currently broken...
    std::shared_lock lock( m_cacheMutex );
    std::copy( m_customFields.begin(), m_customFields.end(), std::back_inserter( aNames ) );
}


void SCH_IO_HTTP_LIB::GetDefaultSymbolFields( std::vector<wxString>& aNames )
{
    std::copy( m_defaultShownFields.begin(), m_defaultShownFields.end(), std::back_inserter( aNames ) );
}


void SCH_IO_HTTP_LIB::ensureSettings( const wxString& aSettingsPath )
{
    auto tryLoad =
            [&]()
            {
                if( !m_settings->LoadFromFile() )
                    THROW_IO_ERRORF( _( "HTTP library settings file %s missing or invalid." ), aSettingsPath );

                if( m_settings->m_Source.api_version.empty() )
                {
                    THROW_IO_ERRORF( _( "HTTP library settings file %s is missing the API version number." ),
                                     aSettingsPath );
                }

                if( m_settings->getSupportedAPIVersion() != m_settings->m_Source.api_version )
                {
                    THROW_IO_ERRORF( _( "HTTP library settings file %s uses API version %s, but KiCad requires "
                                        "version %s." ),
                                     aSettingsPath,
                                     m_settings->m_Source.api_version,
                                     m_settings->getSupportedAPIVersion() );
                }

                if( m_settings->m_Source.root_url.empty() )
                    THROW_IO_ERRORF( _( "HTTP library settings file %s is missing the root URL." ), aSettingsPath );

                // map lib source type
                m_settings->m_Source.type = m_settings->get_HTTP_LIB_SOURCE_TYPE();

                if( m_settings->m_Source.type == HTTP_LIB_SOURCE_TYPE::INVALID )
                    THROW_IO_ERRORF( _( "HTTP library settings file %s has invalid library type." ), aSettingsPath );

                // make sure that the root url finishes with a forward slash
                if( m_settings->m_Source.root_url.at( m_settings->m_Source.root_url.length() - 1 ) != '/' )
                    m_settings->m_Source.root_url += "/";

                // Append api version to root URL
                m_settings->m_Source.root_url += m_settings->m_Source.api_version + "/";

                if( m_sourcePatcher )
                    m_sourcePatcher( m_settings->m_Source );
            };

    if( !m_settings && !aSettingsPath.IsEmpty() )
    {
        std::string path( aSettingsPath.ToUTF8() );
        m_settings = std::make_unique<HTTP_LIB_SETTINGS>( path );

        m_settings->SetReadOnly( true );

        tryLoad();
    }
    else if( !m_conn && m_settings )
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
        THROW_IO_ERRORF( _( "Could not connect to %s. Errors: %s" ), m_settings->m_Source.root_url, m_lastError );
}


void SCH_IO_HTTP_LIB::connect()
{
    wxCHECK_RET( m_settings, "Call ensureSettings before connect()!" );

    {
        std::unique_lock connLock( m_cacheMutex );

        if( !m_conn )
        {
            if( m_connectionFactory )
                m_conn = m_connectionFactory( m_settings->m_Source );
            else
                m_conn = std::make_unique<HTTP_LIB_CONNECTION>( m_settings->m_Source, true );

            if( !m_conn->IsValidEndpoint() )
            {
                m_lastError = m_conn->GetLastError();

                // Make sure we release pointer so we are able to query API again next time
                m_conn.reset();
            }
        }
    }
}


LIB_SYMBOL* SCH_IO_HTTP_LIB::loadSymbolFromPart( const wxString& aLibraryPath,
                                                 const wxString& aSymbolName,
                                                 const HTTP_LIB_CATEGORY& aCategory,
                                                 const HTTP_LIB_PART& aPart,
                                                 std::set<wxString>& aCustomFields )
{
    LIB_SYMBOL* symbol = nullptr;
    LIB_SYMBOL* originalSymbol = nullptr;
    LIB_ID      symbolId;

    std::string symbolIdStr = aPart.symbolIdStr;

    // Extract library nickname from the library path (e.g., "/path/to/W5.kicad_httplib" -> "W5")
    wxFileName libFileName( aLibraryPath );
    wxString   libNickname = libFileName.GetName();

    // Get or Create the symbol using the found symbol
    if( !symbolIdStr.empty() )
    {
        symbolId.Parse( symbolIdStr );

        // A part's symbolIdStr may resolve back into this same HTTP library (issue #24249,
        // e.g. a mistyped library nickname).  The adapter routes that lookup back into
        // SCH_IO_HTTP_LIB::LoadSymbol, which would re-enter loadSymbolFromPart until the stack
        // overflows.  Track in-flight LIB_IDs and skip the recursive load on re-entry.
        struct CYCLE_GUARD
        {
            std::unordered_set<wxString>* set;
            wxString                      key;
            bool                          owns = false;

            ~CYCLE_GUARD()
            {
                if( owns )
                    set->erase( key );
            }
        } guard{ &m_inProgressLoads, {}, false };

        bool cycle = false;

        if( symbolId.IsValid() )
        {
            guard.key = symbolId.Format().wx_str();
            guard.owns = m_inProgressLoads.insert( guard.key ).second;
            cycle = !guard.owns;

            if( cycle )
            {
                wxLogTrace( traceHTTPLib,
                            wxT( "loadSymbolFromPart: cycle detected resolving '%s' "
                                 "(part '%s'); skipping recursive load" ),
                            symbolIdStr, aSymbolName );
            }
            else
            {
                originalSymbol = m_adapter->LoadSymbol( symbolId );
            }
        }

        if( originalSymbol )
        {
            wxLogTrace( traceHTTPLib, wxT( "loadSymbolFromPart: found original symbol '%s'" ),
                        symbolIdStr );

            symbol = originalSymbol->Duplicate();
            symbol->SetSourceLibId( symbolId );
            symbol->SetName( aSymbolName );

            LIB_ID libId = symbol->GetLibId();
            libId.SetLibNickname( libNickname );
            libId.SetSubLibraryName( aCategory.name );
            symbol->SetLibId( libId );
        }
        else if( cycle )
        {
            wxLogTrace( traceHTTPLib, wxT( "loadSymbolFromPart: source symbol '%s' is a "
                                           "self-reference, will create empty symbol" ),
                        symbolIdStr );
        }
        else if( !symbolId.IsValid() )
        {
            wxLogTrace( traceHTTPLib, wxT( "loadSymbolFromPart: source symbol id '%s' is invalid, "
                                           "will create empty symbol" ), symbolIdStr );
        }
        else
        {
            wxLogTrace( traceHTTPLib, wxT( "loadSymbolFromPart: source symbol '%s' not found, "
                                           "will create empty symbol" ), symbolIdStr );
        }
    }

    std::lock_guard lock( m_symbolLoadMutex );

    if( !symbol )
    {
        // Actual symbol not found: return metadata only; error will be
        // indicated in the symbol chooser
        symbol = new LIB_SYMBOL( aSymbolName );

        LIB_ID libId = symbol->GetLibId();
        libId.SetLibNickname( libNickname );
        libId.SetSubLibraryName( aCategory.name );
        symbol->SetLibId( libId );
    }

    symbol->SetExcludedFromBOM( aPart.exclude_from_bom );
    symbol->SetExcludedFromBoard( aPart.exclude_from_board );
    symbol->SetExcludedFromSim( aPart.exclude_from_sim );

    wxArrayString fp_filters;

    for( auto& [fieldName, fieldProperties] : aPart.fields )
    {
        wxString lowerFieldName = wxString( fieldName ).Lower();

        if( lowerFieldName == footprint_field )
        {
            SCH_FIELD*        field = &symbol->GetFootprintField();
            wxStringTokenizer tokenizer( std::get<0>( fieldProperties ), ";\t\r\n", wxTOKEN_STRTOK );

            while( tokenizer.HasMoreTokens() )
                fp_filters.Add( tokenizer.GetNextToken() );

            if( fp_filters.size() > 0 )
                field->SetText( fp_filters[0] );

            field->SetVisible( std::get<1>( fieldProperties ) );
        }
        else if( lowerFieldName == description_field )
        {
            SCH_FIELD* field = &symbol->GetDescriptionField();
            field->SetText( std::get<0>( fieldProperties ) );
            field->SetVisible( std::get<1>( fieldProperties ) );
        }
        else if( lowerFieldName == value_field )
        {
            SCH_FIELD* field = &symbol->GetValueField();
            field->SetText( std::get<0>( fieldProperties ) );
            field->SetVisible( std::get<1>( fieldProperties ) );
        }
        else if( lowerFieldName == datasheet_field )
        {
            SCH_FIELD* field = &symbol->GetDatasheetField();
            field->SetText( std::get<0>( fieldProperties ) );
            field->SetVisible( std::get<1>( fieldProperties ) );
        }
        else if( lowerFieldName == reference_field )
        {
            SCH_FIELD* field = &symbol->GetReferenceField();
            field->SetText( std::get<0>( fieldProperties ) );
            field->SetVisible( std::get<1>( fieldProperties ) );
        }
        else if( lowerFieldName == keywords_field )
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
            SCH_FIELD* field = symbol->GetField( fieldName );

            if( field != nullptr )
            {
                // adjust values accordingly
                field->SetText( std::get<0>( fieldProperties ) );
                field->SetVisible( std::get<1>( fieldProperties ) );
            }
            else
            {
                // Generic fields
                field = new SCH_FIELD( symbol, FIELD_T::USER );
                field->SetName( fieldName );

                field->SetText( std::get<0>( fieldProperties ) );
                field->SetVisible( std::get<1>( fieldProperties ) );
                symbol->AddField( field );

                aCustomFields.insert( fieldName );
            }
        }
    }

    symbol->SetDescription( aPart.desc );
    symbol->SetKeyWords( aPart.keywords );

    for( const std::string& filter : aPart.fp_filters )
        fp_filters.push_back( filter );

    symbol->SetFPFilters( fp_filters );

    // Pin-to-pad maps (issue #2282): attach non-destructively.  Prefer the spec-form named maps +
    // associations when the payload supplies them; otherwise fall back to the legacy flat form for
    // one release, bound to the symbol's concrete Footprint field (fp_filters may carry globs).
    if( !aPart.named_pin_maps.IsEmpty() || !aPart.associated_footprints.empty() )
    {
        symbol->SetPinMaps( aPart.named_pin_maps );
        symbol->SetAssociatedFootprints( aPart.associated_footprints );
    }
    else
    {
        const wxString assignedFootprint = symbol->GetFootprintField().GetText();

        if( !aPart.pin_map.empty() && !assignedFootprint.IsEmpty() )
        {
            const wxString mapName = wxS( "HTTP Library" );

            symbol->PinMaps().AddOrReplace( MakeLegacyPinMap( mapName, aPart.pin_map ) );

            LIB_ID fpId;
            fpId.Parse( assignedFootprint );
            symbol->SetAssociatedFootprints( { { fpId, mapName } } );
        }
    }

    return symbol;
}

void SCH_IO_HTTP_LIB::SaveSymbol( const wxString& aLibraryPath, const LIB_SYMBOL* aSymbol,
                                  const std::map<std::string, UTF8>* aProperties )
{
    // TODO: Implement this sometime;
}
