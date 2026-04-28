/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 3 of the License, or (at your
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

#include <remote_symbol_import_job.h>

#include "remote_symbol_import_utils.h"

#include <eeschema_settings.h>
#include <io/io_mgr.h>
#include <lib_id.h>
#include <lib_symbol.h>
#include <libraries/library_manager.h>
#include <libraries/symbol_library_adapter.h>
#include <pgm_base.h>
#include <project_sch.h>
#include <remote_symbol_download_manager.h>
#include <sch_edit_frame.h>
#include <sch_io/sch_io.h>
#include <sch_io/sch_io_mgr.h>
#include <settings/settings_manager.h>
#include <string_view>

#include <wx/intl.h>


namespace
{
bool validateSymbolPayload( const std::vector<uint8_t>& aPayload, const wxString& aExpectedSymbolName,
                            wxString& aError )
{
    if( aExpectedSymbolName.IsEmpty() )
        return true;

    if( aPayload.empty() )
    {
        aError = _( "Downloaded symbol payload was empty." );
        return false;
    }

    std::string_view payload( reinterpret_cast<const char*>( aPayload.data() ), aPayload.size() );

    if( payload.find( "(kicad_symbol_lib" ) == std::string_view::npos )
    {
        aError = _( "Downloaded symbol payload was not a KiCad symbol library." );
        return false;
    }

    const std::string symbolToken =
            wxString::Format( wxS( "(symbol \"%s\"" ), aExpectedSymbolName ).ToStdString();

    if( payload.find( symbolToken ) == std::string_view::npos )
    {
        aError = wxString::Format(
                _( "Downloaded symbol payload did not include expected symbol '%s'." ),
                aExpectedSymbolName );
        return false;
    }

    return true;
}
} // namespace


REMOTE_SYMBOL_IMPORT_JOB::REMOTE_SYMBOL_IMPORT_JOB( SCH_EDIT_FRAME* aFrame,
                                                    REMOTE_SYMBOL_DOWNLOAD_MANAGER* aDownloader ) :
        m_frame( aFrame ),
        m_downloader( aDownloader )
{
    if( !m_downloader )
    {
        m_ownedDownloader = std::make_unique<REMOTE_SYMBOL_DOWNLOAD_MANAGER>();
        m_downloader = m_ownedDownloader.get();
    }
}


bool REMOTE_SYMBOL_IMPORT_JOB::Import( const REMOTE_PROVIDER_METADATA& aProvider,
                                       const REMOTE_SYMBOL_IMPORT_CONTEXT& aContext,
                                       const REMOTE_PROVIDER_PART_MANIFEST& aManifest,
                                       bool aPlaceSymbol, wxString& aError )
{
    aError.clear();

    EESCHEMA_SETTINGS* settings = GetAppSettings<EESCHEMA_SETTINGS>( "eeschema" );

    if( !settings )
    {
        aError = _( "Unable to load schematic settings." );
        return false;
    }

    wxFileName baseDir;

    if( !EnsureRemoteDestinationRoot( baseDir, aError ) )
        return false;

    long long remainingBudget = aProvider.max_download_bytes;
    const bool addToGlobal = settings->m_RemoteSymbol.add_to_global_table;
    const bool strictLibraryTables = m_frame != nullptr;
    const wxString prefix = RemoteLibraryPrefix();
    const LIBRARY_TABLE_SCOPE scope =
            addToGlobal ? LIBRARY_TABLE_SCOPE::GLOBAL : LIBRARY_TABLE_SCOPE::PROJECT;
    bool     importedSymbol = false;
    wxString placedNickname;
    wxString placedSymbolName;

    // Sort asset indices: footprints first, then 3D/SPICE, then symbols. The symbol's
    // Footprint field references a LIB_ID that must already exist on disk and (when
    // running interactive) be registered in the footprint library table by the time the
    // symbol file is written, otherwise CvPcb / placement-time resolution misses it.
    std::vector<size_t> footprintIdx, otherIdx, symbolIdx;

    for( size_t i = 0; i < aManifest.assets.size(); ++i )
    {
        const wxString& type = aManifest.assets[i].asset_type;

        if( type == wxS( "footprint" ) )
            footprintIdx.push_back( i );
        else if( type == wxS( "symbol" ) )
            symbolIdx.push_back( i );
        else
            otherIdx.push_back( i );
    }

    std::vector<LIB_ID> footprintLinks;

    auto downloadAsset = [&]( size_t i, REMOTE_SYMBOL_FETCHED_ASSET& fetched ) -> bool
    {
        if( !downloader().DownloadAndVerify( aProvider, aManifest.assets[i], remainingBudget,
                                             fetched, aError ) )
            return false;

        remainingBudget -= aManifest.assets[i].size_bytes;
        return true;
    };

    // --- Footprints ---
    for( size_t i : footprintIdx )
    {
        const REMOTE_PROVIDER_PART_ASSET& asset = aManifest.assets[i];
        REMOTE_SYMBOL_FETCHED_ASSET       fetched;

        if( !downloadAsset( i, fetched ) )
            return false;

        wxFileName fpRoot = baseDir;
        fpRoot.AppendDir( wxS( "footprints" ) );

        // Resolve logical names with the same fallbacks the original code used.
        const wxString resolvedLib =
                asset.target_library.IsEmpty() ? asset.name : asset.target_library;
        const wxString resolvedName =
                asset.target_name.IsEmpty() ? asset.name : asset.target_name;

        const LIB_ID    fpLibId = BuildRemoteLibId( resolvedLib, resolvedName );
        const wxString  nickname = fpLibId.GetUniStringLibNickname();

        wxFileName libDir = fpRoot;
        libDir.AppendDir( nickname + wxS( ".pretty" ) );

        wxString fileName = fpLibId.GetUniStringLibItemName();

        if( !fileName.Lower().EndsWith( wxS( ".kicad_mod" ) ) )
            fileName += wxS( ".kicad_mod" );

        wxFileName outFile( libDir );
        outFile.SetFullName( fileName );

        if( !WriteRemoteBinaryFile( outFile, fetched.payload, aError ) )
            return false;

        if( strictLibraryTables )
        {
            if( !EnsureRemoteLibraryEntry( LIBRARY_TABLE_TYPE::FOOTPRINT, libDir, nickname,
                                           addToGlobal, true, aError ) )
                return false;

            LIBRARY_MANAGER& libMgr = Pgm().GetLibraryManager();
            libMgr.ReloadLibraryEntry( LIBRARY_TABLE_TYPE::FOOTPRINT, nickname, scope );
            libMgr.LoadLibraryEntry( LIBRARY_TABLE_TYPE::FOOTPRINT, nickname );
        }

        footprintLinks.push_back( fpLibId );
    }

    // --- 3D models, SPICE ---
    for( size_t i : otherIdx )
    {
        const REMOTE_PROVIDER_PART_ASSET& asset = aManifest.assets[i];
        REMOTE_SYMBOL_FETCHED_ASSET       fetched;

        if( !downloadAsset( i, fetched ) )
            return false;

        if( asset.asset_type == wxS( "3dmodel" ) )
        {
            wxFileName modelDir = baseDir;
            modelDir.AppendDir( prefix + wxS( "_3d" ) );

            wxString fileName = SanitizeRemoteFileComponent(
                    asset.target_name.IsEmpty() ? asset.name : asset.target_name,
                    prefix + wxS( "_model" ) );

            wxFileName outFile( modelDir );
            outFile.SetFullName( fileName );

            if( !WriteRemoteBinaryFile( outFile, fetched.payload, aError ) )
                return false;
        }
        else if( asset.asset_type == wxS( "spice" ) )
        {
            wxFileName spiceDir = baseDir;
            spiceDir.AppendDir( prefix + wxS( "_spice" ) );

            wxString fileName = SanitizeRemoteFileComponent(
                    asset.target_name.IsEmpty() ? asset.name : asset.target_name,
                    prefix + wxS( "_model.cir" ) );

            if( !fileName.Lower().EndsWith( wxS( ".cir" ) ) )
                fileName += wxS( ".cir" );

            wxFileName outFile( spiceDir );
            outFile.SetFullName( fileName );

            if( !WriteRemoteBinaryFile( outFile, fetched.payload, aError ) )
                return false;
        }
    }

    // --- Symbols ---
    for( size_t i : symbolIdx )
    {
        const REMOTE_PROVIDER_PART_ASSET& asset = aManifest.assets[i];
        REMOTE_SYMBOL_FETCHED_ASSET       fetched;

        if( !downloadAsset( i, fetched ) )
            return false;

        wxFileName symbolDir = baseDir;
        symbolDir.AppendDir( wxS( "symbols" ) );

        const wxString libraryName = SanitizeRemoteFileComponent(
                asset.target_library.IsEmpty() ? aContext.library_name : asset.target_library,
                wxS( "symbols" ), true );
        const wxString symbolName =
                asset.target_name.IsEmpty() ? aContext.symbol_name : asset.target_name;
        const wxString nickname = prefix + wxS( "_" ) + libraryName;

        wxFileName outFile( symbolDir );
        outFile.SetFullName( nickname + wxS( ".kicad_sym" ) );

        if( !validateSymbolPayload( fetched.payload, symbolName, aError ) )
            return false;

        // Deserialize → mutate → save so the persisted symbol's Footprint field is
        // already pointing at the local LIB_IDs of the bundle's footprints.
        std::unique_ptr<LIB_SYMBOL> loaded =
                LoadRemoteSymbolFromPayload( fetched.payload, symbolName, aError );

        if( !loaded )
            return false;

        loaded->SetName( symbolName );
        LIB_ID savedId;
        savedId.SetLibNickname( nickname );
        savedId.SetLibItemName( symbolName );
        loaded->SetLibId( savedId );

        ApplyFootprintLinks( *loaded, footprintLinks );

        symbolDir.Mkdir( wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL );

        if( strictLibraryTables )
        {
            if( !EnsureRemoteLibraryEntry( LIBRARY_TABLE_TYPE::SYMBOL, outFile, nickname,
                                           addToGlobal, true, aError ) )
                return false;

            SYMBOL_LIBRARY_ADAPTER* adapter = PROJECT_SCH::SymbolLibAdapter( &m_frame->Prj() );

            if( !adapter
                || adapter->SaveSymbol( nickname, loaded.get(), true )
                           != SYMBOL_LIBRARY_ADAPTER::SAVE_OK )
            {
                aError = _( "Unable to save the downloaded symbol." );
                return false;
            }

            (void) loaded.release();   // ownership transferred to library cache

            LIBRARY_MANAGER& libMgr = Pgm().GetLibraryManager();
            libMgr.ReloadLibraryEntry( LIBRARY_TABLE_TYPE::SYMBOL, nickname, scope );
            libMgr.LoadLibraryEntry( LIBRARY_TABLE_TYPE::SYMBOL, nickname );
        }
        else
        {
            // Headless: write the raw payload to disk so the SCH plugin's library cache
            // has an existing file to load, then re-save through the plugin to replace
            // the symbol entry with the mutated (link-applied) copy. When there are no
            // links to apply, the raw payload on disk is already correct and we skip
            // the second save.
            if( !WriteRemoteBinaryFile( outFile, fetched.payload, aError ) )
                return false;

            if( !footprintLinks.empty() )
            {
                try
                {
                    IO_RELEASER<SCH_IO> plugin( SCH_IO_MGR::FindPlugin( SCH_IO_MGR::SCH_KICAD ) );

                    if( !plugin )
                    {
                        aError = _( "Unable to access the KiCad symbol plugin." );
                        return false;
                    }

                    plugin->SaveSymbol( outFile.GetFullPath(), loaded.get() );
                    (void) loaded.release();   // ownership transferred to plugin's cache
                }
                catch( const IO_ERROR& e )
                {
                    aError = wxString::Format( _( "Unable to save the downloaded symbol: %s" ),
                                               e.What() );
                    return false;
                }
            }
        }

        importedSymbol = true;
        placedNickname = nickname;
        placedSymbolName = symbolName;
    }

    if( aPlaceSymbol )
    {
        if( !importedSymbol )
        {
            aError = _( "No symbol asset was available to place." );
            return false;
        }

        if( !PlaceRemoteDownloadedSymbol( m_frame, placedNickname, placedSymbolName, aError ) )
            return false;
    }

    return true;
}


const REMOTE_SYMBOL_DOWNLOAD_MANAGER& REMOTE_SYMBOL_IMPORT_JOB::downloader() const
{
    return *m_downloader;
}
