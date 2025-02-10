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

#include <widgets/wx_panel.h>
#include <widgets/std_bitmap_button.h>

#include <bitmaps.h>
#include <dialogs/panel_setup_time_domain_parameters.h>
#include <pcb_edit_frame.h>
#include <grid_tricks.h>
#include <layer_ids.h>
#include <pgm_base.h>
#include <widgets/grid_icon_text_helpers.h>
#include <widgets/paged_dialog.h>

PANEL_SETUP_TIME_DOMAIN_PARAMETERS::PANEL_SETUP_TIME_DOMAIN_PARAMETERS(
        wxWindow* aParentWindow, PCB_EDIT_FRAME* aFrame, BOARD* aBoard,
        std::shared_ptr<TIME_DOMAIN_PARAMETERS> aTimeDomainParameters ) :
        PANEL_SETUP_TIME_DOMAIN_PARAMETERS_BASE( aParentWindow ),
        m_timeDomainParameters( std::move( aTimeDomainParameters ) ), m_frame( aFrame ), m_board( aFrame->GetBoard() )
{
    m_timeDomainParametersPane->SetBorders( true, false, false, false );

    // Set up units
    m_unitsProvider = std::make_unique<UNITS_PROVIDER>( pcbIUScale, m_frame->GetUserUnits() );
    m_tracePropagationGrid->SetUnitsProvider( m_unitsProvider.get() );
    m_viaPropagationGrid->SetUnitsProvider( m_unitsProvider.get() );

    Freeze();

    m_splitter->SetMinimumPaneSize( FromDIP( m_splitter->GetMinimumPaneSize() ) );

    // Set up the tuning profiles grid
    m_tracePropagationGrid->BeginBatch();
    m_tracePropagationGrid->SetUseNativeColLabels();

    m_tracePropagationGrid->EnsureColLabelsVisible();
    m_tracePropagationGrid->SetDefaultRowSize( m_tracePropagationGrid->GetDefaultRowSize() + 4 );
    m_tracePropagationGrid->PushEventHandler( new GRID_TRICKS( m_tracePropagationGrid ) );
    m_tracePropagationGrid->SetSelectionMode( wxGrid::wxGridSelectRows );

    m_addDelayProfileButton->SetBitmap( KiBitmapBundle( BITMAPS::small_plus ) );
    m_removeDelayProfileButton->SetBitmap( KiBitmapBundle( BITMAPS::small_trash ) );

    m_tracePropagationGrid->EndBatch();

    m_tracePropagationGrid->Connect(
            wxEVT_GRID_CELL_CHANGING,
            wxGridEventHandler( PANEL_SETUP_TIME_DOMAIN_PARAMETERS::OnDelayProfileGridCellChanging ), nullptr, this );

    // Set up the via override grid
    m_viaPropagationGrid->BeginBatch();
    m_viaPropagationGrid->SetUseNativeColLabels();

    m_viaPropagationGrid->EnsureColLabelsVisible();
    m_viaPropagationGrid->SetDefaultRowSize( m_viaPropagationGrid->GetDefaultRowSize() + 4 );
    m_viaPropagationGrid->PushEventHandler( new GRID_TRICKS( m_viaPropagationGrid ) );
    m_viaPropagationGrid->SetSelectionMode( wxGrid::wxGridSelectRows );

    std::vector<int> viaColIds;
    m_viaPropagationGrid->SetAutoEvalColUnits( VIA_GRID_DELAY,
                                               m_unitsProvider->GetUnitsFromType( EDA_DATA_TYPE::TIME ) );
    viaColIds.push_back( VIA_GRID_DELAY );
    m_viaPropagationGrid->SetAutoEvalCols( viaColIds );
    m_viaPropagationGrid->EndBatch();

    m_addViaOverrideButton->SetBitmap( KiBitmapBundle( BITMAPS::small_plus ) );
    m_removeViaOverrideButton->SetBitmap( KiBitmapBundle( BITMAPS::small_trash ) );

    setColumnWidths();

    Thaw();
}


PANEL_SETUP_TIME_DOMAIN_PARAMETERS::~PANEL_SETUP_TIME_DOMAIN_PARAMETERS()
{
    // Delete the GRID_TRICKS
    m_tracePropagationGrid->PopEventHandler( true );
    m_viaPropagationGrid->PopEventHandler( true );

    m_tracePropagationGrid->Disconnect(
            wxEVT_GRID_CELL_CHANGING,
            wxGridEventHandler( PANEL_SETUP_TIME_DOMAIN_PARAMETERS::OnDelayProfileGridCellChanging ), nullptr, this );
}


bool PANEL_SETUP_TIME_DOMAIN_PARAMETERS::TransferDataToWindow()
{
    m_tracePropagationGrid->ClearRows();
    m_viaPropagationGrid->ClearRows();

    const std::vector<TIME_DOMAIN_TUNING_PROFILE>& delayProfiles = m_timeDomainParameters->GetDelayProfiles();

    SyncCopperLayers( m_board->GetCopperLayerCount() );

    for( const TIME_DOMAIN_TUNING_PROFILE& profile : delayProfiles )
    {
        addProfileRow( profile );

        for( const TUNING_PROFILE_VIA_OVERRIDE_ENTRY& viaOverride : profile.m_ViaOverrides )
            addViaRow( profile.m_ProfileName, viaOverride );
    }

    updateViaProfileNamesEditor();

    return true;
}


bool PANEL_SETUP_TIME_DOMAIN_PARAMETERS::TransferDataFromWindow()
{
    if( !Validate() )
        return false;

    m_timeDomainParameters->ClearDelayProfiles();

    for( int i = 0; i < m_tracePropagationGrid->GetNumberRows(); ++i )
    {
        TIME_DOMAIN_TUNING_PROFILE profile = getProfileRow( i );
        wxString                   profileName = profile.m_ProfileName;

        for( int j = 0; j < m_viaPropagationGrid->GetNumberRows(); ++j )
        {
            if( m_viaPropagationGrid->GetCellValue( j, VIA_GRID_PROFILE_NAME ) == profileName )
                profile.m_ViaOverrides.emplace_back( getViaRow( j ) );
        }

        m_timeDomainParameters->AddDelayProfile( std::move( profile ) );
    }

    return true;
}


void PANEL_SETUP_TIME_DOMAIN_PARAMETERS::addProfileRow( const TIME_DOMAIN_TUNING_PROFILE& aDelayProfile )
{
    const int rowId = m_tracePropagationGrid->GetNumberRows();
    m_tracePropagationGrid->AppendRows();

    m_tracePropagationGrid->SetCellValue( rowId, PROFILE_GRID_PROFILE_NAME, aDelayProfile.m_ProfileName );
    m_tracePropagationGrid->SetUnitValue( rowId, PROFILE_GRID_VIA_PROP_DELAY, aDelayProfile.m_ViaPropagationDelay );

    for( const auto& [layerId, velocity] : aDelayProfile.m_LayerPropagationDelays )
    {
        if( !m_copperLayerIdsToColumns.contains( layerId ) )
            continue;

        int col = m_copperLayerIdsToColumns[layerId];

        if( col < m_tracePropagationGrid->GetNumberCols() )
        {
            m_tracePropagationGrid->SetUnitValue( rowId, col, velocity );
        }
    }
}


TIME_DOMAIN_TUNING_PROFILE PANEL_SETUP_TIME_DOMAIN_PARAMETERS::getProfileRow( const int aRow )
{
    TIME_DOMAIN_TUNING_PROFILE entry;
    entry.m_ProfileName = getProfileNameForProfileGridRow( aRow );
    entry.m_ViaPropagationDelay = m_tracePropagationGrid->GetUnitValue( aRow, PROFILE_GRID_VIA_PROP_DELAY );

    std::map<PCB_LAYER_ID, int> propDelays;

    for( const auto& [layer, col] : m_copperLayerIdsToColumns )
        propDelays[layer] = m_tracePropagationGrid->GetUnitValue( aRow, col );

    entry.m_LayerPropagationDelays = std::move( propDelays );

    return entry;
}


void PANEL_SETUP_TIME_DOMAIN_PARAMETERS::addViaRow( const wxString&                          aProfileName,
                                                    const TUNING_PROFILE_VIA_OVERRIDE_ENTRY& aViaOverrideEntry ) const
{
    const int rowId = m_viaPropagationGrid->GetNumberRows();
    m_viaPropagationGrid->AppendRows();
    m_viaPropagationGrid->SetCellValue( rowId, VIA_GRID_PROFILE_NAME, aProfileName );
    m_viaPropagationGrid->SetCellValue( rowId, VIA_GRID_SIGNAL_LAYER_FROM,
                                        m_board->GetLayerName( aViaOverrideEntry.m_SignalLayerFrom ) );
    m_viaPropagationGrid->SetCellValue( rowId, VIA_GRID_SIGNAL_LAYER_TO,
                                        m_board->GetLayerName( aViaOverrideEntry.m_SignalLayerTo ) );
    m_viaPropagationGrid->SetCellValue( rowId, VIA_GRID_VIA_LAYER_FROM,
                                        m_board->GetLayerName( aViaOverrideEntry.m_ViaLayerFrom ) );
    m_viaPropagationGrid->SetCellValue( rowId, VIA_GRID_VIA_LAYER_TO,
                                        m_board->GetLayerName( aViaOverrideEntry.m_ViaLayerTo ) );
    m_viaPropagationGrid->SetUnitValue( rowId, VIA_GRID_DELAY, aViaOverrideEntry.m_Delay );
}


TUNING_PROFILE_VIA_OVERRIDE_ENTRY PANEL_SETUP_TIME_DOMAIN_PARAMETERS::getViaRow( const int aRow )
{
    // Get layer info
    const wxString signalLayerFrom = m_viaPropagationGrid->GetCellValue( aRow, VIA_GRID_SIGNAL_LAYER_FROM );
    const wxString signalLayerTo = m_viaPropagationGrid->GetCellValue( aRow, VIA_GRID_SIGNAL_LAYER_TO );
    const wxString viaLayerFrom = m_viaPropagationGrid->GetCellValue( aRow, VIA_GRID_VIA_LAYER_FROM );
    const wxString viaLayerTo = m_viaPropagationGrid->GetCellValue( aRow, VIA_GRID_VIA_LAYER_TO );
    PCB_LAYER_ID   signalLayerIdFrom = m_layerNamesToIDs[signalLayerFrom];
    PCB_LAYER_ID   signalLayerIdTo = m_layerNamesToIDs[signalLayerTo];
    PCB_LAYER_ID   viaLayerIdFrom = m_layerNamesToIDs[viaLayerFrom];
    PCB_LAYER_ID   viaLayerIdTo = m_layerNamesToIDs[viaLayerTo];

    // Order layers in stackup order (from F_Cu first)
    if( IsCopperLayerLowerThan( signalLayerIdFrom, signalLayerIdTo ) )
        std::swap( signalLayerIdFrom, signalLayerIdTo );

    if( IsCopperLayerLowerThan( viaLayerIdFrom, viaLayerIdTo ) )
        std::swap( viaLayerIdFrom, viaLayerIdTo );

    const TUNING_PROFILE_VIA_OVERRIDE_ENTRY entry{ signalLayerIdFrom, signalLayerIdTo, viaLayerIdFrom, viaLayerIdTo,
                                                   m_viaPropagationGrid->GetUnitValue( aRow, VIA_GRID_DELAY ) };

    return entry;
}


void PANEL_SETUP_TIME_DOMAIN_PARAMETERS::SyncCopperLayers( int aNumCopperLayers )
{
    m_prevLayerNamesToIDs = m_layerNamesToIDs;
    m_copperLayerIdsToColumns.clear();
    m_copperColumnsToLayerId.clear();
    m_layerNames.clear();
    m_layerNamesToIDs.clear();

    int colIdx = PROFILE_GRID_NUM_REQUIRED_COLS;

    for( const auto& layer : LSET::AllCuMask( aNumCopperLayers ).CuStack() )
    {
        wxString layerName = m_board->GetLayerName( layer );
        m_layerNames.emplace_back( layerName );
        m_layerNamesToIDs[layerName] = layer;
        m_copperLayerIdsToColumns[layer] = colIdx;
        m_copperColumnsToLayerId[colIdx] = layer;
        ++colIdx;
    }

    updateProfileGridColumns();
    updateViaGridColumns();
    setColumnWidths();
}


void PANEL_SETUP_TIME_DOMAIN_PARAMETERS::setColumnWidths()
{
    const int minValueWidth = m_tracePropagationGrid->GetTextExtent( wxT( "000.00 ps/mm" ) ).x;
    const int minNameWidth = m_tracePropagationGrid->GetTextExtent( wxT( "MMMMMMMMMMMM" ) ).x;

    for( int i = 0; i < m_tracePropagationGrid->GetNumberCols(); ++i )
    {
        const int titleSize = m_tracePropagationGrid->GetTextExtent( m_tracePropagationGrid->GetColLabelValue( i ) ).x;

        if( i == PROFILE_GRID_PROFILE_NAME )
            m_tracePropagationGrid->SetColSize( i, std::max( titleSize, minNameWidth ) );
        else
            m_tracePropagationGrid->SetColSize( i, std::max( titleSize, minValueWidth ) );
    }

    for( int i = 0; i < m_viaPropagationGrid->GetNumberCols(); ++i )
    {
        const int titleSize = GetTextExtent( m_viaPropagationGrid->GetColLabelValue( i ) ).x;
        if( i == VIA_GRID_PROFILE_NAME )
            m_viaPropagationGrid->SetColSize( i, std::max( titleSize, minNameWidth ) );
        else
            m_viaPropagationGrid->SetColSize( i, titleSize + 30 );
    }
}


void PANEL_SETUP_TIME_DOMAIN_PARAMETERS::updateProfileGridColumns()
{
    const int newCopperLayers = static_cast<int>( m_copperLayerIdsToColumns.size() );
    const int curCopperLayers = m_tracePropagationGrid->GetNumberCols() - PROFILE_GRID_NUM_REQUIRED_COLS;

    if( newCopperLayers < curCopperLayers )
    {
        // TODO: WARN OF DELETING DATA?
        m_tracePropagationGrid->DeleteCols( curCopperLayers - newCopperLayers + PROFILE_GRID_NUM_REQUIRED_COLS,
                                            curCopperLayers - newCopperLayers );
    }
    else if( newCopperLayers > curCopperLayers )
    {
        m_tracePropagationGrid->AppendCols( newCopperLayers - curCopperLayers );
    }

    std::vector<int> copperColIds;

    m_tracePropagationGrid->SetAutoEvalColUnits( PROFILE_GRID_VIA_PROP_DELAY,
                                                 m_unitsProvider->GetUnitsFromType( EDA_DATA_TYPE::LENGTH_DELAY ) );
    copperColIds.push_back( PROFILE_GRID_VIA_PROP_DELAY );

    for( const auto& [colIdx, layerId] : m_copperColumnsToLayerId )
    {
        m_tracePropagationGrid->SetColLabelValue( colIdx, m_board->GetLayerName( layerId ) );
        m_tracePropagationGrid->SetAutoEvalColUnits( colIdx,
                                                     m_unitsProvider->GetUnitsFromType( EDA_DATA_TYPE::LENGTH_DELAY ) );
        copperColIds.push_back( colIdx );
    }

    m_tracePropagationGrid->SetAutoEvalCols( copperColIds );

    m_tracePropagationGrid->EnsureColLabelsVisible();
    m_tracePropagationGrid->Refresh();
}


void PANEL_SETUP_TIME_DOMAIN_PARAMETERS::updateViaGridColumns()
{
    wxArrayString layerNames;
    std::ranges::for_each( m_layerNames,
                           [&layerNames]( const wxString& aLayerName )
                           {
                               layerNames.push_back( aLayerName );
                           } );

    // Save the current data
    std::vector<wxString> currentSignalLayersFrom;
    std::vector<wxString> currentSignalLayersTo;
    std::vector<wxString> currentViaLayersFrom;
    std::vector<wxString> currentViaLayersTo;

    for( int row = 0; row < m_viaPropagationGrid->GetNumberRows(); ++row )
    {
        currentSignalLayersFrom.emplace_back( m_viaPropagationGrid->GetCellValue( row, VIA_GRID_SIGNAL_LAYER_FROM ) );
        currentSignalLayersTo.emplace_back( m_viaPropagationGrid->GetCellValue( row, VIA_GRID_SIGNAL_LAYER_TO ) );
        currentViaLayersFrom.emplace_back( m_viaPropagationGrid->GetCellValue( row, VIA_GRID_VIA_LAYER_FROM ) );
        currentViaLayersTo.emplace_back( m_viaPropagationGrid->GetCellValue( row, VIA_GRID_VIA_LAYER_TO ) );
    }

    // Reset the via layers lists
    wxGridCellAttr* attr = new wxGridCellAttr;
    attr->SetEditor( new wxGridCellChoiceEditor( layerNames, false ) );
    m_viaPropagationGrid->SetColAttr( VIA_GRID_SIGNAL_LAYER_FROM, attr );

    attr = new wxGridCellAttr;
    attr->SetEditor( new wxGridCellChoiceEditor( layerNames, false ) );
    m_viaPropagationGrid->SetColAttr( VIA_GRID_SIGNAL_LAYER_TO, attr );

    attr = new wxGridCellAttr;
    attr->SetEditor( new wxGridCellChoiceEditor( layerNames, false ) );
    m_viaPropagationGrid->SetColAttr( VIA_GRID_VIA_LAYER_FROM, attr );

    attr = new wxGridCellAttr;
    attr->SetEditor( new wxGridCellChoiceEditor( layerNames, false ) );
    m_viaPropagationGrid->SetColAttr( VIA_GRID_VIA_LAYER_TO, attr );

    // Restore the data, changing or resetting layer names if required
    for( int row = 0; row < m_viaPropagationGrid->GetNumberRows(); ++row )
    {
        const PCB_LAYER_ID lastSignalFromId = m_prevLayerNamesToIDs[currentSignalLayersFrom[row]];

        if( m_copperLayerIdsToColumns.contains( lastSignalFromId ) )
            m_viaPropagationGrid->SetCellValue( row, VIA_GRID_SIGNAL_LAYER_FROM,
                                                m_board->GetLayerName( lastSignalFromId ) );
        else
            m_viaPropagationGrid->SetCellValue( row, VIA_GRID_SIGNAL_LAYER_FROM, m_layerNames.front() );

        const PCB_LAYER_ID lastSignalToId = m_prevLayerNamesToIDs[currentSignalLayersTo[row]];

        if( m_copperLayerIdsToColumns.contains( lastSignalToId ) )
            m_viaPropagationGrid->SetCellValue( row, VIA_GRID_SIGNAL_LAYER_TO,
                                                m_board->GetLayerName( lastSignalToId ) );
        else
            m_viaPropagationGrid->SetCellValue( row, VIA_GRID_SIGNAL_LAYER_TO, m_layerNames.back() );

        const PCB_LAYER_ID lastViaFromId = m_prevLayerNamesToIDs[currentViaLayersFrom[row]];

        if( m_copperLayerIdsToColumns.contains( lastViaFromId ) )
            m_viaPropagationGrid->SetCellValue( row, VIA_GRID_VIA_LAYER_FROM, m_board->GetLayerName( lastViaFromId ) );
        else
            m_viaPropagationGrid->SetCellValue( row, VIA_GRID_VIA_LAYER_FROM, m_layerNames.front() );

        const PCB_LAYER_ID lastViaToId = m_prevLayerNamesToIDs[currentViaLayersTo[row]];

        if( m_copperLayerIdsToColumns.contains( lastViaToId ) )
            m_viaPropagationGrid->SetCellValue( row, VIA_GRID_VIA_LAYER_TO, m_board->GetLayerName( lastViaToId ) );
        else
            m_viaPropagationGrid->SetCellValue( row, VIA_GRID_VIA_LAYER_TO, m_layerNames.back() );
    }
}


void PANEL_SETUP_TIME_DOMAIN_PARAMETERS::OnAddDelayProfileClick( wxCommandEvent& event )
{
    if( !m_tracePropagationGrid->CommitPendingChanges() )
        return;

    const int rowId = m_tracePropagationGrid->GetNumberRows();
    m_tracePropagationGrid->AppendRows();
    m_tracePropagationGrid->SetCellValue( rowId, PROFILE_GRID_PROFILE_NAME, "" );

    for( int i = PROFILE_GRID_VIA_PROP_DELAY; i < m_tracePropagationGrid->GetNumberCols(); ++i )
        m_tracePropagationGrid->SetUnitValue( rowId, i, 0 );

    const int newRow = m_tracePropagationGrid->GetNumberRows() - 1;
    m_tracePropagationGrid->MakeCellVisible( newRow, PROFILE_GRID_PROFILE_NAME );
    m_tracePropagationGrid->SetGridCursor( newRow, PROFILE_GRID_PROFILE_NAME );
    m_tracePropagationGrid->EnableCellEditControl( true );
    m_tracePropagationGrid->ShowCellEditControl();
}


void PANEL_SETUP_TIME_DOMAIN_PARAMETERS::OnRemoveDelayProfileClick( wxCommandEvent& event )
{
    if( !m_tracePropagationGrid->CommitPendingChanges() )
        return;

    const int curRow = m_tracePropagationGrid->GetGridCursorRow();

    if( curRow < 0 )
        return;

    wxString profileName = getProfileNameForProfileGridRow( curRow );

    // Delete associated via overrides
    for( int viaRow = m_viaPropagationGrid->GetNumberRows() - 1; viaRow >= 0; --viaRow )
    {
        if( m_viaPropagationGrid->GetCellValue( viaRow, VIA_GRID_PROFILE_NAME ) == profileName )
            m_viaPropagationGrid->DeleteRows( viaRow, 1 );
    }

    // Delete tuning profile
    m_tracePropagationGrid->DeleteRows( curRow, 1 );

    m_tracePropagationGrid->MakeCellVisible( std::max( 0, curRow - 1 ),
                                             m_tracePropagationGrid->GetGridCursorCol() );
    m_tracePropagationGrid->SetGridCursor( std::max( 0, curRow - 1 ),
                                           m_tracePropagationGrid->GetGridCursorCol() );

    updateViaProfileNamesEditor();
}


void PANEL_SETUP_TIME_DOMAIN_PARAMETERS::OnAddViaOverrideClick( wxCommandEvent& event )
{
    if( !m_viaPropagationGrid->CommitPendingChanges() )
        return;

    const int rowId = m_viaPropagationGrid->GetNumberRows();
    m_viaPropagationGrid->AppendRows();
    m_viaPropagationGrid->SetCellValue( rowId, PROFILE_GRID_PROFILE_NAME, "" );
    m_viaPropagationGrid->SetCellValue( rowId, VIA_GRID_SIGNAL_LAYER_FROM, m_layerNames.front() );
    m_viaPropagationGrid->SetCellValue( rowId, VIA_GRID_SIGNAL_LAYER_TO, m_layerNames.back() );
    m_viaPropagationGrid->SetCellValue( rowId, VIA_GRID_VIA_LAYER_FROM, m_layerNames.front() );
    m_viaPropagationGrid->SetCellValue( rowId, VIA_GRID_VIA_LAYER_TO, m_layerNames.back() );
    m_viaPropagationGrid->SetUnitValue( rowId, VIA_GRID_DELAY, 0 );
}


void PANEL_SETUP_TIME_DOMAIN_PARAMETERS::OnRemoveViaOverrideClick( wxCommandEvent& event )
{
    if( !m_viaPropagationGrid->CommitPendingChanges() )
        return;

    int curRow = m_viaPropagationGrid->GetGridCursorRow();

    if( curRow < 0 )
        return;

    m_viaPropagationGrid->DeleteRows( curRow, 1 );

    m_viaPropagationGrid->MakeCellVisible( std::max( 0, curRow - 1 ), m_viaPropagationGrid->GetGridCursorCol() );
    m_viaPropagationGrid->SetGridCursor( std::max( 0, curRow - 1 ), m_viaPropagationGrid->GetGridCursorCol() );
}


void PANEL_SETUP_TIME_DOMAIN_PARAMETERS::OnDelayProfileGridCellChanging( wxGridEvent& event )
{
    if( event.GetCol() == PROFILE_GRID_PROFILE_NAME )
    {
        if( validateDelayProfileName( event.GetRow(), event.GetString() ) )
        {
            const wxString oldName = getProfileNameForProfileGridRow( event.GetRow() );
            wxString       newName = event.GetString();
            newName.Trim( true ).Trim( false );

            if( !oldName.IsEmpty() )
            {
                m_viaPropagationGrid->Freeze();

                updateViaProfileNamesEditor( oldName, newName );

                // Update changed profile names
                for( int row = 0; row < m_viaPropagationGrid->GetNumberRows(); ++row )
                {
                    if( m_viaPropagationGrid->GetCellValue( row, VIA_GRID_PROFILE_NAME ) == oldName )
                        m_viaPropagationGrid->SetCellValue( row, VIA_GRID_PROFILE_NAME, newName );
                }

                m_viaPropagationGrid->Thaw();
            }
            else
            {
                updateViaProfileNamesEditor( oldName, newName );
            }
        }
        else
        {
            event.Veto();
        }
    }
}


void PANEL_SETUP_TIME_DOMAIN_PARAMETERS::updateViaProfileNamesEditor( const wxString& aOldName,
                                                                      const wxString& aNewName ) const
{
    wxArrayString profileNames;

    for( int i = 0; i < m_tracePropagationGrid->GetNumberRows(); ++i )
    {
        wxString profileName = getProfileNameForProfileGridRow( i );

        if( profileName == aOldName )
            profileName = aNewName;

        profileNames.push_back( profileName );
    }

    std::ranges::sort( profileNames,
                       []( const wxString& a, const wxString& b )
                       {
                           return a.CmpNoCase( b ) < 0;
                       } );

    wxGridCellAttr* attr = new wxGridCellAttr;
    attr->SetEditor( new wxGridCellChoiceEditor( profileNames, false ) );
    m_viaPropagationGrid->SetColAttr( VIA_GRID_PROFILE_NAME, attr );
}


bool PANEL_SETUP_TIME_DOMAIN_PARAMETERS::validateDelayProfileName( int aRow, const wxString& aName, bool focusFirst )
{
    wxString tmp = aName;
    tmp.Trim( true );
    tmp.Trim( false );

    if( tmp.IsEmpty() )
    {
        const wxString msg = _( "Tuning profile must have a name" );
        PAGED_DIALOG::GetDialog( this )->SetError( msg, this, m_tracePropagationGrid, aRow, PROFILE_GRID_PROFILE_NAME );
        return false;
    }

    for( int ii = 0; ii < m_tracePropagationGrid->GetNumberRows(); ii++ )
    {
        if( ii != aRow && getProfileNameForProfileGridRow( ii ).CmpNoCase( tmp ) == 0 )
        {
            const wxString msg = _( "Tuning profile name already in use" );
            PAGED_DIALOG::GetDialog( this )->SetError( msg, this, m_tracePropagationGrid, focusFirst ? aRow : ii,
                                                       PROFILE_GRID_PROFILE_NAME );
            return false;
        }
    }

    return true;
}


bool PANEL_SETUP_TIME_DOMAIN_PARAMETERS::Validate()
{
    if( !m_tracePropagationGrid->CommitPendingChanges() || !m_viaPropagationGrid->CommitPendingChanges() )
        return false;

    // Test delay profile parameters
    for( int row = 0; row < m_tracePropagationGrid->GetNumberRows(); row++ )
    {
        const wxString profileName = getProfileNameForProfileGridRow( row );

        if( !validateDelayProfileName( row, profileName, false ) )
            return false;
    }

    // Test via override parameters
    if( !validateViaRows() )
        return false;

    return true;
}


bool PANEL_SETUP_TIME_DOMAIN_PARAMETERS::validateViaRows()
{
    std::map<wxString, std::set<TUNING_PROFILE_VIA_OVERRIDE_ENTRY>> rowCache;

    for( int row = 0; row < m_viaPropagationGrid->GetNumberRows(); row++ )
    {
        TUNING_PROFILE_VIA_OVERRIDE_ENTRY entry = getViaRow( row );
        const wxString profileName = m_viaPropagationGrid->GetCellValue( row, VIA_GRID_PROFILE_NAME );
        std::set<TUNING_PROFILE_VIA_OVERRIDE_ENTRY>& viaOverrides = rowCache[profileName];

        if( viaOverrides.contains( entry ) )
        {
            const wxString msg = _( "Via override configuration is duplicated" );
            PAGED_DIALOG::GetDialog( this )->SetError( msg, this, m_viaPropagationGrid, row, VIA_GRID_PROFILE_NAME );
            return false;
        }
        else
        {
            viaOverrides.insert( entry );
        }
    }

    return true;
}


std::vector<wxString> PANEL_SETUP_TIME_DOMAIN_PARAMETERS::GetDelayProfileNames() const
{
    std::vector<wxString> profileNames;

    for( int i = 0; i < m_tracePropagationGrid->GetNumberRows(); i++ )
    {
        const wxString profileName = getProfileNameForProfileGridRow( i );
        profileNames.emplace_back( profileName );
    }

    std::ranges::sort( profileNames,
                       []( const wxString& a, const wxString& b )
                       {
                           return a.CmpNoCase( b ) < 0;
                       } );

    return profileNames;
}


void PANEL_SETUP_TIME_DOMAIN_PARAMETERS::ImportSettingsFrom(
        const std::shared_ptr<TIME_DOMAIN_PARAMETERS>& aOtherParameters )
{
    std::shared_ptr<TIME_DOMAIN_PARAMETERS> savedParameters = m_timeDomainParameters;

    m_timeDomainParameters = aOtherParameters;
    TransferDataToWindow();

    updateViaProfileNamesEditor();

    m_viaPropagationGrid->ForceRefresh();

    m_timeDomainParameters = std::move( savedParameters );
}


wxString PANEL_SETUP_TIME_DOMAIN_PARAMETERS::getProfileNameForProfileGridRow( const int aRow ) const
{
    wxString profileName = m_tracePropagationGrid->GetCellValue( aRow, PROFILE_GRID_PROFILE_NAME );
    profileName.Trim( true ).Trim( false );
    return profileName;
}