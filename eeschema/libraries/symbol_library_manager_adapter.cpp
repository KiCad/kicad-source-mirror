/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 * @author Jon Evans <jon@craftyjon.com>
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

#include <common.h>
#include <ki_exception.h>
#include <wx/log.h>

#include <lib_symbol.h>
#include <libraries/symbol_library_manager_adapter.h>
#include <pgm_base.h>
#include <thread_pool.h>
#include <trace_helpers.h>
#include <settings/settings_manager.h>


std::map<wxString, LIB_DATA> SYMBOL_LIBRARY_MANAGER_ADAPTER::GlobalLibraries;

std::mutex SYMBOL_LIBRARY_MANAGER_ADAPTER::GlobalLibraryMutex;


SYMBOL_LIBRARY_MANAGER_ADAPTER::SYMBOL_LIBRARY_MANAGER_ADAPTER( LIBRARY_MANAGER& aManager,
                                                                PROJECT& aProject ) :
            LIBRARY_MANAGER_ADAPTER( aManager, aProject )
{
}


LIBRARY_RESULT<LIB_DATA*> SYMBOL_LIBRARY_MANAGER_ADAPTER::loadIfNeeded( const wxString& aNickname )
{
    auto tryLoadFromScope =
        [&]( LIBRARY_TABLE_SCOPE aScope, std::map<wxString, LIB_DATA>& aTarget,
             std::mutex& aMutex ) -> LIBRARY_RESULT<LIB_DATA*>
        {
            bool present = false;

            {
                std::lock_guard lock( aMutex );
                present = aTarget.contains( aNickname );
            }

            if( !present )
            {
                if( auto result = m_manager.GetRow( LIBRARY_TABLE_TYPE::SYMBOL, aNickname, aScope) )
                {
                    const LIBRARY_TABLE_ROW* row = *result;
                    wxLogTrace( traceLibraries, "Library %s (%s) not yet loaded, will attempt...",
                                aNickname, magic_enum::enum_name( aScope ) );

                    // TODO(JE) do we maintain a precondition that URIs are absolute paths after expansion?
                    SCH_IO_MGR::SCH_FILE_T type = SCH_IO_MGR::EnumFromStr( row->Type() );

                    if( type == SCH_IO_MGR::SCH_FILE_UNKNOWN )
                    {
                        wxLogTrace( traceLibraries, "Plugin type %s is unknown!", row->Type() );
                        wxString msg =
                            wxString::Format( _( "Unknown library type %s " ), row->Type() );
                        return tl::unexpected( LIBRARY_ERROR( msg ) );
                    }

                    SCH_IO* plugin = SCH_IO_MGR::FindPlugin( type );
                    wxCHECK( plugin, tl::unexpected( LIBRARY_ERROR( _( "Internal error" ) ) ) );

                    plugin->SetLibraryManagerAdapter( this );

                    std::lock_guard lock( aMutex );

                    aTarget[ row->Nickname() ].row = row;
                    aTarget[ row->Nickname() ].plugin.reset( plugin );

                    wxLogTrace( traceLibraries, "Library %s (%s) loaded",
                                aNickname, magic_enum::enum_name( aScope ) );

                    return &aTarget.at( aNickname );
                }

                return nullptr;
            }

            return &aTarget.at( aNickname );
        };

    LIBRARY_RESULT<LIB_DATA*> result =
            tryLoadFromScope( LIBRARY_TABLE_SCOPE::PROJECT, m_libraries, m_libraries_mutex );

    if( !result.has_value() || *result )
        return result;

    result = tryLoadFromScope( LIBRARY_TABLE_SCOPE::GLOBAL, GlobalLibraries, GlobalLibraryMutex );

    if( !result.has_value() || *result )
        return result;

    wxString msg = wxString::Format( _( "Library %s not found" ), aNickname );
    return tl::unexpected( LIBRARY_ERROR( msg ) );
}


std::optional<const LIB_DATA*> SYMBOL_LIBRARY_MANAGER_ADAPTER::fetchIfLoaded(
        const wxString& aNickname ) const
{
    if( m_libraries.contains( aNickname ) )
        return &m_libraries.at( aNickname );

    if( GlobalLibraries.contains( aNickname ) )
        return &GlobalLibraries.at( aNickname );

    return std::nullopt;
}


wxString SYMBOL_LIBRARY_MANAGER_ADAPTER::getUri( const LIBRARY_TABLE_ROW* aRow )
{
    wxFileName path( ExpandEnvVarSubstitutions( aRow->URI(), &m_project ) );
    path.MakeAbsolute();
    return path.GetFullPath();
}


std::vector<LIB_SYMBOL*> SYMBOL_LIBRARY_MANAGER_ADAPTER::GetSymbols( const wxString& aNickname )
{
    std::vector<LIB_SYMBOL*> symbols;

    LIBRARY_RESULT<const LIB_DATA*> maybeLib = loadIfNeeded( aNickname );

    if( !maybeLib )
        return symbols;

    const LIB_DATA* lib = *maybeLib;
    std::map<std::string, UTF8> options = lib->row->GetOptionsMap();

    try
    {
        lib->plugin->EnumerateSymbolLib( symbols, getUri( lib->row ),
                                         &options );
    }
    catch( IO_ERROR& e )
    {
        wxLogTrace( traceLibraries, "Exception enumerating library %s: %s",
                    lib->row->Nickname(), e.What() );
    }

    for( LIB_SYMBOL* symbol : symbols )
    {
        LIB_ID id = symbol->GetLibId();
        id.SetLibNickname( lib->row->Nickname() );
        symbol->SetLibId( id );
    }

    return symbols;
}


std::vector<wxString> SYMBOL_LIBRARY_MANAGER_ADAPTER::GetSymbolNames( const wxString& aNickname )
{
    // TODO(JE) can we kill wxArrayString in internal API?
    wxArrayString namesAS;
    std::vector<wxString> names;

    if( LIBRARY_RESULT<const LIB_DATA*> lib = loadIfNeeded( aNickname ) )
        ( *lib )->plugin->EnumerateSymbolLib( namesAS, getUri( ( *lib )->row ) );

    for( const wxString& name : namesAS )
        names.emplace_back( name );

    return names;
}


LIB_SYMBOL* SYMBOL_LIBRARY_MANAGER_ADAPTER::LoadSymbol( const wxString& aNickname,
                                                        const wxString& aName )
{
    if( LIBRARY_RESULT<const LIB_DATA*> lib = loadIfNeeded( aNickname ) )
    {
        if( LIB_SYMBOL* symbol = ( *lib )->plugin->LoadSymbol( getUri( ( *lib )->row ), aName ) )
        {
            LIB_ID id = symbol->GetLibId();
            id.SetLibNickname( ( *lib )->row->Nickname() );
            symbol->SetLibId( id );
            return symbol;
        }
    }
    else
    {
        wxLogTrace( traceLibraries, "LoadSymbol: requested library %s not loaded", aNickname );
    }

    return nullptr;
}


SYMBOL_LIBRARY_MANAGER_ADAPTER::SAVE_T SYMBOL_LIBRARY_MANAGER_ADAPTER::SaveSymbol(
    const wxString& aNickname, const LIB_SYMBOL* aSymbol, bool aOverwrite )
{
    wxCHECK_MSG( false, SAVE_SKIPPED, "Unimplemented!" );
}


void SYMBOL_LIBRARY_MANAGER_ADAPTER::DeleteSymbol( const wxString& aNickname,
                                                   const wxString& aSymbolName )
{
    wxCHECK_MSG( false, /* void */, "Unimplemented!" );
}


bool SYMBOL_LIBRARY_MANAGER_ADAPTER::IsSymbolLibWritable( const wxString& aLib )
{
    if( m_libraries.contains( aLib ) )
        return m_libraries[aLib].plugin->IsLibraryWritable( getUri( m_libraries[aLib].row ) );

    if( GlobalLibraries.contains( aLib ) )
        return GlobalLibraries[aLib].plugin->IsLibraryWritable( getUri( GlobalLibraries[aLib].row ) );

    return false;
}


bool SYMBOL_LIBRARY_MANAGER_ADAPTER::IsLibraryLoaded( const wxString& aNickname )
{
    // TODO: row->IsOK() doesn't actually tell you if a library is loaded
    // Once we are preloading libraries we can cache the status of plugin load instead

    if( m_libraries.contains( aNickname ) )
        return m_libraries[aNickname].row->IsOk();

    if( GlobalLibraries.contains( aNickname ) )
        return GlobalLibraries[aNickname].row->IsOk();

    return false;
}


void SYMBOL_LIBRARY_MANAGER_ADAPTER::AsyncLoad()
{
    // TODO(JE) any reason to clean these up earlier?
    std::erase_if( m_futures,
                   []( const std::future<void>& aFuture ) { return aFuture.valid(); } );

    if( !m_futures.empty() )
    {
        wxLogTrace( traceLibraries, "Cannot AsyncLoad, futures from a previous call remain!" );
        return;
    }

    std::vector<const LIBRARY_TABLE_ROW*> rows = m_manager.Rows( LIBRARY_TABLE_TYPE::SYMBOL );

    m_loadTotal = rows.size();
    m_loadCount.store( 0 );

    thread_pool& tp = GetKiCadThreadPool();

    for( const LIBRARY_TABLE_ROW* row : rows )
    {
        wxString nickname = row->Nickname();
        LIBRARY_TABLE_SCOPE scope = row->Scope();

        m_futures.emplace_back( tp.submit_task(
            [this, nickname, scope]()
            {
                LIBRARY_RESULT<LIB_DATA*> result = loadIfNeeded( nickname );

                if( result.has_value() )
                {
                    LIB_DATA* lib = *result;
                    wxArrayString dummyList;
                    std::lock_guard lock ( lib->mutex );
                    lib->status.load_status = LOAD_STATUS::LOADING;

                    std::map<std::string, UTF8> options = lib->row->GetOptionsMap();

                    try
                    {
                        lib->plugin->EnumerateSymbolLib( dummyList, getUri( lib->row ),
                                                         &options );
                        //std::this_thread::sleep_for( std::chrono::milliseconds( 1000 ) );
                        lib->status.load_status = LOAD_STATUS::LOADED;
                    }
                    catch( IO_ERROR& e )
                    {
                        lib->status.load_status = LOAD_STATUS::ERROR;
                        lib->status.error = LIBRARY_ERROR( { e.What() } );
                    }
                }
                else
                {
                    switch( scope )
                    {
                    case LIBRARY_TABLE_SCOPE::GLOBAL:
                    {
                        std::lock_guard lock( GlobalLibraryMutex );

                        GlobalLibraries[nickname].status = LIB_STATUS( {
                            .load_status = LOAD_STATUS::ERROR,
                            .error = result.error()
                        } );

                        break;
                    }

                    case LIBRARY_TABLE_SCOPE::PROJECT:
                    {
                        std::lock_guard lock( m_libraries_mutex );

                        m_libraries[nickname].status = LIB_STATUS( {
                            .load_status = LOAD_STATUS::ERROR,
                            .error = result.error()
                        } );

                        break;
                    }

                    default:
                        wxFAIL_MSG( "Unexpected library table scope" );
                    }
                }

                ++m_loadCount;
            } ) );
    }

    wxLogTrace( traceLibraries, "Started async load of %zu libraries", m_loadTotal );
}


std::optional<float> SYMBOL_LIBRARY_MANAGER_ADAPTER::AsyncLoadProgress() const
{
    if( m_loadTotal == 0 )
        return std::nullopt;

    size_t loaded = m_loadCount.load();
    return loaded / static_cast<float>( m_loadTotal );
}


void SYMBOL_LIBRARY_MANAGER_ADAPTER::BlockUntilLoaded() const
{
    for( const std::future<void>& future : m_futures )
        future.wait();
}


std::optional<LIBRARY_ERROR> SYMBOL_LIBRARY_MANAGER_ADAPTER::LibraryError(
        const wxString& aNickname ) const
{
    if( std::optional<const LIB_DATA*> result = fetchIfLoaded( aNickname ); auto row = *result )
    {
        if( row->status.error )
            return row->status.error;
    }

    return std::nullopt;
}


std::vector<std::pair<wxString, LIB_STATUS>> SYMBOL_LIBRARY_MANAGER_ADAPTER::GetLibraryStatus() const
{
    std::vector<std::pair<wxString, LIB_STATUS>> ret;

    for( const LIBRARY_TABLE_ROW* row : m_manager.Rows( LIBRARY_TABLE_TYPE::SYMBOL ) )
    {
        if( std::optional<const LIB_DATA*> result = fetchIfLoaded( row->Nickname() );
            const LIB_DATA* rowData = *result )
        {
            ret.emplace_back( std::make_pair( row->Nickname(), rowData->status ) );
        }
        else
        {
            // This should probably never happen, but until that can be proved...
            ret.emplace_back( std::make_pair( row->Nickname(), LIB_STATUS( {
                    .load_status = LOAD_STATUS::ERROR,
                    .error = LIBRARY_ERROR( _( "Library not found in library table" ) )
                } ) ) );
        }
    }

    return ret;
}


int SYMBOL_LIBRARY_MANAGER_ADAPTER::GetModifyHash() const
{
    int hash = 0;

    for( const LIBRARY_TABLE_ROW* row : m_manager.Rows( LIBRARY_TABLE_TYPE::SYMBOL ) )
    {
        if( std::optional<const LIB_DATA*> result = fetchIfLoaded( row->Nickname() );
            const LIB_DATA* rowData = *result )
        {
            hash += rowData->plugin->GetModifyHash();
        }
    }

    return hash;
}
