#include <build_version.h>
#include <widgets/webview_panel.h>
#include <wx/sizer.h>
#include <wx/webviewarchivehandler.h>
#include <wx/webviewfshandler.h>
#include <wx/utils.h>
#include <wx/log.h>

WEBVIEW_PANEL::WEBVIEW_PANEL( wxWindow* parent ) :
        wxPanel( parent ),
        m_browser( wxWebView::New() )
{
    wxBoxSizer* sizer = new wxBoxSizer( wxVERTICAL );

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
    Bind( wxEVT_WEBVIEW_LOADED, &WEBVIEW_PANEL::OnWebViewLoaded, this, m_browser->GetId() );
}

WEBVIEW_PANEL::~WEBVIEW_PANEL()
{
}

void WEBVIEW_PANEL::LoadURL( const wxString& url )
{
    m_browser->LoadURL( url );
}

bool WEBVIEW_PANEL::AddMessageHandler( const wxString& name, MESSAGE_HANDLER handler )
{
    m_msgHandlers.emplace( name, std::move(handler) );
    return true;
}

void WEBVIEW_PANEL::ClearMessageHandlers()
{
    m_msgHandlers.clear();
}

void WEBVIEW_PANEL::OnNavigationRequest( wxWebViewEvent& evt )
{
    // Default behavior: open external links in the system browser
    if( !evt.GetURL().StartsWith( "file:" ) && !evt.GetURL().StartsWith( "wxfs:" )
        && !evt.GetURL().StartsWith( "memory:" ) )
    {
        wxLaunchDefaultBrowser( evt.GetURL() );
        evt.Veto();
    }
}

void WEBVIEW_PANEL::OnWebViewLoaded( wxWebViewEvent& evt )
{
    if( !m_initialized )
    {
        // Defer handler registration to avoid running during modal dialog/yield
        CallAfter([this]() {
            static bool handler_added_inner = false;
            if (!handler_added_inner) {

                for( const auto& handler : m_msgHandlers )
                {
                    if( !m_browser->AddScriptMessageHandler( handler.first ) )
                    {
                        wxLogDebug( "Could not add script message handler %s", handler.first );
                    }
                }

                handler_added_inner = true;
            }

            // Inject navigation hook for SPA/JS navigation to prevent webkit crashing without new window
            m_browser->AddUserScript(R"(
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
            )");
        });

        m_initialized = true;
    }
}

void WEBVIEW_PANEL::OnNewWindow( wxWebViewEvent& evt )
{
    m_browser->LoadURL( evt.GetURL() );
    evt.Veto(); // Prevent default behavior of opening a new window
    wxLogTrace( "webview", "New window requested for URL: %s", evt.GetURL() );
    wxLogTrace( "webview", "Target: %s", evt.GetTarget() );
    wxLogTrace( "webview", "Action flags: %d", static_cast<int>(evt.GetNavigationAction()) );
    wxLogTrace( "webview", "Message handler: %s", evt.GetMessageHandler() );
}

void WEBVIEW_PANEL::OnScriptMessage( wxWebViewEvent& evt )
{
    wxLogTrace( "webview", "Script message received: %s for handler %s", evt.GetString(), evt.GetMessageHandler() );

    if( evt.GetMessageHandler().IsEmpty() )
    {
        wxLogDebug( "No message handler specified for script message: %s", evt.GetString() );
        return;
    }

    auto it = m_msgHandlers.find( evt.GetMessageHandler() );
    if( it == m_msgHandlers.end() )
    {
        wxLogDebug( "No handler registered for message: %s", evt.GetMessageHandler() );
        return;
    }

    // Call the registered handler with the message
    wxLogTrace( "webview", "Calling handler for message: %s", evt.GetMessageHandler() );
    it->second( evt.GetString() );
}

void WEBVIEW_PANEL::OnScriptResult( wxWebViewEvent& evt )
{
    if( evt.IsError() )
        wxLogDebug( "Async script execution failed: %s", evt.GetString() );
}

void WEBVIEW_PANEL::OnError( wxWebViewEvent& evt )
{
    wxLogDebug( "WebView error: %s", evt.GetString() );
}
