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

    AddUnitSymbol( *m_MaskClearanceTitle );

    int Internal_Unit = m_Parent->m_InternalUnits;
    PutValueInLocalUnits( *m_OptMaskMargin, g_DesignSettings.m_MaskMargin, Internal_Unit );
}


/*******************************************************************/
void DIALOG_PADS_MASK_CLEARANCE::OnButtonOkClick( wxCommandEvent& event )
/*******************************************************************/
{
    g_DesignSettings.m_MaskMargin =
        ReturnValueFromTextCtrl( *m_OptMaskMargin, m_Parent->m_InternalUnits );

    EndModal( 1 );
}


/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_CANCEL
 */

void DIALOG_PADS_MASK_CLEARANCE::OnButtonCancelClick( wxCommandEvent& event )
{
    EndModal( 0 );
}

