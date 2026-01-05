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
#include <memory>

#include <common.h>
#include <dialog_shim.h>
#include <ki_exception.h>
#include <wx/log.h>

#include <lib_symbol.h>
#include <libraries/symbol_library_adapter.h>

#include <env_vars.h>
#include <pgm_base.h>
#include <project.h>
#include <thread_pool.h>
#include <trace_helpers.h>
#include <settings/settings_manager.h>


const char* SYMBOL_LIBRARY_ADAPTER::PropPowerSymsOnly = "pwr_sym_only";
const char* SYMBOL_LIBRARY_ADAPTER::PropNonPowerSymsOnly = "non_pwr_sym_only";

std::map<wxString, LIB_DATA> SYMBOL_LIBRARY_ADAPTER::GlobalLibraries;

std::mutex SYMBOL_LIBRARY_ADAPTER::GlobalLibraryMutex;


SYMBOL_LIBRARY_ADAPTER::SYMBOL_LIBRARY_ADAPTER( LIBRARY_MANAGER& aManager ) :
            LIBRARY_MANAGER_ADAPTER( aManager )
{
}


wxString SYMBOL_LIBRARY_ADAPTER::GlobalPathEnvVariableName()
{
    return ENV_VAR::GetVersionedEnvVarName( wxS( "SYMBOL_DIR" ) );
}


SCH_IO* SYMBOL_LIBRARY_ADAPTER::schplugin( const LIB_DATA* aRow )
{
    SCH_IO* ret = dynamic_cast<SCH_IO*>( aRow->plugin.get() );
    wxCHECK( aRow->plugin && ret, nullptr );
    return ret;
}


/// Loads or reloads the given library, if it exists
std::optional<LIB_STATUS> SYMBOL_LIBRARY_ADAPTER::LoadOne( LIB_DATA* aLib )
{
    std::lock_guard lock ( aLib->mutex );
    aLib->status.load_status = LOAD_STATUS::LOADING;

    std::map<std::string, UTF8> options = aLib->row->GetOptionsMap();

    try
    {
        wxArrayString dummyList;
        schplugin( aLib )->EnumerateSymbolLib( dummyList, getUri( aLib->row ), &options );
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


std::optional<LIB_STATUS> SYMBOL_LIBRARY_ADAPTER::LoadOne( const wxString& nickname )
{
    LIBRARY_RESULT<LIB_DATA*> result = loadIfNeeded( nickname );

    if( result.has_value() )
        return LoadOne( *result );

    return LIB_STATUS { .load_status = LOAD_STATUS::LOAD_ERROR,
                        .error = LIBRARY_ERROR( { result.error() } ) };
}


LIBRARY_RESULT<IO_BASE*> SYMBOL_LIBRARY_ADAPTER::createPlugin( const LIBRARY_TABLE_ROW* row )
{
    SCH_IO_MGR::SCH_FILE_T type = SCH_IO_MGR::EnumFromStr( row->Type() );

    if( type == SCH_IO_MGR::SCH_FILE_UNKNOWN )
    {
        wxLogTrace( traceLibraries, "Sym: Plugin type %s is unknown!", row->Type() );
        wxString msg = wxString::Format( _( "Unknown library type %s " ), row->Type() );
        return tl::unexpected( LIBRARY_ERROR( msg ) );
    }

    SCH_IO* plugin = SCH_IO_MGR::FindPlugin( type );
    wxCHECK( plugin, tl::unexpected( LIBRARY_ERROR( _( "Internal error" ) ) ) );

    plugin->SetLibraryManagerAdapter( this );

    wxLogTrace( traceLibraries, "Sym: Library %s (%s) plugin created", row->Nickname(),
                magic_enum::enum_name( row->Scope() ) );

    return plugin;
}


std::vector<LIB_SYMBOL*> SYMBOL_LIBRARY_ADAPTER::GetSymbols( const wxString& aNickname, SYMBOL_TYPE aType )
{
    std::vector<LIB_SYMBOL*> symbols;

    std::optional<const LIB_DATA*> maybeLib = fetchIfLoaded( aNickname );

    if( !maybeLib )
        return symbols;

    const LIB_DATA* lib = *maybeLib;
    std::map<std::string, UTF8> options = lib->row->GetOptionsMap();

    if( aType == SYMBOL_TYPE::POWER_ONLY )
        options[PropPowerSymsOnly] = "";

    try
    {
        schplugin( lib )->EnumerateSymbolLib( symbols, getUri( lib->row ), &options );
    }
    catch( IO_ERROR& e )
    {
        wxLogTrace( traceLibraries, "Sym: Exception enumerating library %s: %s", lib->row->Nickname(), e.What() );
    }

    for( LIB_SYMBOL* symbol : symbols )
    {
        LIB_ID id = symbol->GetLibId();
        id.SetLibNickname( lib->row->Nickname() );
        symbol->SetLibId( id );
    }

    return symbols;
}


std::vector<wxString> SYMBOL_LIBRARY_ADAPTER::GetSymbolNames( const wxString& aNickname, SYMBOL_TYPE aType )
{
    // TODO(JE) can we kill wxArrayString in internal API?
    wxArrayString namesAS;
    std::vector<wxString> names;

    if( std::optional<const LIB_DATA*> maybeLib = fetchIfLoaded( aNickname ) )
    {
        const LIB_DATA* lib = *maybeLib;
        std::map<std::string, UTF8> options = lib->row->GetOptionsMap();

        if( aType == SYMBOL_TYPE::POWER_ONLY )
            options[PropPowerSymsOnly] = "";

        schplugin( lib )->EnumerateSymbolLib( namesAS, getUri( lib->row ), &options );
    }

    for( const wxString& name : namesAS )
        names.emplace_back( name );

    return names;
}


LIB_SYMBOL* SYMBOL_LIBRARY_ADAPTER::LoadSymbol( const wxString& aNickname, const wxString& aName )
{
    if( std::optional<const LIB_DATA*> lib = fetchIfLoaded( aNickname ) )
    {
        if( LIB_SYMBOL* symbol = schplugin( *lib )->LoadSymbol( getUri( ( *lib )->row ), aName ) )
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


SYMBOL_LIBRARY_ADAPTER::SAVE_T SYMBOL_LIBRARY_ADAPTER::SaveSymbol( const wxString& aNickname,
                                                                   const LIB_SYMBOL* aSymbol, bool aOverwrite )
{
    wxCHECK( aSymbol, SAVE_SKIPPED );

    LIBRARY_RESULT<LIB_DATA*> libResult = loadIfNeeded( aNickname );

    if( !libResult.has_value() )
    {
        wxLogTrace( traceLibraries, "SaveSymbol: unable to load library %s: %s", aNickname,
                    libResult.error().message );
        return SAVE_SKIPPED;
    }

    LIB_DATA* lib = *libResult;

    if( !lib )
    {
        wxLogTrace( traceLibraries, "SaveSymbol: library %s not found", aNickname );
        return SAVE_SKIPPED;
    }

    SCH_IO* plugin = schplugin( lib );
    wxCHECK( plugin, SAVE_SKIPPED );

    std::map<std::string, UTF8> options = lib->row->GetOptionsMap();

    if( !aOverwrite )
    {
        try
        {
            std::unique_ptr<LIB_SYMBOL> existing( plugin->LoadSymbol( getUri( lib->row ), aSymbol->GetName(),
                                                                      &options ) );

            if( existing )
                return SAVE_SKIPPED;
        }
        catch( const IO_ERROR& e )
        {
            wxLogTrace( traceLibraries, "SaveSymbol: error checking for existing symbol %s:%s: %s", aNickname,
                        aSymbol->GetName(), e.What() );
            return SAVE_SKIPPED;
        }
    }

    try
    {
        plugin->SaveSymbol( getUri( lib->row ), aSymbol, &options );
    }
    catch( const IO_ERROR& e )
    {
        wxLogTrace( traceLibraries, "SaveSymbol: error saving %s:%s: %s", aNickname, aSymbol->GetName(), e.What() );
        return SAVE_SKIPPED;
    }

    return SAVE_OK;
}


void SYMBOL_LIBRARY_ADAPTER::DeleteSymbol( const wxString& aNickname, const wxString& aSymbolName )
{
    wxCHECK_MSG( false, /* void */, "Unimplemented!" );
}


bool SYMBOL_LIBRARY_ADAPTER::IsSymbolLibWritable( const wxString& aLib )
{
    if( m_libraries.contains( aLib ) )
        return m_libraries[aLib].plugin->IsLibraryWritable( getUri( m_libraries[aLib].row ) );

    if( GlobalLibraries.contains( aLib ) )
        return GlobalLibraries[aLib].plugin->IsLibraryWritable( getUri( GlobalLibraries[aLib].row ) );

    return false;
}


void SYMBOL_LIBRARY_ADAPTER::AsyncLoad()
{
    // TODO(JE) any reason to clean these up earlier?
    std::erase_if( m_futures,
                   []( const std::future<void>& aFuture )
                   {
                       return aFuture.valid();
                   } );

    if( !m_futures.empty() )
    {
        wxLogTrace( traceLibraries, "Sym: Cannot AsyncLoad, futures from a previous call remain!" );
        return;
    }

    std::vector<LIBRARY_TABLE_ROW*> rows = m_manager.Rows( LIBRARY_TABLE_TYPE::SYMBOL );

    m_loadTotal = rows.size();
    m_loadCount.store( 0 );

    if( m_loadTotal == 0 )
    {
        wxLogTrace( traceLibraries, "Sym: AsyncLoad: no libraries left to load; exiting" );
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
                            schplugin( lib )->EnumerateSymbolLib( dummyList, getUri( lib->row ), &options );
                            wxLogTrace( traceLibraries, "Sym: %s: library enumerated %zu items", nickname,
                                        dummyList.size() );
                            lib->status.load_status = LOAD_STATUS::LOADED;
                        }
                        catch( IO_ERROR& e )
                        {
                            lib->status.load_status = LOAD_STATUS::LOAD_ERROR;
                            lib->status.error = LIBRARY_ERROR( { e.What() } );
                            wxLogTrace( traceLibraries, "Sym: %s: plugin threw exception: %s", nickname, e.What() );
                        }
                    }
                    else
                    {
                        switch( scope )
                        {
                        case LIBRARY_TABLE_SCOPE::GLOBAL:
                        {
                            std::lock_guard lock( GlobalLibraryMutex );

                            GlobalLibraries[nickname].status = LIB_STATUS( { .load_status = LOAD_STATUS::LOAD_ERROR,
                                                                             .error = result.error() } );

                            break;
                        }

                        case LIBRARY_TABLE_SCOPE::PROJECT:
                        {
                            wxLogTrace( traceLibraries, "Sym: project library error: %s: %s", nickname,
                                        result.error().message );
                            std::lock_guard lock( m_libraries_mutex );

                            m_libraries[nickname].status = LIB_STATUS( { .load_status = LOAD_STATUS::LOAD_ERROR,
                                                                         .error = result.error() } );

                            break;
                        }

                        default:
                            wxFAIL_MSG( "Unexpected library table scope" );
                        }
                    }

                    ++m_loadCount;
                }, BS::pr::lowest ) );
    }

    if( m_loadTotal )
        wxLogTrace( traceLibraries, "Sym: Started async load of %zu libraries", m_loadTotal );
}


std::optional<LIB_STATUS> SYMBOL_LIBRARY_ADAPTER::GetLibraryStatus( const wxString& aNickname ) const
{
    if( m_libraries.contains( aNickname ) )
        return m_libraries.at( aNickname ).status;

    if( GlobalLibraries.contains( aNickname ) )
        return GlobalLibraries.at( aNickname ).status;

    return std::nullopt;
}


std::vector<wxString> SYMBOL_LIBRARY_ADAPTER::GetAvailableExtraFields( const wxString& aNickname )
{
    std::vector<wxString> fields;

    if( std::optional<LIB_DATA*> result = fetchIfLoaded( aNickname ) )
    {
        LIB_DATA* rowData = *result;
        int hash = schplugin( rowData )->GetModifyHash();

        if( hash != rowData->modify_hash )
        {
            rowData->modify_hash = hash;
            schplugin( rowData )->GetAvailableSymbolFields( rowData->available_fields_cache );
        }

        return rowData->available_fields_cache;
    }

    return fields;
}


bool SYMBOL_LIBRARY_ADAPTER::SupportsSubLibraries( const wxString& aNickname ) const
{
    if( std::optional<const LIB_DATA*> result = fetchIfLoaded( aNickname ) )
    {
        const LIB_DATA* rowData = *result;
        return schplugin( rowData )->SupportsSubLibraries();
    }

    return false;
}


std::vector<SUB_LIBRARY> SYMBOL_LIBRARY_ADAPTER::GetSubLibraries( const wxString& aNickname ) const
{
    std::vector<SUB_LIBRARY> ret;

    if( std::optional<const LIB_DATA*> result = fetchIfLoaded( aNickname ) )
    {
        const LIB_DATA* rowData = *result;
        std::vector<wxString> names;
        schplugin( rowData )->GetSubLibraryNames( names );

        for( const wxString& name : names )
        {
            ret.emplace_back( SUB_LIBRARY { .nickname = name,
                                            .description = schplugin( rowData )->GetSubLibraryDescription( name ) } );
        }
    }

    return ret;
}


bool SYMBOL_LIBRARY_ADAPTER::SupportsConfigurationDialog( const wxString& aNickname ) const
{
    if( std::optional<const LIB_DATA*> result = fetchIfLoaded( aNickname ) )
        return ( *result )->plugin->SupportsConfigurationDialog();

    return false;
}


void SYMBOL_LIBRARY_ADAPTER::ShowConfigurationDialog( const wxString& aNickname, wxWindow* aParent ) const
{
    std::optional<const LIB_DATA*> optRow = fetchIfLoaded( aNickname );

    if( !optRow || !( *optRow )->plugin->SupportsConfigurationDialog() )
        return;

    DIALOG_SHIM* dialog = ( *optRow )->plugin->CreateConfigurationDialog( aParent );
    dialog->ShowModal();
}


int SYMBOL_LIBRARY_ADAPTER::GetModifyHash() const
{
    int hash = 0;

    for( const LIBRARY_TABLE_ROW* row : m_manager.Rows( Type() ) )
    {
        if( std::optional<const LIB_DATA*> result = fetchIfLoaded( row->Nickname() ) )
        {
            const LIB_DATA* rowData = *result;
            wxCHECK2( rowData->row, continue );
            hash += schplugin( rowData )->GetModifyHash();
        }
    }

    return hash;
}
