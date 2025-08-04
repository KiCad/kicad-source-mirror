#ifndef WEBVIEW_PANEL_H
#define WEBVIEW_PANEL_H

#include <wx/panel.h>
#include <wx/webview.h>
#include <functional>
#include <map>

class WEBVIEW_PANEL : public wxPanel
{
public:
    using MESSAGE_HANDLER = std::function<void( const wxString& )>;

    explicit WEBVIEW_PANEL( wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition,
                            const wxSize& size = wxDefaultSize, const int style = 0 );
    ~WEBVIEW_PANEL() override;

    wxWebView* GetWebView() const { return m_browser; }

    void LoadURL( const wxString& url );
    void SetPage( const wxString& htmlContent );

    bool AddMessageHandler( const wxString& name, MESSAGE_HANDLER handler );
    void ClearMessageHandlers();

protected:
    void OnNavigationRequest( wxWebViewEvent& evt );
    void OnWebViewLoaded( wxWebViewEvent& evt );
    void OnNewWindow( wxWebViewEvent& evt );
    void OnScriptMessage( wxWebViewEvent& evt );
    void OnScriptResult( wxWebViewEvent& evt );
    void OnError( wxWebViewEvent& evt );

private:
    bool                                m_initialized;
    wxWebView*                          m_browser;
    std::map<wxString, MESSAGE_HANDLER> m_msgHandlers;
};

#endif // WEBVIEW_PANEL_H
