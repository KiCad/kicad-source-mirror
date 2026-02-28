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

#include "panel_remote_symbol.h"

#include <bitmaps.h>
#include <build_version.h>
#include <common.h>
#include <ki_exception.h>
#include <pgm_base.h>
#include <project.h>
#include <project_sch.h>
#include <sch_edit_frame.h>
#include <sch_symbol.h>
#include <dialogs/dialog_remote_symbol_config.h>
#include <eeschema_settings.h>
#include <settings/kicad_settings.h>
#include <settings/settings_manager.h>
#include <libraries/symbol_library_adapter.h>
#include <libraries/library_manager.h>
#include <lib_symbol.h>
#include <sch_io/sch_io_mgr.h>
#include <tool/tool_manager.h>
#include <tools/sch_actions.h>
#include <widgets/bitmap_button.h>
#include <widgets/webview_panel.h>
#include <nlohmann/json.hpp>
#include <kiplatform/ui.h>

#ifndef wxUSE_BASE64
#define wxUSE_BASE64 1
#endif
#include <wx/base64.h>

#include <wx/datetime.h>
#include <wx/choice.h>
#include <wx/dir.h>
#include <wx/ffile.h>
#include <wx/filefn.h>
#include <wx/intl.h>
#include <wx/log.h>
#include <wx/mstream.h>
#include <wx/settings.h>
#include <wx/sizer.h>
#include <wx/strconv.h>
#include <wx/webview.h>
#include <zstd.h>

wxString PANEL_REMOTE_SYMBOL::jsonString( const nlohmann::json& aObject, const char* aKey ) const
{
    auto it = aObject.find( aKey );

    if( it != aObject.end() && it->is_string() )
        return wxString::FromUTF8( it->get<std::string>() );

    return wxString();
}

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

    unsigned long long expectedSize =
            ZSTD_getFrameContentSize( aInput.data(), aInput.size() );

    if( expectedSize == ZSTD_CONTENTSIZE_ERROR || expectedSize == ZSTD_CONTENTSIZE_UNKNOWN )
        expectedSize = static_cast<unsigned long long>( aInput.size() ) * 4;

    aOutput.resize( expectedSize );

    size_t decompressed = ZSTD_decompress( aOutput.data(), expectedSize,
                                           aInput.data(), aInput.size() );

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

wxString PANEL_REMOTE_SYMBOL::sanitizeFileComponent( const wxString& aValue,
                                                    const wxString& aDefault ) const
{
    wxString result = aValue;
    result.Trim( true ).Trim( false );

    if( result.IsEmpty() )
        result = aDefault;

    for( size_t i = 0; i < result.length(); ++i )
    {
        wxUniChar ch = result[i];

        if( ch == '/' || ch == '\\' || ch == ':' )
            result[i] = '_';
    }

    return result;
}

bool PANEL_REMOTE_SYMBOL::writeBinaryFile( const wxFileName& aOutput,
                                           const std::vector<uint8_t>& aPayload,
                                           wxString& aError ) const
{
    if( aPayload.empty() )
    {
        aError = _( "Payload was empty." );
        return false;
    }

    wxFileName targetDir = aOutput;
    targetDir.SetFullName( wxEmptyString );

    if( !targetDir.DirExists() )
    {
        if( !targetDir.Mkdir( wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL ) )
        {
            aError = wxString::Format( _( "Unable to create '%s'." ), targetDir.GetFullPath() );
            return false;
        }
    }

    wxFFile file( aOutput.GetFullPath(), wxS( "wb" ) );

    if( !file.IsOpened() )
    {
        aError = wxString::Format( _( "Unable to open '%s' for writing." ), aOutput.GetFullPath() );
        return false;
    }

    if( file.Write( aPayload.data(), aPayload.size() ) != aPayload.size() )
    {
        aError = wxString::Format( _( "Failed to write '%s'." ), aOutput.GetFullPath() );
        return false;
    }

    file.Close();
    return true;
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

    wxFFile file( tempFile.GetFullPath(), wxS( "wb" ) );

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
        {
            symbol.reset( loaded );
            wxLogTrace( wxS( "KI_TRACE_REMOTE_SYMBOL" ),
                        "loadSymbolFromPayload: loaded symbol %s from temporary file %s",
                        aLibItemName.ToUTF8().data(), tempFile.GetFullPath().ToUTF8().data() );
        }
        else
        {
            aError = _( "Symbol payload did not include the expected symbol." );
        }
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
        m_webView( nullptr ),
        m_pcm( std::make_shared<PLUGIN_CONTENT_MANAGER>( []( int ) {} ) ),
    m_sessionId( 0 ),
    m_messageIdCounter( 0 ),
    m_pendingHandshake( false )
{
    wxBoxSizer* topSizer = new wxBoxSizer( wxVERTICAL );

    wxBoxSizer* controlsSizer = new wxBoxSizer( wxHORIZONTAL );
    m_dataSourceChoice = new wxChoice( this, wxID_ANY );
    m_dataSourceChoice->SetMinSize( FromDIP( wxSize( 160, -1 ) ) );
    m_dataSourceChoice->SetToolTip( _( "Select which remote data source to query." ) );
    controlsSizer->Add( m_dataSourceChoice, 1, wxEXPAND | wxRIGHT, FromDIP( 2 ) );

    m_configButton = new BITMAP_BUTTON( this, wxID_ANY );
    m_configButton->SetBitmap( KiBitmapBundle( BITMAPS::config ) );
    m_configButton->SetPadding( FromDIP( 2 ) );
    m_configButton->SetToolTip( _( "Configure remote data sources." ) );
    controlsSizer->Add( m_configButton, 0, wxALIGN_CENTER_VERTICAL );

    topSizer->Add( controlsSizer, 0, wxEXPAND | wxALL, FromDIP( 4 ) );

    m_webView = new WEBVIEW_PANEL( this );
    m_webView->AddMessageHandler( wxS( "kicad" ),
            [this]( const wxString& payload )
            {
                onKicadMessage( payload );
            } );
    m_webView->SetHandleExternalLinks( true );
    m_webView->BindLoadedEvent();

    if( wxWebView* browser = m_webView->GetWebView() )
    {
        browser->Bind( wxEVT_WEBVIEW_LOADED, &PANEL_REMOTE_SYMBOL::onWebViewLoaded, this );
    }

    topSizer->Add( m_webView, 1, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, FromDIP( 2 ) );

    SetSizer( topSizer );

    m_dataSourceChoice->Bind( wxEVT_CHOICE, &PANEL_REMOTE_SYMBOL::onDataSourceChanged, this );
    m_configButton->Bind( wxEVT_BUTTON, &PANEL_REMOTE_SYMBOL::onConfigure, this );

    RefreshDataSources();

    Bind( wxEVT_SYS_COLOUR_CHANGED, wxSysColourChangedEventHandler( PANEL_REMOTE_SYMBOL::onDarkModeToggle ), this );

    wxLogTrace( wxS( "KI_TRACE_REMOTE_SYMBOL" ), "PANEL_REMOTE_SYMBOL constructed (frame=%p)", (void*)aParent );
}


PANEL_REMOTE_SYMBOL::~PANEL_REMOTE_SYMBOL()
{
    Unbind( wxEVT_SYS_COLOUR_CHANGED, wxSysColourChangedEventHandler( PANEL_REMOTE_SYMBOL::onDarkModeToggle ), this );
}


void PANEL_REMOTE_SYMBOL::BindWebViewLoaded()
{
    if( wxWebView* browser = m_webView->GetWebView() )
    {
        browser->Bind( wxEVT_WEBVIEW_LOADED, &PANEL_REMOTE_SYMBOL::onWebViewLoaded, this );
    }
}


void PANEL_REMOTE_SYMBOL::RefreshDataSources()
{
    if( KICAD_SETTINGS* cfg = GetAppSettings<KICAD_SETTINGS>( "kicad" ) )
        m_pcm->SetRepositoryList( cfg->m_PcmRepositories );

    m_dataSources.clear();
    m_dataSourceChoice->Clear();

    const std::vector<PCM_INSTALLATION_ENTRY> installed = m_pcm->GetInstalledPackages();

    for( const PCM_INSTALLATION_ENTRY& entry : installed )
    {
        if( entry.package.type != PT_DATASOURCE )
            continue;

        wxString label = entry.package.name;

        if( !entry.current_version.IsEmpty() )
            label << wxS( " (" ) << entry.current_version << wxS( ")" );

        m_dataSources.push_back( entry );
        m_dataSourceChoice->Append( label );
    }

    if( m_dataSources.empty() )
    {
        m_dataSourceChoice->Enable( false );
        showMessage( _( "No remote data sources are currently installed." ) );
        return;
    }

    m_dataSourceChoice->Enable( true );
    m_dataSourceChoice->SetSelection( 0 );
    loadDataSource( 0 );
}


void PANEL_REMOTE_SYMBOL::onDataSourceChanged( wxCommandEvent& aEvent )
{
    const int selection = aEvent.GetSelection();

    if( selection == wxNOT_FOUND )
        return;

    loadDataSource( static_cast<size_t>( selection ) );
}


void PANEL_REMOTE_SYMBOL::onConfigure( wxCommandEvent& aEvent )
{
    DIALOG_REMOTE_SYMBOL_CONFIG dlg( this );

    dlg.ShowModal();

    RefreshDataSources();
}


void PANEL_REMOTE_SYMBOL::onDarkModeToggle( wxSysColourChangedEvent& aEvent )
{
    RefreshDataSources();
}


bool PANEL_REMOTE_SYMBOL::loadDataSource( size_t aIndex )
{
    if( aIndex >= m_dataSources.size() )
        return false;

    return loadDataSource( m_dataSources[aIndex] );
}


bool PANEL_REMOTE_SYMBOL::loadDataSource( const PCM_INSTALLATION_ENTRY& aEntry )
{
    std::optional<wxFileName> jsonPath = findDataSourceJson( aEntry );

    if( !jsonPath )
    {
        wxLogWarning( "No JSON configuration found for data source %s", aEntry.package.identifier );
        showMessage( wxString::Format( _( "No configuration JSON found for '%s'." ),
                                       aEntry.package.name ) );
        wxLogTrace( wxS( "KI_TRACE_REMOTE_SYMBOL" ), "loadDataSource: no json for %s", aEntry.package.identifier );
        return false;
    }

    wxFFile file( jsonPath->GetFullPath(), "rb" );

    if( !file.IsOpened() )
    {
        wxLogWarning( "Unable to open remote data source JSON: %s", jsonPath->GetFullPath() );
        showMessage( wxString::Format( _( "Unable to open '%s'." ), jsonPath->GetFullPath() ) );
        wxLogTrace( wxS( "KI_TRACE_REMOTE_SYMBOL" ), "loadDataSource: cannot open %s", jsonPath->GetFullPath() );
        return false;
    }

    wxString jsonContent;

    if( !file.ReadAll( &jsonContent, wxConvUTF8 ) )
    {
        wxLogWarning( "Failed to read remote data source JSON: %s", jsonPath->GetFullPath() );
        showMessage( wxString::Format( _( "Unable to read '%s'." ), jsonPath->GetFullPath() ) );
        return false;
    }

    if( std::optional<wxString> url = extractUrlFromJson( jsonContent ) )
    {
        m_pendingHandshake = true;
        m_webView->LoadURL( *url );
        wxLogTrace( wxS( "KI_TRACE_REMOTE_SYMBOL" ), "loadDataSource: loading URL %s", (*url).ToUTF8().data() );
        return true;
    }

    wxLogWarning( "Remote data source JSON did not produce a valid URL for %s",
                  aEntry.package.identifier );
    showMessage( wxString::Format( _( "Unable to load remote data for '%s'." ),
                                   aEntry.package.name ) );
    wxLogTrace( wxS( "KI_TRACE_REMOTE_SYMBOL" ), "loadDataSource: failed to find URL in %s", jsonPath->GetFullPath() );
    return false;
}


std::optional<wxFileName>
PANEL_REMOTE_SYMBOL::findDataSourceJson( const PCM_INSTALLATION_ENTRY& aEntry ) const
{
    wxString cleanId = aEntry.package.identifier;
    cleanId.Replace( '.', '_' );

    wxFileName baseDir = wxFileName::DirName( m_pcm->Get3rdPartyPath() );
    baseDir.AppendDir( wxS( "resources" ) );
    baseDir.AppendDir( cleanId );

    const wxString resourcesPath = baseDir.GetFullPath();

    if( !wxDirExists( resourcesPath ) )
        return std::nullopt;

    const std::vector<wxString> preferredNames = {
        wxS( "remote_symbol.json" ),
        wxS( "datasource.json" )
    };

    for( const wxString& candidate : preferredNames )
    {
        wxFileName file( baseDir );
        file.SetFullName( candidate );

        if( file.FileExists() )
            return file;
    }

    wxDir dir( resourcesPath );

    if( !dir.IsOpened() )
        return std::nullopt;

    wxString jsonFile;

    if( dir.GetFirst( &jsonFile, wxS( "*.json" ), wxDIR_FILES ) )
    {
        wxFileName fallback( baseDir );
        fallback.SetFullName( jsonFile );
        return fallback;
    }

    return std::nullopt;
}


void PANEL_REMOTE_SYMBOL::showMessage( const wxString& aMessage )
{
    if( !m_webView )
        return;

    wxString escaped = aMessage;
    escaped.Replace( "&", "&amp;" );
    escaped.Replace( "<", "&lt;" );
    escaped.Replace( ">", "&gt;" );

    wxColour bgColour = wxSystemSettings::GetColour( wxSYS_COLOUR_WINDOW );
    wxColour fgColour = wxSystemSettings::GetColour( wxSYS_COLOUR_WINDOWTEXT );

    wxString html;
    html << wxS( "<html><head><style>" )
         << wxString::Format( wxS( "body { background-color: #%02x%02x%02x; color: #%02x%02x%02x; "
                                   "font-family: system-ui, sans-serif; padding: 10px; }" ),
                              bgColour.Red(), bgColour.Green(), bgColour.Blue(),
                              fgColour.Red(), fgColour.Green(), fgColour.Blue() )
         << wxS( "</style></head><body><p>" ) << escaped << wxS( "</p></body></html>" );

    m_webView->SetPage( html );
}


std::optional<wxString> PANEL_REMOTE_SYMBOL::extractUrlFromJson( const wxString& aJsonContent ) const
{
    if( aJsonContent.IsEmpty() )
        return std::nullopt;

    wxScopedCharBuffer utf8 = aJsonContent.ToUTF8();

    if( !utf8 || utf8.length() == 0 )
        return std::nullopt;

    try
    {
        nlohmann::json parsed = nlohmann::json::parse( utf8.data() );
        std::string    url;

        if( parsed.is_string() )
        {
            url = parsed.get<std::string>();
        }
        else if( parsed.is_object() )
        {
            auto extractString = [&]( const char* key ) -> std::string
            {
                auto it = parsed.find( key );

                if( it != parsed.end() && it->is_string() )
                    return it->get<std::string>();

                return {};
            };

            auto extractInt = [&]( const char* key ) -> std::optional<int>
            {
                auto it = parsed.find( key );

                if( it == parsed.end() )
                    return std::nullopt;

                if( it->is_number_integer() )
                    return it->get<int>();

                if( it->is_string() )
                {
                    try
                    {
                        return std::stoi( it->get<std::string>() );
                    }
                    catch( ... )
                    {
                    }
                }

                return std::nullopt;
            };

            std::string     host = extractString( "host" );
            std::optional<int> port = extractInt( "port" );
            std::string     path = extractString( "path" );

            if( !host.empty() )
            {
                if( path.empty() )
                    path = "/";

                if( port && *port > 0 )
                    url = wxString::Format( "%s:%d%s", host, *port, path ).ToStdString();
                else
                    url = host + path;
            }

            if( url.empty() )
                url = extractString( "url" );

            if( url.empty() )
            {
                for( const char* key : { "website", "endpoint" } )
                {
                    url = extractString( key );

                    if( !url.empty() )
                        break;
                }
            }

            if( url.empty() )
            {
                for( const auto& [name, value] : parsed.items() )
                {
                    if( value.is_string() )
                    {
                        const std::string candidate = value.get<std::string>();

                        if( candidate.rfind( "http", 0 ) == 0 || candidate.rfind( "file", 0 ) == 0 )
                        {
                            url = candidate;
                            break;
                        }
                    }
                }
            }
        }

        if( url.empty() )
            return std::nullopt;

        return wxString::FromUTF8( url.c_str() );
    }
    catch( const std::exception& e )
    {
        wxLogWarning( "Failed to parse remote symbol JSON: %s", e.what() );
        return std::nullopt;
    }
}


void PANEL_REMOTE_SYMBOL::onKicadMessage( const wxString& aMessage )
{
    wxScopedCharBuffer utf8 = aMessage.ToUTF8();

    if( !utf8 || utf8.length() == 0 )
    {
        wxLogWarning( "Remote symbol RPC: empty payload." );
        wxLogTrace( wxS( "KI_TRACE_REMOTE_SYMBOL" ), "onKicadMessage: empty payload" );
        return;
    }

    try
    {
        wxLogTrace( wxS( "KI_TRACE_REMOTE_SYMBOL" ), "onKicadMessage: received payload size=%d", (int)utf8.length() );
        handleRpcMessage( nlohmann::json::parse( utf8.data() ) );
    }
    catch( const std::exception& e )
    {
        wxLogWarning( "Remote symbol RPC parse error: %s", e.what() );
        wxLogTrace( wxS( "KI_TRACE_REMOTE_SYMBOL" ), "onKicadMessage: parse error %s", e.what() );
    }
}


void PANEL_REMOTE_SYMBOL::onWebViewLoaded( wxWebViewEvent& aEvent )
{
    wxUnusedVar( aEvent );

    if( m_pendingHandshake )
    {
        CallAfter( [this]()
        {
            if( m_pendingHandshake )
            {
                m_pendingHandshake = false;
                beginSessionHandshake();
            }
        } );
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

    sendRpcMessage( wxS( "NEW_SESSION" ), std::move( params ) );
    wxLogTrace( wxS( "KI_TRACE_REMOTE_SYMBOL" ), "beginSessionHandshake: NEW_SESSION sent, session=%s", m_sessionId.AsString() );
}


void PANEL_REMOTE_SYMBOL::sendRpcMessage( const wxString& aCommand,
                                          nlohmann::json aParameters,
                                          std::optional<int> aResponseTo,
                                          const wxString& aStatus,
                                          const std::string& aData,
                                          const wxString& aErrorCode,
                                          const wxString& aErrorMessage )
{
    if( !m_webView )
        return;

    nlohmann::json payload = nlohmann::json::object();
    payload["version"] = REMOTE_SYMBOL_SESSION_VERSION;
    payload["session_id"] = m_sessionId.AsStdString();
    payload["message_id"] = ++m_messageIdCounter;
    payload["command"] = aCommand.ToStdString();

    if( aResponseTo )
        payload["response_to"] = *aResponseTo;

    if( !aStatus.IsEmpty() )
        payload["status"] = aStatus.ToStdString();

    if( !aParameters.is_null() && !aParameters.empty() )
        payload["parameters"] = std::move( aParameters );

    if( !aData.empty() )
        payload["data"] = aData;

    if( !aErrorCode.IsEmpty() )
        payload["error_code"] = aErrorCode.ToStdString();

    if( !aErrorMessage.IsEmpty() )
        payload["error_message"] = aErrorMessage.ToStdString();

    wxString script = wxString::Format( wxS( "window.kiclient.postMessage('%s');" ),
                                        sanitizeForScript( payload.dump() ) );
    m_webView->RunScriptAsync( script );
    wxLogTrace( wxS( "KI_TRACE_REMOTE_SYMBOL" ), "sendRpcMessage: command=%s message_id=%d status=%s",
                aCommand.ToUTF8().data(), payload["message_id"].get<int>(), aStatus.ToUTF8().data() );
}


void PANEL_REMOTE_SYMBOL::respondWithError( const wxString& aCommand, int aResponseTo,
                                            const wxString& aErrorCode,
                                            const wxString& aErrorMessage )
{
    sendRpcMessage( aCommand, nlohmann::json::object(), aResponseTo, wxS( "ERROR" ),
                    std::string(), aErrorCode, aErrorMessage );
}


void PANEL_REMOTE_SYMBOL::handleRpcMessage( const nlohmann::json& aMessage )
{
    if( !aMessage.is_object() )
        return;

    const wxString command = jsonString( aMessage, "command" );

    if( command.IsEmpty() )
        return;

    auto messageIdIt = aMessage.find( "message_id" );

    if( messageIdIt == aMessage.end() || !messageIdIt->is_number_integer() )
        return;

    const int messageId = messageIdIt->get<int>();

    const int version = aMessage.value( "version", 0 );

    if( version != REMOTE_SYMBOL_SESSION_VERSION )
    {
        respondWithError( command, messageId, wxS( "UNSUPPORTED_VERSION" ),
                wxString::Format( _( "Unsupported RPC version %d." ), version ) );
        return;
    }

    const wxString sessionId = jsonString( aMessage, "session_id" );

    if( sessionId.IsEmpty() )
    {
        respondWithError( command, messageId, wxS( "INVALID_PARAMETERS" ),
                          _( "Missing session identifier." ) );
        return;
    }

    if( !sessionId.IsSameAs( m_sessionId.AsString() ) )
        wxLogWarning( "Remote symbol RPC session mismatch (expected %s, got %s).",
                      m_sessionId.AsString(), sessionId );

    const wxString status = jsonString( aMessage, "status" );

    if( status.IsSameAs( wxS( "ERROR" ), false ) )
    {
        wxLogWarning( "Remote symbol RPC error (%s): %s", jsonString( aMessage, "error_code" ),
                      jsonString( aMessage, "error_message" ) );
        return;
    }

    wxLogTrace( wxS( "KI_TRACE_REMOTE_SYMBOL" ), "handleRpcMessage: command=%s message_id=%d status=%s session=%s",
                command.ToUTF8().data(), messageId, status.ToUTF8().data(), sessionId.ToUTF8().data() );

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
        sendRpcMessage( command, std::move( reply ), messageId );
        return;
    }
    else if( command == wxS( "GET_KICAD_VERSION" ) )
    {
        nlohmann::json reply = nlohmann::json::object();
        reply["kicad_version"] = GetSemanticVersion().ToStdString();
        sendRpcMessage( command, std::move( reply ), messageId );
        return;
    }
    else if( command == wxS( "LIST_SUPPORTED_VERSIONS" ) )
    {
        nlohmann::json reply = nlohmann::json::object();
        reply["supported_versions"] = { REMOTE_SYMBOL_SESSION_VERSION };
        sendRpcMessage( command, std::move( reply ), messageId );
        return;
    }
    else if( command == wxS( "CAPABILITIES" ) )
    {
        nlohmann::json reply = nlohmann::json::object();
        reply["commands"] = { "NEW_SESSION", "GET_KICAD_VERSION", "LIST_SUPPORTED_VERSIONS",
                               "CAPABILITIES", "PING", "PONG", "DL_SYMBOL", "DL_FOOTPRINT",
                               "DL_SPICE", "DL_3DMODEL" };
        reply["compression"] = { "NONE", "ZSTD" };
        reply["max_message_size"] = 0;
        sendRpcMessage( command, std::move( reply ), messageId );
        return;
    }
    else if( command == wxS( "PING" ) )
    {
        nlohmann::json reply = nlohmann::json::object();

        if( params.contains( "nonce" ) )
            reply["nonce"] = params["nonce"];

        sendRpcMessage( wxS( "PONG" ), std::move( reply ), messageId );
        return;
    }
    else if( command == wxS( "PONG" ) )
    {
        return;
    }

    const wxString compression = jsonString( params, "compression" );

    if( command.StartsWith( wxS( "DL_" ) ) )
    {
        if( compression.IsEmpty() )
        {
            respondWithError( command, messageId, wxS( "INVALID_PARAMETERS" ),
                              _( "Missing compression metadata." ) );
            return;
        }

        std::vector<uint8_t> decoded;
        wxString error;

        if( !decodeBase64Payload( data, decoded, error ) )
        {
            respondWithError( command, messageId, wxS( "INVALID_PAYLOAD" ), error );
            return;
        }

        std::vector<uint8_t> payload;
    wxScopedCharBuffer compUtf8 = compression.ToUTF8();
    std::string        compressionStr = compUtf8 ? std::string( compUtf8.data() ) : std::string();

    if( !decompressIfNeeded( compressionStr, decoded, payload, error ) )
        {
            respondWithError( command, messageId, wxS( "INVALID_PAYLOAD" ), error );
            return;
        }

        wxLogTrace( wxS( "KI_TRACE_REMOTE_SYMBOL" ), "handleRpcMessage: decoded size=%zu decompressed size=%zu command=%s",
                    decoded.size(), payload.size(), command.ToUTF8().data() );

        bool ok = false;

        if( command == wxS( "DL_SYMBOL" ) )
            ok = receiveSymbol( params, payload, error );
        else if( command == wxS( "DL_FOOTPRINT" ) )
            ok = receiveFootprint( params, payload, error );
        else if( command == wxS( "DL_3DMODEL" ) )
            ok = receive3DModel( params, payload, error );
        else if( command == wxS( "DL_SPICE" ) )
            ok = receiveSPICEModel( params, payload, error );
        else
        {
            respondWithError( command, messageId, wxS( "UNKNOWN_COMMAND" ),
                              wxString::Format( _( "Command '%s' is not supported." ), command ) );
            return;
        }

        if( ok )
            sendRpcMessage( command, nlohmann::json::object(), messageId );
        else
            respondWithError( command, messageId, wxS( "INTERNAL_ERROR" ),
                              error.IsEmpty() ? _( "Unable to store payload." ) : error );

        return;
    }

    respondWithError( command, messageId, wxS( "UNKNOWN_COMMAND" ),
                      wxString::Format( _( "Command '%s' is not supported." ), command ) );
}


bool PANEL_REMOTE_SYMBOL::ensureDestinationRoot( wxFileName& aOutDir, wxString& aError ) const
{
    EESCHEMA_SETTINGS* settings = GetAppSettings<EESCHEMA_SETTINGS>( "eeschema" );

    if( !settings )
    {
        aError = _( "Unable to load schematic settings." );
        return false;
    }

    wxString destination = settings->m_RemoteSymbol.destination_dir;

    if( destination.IsEmpty() )
        destination = EESCHEMA_SETTINGS::REMOTE_SYMBOL_CONFIG::DefaultDestinationDir();

    destination = ExpandEnvVarSubstitutions( destination,
        &Pgm().GetSettingsManager().Prj() );
    destination.Trim( true ).Trim( false );

    if( destination.IsEmpty() )
    {
        aError = _( "Destination directory is not configured." );
        return false;
    }

    wxFileName dir = wxFileName::DirName( destination );
    dir.Normalize( FN_NORMALIZE_FLAGS );

    if( !dir.DirExists() )
    {
        if( !dir.Mkdir( wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL ) )
        {
            aError = wxString::Format( _( "Unable to create directory '%s'." ), dir.GetFullPath() );
            return false;
        }
    }

    aOutDir = dir;
    return true;
}


bool PANEL_REMOTE_SYMBOL::ensureSymbolLibraryEntry( const wxFileName& aLibraryFile,
                                                    const wxString& aNickname, bool aGlobalTable,
                                                    wxString& aError ) const
{
    LIBRARY_MANAGER& manager = Pgm().GetLibraryManager();
    std::optional<LIBRARY_TABLE*> tableOpt = manager.Table( LIBRARY_TABLE_TYPE::SYMBOL,
            aGlobalTable ? LIBRARY_TABLE_SCOPE::GLOBAL : LIBRARY_TABLE_SCOPE::PROJECT );

    if( !tableOpt )
    {
        aError = _( "Unable to access the symbol library table." );
        return false;
    }

    LIBRARY_TABLE* table = *tableOpt;
    const wxString fullPath = aLibraryFile.GetFullPath();

    if( table->HasRow( aNickname ) )
    {
        if( std::optional<LIBRARY_TABLE_ROW*> rowOpt = table->Row( aNickname ); rowOpt )
        {
            LIBRARY_TABLE_ROW* row = *rowOpt;

            if( row->URI() != fullPath )
            {
                row->SetURI( fullPath );

                if( !table->Save() )
                {
                    aError = _( "Failed to update the symbol library table." );
                    return false;
                }
            }
        }

        return true;
    }

    LIBRARY_TABLE_ROW& row = table->InsertRow();
    row.SetNickname( aNickname );
    row.SetURI( fullPath );
    row.SetType( wxS( "KiCad" ) );
    row.SetOptions( wxString() );
    row.SetDescription( _( "Remote download" ) );
    row.SetOk( true );

    if( !table->Save() )
    {
        aError = _( "Failed to save the symbol library table." );
        return false;
    }

    return true;
}


bool PANEL_REMOTE_SYMBOL::ensureFootprintLibraryEntry( const wxFileName& aLibraryDir,
                                                       const wxString& aNickname,
                                                       bool aGlobalTable, wxString& aError ) const
{
    LIBRARY_MANAGER& manager = Pgm().GetLibraryManager();
    std::optional<LIBRARY_TABLE*> tableOpt = manager.Table( LIBRARY_TABLE_TYPE::FOOTPRINT,
            aGlobalTable ? LIBRARY_TABLE_SCOPE::GLOBAL : LIBRARY_TABLE_SCOPE::PROJECT );

    if( !tableOpt )
    {
        aError = _( "Unable to access the footprint library table." );
        return false;
    }

    LIBRARY_TABLE* table = *tableOpt;
    const wxString fullPath = aLibraryDir.GetFullPath();

    if( table->HasRow( aNickname ) )
    {
        if( std::optional<LIBRARY_TABLE_ROW*> rowOpt = table->Row( aNickname ); rowOpt )
        {
            LIBRARY_TABLE_ROW* row = *rowOpt;

            if( row->URI() != fullPath )
            {
                row->SetURI( fullPath );

                if( !table->Save() )
                {
                    aError = _( "Failed to update the footprint library table." );
                    return false;
                }
            }
        }

        return true;
    }

    LIBRARY_TABLE_ROW& row = table->InsertRow();
    row.SetNickname( aNickname );
    row.SetURI( fullPath );
    row.SetType( wxS( "KiCad" ) );
    row.SetOptions( wxString() );
    row.SetDescription( _( "Remote download" ) );
    row.SetOk( true );

    if( !table->Save() )
    {
        aError = _( "Failed to save the footprint library table." );
        return false;
    }

    return true;
}


wxString PANEL_REMOTE_SYMBOL::sanitizedPrefix() const
{
    wxString prefix;

    if( EESCHEMA_SETTINGS* settings = GetAppSettings<EESCHEMA_SETTINGS>( "eeschema" ) )
        prefix = settings->m_RemoteSymbol.library_prefix;

    if( prefix.IsEmpty() )
        prefix = EESCHEMA_SETTINGS::REMOTE_SYMBOL_CONFIG::DefaultLibraryPrefix();

    prefix.Trim( true ).Trim( false );

    if( prefix.IsEmpty() )
        prefix = wxS( "remote" );

    for( size_t i = 0; i < prefix.length(); ++i )
    {
        wxUniChar ch = prefix[i];

        if( !( wxIsalnum( ch ) || ch == '_' || ch == '-' ) )
            prefix[i] = '_';
    }

    return prefix;
}


bool PANEL_REMOTE_SYMBOL::placeDownloadedSymbol( const wxString& aNickname,
                                                 const wxString& aLibItemName,
                                                 wxString& aError )
{
    if( !m_frame )
    {
        aError = _( "No schematic editor is available for placement." );
        wxLogTrace( wxS( "KI_TRACE_REMOTE_SYMBOL" ), "placeDownloadedSymbol: no frame available" );
        return false;
    }

    if( aNickname.IsEmpty() || aLibItemName.IsEmpty() )
    {
        aError = _( "Downloaded symbol metadata is incomplete." );
        return false;
    }

    LIB_ID libId;
    libId.SetLibNickname( aNickname );
    libId.SetLibItemName( aLibItemName );

    LIB_SYMBOL* libSymbol = m_frame->GetLibSymbol( libId );

    if( !libSymbol )
    {
        aError = _( "Unable to load the downloaded symbol for placement." );
        wxLogTrace( wxS( "KI_TRACE_REMOTE_SYMBOL" ), "placeDownloadedSymbol: failed to load libSymbol %s:%s",
                    aNickname.ToUTF8().data(), aLibItemName.ToUTF8().data() );
        return false;
    }

    wxLogTrace( wxS( "KI_TRACE_REMOTE_SYMBOL" ), "placeDownloadedSymbol: loaded libSymbol %s:%s",
                aNickname.ToUTF8().data(), aLibItemName.ToUTF8().data() );

    SCH_SYMBOL* symbol = new SCH_SYMBOL( *libSymbol, libId, &m_frame->GetCurrentSheet(), 1 );
    symbol->SetParent( m_frame->GetScreen() );

    if( EESCHEMA_SETTINGS* cfg = m_frame->eeconfig(); cfg && cfg->m_AutoplaceFields.enable )
    {
        symbol->AutoplaceFields( nullptr, AUTOPLACE_AUTO );
    }

    TOOL_MANAGER* toolMgr = m_frame->GetToolManager();

    if( !toolMgr )
    {
        delete symbol;
        aError = _( "Unable to access the schematic placement tools." );
        wxLogTrace( wxS( "KI_TRACE_REMOTE_SYMBOL" ), "placeDownloadedSymbol: no tool manager available" );
        return false;
    }

    m_frame->Raise();
    toolMgr->PostAction( SCH_ACTIONS::placeSymbol,
                         SCH_ACTIONS::PLACE_SYMBOL_PARAMS{ symbol, true } );
    wxLogTrace( wxS( "KI_TRACE_REMOTE_SYMBOL" ), "placeDownloadedSymbol: posted placeSymbol action for %s:%s",
                aNickname.ToUTF8().data(), aLibItemName.ToUTF8().data() );
    return true;
}


bool PANEL_REMOTE_SYMBOL::receiveSymbol( const nlohmann::json& aParams,
                                         const std::vector<uint8_t>& aPayload,
                                         wxString& aError )
{
    const wxString mode = jsonString( aParams, "mode" );
    const bool     placeAfterDownload = mode.IsSameAs( wxS( "PLACE" ), false );

    if( !mode.IsEmpty() && !mode.IsSameAs( wxS( "SAVE" ), false )
        && !mode.IsSameAs( wxS( "PLACE" ), false ) )
    {
        aError = wxString::Format( _( "Unsupported transfer mode '%s'." ), mode );
        return false;
    }

    const wxString contentType = jsonString( aParams, "content_type" );

    if( !contentType.IsSameAs( wxS( "KICAD_SYMBOL_V1" ), false ) )
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

    const bool addToGlobal = settings->m_RemoteSymbol.add_to_global_table;

    wxFileName baseDir;

    if( !ensureDestinationRoot( baseDir, aError ) )
        return false;

    wxFileName symDir = baseDir;
    symDir.AppendDir( wxS( "symbols" ) );

    if( !symDir.DirExists() && !symDir.Mkdir( wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL ) )
    {
        aError = wxString::Format( _( "Unable to create '%s'." ), symDir.GetFullPath() );
        return false;
    }

    // Get symbol's internal name from "name" parameter
    wxString libItemName = jsonString( aParams, "name" );
    if( libItemName.IsEmpty() )
        libItemName = wxS( "symbol" );
    libItemName.Trim( true ).Trim( false );

    // Get library name from "library" parameter, fallback to "symbols"
    wxString libraryName = jsonString( aParams, "library" );
    if( libraryName.IsEmpty() )
        libraryName = wxS( "symbols" );
    libraryName.Trim( true ).Trim( false );

    wxString sanitizedLibName = sanitizeFileComponent( libraryName, wxS( "symbols" ) );
    const wxString nickname = sanitizedPrefix() + wxS( "_" ) + sanitizedLibName;

    wxFileName outFile( symDir );
    outFile.SetFullName( nickname + wxS( ".kicad_sym" ) );

    if( !ensureSymbolLibraryEntry( outFile, nickname, addToGlobal, aError ) )
        return false;

    SYMBOL_LIBRARY_ADAPTER* adapter = PROJECT_SCH::SymbolLibAdapter( &m_frame->Prj() );

    if( !adapter )
    {
        aError = _( "Unable to access the symbol library manager." );
        return false;
    }

    std::unique_ptr<LIB_SYMBOL> downloadedSymbol = loadSymbolFromPayload( aPayload, libItemName, aError );

    if( !downloadedSymbol )
    {
        if( aError.IsEmpty() )
            aError = _( "Unable to parse the downloaded symbol." );

        return false;
    }

    downloadedSymbol->SetName( libItemName );

    LIB_ID savedId;
    savedId.SetLibNickname( nickname );
    savedId.SetLibItemName( libItemName );
    downloadedSymbol->SetLibId( savedId );

    if( adapter->SaveSymbol( nickname, downloadedSymbol.get(), true )
            != SYMBOL_LIBRARY_ADAPTER::SAVE_OK )
    {
        aError = _( "Unable to save the downloaded symbol." );
        wxLogTrace( wxS( "KI_TRACE_REMOTE_SYMBOL" ),
                    "receiveSymbol: failed to save symbol %s to library %s",
                    libItemName.ToUTF8().data(), nickname.ToUTF8().data() );
        return false;
    }

    wxLogTrace( wxS( "KI_TRACE_REMOTE_SYMBOL" ),
                "receiveSymbol: saved symbol %s into library %s", libItemName.ToUTF8().data(),
                nickname.ToUTF8().data() );

    const LIBRARY_TABLE_SCOPE scope = addToGlobal ? LIBRARY_TABLE_SCOPE::GLOBAL
                                                  : LIBRARY_TABLE_SCOPE::PROJECT;
    Pgm().GetLibraryManager().ReloadLibraryEntry( LIBRARY_TABLE_TYPE::SYMBOL, nickname, scope );

    if( placeAfterDownload )
    {
        wxLogTrace( wxS( "KI_TRACE_REMOTE_SYMBOL" ), "receiveSymbol: placing symbol now (nickname=%s libItem=%s)",
                    nickname.ToUTF8().data(), libItemName.ToUTF8().data() );
        return placeDownloadedSymbol( nickname, libItemName, aError );
    }

    wxLogTrace( wxS( "KI_TRACE_REMOTE_SYMBOL" ), "receiveSymbol: saved symbol (nickname=%s libItem=%s)" ,
                nickname.ToUTF8().data(), libItemName.ToUTF8().data() );

    return true;
}


bool PANEL_REMOTE_SYMBOL::receiveFootprint( const nlohmann::json& aParams,
                                            const std::vector<uint8_t>& aPayload,
                                            wxString& aError )
{
    const wxString mode = jsonString( aParams, "mode" );

    wxLogTrace( wxS( "KI_TRACE_REMOTE_SYMBOL" ), "receiveFootprint: mode=%s", mode.ToUTF8().data() );

    if( !mode.IsEmpty() && !mode.IsSameAs( wxS( "SAVE" ), false )
        && !mode.IsSameAs( wxS( "PLACE" ), false ) )
    {
        aError = wxString::Format( _( "Unsupported transfer mode '%s'." ), mode );
        return false;
    }

    const wxString contentType = jsonString( aParams, "content_type" );

    if( !contentType.IsSameAs( wxS( "KICAD_FOOTPRINT_V1" ), false ) )
    {
        aError = _( "Unsupported footprint payload type." );
        return false;
    }

    wxFileName baseDir;

    if( !ensureDestinationRoot( baseDir, aError ) )
        return false;

    wxFileName fpRoot = baseDir;
    fpRoot.AppendDir( wxS( "footprints" ) );

    if( !fpRoot.DirExists() && !fpRoot.Mkdir( wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL ) )
    {
        aError = wxString::Format( _( "Unable to create '%s'." ), fpRoot.GetFullPath() );
        return false;
    }

    // Get footprint's internal name from "name" parameter
    wxString footprintName = jsonString( aParams, "name" );
    if( footprintName.IsEmpty() )
        footprintName = wxS( "footprint" );
    footprintName = sanitizeFileComponent( footprintName, wxS( "footprint" ) );

    // Get library name from "library" parameter, fallback to "footprints"
    wxString libraryName = jsonString( aParams, "library" );
    if( libraryName.IsEmpty() )
        libraryName = wxS( "footprints" );
    libraryName.Trim( true ).Trim( false );

    wxString sanitizedLibName = sanitizeFileComponent( libraryName, wxS( "footprints" ) );
    wxString libNickname = sanitizedPrefix() + wxS( "_" ) + sanitizedLibName;

    wxFileName libDir = fpRoot;
    libDir.AppendDir( libNickname + wxS( ".pretty" ) );

    if( !libDir.DirExists() && !libDir.Mkdir( wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL ) )
    {
        aError = wxString::Format( _( "Unable to create '%s'." ), libDir.GetFullPath() );
        return false;
    }

    wxString fileName = footprintName;

    if( !fileName.Lower().EndsWith( wxS( ".kicad_mod" ) ) )
        fileName += wxS( ".kicad_mod" );

    wxFileName outFile( libDir );
    outFile.SetFullName( fileName );

    if( !writeBinaryFile( outFile, aPayload, aError ) )
        return false;

    wxLogTrace( wxS( "KI_TRACE_REMOTE_SYMBOL" ), "receiveFootprint: wrote footprint %s in lib %s",
                outFile.GetFullPath(), libNickname.ToUTF8().data() );

    EESCHEMA_SETTINGS* settings = GetAppSettings<EESCHEMA_SETTINGS>( "eeschema" );

    if( !settings )
    {
        aError = _( "Unable to load schematic settings." );
        return false;
    }

    const bool addToGlobal = settings->m_RemoteSymbol.add_to_global_table;

    if( !ensureFootprintLibraryEntry( libDir, libNickname, addToGlobal, aError ) )
        return false;

    const LIBRARY_TABLE_SCOPE scope = addToGlobal ? LIBRARY_TABLE_SCOPE::GLOBAL
                                                  : LIBRARY_TABLE_SCOPE::PROJECT;
    Pgm().GetLibraryManager().ReloadLibraryEntry( LIBRARY_TABLE_TYPE::FOOTPRINT, libNickname, scope );

    return true;
}


bool PANEL_REMOTE_SYMBOL::receive3DModel( const nlohmann::json& aParams,
                                          const std::vector<uint8_t>& aPayload,
                                          wxString& aError )
{
    const wxString mode = jsonString( aParams, "mode" );

    wxLogTrace( wxS( "KI_TRACE_REMOTE_SYMBOL" ), "receive3DModel: mode=%s", mode.ToUTF8().data() );

    if( !mode.IsEmpty() && !mode.IsSameAs( wxS( "SAVE" ), false )
        && !mode.IsSameAs( wxS( "PLACE" ), false ) )
    {
        aError = wxString::Format( _( "Unsupported transfer mode '%s'." ), mode );
        return false;
    }

    const wxString contentType = jsonString( aParams, "content_type" );

    if( !contentType.IsSameAs( wxS( "KICAD_3D_MODEL_STEP" ), false ) &&
        !contentType.IsSameAs( wxS( "KICAD_3D_MODEL_WRL" ), false ) )
    {
        aError = _( "Unsupported 3D model payload type." );
        return false;
    }

    wxFileName baseDir;

    if( !ensureDestinationRoot( baseDir, aError ) )
        return false;

    wxFileName modelDir = baseDir;
    modelDir.AppendDir( sanitizedPrefix() + wxS( "_3d" ) );

    if( !modelDir.DirExists() && !modelDir.Mkdir( wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL ) )
    {
        aError = wxString::Format( _( "Unable to create '%s'." ), modelDir.GetFullPath() );
        return false;
    }

    wxString fileName = jsonString( aParams, "name" );

    if( fileName.IsEmpty() )
        fileName = sanitizedPrefix() + wxS( "_model" );

    fileName = sanitizeFileComponent( fileName, sanitizedPrefix() + wxS( "_model" ) );

    wxString extension = wxS( ".step" );

    if( contentType.IsSameAs( wxS( "KICAD_3D_MODEL_WRL" ), false ) )
        extension = wxS( ".wrl" );

    if( !fileName.Lower().EndsWith( extension ) )
        fileName += extension;

    wxFileName outFile( modelDir );
    outFile.SetFullName( fileName );

    bool ok = writeBinaryFile( outFile, aPayload, aError );

    if( ok )
        wxLogTrace( wxS( "KI_TRACE_REMOTE_SYMBOL" ), "receive3DModel: wrote model %s", outFile.GetFullPath() );

    return ok;
}


bool PANEL_REMOTE_SYMBOL::receiveSPICEModel( const nlohmann::json& aParams,
                                             const std::vector<uint8_t>& aPayload,
                                             wxString& aError )
{
    const wxString mode = jsonString( aParams, "mode" );

    wxLogTrace( wxS( "KI_TRACE_REMOTE_SYMBOL" ), "receiveSPICEModel: mode=%s", mode.ToUTF8().data() );

    if( !mode.IsEmpty() && !mode.IsSameAs( wxS( "SAVE" ), false )
        && !mode.IsSameAs( wxS( "PLACE" ), false ) )
    {
        aError = wxString::Format( _( "Unsupported transfer mode '%s'." ), mode );
        return false;
    }

    const wxString contentType = jsonString( aParams, "content_type" );

    if( !contentType.IsSameAs( wxS( "KICAD_SPICE_MODEL_V1" ), false ) )
    {
        aError = _( "Unsupported SPICE payload type." );
        return false;
    }

    wxFileName baseDir;

    if( !ensureDestinationRoot( baseDir, aError ) )
        return false;

    wxFileName spiceDir = baseDir;
    spiceDir.AppendDir( sanitizedPrefix() + wxS( "_spice" ) );

    if( !spiceDir.DirExists() && !spiceDir.Mkdir( wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL ) )
    {
        aError = wxString::Format( _( "Unable to create '%s'." ), spiceDir.GetFullPath() );
        return false;
    }

    wxString fileName = jsonString( aParams, "name" );

    if( fileName.IsEmpty() )
        fileName = sanitizedPrefix() + wxS( "_model.cir" );

    fileName = sanitizeFileComponent( fileName, sanitizedPrefix() + wxS( "_model.cir" ) );

    if( !fileName.Lower().EndsWith( wxS( ".cir" ) ) )
        fileName += wxS( ".cir" );

    wxFileName outFile( spiceDir );
    outFile.SetFullName( fileName );

    bool ok = writeBinaryFile( outFile, aPayload, aError );

    if( ok )
        wxLogTrace( wxS( "KI_TRACE_REMOTE_SYMBOL" ), "receiveSPICEModel: wrote spice model %s", outFile.GetFullPath() );

    return ok;
}