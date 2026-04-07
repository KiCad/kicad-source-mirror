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
 * along with this program; if not, see <http://www.gnu.org/licenses/>.
 */

#include "panel_remote_symbol.h"
#include "../remote_symbol_import_utils.h"

#include <bitmaps.h>
#include <build_version.h>
#include <common.h>
#include <dialogs/dialog_remote_symbol_config.h>
#include <eeschema_settings.h>
#include <kiplatform/webview.h>
#include <ki_exception.h>
#include <lib_symbol.h>
#include <libraries/library_manager.h>
#include <libraries/symbol_library_adapter.h>
#include <pgm_base.h>
#include <project_sch.h>
#include <remote_provider_utils.h>
#include <sch_edit_frame.h>
#include <string_utils.h>
#include <sch_io/sch_io_mgr.h>
#include <settings/settings_manager.h>
#include <tool/tool_manager.h>
#include <tools/sch_actions.h>
#include <widgets/bitmap_button.h>
#include <widgets/webview_panel.h>
#include <oauth/oauth_pkce.h>

#ifndef wxUSE_BASE64
#define wxUSE_BASE64 1
#endif
#include <wx/base64.h>

#include <wx/choice.h>
#include <wx/datetime.h>
#include <wx/dir.h>
#include <wx/ffile.h>
#include <wx/filefn.h>
#include <wx/intl.h>
#include <wx/log.h>
#include <wx/mstream.h>
#include <wx/settings.h>
#include <wx/sizer.h>
#include <wx/stdpaths.h>
#include <wx/webview.h>

#include <algorithm>

#include <nlohmann/json.hpp>
#include <zstd.h>


bool PANEL_REMOTE_SYMBOL::decodeBase64Payload( const std::string& aEncoded,
                                               std::vector<uint8_t>& aOutput,
                                               wxString& aError ) const
{
    if( aEncoded.empty() )
    {
        aError = _( "Missing payload data." );
        return false;
    }

    wxMemoryBuffer buffer = wxBase64Decode( wxString::FromUTF8( aEncoded.c_str() ) );

    if( buffer.IsEmpty() )
    {
        aError = _( "Failed to decode base64 payload." );
        return false;
    }

    aOutput.resize( buffer.GetDataLen() );
    memcpy( aOutput.data(), buffer.GetData(), buffer.GetDataLen() );
    return true;
}


bool PANEL_REMOTE_SYMBOL::decompressIfNeeded( const std::string& aCompression,
                                              const std::vector<uint8_t>& aInput,
                                              std::vector<uint8_t>& aOutput,
                                              wxString& aError ) const
{
    if( aCompression.empty() || aCompression == "NONE" )
    {
        aOutput = aInput;
        return true;
    }

    if( aCompression != "ZSTD" )
    {
        aError = wxString::Format( _( "Unsupported compression '%s'." ),
                                   wxString::FromUTF8( aCompression ) );
        return false;
    }

    if( aInput.empty() )
    {
        aError = _( "Compressed payload was empty." );
        return false;
    }

    unsigned long long expectedSize = ZSTD_getFrameContentSize( aInput.data(), aInput.size() );

    if( expectedSize == ZSTD_CONTENTSIZE_ERROR || expectedSize == ZSTD_CONTENTSIZE_UNKNOWN )
        expectedSize = static_cast<unsigned long long>( aInput.size() ) * 4;

    static constexpr unsigned long long FALLBACK_MAX = 64ULL * 1024 * 1024;

    unsigned long long maxBytes = ( m_hasSelectedProviderMetadata
                                    && m_selectedProviderMetadata.max_download_bytes > 0 )
                                          ? static_cast<unsigned long long>(
                                                    m_selectedProviderMetadata.max_download_bytes )
                                          : FALLBACK_MAX;

    if( expectedSize > maxBytes )
    {
        aError = wxString::Format( _( "Decompressed size %llu exceeds limit %llu." ),
                                   expectedSize, maxBytes );
        return false;
    }

    aOutput.resize( expectedSize );

    size_t decompressed = ZSTD_decompress( aOutput.data(), expectedSize, aInput.data(), aInput.size() );

    if( ZSTD_isError( decompressed ) )
    {
        aError = wxString::Format( _( "ZSTD decompression failed: %s" ),
                                   wxString::FromUTF8( ZSTD_getErrorName( decompressed ) ) );
        return false;
    }

    aOutput.resize( decompressed );
    return true;
}


wxString PANEL_REMOTE_SYMBOL::sanitizeForScript( const std::string& aJson ) const
{
    wxString script = wxString::FromUTF8( aJson.c_str() );
    script.Replace( "\\", "\\\\" );
    script.Replace( "'", "\\'" );
    return script;
}


std::unique_ptr<LIB_SYMBOL> PANEL_REMOTE_SYMBOL::loadSymbolFromPayload( const std::vector<uint8_t>& aPayload,
                                                                        const wxString& aLibItemName,
                                                                        wxString& aError ) const
{
    if( aPayload.empty() )
    {
        aError = _( "Symbol payload was empty." );
        return nullptr;
    }

    wxString tempPath = wxFileName::CreateTempFileName( wxS( "remote_symbol" ) );

    if( tempPath.IsEmpty() )
    {
        aError = _( "Unable to create a temporary file for the symbol payload." );
        return nullptr;
    }

    wxFileName tempFile( tempPath );
    wxFFile    file( tempFile.GetFullPath(), wxS( "wb" ) );

    if( !file.IsOpened() )
    {
        aError = _( "Unable to create a temporary file for the symbol payload." );
        wxRemoveFile( tempFile.GetFullPath() );
        return nullptr;
    }

    if( file.Write( aPayload.data(), aPayload.size() ) != aPayload.size() )
    {
        aError = _( "Failed to write the temporary symbol payload." );
        file.Close();
        wxRemoveFile( tempFile.GetFullPath() );
        return nullptr;
    }

    file.Close();

    IO_RELEASER<SCH_IO> plugin( SCH_IO_MGR::FindPlugin( SCH_IO_MGR::SCH_KICAD ) );

    if( !plugin )
    {
        aError = _( "Unable to access the KiCad symbol plugin." );
        wxRemoveFile( tempFile.GetFullPath() );
        return nullptr;
    }

    std::unique_ptr<LIB_SYMBOL> symbol;

    try
    {
        LIB_SYMBOL* loaded = plugin->LoadSymbol( tempFile.GetFullPath(), aLibItemName );

        if( loaded )
            symbol = std::make_unique<LIB_SYMBOL>( *loaded );
        else
            aError = _( "Symbol payload did not include the expected symbol." );
    }
    catch( const IO_ERROR& e )
    {
        aError = wxString::Format( _( "Unable to decode the symbol payload: %s" ), e.What() );
    }

    wxRemoveFile( tempFile.GetFullPath() );
    return symbol;
}


PANEL_REMOTE_SYMBOL::PANEL_REMOTE_SYMBOL( SCH_EDIT_FRAME* aParent ) :
        wxPanel( aParent ),
        m_frame( aParent ),
        m_dataSourceChoice( nullptr ),
        m_configButton( nullptr ),
        m_refreshButton( nullptr ),
        m_webView( nullptr ),
        m_selectedProviderIndex( wxNOT_FOUND ),
        m_hasSelectedProviderMetadata( false ),
        m_messageIdCounter( 0 ),
        m_pendingHandshake( false )
{
    wxBoxSizer* topSizer = new wxBoxSizer( wxVERTICAL );
    wxBoxSizer* controlsSizer = new wxBoxSizer( wxHORIZONTAL );

    m_dataSourceChoice = new wxChoice( this, wxID_ANY );
    m_dataSourceChoice->SetMinSize( FromDIP( wxSize( 180, -1 ) ) );
    controlsSizer->Add( m_dataSourceChoice, 1, wxEXPAND | wxRIGHT, FromDIP( 4 ) );

    m_refreshButton = new BITMAP_BUTTON( this, wxID_ANY );
    m_refreshButton->SetBitmap( KiBitmapBundle( BITMAPS::reload ) );
    m_refreshButton->SetToolTip( _( "Reload current provider page" ) );
    controlsSizer->Add( m_refreshButton, 0, wxRIGHT, FromDIP( 2 ) );

    m_configButton = new BITMAP_BUTTON( this, wxID_ANY );
    m_configButton->SetBitmap( KiBitmapBundle( BITMAPS::config ) );
    m_configButton->SetToolTip( _( "Configure remote providers" ) );
    controlsSizer->Add( m_configButton, 0, wxALIGN_CENTER_VERTICAL );

    topSizer->Add( controlsSizer, 0, wxEXPAND | wxALL, FromDIP( 4 ) );
    SetSizer( topSizer );

    m_dataSourceChoice->Bind( wxEVT_CHOICE, &PANEL_REMOTE_SYMBOL::onDataSourceChanged, this );
    m_configButton->Bind( wxEVT_BUTTON, &PANEL_REMOTE_SYMBOL::onConfigure, this );
    m_refreshButton->Bind( wxEVT_BUTTON, &PANEL_REMOTE_SYMBOL::onRefresh, this );
    Bind( EVT_OAUTH_LOOPBACK_RESULT, &PANEL_REMOTE_SYMBOL::onOAuthLoopback, this );
    Bind( wxEVT_SYS_COLOUR_CHANGED, wxSysColourChangedEventHandler( PANEL_REMOTE_SYMBOL::onDarkModeToggle ), this );

    RefreshDataSources();
}


PANEL_REMOTE_SYMBOL::~PANEL_REMOTE_SYMBOL()
{
    SaveCookies();
    Unbind( wxEVT_SYS_COLOUR_CHANGED, wxSysColourChangedEventHandler( PANEL_REMOTE_SYMBOL::onDarkModeToggle ), this );
}


void PANEL_REMOTE_SYMBOL::Activate()
{
    ensureWebView();
    RefreshDataSources();
}


void PANEL_REMOTE_SYMBOL::ensureWebView()
{
    if( m_webView )
        return;

    m_webView = new WEBVIEW_PANEL( this );
    m_webView->AddMessageHandler( wxS( "kicad" ),
                                  [this]( const wxString& aPayload )
                                  {
                                      onKicadMessage( aPayload );
                                  } );
    m_webView->SetHandleExternalLinks( true );
    m_webView->BindLoadedEvent();

    if( wxWebView* browser = m_webView->GetWebView() )
        browser->Bind( wxEVT_WEBVIEW_LOADED, &PANEL_REMOTE_SYMBOL::onWebViewLoaded, this );

    GetSizer()->Add( m_webView, 1, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, FromDIP( 2 ) );
    Layout();
}


void PANEL_REMOTE_SYMBOL::BindWebViewLoaded()
{
    if( wxWebView* browser = m_webView ? m_webView->GetWebView() : nullptr )
        browser->Bind( wxEVT_WEBVIEW_LOADED, &PANEL_REMOTE_SYMBOL::onWebViewLoaded, this );
}


wxFileName PANEL_REMOTE_SYMBOL::cookieFilePath( const wxString& aProviderId ) const
{
    wxFileName cookieFile( wxStandardPaths::Get().GetUserDataDir(), wxEmptyString );
    cookieFile.AppendDir( wxS( "remote-provider-cookies" ) );
    cookieFile.Mkdir( wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL );
    cookieFile.SetFullName( SanitizeRemoteFileComponent( aProviderId, wxS( "provider" ), true ) + wxS( ".json" ) );
    return cookieFile;
}


void PANEL_REMOTE_SYMBOL::SaveCookies()
{
    if( !m_webView || m_selectedProviderIndex == wxNOT_FOUND || m_selectedProviderIndex >= static_cast<int>( m_providerEntries.size() ) )
        return;

    if( wxWebView* browser = m_webView->GetWebView() )
    {
        const wxFileName cookieFile = cookieFilePath( m_providerEntries[m_selectedProviderIndex].provider_id );
        KIPLATFORM::WEBVIEW::SaveCookies( browser, cookieFile.GetFullPath() );
    }
}


void PANEL_REMOTE_SYMBOL::LoadCookies()
{
    if( !m_webView || m_selectedProviderIndex == wxNOT_FOUND || m_selectedProviderIndex >= static_cast<int>( m_providerEntries.size() ) )
        return;

    if( wxWebView* browser = m_webView->GetWebView() )
    {
        const wxFileName cookieFile = cookieFilePath( m_providerEntries[m_selectedProviderIndex].provider_id );

        if( cookieFile.FileExists() )
            KIPLATFORM::WEBVIEW::LoadCookies( browser, cookieFile.GetFullPath() );
    }
}


void PANEL_REMOTE_SYMBOL::clearCookies( bool aDeleteSavedCookieFile )
{
    if( wxWebView* browser = m_webView ? m_webView->GetWebView() : nullptr )
        KIPLATFORM::WEBVIEW::DeleteCookies( browser );

    if( aDeleteSavedCookieFile && m_selectedProviderIndex != wxNOT_FOUND
        && m_selectedProviderIndex < static_cast<int>( m_providerEntries.size() ) )
    {
        const wxFileName cookieFile = cookieFilePath( m_providerEntries[m_selectedProviderIndex].provider_id );

        if( cookieFile.FileExists() )
            wxRemoveFile( cookieFile.GetFullPath() );
    }
}


void PANEL_REMOTE_SYMBOL::RefreshDataSources()
{
    SaveCookies();

    EESCHEMA_SETTINGS* settings = GetAppSettings<EESCHEMA_SETTINGS>( "eeschema" );

    m_providerEntries.clear();
    m_dataSourceChoice->Clear();
    m_selectedProviderIndex = wxNOT_FOUND;
    m_hasSelectedProviderMetadata = false;

    if( settings )
        m_providerEntries = settings->m_RemoteSymbol.providers;

    for( const REMOTE_PROVIDER_ENTRY& entry : m_providerEntries )
    {
        wxString label = entry.display_name_override.IsEmpty() ? entry.metadata_url : entry.display_name_override;
        m_dataSourceChoice->Append( label );
    }

    if( m_providerEntries.empty() )
    {
        m_dataSourceChoice->Enable( false );

        if( m_webView )
            showMessage( _( "No remote providers configured." ) );

        return;
    }

    m_dataSourceChoice->Enable( true );

    int selected = 0;

    if( settings && !settings->m_RemoteSymbol.last_used_provider_id.IsEmpty() )
    {
        for( size_t ii = 0; ii < m_providerEntries.size(); ++ii )
        {
            if( m_providerEntries[ii].provider_id == settings->m_RemoteSymbol.last_used_provider_id )
            {
                selected = static_cast<int>( ii );
                break;
            }
        }
    }

    m_dataSourceChoice->SetSelection( selected );

    if( m_webView )
        loadProvider( selected );
}


void PANEL_REMOTE_SYMBOL::onDataSourceChanged( wxCommandEvent& aEvent )
{
    loadProvider( aEvent.GetSelection() );
}


void PANEL_REMOTE_SYMBOL::onConfigure( wxCommandEvent& aEvent )
{
    wxUnusedVar( aEvent );

    DIALOG_REMOTE_SYMBOL_CONFIG dlg( this );
    dlg.ShowModal();
    RefreshDataSources();
}


void PANEL_REMOTE_SYMBOL::onRefresh( wxCommandEvent& aEvent )
{
    wxUnusedVar( aEvent );

    if( m_selectedProviderIndex != wxNOT_FOUND )
        loadProvider( m_selectedProviderIndex );
}


void PANEL_REMOTE_SYMBOL::onDarkModeToggle( wxSysColourChangedEvent& aEvent )
{
    aEvent.Skip();

    if( m_selectedProviderIndex != wxNOT_FOUND )
        loadProvider( m_selectedProviderIndex );
}


bool PANEL_REMOTE_SYMBOL::loadProvider( int aIndex )
{
    if( aIndex < 0 || aIndex >= static_cast<int>( m_providerEntries.size() ) )
        return false;

    SaveCookies();
    clearCookies( false );

    REMOTE_PROVIDER_METADATA metadata;
    REMOTE_PROVIDER_ERROR    error;

    if( !m_providerClient.DiscoverProvider( m_providerEntries[aIndex].metadata_url, metadata, error ) )
    {
        showMessage( error.message.IsEmpty() ? _( "Unable to load remote provider metadata." ) : error.message );
        return false;
    }

    m_selectedProviderIndex = aIndex;
    m_selectedProviderMetadata = metadata;
    m_hasSelectedProviderMetadata = true;
    m_pendingHandshake = true;

    if( EESCHEMA_SETTINGS* settings = GetAppSettings<EESCHEMA_SETTINGS>( "eeschema" ) )
        settings->m_RemoteSymbol.last_used_provider_id = m_providerEntries[aIndex].provider_id;

    LoadCookies();
    loadProviderPage( metadata, loadAccessToken( m_providerEntries[aIndex] ) );
    return true;
}


void PANEL_REMOTE_SYMBOL::loadProviderPage( const REMOTE_PROVIDER_METADATA& aMetadata,
                                            const wxString& aAccessToken )
{
    if( !m_webView )
        return;

    m_pendingHandshake = true;

    if( !aAccessToken.IsEmpty() && !aMetadata.session_bootstrap_url.IsEmpty() )
    {
        bootstrapAuthenticatedSession( aMetadata, aAccessToken );
        return;
    }

    m_webView->LoadURL( aMetadata.panel_url );
}


void PANEL_REMOTE_SYMBOL::bootstrapAuthenticatedSession( const REMOTE_PROVIDER_METADATA& aMetadata,
                                                         const wxString& aAccessToken )
{
    wxString              nonceUrl;
    REMOTE_PROVIDER_ERROR error;

    if( !m_providerClient.ExchangeBootstrapNonce( aMetadata, aAccessToken, nonceUrl, error ) )
    {
        wxLogWarning( "Session bootstrap nonce exchange failed: %s", error.message );
        m_webView->LoadURL( aMetadata.panel_url );
        return;
    }

    m_webView->LoadURL( nonceUrl );
}


void PANEL_REMOTE_SYMBOL::showMessage( const wxString& aMessage )
{
    if( !m_webView )
        return;

    wxColour bgColour = wxSystemSettings::GetColour( wxSYS_COLOUR_WINDOW );
    wxColour fgColour = wxSystemSettings::GetColour( wxSYS_COLOUR_WINDOWTEXT );

    wxString html;
    html << wxS( "<html><head><style>" )
         << wxString::Format( wxS( "body { background-color: #%02x%02x%02x; color: #%02x%02x%02x; "
                                   "font-family: system-ui, sans-serif; padding: 10px; }" ),
                              bgColour.Red(), bgColour.Green(), bgColour.Blue(),
                              fgColour.Red(), fgColour.Green(), fgColour.Blue() )
         << wxS( "</style></head><body><p>" ) << EscapeHTML( aMessage )
         << wxS( "</p></body></html>" );

    m_webView->SetPage( html );
}


void PANEL_REMOTE_SYMBOL::onWebViewLoaded( wxWebViewEvent& aEvent )
{
    if( m_pendingHandshake )
    {
        const wxString url = aEvent.GetURL();

        if( !url.StartsWith( wxS( "file://" ) ) )
        {
            CallAfter( [this]()
            {
                m_pendingHandshake = false;
                beginSessionHandshake();
            } );
        }
    }

    aEvent.Skip();
}


void PANEL_REMOTE_SYMBOL::beginSessionHandshake()
{
    if( !m_webView )
        return;

    m_sessionId = KIID();
    m_messageIdCounter = 0;

    nlohmann::json params = nlohmann::json::object();
    params["client_name"] = "KiCad";
    params["client_version"] = GetSemanticVersion().ToStdString();
    params["supported_versions"] = { REMOTE_SYMBOL_SESSION_VERSION };
    sendRpcNotification( wxS( "NEW_SESSION" ), std::move( params ) );
}


void PANEL_REMOTE_SYMBOL::sendRpcEnvelope( nlohmann::json aEnvelope )
{
    if( !m_webView )
        return;

    const wxString script = wxString::Format( wxS( "window.kiclient.postMessage('%s');" ),
                                              sanitizeForScript( aEnvelope.dump() ) );
    m_webView->RunScriptAsync( script );
}


void PANEL_REMOTE_SYMBOL::sendRpcReply( const wxString& aCommand, int aResponseTo,
                                        nlohmann::json aParameters )
{
    nlohmann::json envelope = nlohmann::json::object();
    envelope["version"] = REMOTE_SYMBOL_SESSION_VERSION;
    envelope["session_id"] = m_sessionId.AsStdString();
    envelope["message_id"] = ++m_messageIdCounter;
    envelope["command"] = aCommand.ToStdString();
    envelope["status"] = "OK";
    envelope["response_to"] = aResponseTo;

    if( !aParameters.is_null() && !aParameters.empty() )
        envelope["parameters"] = std::move( aParameters );

    sendRpcEnvelope( std::move( envelope ) );
}


void PANEL_REMOTE_SYMBOL::sendRpcError( const wxString& aCommand, int aResponseTo,
                                        const wxString& aErrorCode, const wxString& aErrorMessage )
{
    nlohmann::json envelope = nlohmann::json::object();
    envelope["version"] = REMOTE_SYMBOL_SESSION_VERSION;
    envelope["session_id"] = m_sessionId.AsStdString();
    envelope["message_id"] = ++m_messageIdCounter;
    envelope["command"] = aCommand.ToStdString();
    envelope["status"] = "ERROR";
    envelope["response_to"] = aResponseTo;
    envelope["error_code"] = aErrorCode.ToStdString();
    envelope["error_message"] = aErrorMessage.ToStdString();
    sendRpcEnvelope( std::move( envelope ) );
}


void PANEL_REMOTE_SYMBOL::sendRpcNotification( const wxString& aCommand, nlohmann::json aParameters )
{
    nlohmann::json envelope = nlohmann::json::object();
    envelope["version"] = REMOTE_SYMBOL_SESSION_VERSION;
    envelope["session_id"] = m_sessionId.AsStdString();
    envelope["message_id"] = ++m_messageIdCounter;
    envelope["command"] = aCommand.ToStdString();
    envelope["status"] = "OK";

    if( !aParameters.is_null() && !aParameters.empty() )
        envelope["parameters"] = std::move( aParameters );

    sendRpcEnvelope( std::move( envelope ) );
}


void PANEL_REMOTE_SYMBOL::onKicadMessage( const wxString& aMessage )
{
    wxScopedCharBuffer utf8 = aMessage.ToUTF8();

    if( !utf8 || utf8.length() == 0 )
        return;

    try
    {
        handleRpcMessage( nlohmann::json::parse( utf8.data() ) );
    }
    catch( const std::exception& e )
    {
        wxLogWarning( "Remote symbol RPC parse error: %s", e.what() );
    }
}


std::optional<OAUTH_TOKEN_SET> PANEL_REMOTE_SYMBOL::loadTokens( const REMOTE_PROVIDER_ENTRY& aProvider ) const
{
    return m_tokenStore.LoadTokens( aProvider.provider_id, wxS( "default" ) );
}


wxString PANEL_REMOTE_SYMBOL::loadAccessToken( const REMOTE_PROVIDER_ENTRY& aProvider )
{
    std::optional<OAUTH_TOKEN_SET> tokens = loadTokens( aProvider );

    if( !tokens )
        return wxString();

    const long long now = static_cast<long long>( wxDateTime::Now().GetTicks() );

    if( tokens->expires_at == 0 || tokens->expires_at > now + 60 )
        return tokens->access_token;

    if( tokens->refresh_token.IsEmpty() || !m_hasSelectedProviderMetadata )
        return wxString();

    REMOTE_PROVIDER_OAUTH_SERVER_METADATA oauthMetadata;
    REMOTE_PROVIDER_ERROR                 error;

    if( !m_providerClient.FetchOAuthServerMetadata( m_selectedProviderMetadata, oauthMetadata, error ) )
        return wxString();

    OAUTH_TOKEN_SET refreshed;

    if( !m_providerClient.RefreshAccessToken( oauthMetadata, m_selectedProviderMetadata.auth.client_id,
                                              tokens->refresh_token, refreshed, error ) )
    {
        return wxString();
    }

    if( refreshed.refresh_token.IsEmpty() )
        refreshed.refresh_token = tokens->refresh_token;

    if( !m_tokenStore.StoreTokens( aProvider.provider_id, wxS( "default" ), refreshed ) )
        return wxString();

    return refreshed.access_token;
}


bool PANEL_REMOTE_SYMBOL::startInteractiveLogin( const REMOTE_PROVIDER_ENTRY& aProvider,
                                                 const REMOTE_PROVIDER_METADATA& aMetadata,
                                                 wxString& aError )
{
    aError.clear();

    if( aMetadata.auth.type != REMOTE_PROVIDER_AUTH_TYPE::OAUTH2 )
        return true;

    if( m_oauthLoopbackServer )
    {
        aError = _( "A remote provider sign-in flow is already in progress." );
        return false;
    }

    REMOTE_PROVIDER_ERROR error;

    if( !m_providerClient.FetchOAuthServerMetadata( aMetadata, m_pendingOAuthMetadata, error ) )
    {
        aError = error.message;
        return false;
    }

    m_pendingOAuthSession = OAUTH_SESSION();
    m_pendingOAuthSession.authorization_endpoint = m_pendingOAuthMetadata.authorization_endpoint;
    m_pendingOAuthSession.client_id = aMetadata.auth.client_id;

    wxArrayString scopes;

    for( const wxString& scope : aMetadata.auth.scopes )
        scopes.Add( scope );

    m_pendingOAuthSession.scope = wxJoin( scopes, ' ' );
    m_pendingOAuthSession.state = OAUTH_PKCE::GenerateState();
    m_pendingOAuthSession.code_verifier = OAUTH_PKCE::GenerateCodeVerifier();
    m_pendingProviderId = aProvider.provider_id;
    m_oauthLoopbackServer = std::make_unique<OAUTH_LOOPBACK_SERVER>(
            this, wxS( "/oauth/callback" ), m_pendingOAuthSession.state );

    if( !m_oauthLoopbackServer->Start() )
    {
        m_oauthLoopbackServer.reset();
        m_pendingProviderId.clear();
        aError = _( "Unable to start the local OAuth callback listener." );
        return false;
    }

    m_pendingOAuthSession.redirect_uri = m_oauthLoopbackServer->GetRedirectUri();

    if( !wxLaunchDefaultBrowser( m_pendingOAuthSession.BuildAuthorizationUrl(), wxBROWSER_NEW_WINDOW ) )
    {
        m_oauthLoopbackServer.reset();
        m_pendingProviderId.clear();
        aError = _( "Unable to open the system browser for sign-in." );
        return false;
    }

    return true;
}


bool PANEL_REMOTE_SYMBOL::signOutProvider( const REMOTE_PROVIDER_ENTRY& aProvider, wxString& aError )
{
    aError.clear();

    if( std::optional<OAUTH_TOKEN_SET> tokens = loadTokens( aProvider ); tokens )
    {
        REMOTE_PROVIDER_OAUTH_SERVER_METADATA oauthMetadata;
        REMOTE_PROVIDER_ERROR                 error;

        if( m_hasSelectedProviderMetadata
            && m_providerClient.FetchOAuthServerMetadata( m_selectedProviderMetadata, oauthMetadata, error ) )
        {
            const wxString tokenToRevoke = !tokens->refresh_token.IsEmpty() ? tokens->refresh_token
                                                                             : tokens->access_token;
            REMOTE_PROVIDER_ERROR revokeError;
            m_providerClient.RevokeToken( oauthMetadata, m_selectedProviderMetadata.auth.client_id,
                                          tokenToRevoke, revokeError );
        }
    }

    if( !m_tokenStore.DeleteTokens( aProvider.provider_id, wxS( "default" ) ) )
    {
        aError = _( "Failed to delete stored remote provider tokens." );
        return false;
    }

    clearCookies();

    if( EESCHEMA_SETTINGS* settings = GetAppSettings<EESCHEMA_SETTINGS>( "eeschema" ) )
    {
        if( REMOTE_PROVIDER_ENTRY* provider = settings->m_RemoteSymbol.FindProviderById( aProvider.provider_id ) )
        {
            provider->last_account_label.clear();
            provider->last_auth_status = wxS( "signed_out" );
        }
    }

    return true;
}


void PANEL_REMOTE_SYMBOL::handleRpcMessage( const nlohmann::json& aMessage )
{
    if( !aMessage.is_object() )
        return;

    const wxString command = RemoteProviderJsonString( aMessage, "command" );

    if( command.IsEmpty() )
        return;

    auto messageIdIt = aMessage.find( "message_id" );

    if( messageIdIt == aMessage.end() || !messageIdIt->is_number_integer() )
        return;

    const int messageId = messageIdIt->get<int>();
    const int version = aMessage.value( "version", 0 );

    if( version != REMOTE_SYMBOL_SESSION_VERSION )
    {
        sendRpcError( command, messageId, wxS( "UNSUPPORTED_VERSION" ),
                          wxString::Format( _( "Unsupported RPC version %d." ), version ) );
        return;
    }

    const wxString sessionId = RemoteProviderJsonString( aMessage, "session_id" );

    if( sessionId.IsEmpty() )
    {
        sendRpcError( command, messageId, wxS( "INVALID_PARAMETERS" ),
                          _( "Missing session identifier." ) );
        return;
    }

    if( !sessionId.IsSameAs( m_sessionId.AsString() ) )
    {
        sendRpcError( command, messageId, wxS( "SESSION_MISMATCH" ),
                          _( "Session identifier did not match the active provider session." ) );
        return;
    }

    nlohmann::json params = nlohmann::json::object();
    auto paramsIt = aMessage.find( "parameters" );

    if( paramsIt != aMessage.end() && paramsIt->is_object() )
        params = *paramsIt;

    const std::string data = aMessage.value( "data", std::string() );

    if( command == wxS( "NEW_SESSION" ) )
    {
        nlohmann::json reply = nlohmann::json::object();
        reply["client_name"] = "KiCad";
        reply["client_version"] = GetSemanticVersion().ToStdString();
        reply["supported_versions"] = { REMOTE_SYMBOL_SESSION_VERSION };
        sendRpcReply( command, messageId, std::move( reply ) );
        return;
    }

    if( command == wxS( "GET_KICAD_VERSION" ) )
    {
        nlohmann::json reply = nlohmann::json::object();
        reply["kicad_version"] = GetSemanticVersion().ToStdString();
        sendRpcReply( command, messageId, std::move( reply ) );
        return;
    }

    if( command == wxS( "LIST_SUPPORTED_VERSIONS" ) )
    {
        nlohmann::json reply = nlohmann::json::object();
        reply["supported_versions"] = { REMOTE_SYMBOL_SESSION_VERSION };
        sendRpcReply( command, messageId, std::move( reply ) );
        return;
    }

    if( command == wxS( "CAPABILITIES" ) )
    {
        nlohmann::json reply = nlohmann::json::object();
        reply["commands"] = { "NEW_SESSION", "GET_KICAD_VERSION", "LIST_SUPPORTED_VERSIONS",
                              "CAPABILITIES", "GET_SOURCE_INFO", "REMOTE_LOGIN", "DL_SYMBOL",
                              "DL_COMPONENT", "DL_FOOTPRINT", "DL_SPICE", "DL_3DMODEL",
                              "PLACE_COMPONENT" };
        reply["compression"] = { "NONE", "ZSTD" };
        reply["max_message_size"] = 0;
        sendRpcReply( command, messageId, std::move( reply ) );
        return;
    }

    if( command == wxS( "GET_SOURCE_INFO" ) )
    {
        nlohmann::json reply = nlohmann::json::object();
        reply["provider_id"] = m_selectedProviderIndex == wxNOT_FOUND
                                       ? std::string()
                                       : m_providerEntries[m_selectedProviderIndex].provider_id.ToStdString();
        reply["provider_name"] = m_hasSelectedProviderMetadata
                                         ? m_selectedProviderMetadata.provider_name.ToStdString()
                                         : std::string();
        reply["panel_url"] = m_hasSelectedProviderMetadata
                                     ? m_selectedProviderMetadata.panel_url.ToStdString()
                                     : std::string();

        bool authenticated = false;

        if( m_selectedProviderIndex != wxNOT_FOUND )
            authenticated = !loadAccessToken( m_providerEntries[m_selectedProviderIndex] ).IsEmpty();

        reply["authenticated"] = authenticated;
        reply["auth_type"] = m_hasSelectedProviderMetadata
                                     ? ( m_selectedProviderMetadata.auth.type == REMOTE_PROVIDER_AUTH_TYPE::OAUTH2
                                                 ? "oauth2"
                                                 : "none" )
                                     : "none";
        reply["supports_direct_downloads"] =
                m_hasSelectedProviderMetadata && m_selectedProviderMetadata.direct_downloads_v1;
        reply["supports_inline_payloads"] =
                m_hasSelectedProviderMetadata && m_selectedProviderMetadata.inline_payloads_v1;
        sendRpcReply( command, messageId, std::move( reply ) );
        return;
    }

    if( command == wxS( "REMOTE_LOGIN" ) )
    {
        if( m_selectedProviderIndex == wxNOT_FOUND || !m_hasSelectedProviderMetadata )
        {
            sendRpcError( command, messageId, wxS( "NO_PROVIDER" ),
                              _( "No remote provider is currently selected." ) );
            return;
        }

        const REMOTE_PROVIDER_ENTRY& provider = m_providerEntries[m_selectedProviderIndex];
        const bool signOut = params.value( "sign_out", false );
        const bool interactive = params.value( "interactive", true );

        if( signOut )
        {
            wxString error;

            if( !signOutProvider( provider, error ) )
            {
                sendRpcError( command, messageId, wxS( "SIGN_OUT_FAILED" ), error );
                return;
            }

            loadProvider( m_selectedProviderIndex );

            nlohmann::json reply = nlohmann::json::object();
            reply["authenticated"] = false;
            reply["signed_out"] = true;
            sendRpcReply( command, messageId, std::move( reply ) );
            return;
        }

        const wxString accessToken = loadAccessToken( provider );

        if( !accessToken.IsEmpty() )
        {
            nlohmann::json reply = nlohmann::json::object();
            reply["authenticated"] = true;
            reply["provider_id"] = provider.provider_id.ToStdString();
            sendRpcReply( command, messageId, std::move( reply ) );
            return;
        }

        if( m_selectedProviderMetadata.auth.type != REMOTE_PROVIDER_AUTH_TYPE::OAUTH2 )
        {
            nlohmann::json reply = nlohmann::json::object();
            reply["authenticated"] = false;
            reply["auth_type"] = "none";
            sendRpcReply( command, messageId, std::move( reply ) );
            return;
        }

        if( !interactive )
        {
            nlohmann::json reply = nlohmann::json::object();
            reply["authenticated"] = false;
            reply["started"] = false;
            sendRpcReply( command, messageId, std::move( reply ) );
            return;
        }

        wxString error;

        if( !startInteractiveLogin( provider, m_selectedProviderMetadata, error ) )
        {
            sendRpcError( command, messageId, wxS( "LOGIN_FAILED" ), error );
            return;
        }

        nlohmann::json reply = nlohmann::json::object();
        reply["authenticated"] = false;
        reply["started"] = true;
        sendRpcReply( command, messageId, std::move( reply ) );
        return;
    }

    if( command == wxS( "PLACE_COMPONENT" ) || command == wxS( "DL_COMPONENT" ) || command == wxS( "DL_SYMBOL" )
        || command == wxS( "DL_FOOTPRINT" ) || command == wxS( "DL_3DMODEL" ) || command == wxS( "DL_SPICE" ) )
    {
        const bool placeSymbol = command == wxS( "PLACE_COMPONENT" )
                                 || RemoteProviderJsonString( params, "mode" ).IsSameAs( wxS( "PLACE" ), false );
        const bool isComponent = command == wxS( "DL_COMPONENT" ) || command == wxS( "PLACE_COMPONENT" );
        wxString error;
        bool ok = false;

        if( isComponent && params.contains( "assets" ) && params["assets"].is_array() )
        {
            ok = receiveComponentManifest( params, placeSymbol, error );
        }
        else
        {
            const wxString compression = RemoteProviderJsonString( params, "compression" );
            std::vector<uint8_t> decoded;

            if( !decodeBase64Payload( data, decoded, error ) )
            {
                sendRpcError( command, messageId, wxS( "INVALID_PAYLOAD" ), error );
                return;
            }

            std::vector<uint8_t> payload;
            const std::string compressionStr =
                    compression.IsEmpty() ? std::string() : std::string( compression.ToUTF8().data() );

            if( !decompressIfNeeded( compressionStr, decoded, payload, error ) )
            {
                sendRpcError( command, messageId, wxS( "INVALID_PAYLOAD" ), error );
                return;
            }

            if( isComponent )
                ok = receiveComponent( params, payload, placeSymbol, error );
            else if( command == wxS( "DL_SYMBOL" ) )
                ok = receiveSymbol( params, payload, error );
            else if( command == wxS( "DL_FOOTPRINT" ) )
                ok = receiveFootprint( params, payload, error );
            else if( command == wxS( "DL_3DMODEL" ) )
                ok = receive3DModel( params, payload, error );
            else if( command == wxS( "DL_SPICE" ) )
                ok = receiveSPICEModel( params, payload, error );
        }

        if( ok )
            sendRpcReply( command, messageId );
        else
            sendRpcError( command, messageId, wxS( "IMPORT_FAILED" ),
                              error.IsEmpty() ? _( "Unable to process provider payload." ) : error );

        return;
    }

    sendRpcError( command, messageId, wxS( "UNKNOWN_COMMAND" ),
                      wxString::Format( _( "Command '%s' is not supported." ), command ) );
}


bool PANEL_REMOTE_SYMBOL::receiveSymbol( const nlohmann::json& aParams,
                                         const std::vector<uint8_t>& aPayload,
                                         wxString& aError )
{
    const wxString mode = RemoteProviderJsonString( aParams, "mode" );
    const bool placeAfterDownload = mode.IsSameAs( wxS( "PLACE" ), false );

    if( !mode.IsEmpty() && !mode.IsSameAs( wxS( "SAVE" ), false ) && !placeAfterDownload )
    {
        aError = wxString::Format( _( "Unsupported transfer mode '%s'." ), mode );
        return false;
    }

    if( !RemoteProviderJsonString( aParams, "content_type" ).IsSameAs( wxS( "KICAD_SYMBOL_V1" ), false ) )
    {
        aError = _( "Unsupported symbol payload type." );
        return false;
    }

    if( !m_frame )
    {
        aError = _( "No schematic editor is available to store symbols." );
        return false;
    }

    EESCHEMA_SETTINGS* settings = GetAppSettings<EESCHEMA_SETTINGS>( "eeschema" );

    if( !settings )
    {
        aError = _( "Unable to load schematic settings." );
        return false;
    }

    wxFileName baseDir;

    if( !EnsureRemoteDestinationRoot( baseDir, aError ) )
        return false;

    wxFileName symDir = baseDir;
    symDir.AppendDir( wxS( "symbols" ) );
    symDir.Mkdir( wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL );

    wxString libItemName = RemoteProviderJsonString( aParams, "name" );

    if( libItemName.IsEmpty() )
        libItemName = wxS( "symbol" );

    wxString libraryName = RemoteProviderJsonString( aParams, "library" );

    if( libraryName.IsEmpty() )
        libraryName = wxS( "symbols" );

    const wxString nickname = RemoteLibraryPrefix() + wxS( "_" )
                              + SanitizeRemoteFileComponent( libraryName, wxS( "symbols" ), true );

    wxFileName outFile( symDir );
    outFile.SetFullName( nickname + wxS( ".kicad_sym" ) );

    if( !EnsureRemoteLibraryEntry( LIBRARY_TABLE_TYPE::SYMBOL, outFile, nickname,
                                     settings->m_RemoteSymbol.add_to_global_table, true, aError ) )
        return false;

    SYMBOL_LIBRARY_ADAPTER* adapter = PROJECT_SCH::SymbolLibAdapter( &m_frame->Prj() );

    if( !adapter )
    {
        aError = _( "Unable to access the symbol library manager." );
        return false;
    }

    std::unique_ptr<LIB_SYMBOL> downloadedSymbol = loadSymbolFromPayload( aPayload, libItemName, aError );

    if( !downloadedSymbol )
        return false;

    downloadedSymbol->SetName( libItemName );
    LIB_ID savedId;
    savedId.SetLibNickname( nickname );
    savedId.SetLibItemName( libItemName );
    downloadedSymbol->SetLibId( savedId );

    if( adapter->SaveSymbol( nickname, downloadedSymbol.release(), true ) != SYMBOL_LIBRARY_ADAPTER::SAVE_OK )
    {
        aError = _( "Unable to save the downloaded symbol." );
        return false;
    }

    const LIBRARY_TABLE_SCOPE scope = settings->m_RemoteSymbol.add_to_global_table
                                              ? LIBRARY_TABLE_SCOPE::GLOBAL
                                              : LIBRARY_TABLE_SCOPE::PROJECT;

    // Reload the library entry to pick up the new file, then force a full load so the
    // library reaches LOADED state. Without the LoadOne call the library stays in LOADING
    // state and GetLibSymbol / the symbol chooser cannot see it.
    Pgm().GetLibraryManager().ReloadLibraryEntry( LIBRARY_TABLE_TYPE::SYMBOL, nickname, scope );
    adapter->LoadOne( nickname );

    if( placeAfterDownload )
        return PlaceRemoteDownloadedSymbol( m_frame, nickname, libItemName, aError );

    return true;
}


bool PANEL_REMOTE_SYMBOL::receiveComponent( const nlohmann::json& aParams,
                                            const std::vector<uint8_t>& aPayload,
                                            bool aPlaceSymbol, wxString& aError )
{
    nlohmann::json components;

    try
    {
        components = nlohmann::json::parse( aPayload.begin(), aPayload.end() );
    }
    catch( const std::exception& e )
    {
        aError = wxString::Format( _( "Failed to parse component list: %s" ), e.what() );
        return false;
    }

    if( !components.is_array() || components.empty() )
    {
        aError = _( "Component list must be a non-empty array." );
        return false;
    }

    const wxString libraryName = RemoteProviderJsonString( aParams, "library" );

    for( const nlohmann::json& entry : components )
    {
        if( !entry.is_object() )
        {
            aError = _( "Component entries must be objects." );
            return false;
        }

        std::string entryType = entry.value( "type", "" );

        if( entryType.empty() )
        {
            aError = _( "Component entry was missing a type." );
            return false;
        }

        std::transform( entryType.begin(), entryType.end(), entryType.begin(),
                        []( unsigned char c ) { return static_cast<char>( std::tolower( c ) ); } );

        std::vector<uint8_t> content;

        if( !decodeBase64Payload( entry.value( "content", std::string() ), content, aError ) )
            return false;

        const std::string compression = entry.value( "compression", std::string() );

        if( !compression.empty() && compression != "NONE" )
        {
            std::vector<uint8_t> decoded = content;

            if( !decompressIfNeeded( compression, decoded, content, aError ) )
                return false;
        }

        wxString entryName = wxString::FromUTF8( entry.value( "name", "" ) );
        nlohmann::json entryParams = nlohmann::json::object();

        if( !libraryName.IsEmpty() )
            entryParams["library"] = libraryName.ToStdString();

        if( !entryName.IsEmpty() )
            entryParams["name"] = entryName.ToStdString();

        bool ok = false;

        if( entryType == "symbol" )
        {
            entryParams["content_type"] = "KICAD_SYMBOL_V1";
            entryParams["mode"] = aPlaceSymbol ? "PLACE" : "SAVE";
            ok = receiveSymbol( entryParams, content, aError );
        }
        else if( entryType == "footprint" )
        {
            entryParams["content_type"] = "KICAD_FOOTPRINT_V1";
            entryParams["mode"] = "SAVE";
            ok = receiveFootprint( entryParams, content, aError );
        }
        else if( entryType == "3dmodel" )
        {
            entryParams["content_type"] = "KICAD_3D_MODEL_STEP";
            entryParams["mode"] = "SAVE";
            ok = receive3DModel( entryParams, content, aError );
        }
        else if( entryType == "spice" )
        {
            entryParams["content_type"] = "KICAD_SPICE_MODEL_V1";
            entryParams["mode"] = "SAVE";
            ok = receiveSPICEModel( entryParams, content, aError );
        }
        else
        {
            aError = wxString::Format( _( "Unsupported component type '%s'." ),
                                       wxString::FromUTF8( entryType.c_str() ) );
            return false;
        }

        if( !ok )
            return false;
    }

    return true;
}


bool PANEL_REMOTE_SYMBOL::receiveFootprint( const nlohmann::json& aParams,
                                            const std::vector<uint8_t>& aPayload,
                                            wxString& aError )
{
    if( !RemoteProviderJsonString( aParams, "content_type" ).IsSameAs( wxS( "KICAD_FOOTPRINT_V1" ), false ) )
    {
        aError = _( "Unsupported footprint payload type." );
        return false;
    }

    wxFileName baseDir;

    if( !EnsureRemoteDestinationRoot( baseDir, aError ) )
        return false;

    wxFileName fpRoot = baseDir;
    fpRoot.AppendDir( wxS( "footprints" ) );
    fpRoot.Mkdir( wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL );

    wxString footprintName = SanitizeRemoteFileComponent( RemoteProviderJsonString( aParams, "name" ), wxS( "footprint" ) );
    wxString libraryName = RemoteProviderJsonString( aParams, "library" );

    if( libraryName.IsEmpty() )
        libraryName = wxS( "footprints" );

    const wxString libNickname = RemoteLibraryPrefix() + wxS( "_" )
                                 + SanitizeRemoteFileComponent( libraryName, wxS( "footprints" ), true );

    wxFileName libDir = fpRoot;
    libDir.AppendDir( libNickname + wxS( ".pretty" ) );
    libDir.Mkdir( wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL );

    if( !footprintName.Lower().EndsWith( wxS( ".kicad_mod" ) ) )
        footprintName += wxS( ".kicad_mod" );

    wxFileName outFile( libDir );
    outFile.SetFullName( footprintName );

    if( !WriteRemoteBinaryFile( outFile, aPayload, aError ) )
        return false;

    EESCHEMA_SETTINGS* settings = GetAppSettings<EESCHEMA_SETTINGS>( "eeschema" );

    if( !settings )
    {
        aError = _( "Unable to load schematic settings." );
        return false;
    }

    if( !EnsureRemoteLibraryEntry( LIBRARY_TABLE_TYPE::FOOTPRINT, libDir, libNickname,
                                     settings->m_RemoteSymbol.add_to_global_table, true, aError ) )
        return false;

    const LIBRARY_TABLE_SCOPE scope = settings->m_RemoteSymbol.add_to_global_table
                                              ? LIBRARY_TABLE_SCOPE::GLOBAL
                                              : LIBRARY_TABLE_SCOPE::PROJECT;

    LIBRARY_MANAGER& libMgr = Pgm().GetLibraryManager();
    libMgr.ReloadLibraryEntry( LIBRARY_TABLE_TYPE::FOOTPRINT, libNickname, scope );
    libMgr.LoadLibraryEntry( LIBRARY_TABLE_TYPE::FOOTPRINT, libNickname );
    return true;
}


bool PANEL_REMOTE_SYMBOL::receive3DModel( const nlohmann::json& aParams,
                                          const std::vector<uint8_t>& aPayload,
                                          wxString& aError )
{
    wxFileName baseDir;

    if( !EnsureRemoteDestinationRoot( baseDir, aError ) )
        return false;

    wxFileName modelDir = baseDir;
    modelDir.AppendDir( RemoteLibraryPrefix() + wxS( "_3d" ) );
    modelDir.Mkdir( wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL );

    wxString fileName = RemoteProviderJsonString( aParams, "name" );

    if( fileName.IsEmpty() )
        fileName = RemoteLibraryPrefix() + wxS( "_model.step" );

    fileName = SanitizeRemoteFileComponent( fileName, RemoteLibraryPrefix() + wxS( "_model.step" ) );

    wxFileName outFile( modelDir );
    outFile.SetFullName( fileName );
    return WriteRemoteBinaryFile( outFile, aPayload, aError );
}


bool PANEL_REMOTE_SYMBOL::receiveSPICEModel( const nlohmann::json& aParams,
                                             const std::vector<uint8_t>& aPayload,
                                             wxString& aError )
{
    wxFileName baseDir;

    if( !EnsureRemoteDestinationRoot( baseDir, aError ) )
        return false;

    wxFileName spiceDir = baseDir;
    spiceDir.AppendDir( RemoteLibraryPrefix() + wxS( "_spice" ) );
    spiceDir.Mkdir( wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL );

    wxString fileName = RemoteProviderJsonString( aParams, "name" );

    if( fileName.IsEmpty() )
        fileName = RemoteLibraryPrefix() + wxS( "_model.cir" );

    fileName = SanitizeRemoteFileComponent( fileName, RemoteLibraryPrefix() + wxS( "_model.cir" ) );

    if( !fileName.Lower().EndsWith( wxS( ".cir" ) ) )
        fileName += wxS( ".cir" );

    wxFileName outFile( spiceDir );
    outFile.SetFullName( fileName );
    return WriteRemoteBinaryFile( outFile, aPayload, aError );
}


bool PANEL_REMOTE_SYMBOL::receiveComponentManifest( const nlohmann::json& aParams,
                                                    bool aPlaceSymbol, wxString& aError )
{
    if( !m_hasSelectedProviderMetadata )
    {
        aError = _( "Remote provider metadata is not loaded." );
        return false;
    }

    REMOTE_PROVIDER_PART_MANIFEST manifest;
    manifest.part_id = RemoteProviderJsonString( aParams, "part_id" );
    manifest.display_name = RemoteProviderJsonString( aParams, "display_name" );
    manifest.summary = RemoteProviderJsonString( aParams, "summary" );
    manifest.license = RemoteProviderJsonString( aParams, "license" );

    for( const nlohmann::json& assetJson : aParams.at( "assets" ) )
    {
        REMOTE_PROVIDER_PART_ASSET asset;
        asset.asset_type = RemoteProviderJsonString( assetJson, "asset_type" );
        asset.name = RemoteProviderJsonString( assetJson, "name" );
        asset.target_library = RemoteProviderJsonString( assetJson, "target_library" );
        asset.target_name = RemoteProviderJsonString( assetJson, "target_name" );
        asset.content_type = RemoteProviderJsonString( assetJson, "content_type" );
        asset.download_url = RemoteProviderJsonString( assetJson, "download_url" );
        asset.sha256 = RemoteProviderJsonString( assetJson, "sha256" );
        asset.required = assetJson.value( "required", false );

        auto sizeIt = assetJson.find( "size_bytes" );

        if( sizeIt != assetJson.end() && sizeIt->is_number_integer() )
            asset.size_bytes = sizeIt->get<long long>();

        if( asset.asset_type.IsEmpty() || asset.content_type.IsEmpty() || asset.download_url.IsEmpty()
            || asset.size_bytes <= 0 )
        {
            aError = _( "Manifest assets require asset_type, content_type, size_bytes, and download_url." );
            return false;
        }

        manifest.assets.push_back( asset );
    }

    REMOTE_SYMBOL_IMPORT_CONTEXT context;
    context.symbol_name = RemoteProviderJsonString( aParams, "symbol_name" );
    context.library_name = RemoteProviderJsonString( aParams, "library_name" );

    if( context.symbol_name.IsEmpty() )
    {
        for( const REMOTE_PROVIDER_PART_ASSET& asset : manifest.assets )
        {
            if( asset.asset_type == wxS( "symbol" ) )
            {
                context.symbol_name = asset.target_name;
                context.library_name = asset.target_library;
                break;
            }
        }
    }

    REMOTE_SYMBOL_IMPORT_JOB job( m_frame );
    return job.Import( m_selectedProviderMetadata, context, manifest, aPlaceSymbol, aError );
}


void PANEL_REMOTE_SYMBOL::onOAuthLoopback( wxCommandEvent& aEvent )
{
    m_oauthLoopbackServer.reset();

    if( !aEvent.GetInt() || m_pendingProviderId.IsEmpty() || !m_hasSelectedProviderMetadata )
    {
        m_pendingProviderId.clear();
        showMessage( _( "Remote provider sign-in was cancelled or failed." ) );
        return;
    }

    OAUTH_TOKEN_SET       tokens;
    REMOTE_PROVIDER_ERROR error;

    if( !m_providerClient.ExchangeAuthorizationCode( m_pendingOAuthMetadata, m_pendingOAuthSession,
                                                     aEvent.GetString(), tokens, error ) )
    {
        m_pendingProviderId.clear();
        showMessage( error.message.IsEmpty() ? _( "Remote provider sign-in failed." ) : error.message );
        return;
    }

    if( tokens.refresh_token.IsEmpty() )
    {
        if( std::optional<OAUTH_TOKEN_SET> existing = m_tokenStore.LoadTokens( m_pendingProviderId, wxS( "default" ) );
            existing )
        {
            tokens.refresh_token = existing->refresh_token;
        }
    }

    if( !m_tokenStore.StoreTokens( m_pendingProviderId, wxS( "default" ), tokens ) )
    {
        m_pendingProviderId.clear();
        showMessage( _( "Failed to store remote provider tokens securely." ) );
        return;
    }

    if( EESCHEMA_SETTINGS* settings = GetAppSettings<EESCHEMA_SETTINGS>( "eeschema" ) )
    {
        if( REMOTE_PROVIDER_ENTRY* provider = settings->m_RemoteSymbol.FindProviderById( m_pendingProviderId ) )
        {
            provider->last_account_label = wxS( "default" );
            provider->last_auth_status = wxS( "signed_in" );
        }
    }

    m_pendingProviderId.clear();

    if( m_selectedProviderIndex != wxNOT_FOUND )
        loadProvider( m_selectedProviderIndex );
}
