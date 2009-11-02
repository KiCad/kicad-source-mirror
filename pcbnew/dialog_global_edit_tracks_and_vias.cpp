/////////////////////////////////////////////////////////////////////////////

// Name:        dialog_global_edit_tracks_and_vias.cpp
// Author:      jean-pierre Charras
// Created:     30 oct 2009
// Licence:     GPL
/////////////////////////////////////////////////////////////////////////////

#include "fctsys.h"
#include "common.h"
#include "confirm.h"
#include "pcbnew.h"
#include "wxPcbStruct.h"
#include "class_drawpanel.h"

#include "dialog_global_edit_tracks_and_vias.h"

/**
 *  DIALOG_GLOBAL_EDIT_TRACKS_AND_VIAS_BASE, derived from DIALOG_GLOBAL_EDIT_TRACKS_AND_VIAS_BASE_BASE
 *  @see dialog_global_edit_tracks_and_vias_base.h and dialog_global_edit_tracks_and_vias_base.cpp,
 *  automatically created by wxFormBuilder
 */

DIALOG_GLOBAL_EDIT_TRACKS_AND_VIAS::DIALOG_GLOBAL_EDIT_TRACKS_AND_VIAS(
    WinEDA_PcbFrame* aParent, int aNetcode ) :
    DIALOG_GLOBAL_EDIT_TRACKS_AND_VIAS_BASE( aParent )
{
    m_Parent  = aParent;
    m_Netcode = aNetcode;
    MyInit();
    Layout();
    GetSizer()->Fit( this );
    GetSizer()->SetSizeHints( this );
}


/*************************************************/
void DIALOG_GLOBAL_EDIT_TRACKS_AND_VIAS::MyInit()
/*************************************************/
{
    SetFocus();

    wxString      msg;

    // Display current setup for tracks and vias
    int           Internal_Unit = m_Parent->m_InternalUnits;
    BOARD*        board = m_Parent->GetBoard();
    NETCLASSES&   netclasses = board->m_NetClasses;
    NETINFO_ITEM* net = board->FindNet( m_Netcode );
    NETCLASS*     netclass = netclasses.GetDefault();
    if( net )
    {
        m_CurrentNetName->SetLabel( net->GetNetname() );
        m_CurrentNetclassName->SetLabel( board->m_CurrentNetClassName );
        netclass = netclasses.Find( board->m_CurrentNetClassName );
    }

    // Enable/disable the option "copy current to net" if we ause only default netclass values
    if( !board->m_TrackWidthSelector && !board->m_ViaSizeSelector )
    {
        m_Net2CurrValueButton->Enable( false );
        m_Net2CurrValueText->Enable( false );
    }

    // Display current values, and current netclass values:
    int value = netclass->GetTrackWidth();
    msg = ReturnStringFromValue( g_UnitMetric, value, Internal_Unit, true );
    m_gridDisplayCurrentSettings->SetCellValue( 0, 0, msg  );
    if( board->m_TrackWidthSelector )
    {
        value = board->GetCurrentTrackWidth();
        msg   = ReturnStringFromValue( g_UnitMetric, value, Internal_Unit, true );
    }
    else
        msg = _( "Default" );
    m_gridDisplayCurrentSettings->SetCellValue( 1, 0, msg  );

    // recompute the column widths here, after setting texts

    value = netclass->GetViaDiameter();
    msg   = ReturnStringFromValue( g_UnitMetric, value, Internal_Unit, true );
    m_gridDisplayCurrentSettings->SetCellValue( 0, 1, msg  );
    if( board->m_ViaSizeSelector )
    {
        value = board->GetCurrentViaSize();
        msg   = ReturnStringFromValue( g_UnitMetric, value, Internal_Unit, true );
    }
    else
        msg = _( "Default" );
    m_gridDisplayCurrentSettings->SetCellValue( 1, 1, msg  );

    value = netclass->GetViaDrill();
    msg   = ReturnStringFromValue( g_UnitMetric, value, Internal_Unit, true );
    m_gridDisplayCurrentSettings->SetCellValue( 0, 2, msg  );
    value = board->GetCurrentViaDrill();
    if( value >= 0 )
        msg = ReturnStringFromValue( g_UnitMetric, value, Internal_Unit, true );
    else
        msg = _( "Default" );
    m_gridDisplayCurrentSettings->SetCellValue( 1, 2, msg  );

    value = netclass->GetuViaDiameter();
    msg   = ReturnStringFromValue( g_UnitMetric, value, Internal_Unit, true );
    m_gridDisplayCurrentSettings->SetCellValue( 0, 3, msg  );
#if 0   // Currently we use always the default netclass value
    value = board->GetCurrentMicroViaSize();
    msg   = ReturnStringFromValue( g_UnitMetric, value, Internal_Unit, true );
#endif
    msg = _( "Default" );
    m_gridDisplayCurrentSettings->SetCellValue( 1, 3, msg  );

    value = netclass->GetuViaDrill();
    msg   = ReturnStringFromValue( g_UnitMetric, value, Internal_Unit, true );
    m_gridDisplayCurrentSettings->SetCellValue( 0, 4, msg  );
#if 0   // Currently we use always the default netclass value
    value = board->GetCurrentMicroViaDrill();
    if( value >= 0 )
        msg = ReturnStringFromValue( g_UnitMetric, value, Internal_Unit, true );
    else
#endif
    msg = _( "Default" );
    m_gridDisplayCurrentSettings->SetCellValue( 1, 4, msg  );

    // Set all cells Roead Only
    for( int ii = 0; ii < m_gridDisplayCurrentSettings->GetNumberRows(); ii++ )
    {
        for( int jj = 0; jj < m_gridDisplayCurrentSettings->GetNumberCols(); jj++ )
            m_gridDisplayCurrentSettings->SetReadOnly( ii, jj, true );
    }

    m_gridDisplayCurrentSettings->Fit();
}


/*******************************************************************/
void DIALOG_GLOBAL_EDIT_TRACKS_AND_VIAS::OnOkClick( wxCommandEvent& event )
/*******************************************************************/
{
    bool change = false;

    switch( event.GetId() )
    {
    case ID_CURRENT_VALUES_TO_CURRENT_NET:
        if( !IsOK( this,
                  _( "Set current Net tracks and vias sizes and drill to the current values?" ) ) )
            return;
        {
            wxBusyCursor dummy;
            change = m_Parent->Change_Net_Tracks_And_Vias_Sizes( m_Netcode, false );
        }
        break;

    case ID_NETCLASS_VALUES_TO_CURRENT_NET:
        if( !IsOK( this,
                  _(
                      "Set current Net tracks and vias sizes and drill to the Netclass default value?" ) ) )
            return;
        {
            wxBusyCursor dummy;
            change = m_Parent->Change_Net_Tracks_And_Vias_Sizes( m_Netcode, true );
        }
        break;

    case ID_ALL_TRACKS_VIAS:
        if( !IsOK( this, _( "Set All Tracks and Vias to Netclass value" ) ) )
            return;
        {
            wxBusyCursor dummy;
            change = m_Parent->Reset_All_Tracks_And_Vias_To_Netclass_Values( true, true );
        }
        break;

    case ID_ALL_VIAS:
        if( !IsOK( this, _( "Set All Via to Netclass value" ) ) )
            return;
        {
            wxBusyCursor dummy;
            change = m_Parent->Reset_All_Tracks_And_Vias_To_Netclass_Values( false, true );
        }
        break;

    case ID_ALL_TRACKS:
        if( !IsOK( this, _( "Set All Track to Netclass value" ) ) )
            return;
        {
            wxBusyCursor dummy;
            change = m_Parent->Reset_All_Tracks_And_Vias_To_Netclass_Values( true, false );
        }
        break;
    }

    EndModal( 1 );
    if( change )
        m_Parent->DrawPanel->Refresh();
}


/*******************************
 *event handler for wxID_CANCEL
 ******************************/
void DIALOG_GLOBAL_EDIT_TRACKS_AND_VIAS::OnCancelClick( wxCommandEvent& event )
{
    EndModal( 0 );
}
