/**
 * @file gerbview_dialog_display_options_frame.cpp
 * Set some display options for GerbView
 */


#include <fctsys.h>
#include <common.h>
#include <macros.h>
#include <class_drawpanel.h>

#include <pcbplot.h>
#include <gerbview.h>
#include <gerbview_frame.h>
#include <gerbview_dialog_display_options_frame_base.h>


/*******************************************/
/* Dialog frame to select display options */
/*******************************************/
class DIALOG_DISPLAY_OPTIONS : public DIALOG_DISPLAY_OPTIONS_BASE
{
private:
    GERBVIEW_FRAME* m_Parent;

public:

    DIALOG_DISPLAY_OPTIONS( GERBVIEW_FRAME* parent );
    ~DIALOG_DISPLAY_OPTIONS() {};

private:
    void OnOKBUttonClick( wxCommandEvent& event );
    void OnCancelButtonClick( wxCommandEvent& event );
    void initOptDialog( );
    void OnMiddleBtnPanEnbl( wxCommandEvent& event )
    {
        m_OptMiddleButtonPanLimited->Enable( m_OptMiddleButtonPan->GetValue() );
    }
};


void GERBVIEW_FRAME::InstallGerberOptionsDialog( wxCommandEvent& event )
{
    DIALOG_DISPLAY_OPTIONS dlg( this );
    int opt = dlg.ShowModal();

    if( opt > 0 )
        m_canvas->Refresh();
}

DIALOG_DISPLAY_OPTIONS::DIALOG_DISPLAY_OPTIONS( GERBVIEW_FRAME *parent) :
    DIALOG_DISPLAY_OPTIONS_BASE( parent, wxID_ANY )
{
    m_Parent = parent;
    SetFocus();
    initOptDialog( );

    GetSizer()->Fit( this );
    GetSizer()->SetSizeHints( this );
    Center();
    m_sdbSizer1OK->SetDefault();
}


void DIALOG_DISPLAY_OPTIONS::OnCancelButtonClick( wxCommandEvent& event )
{
    EndModal( 0 );
}


void DIALOG_DISPLAY_OPTIONS::initOptDialog( )
{
    m_PolarDisplay->SetSelection( m_Parent->m_DisplayOptions.m_DisplayPolarCood ? 1 : 0 );
    m_BoxUnits->SetSelection( g_UserUnit ? 1 : 0 );
    m_CursorShape->SetSelection( m_Parent->GetCursorShape() ? 1 : 0 );

    // Show Option Draw Lines. We use DisplayPcbTrackFill as Lines draw option
    m_OptDisplayLines->SetSelection( m_Parent->m_DisplayOptions.m_DisplayLinesFill ? 1 : 0 );
    m_OptDisplayFlashedItems->SetSelection( m_Parent->m_DisplayOptions.m_DisplayFlashedItemsFill ? 1 : 0);

    // Show Option Draw polygons
    m_OptDisplayPolygons->SetSelection( m_Parent->m_DisplayOptions.m_DisplayPolygonsFill ? 1 : 0 );

    m_ShowPageLimits->SetSelection(0);

    if( m_Parent->GetShowBorderAndTitleBlock() )
    {
        wxString    curPaperType = m_Parent->GetPageSettings().GetType();

        for( unsigned i = 1;  i < DIM( g_GerberPageSizeList );  ++i )
        {
            if( g_GerberPageSizeList[i] == curPaperType )
            {
                m_ShowPageLimits->SetSelection( i );
                break;
            }
        }
    }

    m_OptDisplayDCodes->SetValue( m_Parent->IsElementVisible( DCODES_VISIBLE ) );


    m_OptZoomNoCenter->SetValue( m_Parent->GetCanvas()->GetEnableZoomNoCenter() );
    m_OptMiddleButtonPan->SetValue( m_Parent->GetCanvas()->GetEnableMiddleButtonPan() );
    m_OptMiddleButtonPanLimited->SetValue( m_Parent->GetCanvas()->GetMiddleButtonPanLimited() );
    m_OptMiddleButtonPanLimited->Enable( m_OptMiddleButtonPan->GetValue() );
}


void DIALOG_DISPLAY_OPTIONS::OnOKBUttonClick( wxCommandEvent& event )
{
    m_Parent->m_DisplayOptions.m_DisplayPolarCood =
        (m_PolarDisplay->GetSelection() == 0) ? false : true;
    g_UserUnit  = (m_BoxUnits->GetSelection() == 0) ? INCHES : MILLIMETRES;
    m_Parent->SetCursorShape( m_CursorShape->GetSelection() );

    if( m_OptDisplayLines->GetSelection() == 1 )
        m_Parent->m_DisplayOptions.m_DisplayLinesFill = true;
    else
        m_Parent->m_DisplayOptions.m_DisplayLinesFill = false;

    if( m_OptDisplayFlashedItems->GetSelection() == 1 )
    {
        m_Parent->m_DisplayOptions.m_DisplayFlashedItemsFill = true;
    }
    else
    {
        m_Parent->m_DisplayOptions.m_DisplayFlashedItemsFill = false;
    }


    if( m_OptDisplayPolygons->GetSelection() == 0 )
        m_Parent->m_DisplayOptions.m_DisplayPolygonsFill = false;
    else
        m_Parent->m_DisplayOptions.m_DisplayPolygonsFill = true;

    m_Parent->SetElementVisibility( DCODES_VISIBLE, m_OptDisplayDCodes->GetValue() );

    int idx = m_ShowPageLimits->GetSelection();

    m_Parent->SetShowBorderAndTitleBlock( idx > 0  ?  true : false );

    PAGE_INFO   pageInfo( g_GerberPageSizeList[idx] );

    m_Parent->SetPageSettings( pageInfo );

    m_Parent->GetCanvas()->SetEnableZoomNoCenter( m_OptZoomNoCenter->GetValue() );
    m_Parent->GetCanvas()->SetEnableMiddleButtonPan( m_OptMiddleButtonPan->GetValue() );
    m_Parent->GetCanvas()->SetMiddleButtonPanLimited( m_OptMiddleButtonPanLimited->GetValue() );

    m_Parent->GetCanvas()->Refresh();

    EndModal( 1 );
}

