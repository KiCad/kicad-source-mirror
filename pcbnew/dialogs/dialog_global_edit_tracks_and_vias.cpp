/////////////////////////////////////////////////////////////////////////////
// Name:        dialog_global_edit_tracks_and_vias.cpp
// Author:      jean-pierre Charras
// Created:     30 oct 2009
// Licence:     GPL
/////////////////////////////////////////////////////////////////////////////

#include <fctsys.h>
#include <confirm.h>
#include <pcbnew.h>
#include <wxPcbStruct.h>
#include <class_drawpanel.h>
#include <base_units.h>

#include <class_board.h>

#include <dialog_global_edit_tracks_and_vias.h>


/**
 *  DIALOG_GLOBAL_EDIT_TRACKS_AND_VIAS, derived from DIALOG_GLOBAL_EDIT_TRACKS_AND_VIAS_BASE_BASE
 *  @see dialog_global_edit_tracks_and_vias_base.h and dialog_global_edit_tracks_and_vias_base.cpp,
 *  automatically created by wxFormBuilder
 */

DIALOG_GLOBAL_EDIT_TRACKS_AND_VIAS::DIALOG_GLOBAL_EDIT_TRACKS_AND_VIAS( PCB_EDIT_FRAME* aParent,
                                                                        int             aNetcode ) :
    DIALOG_GLOBAL_EDIT_TRACKS_AND_VIAS_BASE( aParent )
{
    m_Parent  = aParent;
    m_Netcode = aNetcode;
    m_OptionID = 0;
    MyInit();
    GetSizer()->SetSizeHints( this );
    Layout();
}


/*************************************************/
void DIALOG_GLOBAL_EDIT_TRACKS_AND_VIAS::MyInit()
/*************************************************/
{
    SetFocus();

    wxString      msg;

    // Display current setup for tracks and vias
    BOARD*        board = m_Parent->GetBoard();
    BOARD_DESIGN_SETTINGS& dsnSettings = board->GetDesignSettings();
    NETCLASSES&   netclasses = dsnSettings.m_NetClasses;
    NETCLASS*     netclass = netclasses.GetDefault();
    NETINFO_ITEM* net = board->FindNet( m_Netcode );

    if( net )
    {
        m_CurrentNetName->SetLabel( net->GetNetname() );
        m_CurrentNetclassName->SetLabel( dsnSettings.GetCurrentNetClassName() );
        netclass = netclasses.Find( dsnSettings.GetCurrentNetClassName() );
    }

    /* Disable the option "copy current to net" if we have only default netclass values
     * i.e. when m_TrackWidthSelector and m_ViaSizeSelector are set to 0
     */
    if( !dsnSettings.GetTrackWidthIndex() && !dsnSettings.GetViaSizeIndex() )
    {
        m_Net2CurrValueButton->Enable( false );
        m_OptionID = ID_NETCLASS_VALUES_TO_CURRENT_NET;
        m_NetUseNetclassValueButton->SetValue(true);
    }
    else
     {
        m_OptionID = ID_CURRENT_VALUES_TO_CURRENT_NET;
        m_Net2CurrValueButton->SetValue(true);
    }

    // Display current values, and current netclass values:
    int value = netclass->GetTrackWidth();      // Display track width
    msg = StringFromValue( g_UserUnit, value, true );
    m_gridDisplayCurrentSettings->SetCellValue( 0, 0, msg  );

    if( dsnSettings.GetTrackWidthIndex() )
    {
        value = dsnSettings.GetCurrentTrackWidth();
        msg   = StringFromValue( g_UserUnit, value, true );
    }
    else
        msg = _( "Default" );

    m_gridDisplayCurrentSettings->SetCellValue( 1, 0, msg  );

    value = netclass->GetViaDiameter();      // Display via diameter
    msg   = StringFromValue( g_UserUnit, value, true );
    m_gridDisplayCurrentSettings->SetCellValue( 0, 1, msg  );

    if( dsnSettings.GetViaSizeIndex() )
    {
        value = dsnSettings.GetCurrentViaSize();
        msg   = StringFromValue( g_UserUnit, value, true );
    }
    else
        msg = _( "Default" );
    m_gridDisplayCurrentSettings->SetCellValue( 1, 1, msg  );

    value = netclass->GetViaDrill();      // Display via drill
    msg   = StringFromValue( g_UserUnit, value, true );
    m_gridDisplayCurrentSettings->SetCellValue( 0, 2, msg  );
    value = dsnSettings.GetCurrentViaDrill();
    if( value >= 0 )
        msg = StringFromValue( g_UserUnit, value, true );
    else
        msg = _( "Default" );
    m_gridDisplayCurrentSettings->SetCellValue( 1, 2, msg  );

    value = netclass->GetuViaDiameter();      // Display micro via diameter
    msg   = StringFromValue( g_UserUnit, value, true );
    m_gridDisplayCurrentSettings->SetCellValue( 0, 3, msg  );
#if 0   // Currently we use always the default netclass value
    value = board->GetCurrentMicroViaSize();
    msg   = StringFromValue( g_UserUnit, value, true );
#endif
    msg = _( "Default" );
    m_gridDisplayCurrentSettings->SetCellValue( 1, 3, msg  );

    value = netclass->GetuViaDrill();      // Display micro via drill
    msg   = StringFromValue( g_UserUnit, value, true );
    m_gridDisplayCurrentSettings->SetCellValue( 0, 4, msg  );
#if 0   // Currently we use always the default netclass value
    value = board->GetCurrentMicroViaDrill();
    if( value >= 0 )
        msg = StringFromValue( g_UserUnit, value, true );
    else
#endif
    msg = _( "Default" );
    m_gridDisplayCurrentSettings->SetCellValue( 1, 4, msg  );

    // Set all cells Read Only
    for( int ii = 0; ii < m_gridDisplayCurrentSettings->GetNumberRows(); ii++ )
    {
        for( int jj = 0; jj < m_gridDisplayCurrentSettings->GetNumberCols(); jj++ )
            m_gridDisplayCurrentSettings->SetReadOnly( ii, jj, true );
    }

    // needs wxWidgets version >= 2.8.8:
    m_gridDisplayCurrentSettings->SetRowLabelSize(wxGRID_AUTOSIZE);

    m_gridDisplayCurrentSettings->Fit();
}


/*******************************************************************/
void DIALOG_GLOBAL_EDIT_TRACKS_AND_VIAS::OnOkClick( wxCommandEvent& event )
/*******************************************************************/
{
    bool change = false;

    switch( m_OptionID )
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
        m_Parent->GetCanvas()->Refresh();
}


/*******************************
 *event handler for wxID_CANCEL
 ******************************/
void DIALOG_GLOBAL_EDIT_TRACKS_AND_VIAS::OnCancelClick( wxCommandEvent& event )
{
    EndModal( 0 );
}
