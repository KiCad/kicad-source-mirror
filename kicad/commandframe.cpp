/*****************************************************/
/*  commandframe.cpp: window handling comman buttons */
/*****************************************************/

#include "fctsys.h"
#include "common.h"
#include "bitmaps.h"
#include "macros.h"

#include "kicad.h"

#include "../bitmap2component/bitmap2component_16x16.xpm"

RIGHT_KM_FRAME::RIGHT_KM_FRAME( WinEDA_MainFrame* parent ) :
    wxSashLayoutWindow( parent, wxID_ANY )
{
    #define BUTTON_HEIGHT 32
    m_Parent    = parent;
    m_DialogWin = NULL;
    m_ButtPanel = new wxPanel( this, wxID_ANY );
    m_ButtonSeparation     = 10;    // control of command buttons position
    m_ButtonsListPosition.x = 20;
    m_ButtonsListPosition.y = 20 + BUTTON_HEIGHT;
    m_ButtonLastPosition = m_ButtonsListPosition;
    m_ButtonsPanelHeight   = m_ButtonsListPosition.y + 20;
    CreateCommandToolbar();
    m_DialogWin = new wxTextCtrl( this, wxID_ANY, wxEmptyString,
                                  wxDefaultPosition, wxDefaultSize,
                                  wxTE_MULTILINE | wxSUNKEN_BORDER | wxTE_READONLY );

};

void RIGHT_KM_FRAME::OnSize( wxSizeEvent& event )
{
    #define EXTRA_MARGE 4
    wxSize wsize = GetClientSize();
    wsize.x -= EXTRA_MARGE;
    wsize.y -= m_ButtonsPanelHeight + EXTRA_MARGE;
    wxPoint wpos;
    wpos.x = EXTRA_MARGE/2;
    wpos.y = m_ButtonsPanelHeight + (EXTRA_MARGE/2);
    if( m_DialogWin )
    {
        m_DialogWin->SetSize( wsize );
        m_DialogWin->SetPosition(wpos );
    }

    wpos.y = EXTRA_MARGE/2;
    m_ButtPanel->SetPosition(wpos );
    wsize.y -= m_ButtonsPanelHeight - EXTRA_MARGE;
    m_ButtPanel->SetSize( wsize );
    m_ButtPanel->Refresh();

    event.Skip();
}


BEGIN_EVENT_TABLE( RIGHT_KM_FRAME, wxSashLayoutWindow )
EVT_SIZE( RIGHT_KM_FRAME::OnSize )
END_EVENT_TABLE()


/*************************************************/
void RIGHT_KM_FRAME::CreateCommandToolbar( void )
/*************************************************/

/** Function CreateCommandToolbar
 * create the buttons to call eescheman cvpcb, pcbnew and gerbview
 */
{
    wxBitmapButton* btn;

    wxWindow*       parent = m_ButtPanel;

    btn = new wxBitmapButton( parent, ID_TO_EESCHEMA, wxBitmap( icon_eeschema_xpm ) );
    btn->SetToolTip( _( "EESchema (Schematic editor)" ) );
    AddFastLaunch( btn );

    btn = new wxBitmapButton( parent, ID_TO_CVPCB, wxBitmap( icon_cvpcb_xpm ) );
    btn->SetToolTip( _( "CVpcb (Components to modules)" ) );
    AddFastLaunch( btn );

    btn = new wxBitmapButton( parent, ID_TO_PCB, wxBitmap( a_icon_pcbnew_xpm ) );
    btn->SetToolTip( _( "PCBnew (PCB editor)" ) );
    AddFastLaunch( btn );

    btn = new wxBitmapButton( parent, ID_TO_GERBVIEW, wxBitmap( icon_gerbview_xpm ) );
    btn->SetToolTip( _( "GerbView (Gerber viewer)" ) );
    AddFastLaunch( btn );

    btn = new wxBitmapButton( parent, ID_TO_BITMAP_CONVERTER,
            wxBitmap( bitmap2component_16x16_xpm ) );
    btn->SetToolTip( _( "Bitmap2Component (Bitmap converter to create logos)" ) );
    AddFastLaunch( btn );

}


/****************************************************************/
void RIGHT_KM_FRAME::AddFastLaunch( wxBitmapButton* button )
/****************************************************************/

/** Function AddFastLaunch
 * add a  Bitmap Button (fast launch button) to the window
 * @param button = wxBitmapButton to add to the window
 */
{
    wxPoint buttPos = m_ButtonLastPosition;
    buttPos.y -= button->GetSize().GetHeight();
    button->Move( buttPos );
    m_ButtonLastPosition.x += button->GetSize().GetWidth() + m_ButtonSeparation;
}
