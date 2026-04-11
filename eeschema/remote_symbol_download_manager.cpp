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
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <remote_symbol_download_manager.h>

#include <curl/curl.h>
#include <kicad_curl/kicad_curl_easy.h>
#include <picosha2.h>
#include <remote_provider_utils.h>
#include <wx/intl.h>


namespace
{
bool validateAssetUrl( const REMOTE_PROVIDER_METADATA& aProvider, const wxString& aUrl,
                       wxString& aError )
{
    if( !ValidateRemoteUrlSecurity( aUrl, aProvider.allow_insecure_localhost, aError,
                                     _( "Remote asset URL" ) ) )
    {
        return false;
    }

    const wxString assetOrigin = NormalizedUrlOrigin( aUrl );
    const wxString apiOrigin = NormalizedUrlOrigin( aProvider.api_base_url );
    const wxString panelOrigin = NormalizedUrlOrigin( aProvider.panel_url );
    const bool originAllowed =
            ( !apiOrigin.IsEmpty() && assetOrigin == apiOrigin )
            || ( !panelOrigin.IsEmpty() && assetOrigin == panelOrigin );

    if( assetOrigin.IsEmpty() || !originAllowed )
    {
        aError = _( "Remote asset URL origin must match the selected provider." );
        return false;
    }

    return true;
}


bool defaultFetchHandler( const wxString& aUrl, REMOTE_SYMBOL_FETCH_RESPONSE& aResponse, wxString& aError )
{
    KICAD_CURL_EASY curl;
    curl.SetUserAgent( "KiCad-RemoteProvider/1.0" );
    curl.SetFollowRedirects( false );
    curl.SetConnectTimeout( 10 );

    if( !curl.SetURL( aUrl.ToStdString() ) )
    {
        aError = wxString::Format( _( "Unable to set download URL '%s'." ), aUrl );
        return false;
    }

    const int result = curl.Perform();

    if( result != CURLE_OK )
    {
        aError = wxString::FromUTF8( curl.GetErrorText( result ).c_str() );
        return false;
    }

    aResponse.status_code = curl.GetResponseStatusCode();

    char* contentType = nullptr;
    curl_easy_getinfo( static_cast<CURL*>( curl.GetCurl() ), CURLINFO_CONTENT_TYPE, &contentType );

    if( contentType )
        aResponse.content_type = wxString::FromUTF8( contentType );

    const std::string& buffer = curl.GetBuffer();
    aResponse.payload.assign( buffer.begin(), buffer.end() );
    return true;
}


wxString sha256Hex( const std::vector<uint8_t>& aPayload )
{
    std::string hashHex;
    picosha2::hash256_hex_string( aPayload.begin(), aPayload.end(), hashHex );
    return wxString::FromUTF8( hashHex.c_str() );
}
} // namespace


REMOTE_SYMBOL_DOWNLOAD_MANAGER::REMOTE_SYMBOL_DOWNLOAD_MANAGER() :
        m_handler( defaultFetchHandler )
{
}


REMOTE_SYMBOL_DOWNLOAD_MANAGER::REMOTE_SYMBOL_DOWNLOAD_MANAGER( FETCH_HANDLER aHandler ) :
        m_handler( std::move( aHandler ) )
{
}


bool REMOTE_SYMBOL_DOWNLOAD_MANAGER::DownloadAndVerify( const REMOTE_PROVIDER_METADATA& aProvider,
                                                        const REMOTE_PROVIDER_PART_ASSET& aAsset,
                                                        long long aRemainingBudget,
                                                        REMOTE_SYMBOL_FETCHED_ASSET& aFetched,
                                                        wxString& aError ) const
{
    aFetched = REMOTE_SYMBOL_FETCHED_ASSET();
    aError.clear();

    if( aAsset.size_bytes <= 0 )
    {
        aError = _( "Remote asset manifest declared an invalid size." );
        return false;
    }

    if( aAsset.sha256.IsEmpty() )
    {
        aError = _( "Remote asset manifest must declare sha256 for URL-based downloads." );
        return false;
    }

    if( !validateAssetUrl( aProvider, aAsset.download_url, aError ) )
        return false;

    if( aRemainingBudget >= 0 && aAsset.size_bytes > aRemainingBudget )
    {
        aError = _( "Remote asset exceeds the provider download limit." );
        return false;
    }

    REMOTE_SYMBOL_FETCH_RESPONSE response;

    if( !m_handler( aAsset.download_url, response, aError ) )
        return false;

    if( response.status_code < 200 || response.status_code >= 300 )
    {
        aError = wxString::Format( _( "Remote download failed with HTTP %d." ), response.status_code );
        return false;
    }

    if( response.content_type != aAsset.content_type )
    {
        aError = _( "Remote asset content type did not match the manifest." );
        return false;
    }

    if( static_cast<long long>( response.payload.size() ) != aAsset.size_bytes )
    {
        aError = _( "Remote asset size did not match the manifest." );
        return false;
    }

    if( aRemainingBudget >= 0 && static_cast<long long>( response.payload.size() ) > aRemainingBudget )
    {
        aError = _( "Remote asset exceeds the remaining download limit." );
        return false;
    }

    if( !aAsset.sha256.IsEmpty() && !sha256Hex( response.payload ).IsSameAs( aAsset.sha256, false ) )
    {
        aError = _( "Remote asset digest did not match the manifest." );
        return false;
    }

    aFetched.content_type = response.content_type;
    aFetched.payload = std::move( response.payload );
    return true;
}
