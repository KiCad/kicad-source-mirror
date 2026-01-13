/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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

#include <build_version.h>

#include <tool/tool_manager.h>
#include <tool/tool_base.h>

#include <widgets/webview_panel.h>
#include <wx/sizer.h>
#include <wx/webviewarchivehandler.h>
#include <wx/webviewfshandler.h>
#include <wx/utils.h>
#include <wx/log.h>

WEBVIEW_PANEL::WEBVIEW_PANEL( wxWindow* aParent, wxWindowID aId, const wxPoint& aPos, const wxSize& aSize,
                              const int aStyle, TOOL_MANAGER* aToolManager, TOOL_BASE* aTool ) :
        wxPanel( aParent, aId, aPos, aSize, aStyle ),
        m_initialized( false ),
        m_handleExternalLinks( false ),
        m_loadError( false ),
        m_loadedEventBound( false ),
        m_browser( wxWebView::New() ),
        m_toolManager( aToolManager ),
        m_tool( aTool )
{
    wxBoxSizer* sizer = new wxBoxSizer( wxVERTICAL );

    if( !wxGetEnv( wxT( "WEBKIT_DISABLE_COMPOSITING_MODE" ), nullptr ) )
    {
        wxSetEnv( wxT( "WEBKIT_DISABLE_COMPOSITING_MODE" ), wxT( "1" ) );
    }

#ifdef __WXMAC__
    m_browser->RegisterHandler( wxSharedPtr<wxWebViewHandler>( new wxWebViewArchiveHandler( "wxfs" ) ) );
    m_browser->RegisterHandler( wxSharedPtr<wxWebViewHandler>( new wxWebViewFSHandler( "memory" ) ) );
#endif
    m_browser->SetUserAgent( wxString::Format( "KiCad/%s WebView/%s", GetMajorMinorPatchVersion(), wxGetOsDescription() ) );
    m_browser->Create( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize );
    sizer->Add( m_browser, 1, wxEXPAND );
    SetSizer( sizer );

#ifndef __WXMAC__
    m_browser->RegisterHandler( wxSharedPtr<wxWebViewHandler>( new wxWebViewArchiveHandler( "wxfs" ) ) );
    m_browser->RegisterHandler( wxSharedPtr<wxWebViewHandler>( new wxWebViewFSHandler( "memory" ) ) );
#endif

    Bind( wxEVT_WEBVIEW_NAVIGATING, &WEBVIEW_PANEL::OnNavigationRequest, this, m_browser->GetId() );
    Bind( wxEVT_WEBVIEW_NEWWINDOW, &WEBVIEW_PANEL::OnNewWindow, this, m_browser->GetId() );
    Bind( wxEVT_WEBVIEW_SCRIPT_MESSAGE_RECEIVED, &WEBVIEW_PANEL::OnScriptMessage, this, m_browser->GetId() );
    Bind( wxEVT_WEBVIEW_SCRIPT_RESULT, &WEBVIEW_PANEL::OnScriptResult, this, m_browser->GetId() );
    Bind( wxEVT_WEBVIEW_ERROR, &WEBVIEW_PANEL::OnError, this, m_browser->GetId() );
}

WEBVIEW_PANEL::~WEBVIEW_PANEL()
{
}

void WEBVIEW_PANEL::BindLoadedEvent()
{
    if( m_loadedEventBound )
        return;

    Bind( wxEVT_WEBVIEW_LOADED, &WEBVIEW_PANEL::OnWebViewLoaded, this, m_browser->GetId() );
    m_loadedEventBound = true;
}

void WEBVIEW_PANEL::LoadURL( const wxString& aURL )
{
    wxLogTrace( "webview", "Loading URL: %s", aURL );

    if( aURL.starts_with( "file:/" ) && !aURL.starts_with( "file:///" ) )
    {
        wxString new_url = wxString( "file:///" ) + aURL.AfterFirst( '/' );
        m_browser->LoadURL( new_url );
        return;
    }

    if( !aURL.StartsWith( "http://" ) && !aURL.StartsWith( "https://" ) && !aURL.StartsWith( "file://" ) )
    {
        wxLogError( "Invalid URL: %s", aURL );
        return;
    }

    m_browser->LoadURL( aURL );
}

void WEBVIEW_PANEL::SetPage( const wxString& aHtmlContent )
{
    wxLogTrace( "webview", "Setting page content" );
    m_browser->SetPage( aHtmlContent, "file://" );
}

bool WEBVIEW_PANEL::AddMessageHandler( const wxString& aName, MESSAGE_HANDLER aHandler )
{
    wxLogTrace( "webview", "Adding message handler for: %s", aName );
    auto it = m_msgHandlers.find( aName );

    if( it != m_msgHandlers.end() )
    {
        it->second = std::move( aHandler );
        return true;
    }

    m_msgHandlers.emplace( aName, std::move( aHandler ) );

    if( m_initialized )
    {
        if( !m_browser->AddScriptMessageHandler( aName ) )
            wxLogTrace( "webview", "Could not add script message handler %s", aName );
    }

    return true;
}

void WEBVIEW_PANEL::ClearMessageHandlers()
{
    wxLogTrace( "webview", "Clearing all message handlers" );

    for( const auto& handler : m_msgHandlers )
        m_browser->RemoveScriptMessageHandler( handler.first );

    m_msgHandlers.clear();
}

void WEBVIEW_PANEL::OnNavigationRequest( wxWebViewEvent& aEvt )
{
    m_loadError = false;
    wxLogTrace( "webview", "Navigation request to URL: %s", aEvt.GetURL() );
    // Default behavior: open external links in the system browser
    bool isExternal = aEvt.GetURL().StartsWith( "http://" ) || aEvt.GetURL().StartsWith( "https://" );

    if( isExternal && !m_handleExternalLinks )
    {
        wxLogTrace( "webview", "Opening external URL in system browser: %s", aEvt.GetURL() );
        wxLaunchDefaultBrowser( aEvt.GetURL() );
        aEvt.Veto();
    }
}

void WEBVIEW_PANEL::OnWebViewLoaded( wxWebViewEvent& aEvt )
{
    if( !m_initialized )
    {
        // Defer handler registration to avoid running during modal dialog/yield
        auto initFunc = [this]() {
            for( const auto& handler : m_msgHandlers )
            {
                if( !m_browser->AddScriptMessageHandler( handler.first ) )
                {
                    wxLogTrace( "webview", "Could not add script message handler %s", handler.first );
                }
            }

             // Inject navigation hook for SPA/JS navigation to prevent webkit crashing without new window
            m_browser->AddUserScript( R"(
                (function() {
                    // Change window.open to navigate in the same window
                    window.open = function(url) { if (url) window.location.href = url; return null; };
                    window.showModalDialog = function() { return null; };

                    if (window.external && window.external.invoke) {
                        function notifyHost() {
                            window.external.invoke('navigation:' + window.location.href);
                        }
                        window.addEventListener('popstate', notifyHost);
                        window.addEventListener('pushstate', notifyHost);
                        window.addEventListener('replacestate', notifyHost);
                        ['pushState', 'replaceState'].forEach(function(type) {
                            var orig = history[type];
                            history[type] = function() {
                                var rv = orig.apply(this, arguments);
                                window.dispatchEvent(new Event(type.toLowerCase()));
                                return rv;
                            };
                        });
                    }
                })();
            )" );

        };

        if( m_toolManager && m_tool )
            m_toolManager->RunMainStack( m_tool, initFunc );
        else
            CallAfter( initFunc );

        m_initialized = true;
    }

    aEvt.Skip();
}

void WEBVIEW_PANEL::OnNewWindow( wxWebViewEvent& aEvt )
{
    m_browser->LoadURL( aEvt.GetURL() );
    aEvt.Veto(); // Prevent default behavior of opening a new window
    wxLogTrace( "webview", "New window requested for URL: %s", aEvt.GetURL() );
    wxLogTrace( "webview", "Target: %s", aEvt.GetTarget() );
    wxLogTrace( "webview", "Action flags: %d", static_cast<int>(aEvt.GetNavigationAction()) );
    wxLogTrace( "webview", "Message handler: %s", aEvt.GetMessageHandler() );
}

void WEBVIEW_PANEL::OnScriptMessage( wxWebViewEvent& aEvt )
{
    wxLogTrace( "webview", "Script message received: %s for handler %s", aEvt.GetString(), aEvt.GetMessageHandler() );
    wxString handler = aEvt.GetMessageHandler();
    handler.Trim(true).Trim(false);

    if( handler.IsEmpty() )
    {
        for( auto handlerPair : m_msgHandlers )
        {
            wxLogTrace( "webview", "No handler specified, trying: %s", handlerPair.first );
            handlerPair.second( aEvt.GetString() );
        }

        return;
    }

    auto it = m_msgHandlers.find( handler );

    if( it == m_msgHandlers.end() )
    {
        wxLogTrace( "webview", "No handler registered for message: %s", handler );
        return;
    }

    // Call the registered handler with the message
    wxLogTrace( "webview", "Calling handler for message: %s", handler );
    it->second( aEvt.GetString() );
}


void WEBVIEW_PANEL::OnScriptResult( wxWebViewEvent& aEvt )
{
    if( aEvt.IsError() )
        wxLogTrace( "webview", "Async script execution failed: %s", aEvt.GetString() );
}

void WEBVIEW_PANEL::OnError( wxWebViewEvent& aEvt )
{
    m_loadError = true;
    wxLogTrace( "webview", "WebView error: %s", aEvt.GetString() );
}
