/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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

#include <pcb_edit_frame.h>
#include <board_design_settings.h>
#include <board_stackup_manager/stackup_predefined_prms.h>
#include "panel_board_finish.h"


PANEL_SETUP_BOARD_FINISH::PANEL_SETUP_BOARD_FINISH( wxWindow* aParentWindow,
                                                    PCB_EDIT_FRAME* aFrame ) :
        PANEL_SETUP_BOARD_FINISH_BASE( aParentWindow )
{
    m_frame = aFrame;
    m_board = m_frame->GetBoard();
    m_brdSettings = &m_board->GetDesignSettings();

    // Get the translated list of choices and init m_choiceFinish
    wxArrayString finish_list = GetStandardCopperFinishes( true );
    m_choiceFinish->Append( finish_list );
    m_choiceFinish->SetSelection( 0 );      // Will be correctly set later

    synchronizeWithBoard();
}


PANEL_SETUP_BOARD_FINISH::~PANEL_SETUP_BOARD_FINISH()
{
}


void PANEL_SETUP_BOARD_FINISH::synchronizeWithBoard()
{
    const BOARD_STACKUP& brd_stackup = m_brdSettings->GetStackupDescriptor();

    m_choiceEdgeConn->SetSelection( brd_stackup.m_EdgeConnectorConstraints );
    m_cbEgdesPlated->SetValue( brd_stackup.m_EdgePlating );

    // find the choice depending on the initial finish setting
    wxArrayString initial_finish_list = GetStandardCopperFinishes( false );
    unsigned idx;

    for( idx = 0; idx < initial_finish_list.GetCount(); idx++ )
    {
        if( initial_finish_list[idx] ==  brd_stackup.m_FinishType )
            break;
    }

    // Now init the choice (use last choice: "User defined" if not found )
    if( idx >= initial_finish_list.GetCount() )
        idx = initial_finish_list.GetCount()-1;

    m_choiceFinish->SetSelection( idx );
}


bool PANEL_SETUP_BOARD_FINISH::TransferDataFromWindow()
{
    BOARD_STACKUP& brd_stackup = m_brdSettings->GetStackupDescriptor();

    if( TransferDataFromWindow( brd_stackup ) )
        m_frame->OnModify();

    return true;
}


bool PANEL_SETUP_BOARD_FINISH::TransferDataFromWindow( BOARD_STACKUP& aStackup )
{
    wxArrayString finish_list = GetStandardCopperFinishes( false );
    int finish = m_choiceFinish->GetSelection() >= 0 ? m_choiceFinish->GetSelection() : 0;
    bool modified = aStackup.m_FinishType != finish_list[finish];
    aStackup.m_FinishType = finish_list[finish];

    int edge = m_choiceEdgeConn->GetSelection();
    modified |= aStackup.m_EdgeConnectorConstraints != (BS_EDGE_CONNECTOR_CONSTRAINTS) edge;
    aStackup.m_EdgeConnectorConstraints = (BS_EDGE_CONNECTOR_CONSTRAINTS) edge;

    modified |= aStackup.m_EdgePlating != m_cbEgdesPlated->GetValue();
    aStackup.m_EdgePlating = m_cbEgdesPlated->GetValue();

    return modified;
}


void PANEL_SETUP_BOARD_FINISH::ImportSettingsFrom( BOARD* aBoard )
{
    BOARD*                 savedBrd = m_board;
    BOARD_DESIGN_SETTINGS* savedSettings = m_brdSettings;
    m_brdSettings = &aBoard->GetDesignSettings();

    synchronizeWithBoard();

    m_brdSettings = savedSettings;
    m_board = savedBrd;
}
