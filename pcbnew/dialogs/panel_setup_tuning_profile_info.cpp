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


#include <dialogs/panel_setup_tuning_profile_info.h>

#include <widgets/std_bitmap_button.h>
#include <bitmaps.h>
#include <confirm.h>
#include <grid_tricks.h>
#include <panel_setup_tuning_profiles.h>
#include <pcb_edit_frame.h>
#include <transline_calculations/units.h>
#include <widgets/grid_text_button_helpers.h>
#include <widgets/paged_dialog.h>
#include <widgets/wx_grid.h>

PANEL_SETUP_TUNING_PROFILE_INFO::PANEL_SETUP_TUNING_PROFILE_INFO( wxWindow*                    aParentWindow,
                                                                  PANEL_SETUP_TUNING_PROFILES* parentPanel ) :
        PANEL_SETUP_TUNING_PROFILE_INFO_BASE( aParentWindow ),
        m_parentPanel( parentPanel ),
        m_viaPropagationUnits( parentPanel->m_frame, m_viaPropagationSpeedLabel, m_viaPropagationSpeed,
                               m_viaPropSpeedUnits )
{
    Freeze();
    initPanel();
    Thaw();
}


void PANEL_SETUP_TUNING_PROFILE_INFO::initPanel()
{
    if( EDA_UNIT_UTILS::IsImperialUnit( m_parentPanel->m_unitsProvider->GetUserUnits() ) )
        m_viaPropagationUnits.SetUnits( EDA_UNITS::PS_PER_INCH );
    else
        m_viaPropagationUnits.SetUnits( EDA_UNITS::PS_PER_CM );

    m_viaPropagationUnits.SetDataType( EDA_DATA_TYPE::LENGTH_DELAY );

    int x = 0, y = 0;
    m_name->GetTextExtent( "XXXXXXXXXXXXXXXXXXXXX", &x, &y );
    m_name->SetMinSize( wxSize( x, -1 ) );
    m_targetImpedance->GetTextExtent( "XXXXXXXXX", &x, &y );
    m_targetImpedance->SetMinSize( wxSize( x, -1 ) );

    m_viaPropagationUnits.SetValue( 0 );

    m_addTrackPropogationLayer->SetBitmap( KiBitmapBundle( BITMAPS::small_plus ) );
    m_deleteTrackPropogationLayer->SetBitmap( KiBitmapBundle( BITMAPS::small_trash ) );

    m_addViaPropagationOverride->SetBitmap( KiBitmapBundle( BITMAPS::small_plus ) );
    m_removeViaPropagationOverride->SetBitmap( KiBitmapBundle( BITMAPS::small_trash ) );

    m_targetImpedance->SetValue( "0" );

    UNITS_PROVIDER* unitsProvider = m_parentPanel->m_unitsProvider.get();

    m_trackPropagationGrid->SetUnitsProvider( unitsProvider );
    m_viaOverrides->SetUnitsProvider( unitsProvider );

    // Configure the track grid
    m_trackPropagationGrid->BeginBatch();
    m_trackPropagationGrid->SetUseNativeColLabels();

    m_trackPropagationGrid->EnsureColLabelsVisible();
    m_trackPropagationGrid->PushEventHandler( new GRID_TRICKS( m_trackPropagationGrid ) );
    m_trackPropagationGrid->SetSelectionMode( wxGrid::wxGridSelectRows );

    std::vector<int> trackColIds;
    m_trackPropagationGrid->SetAutoEvalColUnits( TRACK_GRID_DELAY,
                                                 unitsProvider->GetUnitsFromType( EDA_DATA_TYPE::LENGTH_DELAY ) );
    trackColIds.push_back( TRACK_GRID_DELAY );
    m_trackPropagationGrid->SetAutoEvalColUnits( TRACK_GRID_TRACK_WIDTH,
                                                 unitsProvider->GetUnitsFromType( EDA_DATA_TYPE::DISTANCE ) );
    trackColIds.push_back( TRACK_GRID_TRACK_WIDTH );
    m_trackPropagationGrid->SetAutoEvalColUnits( TRACK_GRID_TRACK_GAP,
                                                 unitsProvider->GetUnitsFromType( EDA_DATA_TYPE::DISTANCE ) );
    trackColIds.push_back( TRACK_GRID_TRACK_GAP );
    m_trackPropagationGrid->SetAutoEvalCols( trackColIds );

    // Add the calculation editors
    wxGridCellAttr* attr = new wxGridCellAttr;
    attr->SetEditor( new GRID_CELL_RUN_FUNCTION_EDITOR(
            m_parentPanel->m_dlg,
            [this]( int row, int col )
            {
                calculateTrackParametersForCell( row, col );
            },
            false ) );
    m_trackPropagationGrid->SetColAttr( TRACK_GRID_TRACK_WIDTH, attr );

    attr = new wxGridCellAttr;
    attr->SetEditor( new GRID_CELL_RUN_FUNCTION_EDITOR(
            m_parentPanel->m_dlg,
            [this]( int row, int col )
            {
                calculateTrackParametersForCell( row, col );
            },
            false ) );
    m_trackPropagationGrid->SetColAttr( TRACK_GRID_TRACK_GAP, attr );

    attr = new wxGridCellAttr;
    attr->SetEditor( new GRID_CELL_RUN_FUNCTION_EDITOR(
            m_parentPanel->m_dlg,
            [this]( int row, int col )
            {
                calculateTrackParametersForCell( row, col );
            },
            false ) );
    m_trackPropagationGrid->SetColAttr( TRACK_GRID_DELAY, attr );

    m_trackPropagationGrid->EndBatch();

    // Configure the via grid
    m_viaOverrides->BeginBatch();
    m_viaOverrides->SetUseNativeColLabels();

    m_viaOverrides->EnsureColLabelsVisible();
    m_viaOverrides->PushEventHandler( new GRID_TRICKS( m_viaOverrides ) );
    m_viaOverrides->SetSelectionMode( wxGrid::wxGridSelectRows );

    std::vector<int> viaColIds;
    m_viaOverrides->SetAutoEvalColUnits( VIA_GRID_DELAY, unitsProvider->GetUnitsFromType( EDA_DATA_TYPE::TIME ) );
    viaColIds.push_back( VIA_GRID_DELAY );
    m_viaOverrides->SetAutoEvalCols( viaColIds );
    m_viaOverrides->EndBatch();

    setColumnWidths();

    // Hide the trace gap as we start in single mode
    m_trackPropagationGrid->HideCol( TRACK_GRID_TRACK_GAP );

    UpdateLayerNames();
    Layout();
}


void PANEL_SETUP_TUNING_PROFILE_INFO::LoadProfile( const TUNING_PROFILE& aProfile )
{
    BOARD* board = m_parentPanel->m_board;

    m_name->SetValue( aProfile.m_ProfileName );
    m_type->SetSelection( static_cast<int>( aProfile.m_Type ) );
    onChangeProfileType( aProfile.m_Type );
    m_targetImpedance->SetValue( wxString::FromDouble( aProfile.m_TargetImpedance ) );
    m_enableDelayTuning->SetValue( aProfile.m_EnableTimeDomainTuning );
    m_viaPropagationUnits.SetValue( aProfile.m_ViaPropagationDelay );

    for( const auto& entry : aProfile.m_TrackPropagationEntries )
    {
        const int row = m_trackPropagationGrid->GetNumberRows();
        m_trackPropagationGrid->AppendRows();

        m_trackPropagationGrid->SetCellValue( row, TRACK_GRID_SIGNAL_LAYER,
                                              board->GetLayerName( entry.GetSignalLayer() ) );

        if( entry.GetTopReferenceLayer() != UNDEFINED_LAYER )
        {
            m_trackPropagationGrid->SetCellValue( row, TRACK_GRID_TOP_REFERENCE,
                                                  board->GetLayerName( entry.GetTopReferenceLayer() ) );
        }

        if( entry.GetBottomReferenceLayer() != UNDEFINED_LAYER )
        {
            m_trackPropagationGrid->SetCellValue( row, TRACK_GRID_BOTTOM_REFERENCE,
                                                  board->GetLayerName( entry.GetBottomReferenceLayer() ) );
        }

        m_trackPropagationGrid->SetUnitValue( row, TRACK_GRID_TRACK_WIDTH, entry.GetWidth() );
        m_trackPropagationGrid->SetUnitValue( row, TRACK_GRID_TRACK_GAP, entry.GetDiffPairGap() );
        m_trackPropagationGrid->SetUnitValue( row, TRACK_GRID_DELAY, entry.GetDelay( true ) );
    }

    for( const auto& entry : aProfile.m_ViaOverrides )
    {
        const int row = m_viaOverrides->GetNumberRows();
        m_viaOverrides->AppendRows();

        m_viaOverrides->SetCellValue( row, VIA_GRID_SIGNAL_LAYER_FROM, board->GetLayerName( entry.m_SignalLayerFrom ) );
        m_viaOverrides->SetCellValue( row, VIA_GRID_SIGNAL_LAYER_TO, board->GetLayerName( entry.m_SignalLayerTo ) );
        m_viaOverrides->SetCellValue( row, VIA_GRID_VIA_LAYER_FROM, board->GetLayerName( entry.m_ViaLayerFrom ) );
        m_viaOverrides->SetCellValue( row, VIA_GRID_VIA_LAYER_TO, board->GetLayerName( entry.m_ViaLayerTo ) );
        m_viaOverrides->SetUnitValue( row, VIA_GRID_DELAY, entry.m_Delay );
    }

    UpdateLayerNames();
}


TUNING_PROFILE PANEL_SETUP_TUNING_PROFILE_INFO::GetProfile() const
{
    TUNING_PROFILE profile;
    profile.m_ProfileName = m_name->GetValue();
    profile.m_Type = static_cast<TUNING_PROFILE::PROFILE_TYPE>( m_type->GetSelection() );
    profile.m_EnableTimeDomainTuning = m_enableDelayTuning->GetValue();
    profile.m_ViaPropagationDelay = m_viaPropagationUnits.GetIntValue();

    double targetImpedance;

    if( m_targetImpedance->GetValue().ToDouble( &targetImpedance ) )
        profile.m_TargetImpedance = targetImpedance;
    else
        profile.m_TargetImpedance = 0.0;

    for( int row = 0; row < m_trackPropagationGrid->GetNumberRows(); row++ )
    {
        DELAY_PROFILE_TRACK_PROPAGATION_ENTRY entry;

        wxString signalLayerName = m_trackPropagationGrid->GetCellValue( row, TRACK_GRID_SIGNAL_LAYER );
        entry.SetSignalLayer( m_parentPanel->m_layerNamesToIDs[signalLayerName] );

        if( wxString topReferenceLayerName = m_trackPropagationGrid->GetCellValue( row, TRACK_GRID_TOP_REFERENCE );
            m_parentPanel->m_layerNamesToIDs.contains( topReferenceLayerName ) )
        {
            entry.SetTopReferenceLayer( m_parentPanel->m_layerNamesToIDs[topReferenceLayerName] );
        }
        else
        {
            entry.SetTopReferenceLayer( UNDEFINED_LAYER );
        }

        if( wxString bottomReferenceLayerName =
                    m_trackPropagationGrid->GetCellValue( row, TRACK_GRID_BOTTOM_REFERENCE );
            m_parentPanel->m_layerNamesToIDs.contains( bottomReferenceLayerName ) )
        {
            entry.SetBottomReferenceLayer( m_parentPanel->m_layerNamesToIDs[bottomReferenceLayerName] );
        }
        else
        {
            entry.SetBottomReferenceLayer( UNDEFINED_LAYER );
        }

        entry.SetWidth( m_trackPropagationGrid->GetUnitValue( row, TRACK_GRID_TRACK_WIDTH ) );
        entry.SetDiffPairGap( m_trackPropagationGrid->GetUnitValue( row, TRACK_GRID_TRACK_GAP ) );
        entry.SetDelay( m_trackPropagationGrid->GetUnitValue( row, TRACK_GRID_DELAY ) );
        entry.SetEnableTimeDomainTuning( profile.m_EnableTimeDomainTuning );

        profile.m_TrackPropagationEntries.push_back( entry );
        profile.m_TrackPropagationEntriesMap[entry.GetSignalLayer()] = entry;
    }

    for( int row = 0; row < m_viaOverrides->GetNumberRows(); row++ )
    {
        const wxString signalLayerFrom = m_viaOverrides->GetCellValue( row, VIA_GRID_SIGNAL_LAYER_FROM );
        const wxString signalLayerTo = m_viaOverrides->GetCellValue( row, VIA_GRID_SIGNAL_LAYER_TO );
        const wxString viaLayerFrom = m_viaOverrides->GetCellValue( row, VIA_GRID_VIA_LAYER_FROM );
        const wxString viaLayerTo = m_viaOverrides->GetCellValue( row, VIA_GRID_VIA_LAYER_TO );
        PCB_LAYER_ID   signalLayerIdFrom = m_parentPanel->m_layerNamesToIDs[signalLayerFrom];
        PCB_LAYER_ID   signalLayerIdTo = m_parentPanel->m_layerNamesToIDs[signalLayerTo];
        PCB_LAYER_ID   viaLayerIdFrom = m_parentPanel->m_layerNamesToIDs[viaLayerFrom];
        PCB_LAYER_ID   viaLayerIdTo = m_parentPanel->m_layerNamesToIDs[viaLayerTo];

        // Order layers in stackup order (from F_Cu first)
        if( IsCopperLayerLowerThan( signalLayerIdFrom, signalLayerIdTo ) )
            std::swap( signalLayerIdFrom, signalLayerIdTo );

        if( IsCopperLayerLowerThan( viaLayerIdFrom, viaLayerIdTo ) )
            std::swap( viaLayerIdFrom, viaLayerIdTo );

        const DELAY_PROFILE_VIA_OVERRIDE_ENTRY entry{ signalLayerIdFrom, signalLayerIdTo, viaLayerIdFrom, viaLayerIdTo,
                                                      m_viaOverrides->GetUnitValue( row, VIA_GRID_DELAY ) };
        profile.m_ViaOverrides.push_back( entry );
    }

    return profile;
}


PANEL_SETUP_TUNING_PROFILE_INFO::~PANEL_SETUP_TUNING_PROFILE_INFO()
{
    m_trackPropagationGrid->PopEventHandler( true );
    m_viaOverrides->PopEventHandler( true );
}


void PANEL_SETUP_TUNING_PROFILE_INFO::UpdateLayerNames()
{
    wxArrayString layerNames, layerNamesWithNone;
    layerNamesWithNone.push_back( "<None>" );
    std::ranges::for_each( m_parentPanel->m_layerNames,
                           [&layerNames, &layerNamesWithNone]( const wxString& aLayerName )
                           {
                               layerNames.push_back( aLayerName );
                               layerNamesWithNone.push_back( aLayerName );
                           } );


    // Save the current data - track grid
    std::vector<wxString> currentSignalLayer;
    std::vector<wxString> currentTopReferenceLayer;
    std::vector<wxString> currentBottomReferenceLayer;

    for( int row = 0; row < m_trackPropagationGrid->GetNumberRows(); ++row )
    {
        currentSignalLayer.emplace_back( m_trackPropagationGrid->GetCellValue( row, TRACK_GRID_SIGNAL_LAYER ) );
        currentTopReferenceLayer.emplace_back( m_trackPropagationGrid->GetCellValue( row, TRACK_GRID_TOP_REFERENCE ) );
        currentBottomReferenceLayer.emplace_back(
                m_trackPropagationGrid->GetCellValue( row, TRACK_GRID_BOTTOM_REFERENCE ) );
    }

    // Save the current data - via grid
    std::vector<wxString> currentSignalLayersFrom;
    std::vector<wxString> currentSignalLayersTo;
    std::vector<wxString> currentViaLayersFrom;
    std::vector<wxString> currentViaLayersTo;

    for( int row = 0; row < m_viaOverrides->GetNumberRows(); ++row )
    {
        currentSignalLayersFrom.emplace_back( m_viaOverrides->GetCellValue( row, VIA_GRID_SIGNAL_LAYER_FROM ) );
        currentSignalLayersTo.emplace_back( m_viaOverrides->GetCellValue( row, VIA_GRID_SIGNAL_LAYER_TO ) );
        currentViaLayersFrom.emplace_back( m_viaOverrides->GetCellValue( row, VIA_GRID_VIA_LAYER_FROM ) );
        currentViaLayersTo.emplace_back( m_viaOverrides->GetCellValue( row, VIA_GRID_VIA_LAYER_TO ) );
    }

    // Reset the via layers lists
    wxGridCellAttr* attr = new wxGridCellAttr;
    attr->SetEditor( new wxGridCellChoiceEditor( layerNames, false ) );
    m_trackPropagationGrid->SetColAttr( TRACK_GRID_SIGNAL_LAYER, attr );

    attr = new wxGridCellAttr;
    attr->SetEditor( new wxGridCellChoiceEditor( layerNamesWithNone, false ) );
    m_trackPropagationGrid->SetColAttr( TRACK_GRID_TOP_REFERENCE, attr );

    attr = new wxGridCellAttr;
    attr->SetEditor( new wxGridCellChoiceEditor( layerNamesWithNone, false ) );
    m_trackPropagationGrid->SetColAttr( TRACK_GRID_BOTTOM_REFERENCE, attr );

    attr = new wxGridCellAttr;
    attr->SetEditor( new wxGridCellChoiceEditor( layerNames, false ) );
    m_viaOverrides->SetColAttr( VIA_GRID_SIGNAL_LAYER_FROM, attr );

    attr = new wxGridCellAttr;
    attr->SetEditor( new wxGridCellChoiceEditor( layerNames, false ) );
    m_viaOverrides->SetColAttr( VIA_GRID_SIGNAL_LAYER_TO, attr );

    attr = new wxGridCellAttr;
    attr->SetEditor( new wxGridCellChoiceEditor( layerNames, false ) );
    m_viaOverrides->SetColAttr( VIA_GRID_VIA_LAYER_FROM, attr );

    attr = new wxGridCellAttr;
    attr->SetEditor( new wxGridCellChoiceEditor( layerNames, false ) );
    m_viaOverrides->SetColAttr( VIA_GRID_VIA_LAYER_TO, attr );

    // Restore the data, changing or resetting layer names if required
    for( int row = 0; row < m_trackPropagationGrid->GetNumberRows(); ++row )
    {
        if( m_parentPanel->m_prevLayerNamesToIDs.contains( currentSignalLayer[row] ) )
        {
            PCB_LAYER_ID lastSignalId = m_parentPanel->m_prevLayerNamesToIDs[currentSignalLayer[row]];

            if( m_parentPanel->m_copperLayerIdsToIndex.contains( lastSignalId ) )
                m_trackPropagationGrid->SetCellValue( row, TRACK_GRID_SIGNAL_LAYER,
                                                      m_parentPanel->m_board->GetLayerName( lastSignalId ) );
            else
                m_trackPropagationGrid->SetCellValue( row, TRACK_GRID_SIGNAL_LAYER,
                                                      m_parentPanel->m_layerNames.front() );
        }
        else
        {
            m_trackPropagationGrid->SetCellValue( row, TRACK_GRID_SIGNAL_LAYER, m_parentPanel->m_layerNames.front() );
        }

        if( m_parentPanel->m_prevLayerNamesToIDs.contains( currentTopReferenceLayer[row] ) )
        {
            const PCB_LAYER_ID lastTopReferenceId = m_parentPanel->m_prevLayerNamesToIDs[currentTopReferenceLayer[row]];

            if( m_parentPanel->m_copperLayerIdsToIndex.contains( lastTopReferenceId ) )
                m_trackPropagationGrid->SetCellValue( row, TRACK_GRID_TOP_REFERENCE,
                                                      m_parentPanel->m_board->GetLayerName( lastTopReferenceId ) );
            else
                m_trackPropagationGrid->SetCellValue( row, TRACK_GRID_TOP_REFERENCE, layerNamesWithNone[0] );
        }
        else
        {
            m_trackPropagationGrid->SetCellValue( row, TRACK_GRID_TOP_REFERENCE, layerNamesWithNone[0] );
        }

        if( m_parentPanel->m_prevLayerNamesToIDs.contains( currentBottomReferenceLayer[row] ) )
        {
            const PCB_LAYER_ID lastBottomReferenceId =
                    m_parentPanel->m_prevLayerNamesToIDs[currentBottomReferenceLayer[row]];

            if( m_parentPanel->m_copperLayerIdsToIndex.contains( lastBottomReferenceId ) )
                m_trackPropagationGrid->SetCellValue( row, TRACK_GRID_BOTTOM_REFERENCE,
                                                      m_parentPanel->m_board->GetLayerName( lastBottomReferenceId ) );
            else
                m_trackPropagationGrid->SetCellValue( row, TRACK_GRID_BOTTOM_REFERENCE, layerNamesWithNone[0] );
        }
        else
        {
            m_trackPropagationGrid->SetCellValue( row, TRACK_GRID_BOTTOM_REFERENCE, layerNamesWithNone[0] );
        }
    }

    for( int row = 0; row < m_viaOverrides->GetNumberRows(); ++row )
    {
        const PCB_LAYER_ID lastSignalFromId = m_parentPanel->m_prevLayerNamesToIDs[currentSignalLayersFrom[row]];

        if( m_parentPanel->m_copperLayerIdsToIndex.contains( lastSignalFromId ) )
            m_viaOverrides->SetCellValue( row, VIA_GRID_SIGNAL_LAYER_FROM,
                                          m_parentPanel->m_board->GetLayerName( lastSignalFromId ) );
        else
            m_viaOverrides->SetCellValue( row, VIA_GRID_SIGNAL_LAYER_FROM, m_parentPanel->m_layerNames.front() );

        const PCB_LAYER_ID lastSignalToId = m_parentPanel->m_prevLayerNamesToIDs[currentSignalLayersTo[row]];

        if( m_parentPanel->m_copperLayerIdsToIndex.contains( lastSignalToId ) )
            m_viaOverrides->SetCellValue( row, VIA_GRID_SIGNAL_LAYER_TO,
                                          m_parentPanel->m_board->GetLayerName( lastSignalToId ) );
        else
            m_viaOverrides->SetCellValue( row, VIA_GRID_SIGNAL_LAYER_TO, m_parentPanel->m_layerNames.back() );

        const PCB_LAYER_ID lastViaFromId = m_parentPanel->m_prevLayerNamesToIDs[currentViaLayersFrom[row]];

        if( m_parentPanel->m_copperLayerIdsToIndex.contains( lastViaFromId ) )
            m_viaOverrides->SetCellValue( row, VIA_GRID_VIA_LAYER_FROM,
                                          m_parentPanel->m_board->GetLayerName( lastViaFromId ) );
        else
            m_viaOverrides->SetCellValue( row, VIA_GRID_VIA_LAYER_FROM, m_parentPanel->m_layerNames.front() );

        const PCB_LAYER_ID lastViaToId = m_parentPanel->m_prevLayerNamesToIDs[currentViaLayersTo[row]];

        if( m_parentPanel->m_copperLayerIdsToIndex.contains( lastViaToId ) )
            m_viaOverrides->SetCellValue( row, VIA_GRID_VIA_LAYER_TO,
                                          m_parentPanel->m_board->GetLayerName( lastViaToId ) );
        else
            m_viaOverrides->SetCellValue( row, VIA_GRID_VIA_LAYER_TO, m_parentPanel->m_layerNames.back() );
    }
}


void PANEL_SETUP_TUNING_PROFILE_INFO::setColumnWidths()
{
    const int minValueWidth = m_trackPropagationGrid->GetTextExtent( wxT( "000.0000 ps/mm" ) ).x;

    for( int i = 0; i < m_trackPropagationGrid->GetNumberCols(); ++i )
    {
        const int titleSize = m_trackPropagationGrid->GetTextExtent( m_trackPropagationGrid->GetColLabelValue( i ) ).x;

        if( i == TRACK_GRID_SIGNAL_LAYER || i == TRACK_GRID_TOP_REFERENCE || i == TRACK_GRID_BOTTOM_REFERENCE )
            m_trackPropagationGrid->SetColSize( i, titleSize + 30 );
        else
            m_trackPropagationGrid->SetColSize( i, std::max( titleSize, minValueWidth ) );
    }

    for( int i = 0; i < m_viaOverrides->GetNumberCols(); ++i )
    {
        const int titleSize = GetTextExtent( m_viaOverrides->GetColLabelValue( i ) ).x;
        if( i == VIA_GRID_DELAY )
            m_viaOverrides->SetColSize( i, std::max( titleSize, minValueWidth ) );
        else
            m_viaOverrides->SetColSize( i, titleSize + 30 );
    }

    const int impedanceWidth = m_targetImpedance->GetTextExtent( wxT( "0000.00" ) ).x;
    m_targetImpedance->SetSize( impedanceWidth, m_targetImpedance->GetSize().GetHeight() );

    Layout();
}


void PANEL_SETUP_TUNING_PROFILE_INFO::OnProfileNameChanged( wxCommandEvent& event )
{
    const wxString newName = event.GetString();
    m_parentPanel->UpdateProfileName( this, newName );
}


void PANEL_SETUP_TUNING_PROFILE_INFO::OnChangeProfileType( wxCommandEvent& event )
{
    if( event.GetSelection() == 0 )
        onChangeProfileType( TUNING_PROFILE::PROFILE_TYPE::SINGLE );
    else
        onChangeProfileType( TUNING_PROFILE::PROFILE_TYPE::DIFFERENTIAL );
}


void PANEL_SETUP_TUNING_PROFILE_INFO::onChangeProfileType( TUNING_PROFILE::PROFILE_TYPE aType ) const
{
    m_trackPropagationGrid->CommitPendingChanges();
    m_viaOverrides->CommitPendingChanges();

    if( aType == TUNING_PROFILE::PROFILE_TYPE::SINGLE )
        m_trackPropagationGrid->HideCol( TRACK_GRID_TRACK_GAP );
    else
        m_trackPropagationGrid->ShowCol( TRACK_GRID_TRACK_GAP );
}


void PANEL_SETUP_TUNING_PROFILE_INFO::OnAddTrackRow( wxCommandEvent& event )
{
    const int numRows = m_trackPropagationGrid->GetNumberRows();
    m_trackPropagationGrid->InsertRows( m_trackPropagationGrid->GetNumberRows() );

    auto setFrontRowLayers = [&]( const int row )
    {
        auto nameItr = m_parentPanel->m_layerNames.begin();

        if( nameItr == m_parentPanel->m_layerNames.end() )
            return;

        m_trackPropagationGrid->SetCellValue( row, TRACK_GRID_SIGNAL_LAYER, *nameItr );

        ++nameItr;

        if( nameItr == m_parentPanel->m_layerNames.end() )
            return;

        m_trackPropagationGrid->SetCellValue( row, TRACK_GRID_BOTTOM_REFERENCE, *nameItr );
    };

    auto setRowLayers = [&]()
    {
        if( numRows == 0 )
        {
            setFrontRowLayers( 0 );
            return;
        }

        const wxString lastSignalLayerName =
                m_trackPropagationGrid->GetCellValue( numRows - 1, TRACK_GRID_SIGNAL_LAYER );
        auto nameItr = std::find( m_parentPanel->m_layerNames.begin(), m_parentPanel->m_layerNames.end(),
                                  lastSignalLayerName );

        if( nameItr == m_parentPanel->m_layerNames.end() )
            return;

        if( nameItr == m_parentPanel->m_layerNames.end() - 1 )
        {
            setFrontRowLayers( numRows );
            return;
        }

        ++nameItr;

        if( nameItr == m_parentPanel->m_layerNames.end() )
            return;

        m_trackPropagationGrid->SetCellValue( numRows, TRACK_GRID_SIGNAL_LAYER, *nameItr );
        m_trackPropagationGrid->SetCellValue( numRows, TRACK_GRID_TOP_REFERENCE, *( nameItr - 1 ) );

        ++nameItr;

        if( nameItr != m_parentPanel->m_layerNames.end() )
            m_trackPropagationGrid->SetCellValue( numRows, TRACK_GRID_BOTTOM_REFERENCE, *nameItr );
    };

    setRowLayers();

    m_trackPropagationGrid->SetUnitValue( numRows, TRACK_GRID_TRACK_WIDTH, 0 );
    m_trackPropagationGrid->SetUnitValue( numRows, TRACK_GRID_TRACK_GAP, 0 );
    m_trackPropagationGrid->SetUnitValue( numRows, TRACK_GRID_DELAY, 0 );
    UpdateLayerNames();
}


void PANEL_SETUP_TUNING_PROFILE_INFO::OnRemoveTrackRow( wxCommandEvent& event )
{
    wxArrayInt selRows = m_trackPropagationGrid->GetSelectedRows();

    if( selRows.size() == 1 )
        m_trackPropagationGrid->DeleteRows( selRows[0] );
}


void PANEL_SETUP_TUNING_PROFILE_INFO::OnAddViaOverride( wxCommandEvent& event )
{
    const int numRows = m_viaOverrides->GetNumberRows();
    m_viaOverrides->InsertRows( numRows );
    m_viaOverrides->SetUnitValue( numRows, VIA_GRID_DELAY, 0 );
    m_viaOverrides->SetCellValue( numRows, VIA_GRID_SIGNAL_LAYER_FROM, m_parentPanel->m_layerNames.front() );
    m_viaOverrides->SetCellValue( numRows, VIA_GRID_SIGNAL_LAYER_TO, m_parentPanel->m_layerNames.back() );
    m_viaOverrides->SetCellValue( numRows, VIA_GRID_VIA_LAYER_FROM, m_parentPanel->m_layerNames.front() );
    m_viaOverrides->SetCellValue( numRows, VIA_GRID_VIA_LAYER_TO, m_parentPanel->m_layerNames.back() );
    UpdateLayerNames();
}


void PANEL_SETUP_TUNING_PROFILE_INFO::OnRemoveViaOverride( wxCommandEvent& event )
{
    wxArrayInt selRows = m_viaOverrides->GetSelectedRows();

    if( selRows.size() == 1 )
        m_viaOverrides->DeleteRows( selRows[0] );
}


wxString PANEL_SETUP_TUNING_PROFILE_INFO::GetProfileName() const
{
    return m_name->GetValue();
}


double PANEL_SETUP_TUNING_PROFILE_INFO::calculateSkinDepth( const double aFreq, const double aMurc,
                                                            const double aSigma )
{
    return 1.0 / sqrt( M_PI * aFreq * aMurc * TRANSLINE_CALCULATIONS::MU0 * aSigma );
}


int PANEL_SETUP_TUNING_PROFILE_INFO::getStackupLayerId( const std::vector<BOARD_STACKUP_ITEM*>& aLayerList,
                                                        PCB_LAYER_ID                            aPcbLayerId )
{
    bool layerFound = false;
    int  layerStackupId = 0;

    while( layerStackupId < static_cast<int>( aLayerList.size() ) && !layerFound )
    {
        if( aLayerList.at( layerStackupId )->GetBrdLayerId() != aPcbLayerId )
            ++layerStackupId;
        else
            layerFound = true;
    }

    if( !layerFound )
        return -1;

    return layerStackupId;
}


double PANEL_SETUP_TUNING_PROFILE_INFO::getTargetImpedance() const
{
    const wxString zStr = m_targetImpedance->GetValue();

    double z;
    if( !zStr.ToDouble( &z ) )
        z = -1;

    return z;
}


PANEL_SETUP_TUNING_PROFILE_INFO::DIELECTRIC_INFO PANEL_SETUP_TUNING_PROFILE_INFO::calculateAverageDielectricConstants(
        const std::vector<BOARD_STACKUP_ITEM*>& aStackupLayerList, const std::vector<int>& dielectricLayerStackupIds,
        const EDA_IU_SCALE& aIuScale )
{
    double totalHeight = 0.0;
    double e_r = 0.0;
    double lossTangent = 0.0;

    for( int i : dielectricLayerStackupIds )
    {
        const BOARD_STACKUP_ITEM* layer = aStackupLayerList.at( i );

        for( int subLayerIdx = 0; subLayerIdx < layer->GetSublayersCount(); ++subLayerIdx )
        {
            totalHeight += aIuScale.IUTomm( layer->GetThickness( subLayerIdx ) );
            e_r += layer->GetEpsilonR( subLayerIdx ) * aIuScale.IUTomm( layer->GetThickness( subLayerIdx ) );
            lossTangent += layer->GetLossTangent( subLayerIdx ) * aIuScale.IUTomm( layer->GetThickness( subLayerIdx ) );
        }
    }

    e_r = e_r / totalHeight;
    lossTangent = lossTangent / totalHeight;
    totalHeight /= 1000.0; // Convert from mm to m

    return { totalHeight, e_r, lossTangent };
}


void PANEL_SETUP_TUNING_PROFILE_INFO::getDielectricLayers( const std::vector<BOARD_STACKUP_ITEM*>& aStackupLayerList,
                                                           const int aSignalLayerId, const int aReferenceLayerId,
                                                           std::vector<int>& aDielectricLayerStackupIds )
{
    for( int i = std::min( aSignalLayerId, aReferenceLayerId ) + 1; i < std::max( aSignalLayerId, aReferenceLayerId );
         ++i )
    {
        const BOARD_STACKUP_ITEM* layer = aStackupLayerList.at( i );

        if( layer->GetType() != BS_ITEM_TYPE_DIELECTRIC )
            continue;

        if( !layer->HasEpsilonRValue() )
            continue;

        aDielectricLayerStackupIds.push_back( i );
    }
}


void PANEL_SETUP_TUNING_PROFILE_INFO::calculateTrackParametersForCell( const int aRow, const int aCol )
{
    // Determine if this is a stripline or microstrip geometry
    const wxString signalLayerName = m_trackPropagationGrid->GetCellValue( aRow, TRACK_GRID_SIGNAL_LAYER );

    if( !m_parentPanel->m_layerNamesToIDs.contains( signalLayerName ) )
        return;

    const PCB_LAYER_ID                 signalLayer = m_parentPanel->m_layerNamesToIDs.at( signalLayerName );
    const TUNING_PROFILE::PROFILE_TYPE profileType = m_type->GetSelection() == 0
                                                             ? TUNING_PROFILE::PROFILE_TYPE::SINGLE
                                                             : TUNING_PROFILE::PROFILE_TYPE::DIFFERENTIAL;
    const bool                         isMicrostrip = IsFrontLayer( signalLayer ) || IsBackLayer( signalLayer );
    CalculationType                    calculationType;

    switch( aCol )
    {
    case TRACK_GRID_TRACK_WIDTH: calculationType = CalculationType::WIDTH; break;
    case TRACK_GRID_TRACK_GAP: calculationType = CalculationType::GAP; break;
    case TRACK_GRID_DELAY: calculationType = CalculationType::DELAY; break;
    default: calculationType = CalculationType::WIDTH; break;
    }

    if( profileType == TUNING_PROFILE::PROFILE_TYPE::DIFFERENTIAL ) // Differential tracks mode
    {
        int calculatedWidth = 0;
        int calculatedGap = 0;
        int calculatedDelay = 0;

        CALCULATION_RESULT result;

        if( isMicrostrip )
            result = calculateDifferentialMicrostrip( aRow, calculationType );
        else
            result = calculateDifferentialStripline( aRow, calculationType );

        if( !result.OK )
        {
            DisplayErrorMessage( m_parentPanel->m_dlg, wxString::Format( _( "Error: %s" ), result.ErrorMsg ) );
            return;
        }

        calculatedWidth = result.Width;
        calculatedGap = result.DiffPairGap;
        calculatedDelay = result.Delay;

        const bool widthOk = calculatedWidth > 0;
        const bool gapOk = calculatedGap > 0;
        const bool delayOk = calculatedDelay > 0;

        if( !widthOk )
        {
            DisplayErrorMessage( m_parentPanel->m_dlg, _( "Could not compute track width" ) );
            return;
        }
        else if( !gapOk )
        {
            DisplayErrorMessage( m_parentPanel->m_dlg, _( "Could not compute differential pair gap" ) );
            return;
        }
        else if( !delayOk )
        {
            DisplayErrorMessage( m_parentPanel->m_dlg, _( "Could not compute track propagation delay" ) );
            return;
        }

        if( calculationType == CalculationType::WIDTH )
        {
            m_trackPropagationGrid->SetUnitValue( aRow, TRACK_GRID_TRACK_WIDTH, calculatedWidth );
        }
        else if( calculationType == CalculationType::GAP )
        {
            m_trackPropagationGrid->SetUnitValue( aRow, TRACK_GRID_TRACK_GAP, calculatedGap );
        }

        m_trackPropagationGrid->SetUnitValue( aRow, TRACK_GRID_DELAY, calculatedDelay );
    }
    else // Single track mode
    {
        int calculatedWidth = 0;
        int calculatedDelay = 0;

        CALCULATION_RESULT result;

        if( isMicrostrip )
            result = calculateSingleMicrostrip( aRow, calculationType );
        else
            result = calculateSingleStripline( aRow, calculationType );

        if( !result.OK )
        {
            DisplayErrorMessage( m_parentPanel->m_dlg, wxString::Format( _( "Error: %s" ), result.ErrorMsg ) );
            return;
        }

        calculatedWidth = result.Width;
        calculatedDelay = result.Delay;

        const bool widthOk = calculatedWidth > 0;
        const bool delayOk = calculatedDelay > 0;

        if( !widthOk )
        {
            DisplayErrorMessage( m_parentPanel->m_dlg, _( "Could not compute track width" ) );
            return;
        }
        else if( !delayOk )
        {
            DisplayErrorMessage( m_parentPanel->m_dlg, _( "Could not compute track propagation delay" ) );
            return;
        }

        if( calculationType == CalculationType::WIDTH )
        {
            m_trackPropagationGrid->SetUnitValue( aRow, TRACK_GRID_TRACK_WIDTH, calculatedWidth );
        }

        m_trackPropagationGrid->SetUnitValue( aRow, TRACK_GRID_DELAY, calculatedDelay );
    }
}


bool PANEL_SETUP_TUNING_PROFILE_INFO::ValidateProfile( const size_t aPageIndex )
{
    if( m_name->GetValue() == wxEmptyString )
    {
        m_parentPanel->m_tuningProfiles->SetSelection( aPageIndex );

        const wxString msg = _( "Tuning profile must have a name" );
        PAGED_DIALOG::GetDialog( m_parentPanel )->SetError( msg, this, m_name );
        return false;
    }

    std::set<wxString> layerNames;

    for( int i = 0; i < m_trackPropagationGrid->GetNumberRows(); ++i )
    {
        const wxString& layerName = m_trackPropagationGrid->GetCellValue( i, TRACK_GRID_SIGNAL_LAYER );

        if( layerNames.contains( layerName ) )
        {
            m_parentPanel->m_tuningProfiles->SetSelection( aPageIndex );

            const wxString msg = _( "Duplicated signal layer configuration in tuning profile" );
            PAGED_DIALOG::GetDialog( m_parentPanel )->SetError( msg, m_parentPanel, m_trackPropagationGrid, i,
                                                                TRACK_GRID_SIGNAL_LAYER );
            return false;
        }

        layerNames.insert( layerName );
    }

    return true;
}


/*****************************************************************************************************************
 *                                        SIMULATION / ANALYSIS PLUMBING
 ****************************************************************************************************************/

std::pair<PANEL_SETUP_TUNING_PROFILE_INFO::CALCULATION_BOARD_PARAMETERS,
          PANEL_SETUP_TUNING_PROFILE_INFO::CALCULATION_RESULT>
PANEL_SETUP_TUNING_PROFILE_INFO::getMicrostripBoardParameters( const int aRow, const EDA_IU_SCALE& aScale )
{
    // Get the signal layer information from the stackup
    BOARD_STACKUP                           stackup = m_parentPanel->m_board->GetStackupOrDefault();
    const std::vector<BOARD_STACKUP_ITEM*>& stackupLayerList = stackup.GetList();

    const wxString signalLayerName = m_trackPropagationGrid->GetCellValue( aRow, TRACK_GRID_SIGNAL_LAYER );

    if( !m_parentPanel->m_layerNamesToIDs.contains( signalLayerName ) )
        return { {}, CALCULATION_RESULT{ _( "Signal layer not found in stackup" ) } };

    const PCB_LAYER_ID signalLayer = m_parentPanel->m_layerNamesToIDs[signalLayerName];

    // Microstrip can only be on an outer copper layer
    if( signalLayer != F_Cu && signalLayer != B_Cu )
        return { {}, CALCULATION_RESULT{ _( "Internal error: Microstrip can only be on an outer copper layer" ) } };

    const int signalLayerStackupId = getStackupLayerId( stackupLayerList, signalLayer );

    if( signalLayerStackupId == -1 )
        return { {}, CALCULATION_RESULT{ _( "Signal layer not found in stackup" ) } };

    const double signalLayerThickness =
            aScale.IUTomm( stackupLayerList.at( signalLayerStackupId )->GetThickness() ) / 1000.0;

    if( signalLayerThickness <= 0 )
        return { {}, CALCULATION_RESULT{ _( "Signal layer thickness must be greater than 0" ) } };

    // Get reference layer
    wxString referenceLayerName;

    if( signalLayer == F_Cu )
        referenceLayerName = m_trackPropagationGrid->GetCellValue( aRow, TRACK_GRID_BOTTOM_REFERENCE );
    else
        referenceLayerName = m_trackPropagationGrid->GetCellValue( aRow, TRACK_GRID_TOP_REFERENCE );

    if( !m_parentPanel->m_layerNamesToIDs.contains( referenceLayerName ) )
        return { {}, CALCULATION_RESULT{ _( "Reference layer not found in stackup" ) } };

    const PCB_LAYER_ID referenceLayer = m_parentPanel->m_layerNamesToIDs[referenceLayerName];
    const int          referenceLayerStackupId = getStackupLayerId( stackupLayerList, referenceLayer );

    if( signalLayerStackupId == referenceLayerStackupId )
        return { {}, CALCULATION_RESULT{ _( "Reference layer must be different to signal layer" ) } };

    // Get the dielectric layers between signal and reference layers
    std::vector<int> dielectricLayerStackupIds;
    getDielectricLayers( stackupLayerList, signalLayerStackupId, referenceLayerStackupId, dielectricLayerStackupIds );

    // Calculate geometric average of the dielectric materials
    const DIELECTRIC_INFO dielectricInfo =
            calculateAverageDielectricConstants( stackupLayerList, dielectricLayerStackupIds, aScale );

    if( dielectricInfo.Height <= 0.0 )
        return { {}, CALCULATION_RESULT{ _( "Dielectric height must be greater than 0" ) } };

    CALCULATION_BOARD_PARAMETERS boardParameters{ dielectricInfo.E_r, dielectricInfo.Height, 0.0, signalLayerThickness,
                                                  dielectricInfo.Loss_Tangent };
    CALCULATION_RESULT           result;
    result.OK = true;

    return { boardParameters, result };
}


std::pair<PANEL_SETUP_TUNING_PROFILE_INFO::CALCULATION_BOARD_PARAMETERS,
          PANEL_SETUP_TUNING_PROFILE_INFO::CALCULATION_RESULT>
PANEL_SETUP_TUNING_PROFILE_INFO::getStriplineBoardParameters( const int aRow, const EDA_IU_SCALE& aScale )
{
    // Get the signal layer information from the stackup
    BOARD_STACKUP                           stackup = m_parentPanel->m_board->GetStackupOrDefault();
    const std::vector<BOARD_STACKUP_ITEM*>& stackupLayerList = stackup.GetList();

    const wxString signalLayerName = m_trackPropagationGrid->GetCellValue( aRow, TRACK_GRID_SIGNAL_LAYER );

    if( !m_parentPanel->m_layerNamesToIDs.contains( signalLayerName ) )
        return { {}, CALCULATION_RESULT{ _( "Signal layer not found in stackup" ) } };

    const PCB_LAYER_ID signalLayer = m_parentPanel->m_layerNamesToIDs[signalLayerName];
    const int          signalLayerStackupId = getStackupLayerId( stackupLayerList, signalLayer );

    if( signalLayerStackupId == -1 )
        return { {}, CALCULATION_RESULT{ _( "Signal layer not found in stackup" ) } };

    const double signalLayerThickness =
            aScale.IUTomm( stackupLayerList.at( signalLayerStackupId )->GetThickness() ) / 1000.0;

    if( signalLayerThickness <= 0 )
        return { {}, CALCULATION_RESULT{ _( "Signal layer thickness must be greater than 0" ) } };

    // Get top reference layer
    const wxString topReferenceLayerName = m_trackPropagationGrid->GetCellValue( aRow, TRACK_GRID_TOP_REFERENCE );

    if( !m_parentPanel->m_layerNamesToIDs.contains( topReferenceLayerName ) )
        return { {}, CALCULATION_RESULT{ _( "Top reference layer not found in stackup" ) } };

    const PCB_LAYER_ID topReferenceLayer = m_parentPanel->m_layerNamesToIDs[topReferenceLayerName];
    const int          topReferenceLayerStackupId = getStackupLayerId( stackupLayerList, topReferenceLayer );

    if( !IsCopperLayerLowerThan( signalLayer, topReferenceLayer ) )
        return { {}, CALCULATION_RESULT{ _( "Top reference layer must be above signal layer in board stackup" ) } };

    // Get bottom reference layer
    wxString bottomReferenceLayerName = m_trackPropagationGrid->GetCellValue( aRow, TRACK_GRID_BOTTOM_REFERENCE );

    if( !m_parentPanel->m_layerNamesToIDs.contains( bottomReferenceLayerName ) )
        return { {}, CALCULATION_RESULT{ _( "Bottom reference layer not found in stackup" ) } };

    const PCB_LAYER_ID bottomReferenceLayer = m_parentPanel->m_layerNamesToIDs[bottomReferenceLayerName];
    const int          bottomReferenceLayerStackupId = getStackupLayerId( stackupLayerList, bottomReferenceLayer );

    if( !IsCopperLayerLowerThan( bottomReferenceLayer, signalLayer ) )
        return { {}, CALCULATION_RESULT{ _( "Bottom reference layer must be below signal layer in board stackup" ) } };

    // Get the dielectric layers between signal and reference layers
    std::vector<int> topDielectricLayerStackupIds, bottomDielectricLayerStackupIds;

    getDielectricLayers( stackupLayerList, signalLayerStackupId, topReferenceLayerStackupId,
                         topDielectricLayerStackupIds );
    getDielectricLayers( stackupLayerList, signalLayerStackupId, bottomReferenceLayerStackupId,
                         bottomDielectricLayerStackupIds );

    // Calculate geometric average of the dielectric materials
    std::vector<int> allDielectricLayerStackupIds( topDielectricLayerStackupIds );
    allDielectricLayerStackupIds.insert( allDielectricLayerStackupIds.end(), bottomDielectricLayerStackupIds.begin(),
                                         bottomDielectricLayerStackupIds.end() );

    const DIELECTRIC_INFO topDielectricInfo =
            calculateAverageDielectricConstants( stackupLayerList, topDielectricLayerStackupIds, aScale );
    const DIELECTRIC_INFO bottomDielectricInfo =
            calculateAverageDielectricConstants( stackupLayerList, bottomDielectricLayerStackupIds, aScale );
    const DIELECTRIC_INFO allDielectricInfo =
            calculateAverageDielectricConstants( stackupLayerList, allDielectricLayerStackupIds, aScale );

    if( topDielectricInfo.Height <= 0.0 && bottomDielectricInfo.Height <= 0.0 )
        return { {}, CALCULATION_RESULT{ _( "Dielectric heights must be greater than 0" ) } };

    CALCULATION_BOARD_PARAMETERS boardParameters{ allDielectricInfo.E_r, topDielectricInfo.Height,
                                                  bottomDielectricInfo.Height, signalLayerThickness,
                                                  allDielectricInfo.Loss_Tangent };
    CALCULATION_RESULT           result;
    result.OK = true;

    return { boardParameters, result };
}


PANEL_SETUP_TUNING_PROFILE_INFO::CALCULATION_RESULT
PANEL_SETUP_TUNING_PROFILE_INFO::calculateSingleMicrostrip( const int aRow, CalculationType aCalculationType )
{
    const EDA_IU_SCALE& iuScale = m_parentPanel->m_unitsProvider->GetIuScale();

    // Get the target impedance
    const double targetZ = getTargetImpedance();

    if( targetZ <= 0 )
        return CALCULATION_RESULT{ _( "Target impedance must be greater than 0" ) };

    // Get board parameters
    auto [boardParameters, result] = getMicrostripBoardParameters( aRow, iuScale );

    if( !result.OK )
        return result;

    // Set calculation parameters
    if( aCalculationType == CalculationType::WIDTH )
    {
        m_microstripCalc.SetParameter( TRANSLINE_PARAMETERS::PHYS_WIDTH, 0.001 );
    }
    else if( aCalculationType == CalculationType::DELAY )
    {
        const int widthInt = m_trackPropagationGrid->GetUnitValue( aRow, TRACK_GRID_TRACK_WIDTH );

        const double width = iuScale.IUTomm( widthInt ) / 1000.0;
        m_microstripCalc.SetParameter( TRANSLINE_PARAMETERS::PHYS_WIDTH, width );
    }

    // Run the synthesis or analysis
    m_microstripCalc.SetParameter( TRANSLINE_PARAMETERS::SIGMA, 1.0 / RHO );
    m_microstripCalc.SetParameter( TRANSLINE_PARAMETERS::EPSILON_EFF, 1.0 );
    m_microstripCalc.SetParameter( TRANSLINE_PARAMETERS::SKIN_DEPTH, calculateSkinDepth( 1.0, 1.0, 1.0 / RHO ) );
    m_microstripCalc.SetParameter( TRANSLINE_PARAMETERS::EPSILONR, boardParameters.DielectricConstant );
    m_microstripCalc.SetParameter( TRANSLINE_PARAMETERS::H_T, 1e+20 );
    m_microstripCalc.SetParameter( TRANSLINE_PARAMETERS::H, boardParameters.TopDielectricLayerThickness );
    m_microstripCalc.SetParameter( TRANSLINE_PARAMETERS::T, boardParameters.SignalLayerThickness );
    m_microstripCalc.SetParameter( TRANSLINE_PARAMETERS::Z0, targetZ );
    m_microstripCalc.SetParameter( TRANSLINE_PARAMETERS::FREQUENCY, 1000000000.0 );
    m_microstripCalc.SetParameter( TRANSLINE_PARAMETERS::ROUGH, 0 );
    m_microstripCalc.SetParameter( TRANSLINE_PARAMETERS::TAND, boardParameters.LossTangent );
    m_microstripCalc.SetParameter( TRANSLINE_PARAMETERS::PHYS_LEN, 10 );
    m_microstripCalc.SetParameter( TRANSLINE_PARAMETERS::MUR, 1 );
    m_microstripCalc.SetParameter( TRANSLINE_PARAMETERS::MURC, 1 );
    m_microstripCalc.SetParameter( TRANSLINE_PARAMETERS::ANG_L, 1 );

    if( aCalculationType == CalculationType::WIDTH )
        m_microstripCalc.Synthesize( SYNTHESIZE_OPTS::DEFAULT );
    else
        m_microstripCalc.Analyse();

    std::unordered_map<TRANSLINE_PARAMETERS, std::pair<double, TRANSLINE_STATUS>>& results =
            [this, aCalculationType]() -> decltype( m_microstripCalc.GetSynthesisResults() )
    {
        if( aCalculationType == CalculationType::WIDTH )
            return m_microstripCalc.GetSynthesisResults();

        return m_microstripCalc.GetAnalysisResults();
    }();

    if( results[TRANSLINE_PARAMETERS::PHYS_WIDTH].second != TRANSLINE_STATUS::OK )
        return CALCULATION_RESULT{ _( "Width calculation failed" ) };

    if( results[TRANSLINE_PARAMETERS::UNIT_PROP_DELAY].second != TRANSLINE_STATUS::OK )
        return CALCULATION_RESULT{ _( "Delay calculation failed" ) };

    int width = static_cast<int>( EDA_UNIT_UTILS::UI::FromUserUnit(
            iuScale, EDA_UNITS::MM, results[TRANSLINE_PARAMETERS::PHYS_WIDTH].first * 1000.0 ) );
    int propDelay = static_cast<int>( EDA_UNIT_UTILS::UI::FromUserUnit(
            iuScale, EDA_UNITS::PS_PER_CM, results[TRANSLINE_PARAMETERS::UNIT_PROP_DELAY].first ) );

    return CALCULATION_RESULT{ width, propDelay };
}


PANEL_SETUP_TUNING_PROFILE_INFO::CALCULATION_RESULT
PANEL_SETUP_TUNING_PROFILE_INFO::calculateSingleStripline( const int aRow, CalculationType aCalculationType )
{
    const EDA_IU_SCALE& iuScale = m_parentPanel->m_unitsProvider->GetIuScale();

    // Get the target impedance
    const double targetZ = getTargetImpedance();

    if( targetZ <= 0 )
        return CALCULATION_RESULT{ _( "Target impedance must be greater than 0" ) };

    // Get board parameters
    auto [boardParameters, result] = getStriplineBoardParameters( aRow, iuScale );

    if( !result.OK )
        return result;

    // Set calculation parameters
    if( aCalculationType == CalculationType::WIDTH )
    {
        m_striplineCalc.SetParameter( TRANSLINE_PARAMETERS::PHYS_WIDTH, 0.001 );
    }
    else if( aCalculationType == CalculationType::DELAY )
    {
        const int widthInt = m_trackPropagationGrid->GetUnitValue( aRow, TRACK_GRID_TRACK_WIDTH );

        const double width = iuScale.IUTomm( widthInt ) / 1000.0;
        m_striplineCalc.SetParameter( TRANSLINE_PARAMETERS::PHYS_WIDTH, width );
    }

    // Run the synthesis
    m_striplineCalc.SetParameter( TRANSLINE_PARAMETERS::SKIN_DEPTH, calculateSkinDepth( 1.0, 1.0, 1.0 / RHO ) );
    m_striplineCalc.SetParameter( TRANSLINE_PARAMETERS::EPSILONR, boardParameters.DielectricConstant );
    m_striplineCalc.SetParameter( TRANSLINE_PARAMETERS::T, boardParameters.SignalLayerThickness );
    m_striplineCalc.SetParameter( TRANSLINE_PARAMETERS::STRIPLINE_A, boardParameters.TopDielectricLayerThickness );
    m_striplineCalc.SetParameter( TRANSLINE_PARAMETERS::H, boardParameters.TopDielectricLayerThickness
                                                                   + boardParameters.SignalLayerThickness
                                                                   + boardParameters.BottomDielectricLayerThickness );
    m_striplineCalc.SetParameter( TRANSLINE_PARAMETERS::Z0, targetZ );
    m_striplineCalc.SetParameter( TRANSLINE_PARAMETERS::PHYS_LEN, 1.0 );
    m_striplineCalc.SetParameter( TRANSLINE_PARAMETERS::FREQUENCY, 1000000000.0 );
    m_striplineCalc.SetParameter( TRANSLINE_PARAMETERS::TAND, boardParameters.LossTangent );
    m_striplineCalc.SetParameter( TRANSLINE_PARAMETERS::ANG_L, 1.0 );
    m_striplineCalc.SetParameter( TRANSLINE_PARAMETERS::SIGMA, 1.0 / RHO );
    m_striplineCalc.SetParameter( TRANSLINE_PARAMETERS::MURC, 1 );

    if( aCalculationType == CalculationType::WIDTH )
        m_striplineCalc.Synthesize( SYNTHESIZE_OPTS::DEFAULT );
    else
        m_striplineCalc.Analyse();

    std::unordered_map<TRANSLINE_PARAMETERS, std::pair<double, TRANSLINE_STATUS>>& results =
            [this, aCalculationType]() -> decltype( m_striplineCalc.GetSynthesisResults() )
    {
        if( aCalculationType == CalculationType::WIDTH )
            return m_striplineCalc.GetSynthesisResults();

        return m_striplineCalc.GetAnalysisResults();
    }();

    if( results[TRANSLINE_PARAMETERS::PHYS_WIDTH].second != TRANSLINE_STATUS::OK )
        return CALCULATION_RESULT{ _( "Width calculation failed" ) };

    if( results[TRANSLINE_PARAMETERS::UNIT_PROP_DELAY].second != TRANSLINE_STATUS::OK )
        return CALCULATION_RESULT{ _( "Delay calculation failed" ) };

    int width = static_cast<int>( EDA_UNIT_UTILS::UI::FromUserUnit(
            iuScale, EDA_UNITS::MM, results[TRANSLINE_PARAMETERS::PHYS_WIDTH].first * 1000.0 ) );
    int propDelay = static_cast<int>( EDA_UNIT_UTILS::UI::FromUserUnit(
            iuScale, EDA_UNITS::PS_PER_CM, results[TRANSLINE_PARAMETERS::UNIT_PROP_DELAY].first ) );

    return CALCULATION_RESULT{ width, propDelay };
}


PANEL_SETUP_TUNING_PROFILE_INFO::CALCULATION_RESULT
PANEL_SETUP_TUNING_PROFILE_INFO::calculateDifferentialMicrostrip( const int aRow, CalculationType aCalculationType )
{
    const EDA_IU_SCALE& iuScale = m_parentPanel->m_unitsProvider->GetIuScale();

    // Get the target impedance
    const double targetZ = getTargetImpedance();

    if( targetZ <= 0 )
        return CALCULATION_RESULT{ _( "Target impedance must be greater than 0" ) };

    // Get board parameters
    auto [boardParameters, result] = getMicrostripBoardParameters( aRow, iuScale );

    if( !result.OK )
        return result;

    // Set calculation parameters
    double width = 0.0;
    double gap = 0.0;

    const std::optional<int> widthOpt = m_trackPropagationGrid->GetOptionalUnitValue( aRow, TRACK_GRID_TRACK_WIDTH );
    const std::optional<int> gapOpt = m_trackPropagationGrid->GetOptionalUnitValue( aRow, TRACK_GRID_TRACK_GAP );

    if( aCalculationType == CalculationType::WIDTH )
    {
        if( !gapOpt || *gapOpt <= 0 )
            return CALCULATION_RESULT{ _( "Diff pair gap must be greater than 0 to calculate width" ) };

        gap = iuScale.IUTomm( gapOpt.value() ) / 1000.0;
    }
    else if( aCalculationType == CalculationType::GAP )
    {
        if( !widthOpt || *widthOpt <= 0 )
            return CALCULATION_RESULT{ _( "Width must be greater than 0 to calculate diff pair gap" ) };

        width = iuScale.IUTomm( widthOpt.value() ) / 1000.0;
    }
    else if( aCalculationType == CalculationType::DELAY )
    {
        if( !widthOpt || !gapOpt || *widthOpt <= 0 || *gapOpt <= 0 )
            return CALCULATION_RESULT{ _( "Width and diff pair gap must be greater than 0 to calculate delay" ) };

        width = iuScale.IUTomm( widthOpt.value() ) / 1000.0;
        gap = iuScale.IUTomm( gapOpt.value() ) / 1000.0;
    }

    // Run the synthesis
    m_coupledMicrostripCalc.SetParameter( TRANSLINE_PARAMETERS::Z0_E, targetZ / 2.0 );
    m_coupledMicrostripCalc.SetParameter( TRANSLINE_PARAMETERS::Z0_O, targetZ / 2.0 );
    m_coupledMicrostripCalc.SetParameter( TRANSLINE_PARAMETERS::Z_DIFF, targetZ );
    m_coupledMicrostripCalc.SetParameter( TRANSLINE_PARAMETERS::PHYS_WIDTH, width );
    m_coupledMicrostripCalc.SetParameter( TRANSLINE_PARAMETERS::PHYS_S, gap );
    m_coupledMicrostripCalc.SetParameter( TRANSLINE_PARAMETERS::EPSILONR, boardParameters.DielectricConstant );
    m_coupledMicrostripCalc.SetParameter( TRANSLINE_PARAMETERS::PHYS_LEN, 10 );
    m_coupledMicrostripCalc.SetParameter( TRANSLINE_PARAMETERS::H, boardParameters.TopDielectricLayerThickness );
    m_coupledMicrostripCalc.SetParameter( TRANSLINE_PARAMETERS::T, boardParameters.SignalLayerThickness );
    m_coupledMicrostripCalc.SetParameter( TRANSLINE_PARAMETERS::H_T, 1e+20 );
    m_coupledMicrostripCalc.SetParameter( TRANSLINE_PARAMETERS::FREQUENCY, 1000000000.0 );
    m_coupledMicrostripCalc.SetParameter( TRANSLINE_PARAMETERS::MURC, 1 );
    m_coupledMicrostripCalc.SetParameter( TRANSLINE_PARAMETERS::SKIN_DEPTH, calculateSkinDepth( 1.0, 1.0, 1.0 / RHO ) );
    m_coupledMicrostripCalc.SetParameter( TRANSLINE_PARAMETERS::SIGMA, 1.0 / RHO );
    m_coupledMicrostripCalc.SetParameter( TRANSLINE_PARAMETERS::ROUGH, 0 );
    m_coupledMicrostripCalc.SetParameter( TRANSLINE_PARAMETERS::TAND, boardParameters.LossTangent );
    m_coupledMicrostripCalc.SetParameter( TRANSLINE_PARAMETERS::ANG_L, 1 );

    switch( aCalculationType )
    {
    case CalculationType::WIDTH: m_coupledMicrostripCalc.Synthesize( SYNTHESIZE_OPTS::FIX_SPACING ); break;
    case CalculationType::GAP: m_coupledMicrostripCalc.Synthesize( SYNTHESIZE_OPTS::FIX_WIDTH ); break;
    case CalculationType::DELAY: m_coupledMicrostripCalc.Analyse(); break;
    }

    std::unordered_map<TRANSLINE_PARAMETERS, std::pair<double, TRANSLINE_STATUS>>& results =
            [this, aCalculationType]() -> decltype( m_microstripCalc.GetSynthesisResults() )
    {
        if( aCalculationType == CalculationType::WIDTH || aCalculationType == CalculationType::GAP )
            return m_coupledMicrostripCalc.GetSynthesisResults();

        return m_coupledMicrostripCalc.GetAnalysisResults();
    }();

    if( results[TRANSLINE_PARAMETERS::PHYS_WIDTH].second != TRANSLINE_STATUS::OK )
        return CALCULATION_RESULT{ _( "Width calculation failed" ) };

    if( results[TRANSLINE_PARAMETERS::PHYS_S].second != TRANSLINE_STATUS::OK )
        return CALCULATION_RESULT{ _( "Diff pair gap calculation failed" ) };

    if( results[TRANSLINE_PARAMETERS::UNIT_PROP_DELAY_ODD].second != TRANSLINE_STATUS::OK )
        return CALCULATION_RESULT{ _( "Delay calculation failed" ) };

    int calcWidth = static_cast<int>( EDA_UNIT_UTILS::UI::FromUserUnit(
            iuScale, EDA_UNITS::MM, results[TRANSLINE_PARAMETERS::PHYS_WIDTH].first * 1000.0 ) );
    int calcGap = static_cast<int>( EDA_UNIT_UTILS::UI::FromUserUnit(
            iuScale, EDA_UNITS::MM, results[TRANSLINE_PARAMETERS::PHYS_S].first * 1000.0 ) );
    int propDelay = static_cast<int>( EDA_UNIT_UTILS::UI::FromUserUnit(
            iuScale, EDA_UNITS::PS_PER_CM, results[TRANSLINE_PARAMETERS::UNIT_PROP_DELAY_ODD].first ) );

    return CALCULATION_RESULT{ calcWidth, calcGap, propDelay };
}


PANEL_SETUP_TUNING_PROFILE_INFO::CALCULATION_RESULT
PANEL_SETUP_TUNING_PROFILE_INFO::calculateDifferentialStripline( const int aRow, CalculationType aCalculationType )
{
    const EDA_IU_SCALE& iuScale = m_parentPanel->m_unitsProvider->GetIuScale();

    // Get the target impedance
    const double targetZ = getTargetImpedance();

    if( targetZ <= 0 )
        return CALCULATION_RESULT{ _( "Target impedance must be greater than 0" ) };

    // Get board parameters
    auto [boardParameters, result] = getStriplineBoardParameters( aRow, iuScale );

    if( !result.OK )
        return result;

    // Set calculation parameters
    double width = 0.0;
    double gap = 0.0;

    const std::optional<int> widthOpt = m_trackPropagationGrid->GetOptionalUnitValue( aRow, TRACK_GRID_TRACK_WIDTH );
    const std::optional<int> gapOpt = m_trackPropagationGrid->GetOptionalUnitValue( aRow, TRACK_GRID_TRACK_GAP );

    if( aCalculationType == CalculationType::WIDTH )
    {
        if( !gapOpt || *gapOpt <= 0 )
            return CALCULATION_RESULT{ _( "Diff pair gap must be greater than 0 to calculate width" ) };

        gap = iuScale.IUTomm( gapOpt.value() ) / 1000.0;
    }
    else if( aCalculationType == CalculationType::GAP )
    {
        if( !widthOpt || *widthOpt <= 0 )
            return CALCULATION_RESULT{ _( "Width must be greater than 0 to calculate diff pair gap" ) };

        width = iuScale.IUTomm( widthOpt.value() ) / 1000.0;
    }
    else if( aCalculationType == CalculationType::DELAY )
    {
        if( !widthOpt || !gapOpt || *widthOpt <= 0 || *gapOpt <= 0 )
            return CALCULATION_RESULT{ _( "Width and diff pair gap must be greater than 0 to calculate delay" ) };

        width = iuScale.IUTomm( widthOpt.value() ) / 1000.0;
        gap = iuScale.IUTomm( gapOpt.value() ) / 1000.0;
    }

    // Run the synthesis
    m_coupledStriplineCalc.SetParameter( TRANSLINE_PARAMETERS::Z0_E, targetZ / 2.0 );
    m_coupledStriplineCalc.SetParameter( TRANSLINE_PARAMETERS::Z0_O, targetZ / 2.0 );
    m_coupledStriplineCalc.SetParameter( TRANSLINE_PARAMETERS::Z_DIFF, targetZ );
    m_coupledStriplineCalc.SetParameter( TRANSLINE_PARAMETERS::PHYS_WIDTH, width );
    m_coupledStriplineCalc.SetParameter( TRANSLINE_PARAMETERS::PHYS_S, gap );
    m_coupledStriplineCalc.SetParameter( TRANSLINE_PARAMETERS::T, boardParameters.SignalLayerThickness );
    m_coupledStriplineCalc.SetParameter(
            TRANSLINE_PARAMETERS::H, boardParameters.TopDielectricLayerThickness + boardParameters.SignalLayerThickness
                                             + boardParameters.BottomDielectricLayerThickness );
    m_coupledStriplineCalc.SetParameter( TRANSLINE_PARAMETERS::EPSILONR, boardParameters.DielectricConstant );
    m_coupledStriplineCalc.SetParameter( TRANSLINE_PARAMETERS::SKIN_DEPTH, calculateSkinDepth( 1.0, 1.0, 1.0 / RHO ) );
    m_coupledStriplineCalc.SetParameter( TRANSLINE_PARAMETERS::PHYS_LEN, 1.0 );
    m_coupledStriplineCalc.SetParameter( TRANSLINE_PARAMETERS::FREQUENCY, 1000000000.0 );
    m_coupledStriplineCalc.SetParameter( TRANSLINE_PARAMETERS::ANG_L, 1.0 );
    m_coupledStriplineCalc.SetParameter( TRANSLINE_PARAMETERS::SIGMA, 1.0 / RHO );
    m_coupledStriplineCalc.SetParameter( TRANSLINE_PARAMETERS::MURC, 1 );

    switch( aCalculationType )
    {
    case CalculationType::WIDTH: m_coupledStriplineCalc.Synthesize( SYNTHESIZE_OPTS::FIX_SPACING ); break;
    case CalculationType::GAP: m_coupledStriplineCalc.Synthesize( SYNTHESIZE_OPTS::FIX_WIDTH ); break;
    case CalculationType::DELAY: m_coupledStriplineCalc.Analyse(); break;
    }

    std::unordered_map<TRANSLINE_PARAMETERS, std::pair<double, TRANSLINE_STATUS>>& results =
            [this, aCalculationType]() -> decltype( m_coupledStriplineCalc.GetSynthesisResults() )
    {
        if( aCalculationType == CalculationType::WIDTH || aCalculationType == CalculationType::GAP )
            return m_coupledStriplineCalc.GetSynthesisResults();

        return m_coupledStriplineCalc.GetAnalysisResults();
    }();

    if( results[TRANSLINE_PARAMETERS::PHYS_WIDTH].second != TRANSLINE_STATUS::OK )
        return CALCULATION_RESULT{ _( "Width calculation failed" ) };

    if( results[TRANSLINE_PARAMETERS::PHYS_S].second != TRANSLINE_STATUS::OK )
        return CALCULATION_RESULT{ _( "Diff pair gap calculation failed" ) };

    if( results[TRANSLINE_PARAMETERS::UNIT_PROP_DELAY_ODD].second != TRANSLINE_STATUS::OK )
        return CALCULATION_RESULT{ _( "Delay calculation failed" ) };

    int calcWidth = static_cast<int>( EDA_UNIT_UTILS::UI::FromUserUnit(
            iuScale, EDA_UNITS::MM, results[TRANSLINE_PARAMETERS::PHYS_WIDTH].first * 1000.0 ) );
    int calcGap = static_cast<int>( EDA_UNIT_UTILS::UI::FromUserUnit(
            iuScale, EDA_UNITS::MM, results[TRANSLINE_PARAMETERS::PHYS_S].first * 1000.0 ) );
    int propDelay = static_cast<int>( EDA_UNIT_UTILS::UI::FromUserUnit(
            iuScale, EDA_UNITS::PS_PER_CM, results[TRANSLINE_PARAMETERS::UNIT_PROP_DELAY_ODD].first ) );

    return CALCULATION_RESULT{ calcWidth, calcGap, propDelay };
}
