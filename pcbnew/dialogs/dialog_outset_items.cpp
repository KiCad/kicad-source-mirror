
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
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include "dialogs/dialog_outset_items.h"

#include <board.h>
#include <board_design_settings.h>
#include <pcb_layer_box_selector.h>
#include <wx/msgdlg.h>
#include <confirm.h>

/**
 * Some handy preset values for common outset distances.
 */
static const std::vector<int> s_outsetPresetValue{
    // Outsetting a 0.1mm line to touch a 0.1mm line
    pcbIUScale.mmToIU( 0.1 ),
    // 0.12mm line to touch a 0.1mm line
    pcbIUScale.mmToIU( 0.11 ),
    // IPC dense courtyard
    pcbIUScale.mmToIU( 0.15 ),
    // IPC normal courtyard
    pcbIUScale.mmToIU( 0.25 ),
    // Keep 0.12mm silkscreen line 0.2mm from copper
    pcbIUScale.mmToIU( 0.26 ),
    // IPC connector courtyard
    pcbIUScale.mmToIU( 0.5 ),
    // Common router bits
    pcbIUScale.mmToIU( 1.0 ),
    pcbIUScale.mmToIU( 2.0 ),
};

// Ther user can also get the current board design settings widths
// with the "Layer Default" button.
static const std::vector<int> s_presetLineWidths{
    // Courtyard
    pcbIUScale.mmToIU( 0.05 ),
    pcbIUScale.mmToIU( 0.1 ),
    // Silkscreen
    pcbIUScale.mmToIU( 0.12 ),
    pcbIUScale.mmToIU( 0.15 ),
    pcbIUScale.mmToIU( 0.2 ),
};

static const std::vector<int> s_presetGridRounding{
    // 0.01 is a common IPC grid round-off value
    pcbIUScale.mmToIU( 0.01 ),
};

static int s_gridRoundValuePersist = s_presetGridRounding[0];

static std::vector<int> s_outsetRecentValues;
static std::vector<int> s_lineWidthRecentValues;
static std::vector<int> s_gridRoundingRecentValues;


DIALOG_OUTSET_ITEMS::DIALOG_OUTSET_ITEMS( PCB_BASE_FRAME&             aParent,
                                          OUTSET_ROUTINE::PARAMETERS& aParams ) :
        DIALOG_OUTSET_ITEMS_BASE( &aParent ),
        m_parent( aParent ),
        m_params( aParams ),
        m_outset( &aParent, m_outsetLabel, m_outsetEntry, m_outsetUnit ),
        m_lineWidth( &aParent, m_lineWidthLabel, m_lineWidthEntry, m_lineWidthUnit ),
        m_roundingGrid( &aParent, m_gridRoundingLabel, m_gridRoundingEntry, m_gridRoundingUnit )
{
    m_LayerSelectionCtrl->ShowNonActivatedLayers( false );
    m_LayerSelectionCtrl->SetLayersHotkeys( false );
    m_LayerSelectionCtrl->SetBoardFrame( &aParent );
    m_LayerSelectionCtrl->Resync();

    const auto fillOptionList =
            [&]( UNIT_BINDER& aCombo, const std::vector<int>& aPresets, const std::vector<int>& aRecentPresets )
            {
                std::vector<long long int> optionList;
                optionList.reserve( aPresets.size() + aRecentPresets.size() );

                for( const int val : aPresets )
                    optionList.push_back( val );

                for( const int val : aRecentPresets )
                    optionList.push_back( val );

                // Sort the vector and remove duplicates
                std::sort( optionList.begin(), optionList.end() );
                optionList.erase( std::unique( optionList.begin(), optionList.end() ), optionList.end() );

                aCombo.SetOptionsList( optionList );
            };

    fillOptionList( m_outset, s_outsetPresetValue, s_outsetRecentValues );
    fillOptionList( m_lineWidth, s_presetLineWidths, s_lineWidthRecentValues );
    fillOptionList( m_roundingGrid, s_presetGridRounding, s_gridRoundingRecentValues );

    SetupStandardButtons();
    finishDialogSettings();
}


DIALOG_OUTSET_ITEMS::~DIALOG_OUTSET_ITEMS()
{
}


void DIALOG_OUTSET_ITEMS::OnLayerDefaultClick( wxCommandEvent& event )
{
    const BOARD_DESIGN_SETTINGS& settings = m_parent.GetBoard()->GetDesignSettings();

    const PCB_LAYER_ID selLayer = ToLAYER_ID( m_LayerSelectionCtrl->GetLayerSelection() );
    const int          defaultWidth = settings.GetLineThickness( selLayer );

    m_lineWidth.SetValue( defaultWidth );
}


void DIALOG_OUTSET_ITEMS::OnCopyLayersChecked( wxCommandEvent& event )
{
    m_LayerSelectionCtrl->Enable( !m_copyLayers->GetValue() );
}


void DIALOG_OUTSET_ITEMS::OnRoundToGridChecked( wxCommandEvent& event )
{
    m_gridRoundingEntry->Enable( m_roundToGrid->IsChecked() );
}


bool DIALOG_OUTSET_ITEMS::TransferDataToWindow()
{
    m_LayerSelectionCtrl->SetLayerSelection( m_params.layer );
    m_outset.SetValue( m_params.outsetDistance );
    m_roundCorners->SetValue( m_params.roundCorners );
    m_lineWidth.SetValue( m_params.lineWidth );

    m_roundToGrid->SetValue( m_params.gridRounding.has_value() );

    m_roundingGrid.SetValue( m_params.gridRounding.value_or( s_gridRoundValuePersist ) );

    m_copyLayers->SetValue( m_params.useSourceLayers );
    m_copyWidths->SetValue( m_params.useSourceWidths );

    m_gridRoundingEntry->Enable( m_roundToGrid->IsChecked() );
    m_LayerSelectionCtrl->Enable( !m_copyLayers->GetValue() );

    m_deleteSourceItems->SetValue( m_params.deleteSourceItems );

    return true;
}


bool DIALOG_OUTSET_ITEMS::TransferDataFromWindow()
{
    if( m_outset.GetIntValue() <= 0 )
    {
        DisplayErrorMessage( this, _( "Outset must be a positive value." ) );
        return false;
    }

    if( m_lineWidth.GetIntValue() <= 0 )
    {
        DisplayErrorMessage( this, _( "Line width must be a positive value." ) );
        return false;
    }

    m_params.layer = ToLAYER_ID( m_LayerSelectionCtrl->GetLayerSelection() );
    m_params.outsetDistance = m_outset.GetIntValue();
    m_params.roundCorners = m_roundCorners->GetValue();
    m_params.lineWidth = m_lineWidth.GetIntValue();

    m_params.useSourceLayers = m_copyLayers->GetValue();
    m_params.useSourceWidths = m_copyWidths->GetValue();

    if( m_roundToGrid->IsChecked() )
        m_params.gridRounding = m_roundingGrid.GetValue();
    else
        m_params.gridRounding = std::nullopt;

    s_gridRoundValuePersist = m_roundingGrid.GetIntValue();

    m_params.deleteSourceItems = m_deleteSourceItems->GetValue();

    // Keep the recent values list up to date
    const auto saveRecentValue =
            []( std::vector<int>& aRecentValues, int aValue )
            {
                const auto it = std::find( aRecentValues.begin(), aRecentValues.end(), aValue );

                // Already have it
                if( it != aRecentValues.end() )
                    return;

                aRecentValues.push_back( aValue );
            };

    saveRecentValue( s_outsetRecentValues, m_params.outsetDistance );
    saveRecentValue( s_lineWidthRecentValues, m_params.lineWidth );

    if( m_params.gridRounding )
        saveRecentValue( s_gridRoundingRecentValues, m_params.gridRounding.value() );

    return true;
}