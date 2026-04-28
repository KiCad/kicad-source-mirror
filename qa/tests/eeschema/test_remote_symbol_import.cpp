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
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <boost/test/unit_test.hpp>

#include <eeschema_settings.h>
#include <io/io_mgr.h>
#include <lib_symbol.h>
#include <picosha2.h>
#include <remote_symbol_download_manager.h>
#include <remote_symbol_import_job.h>
#include <sch_io/sch_io.h>
#include <sch_io/sch_io_mgr.h>
#include <settings/settings_manager.h>

#include <wx/filefn.h>
#include <wx/filename.h>
#include <wx/stdpaths.h>


namespace
{
std::string symbolPayload( const char* aName )
{
    return wxString::Format(
                   wxS( "(kicad_symbol_lib (version 20220914) (generator kicad_symbol_editor)\n"
                        "  (symbol \"%s\" (in_bom yes) (on_board yes)\n"
                        "    (property \"Reference\" \"R\" (at 0 0 0)\n"
                        "      (effects (font (size 1.27 1.27)))\n"
                        "    )\n"
                        "    (property \"Value\" \"%s\" (at 0 0 0)\n"
                        "      (effects (font (size 1.27 1.27)))\n"
                        "    )\n"
                        "    (property \"Footprint\" \"\" (at 0 0 0)\n"
                        "      (effects (font (size 1.27 1.27)) hide)\n"
                        "    )\n"
                        "    (property \"Datasheet\" \"\" (at 0 0 0)\n"
                        "      (effects (font (size 1.27 1.27)) hide)\n"
                        "    )\n"
                        "    (symbol \"%s_0_1\"\n"
                        "      (rectangle (start -1.27 -1.27) (end 1.27 1.27)\n"
                        "        (stroke (width 0) (type default))\n"
                        "        (fill (type background))\n"
                        "      )\n"
                        "    )\n"
                        "    (symbol \"%s_1_1\"\n"
                        "      (pin passive line (at -3.81 0 0) (length 2.54)\n"
                        "        (name \"PIN\" (effects (font (size 1.27 1.27))))\n"
                        "        (number \"1\" (effects (font (size 1.27 1.27))))\n"
                        "      )\n"
                        "    )\n"
                        "  )\n"
                        ")\n" ),
                   wxString::FromUTF8( aName ), wxString::FromUTF8( aName ),
                   wxString::FromUTF8( aName ), wxString::FromUTF8( aName ) )
            .ToStdString();
}


wxString sha256Hex( const std::string& aPayload )
{
    std::string hashHex;
    picosha2::hash256_hex_string( aPayload.begin(), aPayload.end(), hashHex );
    return wxString::FromUTF8( hashHex.c_str() );
}


wxString tempDir()
{
    wxString path = wxFileName::CreateTempFileName( wxS( "remote-symbol-import" ) );
    wxRemoveFile( path );
    wxMkdir( path );
    return path;
}


REMOTE_PROVIDER_METADATA provider()
{
    REMOTE_PROVIDER_METADATA metadata;
    metadata.provider_name = wxString( "Acme" );
    metadata.provider_version = wxString( "1.0.0" );
    metadata.api_base_url = wxString( "https://provider.example.test/api" );
    metadata.max_download_bytes = 4096;
    metadata.parts_v1 = true;
    metadata.direct_downloads_v1 = true;
    return metadata;
}

REMOTE_SYMBOL_IMPORT_CONTEXT importContext()
{
    REMOTE_SYMBOL_IMPORT_CONTEXT context;
    context.symbol_name = wxString( "R" );
    context.library_name = wxString( "Device" );
    return context;
}


REMOTE_PROVIDER_PART_MANIFEST manifest()
{
    REMOTE_PROVIDER_PART_MANIFEST manifest;
    manifest.part_id = wxString( "acme-res-10k" );
    manifest.display_name = wxString( "RC0603FR-0710KL" );

    REMOTE_PROVIDER_PART_ASSET symbol;
    const std::string symbolBlob = symbolPayload( "R" );
    symbol.asset_type = wxString( "symbol" );
    symbol.name = wxString( "acme-res-10k.kicad_sym" );
    symbol.content_type = wxString( "application/x-kicad-symbol" );
    symbol.size_bytes = static_cast<long long>( symbolBlob.size() );
    symbol.sha256 = sha256Hex( symbolBlob );
    symbol.download_url = wxString( "https://provider.example.test/downloads/acme-res-10k.kicad_sym" );
    symbol.required = true;
    symbol.target_library = wxString( "Device" );
    symbol.target_name = wxString( "R" );

    REMOTE_PROVIDER_PART_ASSET footprint;
    footprint.asset_type = wxString( "footprint" );
    footprint.name = wxString( "R_0603.pretty" );
    footprint.content_type = wxString( "application/x-kicad-footprint" );
    footprint.size_bytes = 44;
    footprint.sha256 = wxString( "8d8090740282c9ec23541a148af0ae57543e0da581e00e714e066dc4a1adefb0" );
    footprint.download_url = wxString( "https://provider.example.test/downloads/R_0603.pretty" );
    footprint.required = false;
    footprint.target_library = wxString( "Resistor_SMD" );
    footprint.target_name = wxString( "R_0603_1608Metric" );

    manifest.assets = { symbol, footprint };
    return manifest;
}
} // namespace


BOOST_AUTO_TEST_SUITE( RemoteSymbolImportTests )

BOOST_AUTO_TEST_CASE( ImportWritesDownloadedAssets )
{
    const wxString outputDir = tempDir();
    EESCHEMA_SETTINGS* settings = GetAppSettings<EESCHEMA_SETTINGS>( "eeschema" );
    BOOST_REQUIRE( settings );
    settings->m_RemoteSymbol.destination_dir = outputDir;
    settings->m_RemoteSymbol.library_prefix = wxString( "testremote" );
    settings->m_RemoteSymbol.add_to_global_table = true;

    REMOTE_SYMBOL_DOWNLOAD_MANAGER downloader(
            [&]( const wxString& aUrl, REMOTE_SYMBOL_FETCH_RESPONSE& aResponse, wxString& aError )
            {
                wxUnusedVar( aError );
                aResponse.status_code = 200;

                if( aUrl.EndsWith( wxString( "acme-res-10k.kicad_sym" ) ) )
                {
                    aResponse.content_type = wxString( "application/x-kicad-symbol" );
                    const std::string payload = symbolPayload( "R" );
                    aResponse.payload.assign( payload.begin(), payload.end() );
                    return true;
                }

                aResponse.content_type = wxString( "application/x-kicad-footprint" );
                const std::string payload = "(module \"R_0603_1608Metric\" (layer \"F.Cu\"))\n";
                aResponse.payload.assign( payload.begin(), payload.end() );
                return true;
            } );

    REMOTE_SYMBOL_IMPORT_JOB job( nullptr, &downloader );
    wxString error;

    BOOST_REQUIRE( job.Import( provider(), importContext(), manifest(), false, error ) );

    wxFileName symbolPath( outputDir, wxString() );
    symbolPath.AppendDir( wxString( "symbols" ) );
    symbolPath.SetFullName( wxString( "testremote_device.kicad_sym" ) );
    BOOST_CHECK( symbolPath.FileExists() );

    wxFileName footprintPath( outputDir, wxString() );
    footprintPath.AppendDir( wxString( "footprints" ) );
    footprintPath.AppendDir( wxString( "testremote_resistor_smd.pretty" ) );
    footprintPath.SetFullName( wxString( "R_0603_1608Metric.kicad_mod" ) );
    BOOST_CHECK( footprintPath.FileExists() );

    // The downloaded symbol's Footprint field must point to the local LIB_ID of the
    // bundled footprint so that subsequent placement / CvPcb sees the link.
    IO_RELEASER<SCH_IO> plugin( SCH_IO_MGR::FindPlugin( SCH_IO_MGR::SCH_KICAD ) );
    BOOST_REQUIRE( plugin );

    LIB_SYMBOL* loaded = plugin->LoadSymbol( symbolPath.GetFullPath(), wxString( "R" ) );
    BOOST_REQUIRE( loaded );
    BOOST_CHECK_EQUAL( loaded->GetFootprintProp().ToStdString(),
                       std::string( "testremote_resistor_smd:R_0603_1608Metric" ) );
}

BOOST_AUTO_TEST_CASE( ImportRejectsSymbolPayloadThatDoesNotContainExpectedName )
{
    const wxString outputDir = tempDir();
    EESCHEMA_SETTINGS* settings = GetAppSettings<EESCHEMA_SETTINGS>( "eeschema" );
    BOOST_REQUIRE( settings );
    settings->m_RemoteSymbol.destination_dir = outputDir;
    settings->m_RemoteSymbol.library_prefix = wxString( "testremote" );
    settings->m_RemoteSymbol.add_to_global_table = true;

    REMOTE_SYMBOL_DOWNLOAD_MANAGER downloader(
            [&]( const wxString& aUrl, REMOTE_SYMBOL_FETCH_RESPONSE& aResponse, wxString& aError )
            {
                wxUnusedVar( aError );
                aResponse.status_code = 200;

                if( aUrl.EndsWith( wxString( "acme-res-10k.kicad_sym" ) ) )
                {
                    aResponse.content_type = wxString( "application/x-kicad-symbol" );
                    const std::string payload = symbolPayload( "WrongName" );
                    aResponse.payload.assign( payload.begin(), payload.end() );
                    return true;
                }

                aResponse.content_type = wxString( "application/x-kicad-footprint" );
                const std::string payload = "(module \"R_0603_1608Metric\" (layer \"F.Cu\"))\n";
                aResponse.payload.assign( payload.begin(), payload.end() );
                return true;
            } );

    REMOTE_SYMBOL_IMPORT_JOB job( nullptr, &downloader );
    wxString error;

    BOOST_CHECK( !job.Import( provider(), importContext(), manifest(), false, error ) );
    BOOST_CHECK( !error.IsEmpty() );
}

BOOST_AUTO_TEST_CASE( ImportLinksFirstFootprintAndAddsAlternatesAsFilters )
{
    const wxString outputDir = tempDir();
    EESCHEMA_SETTINGS* settings = GetAppSettings<EESCHEMA_SETTINGS>( "eeschema" );
    BOOST_REQUIRE( settings );
    settings->m_RemoteSymbol.destination_dir = outputDir;
    settings->m_RemoteSymbol.library_prefix = wxString( "testremote" );
    settings->m_RemoteSymbol.add_to_global_table = true;

    REMOTE_PROVIDER_PART_MANIFEST multiManifest;
    multiManifest.part_id = wxString( "acme-res-10k" );

    REMOTE_PROVIDER_PART_ASSET symbolAsset;
    const std::string symbolBlob = symbolPayload( "R" );
    symbolAsset.asset_type = wxString( "symbol" );
    symbolAsset.name = wxString( "acme-res-10k.kicad_sym" );
    symbolAsset.content_type = wxString( "application/x-kicad-symbol" );
    symbolAsset.size_bytes = static_cast<long long>( symbolBlob.size() );
    symbolAsset.sha256 = sha256Hex( symbolBlob );
    symbolAsset.download_url = wxString( "https://provider.example.test/downloads/acme-res-10k.kicad_sym" );
    symbolAsset.target_library = wxString( "Device" );
    symbolAsset.target_name = wxString( "R" );

    const std::string fpBlob0603 = "(module \"R_0603_1608Metric\" (layer \"F.Cu\"))\n";
    const std::string fpBlob0805 = "(module \"R_0805_2012Metric\" (layer \"F.Cu\"))\n";

    REMOTE_PROVIDER_PART_ASSET fpPrimary;
    fpPrimary.asset_type = wxString( "footprint" );
    fpPrimary.name = wxString( "R_0603.pretty" );
    fpPrimary.content_type = wxString( "application/x-kicad-footprint" );
    fpPrimary.size_bytes = static_cast<long long>( fpBlob0603.size() );
    fpPrimary.sha256 = sha256Hex( fpBlob0603 );
    fpPrimary.download_url = wxString( "https://provider.example.test/downloads/R_0603.kicad_mod" );
    fpPrimary.target_library = wxString( "Resistor_SMD" );
    fpPrimary.target_name = wxString( "R_0603_1608Metric" );

    REMOTE_PROVIDER_PART_ASSET fpAlt;
    fpAlt.asset_type = wxString( "footprint" );
    fpAlt.name = wxString( "R_0805.pretty" );
    fpAlt.content_type = wxString( "application/x-kicad-footprint" );
    fpAlt.size_bytes = static_cast<long long>( fpBlob0805.size() );
    fpAlt.sha256 = sha256Hex( fpBlob0805 );
    fpAlt.download_url = wxString( "https://provider.example.test/downloads/R_0805.kicad_mod" );
    fpAlt.target_library = wxString( "Resistor_SMD" );
    fpAlt.target_name = wxString( "R_0805_2012Metric" );

    // Intentionally list the symbol BEFORE the footprints to verify the import job
    // reorders processing so the symbol's footprint field can resolve.
    multiManifest.assets = { symbolAsset, fpPrimary, fpAlt };

    REMOTE_SYMBOL_DOWNLOAD_MANAGER downloader(
            [&]( const wxString& aUrl, REMOTE_SYMBOL_FETCH_RESPONSE& aResponse, wxString& aError )
            {
                wxUnusedVar( aError );
                aResponse.status_code = 200;

                if( aUrl.EndsWith( wxString( "acme-res-10k.kicad_sym" ) ) )
                {
                    aResponse.content_type = wxString( "application/x-kicad-symbol" );
                    aResponse.payload.assign( symbolBlob.begin(), symbolBlob.end() );
                    return true;
                }

                if( aUrl.EndsWith( wxString( "R_0805.kicad_mod" ) ) )
                {
                    aResponse.content_type = wxString( "application/x-kicad-footprint" );
                    aResponse.payload.assign( fpBlob0805.begin(), fpBlob0805.end() );
                    return true;
                }

                aResponse.content_type = wxString( "application/x-kicad-footprint" );
                aResponse.payload.assign( fpBlob0603.begin(), fpBlob0603.end() );
                return true;
            } );

    REMOTE_SYMBOL_IMPORT_JOB job( nullptr, &downloader );
    wxString error;
    BOOST_REQUIRE( job.Import( provider(), importContext(), multiManifest, false, error ) );

    wxFileName symbolPath( outputDir, wxString() );
    symbolPath.AppendDir( wxString( "symbols" ) );
    symbolPath.SetFullName( wxString( "testremote_device.kicad_sym" ) );
    BOOST_REQUIRE( symbolPath.FileExists() );

    IO_RELEASER<SCH_IO> plugin( SCH_IO_MGR::FindPlugin( SCH_IO_MGR::SCH_KICAD ) );
    BOOST_REQUIRE( plugin );

    LIB_SYMBOL* loaded = plugin->LoadSymbol( symbolPath.GetFullPath(), wxString( "R" ) );
    BOOST_REQUIRE( loaded );

    BOOST_CHECK_EQUAL( loaded->GetFootprintProp().ToStdString(),
                       std::string( "testremote_resistor_smd:R_0603_1608Metric" ) );

    const wxArrayString filters = loaded->GetFPFilters();
    BOOST_CHECK_EQUAL( filters.GetCount(), 1u );
    if( filters.GetCount() == 1 )
        BOOST_CHECK_EQUAL( filters[0].ToStdString(), std::string( "R_0805_2012Metric" ) );
}


BOOST_AUTO_TEST_SUITE_END()
