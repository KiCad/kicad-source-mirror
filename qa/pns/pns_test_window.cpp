#include <pcb_test_frame.h>
#include <pcb_painter.h>

#include <view/view_overlay.h>

#include "pns_log_viewer_frame_base.h"
#include "pns_log.h"


class PNS_TEST_FRAME : public PNS_LOG_VIEWER_FRAME_BASE
{
public:
    PNS_TEST_FRAME( wxFrame* frame,
            const wxString& title,
            const wxPoint& pos  = wxDefaultPosition,
            const wxSize& size  = wxDefaultSize,
            long style = wxDEFAULT_FRAME_STYLE ) :
        PNS_LOG_VIEWER_FRAME_BASE( frame, title, pos, size, style )
    {
        #if 0
                // Make a menubar
        wxMenu* fileMenu = new wxMenu;

        fileMenu->Append( wxID_OPEN, wxT( "&Open..." ) );
        fileMenu->AppendSeparator();
        fileMenu->Append( wxID_EXIT, wxT( "E&xit" ) );
        wxMenuBar* menuBar = new wxMenuBar;
        menuBar->Append( fileMenu, wxT( "&File" ) );

        createMenus( menuBar );
        SetMenuBar( menuBar );

        createUserUI( this );

        Show( true );
        Maximize();
        Raise();

        auto settings = static_cast<KIGFX::PCB_RENDER_SETTINGS*> ( m_galPanel->GetView()->GetPainter()->GetSettings() );
        settings->SetZoneDisplayMode( KIGFX::PCB_RENDER_SETTINGS::DZ_HIDE_FILLED );
        #endif
    }

    virtual ~PNS_TEST_FRAME() {}

    void LoadLogFile( const std::string& aFileName );

private:
#if 0
    void createMenus( wxMenuBar *menubar ) override
    {
        printf("pns::create menus\n");
        wxMenu* testMenu = new wxMenu;

        testMenu->Append( -1, wxT( "Select run" ) );
        testMenu->Append( -1, wxT( "Replay" ) );
        testMenu->Append( -1, wxT( "Dump geometry to file" ) );

        menubar->Append( testMenu, wxT("Tests") );
    }

    void createUserUI( wxWindow *aParent ) override;
    void drawLoggedItems();

    wxSlider* m_historySlider;
#endif

    std::unique_ptr<KIGFX::VIEW_OVERLAY> m_overlay;
    PNS_LOG_FILE m_logFile;
    PNS_TEST_ENVIRONMENT m_env;
};


wxFrame* CreateMainFrame( const std::string& aFileName )
{
    auto frame = new PNS_TEST_FRAME( nullptr, wxT( "P&S Test" ) );

    frame->LoadLogFile( aFileName );

    return frame;
}


void PNS_TEST_FRAME::LoadLogFile( const std::string& aFileName )
{
    printf("Loading P&S log data from '%s'\n", aFileName.c_str() );
    
    bool rv = m_logFile.Load( aFileName );

    if(!rv)
    {
        printf("Log load failure\n");
        return;
    }

    SetBoard( m_logFile.GetBoard() );

    m_overlay.reset( new KIGFX::VIEW_OVERLAY() );
    m_galPanel->GetView()->Add( m_overlay.get() );

    m_env.SetMode( PNS::PNS_MODE_ROUTE_SINGLE );
    m_env.ReplayLog( &m_logFile );

    drawLoggedItems();
}