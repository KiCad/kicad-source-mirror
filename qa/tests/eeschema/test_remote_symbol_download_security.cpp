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
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <boost/test/unit_test.hpp>

#include <remote_provider_metadata.h>
#include <remote_symbol_download_manager.h>


namespace
{
REMOTE_PROVIDER_METADATA provider()
{
    REMOTE_PROVIDER_METADATA metadata;
    metadata.api_base_url = wxString( "https://provider.example.test/api" );
    metadata.panel_url = wxString( "https://provider.example.test/app" );
    metadata.max_download_bytes = 4096;
    return metadata;
}


REMOTE_PROVIDER_PART_ASSET baseAsset()
{
    REMOTE_PROVIDER_PART_ASSET asset;
    asset.asset_type = wxString( "symbol" );
    asset.name = wxString( "test.kicad_sym" );
    asset.content_type = wxString( "application/x-kicad-symbol" );
    asset.size_bytes = 5;
    asset.sha256 = wxString( "5994471abb01112afcc18159f6cc74b4f511b99806da59b3caf5a9c173cacfc5" );
    asset.download_url = wxString( "https://provider.example.test/downloads/test.kicad_sym" );
    asset.required = true;
    asset.target_library = wxString( "Device" );
    asset.target_name = wxString( "R" );
    return asset;
}
} // namespace


BOOST_AUTO_TEST_SUITE( RemoteSymbolDownloadSecurityTests )

BOOST_AUTO_TEST_CASE( DigestMismatchRejected )
{
    REMOTE_SYMBOL_DOWNLOAD_MANAGER manager(
            [&]( const wxString& aUrl, REMOTE_SYMBOL_FETCH_RESPONSE& aResponse, wxString& aError )
            {
                wxUnusedVar( aUrl );
                wxUnusedVar( aError );
                aResponse.status_code = 200;
                aResponse.content_type = wxString( "application/x-kicad-symbol" );
                aResponse.payload = { 'b', 'a', 'd', 'd', '!' };
                return true;
            } );

    REMOTE_SYMBOL_FETCHED_ASSET fetched;
    wxString error;
    BOOST_CHECK( !manager.DownloadAndVerify( provider(), baseAsset(), 10, fetched, error ) );
    BOOST_CHECK( error.Contains( wxString( "digest" ) ) );
}

BOOST_AUTO_TEST_CASE( SizeMismatchRejected )
{
    REMOTE_SYMBOL_DOWNLOAD_MANAGER manager(
            [&]( const wxString& aUrl, REMOTE_SYMBOL_FETCH_RESPONSE& aResponse, wxString& aError )
            {
                wxUnusedVar( aUrl );
                wxUnusedVar( aError );
                aResponse.status_code = 200;
                aResponse.content_type = wxString( "application/x-kicad-symbol" );
                aResponse.payload = { '1', '2', '3', '4' };
                return true;
            } );

    REMOTE_SYMBOL_FETCHED_ASSET fetched;
    wxString error;
    BOOST_CHECK( !manager.DownloadAndVerify( provider(), baseAsset(), 10, fetched, error ) );
    BOOST_CHECK( error.Contains( wxString( "size" ) ) );
}

BOOST_AUTO_TEST_CASE( ContentTypeMismatchRejected )
{
    REMOTE_SYMBOL_DOWNLOAD_MANAGER manager(
            [&]( const wxString& aUrl, REMOTE_SYMBOL_FETCH_RESPONSE& aResponse, wxString& aError )
            {
                wxUnusedVar( aUrl );
                wxUnusedVar( aError );
                aResponse.status_code = 200;
                aResponse.content_type = wxString( "text/plain" );
                aResponse.payload = { '1', '2', '3', '4', '5' };
                return true;
            } );

    REMOTE_SYMBOL_FETCHED_ASSET fetched;
    wxString error;
    BOOST_CHECK( !manager.DownloadAndVerify( provider(), baseAsset(), 10, fetched, error ) );
    BOOST_CHECK( error.Contains( wxString( "content type" ) ) );
}

BOOST_AUTO_TEST_CASE( OversizeAssetRejectedBeforeImport )
{
    REMOTE_PROVIDER_PART_ASSET asset = baseAsset();
    asset.size_bytes = 32;

    REMOTE_SYMBOL_DOWNLOAD_MANAGER manager(
            [&]( const wxString& aUrl, REMOTE_SYMBOL_FETCH_RESPONSE& aResponse, wxString& aError )
            {
                wxUnusedVar( aUrl );
                wxUnusedVar( aError );
                aResponse.status_code = 200;
                aResponse.content_type = wxString( "application/x-kicad-symbol" );
                aResponse.payload = std::vector<uint8_t>( 32, 'x' );
                return true;
            } );

    REMOTE_SYMBOL_FETCHED_ASSET fetched;
    wxString error;
    BOOST_CHECK( !manager.DownloadAndVerify( provider(), asset, 16, fetched, error ) );
    BOOST_CHECK( error.Contains( wxString( "limit" ) ) );
}

BOOST_AUTO_TEST_CASE( UrlBasedAssetsRequireDigest )
{
    REMOTE_PROVIDER_PART_ASSET asset = baseAsset();
    asset.sha256.clear();

    REMOTE_SYMBOL_DOWNLOAD_MANAGER manager(
            [&]( const wxString& aUrl, REMOTE_SYMBOL_FETCH_RESPONSE& aResponse, wxString& aError )
            {
                wxUnusedVar( aUrl );
                wxUnusedVar( aResponse );
                wxUnusedVar( aError );
                return true;
            } );

    REMOTE_SYMBOL_FETCHED_ASSET fetched;
    wxString error;
    BOOST_CHECK( !manager.DownloadAndVerify( provider(), asset, 10, fetched, error ) );
    BOOST_CHECK( error.Contains( wxString( "sha256" ) ) );
}

BOOST_AUTO_TEST_CASE( UrlBasedAssetsMustStayOnProviderOrigin )
{
    REMOTE_PROVIDER_PART_ASSET asset = baseAsset();
    asset.download_url = wxString( "https://evil.example.test/downloads/test.kicad_sym" );

    REMOTE_SYMBOL_DOWNLOAD_MANAGER manager(
            [&]( const wxString& aUrl, REMOTE_SYMBOL_FETCH_RESPONSE& aResponse, wxString& aError )
            {
                wxUnusedVar( aUrl );
                wxUnusedVar( aResponse );
                wxUnusedVar( aError );
                return true;
            } );

    REMOTE_SYMBOL_FETCHED_ASSET fetched;
    wxString error;
    BOOST_CHECK( !manager.DownloadAndVerify( provider(), asset, 10, fetched, error ) );
    BOOST_CHECK( error.Contains( wxString( "origin" ) ) );
}

BOOST_AUTO_TEST_SUITE_END()
