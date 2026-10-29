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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <pcb_edit_frame.h>
#include <board_design_settings.h>
#include <netclass.h>
#include <project/net_settings.h>
#include <board_stackup_manager/stackup_predefined_prms.h>
#include <via_protection_rules.h>
#include <widgets/via_cross_section_preview.h>
#include "panel_board_finish.h"


const std::vector<PANEL_SETUP_BOARD_FINISH::VIA_PROTECTION_CONTROL>& PANEL_SETUP_BOARD_FINISH::viaProtectionControls()
{
    using STATE = VIA_CROSS_SECTION_PREVIEW::STATE;

    static const std::vector<VIA_PROTECTION_CONTROL> controls = {
        { &PANEL_SETUP_BOARD_FINISH::m_fillVias, &BOARD_DESIGN_SETTINGS::m_FillVias, &STATE::m_Filled },
        { &PANEL_SETUP_BOARD_FINISH::m_capVias, &BOARD_DESIGN_SETTINGS::m_CapVias, &STATE::m_Capped },
        { &PANEL_SETUP_BOARD_FINISH::m_plugViasFront, &BOARD_DESIGN_SETTINGS::m_PlugViasFront, &STATE::m_PlugFront },
        { &PANEL_SETUP_BOARD_FINISH::m_plugViasBack, &BOARD_DESIGN_SETTINGS::m_PlugViasBack, &STATE::m_PlugBack },
        { &PANEL_SETUP_BOARD_FINISH::m_tentViasFront, &BOARD_DESIGN_SETTINGS::m_TentViasFront, &STATE::m_TentFront },
        { &PANEL_SETUP_BOARD_FINISH::m_tentViasBack, &BOARD_DESIGN_SETTINGS::m_TentViasBack, &STATE::m_TentBack },
        { &PANEL_SETUP_BOARD_FINISH::m_coverViasFront, &BOARD_DESIGN_SETTINGS::m_CoverViasFront, &STATE::m_CoverFront },
        { &PANEL_SETUP_BOARD_FINISH::m_coverViasBack, &BOARD_DESIGN_SETTINGS::m_CoverViasBack, &STATE::m_CoverBack },
    };

    return controls;
}


PANEL_SETUP_BOARD_FINISH::PANEL_SETUP_BOARD_FINISH( wxWindow* aParentWindow, PCB_EDIT_FRAME* aFrame ) :
        PANEL_SETUP_BOARD_FINISH_BASE( aParentWindow )
{
    m_frame = aFrame;
    m_board = m_frame->GetBoard();
    m_brdSettings = &m_board->GetDesignSettings();
    m_stackup = m_board->GetStackupOrDefault();
    m_copperLayerCount = m_board->GetCopperLayerCount();

    // Get the translated list of choices and init m_choiceFinish
    wxArrayString finish_list = GetStandardCopperFinishes( true );
    m_choiceFinish->Append( finish_list );
    m_choiceFinish->SetSelection( 0 );      // Will be correctly set later

    for( const VIA_PROTECTION_CONTROL& control : viaProtectionControls() )
        ( this->*control.m_CheckBox )->Bind( wxEVT_CHECKBOX, &PANEL_SETUP_BOARD_FINISH::onViaProtectionChanged, this );

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

    for( const VIA_PROTECTION_CONTROL& control : viaProtectionControls() )
        ( this->*control.m_CheckBox )->SetValue( m_brdSettings->*control.m_Setting );

    enableViaProtection();
    updateViaPreview();
}


void PANEL_SETUP_BOARD_FINISH::enableViaProtection()
{
    const bool filled = m_fillVias->GetValue();

    m_capVias->Enable( VIA_PROTECTION_RULES::CapAllowed( filled ) );
    m_plugViasFront->Enable( VIA_PROTECTION_RULES::PlugAllowed( filled ) );
    m_plugViasBack->Enable( VIA_PROTECTION_RULES::PlugAllowed( filled ) );
    m_coverViasFront->Enable(
            VIA_PROTECTION_RULES::CoverAllowed( filled, m_tentViasFront->GetValue(), m_plugViasFront->GetValue() ) );
    m_coverViasBack->Enable(
            VIA_PROTECTION_RULES::CoverAllowed( filled, m_tentViasBack->GetValue(), m_plugViasBack->GetValue() ) );
}


void PANEL_SETUP_BOARD_FINISH::updateViaPreview()
{
    VIA_CROSS_SECTION_PREVIEW::STATE state;

    state.m_InnerLayers = std::max( 0, m_copperLayerCount - 2 );
    VIA_CROSS_SECTION_GEOMETRY::SetLayersFromStackup( state, m_stackup, m_copperLayerCount );

    const std::shared_ptr<NETCLASS>& netclass = m_brdSettings->m_NetSettings->GetDefaultNetclass();

    if( netclass->GetViaDrill() > 0 )
        state.m_PadRatio = (double) netclass->GetViaDiameter() / netclass->GetViaDrill();

    for( const VIA_PROTECTION_CONTROL& control : viaProtectionControls() )
        state.*control.m_State = ( this->*control.m_CheckBox )->GetValue();

    m_viaPreview->SetState( state );
}


void PANEL_SETUP_BOARD_FINISH::onViaProtectionChanged( wxCommandEvent& aEvent )
{
    enableViaProtection();

    // A choice the user has just ruled out is cleared. A stored one is only greyed.
    bool cleared = false;

    for( const VIA_PROTECTION_CONTROL& control : viaProtectionControls() )
    {
        wxCheckBox* checkBox = this->*control.m_CheckBox;

        if( !checkBox->IsEnabled() && checkBox->GetValue() )
        {
            checkBox->SetValue( false );
            cleared = true;
        }
    }

    if( cleared )
        enableViaProtection();

    updateViaPreview();
}


void PANEL_SETUP_BOARD_FINISH::SetStackup( const BOARD_STACKUP& aStackup, int aCopperLayerCount )
{
    m_stackup = aStackup;
    m_copperLayerCount = aCopperLayerCount;
    updateViaPreview();
}


bool PANEL_SETUP_BOARD_FINISH::TransferDataFromWindow()
{
    BOARD_STACKUP& brd_stackup = m_brdSettings->GetStackupDescriptor();

    bool modified = TransferDataFromWindow( brd_stackup );

    for( const VIA_PROTECTION_CONTROL& control : viaProtectionControls() )
    {
        bool&      setting = m_brdSettings->*control.m_Setting;
        const bool value = ( this->*control.m_CheckBox )->GetValue();

        modified |= setting != value;
        setting = value;
    }

    if( modified )
    {
        m_board->BumpDrillModelGeneration();
        m_frame->OnModify();
    }

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

    updateViaPreview();
}
