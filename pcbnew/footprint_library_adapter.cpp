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

#include <chrono>
#include <env_vars.h>
#include <footprint_info_impl.h>
#include <thread_pool.h>
#include <trace_helpers.h>
#include <footprint.h>

#include <magic_enum.hpp>
#include <wx/hash.h>
#include <wx/log.h>

using namespace std::chrono_literals;


LEAK_AT_EXIT<std::map<wxString, LIB_DATA>> FOOTPRINT_LIBRARY_ADAPTER::GlobalLibraries;

std::shared_mutex FOOTPRINT_LIBRARY_ADAPTER::GlobalLibraryMutex;

LEAK_AT_EXIT<std::map<wxString, std::vector<std::unique_ptr<FOOTPRINT>>>> FOOTPRINT_LIBRARY_ADAPTER::PreloadedFootprints;

std::shared_mutex FOOTPRINT_LIBRARY_ADAPTER::PreloadedFootprintsMutex;


FOOTPRINT_LIBRARY_ADAPTER::FOOTPRINT_LIBRARY_ADAPTER( LIBRARY_MANAGER& aManager ) :
        LIBRARY_MANAGER_ADAPTER( aManager )
{
}


wxString FOOTPRINT_LIBRARY_ADAPTER::GlobalPathEnvVariableName()
{
    return ENV_VAR::GetVersionedEnvVarName( wxS( "FOOTPRINT_DIR" ) );
}


void FOOTPRINT_LIBRARY_ADAPTER::enumerateLibrary( LIB_DATA* aLib )
{
    wxArrayString namesAS;
    std::map<std::string, UTF8> options = aLib->row->GetOptionsMap();
    PCB_IO* plugin = pcbplugin( aLib );
    wxString uri = getUri( aLib->row );
    wxString nickname = aLib->row->Nickname();

    // FootprintEnumerate populates the plugin's internal FP_CACHE with parsed footprints
    plugin->FootprintEnumerate( namesAS, uri, false, &options );

    std::vector<std::unique_ptr<FOOTPRINT>> footprints;
    footprints.reserve( namesAS.size() );

    // For plugins with internal caches (like kicad_sexpr), GetEnumeratedFootprint returns
    // a borrowed pointer and ClearCachedFootprints handles cleanup. For other plugins,
    // GetEnumeratedFootprint allocates new memory that we must delete after cloning.
    const bool pluginCaches = plugin->CachesEnumeratedFootprints();

    for( const wxString& footprintName : namesAS )
    {
        try
        {
            const FOOTPRINT* cached = plugin->GetEnumeratedFootprint( uri, footprintName, &options );

            if( !cached )
                continue;

            FOOTPRINT* footprint = static_cast<FOOTPRINT*>( cached->Duplicate( IGNORE_PARENT_GROUP ) );
            footprint->SetParent( nullptr );

            // For non-caching plugins, delete the allocated footprint now that we've cloned it
            if( !pluginCaches )
                delete cached;

            LIB_ID id = footprint->GetFPID();
            id.SetLibNickname( nickname );
            footprint->SetFPID( id );
            footprints.emplace_back( footprint );
        }
        catch( IO_ERROR& e )
        {
            wxLogTrace( traceLibraries, "FP: %s:%s enumeration error: %s",
                        nickname, footprintName, e.What() );
        }
    }

    {
        std::unique_lock lock( PreloadedFootprintsMutex );
        PreloadedFootprints.Get()[nickname] = std::move( footprints );
    }

    // Clear the plugin's FP_CACHE now that we've copied footprints to PreloadedFootprints.
    // This eliminates the double-caching that was consuming ~1.2GB of extra RAM.
    plugin->ClearCachedFootprints( uri );
}


std::optional<LIB_STATUS> FOOTPRINT_LIBRARY_ADAPTER::LoadOne( LIB_DATA* aLib )
{
    aLib->status.load_status = LOAD_STATUS::LOADING;

    std::map<std::string, UTF8> options = aLib->row->GetOptionsMap();

    try
    {
        wxArrayString dummyList;
        pcbplugin( aLib )->FootprintEnumerate( dummyList, getUri( aLib->row ), false, &options );
        aLib->status.load_status = LOAD_STATUS::LOADED;
    }
    catch( IO_ERROR& e )
    {
        aLib->status.load_status = LOAD_STATUS::LOAD_ERROR;
        aLib->status.error = LIBRARY_ERROR( { e.What() } );
        wxLogTrace( traceLibraries, "FP: %s: plugin threw exception: %s", aLib->row->Nickname(), e.What() );
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


std::vector<FOOTPRINT*> FOOTPRINT_LIBRARY_ADAPTER::GetFootprints( const wxString& aNickname, bool aBestEfforts )
{
    std::vector<FOOTPRINT*> footprints;

    std::shared_lock lock( PreloadedFootprintsMutex );
    auto it = PreloadedFootprints.Get().find( aNickname );

    if( it == PreloadedFootprints.Get().end() )
        return footprints;

    footprints.reserve( it->second.size() );

    for( const auto& fp : it->second )
        footprints.push_back( fp.get() );

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
            wxLogTrace( traceLibraries, "FP: Exception enumerating library %s: %s", lib->row->Nickname(), e.What() );
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
            return plugin->GetLibraryTimestamp( LIBRARY_MANAGER::GetFullURI( ( *r )->row, true ) )
                    + wxHashTable::MakeKey( *aNickname );
        }
    }

    for( const wxString& nickname : GetLibraryNames() )
    {
        if( std::optional<const LIB_DATA*> r = fetchIfLoaded( nickname ); r.has_value() )
        {
            wxCHECK2( ( *r )->plugin->IsPCB_IO(), continue );
            PCB_IO* plugin = static_cast<PCB_IO*>( ( *r )->plugin.get() );
            hash += plugin->GetLibraryTimestamp( LIBRARY_MANAGER::GetFullURI( ( *r )->row, true ) )
                        + wxHashTable::MakeKey( nickname );
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
    // First check if the footprint is in PreloadedFootprints and clone from there.
    // This avoids re-parsing the file and keeps FP_CACHE from being repopulated.
    {
        std::shared_lock lock( PreloadedFootprintsMutex );
        auto libIt = PreloadedFootprints.Get().find( aNickname );

        if( libIt != PreloadedFootprints.Get().end() )
        {
            for( const auto& fp : libIt->second )
            {
                if( fp->GetFPID().GetLibItemName() == UTF8( aName ) )
                {
                    FOOTPRINT* copy;

                    if( aKeepUUID )
                        copy = static_cast<FOOTPRINT*>( fp->Clone() );
                    else
                        copy = static_cast<FOOTPRINT*>( fp->Duplicate( IGNORE_PARENT_GROUP ) );

                    copy->SetParent( nullptr );
                    return copy;
                }
            }
        }
    }

    // Footprint not found in PreloadedFootprints, fall back to plugin.
    // This re-parses the file but is needed for footprints not yet enumerated.
    if( std::optional<const LIB_DATA*> lib = fetchIfLoaded( aNickname ) )
    {
        try
        {
            if( FOOTPRINT* footprint = pcbplugin( *lib )->FootprintLoad( getUri( ( *lib )->row ), aName, aKeepUUID ) )
            {
                LIB_ID id = footprint->GetFPID();
                id.SetLibNickname( ( *lib )->row->Nickname() );
                footprint->SetFPID( id );
                return footprint;
            }
        }
        catch( const IO_ERROR& ioe )
        {
            wxLogTrace( traceLibraries, "LoadFootprint: error loading %s:%s: %s", aNickname, aName, ioe.What() );
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

        {
            std::unique_lock lock( PreloadedFootprintsMutex );
            auto             it = PreloadedFootprints.Get().find( aNickname );

            if( it != PreloadedFootprints.Get().end() )
            {
                wxString fpName = aFootprint->GetFPID().GetLibItemName();

                if( aOverwrite )
                {
                    auto& footprints = it->second;
                    footprints.erase( std::remove_if( footprints.begin(), footprints.end(),
                                                      [&fpName]( const std::unique_ptr<FOOTPRINT>& fp )
                                                      {
                                                          return fp->GetFPID().GetLibItemName().wx_str() == fpName;
                                                      } ),
                                      footprints.end() );
                }

                FOOTPRINT* clone = static_cast<FOOTPRINT*>( aFootprint->Duplicate( IGNORE_PARENT_GROUP ) );
                clone->SetParent( nullptr );

                LIB_ID id = clone->GetFPID();
                id.SetLibNickname( aNickname );
                clone->SetFPID( id );

                it->second.emplace_back( clone );
            }
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
            return;
        }

        {
            std::unique_lock lock( PreloadedFootprintsMutex );
            auto             it = PreloadedFootprints.Get().find( aNickname );

            if( it != PreloadedFootprints.Get().end() )
            {
                auto& footprints = it->second;
                footprints.erase( std::remove_if( footprints.begin(), footprints.end(),
                                                  [&aFootprintName]( const std::unique_ptr<FOOTPRINT>& fp )
                                                  {
                                                      return fp->GetFPID().GetLibItemName().wx_str() == aFootprintName;
                                                  } ),
                                  footprints.end() );
            }
        }
    }
    else
    {
        wxLogTrace( traceLibraries, "DeleteFootprint: requested library %s not loaded", aNickname );
    }
}


bool FOOTPRINT_LIBRARY_ADAPTER::IsFootprintLibWritable( const wxString& aLib )
{
    {
        std::shared_lock lock( m_librariesMutex );

        if( auto it = m_libraries.find( aLib ); it != m_libraries.end() )
            return it->second.plugin->IsLibraryWritable( getUri( it->second.row ) );
    }

    {
        std::shared_lock lock( GlobalLibraryMutex );

        if( auto it = GlobalLibraries.Get().find( aLib ); it != GlobalLibraries.Get().end() )
            return it->second.plugin->IsLibraryWritable( getUri( it->second.row ) );
    }

    return false;
}


LIBRARY_RESULT<IO_BASE*> FOOTPRINT_LIBRARY_ADAPTER::createPlugin( const LIBRARY_TABLE_ROW* row )
{
    PCB_IO_MGR::PCB_FILE_T type = PCB_IO_MGR::EnumFromStr( row->Type() );

    if( type == PCB_IO_MGR::NESTED_TABLE )
    {
        wxString   msg;
        wxFileName fileName( row->URI() );

        if( fileName.FileExists() )
            return tl::unexpected( LIBRARY_TABLE_OK() );
        else
            msg = wxString::Format( _( "Nested table '%s' not found." ), row->URI() );

        return tl::unexpected( LIBRARY_ERROR( msg ) );
    }
    else if( type == PCB_IO_MGR::PCB_FILE_UNKNOWN )
    {
        wxLogTrace( traceLibraries, "FP: Plugin type %s is unknown!", row->Type() );
        wxString msg = wxString::Format( _( "Unknown library type %s " ), row->Type() );
        return tl::unexpected( LIBRARY_ERROR( msg ) );
    }

    PCB_IO* plugin = PCB_IO_MGR::FindPlugin( type );
    wxCHECK( plugin, tl::unexpected( LIBRARY_ERROR( _( "Internal error" ) ) ) );

    wxLogTrace( traceLibraries, "FP: Library %s (%s) plugin created", row->Nickname(),
                magic_enum::enum_name( row->Scope() ) );

    return plugin;
}


PCB_IO* FOOTPRINT_LIBRARY_ADAPTER::pcbplugin( const LIB_DATA* aRow )
{
    // Note: can't use dynamic_cast across compile units on Mac
    wxCHECK( aRow->plugin->IsPCB_IO(), nullptr );
    PCB_IO* ret = static_cast<PCB_IO*>( aRow->plugin.get() );
    return ret;
}
