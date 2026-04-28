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

#ifndef PANEL_REMOTE_SYMBOL_H
#define PANEL_REMOTE_SYMBOL_H

#include <memory>
#include <optional>
#include <vector>

#include <kiid.h>
#include <lib_id.h>
#include <oauth/oauth_loopback_server.h>
#include <oauth/oauth_session.h>
#include <oauth/secure_token_store.h>
#include <remote_provider_client.h>
#include <remote_provider_metadata.h>
#include <remote_provider_settings.h>
#include <remote_symbol_import_job.h>
#include <wx/filename.h>
#include <wx/panel.h>

#include <nlohmann/json_fwd.hpp>


class BITMAP_BUTTON;
class LIB_SYMBOL;
class SCH_EDIT_FRAME;
class WEBVIEW_PANEL;
class wxChoice;
class wxCommandEvent;
class wxSysColourChangedEvent;
class wxWebViewEvent;

#define REMOTE_SYMBOL_SESSION_VERSION 1


class PANEL_REMOTE_SYMBOL : public wxPanel
{
public:
    explicit PANEL_REMOTE_SYMBOL( SCH_EDIT_FRAME* aParent );
    ~PANEL_REMOTE_SYMBOL() override;

    void Activate();
    void RefreshDataSources();
    bool HasDataSources() const { return !m_providerEntries.empty(); }
    void BindWebViewLoaded();
    void SaveCookies();
    void LoadCookies();

private:
    void ensureWebView();
    void onDataSourceChanged( wxCommandEvent& aEvent );
    void onConfigure( wxCommandEvent& aEvent );
    void onRefresh( wxCommandEvent& aEvent );
    void onWebViewLoaded( wxWebViewEvent& aEvent );
    void onDarkModeToggle( wxSysColourChangedEvent& aEvent );
    void onOAuthLoopback( wxCommandEvent& aEvent );

    bool loadProvider( int aIndex );
    void loadProviderPage( const REMOTE_PROVIDER_METADATA& aMetadata, const wxString& aAccessToken );
    void bootstrapAuthenticatedSession( const REMOTE_PROVIDER_METADATA& aMetadata,
                                        const wxString& aAccessToken );
    void showMessage( const wxString& aMessage );
    void onKicadMessage( const wxString& aMessage );
    void handleRpcMessage( const nlohmann::json& aMessage );
    void beginSessionHandshake();
    void sendRpcReply( const wxString& aCommand, int aResponseTo,
                       nlohmann::json aParameters = nlohmann::json::object() );
    void sendRpcError( const wxString& aCommand, int aResponseTo, const wxString& aErrorCode,
                       const wxString& aErrorMessage );
    void sendRpcNotification( const wxString& aCommand,
                              nlohmann::json aParameters = nlohmann::json::object() );
    void sendRpcEnvelope( nlohmann::json aEnvelope );

    bool startInteractiveLogin( const REMOTE_PROVIDER_ENTRY& aProvider,
                                const REMOTE_PROVIDER_METADATA& aMetadata, wxString& aError );
    bool signOutProvider( const REMOTE_PROVIDER_ENTRY& aProvider, wxString& aError );
    std::optional<OAUTH_TOKEN_SET> loadTokens( const REMOTE_PROVIDER_ENTRY& aProvider ) const;
    wxString loadAccessToken( const REMOTE_PROVIDER_ENTRY& aProvider );
    wxFileName cookieFilePath( const wxString& aProviderId ) const;
    void clearCookies( bool aDeleteSavedCookieFile = true );

    bool receiveComponent( const nlohmann::json& aParams, const std::vector<uint8_t>& aPayload,
                           bool aPlaceSymbol, wxString& aError );
    bool receiveFootprint( const nlohmann::json& aParams, const std::vector<uint8_t>& aPayload,
                           wxString& aError, LIB_ID* aOutLibId = nullptr );
    bool receiveSymbol( const nlohmann::json& aParams, const std::vector<uint8_t>& aPayload,
                        wxString& aError,
                        const std::vector<LIB_ID>& aFootprintLinks = {} );
    bool receive3DModel( const nlohmann::json& aParams, const std::vector<uint8_t>& aPayload,
                         wxString& aError );
    bool receiveSPICEModel( const nlohmann::json& aParams, const std::vector<uint8_t>& aPayload,
                            wxString& aError );
    bool receiveComponentManifest( const nlohmann::json& aParams, bool aPlaceSymbol, wxString& aError );

    wxString sanitizeForScript( const std::string& aJson ) const;
    bool decodeBase64Payload( const std::string& aMessage, std::vector<uint8_t>& aOutPayload,
                              wxString& aError ) const;
    bool decompressIfNeeded( const std::string& aCompression, const std::vector<uint8_t>& aInput,
                             std::vector<uint8_t>& aOutput, wxString& aError ) const;
private:
    SCH_EDIT_FRAME*                          m_frame;
    wxChoice*                                m_dataSourceChoice;
    BITMAP_BUTTON*                           m_configButton;
    BITMAP_BUTTON*                           m_refreshButton;
    WEBVIEW_PANEL*                           m_webView;
    std::vector<REMOTE_PROVIDER_ENTRY>       m_providerEntries;
    int                                      m_selectedProviderIndex;
    REMOTE_PROVIDER_METADATA                 m_selectedProviderMetadata;
    bool                                     m_hasSelectedProviderMetadata;
    KIID                                     m_sessionId;
    int                                      m_messageIdCounter;
    bool                                     m_pendingHandshake;
    REMOTE_PROVIDER_CLIENT                   m_providerClient;
    SECURE_TOKEN_STORE                       m_tokenStore;
    std::unique_ptr<OAUTH_LOOPBACK_SERVER>   m_oauthLoopbackServer;
    REMOTE_PROVIDER_OAUTH_SERVER_METADATA    m_pendingOAuthMetadata;
    OAUTH_SESSION                            m_pendingOAuthSession;
    wxString                                 m_pendingProviderId;
};

#endif // PANEL_REMOTE_SYMBOL_H
