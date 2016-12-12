/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2009-2016 Jean-Pierre Charras, jean-pierre.charras at wanadoo.fr
 * Copyright (C) 1992-2016 KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include <fctsys.h>
#include <confirm.h>
#include <pcbnew.h>
#include <wxPcbStruct.h>
#include <class_drawpanel.h>
#include <base_units.h>

#include <class_board.h>

#include <dialog_global_edit_tracks_and_vias.h>

#include <view/view.h>

/**
 *  DIALOG_GLOBAL_EDIT_TRACKS_AND_VIAS, derived from DIALOG_GLOBAL_EDIT_TRACKS_AND_VIAS_BASE_BASE
 *  @see dialog_global_edit_tracks_and_vias_base.h and dialog_global_edit_tracks_and_vias_base.cpp,
 *  automatically created by wxFormBuilder
 */

DIALOG_GLOBAL_EDIT_TRACKS_AND_VIAS::DIALOG_GLOBAL_EDIT_TRACKS_AND_VIAS( PCB_EDIT_FRAME* aParent,
                                                                        int             aNetcode ) :
    DIALOG_GLOBAL_EDIT_TRACKS_AND_VIAS_BASE( aParent )
{
    m_parent  = aParent;
    m_curr_netcode = aNetcode;
    m_optionID = 0;
    MyInit();

    GetSizer()->SetSizeHints( this );
}


void DIALOG_GLOBAL_EDIT_TRACKS_AND_VIAS::MyInit()
{
    SetFocus();

    // Display current setup for tracks and vias
    m_brd = m_parent->GetBoard();
    buildNetsList();
    updateNetInfo();

    m_gridDisplayCurrentSettings->Fit();
}


void DIALOG_GLOBAL_EDIT_TRACKS_AND_VIAS::updateNetInfo()
{
    BOARD_DESIGN_SETTINGS& brdSettings = m_brd->GetDesignSettings();
    NETCLASSES&   netclasses = brdSettings.m_NetClasses;
    NETCLASSPTR   netclass = netclasses.GetDefault();
    NETINFO_ITEM* net = m_brd->FindNet( m_curr_netcode );

    if( net )
    {
        netclass = net->GetNetClass();
        m_CurrentNetclassName->SetLabel( netclass->GetName() );
    }

    /* Disable the option "copy current to net" if we have only default netclass values
     * i.e. when m_TrackWidthSelector and m_ViaSizeSelector are set to 0
     */
    if( !brdSettings.GetTrackWidthIndex() && !brdSettings.GetViaSizeIndex() )
    {
        m_Net2CurrValueButton->Enable( false );
        m_optionID = ID_NETCLASS_VALUES_TO_CURRENT_NET;
        m_NetUseNetclassValueButton->SetValue(true);
    }
    else
    {
        m_optionID = ID_CURRENT_VALUES_TO_CURRENT_NET;
        m_Net2CurrValueButton->SetValue(true);
    }

    // Display current values, and current netclass values:
    wxString      msg;

    int value = netclass->GetTrackWidth();      // Display track width
    msg = StringFromValue( g_UserUnit, value, true );
    m_gridDisplayCurrentSettings->SetCellValue( 0, 0, msg  );

    if( brdSettings.GetTrackWidthIndex() > 0 )
    {
        value = brdSettings.GetCurrentTrackWidth();
        msg   = StringFromValue( g_UserUnit, value, true );
    }
    else
        msg = _( "Default" );

    m_gridDisplayCurrentSettings->SetCellValue( 1, 0, msg  );

    value = netclass->GetViaDiameter();      // Display via diameter
    msg   = StringFromValue( g_UserUnit, value, true );
    m_gridDisplayCurrentSettings->SetCellValue( 0, 1, msg  );

    if( brdSettings.GetViaSizeIndex() > 0 )
    {
        value = brdSettings.GetCurrentViaSize();
        msg   = StringFromValue( g_UserUnit, value, true );
    }
    else
        msg = _( "Default" );

    m_gridDisplayCurrentSettings->SetCellValue( 1, 1, msg  );

    value = netclass->GetViaDrill();      // Display via drill
    msg   = StringFromValue( g_UserUnit, value, true );
    m_gridDisplayCurrentSettings->SetCellValue( 0, 2, msg  );
    value = brdSettings.GetCurrentViaDrill();

    if( value >= 0 )
        msg = StringFromValue( g_UserUnit, value, true );
    else
        msg = _( "Default" );

    m_gridDisplayCurrentSettings->SetCellValue( 1, 2, msg  );

    value = netclass->GetuViaDiameter();      // Display micro via diameter
    msg   = StringFromValue( g_UserUnit, value, true );
    m_gridDisplayCurrentSettings->SetCellValue( 0, 3, msg  );
    msg = _( "Default" );
    m_gridDisplayCurrentSettings->SetCellValue( 1, 3, msg  );

    value = netclass->GetuViaDrill();      // Display micro via drill
    msg   = StringFromValue( g_UserUnit, value, true );
    m_gridDisplayCurrentSettings->SetCellValue( 0, 4, msg  );
    msg = _( "Default" );
    m_gridDisplayCurrentSettings->SetCellValue( 1, 4, msg  );

    // Set all cells Read Only
    for( int ii = 0; ii < m_gridDisplayCurrentSettings->GetNumberRows(); ii++ )
    {
        for( int jj = 0; jj < m_gridDisplayCurrentSettings->GetNumberCols(); jj++ )
            m_gridDisplayCurrentSettings->SetReadOnly( ii, jj, true );
    }

    m_gridDisplayCurrentSettings->SetRowLabelSize(wxGRID_AUTOSIZE);
}


void DIALOG_GLOBAL_EDIT_TRACKS_AND_VIAS::buildNetsList()
{
    wxString txt;

    // Populate the nets list with nets names
    for( unsigned netcode = 0; netcode < m_brd->GetNetCount(); netcode++ )
    {
        NETINFO_ITEM* net = m_brd->GetNetInfo().GetNetItem( netcode );
        wxString netname = net->GetNetname();

        if( netcode == 0 )  // netcode 0 is the netcode of not connected items
            netname = "<no net>";

        txt.Printf( _( "net %.3d" ), net->GetNet() );

        txt << "    "  << netname;

        m_choiceNetName->Append( txt );
    }

    if( m_curr_netcode < 0 )
        m_curr_netcode = 0;

    m_choiceNetName->SetSelection( m_curr_netcode );

    updateNetInfo();
}

void DIALOG_GLOBAL_EDIT_TRACKS_AND_VIAS::onNetSelection( wxCommandEvent& event )
{
    int idx = m_choiceNetName->GetSelection();

    if( idx == wxNOT_FOUND )
        return;

    m_curr_netcode = idx;
    updateNetInfo();
}

#include <ratsnest_data.h>

void DIALOG_GLOBAL_EDIT_TRACKS_AND_VIAS::OnOkClick( wxCommandEvent& event )
{
    bool change = false;

    switch( m_optionID )
    {
    case ID_CURRENT_VALUES_TO_CURRENT_NET:
        if( !IsOK( this,
                   _( "Set current Net tracks and vias sizes and drill to the current values?" ) ) )
            return;
        {
            wxBusyCursor dummy;
            change = m_parent->Change_Net_Tracks_And_Vias_Sizes( m_curr_netcode, false );
        }
        break;

    case ID_NETCLASS_VALUES_TO_CURRENT_NET:
        if( !IsOK( this,
                   _( "Set current Net tracks and vias sizes and drill to the Netclass default value?" ) ) )
            return;
        {
            wxBusyCursor dummy;
            change = m_parent->Change_Net_Tracks_And_Vias_Sizes( m_curr_netcode, true );
        }
        break;

    case ID_ALL_TRACKS_VIAS:
        if( !IsOK( this, _( "Set All Tracks and Vias to Netclass value" ) ) )
            return;
        {
            wxBusyCursor dummy;
            change = m_parent->Reset_All_Tracks_And_Vias_To_Netclass_Values( true, true );
        }
        break;

    case ID_ALL_VIAS:
        if( !IsOK( this, _( "Set All Via to Netclass value" ) ) )
            return;
        {
            wxBusyCursor dummy;
            change = m_parent->Reset_All_Tracks_And_Vias_To_Netclass_Values( false, true );
        }
        break;

    case ID_ALL_TRACKS:
        if( !IsOK( this, _( "Set All Track to Netclass value" ) ) )
            return;
        {
            wxBusyCursor dummy;
            change = m_parent->Reset_All_Tracks_And_Vias_To_Netclass_Values( true, false );
        }
        break;
    }

    if( change )
    {
        if( m_parent->IsGalCanvasActive() )
        {
            for( TRACK* track = m_parent->GetBoard()->m_Track; track != NULL; track = track->Next() )
                m_parent->GetGalCanvas()->GetView()->Update( track, KIGFX::GEOMETRY );
        }
        else
            m_parent->GetCanvas()->Refresh();
    }

    // Call the default handler
    event.Skip();
}
