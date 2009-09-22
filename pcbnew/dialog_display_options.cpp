/////////////////////////////////////////////////////////////////////////////
// Name:        dialog_general_options.cpp
// Author:      jean-pierre Charras
/////////////////////////////////////////////////////////////////////////////
/* functions relatives to the dialog opened from the main menu :
    Prefernces/display
*/
#include "fctsys.h"
#include "common.h"
#include "class_drawpanel.h"
#include "confirm.h"
#include "pcbnew.h"
#include "wxPcbStruct.h"

#include "pcbnew_id.h"

#include "dialog_display_options_base.h"


class Dialog_Display_Options : public DialogDisplayOptions_base
{
private:
    WinEDA_BasePcbFrame* m_Parent;

    void init();

public:
    Dialog_Display_Options( WinEDA_BasePcbFrame* parent );
    ~Dialog_Display_Options( ) { };
    void OnOkClick( wxCommandEvent& event );
    void OnCancelClick( wxCommandEvent& event );
};


void WinEDA_PcbFrame::InstallDisplayOptionsDialog( wxCommandEvent& aEvent )
{
    Dialog_Display_Options* DisplayOptionsDialog =
        new Dialog_Display_Options( this );

    DisplayOptionsDialog->ShowModal();
    DisplayOptionsDialog->Destroy();
}


/*******************************************************************************/
Dialog_Display_Options::Dialog_Display_Options( WinEDA_BasePcbFrame* parent ) :
    DialogDisplayOptions_base(parent)
/*******************************************************************************/
{
    m_Parent = parent;

    init();
}

/****************************************************************/
void Dialog_Display_Options::init()
/****************************************************************/
{
    SetFocus();

    if ( DisplayOpt.DisplayPcbTrackFill )
        m_OptDisplayTracks->SetSelection(1);

    switch ( DisplayOpt.ShowTrackClearanceMode )
    {
        case DO_NOT_SHOW_CLEARANCE:
            m_OptDisplayTracksClearance->SetSelection(0);
            break;
        case SHOW_CLEARANCE_NEW_TRACKS:
            m_OptDisplayTracksClearance->SetSelection(1);
            break;
        default:
        case SHOW_CLEARANCE_NEW_TRACKS_AND_VIA_AREAS:
            m_OptDisplayTracksClearance->SetSelection(2);
            break;
        case SHOW_CLEARANCE_ALWAYS:
            m_OptDisplayTracksClearance->SetSelection(3);
            break;
    }

    if ( DisplayOpt.DisplayPadFill )
        m_OptDisplayPads->SetSelection(1);
    else
        m_OptDisplayPads->SetSelection(0);

    m_Show_Page_Limits->SetSelection( g_ShowPageLimits ? 0 : 1);

    m_OptDisplayViaHole->SetSelection( DisplayOpt.m_DisplayViaMode );
    m_OptDisplayModTexts->SetSelection( DisplayOpt.DisplayModText );
    m_OptDisplayModEdges->SetSelection( DisplayOpt.DisplayModEdge );
    m_OptDisplayPadClearence->SetValue( DisplayOpt.DisplayPadIsol );
    m_OptDisplayPadNumber->SetValue( DisplayOpt.DisplayPadNum );
    m_OptDisplayPadNoConn->SetValue( DisplayOpt.DisplayPadNoConn );
    m_OptDisplayDrawings->SetSelection( DisplayOpt.DisplayDrawItems );
    m_ShowNetNamesOption->SetSelection( DisplayOpt.DisplayNetNamesMode);

    if( GetSizer() )
    {
        GetSizer()->SetSizeHints( this );
    }
}

/*****************************************************************/
void Dialog_Display_Options::OnCancelClick( wxCommandEvent& event )
/*****************************************************************/
{
    event.Skip();
}

/*************************************************************************/
void Dialog_Display_Options::OnOkClick(wxCommandEvent& event)
/*************************************************************************/
/* Update variables with new options
*/
{
    if ( m_Show_Page_Limits->GetSelection() == 0 ) g_ShowPageLimits = TRUE;
    else g_ShowPageLimits = FALSE;

    if ( m_OptDisplayTracks->GetSelection() == 1)
        DisplayOpt.DisplayPcbTrackFill = TRUE;
    else DisplayOpt.DisplayPcbTrackFill = FALSE;

    m_Parent->m_DisplayPcbTrackFill = DisplayOpt.DisplayPcbTrackFill;
    DisplayOpt.m_DisplayViaMode = m_OptDisplayViaHole->GetSelection();

    switch ( m_OptDisplayTracksClearance->GetSelection() )
    {
        case 0:
            DisplayOpt.ShowTrackClearanceMode = DO_NOT_SHOW_CLEARANCE;
            break;
        case 1:
            DisplayOpt.ShowTrackClearanceMode = SHOW_CLEARANCE_NEW_TRACKS;
            break;
        case 2:
            DisplayOpt.ShowTrackClearanceMode = SHOW_CLEARANCE_NEW_TRACKS_AND_VIA_AREAS;
            break;
        case 3:
            DisplayOpt.ShowTrackClearanceMode = SHOW_CLEARANCE_ALWAYS;
            break;
    }

    m_Parent->m_DisplayModText = DisplayOpt.DisplayModText =
            m_OptDisplayModTexts->GetSelection();
    m_Parent->m_DisplayModEdge = DisplayOpt.DisplayModEdge =
            m_OptDisplayModEdges->GetSelection();

    if (m_OptDisplayPads->GetSelection() == 1 )
        DisplayOpt.DisplayPadFill = true;
    else
        DisplayOpt.DisplayPadFill = false;

    m_Parent->m_DisplayPadFill = DisplayOpt.DisplayPadFill;

    DisplayOpt.DisplayPadIsol = m_OptDisplayPadClearence->GetValue();

    m_Parent->m_DisplayPadNum = DisplayOpt.DisplayPadNum = m_OptDisplayPadNumber->GetValue();

    DisplayOpt.DisplayPadNoConn = m_OptDisplayPadNoConn->GetValue();

    DisplayOpt.DisplayDrawItems = m_OptDisplayDrawings->GetSelection();
    DisplayOpt.DisplayNetNamesMode = m_ShowNetNamesOption->GetSelection();

    m_Parent->DrawPanel->Refresh(TRUE);

    EndModal(1);
}
