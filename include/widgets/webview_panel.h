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

#ifndef WEBVIEW_PANEL_H
#define WEBVIEW_PANEL_H

#include <wx/panel.h>
#include <wx/webview.h>
#include <functional>
#include <map>

class TOOL_MANAGER;
class TOOL_BASE;

class WEBVIEW_PANEL : public wxPanel
{
public:
    using MESSAGE_HANDLER = std::function<void( const wxString& )>;

    explicit WEBVIEW_PANEL( wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition,
                            const wxSize& size = wxDefaultSize, const int style = 0,
                            TOOL_MANAGER* aToolManager = nullptr, TOOL_BASE* aTool = nullptr );
    ~WEBVIEW_PANEL() override;

    wxWebView* GetWebView() const { return m_browser; }

    void LoadURL( const wxString& url );
    void SetPage( const wxString& htmlContent );

    bool AddMessageHandler( const wxString& name, MESSAGE_HANDLER handler );
    void ClearMessageHandlers();

    void SetHandleExternalLinks( bool aHandle ) { m_handleExternalLinks = aHandle; }
    bool GetHandleExternalLinks() const { return m_handleExternalLinks; }

    void RunScriptAsync( const wxString& aScript, void* aClientData = nullptr ) const
    {
        m_browser->RunScriptAsync( aScript, aClientData );
    }

    bool HasLoadError() const { return m_loadError; }

    void BindLoadedEvent();

protected:
    void OnNavigationRequest( wxWebViewEvent& evt );
    void OnWebViewLoaded( wxWebViewEvent& evt );
    void OnNewWindow( wxWebViewEvent& evt );
    void OnScriptMessage( wxWebViewEvent& evt );
    void OnScriptResult( wxWebViewEvent& evt );
    void OnError( wxWebViewEvent& evt );

private:

    bool                                m_initialized;
    bool                                m_handleExternalLinks;
    bool                                m_loadError;
    bool                                m_loadedEventBound;
    wxWebView*                          m_browser;
    std::map<wxString, MESSAGE_HANDLER> m_msgHandlers;
    TOOL_MANAGER*                       m_toolManager;
    TOOL_BASE*                          m_tool;
};

#endif // WEBVIEW_PANEL_H
