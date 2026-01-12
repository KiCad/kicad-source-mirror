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

#include <footprint_library_adapter.h>

#include <env_vars.h>
#include <footprint_info_impl.h>
#include <thread_pool.h>
#include <trace_helpers.h>
#include <footprint.h>

#include <magic_enum.hpp>
#include <wx/hash.h>
#include <wx/log.h>


std::map<wxString, LIB_DATA> FOOTPRINT_LIBRARY_ADAPTER::GlobalLibraries;

std::mutex FOOTPRINT_LIBRARY_ADAPTER::GlobalLibraryMutex;


FOOTPRINT_LIBRARY_ADAPTER::FOOTPRINT_LIBRARY_ADAPTER( LIBRARY_MANAGER& aManager ) :
        LIBRARY_MANAGER_ADAPTER( aManager )
{
}


wxString FOOTPRINT_LIBRARY_ADAPTER::GlobalPathEnvVariableName()
{
    return ENV_VAR::GetVersionedEnvVarName( wxS( "FOOTPRINT_DIR" ) );
}


void FOOTPRINT_LIBRARY_ADAPTER::AsyncLoad()
{
    std::unique_lock<std::mutex> asyncLock( m_loadMutex, std::try_to_lock );

    if( !asyncLock )
        return;

    // TODO(JE) this shares most code with the symbol adapter
    // TODO(JE) any reason to clean these up earlier?
    std::erase_if( m_futures,
                   []( const std::future<void>& aFuture ) { return aFuture.valid(); } );

    if( !m_futures.empty() )
    {
        wxLogTrace( traceLibraries, "FP: Cannot AsyncLoad, futures from a previous call remain!" );
        return;
    }

    std::vector<LIBRARY_TABLE_ROW*> rows = m_manager.Rows( LIBRARY_TABLE_TYPE::FOOTPRINT );

    m_loadTotal = rows.size();
    m_loadCount.store( 0 );

    if( m_loadTotal == 0 )
    {
        wxLogTrace( traceLibraries, "FP: AsyncLoad: no libraries left to load; exiting" );
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

    for( const LIBRARY_TABLE_ROW* row : rows )
    {
        wxString nickname = row->Nickname();
        LIBRARY_TABLE_SCOPE scope = row->Scope();

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
                    LIB_DATA* lib = *result;
                    wxArrayString dummyList;
                    std::lock_guard lock ( lib->mutex );
                    lib->status.load_status = LOAD_STATUS::LOADING;

                    std::map<std::string, UTF8> options = lib->row->GetOptionsMap();

                    try
                    {
                        pcbplugin( lib )->FootprintEnumerate( dummyList, getUri( lib->row ), false, &options );
                        wxLogTrace( traceLibraries, "FP: %s: library enumerated %zu items", nickname, dummyList.size() );
                        lib->status.load_status = LOAD_STATUS::LOADED;
                    }
                    catch( IO_ERROR& e )
                    {
                        lib->status.load_status = LOAD_STATUS::LOAD_ERROR;
                        lib->status.error = LIBRARY_ERROR( { e.What() } );
                        wxLogTrace( traceLibraries, "FP: %s: plugin threw exception: %s", nickname, e.What() );
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
                        wxLogTrace( traceLibraries, "FP: project library error: %s: %s", nickname, result.error().message );
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

    wxLogTrace( traceLibraries, "FP: Started async load of %zu libraries", m_loadTotal );
}


/// Loads or reloads the given library, if it exists
std::optional<LIB_STATUS> FOOTPRINT_LIBRARY_ADAPTER::LoadOne( LIB_DATA* aLib )
{
    std::lock_guard lock ( aLib->mutex );
    aLib->status.load_status = LOAD_STATUS::LOADING;

    std::map<std::string, UTF8> options = aLib->row->GetOptionsMap();

    try
    {
        wxArrayString dummyList;
        pcbplugin( aLib )->FootprintEnumerate( dummyList, getUri( aLib->row ), false, &options );
        wxLogTrace( traceLibraries, "Sym: %s: library enumerated %zu items", aLib->row->Nickname(), dummyList.size() );
        aLib->status.load_status = LOAD_STATUS::LOADED;
    }
    catch( IO_ERROR& e )
    {
        aLib->status.load_status = LOAD_STATUS::LOAD_ERROR;
        aLib->status.error = LIBRARY_ERROR( { e.What() } );
        wxLogTrace( traceLibraries, "Sym: %s: plugin threw exception: %s", aLib->row->Nickname(), e.What() );
    }

    return aLib->status;
}


std::optional<LIB_STATUS> FOOTPRINT_LIBRARY_ADAPTER::LoadOne( const wxString& nickname )
{
    LIBRARY_RESULT<LIB_DATA*> result = loadIfNeeded( nickname );

    if( result.has_value() )
        return LoadOne( *result );

    return LIB_STATUS{
        .load_status = LOAD_STATUS::LOAD_ERROR,
        .error = LIBRARY_ERROR( { result.error() } )
    };
}


std::optional<LIB_STATUS> FOOTPRINT_LIBRARY_ADAPTER::GetLibraryStatus( const wxString& aNickname ) const
{
    // TODO(JE) this could be deduplicated with virtual access to GlobalLibraries
    if( m_libraries.contains( aNickname ) )
        return m_libraries.at( aNickname ).status;

    if( GlobalLibraries.contains( aNickname ) )
        return GlobalLibraries.at( aNickname ).status;

    return std::nullopt;
}


std::vector<FOOTPRINT*> FOOTPRINT_LIBRARY_ADAPTER::GetFootprints( const wxString& aNickname, bool aBestEfforts )
{
    std::vector<FOOTPRINT*> footprints;

    std::optional<const LIB_DATA*> maybeLib = fetchIfLoaded( aNickname );

    if( !maybeLib )
        return footprints;

    const LIB_DATA* lib = *maybeLib;
    std::map<std::string, UTF8> options = lib->row->GetOptionsMap();

    wxArrayString namesAS;

    try
    {
        pcbplugin( lib )->FootprintEnumerate( namesAS, getUri( lib->row ), true, &options );
    }
    catch( IO_ERROR& e )
    {
        wxLogTrace( traceLibraries, "FP: Exception enumerating library %s: %s",
                    lib->row->Nickname(), e.What() );
    }

    for( const wxString& footprintName : namesAS )
    {
        FOOTPRINT* footprint = nullptr;

        try
        {
            footprint = pcbplugin( lib )->FootprintLoad( getUri( lib->row ),footprintName, false, &options );
        }
        catch( IO_ERROR& e )
        {
            wxLogTrace( traceLibraries, "Sym: Exception enumerating library %s: %s",
                        lib->row->Nickname(), e.What() );
            continue;
        }

        wxCHECK2( footprint, continue );

        LIB_ID id = footprint->GetFPID();
        id.SetLibNickname( lib->row->Nickname() );
        footprint->SetFPID( id );
        footprints.emplace_back( footprint );
    }

    return footprints;
}


std::vector<wxString> FOOTPRINT_LIBRARY_ADAPTER::GetFootprintNames( const wxString& aNickname, bool aBestEfforts )
{
    // TODO(JE) can we kill wxArrayString in internal API?
    wxArrayString namesAS;
    std::vector<wxString> names;

    if( std::optional<const LIB_DATA*> maybeLib = fetchIfLoaded( aNickname ) )
    {
        const LIB_DATA* lib = *maybeLib;
        std::map<std::string, UTF8> options = lib->row->GetOptionsMap();

        try
        {
            pcbplugin( lib )->FootprintEnumerate( namesAS, getUri( lib->row ), true, &options );
        }
        catch( IO_ERROR& e )
        {
            wxLogTrace( traceLibraries, "FP: Exception enumerating library %s: %s",
                        lib->row->Nickname(), e.What() );
        }
    }

    for( const wxString& name : namesAS )
        names.emplace_back( name );

    return names;
}


long long FOOTPRINT_LIBRARY_ADAPTER::GenerateTimestamp( const wxString* aNickname )
{
    long long hash = 0;

    if( aNickname )
    {
        wxCHECK( HasLibrary( *aNickname, true ), hash );

        if( std::optional<const LIB_DATA*> r = fetchIfLoaded( *aNickname ); r.has_value() )
        {
            PCB_IO* plugin = dynamic_cast<PCB_IO*>( ( *r )->plugin.get() );
            wxCHECK( plugin, hash );
            return plugin->GetLibraryTimestamp( LIBRARY_MANAGER::GetFullURI( ( *r )->row, true ) ) +
                    wxHashTable::MakeKey( *aNickname );
        }
    }

    for( const wxString& nickname : GetLibraryNames() )
    {
        if( std::optional<const LIB_DATA*> r = fetchIfLoaded( nickname ); r.has_value() )
        {
            // Note: can't use dynamic_cast across compile units on Mac
            wxCHECK2( ( *r )->plugin->IsPCB_IO(), continue );
            PCB_IO* plugin = static_cast<PCB_IO*>( ( *r )->plugin.get() );
            hash += plugin->GetLibraryTimestamp( LIBRARY_MANAGER::GetFullURI( ( *r )->row, true ) ) +
                    wxHashTable::MakeKey( nickname );
        }
    }

    return hash;
}


bool FOOTPRINT_LIBRARY_ADAPTER::FootprintExists( const wxString& aNickname, const wxString& aName )
{
    if( std::optional<const LIB_DATA*> maybeLib = fetchIfLoaded( aNickname ) )
    {
        const LIB_DATA* lib = *maybeLib;
        std::map<std::string, UTF8> options = lib->row->GetOptionsMap();

        return pcbplugin( lib )->FootprintExists( getUri( lib->row ), aName, &options );
    }

    return false;
}


FOOTPRINT* FOOTPRINT_LIBRARY_ADAPTER::LoadFootprint( const wxString& aNickname, const wxString& aName, bool aKeepUUID )
{
    if( std::optional<const LIB_DATA*> lib = fetchIfLoaded( aNickname ) )
    {
        try
        {
            if( FOOTPRINT* footprint = pcbplugin( *lib )->FootprintLoad( getUri( ( *lib )->row ), aName ) )
            {
                LIB_ID id = footprint->GetFPID();
                id.SetLibNickname( ( *lib )->row->Nickname() );
                footprint->SetFPID( id );
                return footprint;
            }
        }
        catch( const IO_ERROR& ioe )
        {
            wxLogTrace( traceLibraries, "LoadFootprint: error loading %s:%s: %s",
                        aNickname, aName, ioe.What() );
        }
    }
    else
    {
        wxLogTrace( traceLibraries, "LoadFootprint: requested library %s not loaded", aNickname );
    }

    return nullptr;
}


FOOTPRINT* FOOTPRINT_LIBRARY_ADAPTER::LoadFootprintWithOptionalNickname( const LIB_ID& aFootprintId, bool aKeepUUID )
{
    wxString nickname = aFootprintId.GetLibNickname();
    wxString footprintName = aFootprintId.GetLibItemName();

    if( nickname.size() )
        return LoadFootprint( nickname, footprintName, aKeepUUID );

    // nickname is empty, sequentially search (alphabetically) all libs/nicks for first match:
    for( const wxString& library : GetLibraryNames() )
    {
        // FootprintLoad() returns NULL on not found, does not throw exception
        // unless there's an IO_ERROR.
        if( FOOTPRINT* ret = LoadFootprint( library, footprintName, aKeepUUID ) )
            return ret;
    }

    return nullptr;
}


FOOTPRINT_LIBRARY_ADAPTER::SAVE_T FOOTPRINT_LIBRARY_ADAPTER::SaveFootprint( const wxString& aNickname,
                                                                            const FOOTPRINT* aFootprint,
                                                                            bool aOverwrite )
{
    wxCHECK( aFootprint, SAVE_SKIPPED );

    if( std::optional<const LIB_DATA*> lib = fetchIfLoaded( aNickname ) )
    {
        if( !aOverwrite )
        {
            wxString fpname = aFootprint->GetFPID().GetLibItemName();

            try
            {
                FOOTPRINT* existing = pcbplugin( *lib )->FootprintLoad( getUri( ( *lib )->row ), fpname, false );

                if( existing )
                {
                    delete existing;
                    return SAVE_SKIPPED;
                }
            }
            catch( IO_ERROR& e )
            {
                wxLogTrace( traceLibraries, "SaveFootprint: error checking for existing footprint %s: %s",
                            aFootprint->GetFPIDAsString(), e.What() );
                return SAVE_SKIPPED;
            }
        }

        try
        {
            pcbplugin( *lib )->FootprintSave( getUri( ( *lib )->row ), aFootprint );
        }
        catch( IO_ERROR& e )
        {
            wxLogTrace( traceLibraries, "SaveFootprint: error saving %s: %s",
                        aFootprint->GetFPIDAsString(), e.What() );
            return SAVE_SKIPPED;
        }

        return SAVE_OK;
    }
    else
    {
        wxLogTrace( traceLibraries, "SaveFootprint: requested library %s not loaded", aNickname );
        return SAVE_SKIPPED;
    }
}

void FOOTPRINT_LIBRARY_ADAPTER::DeleteFootprint( const wxString& aNickname, const wxString& aFootprintName )
{
    if( std::optional<const LIB_DATA*> lib = fetchIfLoaded( aNickname ) )
    {
        try
        {
            pcbplugin( *lib )->FootprintDelete( getUri( ( *lib )->row ), aFootprintName );
        }
        catch( IO_ERROR& e )
        {
            wxLogTrace( traceLibraries, "DeleteFootprint: error deleting %s:%s: %s", aNickname,
                        aFootprintName, e.What() );
        }
    }
    else
    {
        wxLogTrace( traceLibraries, "DeleteFootprint: requested library %s not loaded", aNickname );
    }
}


bool FOOTPRINT_LIBRARY_ADAPTER::IsFootprintLibWritable( const wxString& aLib )
{
    if( m_libraries.contains( aLib ) )
        return m_libraries[aLib].plugin->IsLibraryWritable( getUri( m_libraries[aLib].row ) );

    if( GlobalLibraries.contains( aLib ) )
        return GlobalLibraries[aLib].plugin->IsLibraryWritable( getUri( GlobalLibraries[aLib].row ) );

    return false;
}


LIBRARY_RESULT<IO_BASE*> FOOTPRINT_LIBRARY_ADAPTER::createPlugin( const LIBRARY_TABLE_ROW* row )
{
    PCB_IO_MGR::PCB_FILE_T type = PCB_IO_MGR::EnumFromStr( row->Type() );

    if( type == PCB_IO_MGR::PCB_FILE_UNKNOWN )
    {
        wxLogTrace( traceLibraries, "FP: Plugin type %s is unknown!", row->Type() );
        wxString msg = wxString::Format( _( "Unknown library type %s " ), row->Type() );
        return tl::unexpected( LIBRARY_ERROR( msg ) );
    }

    PCB_IO* plugin = PCB_IO_MGR::FindPlugin( type );
    wxCHECK( plugin, tl::unexpected( LIBRARY_ERROR( _( "Internal error" ) ) ) );

    wxLogTrace( traceLibraries, "FP: Library %s (%s) plugin created",
                 row->Nickname(), magic_enum::enum_name( row->Scope() ) );

    return plugin;
}


PCB_IO* FOOTPRINT_LIBRARY_ADAPTER::pcbplugin( const LIB_DATA* aRow )
{
    // Note: can't use dynamic_cast across compile units on Mac
    wxCHECK( aRow->plugin->IsPCB_IO(), nullptr );
    PCB_IO* ret = static_cast<PCB_IO*>( aRow->plugin.get() );
    return ret;
}
