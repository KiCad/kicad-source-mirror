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
#include <string_view>
#include <libraries/library_manager.h>
#include <pgm_base.h>
#include <remote_symbol_download_manager.h>
#include <settings/settings_manager.h>

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
    bool importedSymbol = false;
    wxString placedNickname;
    wxString placedSymbolName;

    for( const REMOTE_PROVIDER_PART_ASSET& asset : aManifest.assets )
    {
        REMOTE_SYMBOL_FETCHED_ASSET fetched;

        if( !downloader().DownloadAndVerify( aProvider, asset, remainingBudget, fetched, aError ) )
            return false;

        remainingBudget -= asset.size_bytes;

        if( asset.asset_type == wxS( "symbol" ) )
        {
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

            if( !WriteRemoteBinaryFile( outFile, fetched.payload, aError ) )
                return false;

            if( strictLibraryTables )
            {
                if( !EnsureRemoteLibraryEntry( LIBRARY_TABLE_TYPE::SYMBOL, outFile, nickname,
                                               addToGlobal, true, aError ) )
                    return false;

                LIBRARY_MANAGER& libMgr = Pgm().GetLibraryManager();
                const LIBRARY_TABLE_SCOPE scope =
                        addToGlobal ? LIBRARY_TABLE_SCOPE::GLOBAL : LIBRARY_TABLE_SCOPE::PROJECT;

                libMgr.ReloadLibraryEntry( LIBRARY_TABLE_TYPE::SYMBOL, nickname, scope );

                // Force the library to LOADED state so GetLibSymbol and the symbol chooser
                // can see the new symbol immediately. ReloadLibraryEntry leaves it in LOADING.
                libMgr.LoadLibraryEntry( LIBRARY_TABLE_TYPE::SYMBOL, nickname );
            }

            importedSymbol = true;
            placedNickname = nickname;
            placedSymbolName = symbolName;
        }
        else if( asset.asset_type == wxS( "footprint" ) )
        {
            wxFileName fpRoot = baseDir;
            fpRoot.AppendDir( wxS( "footprints" ) );

            const wxString libraryName = SanitizeRemoteFileComponent(
                    asset.target_library.IsEmpty() ? asset.name : asset.target_library,
                    wxS( "footprints" ), true );
            const wxString nickname = prefix + wxS( "_" ) + libraryName;

            wxFileName libDir = fpRoot;
            libDir.AppendDir( nickname + wxS( ".pretty" ) );

            wxString fileName = SanitizeRemoteFileComponent(
                    asset.target_name.IsEmpty() ? asset.name : asset.target_name,
                    wxS( "footprint" ) );

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
                const LIBRARY_TABLE_SCOPE scope =
                        addToGlobal ? LIBRARY_TABLE_SCOPE::GLOBAL : LIBRARY_TABLE_SCOPE::PROJECT;

                libMgr.ReloadLibraryEntry( LIBRARY_TABLE_TYPE::FOOTPRINT, nickname, scope );
                libMgr.LoadLibraryEntry( LIBRARY_TABLE_TYPE::FOOTPRINT, nickname );
            }
        }
        else if( asset.asset_type == wxS( "3dmodel" ) )
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
