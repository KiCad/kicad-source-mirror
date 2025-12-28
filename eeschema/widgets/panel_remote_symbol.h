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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#ifndef PANEL_REMOTE_SYMBOL_H
#define PANEL_REMOTE_SYMBOL_H


#include <memory>
#include <optional>
#include <vector>
#include <map>

#include <pcm.h>
#include <nlohmann/json_fwd.hpp>
#include <wx/filename.h>
#include <wx/panel.h>
#include <kiid.h>

class BITMAP_BUTTON;
class LIB_SYMBOL;
class SCH_EDIT_FRAME;
class WEBVIEW_PANEL;
class wxChoice;
class wxCommandEvent;
class wxWebViewEvent;
class REMOTE_LOGIN_SERVER;

#define REMOTE_SYMBOL_SESSION_VERSION 1

class PANEL_REMOTE_SYMBOL : public wxPanel
{
public:
    explicit PANEL_REMOTE_SYMBOL( SCH_EDIT_FRAME* aParent );
    ~PANEL_REMOTE_SYMBOL();

    void RefreshDataSources();
    bool HasDataSources() const;
    void BindWebViewLoaded();

    /// Store WebView cookies to settings for persistence across sessions
    void SaveCookies();

    /// Restore WebView cookies from settings
    void LoadCookies();

private:
    void onDataSourceChanged( wxCommandEvent& aEvent );
    void onConfigure( wxCommandEvent& aEvent );
    void onRefresh( wxCommandEvent& aEvent );
    void onWebViewLoaded( wxWebViewEvent& aEvent );
    void onRemoteLoginResult( wxCommandEvent& aEvent );

    bool loadDataSource( size_t aIndex );
    bool loadDataSource( const PCM_INSTALLATION_ENTRY& aEntry );
    std::optional<wxFileName> findDataSourceJson( const PCM_INSTALLATION_ENTRY& aEntry ) const;
    void showMessage( const wxString& aMessage );
    std::optional<wxString> extractUrlFromJson( const wxString& aJsonContent ) const;
    void onKicadMessage( const wxString& aMessage );
    void handleRpcMessage( const nlohmann::json& aMessage );
    void beginSessionHandshake();
    void handleRemoteLogin( const nlohmann::json& aParams, int aMessageId );
    void stopLoginServer();
    void storeUserIdForActiveSource( const wxString& aUserId );
    void loadStoredUserIdForActiveSource();
    wxString currentDataSourceKey() const;

    void sendRpcMessage( const wxString& aCommand,
                         nlohmann::json aParameters = nlohmann::json::object(),
                         std::optional<int> aResponseTo = std::nullopt,
                         const wxString& aStatus = wxS( "OK" ),
                         const std::string& aData = std::string(),
                         const wxString& aErrorCode = wxEmptyString,
                         const wxString& aErrorMessage = wxEmptyString );

    void respondWithError( const wxString& aCommand, int aResponseTo,
                           const wxString& aErrorCode, const wxString& aErrorMessage );

    bool ensureDestinationRoot( wxFileName& aOutDir, wxString& aError ) const;
    bool ensureSymbolLibraryEntry( const wxFileName& aLibraryFile, const wxString& aNickname,
                                   bool aGlobalTable, wxString& aError ) const;
    bool ensureFootprintLibraryEntry( const wxFileName& aLibraryDir, const wxString& aNickname,
                                      bool aGlobalTable, wxString& aError ) const;
    wxString sanitizedPrefix() const;

    bool receiveFootprint( const nlohmann::json& aParams, const std::vector<uint8_t>& aPayload,
                           wxString& aError );
    bool receiveSymbol( const nlohmann::json& aParams, const std::vector<uint8_t>& aPayload,
                        wxString& aError );
    bool receive3DModel( const nlohmann::json& aParams, const std::vector<uint8_t>& aPayload,
                         wxString& aError );
    bool receiveSPICEModel( const nlohmann::json& aParams, const std::vector<uint8_t>& aPayload,
                            wxString& aError );
    bool receiveComponent( const nlohmann::json& aParams, const std::vector<uint8_t>& aPayload,
                           wxString& aError, nlohmann::json* aResponseParams = nullptr );
    bool placeDownloadedSymbol( const wxString& aNickname, const wxString& aLibItemName,
                                wxString& aError );

    wxString sanitizeFileComponent( const wxString& aComponent, const wxString& aDefault ) const;
    wxString sanitizeForScript( const std::string& aJson ) const;

    wxString jsonString( const nlohmann::json& aObject, const char* aKey ) const;
    wxString normalizeDataSourceUrl( const wxString& aUrl ) const;

    bool decodeBase64Payload( const std::string& aMessage,
                           std::vector<uint8_t>& aOutPayload,
                           wxString& aError ) const;

    bool decompressIfNeeded( const std::string& aCompression,
                             const std::vector<uint8_t>& aInput,
                             std::vector<uint8_t>& aOutput,
                             wxString& aError ) const;

    bool writeBinaryFile( const wxFileName& aFile,
                          const std::vector<uint8_t>& aData,
                          wxString& aError ) const;

    std::unique_ptr<LIB_SYMBOL> loadSymbolFromPayload( const std::vector<uint8_t>& aPayload,
                                                       const wxString& aLibItemName,
                                                       wxString& aError ) const;

private:
    SCH_EDIT_FRAME*                           m_frame;
    wxChoice*                                 m_dataSourceChoice;
    BITMAP_BUTTON*                            m_configButton;
    BITMAP_BUTTON*                            m_refreshButton;
    WEBVIEW_PANEL*                            m_webView;
    std::shared_ptr<PLUGIN_CONTENT_MANAGER>   m_pcm;
    std::vector<PCM_INSTALLATION_ENTRY>       m_dataSources;
    KIID                                      m_sessionId;
    int                                       m_messageIdCounter;
    bool                                      m_pendingHandshake;
    std::unique_ptr<REMOTE_LOGIN_SERVER>      m_loginServer;
    wxString                                  m_activeDataSourceUrl;
    wxString                                  m_activeUserId;
    bool                                      m_webViewLoadedBound;
};

#endif // PANEL_REMOTE_SYMBOL_H
