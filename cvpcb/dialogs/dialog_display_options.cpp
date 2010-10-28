/////////////////////////////////////////////////////////////////////////////
// Name:        dialog_display_options.cpp
// Licence:     GPL
/////////////////////////////////////////////////////////////////////////////
#include "fctsys.h"

#include "wxstruct.h"
#include "common.h"
#include "cvpcb.h"
//#include "protos.h"
#include "class_drawpanel.h"
#include "cvstruct.h"
#include "class_DisplayFootprintsFrame.h"

#include "dialog_display_options.h"


void DISPLAY_FOOTPRINTS_FRAME::InstallOptionsDisplay( wxCommandEvent& event )
{
    DIALOG_FOOTPRINTS_DISPLAY_OPTIONS* OptionWindow =
        new DIALOG_FOOTPRINTS_DISPLAY_OPTIONS( this );

    OptionWindow->ShowModal();
    OptionWindow->Destroy();
}


DIALOG_FOOTPRINTS_DISPLAY_OPTIONS::DIALOG_FOOTPRINTS_DISPLAY_OPTIONS(
    WinEDA_BasePcbFrame* parent )
    : DIALOG_FOOTPRINTS_DISPLAY_OPTIONS_BASE( parent)
{
    m_Parent = parent;

    initDialog( );
    GetSizer()->SetSizeHints( this );

    Centre();
}

DIALOG_FOOTPRINTS_DISPLAY_OPTIONS::~DIALOG_FOOTPRINTS_DISPLAY_OPTIONS( )
{
}


/*!
 * Control creation for DIALOG_FOOTPRINTS_DISPLAY_OPTIONS
 */

void DIALOG_FOOTPRINTS_DISPLAY_OPTIONS::initDialog()
{
    /* mandatory to use escape key as cancel under wxGTK. */
    SetFocus();

    m_EdgesDisplayOption->SetSelection( m_Parent->m_DisplayModEdge );
    m_TextDisplayOption->SetSelection( m_Parent->m_DisplayModText );
    m_IsShowPadFill->SetValue( m_Parent->m_DisplayPadFill );
    m_IsShowPadNum->SetValue( m_Parent->m_DisplayPadNum );
}



/*!
 * Update settings related to edges, text strings, and pads
 */

void DIALOG_FOOTPRINTS_DISPLAY_OPTIONS::UpdateObjectSettings( void )
{
    m_Parent->m_DisplayModEdge = m_EdgesDisplayOption->GetSelection();
    m_Parent->m_DisplayModText = m_TextDisplayOption->GetSelection();
    m_Parent->m_DisplayPadNum  = m_IsShowPadNum->GetValue();
    m_Parent->m_DisplayPadFill = m_IsShowPadFill->GetValue();
    m_Parent->SetToolbars();
    m_Parent->DrawPanel->Refresh();
}


/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_OK
 */

void DIALOG_FOOTPRINTS_DISPLAY_OPTIONS::OnOkClick( wxCommandEvent& event )
{
    UpdateObjectSettings();
    EndModal( 1 );
}


/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_CANCEL
 */

void DIALOG_FOOTPRINTS_DISPLAY_OPTIONS::OnCancelClick( wxCommandEvent& event )
{
    EndModal( -1 );
}


/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_APPLY
 */

void DIALOG_FOOTPRINTS_DISPLAY_OPTIONS::OnApplyClick( wxCommandEvent& event )
{
    UpdateObjectSettings();
}
