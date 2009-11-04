/////////////////////////////////////////////////////////////////////////////

// Name:        dialog_mask_clearance.cpp
// Author:      jean-pierre Charras
// Modified by:
// Created:     17 feb 2009
// Licence:     GPL
/////////////////////////////////////////////////////////////////////////////

#include "fctsys.h"
#include "common.h"
#include "confirm.h"
#include "pcbnew.h"
#include "wxPcbStruct.h"
#include "class_board_design_settings.h"

#include "dialog_mask_clearance.h"

/**
 *  DIALOG_PADS_MASK_CLEARANCE_BASE, derived from DIALOG_PADS_MASK_CLEARANCE_BASE_BASE
 *  @see dialog_dialog_mask_clearance_base.h and dialog_mask_clearance.cpp,
 *  automatically created by wxFormBuilder
 */

DIALOG_PADS_MASK_CLEARANCE::DIALOG_PADS_MASK_CLEARANCE( WinEDA_PcbFrame* parent ) :
    DIALOG_PADS_MASK_CLEARANCE_BASE( parent )
{
    m_Parent = parent;
    MyInit();
    GetSizer()->SetSizeHints( this );
}


void DIALOG_PADS_MASK_CLEARANCE::MyInit()
{
    SetFocus();

    m_SolderMaskMarginUnits->SetLabel( GetUnitsLabel( g_UnitMetric ) );
    m_SolderPasteMarginUnits->SetLabel( GetUnitsLabel( g_UnitMetric ) );

    int Internal_Unit = m_Parent->m_InternalUnits;
    PutValueInLocalUnits( *m_SolderMaskMarginCtrl,
                          g_DesignSettings.m_SolderMaskMargin,
                          Internal_Unit );
    PutValueInLocalUnits( *m_SolderPasteMarginCtrl,
                          g_DesignSettings.m_SolderPasteMargin,
                          Internal_Unit );
    wxString msg;
    msg.Printf( wxT( "%f" ), g_DesignSettings.m_SolderPasteMarginRatio * 100.0 );
    m_SolderPasteMarginRatioCtrl->SetValue( msg );
}


/*******************************************************************/
void DIALOG_PADS_MASK_CLEARANCE::OnButtonOkClick( wxCommandEvent& event )
/*******************************************************************/
{
    g_DesignSettings.m_SolderMaskMargin =
        ReturnValueFromTextCtrl( *m_SolderMaskMarginCtrl, m_Parent->m_InternalUnits );
    g_DesignSettings.m_SolderPasteMargin =
        ReturnValueFromTextCtrl( *m_SolderPasteMarginCtrl, m_Parent->m_InternalUnits );
    double   dtmp;
    wxString msg = m_SolderPasteMarginRatioCtrl->GetValue();
    msg.ToDouble( &dtmp );

    // A margin ratio de -50% means no paste on a pad, the ratio must be >= 50 %
    if( dtmp < -50 )
        dtmp = -50;
    g_DesignSettings.m_SolderPasteMarginRatio = dtmp / 100;

    EndModal( 1 );
}


/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_CANCEL
 */

void DIALOG_PADS_MASK_CLEARANCE::OnButtonCancelClick( wxCommandEvent& event )
{
    EndModal( 0 );
}
