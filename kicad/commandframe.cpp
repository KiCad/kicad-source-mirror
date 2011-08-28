/*****************************************************/
/*  commandframe.cpp: window handling comman buttons */
/*****************************************************/

#include "fctsys.h"
#include "common.h"
#include "bitmaps.h"
#include "macros.h"

#include "kicad.h"

#include "../bitmap2component/bitmap2component.xpm"
#include "../pcb_calculator/bitmaps/pcb_calculator.xpm"

RIGHT_KM_FRAME::RIGHT_KM_FRAME( KICAD_MANAGER_FRAME* parent ) :
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

}

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

/**
 * Function CreateCommandToolbar
 * create the buttons to call eescheman cvpcb, pcbnew and gerbview
 */
{
    wxBitmapButton* btn;

    btn = AddBitmapButton( ID_TO_EESCHEMA, KiBitmap( icon_eeschema_xpm ) );
    btn->SetToolTip( _( "EESchema (Schematic editor)" ) );

    btn = AddBitmapButton( ID_TO_CVPCB, KiBitmap( icon_cvpcb_xpm ) );
    btn->SetToolTip( _( "CVpcb (Components to modules)" ) );

    btn = AddBitmapButton( ID_TO_PCB, KiBitmap( a_icon_pcbnew_xpm ) );
    btn->SetToolTip( _( "PCBnew (PCB editor)" ) );

    btn = AddBitmapButton( ID_TO_GERBVIEW, KiBitmap( icon_gerbview_xpm ) );
    btn->SetToolTip( _( "GerbView (Gerber viewer)" ) );

    btn = AddBitmapButton( ID_TO_BITMAP_CONVERTER, KiBitmap( bitmap2component_xpm ) );
    btn->SetToolTip( _( "Bitmap2Component (a tool to build a logo from a bitmap)\n\
Creates a component (for Eeschema) or a footprint (for Pcbnew) that shows a B&W picture" ) );

    btn = AddBitmapButton( ID_TO_PCB_CALCULATOR, KiBitmap( pcb_calculator_xpm ) );
    btn->SetToolTip( _( "Pcb calculator" ) );
}


/**
 * Function AddBitmapButton
 * add a  Bitmap Button (fast launch button) to the buttons panel
 * @param aId = the button id
 * @param aBitmap = the wxBitmap used to create the button
 */
wxBitmapButton* RIGHT_KM_FRAME::AddBitmapButton( wxWindowID aId, const wxBitmap & aBitmap  )
{
    wxPoint buttPos = m_ButtonLastPosition;
    wxSize buttSize;
    int btn_margin = 10;
    buttSize.x = aBitmap.GetWidth() + btn_margin;
    buttSize.y = aBitmap.GetHeight() + btn_margin;
    buttPos.y -= buttSize.y;
    wxBitmapButton* btn = new wxBitmapButton( m_ButtPanel, aId, aBitmap, buttPos, buttSize);
    m_ButtonLastPosition.x += buttSize.x + m_ButtonSeparation;

    return btn;
}
