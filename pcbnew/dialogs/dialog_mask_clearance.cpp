/////////////////////////////////////////////////////////////////////////////
// Name:        dialog_mask_clearance.cpp
// Author:      jean-pierre Charras
// Modified by:
// Created:     17 feb 2009
// Licence:     GPL
/////////////////////////////////////////////////////////////////////////////

#include "fctsys.h"
#include "confirm.h"
#include "pcbnew.h"
#include "wxPcbStruct.h"
#include "class_board_design_settings.h"

#include "class_board.h"

#include "dialog_mask_clearance.h"

/**
 *  DIALOG_PADS_MASK_CLEARANCE_BASE, derived from DIALOG_PADS_MASK_CLEARANCE_BASE_BASE
 *  @see dialog_dialog_mask_clearance_base.h and dialog_mask_clearance.cpp,
 *  automatically created by wxFormBuilder
 */

DIALOG_PADS_MASK_CLEARANCE::DIALOG_PADS_MASK_CLEARANCE( PCB_EDIT_FRAME* parent ) :
    DIALOG_PADS_MASK_CLEARANCE_BASE( parent )
{
    m_Parent = parent;
    m_BrdSettings = m_Parent->GetBoard()->GetDesignSettings();

    MyInit();
    m_sdbButtonsSizerOK->SetDefault();
    GetSizer()->SetSizeHints( this );
    Centre();
}


void DIALOG_PADS_MASK_CLEARANCE::MyInit()
{
    SetFocus();

    m_SolderMaskMarginUnits->SetLabel( GetUnitsLabel( g_UserUnit ) );
    m_SolderPasteMarginUnits->SetLabel( GetUnitsLabel( g_UserUnit ) );

    int Internal_Unit = m_Parent->GetInternalUnits();
    PutValueInLocalUnits( *m_SolderMaskMarginCtrl,
                           m_BrdSettings.m_SolderMaskMargin,
                          Internal_Unit );

    // These 2 parameters are usually < 0, so prepare entering a negative
    // value, if current is 0
    PutValueInLocalUnits( *m_SolderPasteMarginCtrl,
                           m_BrdSettings.m_SolderPasteMargin,
                          Internal_Unit );
    if(  m_BrdSettings.m_SolderPasteMargin == 0 )
        m_SolderPasteMarginCtrl->SetValue( wxT( "-" ) +
                                           m_SolderPasteMarginCtrl->GetValue() );
    wxString msg;
    msg.Printf( wxT( "%f" ), m_BrdSettings.m_SolderPasteMarginRatio * 100.0 );
    if(  m_BrdSettings.m_SolderPasteMarginRatio == 0.0 &&
        msg[0] == '0')  // Sometimes Printf add a sign if the value is small
        m_SolderPasteMarginRatioCtrl->SetValue( wxT( "-" ) + msg );
    else
        m_SolderPasteMarginRatioCtrl->SetValue( msg );
}


/*******************************************************************/
void DIALOG_PADS_MASK_CLEARANCE::OnButtonOkClick( wxCommandEvent& event )
/*******************************************************************/
{
    m_BrdSettings.m_SolderMaskMargin =
        ReturnValueFromTextCtrl( *m_SolderMaskMarginCtrl, m_Parent->GetInternalUnits() );

    m_BrdSettings.m_SolderPasteMargin =
        ReturnValueFromTextCtrl( *m_SolderPasteMarginCtrl, m_Parent->GetInternalUnits() );

    double   dtmp = 0;
    wxString msg = m_SolderPasteMarginRatioCtrl->GetValue();
    msg.ToDouble( &dtmp );

    // A margin ratio de -50% means no paste on a pad, the ratio must be >= 50 %
    if( dtmp < -50 )
        dtmp = -50;
    if( dtmp > +100 )
        dtmp = +100;

    m_BrdSettings.m_SolderPasteMarginRatio = dtmp / 100;

    m_Parent->GetBoard()->SetDesignSettings( m_BrdSettings );

    EndModal( 1 );
}


/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_CANCEL
 */

void DIALOG_PADS_MASK_CLEARANCE::OnButtonCancelClick( wxCommandEvent& event )
{
    EndModal( 0 );
}
