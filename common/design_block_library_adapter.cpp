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


#include "design_block.h"


#include <design_block_library_adapter.h>
#include <design_block_io.h>
#include <env_vars.h>
#include <ki_exception.h>
#include <thread_pool.h>
#include <trace_helpers.h>

#include <set>
#include <magic_enum.hpp>
#include <wx/log.h>


std::map<wxString, LIB_DATA> DESIGN_BLOCK_LIBRARY_ADAPTER::GlobalLibraries;

std::mutex DESIGN_BLOCK_LIBRARY_ADAPTER::GlobalLibraryMutex;


DESIGN_BLOCK_LIBRARY_ADAPTER::DESIGN_BLOCK_LIBRARY_ADAPTER( LIBRARY_MANAGER& aManager ) :
        LIBRARY_MANAGER_ADAPTER( aManager )
{
}


wxString DESIGN_BLOCK_LIBRARY_ADAPTER::GlobalPathEnvVariableName()
{
    return ENV_VAR::GetVersionedEnvVarName( wxS( "DESIGN_BLOCK_DIR" ) );
}


DESIGN_BLOCK_IO* DESIGN_BLOCK_LIBRARY_ADAPTER::dbplugin( const LIB_DATA* aRow )
{
    DESIGN_BLOCK_IO* ret = dynamic_cast<DESIGN_BLOCK_IO*>( aRow->plugin.get() );
    wxCHECK( aRow->plugin && ret, nullptr );
    return ret;
}


LIBRARY_RESULT<IO_BASE*> DESIGN_BLOCK_LIBRARY_ADAPTER::createPlugin( const LIBRARY_TABLE_ROW* row )
{
    // TODO(JE) do we maintain a precondition that URIs are absolute paths after expansion?
    DESIGN_BLOCK_IO_MGR::DESIGN_BLOCK_FILE_T type = DESIGN_BLOCK_IO_MGR::EnumFromStr( row->Type() );

    if( type == DESIGN_BLOCK_IO_MGR::DESIGN_BLOCK_FILE_UNKNOWN )
    {
        wxLogTrace( traceLibraries, "Sym: Plugin type %s is unknown!", row->Type() );
        wxString msg = wxString::Format( _( "Unknown library type %s " ), row->Type() );
        return tl::unexpected( LIBRARY_ERROR( msg ) );
    }

    DESIGN_BLOCK_IO* plugin = DESIGN_BLOCK_IO_MGR::FindPlugin( type );
    wxCHECK( plugin, tl::unexpected( LIBRARY_ERROR( _( "Internal error" ) ) ) );

    return plugin;
}


IO_BASE* DESIGN_BLOCK_LIBRARY_ADAPTER::plugin( const LIB_DATA* aRow )
{
    return dbplugin( aRow );
}


/// Loads or reloads the given library, if it exists
std::optional<LIB_STATUS> DESIGN_BLOCK_LIBRARY_ADAPTER::LoadOne( LIB_DATA* aLib )
{
    wxArrayString dummyList;
    std::lock_guard lock ( aLib->mutex );
    aLib->status.load_status = LOAD_STATUS::LOADING;

    std::map<std::string, UTF8> options = aLib->row->GetOptionsMap();

    try
    {
        dbplugin( aLib )->DesignBlockEnumerate( dummyList, getUri( aLib->row ), false, &options );
        wxLogTrace( traceLibraries, "DB: %s: library enumerated %zu items", aLib->row->Nickname(), dummyList.size() );
        aLib->status.load_status = LOAD_STATUS::LOADED;
    }
    catch( IO_ERROR& e )
    {
        aLib->status.load_status = LOAD_STATUS::LOAD_ERROR;
        aLib->status.error = LIBRARY_ERROR( { e.What() } );
        wxLogTrace( traceLibraries, "DB: %s: plugin threw exception: %s", aLib->row->Nickname(), e.What() );
    }

    return aLib->status;
}


std::optional<LIB_STATUS> DESIGN_BLOCK_LIBRARY_ADAPTER::LoadOne( const wxString& nickname )
{
    LIBRARY_RESULT<LIB_DATA*> result = loadIfNeeded( nickname );

    if( result.has_value() )
        return LoadOne( *result );

    return LIB_STATUS{
        .load_status = LOAD_STATUS::LOAD_ERROR,
        .error = LIBRARY_ERROR( { result.error() } )
    };
}


void DESIGN_BLOCK_LIBRARY_ADAPTER::AsyncLoad()
{
    // TODO(JE) library tables - how much of this can be shared with other library types?
    // TODO(JE) any reason to clean these up earlier?
    std::erase_if( m_futures,
                   []( const std::future<void>& aFuture ) { return aFuture.valid(); } );

    if( !m_futures.empty() )
    {
        wxLogTrace( traceLibraries, "DB: Cannot AsyncLoad, futures from a previous call remain!" );
        return;
    }

    std::vector<LIBRARY_TABLE_ROW*> rows = m_manager.Rows( LIBRARY_TABLE_TYPE::DESIGN_BLOCK );

    m_loadTotal = rows.size();
    m_loadCount.store( 0 );

    if( m_loadTotal == 0 )
    {
        wxLogTrace( traceLibraries, "DB: AsyncLoad: no libraries left to load; exiting" );
        return;
    }

    thread_pool& tp = GetKiCadThreadPool();

    auto check =
            []( const wxString& aLib, std::map<wxString, LIB_DATA>& aMap, std::mutex& aMutex )
            {
                std::lock_guard lock( aMutex );

                if( aMap.contains( aLib ) )
                {
                    if( aMap[aLib].status.load_status == LOAD_STATUS::LOADED )
                        return true;

                    aMap.erase( aLib );
                }

                return false;
            };

    std::set<wxString> libNamesCurrentlyValid;

    for( const LIBRARY_TABLE_ROW* row : rows )
    {
        LIBRARY_TABLE_SCOPE scope = row->Scope();
        const wxString& nickname = row->Nickname();

        libNamesCurrentlyValid.insert( nickname );

        // TODO(JE) library tables -- check for modified global files
        if( check( nickname, m_libraries, m_libraries_mutex ) )
        {
            --m_loadTotal;
            continue;
        }

        if( check( nickname, GlobalLibraries, GlobalLibraryMutex ) )
        {
            --m_loadTotal;
            continue;
        }

        m_futures.emplace_back( tp.submit_task(
                [this, nickname, scope]()
                {
                    if( m_abort.load() )
                        return;

                    LIBRARY_RESULT<LIB_DATA*> result = loadIfNeeded( nickname );

                    if( result.has_value() )
                    {
                        std::optional<LIB_STATUS> loadResult = LoadOne( *result );

                        // for design blocks, LoadOne should always return something
                        wxCHECK2( loadResult, ++m_loadCount; return );

                        switch( scope )
                        {
                        case LIBRARY_TABLE_SCOPE::GLOBAL:
                        {
                            std::lock_guard lock( GlobalLibraryMutex );
                            GlobalLibraries[nickname].status = *loadResult;
                            break;
                        }

                        case LIBRARY_TABLE_SCOPE::PROJECT:
                        {
                            std::lock_guard lock( m_libraries_mutex );
                            m_libraries[nickname].status = *loadResult;
                            break;
                        }

                        default:
                            wxFAIL_MSG( "Unexpected library table scope" );
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
                                .load_status = LOAD_STATUS::LOAD_ERROR,
                                .error = result.error()
                            } );

                            break;
                        }

                        case LIBRARY_TABLE_SCOPE::PROJECT:
                        {
                            wxLogTrace( traceLibraries, "DB: project library error: %s: %s", nickname, result.error().message );
                            std::lock_guard lock( m_libraries_mutex );

                            m_libraries[nickname].status = LIB_STATUS( {
                                .load_status = LOAD_STATUS::LOAD_ERROR,
                                .error = result.error()
                            } );

                            break;
                        }

                        default:
                            wxFAIL_MSG( "Unexpected library table scope" );
                        }
                    }

                    ++m_loadCount;
                }, BS::pr::lowest ) );
    }

    // Cleanup libraries that were removed from the table
    {
        std::lock_guard lock( GlobalLibraryMutex );
        std::erase_if( GlobalLibraries, [&]( const auto& pair )
                {
                    return !libNamesCurrentlyValid.contains( pair.first );
                } );
    }

    {
        std::lock_guard lock( m_libraries_mutex );
        std::erase_if( m_libraries, [&]( const auto& pair )
                {
                    return !libNamesCurrentlyValid.contains( pair.first );
                } );
    }

    if( m_loadTotal > 0 )
        wxLogTrace( traceLibraries, "DB: Started async load of %zu libraries", m_loadTotal );
}


std::vector<DESIGN_BLOCK*> DESIGN_BLOCK_LIBRARY_ADAPTER::GetDesignBlocks( const wxString& aNickname )
{
    std::vector<DESIGN_BLOCK*> blocks;

    std::optional<const LIB_DATA*> maybeLib = fetchIfLoaded( aNickname );

    if( !maybeLib )
        return blocks;

    const LIB_DATA* lib = *maybeLib;
    std::map<std::string, UTF8> options = lib->row->GetOptionsMap();
    wxArrayString blockNames;

    try
    {
        dbplugin( lib )->DesignBlockEnumerate( blockNames, getUri( lib->row ), false, &options );
    }
    catch( IO_ERROR& e )
    {
        wxLogTrace( traceLibraries, "DB: Exception enumerating library %s: %s", lib->row->Nickname(), e.What() );
    }

    for( const wxString& blockName : blockNames )
    {
        try
        {
            blocks.emplace_back( dbplugin( lib )->DesignBlockLoad( getUri( lib->row ), blockName, false, &options ) );
        }
        catch( IO_ERROR& e )
        {
            wxLogTrace( traceLibraries, "DB: Exception enumerating design block %s: %s", blockName, e.What() );
        }
    }

    return blocks;
}


std::vector<wxString> DESIGN_BLOCK_LIBRARY_ADAPTER::GetDesignBlockNames( const wxString& aNickname )
{
    // TODO(JE) can we kill wxArrayString in internal API?
    wxArrayString namesAS;
    std::vector<wxString> names;

    if( std::optional<const LIB_DATA*> maybeLib = fetchIfLoaded( aNickname ) )
    {
        const LIB_DATA* lib = *maybeLib;
        std::map<std::string, UTF8> options = lib->row->GetOptionsMap();

        dbplugin( lib )->DesignBlockEnumerate( namesAS, getUri( lib->row ), true, &options );
    }

    for( const wxString& name : namesAS )
        names.emplace_back( name );

    return names;
}


DESIGN_BLOCK* DESIGN_BLOCK_LIBRARY_ADAPTER::LoadDesignBlock( const wxString& aNickname,
                                                             const wxString& aDesignBlockName, bool aKeepUUID )
{
    if( std::optional<const LIB_DATA*> maybeLib = fetchIfLoaded( aNickname ) )
    {
        const LIB_DATA* lib = *maybeLib;
        std::map<std::string, UTF8> options = lib->row->GetOptionsMap();

        DESIGN_BLOCK* db = dbplugin( lib )->DesignBlockLoad( getUri( lib->row ), aDesignBlockName, aKeepUUID, &options );
        db->GetLibId().SetLibNickname( aNickname );
        return db;
    }

    return nullptr;
}


bool DESIGN_BLOCK_LIBRARY_ADAPTER::DesignBlockExists( const wxString& aNickname, const wxString& aDesignBlockName )
{
    if( std::optional<const LIB_DATA*> maybeLib = fetchIfLoaded( aNickname ) )
    {
        const LIB_DATA* lib = *maybeLib;
        std::map<std::string, UTF8> options = lib->row->GetOptionsMap();

        return dbplugin( lib )->DesignBlockExists( getUri( lib->row ), aDesignBlockName, &options );
    }

    return false;
}


const DESIGN_BLOCK* DESIGN_BLOCK_LIBRARY_ADAPTER::GetEnumeratedDesignBlock( const wxString& aNickname,
        const wxString& aDesignBlockName )
{
    if( std::optional<const LIB_DATA*> maybeLib = fetchIfLoaded( aNickname ) )
    {
        const LIB_DATA* lib = *maybeLib;
        std::map<std::string, UTF8> options = lib->row->GetOptionsMap();

        return dbplugin( lib )->GetEnumeratedDesignBlock( getUri( lib->row ), aDesignBlockName, &options );
    }

    return nullptr;
}


DESIGN_BLOCK_LIBRARY_ADAPTER::SAVE_T DESIGN_BLOCK_LIBRARY_ADAPTER::SaveDesignBlock( const wxString& aNickname,
                                                                                    const DESIGN_BLOCK* aDesignBlock,
                                                                                    bool aOverwrite )
{
    if( std::optional<const LIB_DATA*> maybeLib = fetchIfLoaded( aNickname ) )
    {
        const LIB_DATA* lib = *maybeLib;
        std::map<std::string, UTF8> options = lib->row->GetOptionsMap();

        if( !aOverwrite && dbplugin( lib )->DesignBlockExists( getUri( lib->row ), aDesignBlock->GetName(), &options ) )
            return SAVE_SKIPPED;

        dbplugin( lib )->DesignBlockSave( getUri( lib->row ), aDesignBlock, &options );
    }

    return SAVE_OK;
}


void DESIGN_BLOCK_LIBRARY_ADAPTER::DeleteDesignBlock( const wxString& aNickname,
                                                      const wxString& aDesignBlockName )
{
    if( std::optional<const LIB_DATA*> maybeLib = fetchIfLoaded( aNickname ) )
    {
        const LIB_DATA* lib = *maybeLib;
        std::map<std::string, UTF8> options = lib->row->GetOptionsMap();
        return dbplugin( lib )->DesignBlockDelete( getUri( lib->row ), aDesignBlockName, &options );
    }
}


bool DESIGN_BLOCK_LIBRARY_ADAPTER::IsDesignBlockLibWritable( const wxString& aNickname )
{
    if( std::optional<const LIB_DATA*> maybeLib = fetchIfLoaded( aNickname ) )
    {
        const LIB_DATA* lib = *maybeLib;
        return plugin( lib )->IsLibraryWritable( getUri( lib->row ) );
    }

    return false;
}


DESIGN_BLOCK* DESIGN_BLOCK_LIBRARY_ADAPTER::DesignBlockLoadWithOptionalNickname( const LIB_ID& aDesignBlockId,
        bool aKeepUUID )
{
    wxString nickname = aDesignBlockId.GetLibNickname();
    wxString DesignBlockname = aDesignBlockId.GetLibItemName();

    if( nickname.size() )
        return LoadDesignBlock( nickname, DesignBlockname, aKeepUUID );

    // nickname is empty, sequentially search (alphabetically) all libs/nicks for first match:
    for( const wxString& library : GetLibraryNames() )
    {
        // DesignBlockLoad() returns NULL on not found, does not throw exception
        // unless there's an IO_ERROR.
        if( DESIGN_BLOCK* ret = LoadDesignBlock( library, DesignBlockname, aKeepUUID ) )
            return ret;
    }

    return nullptr;
}

