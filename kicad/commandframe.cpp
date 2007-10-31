/*****************************************************/
/*  commandframe.cpp: window handling comman buttons */
/*****************************************************/

#include "fctsys.h"
#include "common.h"

#include "kicad.h"
#include "macros.h"

#define BITMAP wxBitmap

// ----------------------------------------------------------------------------
// resources
// ----------------------------------------------------------------------------

// USE_XPM_BITMAPS
#include "bitmaps.h"
#include "id.h"


/************************************************************************************/
WinEDA_CommandFrame::WinEDA_CommandFrame( wxWindow* parent, int id,
                                          wxPoint pos, wxSize size, long style ) :
    wxSashLayoutWindow( parent, id, pos, size, style )
/************************************************************************************/

/** WinEDA_CommandFrame constructor
 * create the window which the buttons to call eeschema and others...
 */
{
    SetDefaultSize( wxSize( size.x, 100 ) );
    SetOrientation( wxLAYOUT_HORIZONTAL );
    SetAlignment( wxLAYOUT_TOP );
    SetSashVisible( wxSASH_BOTTOM, TRUE );
    SetSashVisible( wxSASH_LEFT, TRUE );
    SetExtraBorderSize( 2 );
    SetFont( *g_StdFont );
    CreateCommandToolbar();
}


/*************************************************/
void WinEDA_CommandFrame::CreateCommandToolbar( void )
/*************************************************/

/** Function CreateCommandToolbar
 * create the buttons to call eescheman cvpcb, pcbnew and gerbview
 */
{
    wxBitmapButton* btn;

    m_ButtonSeparation     = 10;
    m_ButtonLastPosition.x = 20;
    m_ButtonLastPosition.y = 20;

    btn = new wxBitmapButton( this, ID_TO_EESCHEMA, BITMAP( icon_eeschema_xpm ) );
    btn->SetToolTip( _( "eeschema (Schematic editor)" ) );
    AddFastLaunch( btn );

    btn = new wxBitmapButton( this, ID_TO_CVPCB, BITMAP( icon_cvpcb_xpm ) );
    btn->SetToolTip( _( "cvpcb (Components to modules)" ) );
    AddFastLaunch( btn );

    btn = new wxBitmapButton( this, ID_TO_PCB, BITMAP( a_icon_pcbnew_xpm ) );
    btn->SetToolTip( _( "pcbnew (PCB editor)" ) );
    AddFastLaunch( btn );

    btn = new wxBitmapButton( this, ID_TO_GERBVIEW, BITMAP( icon_gerbview_xpm ) );
    btn->SetToolTip( _( "gerbview (Gerber viewer)" ) );
    AddFastLaunch( btn );


    // Set up toolbar

    #ifdef KICAD_PYTHON
    btn = new wxBitmapButton( this, ID_RUN_PYTHON, BITMAP( icon_python_xpm ) );
    btn->SetToolTip( _( "Run Python Script" ) );
    AddFastLaunch( btn );
    #endif
}


/****************************************************************/
void WinEDA_CommandFrame::AddFastLaunch( wxBitmapButton * button )
/****************************************************************/
/** Function AddFastLaunch
  * add a  Bitmap Button (fast launch button) to the window
  * @param button = wxBitmapButton to add to the window
 */
{
    button->Move( m_ButtonLastPosition );
    m_ButtonLastPosition.x += button->GetSize().GetWidth() + m_ButtonSeparation;
}
