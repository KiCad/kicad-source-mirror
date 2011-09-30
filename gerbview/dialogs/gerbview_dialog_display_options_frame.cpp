/*
 * file gerbview_dialog_display_options_frame.cpp
 * Set the display options for GerbView
 */


#include "fctsys.h"
#include "common.h"
#include "class_drawpanel.h"

#include "pcbplot.h"
#include "gerbview.h"
#include "gerbview_dialog_display_options_frame_base.h"


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
};


void GERBVIEW_FRAME::InstallGerberOptionsDialog( wxCommandEvent& event )
{
        DIALOG_DISPLAY_OPTIONS dlg( this );
        int opt = dlg.ShowModal();
        if (opt > 0 )
            DrawPanel->Refresh();
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

    m_PolarDisplay->SetSelection( DisplayOpt.DisplayPolarCood ? 1 : 0 );
    m_BoxUnits->SetSelection( g_UserUnit ? 1 : 0 );
    m_CursorShape->SetSelection( m_Parent->m_CursorShape ? 1 : 0 );

    // Show Option Draw Lines. We use DisplayPcbTrackFill as Lines draw option
    m_OptDisplayLines->SetSelection( DisplayOpt.DisplayPcbTrackFill ? 1 : 0 );
    m_OptDisplayFlashedItems->SetSelection( DisplayOpt.DisplayPadFill ? 1 : 0);
    // Show Option Draw polygons
    m_OptDisplayPolygons->SetSelection( g_DisplayPolygonsModeSketch ? 0 : 1 );

    m_ShowPageLimits->SetSelection(0);
    if( m_Parent->m_Draw_Sheet_Ref )
    {
        for( int ii = 1; g_GerberPageSizeList[ii] != NULL; ii++ )
        {
            if( m_Parent->GetScreen()->m_CurrentSheetDesc == g_GerberPageSizeList[ii] )
            {
                m_ShowPageLimits->SetSelection(ii);
                break;
            }
        }
    }

    m_OptDisplayDCodes->SetValue( m_Parent->IsElementVisible( DCODES_VISIBLE ) );
}

void DIALOG_DISPLAY_OPTIONS::OnOKBUttonClick( wxCommandEvent& event )
{
    DisplayOpt.DisplayPolarCood =
        (m_PolarDisplay->GetSelection() == 0) ? FALSE : TRUE;
    g_UserUnit  = (m_BoxUnits->GetSelection() == 0) ? INCHES : MILLIMETRES;
    m_Parent->m_CursorShape = m_CursorShape->GetSelection();

    if( m_OptDisplayLines->GetSelection() == 1 )
        DisplayOpt.DisplayPcbTrackFill = TRUE;
    else
        DisplayOpt.DisplayPcbTrackFill = FALSE;

    if( m_OptDisplayFlashedItems->GetSelection() == 1 )
    {
        DisplayOpt.DisplayPadFill = true;
        DisplayOpt.DisplayViaFill = true;
    }
    else
    {
        DisplayOpt.DisplayViaFill = false;
        DisplayOpt.DisplayPadFill = false;
    }


    if( m_OptDisplayPolygons->GetSelection() == 0 )
        g_DisplayPolygonsModeSketch = 1;
    else
        g_DisplayPolygonsModeSketch = 0;

    m_Parent->SetElementVisibility( DCODES_VISIBLE, m_OptDisplayDCodes->GetValue() );

    m_Parent->m_DisplayPadFill =  m_Parent->m_DisplayViaFill =
        DisplayOpt.DisplayViaFill;
    m_Parent->m_DisplayPcbTrackFill = DisplayOpt.DisplayPcbTrackFill;

    int idx = m_ShowPageLimits->GetSelection();
    if( idx > 0 )
        m_Parent->m_Draw_Sheet_Ref = true;
    else
        m_Parent->m_Draw_Sheet_Ref = false;

    m_Parent->GetScreen()->m_CurrentSheetDesc = g_GerberPageSizeList[idx];

    EndModal( 1 );
}

