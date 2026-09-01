/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
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

#include <board_design_settings.h>
#include <confirm.h>
#include <lset.h>
#include <pcb_base_edit_frame.h>
#include <pcb_layer_box_selector.h>
#include <pcb_track.h>

#include <dialogs/dialog_microvia_stack.h>
#include <generators/pcb_via_stack.h>


DIALOG_MICROVIA_STACK::DIALOG_MICROVIA_STACK( PCB_BASE_EDIT_FRAME* aParent, PCB_VIA_STACK* aStack ) :
        DIALOG_MICROVIA_STACK_BASE( aParent ),
        m_stack( aStack ),
        m_viaSize( aParent, m_viaSizeLabel, m_viaSizeCtrl, m_viaSizeUnit ),
        m_viaDrill( aParent, m_viaDrillLabel, m_viaDrillCtrl, m_viaDrillUnit ),
        m_pitch( aParent, m_pitchLabel, m_pitchCtrl, m_pitchUnit )
{
    for( PCB_LAYER_BOX_SELECTOR* selector : { m_startLayer, m_endLayer } )
    {
        selector->SetLayersHotkeys( false );
        selector->SetNotAllowedLayerSet( LSET::AllNonCuMask() );
        selector->SetBoardFrame( aParent );
        selector->Resync();
    }

    m_typeChoice->Bind( wxEVT_CHOICE, &DIALOG_MICROVIA_STACK::onTypeChanged, this );
    m_useNetclass->Bind( wxEVT_CHECKBOX, &DIALOG_MICROVIA_STACK::onUseNetclass, this );

    SetupStandardButtons();
    finishDialogSettings();
}


bool DIALOG_MICROVIA_STACK::TransferDataToWindow()
{
    m_startLayer->SetLayerSelection( m_stack->GetStartLayer() );
    m_endLayer->SetLayerSelection( m_stack->GetEndLayer() );
    m_typeChoice->SetSelection( m_stack->GetStyle() == VIA_STACK_STYLE::STAGGERED ? 1 : 0 );

    m_useNetclass->SetValue( m_stack->GetUseNetclass() );
    m_filled->SetValue( m_stack->IsFilled() );
    m_capped->SetValue( m_stack->IsCapped() );

    m_viaSize.SetValue( m_stack->GetViaSize() );
    m_viaDrill.SetValue( m_stack->GetViaDrill() );
    m_pitch.SetValue( m_stack->GetPitch() );

    updateEnableState();
    return true;
}


bool DIALOG_MICROVIA_STACK::TransferDataFromWindow()
{
    PCB_LAYER_ID startLayer = ToLAYER_ID( m_startLayer->GetLayerSelection() );
    PCB_LAYER_ID endLayer = ToLAYER_ID( m_endLayer->GetLayerSelection() );
    bool         staggered = m_typeChoice->GetSelection() == 1;

    if( startLayer == endLayer )
    {
        DisplayErrorMessage( this, _( "The start and end layers must be different." ) );
        return false;
    }

    if( !PCB_VIA_STACK::IsSpanValid( m_stack->GetBoard(), startLayer, endLayer ) )
    {
        DisplayErrorMessage( this, _( "Select a start and an end layer present on this board." ) );
        return false;
    }

    if( staggered && m_pitch.GetIntValue() <= 0 )
    {
        DisplayErrorMessage( this, _( "Staggered stacks need a pitch greater than zero." ) );
        return false;
    }

    if( m_pitch.GetIntValue() > pcbIUScale.mmToIU( MAX_MICROVIA_STACK_PITCH_MM ) )
    {
        wxString limit = EDA_UNIT_UTILS::UI::MessageTextFromValue(
                pcbIUScale, GetUserUnits(), pcbIUScale.mmToIU( MAX_MICROVIA_STACK_PITCH_MM ) );

        DisplayErrorMessage( this, wxString::Format( _( "The pitch must be %s or less." ), limit ) );
        return false;
    }

    m_stack->SetStartLayer( startLayer );
    m_stack->SetEndLayer( endLayer );
    m_stack->SetStyle( staggered ? VIA_STACK_STYLE::STAGGERED : VIA_STACK_STYLE::STACKED );

    m_stack->SetUseNetclass( m_useNetclass->GetValue() );

    // Stacked stacks land on filled copper, so fill is forced on.
    m_stack->SetFilled( staggered ? m_filled->GetValue() : true );
    m_stack->SetCapped( m_capped->GetValue() );

    if( m_useNetclass->GetValue() )
    {
        m_stack->SetViaSize( 0 );
        m_stack->SetViaDrill( 0 );
    }
    else
    {
        if( std::optional<PCB_VIA::VIA_PARAMETER_ERROR> error = PCB_VIA::ValidateViaParameters(
                    m_viaSize.GetIntValue(), m_viaDrill.GetIntValue(), startLayer, endLayer ) )
        {
            DisplayErrorMessage( this, error->m_Message );
            return false;
        }

        m_stack->SetViaSize( m_viaSize.GetIntValue() );
        m_stack->SetViaDrill( m_viaDrill.GetIntValue() );
    }

    m_stack->SetPitch( m_pitch.GetIntValue() );
    return true;
}


void DIALOG_MICROVIA_STACK::updateEnableState()
{
    bool staggered = m_typeChoice->GetSelection() == 1;
    bool useNetclass = m_useNetclass->GetValue();

    // Pitch only applies to staggered stacks.
    m_pitch.Enable( staggered );

    // Size and drill come from the netclass when requested.
    m_viaSize.Enable( !useNetclass );
    m_viaDrill.Enable( !useNetclass );

    // Stacked stacks are always copper filled.
    m_filled->Enable( staggered );

    if( !staggered )
        m_filled->SetValue( true );
}


void DIALOG_MICROVIA_STACK::onTypeChanged( wxCommandEvent& aEvent )
{
    updateEnableState();
}


void DIALOG_MICROVIA_STACK::onUseNetclass( wxCommandEvent& aEvent )
{
    updateEnableState();
}
