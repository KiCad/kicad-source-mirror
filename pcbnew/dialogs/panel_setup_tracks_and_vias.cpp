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


#include <pcb_edit_frame.h>
#include <board_design_settings.h>
#include <pcb_track.h>
#include <bitmaps.h>
#include <widgets/wx_grid.h>
#include <widgets/std_bitmap_button.h>
#include <optional>
#include <grid_tricks.h>

#include <panel_setup_tracks_and_vias.h>


enum TRACK_VAR_GRID_COLUMNS
{
    TR_WIDTH_COL = 0
};

enum VIA_VAR_GRID_COLUMNS
{
    VIA_SIZE_COL = 0,
    VIA_DRILL_COL
};

enum DIFF_VAR_GRID_COLUMNS
{
    DP_WIDTH_COL = 0,
    DP_GAP_COL,
    DP_VIA_GAP_COL
};


PANEL_SETUP_TRACKS_AND_VIAS::PANEL_SETUP_TRACKS_AND_VIAS( wxWindow* aParentWindow,
                                                          PCB_EDIT_FRAME* aFrame ) :
        PANEL_SETUP_TRACKS_AND_VIAS_BASE( aParentWindow )
{
    m_Frame = aFrame;
    m_Pcb = m_Frame->GetBoard();
    m_BrdSettings = &m_Pcb->GetDesignSettings();

    m_trackWidthsAddButton->SetBitmap( KiBitmapBundle( BITMAPS::small_plus ) );
    m_trackWidthsSortButton->SetBitmap( KiBitmapBundle( BITMAPS::small_sort_desc ) );
    m_trackWidthsRemoveButton->SetBitmap( KiBitmapBundle( BITMAPS::small_trash ) );
    m_viaSizesAddButton->SetBitmap( KiBitmapBundle( BITMAPS::small_plus ) );
    m_viaSizesSortButton->SetBitmap( KiBitmapBundle( BITMAPS::small_sort_desc ) );
    m_viaSizesRemoveButton->SetBitmap( KiBitmapBundle( BITMAPS::small_trash ) );
    m_diffPairsAddButton->SetBitmap( KiBitmapBundle( BITMAPS::small_plus ) );
    m_diffPairsSortButton->SetBitmap( KiBitmapBundle( BITMAPS::small_sort_desc ) );
    m_diffPairsRemoveButton->SetBitmap( KiBitmapBundle( BITMAPS::small_trash ) );

    m_trackWidthsGrid->PushEventHandler( new GRID_TRICKS( m_trackWidthsGrid,
                                                          [this]( wxCommandEvent& aEvent )
                                                          {
                                                              OnAddTrackWidthsClick( aEvent );
                                                          } ) );
    m_viaSizesGrid->PushEventHandler( new GRID_TRICKS( m_viaSizesGrid,
                                                       [this]( wxCommandEvent& aEvent )
                                                       {
                                                           OnAddViaSizesClick( aEvent );
                                                       } ) );
    m_diffPairsGrid->PushEventHandler( new GRID_TRICKS( m_diffPairsGrid,
                                                        [this]( wxCommandEvent& aEvent )
                                                        {
                                                            OnAddDiffPairsClick( aEvent );
                                                        } ) );

    m_trackWidthsGrid->SetSelectionMode( wxGrid::wxGridSelectionModes::wxGridSelectRows );
    m_viaSizesGrid->SetSelectionMode( wxGrid::wxGridSelectionModes::wxGridSelectRows );
    m_diffPairsGrid->SetSelectionMode( wxGrid::wxGridSelectionModes::wxGridSelectRows );

    m_trackWidthsGrid->SetUnitsProvider( m_Frame );
    m_viaSizesGrid->SetUnitsProvider( m_Frame );
    m_diffPairsGrid->SetUnitsProvider( m_Frame );

    m_trackWidthsGrid->SetAutoEvalCols( { 0 } );
    m_viaSizesGrid->SetAutoEvalCols( { 0, 1 } );
    m_diffPairsGrid->SetAutoEvalCols( { 0, 1, 2 } );

    m_trackWidthsGrid->SetUseNativeColLabels();
    m_viaSizesGrid->SetUseNativeColLabels();
    m_diffPairsGrid->SetUseNativeColLabels();

    // Ensure width of columns is enough to enter any reasonable value
    WX_GRID* grid_list[] = { m_trackWidthsGrid, m_viaSizesGrid, m_diffPairsGrid, nullptr };
    int min_linesize = m_trackWidthsGrid->GetTextExtent( wxT( "000.000000 mm " ) ).x;

    for( int ii = 0; grid_list[ii]; ii++ )
    {
        WX_GRID* curr_grid = grid_list[ii];

        for( int col = 0; col < curr_grid->GetNumberCols(); col++ )
        {
            int min_w = curr_grid->GetVisibleWidth( col, true, true, true );
            int best_w = std::max( min_linesize, min_w );
            curr_grid->SetColMinimalWidth( col, best_w );
            curr_grid->SetColSize( col,best_w );
        }
    }

    m_Frame->Bind( EDA_EVT_UNITS_CHANGED, &PANEL_SETUP_TRACKS_AND_VIAS::onUnitsChanged, this );
}


PANEL_SETUP_TRACKS_AND_VIAS::~PANEL_SETUP_TRACKS_AND_VIAS()
{
    // Delete the GRID_TRICKS.
    m_trackWidthsGrid->PopEventHandler( true );
    m_viaSizesGrid->PopEventHandler( true );
    m_diffPairsGrid->PopEventHandler( true );

    m_Frame->Unbind( EDA_EVT_UNITS_CHANGED, &PANEL_SETUP_TRACKS_AND_VIAS::onUnitsChanged, this );
}


void PANEL_SETUP_TRACKS_AND_VIAS::OnSortTrackWidthsClick( wxCommandEvent& aEvent )
{
    if( m_trackWidthsGrid->GetNumberRows() < 2 )
        return;

    std::vector<int> trackWidths;
    wxString         msg;

    m_trackWidthsGrid->ClearSelection();
    wxSafeYield();

    wxGridUpdateLocker lock( m_trackWidthsGrid );

    for( int row = 0; row < m_trackWidthsGrid->GetNumberRows();  ++row )
    {
        msg = m_trackWidthsGrid->GetCellValue( row, TR_WIDTH_COL );

        if( !msg.IsEmpty() )
            trackWidths.push_back( m_Frame->ValueFromString( msg ) );
    }

    std::sort( trackWidths.begin(), trackWidths.end() );
    m_trackWidthsGrid->ClearRows( false );

    for( int width : trackWidths )
        AppendTrackWidth( width );
}


void PANEL_SETUP_TRACKS_AND_VIAS::OnSortViaSizesClick( wxCommandEvent& aEvent )
{
    if( m_viaSizesGrid->GetNumberRows() < 2 )
        return;

    std::vector<VIA_DIMENSION> vias;
    wxString                   msg;

    m_viaSizesGrid->ClearSelection();
    wxSafeYield();

    wxGridUpdateLocker lock( m_viaSizesGrid );

    for( int row = 0; row < m_viaSizesGrid->GetNumberRows();  ++row )
    {
        msg = m_viaSizesGrid->GetCellValue( row, VIA_SIZE_COL );

        if( !msg.IsEmpty() )
        {
            VIA_DIMENSION via_dim;
            via_dim.m_Diameter = m_Frame->ValueFromString( msg );

            msg = m_viaSizesGrid->GetCellValue( row, VIA_DRILL_COL );

            if( !msg.IsEmpty() )
                via_dim.m_Drill = m_Frame->ValueFromString( msg );

            vias.push_back( via_dim );
        }
    }

    std::sort( vias.begin(), vias.end() );
    m_viaSizesGrid->ClearRows( false );

    for( const VIA_DIMENSION& via : vias )
        AppendViaSize( via.m_Diameter, via.m_Drill );
}


void PANEL_SETUP_TRACKS_AND_VIAS::OnSortDiffPairsClick( wxCommandEvent& aEvent )
{
    if( m_diffPairsGrid->GetNumberRows() < 2 )
        return;

    wxString                         msg;
    std::vector<DIFF_PAIR_DIMENSION> diffPairs;

    m_diffPairsGrid->ClearSelection();
    wxSafeYield();

    wxGridUpdateLocker lock( m_diffPairsGrid );

    for( int row = 0; row < m_diffPairsGrid->GetNumberRows();  ++row )
    {
        msg = m_diffPairsGrid->GetCellValue( row, DP_WIDTH_COL );

        if( !msg.IsEmpty() )
        {
            DIFF_PAIR_DIMENSION diffPair_dim;
            diffPair_dim.m_Width = m_Frame->ValueFromString( msg );

            msg = m_diffPairsGrid->GetCellValue( row, DP_GAP_COL );
            diffPair_dim.m_Gap = m_Frame->ValueFromString( msg );

            msg = m_diffPairsGrid->GetCellValue( row, DP_VIA_GAP_COL );

            if( !msg.IsEmpty() )
                diffPair_dim.m_ViaGap = m_Frame->ValueFromString( msg );

            diffPairs.push_back( diffPair_dim );
        }
    }

    std::sort( diffPairs.begin(), diffPairs.end() );
    m_diffPairsGrid->ClearRows( false );

    for( const DIFF_PAIR_DIMENSION& dp : diffPairs )
        AppendDiffPairs( dp.m_Width, dp.m_Gap, dp.m_ViaGap );
}


void PANEL_SETUP_TRACKS_AND_VIAS::onUnitsChanged( wxCommandEvent& aEvent )
{
    BOARD_DESIGN_SETTINGS  tempBDS( nullptr, "dummy" );
    BOARD_DESIGN_SETTINGS* saveBDS = m_BrdSettings;

    m_BrdSettings = &tempBDS;       // No, address of stack var does not escape function

    TransferDataFromWindow();
    TransferDataToWindow();

    m_BrdSettings = saveBDS;

    aEvent.Skip();
}


bool PANEL_SETUP_TRACKS_AND_VIAS::TransferDataToWindow()
{
    m_trackWidthsGrid->ClearRows();
    m_viaSizesGrid->ClearRows();
    m_diffPairsGrid->ClearRows();

    // Skip the first item, which is the current netclass value
    for( unsigned ii = 1; ii < m_BrdSettings->m_TrackWidthList.size(); ii++ )
    {
        AppendTrackWidth( m_BrdSettings->m_TrackWidthList[ii] );
    }

    // Skip the first item, which is the current netclass value
    for( unsigned ii = 1; ii < m_BrdSettings->m_ViasDimensionsList.size(); ii++ )
    {
        AppendViaSize( m_BrdSettings->m_ViasDimensionsList[ii].m_Diameter,
                       m_BrdSettings->m_ViasDimensionsList[ii].m_Drill );
    }

    // Skip the first item, which is the current netclass value
    for( unsigned ii = 1; ii < m_BrdSettings->m_DiffPairDimensionsList.size(); ii++ )
    {
        AppendDiffPairs( m_BrdSettings->m_DiffPairDimensionsList[ii].m_Width,
                         m_BrdSettings->m_DiffPairDimensionsList[ii].m_Gap,
                         m_BrdSettings->m_DiffPairDimensionsList[ii].m_ViaGap );
    }

    return true;
}


bool PANEL_SETUP_TRACKS_AND_VIAS::TransferDataFromWindow()
{
    if( !commitPendingChanges() )
        return false;

    std::vector<int>                 trackWidths;
    std::vector<VIA_DIMENSION>       vias;
    std::vector<DIFF_PAIR_DIMENSION> diffPairs;

    // Test ONLY for malformed data.  Design rules and constraints are the business of DRC.

    for( int row = 0; row < m_trackWidthsGrid->GetNumberRows();  ++row )
    {
        if( !m_trackWidthsGrid->GetCellValue( row, TR_WIDTH_COL ).IsEmpty() )
            trackWidths.push_back( m_trackWidthsGrid->GetUnitValue( row, TR_WIDTH_COL ) );
    }

    for( int row = 0; row < m_viaSizesGrid->GetNumberRows();  ++row )
    {
        if( !m_viaSizesGrid->GetCellValue( row, VIA_SIZE_COL ).IsEmpty() )
        {
            VIA_DIMENSION via_dim;
            via_dim.m_Diameter = m_viaSizesGrid->GetUnitValue( row, VIA_SIZE_COL );

            if( !m_viaSizesGrid->GetCellValue( row, VIA_DRILL_COL ).IsEmpty() )
                via_dim.m_Drill = m_viaSizesGrid->GetUnitValue( row, VIA_DRILL_COL );

            vias.push_back( via_dim );
        }
    }

    for( int row = 0; row < m_diffPairsGrid->GetNumberRows();  ++row )
    {
        if( !m_diffPairsGrid->GetCellValue( row, DP_WIDTH_COL ).IsEmpty() )
        {
            DIFF_PAIR_DIMENSION diffPair_dim;
            diffPair_dim.m_Width = m_diffPairsGrid->GetUnitValue( row, DP_WIDTH_COL );

            if( !m_diffPairsGrid->GetCellValue( row, DP_GAP_COL ).IsEmpty() )
                diffPair_dim.m_Gap = m_diffPairsGrid->GetUnitValue( row, DP_GAP_COL );

            if( !m_diffPairsGrid->GetCellValue( row, DP_VIA_GAP_COL ).IsEmpty() )
                diffPair_dim.m_ViaGap = m_diffPairsGrid->GetUnitValue( row, DP_VIA_GAP_COL );

            diffPairs.push_back( diffPair_dim );
        }
    }

    // Sort lists by increasing value
    sort( trackWidths.begin(), trackWidths.end() );
    sort( vias.begin(), vias.end() );
    sort( diffPairs.begin(), diffPairs.end() );

    // These are all stored in project file, not board, so no need for OnModify()

    trackWidths.insert( trackWidths.begin(), 0 );         // dummy value for "use netclass"
    m_BrdSettings->m_TrackWidthList = trackWidths;

    vias.insert( vias.begin(), { 0, 0 } );                // dummy value for "use netclass"
    m_BrdSettings->m_ViasDimensionsList = vias;

    diffPairs.insert( diffPairs.begin(), { 0, 0, 0 } );   // dummy value for "use netclass"
    m_BrdSettings->m_DiffPairDimensionsList = diffPairs;

    return true;
}


bool PANEL_SETUP_TRACKS_AND_VIAS::commitPendingChanges( bool aQuietMode )
{
    return m_trackWidthsGrid->CommitPendingChanges( aQuietMode )
                && m_viaSizesGrid->CommitPendingChanges( aQuietMode )
                && m_diffPairsGrid->CommitPendingChanges( aQuietMode );
}


bool PANEL_SETUP_TRACKS_AND_VIAS::Validate()
{
    if( !commitPendingChanges() )
        return false;

    wxString msg;

    // Test vias
    for( int row = 0; row < m_viaSizesGrid->GetNumberRows();  ++row )
    {
        wxString viaDia = m_viaSizesGrid->GetCellValue( row, VIA_SIZE_COL );
        wxString viaDrill = m_viaSizesGrid->GetCellValue( row, VIA_DRILL_COL );

        std::optional<int> viaDiameter;

        if( !viaDia.IsEmpty() )
            viaDiameter = m_viaSizesGrid->GetUnitValue( row, VIA_SIZE_COL );

        std::optional<int> viaDrillSize;

        if( !viaDrill.IsEmpty() )
            viaDrillSize = m_viaSizesGrid->GetUnitValue( row, VIA_DRILL_COL );

        if( std::optional<PCB_VIA::VIA_PARAMETER_ERROR> error =
                    PCB_VIA::ValidateViaParameters( viaDiameter, viaDrillSize,
                                                    std::nullopt, // primary start layer
                                                    std::nullopt, // primary end layer
                                                    std::nullopt, // secondary drill
                                                    std::nullopt, // secondary start layer
                                                    std::nullopt, // secondary end layer
                                                    std::nullopt, // tertiary drill
                                                    std::nullopt, // tertiary start layer
                                                    std::nullopt, // tertiary end layer
                                                    m_Pcb ? m_Pcb->GetCopperLayerCount() : 0 ) )
        {
            msg = error->m_Message;

            int errorCol = VIA_SIZE_COL;

            if( error->m_Field == PCB_VIA::VIA_PARAMETER_ERROR::FIELD::DRILL )
                errorCol = VIA_DRILL_COL;

            PAGED_DIALOG::GetDialog( this )->SetError( msg, this, m_viaSizesGrid, row, errorCol );
            return false;
        }
    }

    // Test diff pairs
    for( int row = 0; row < m_diffPairsGrid->GetNumberRows();  ++row )
    {
        wxString dpWidth = m_diffPairsGrid->GetCellValue( row, 0 );
        wxString dpGap = m_diffPairsGrid->GetCellValue( row, 1 );

        if( !dpWidth.IsEmpty() && dpGap.IsEmpty() )
        {
            msg = _( "No differential pair gap defined." );
            PAGED_DIALOG::GetDialog( this )->SetError( msg, this, m_diffPairsGrid, row, 1 );
            return false;
        }
    }

    return true;
}


void PANEL_SETUP_TRACKS_AND_VIAS::AppendTrackWidth( const int aWidth )
{
    int i = m_trackWidthsGrid->GetNumberRows();

    m_trackWidthsGrid->AppendRows( 1 );

    m_trackWidthsGrid->SetUnitValue( i, TR_WIDTH_COL, aWidth );
}


void PANEL_SETUP_TRACKS_AND_VIAS::AppendViaSize( int aSize, int aDrill )
{
    int i = m_viaSizesGrid->GetNumberRows();

    m_viaSizesGrid->AppendRows( 1 );

    m_viaSizesGrid->SetUnitValue( i, VIA_SIZE_COL, aSize );

    if( aDrill > 0 )
        m_viaSizesGrid->SetUnitValue( i, VIA_DRILL_COL, aDrill );
}


void PANEL_SETUP_TRACKS_AND_VIAS::AppendDiffPairs( int aWidth, int aGap, int aViaGap )
{
    int i = m_diffPairsGrid->GetNumberRows();

    m_diffPairsGrid->AppendRows( 1 );

    m_diffPairsGrid->SetUnitValue( i, DP_WIDTH_COL, aWidth );

    if( aGap > 0 )
        m_diffPairsGrid->SetUnitValue( i, DP_GAP_COL, aGap );

    if( aViaGap > 0 )
        m_diffPairsGrid->SetUnitValue( i, DP_VIA_GAP_COL, aViaGap );
}


void PANEL_SETUP_TRACKS_AND_VIAS::OnAddTrackWidthsClick( wxCommandEvent& aEvent )
{
    m_trackWidthsGrid->OnAddRow(
            [&]() -> std::pair<int, int>
            {
                AppendTrackWidth( 0 );
                return { m_trackWidthsGrid->GetNumberRows() - 1, TR_WIDTH_COL };
            } );
}


void PANEL_SETUP_TRACKS_AND_VIAS::OnRemoveTrackWidthsClick( wxCommandEvent& event )
{
    m_trackWidthsGrid->OnDeleteRows(
            [&]( int row )
            {
                m_trackWidthsGrid->DeleteRows( row, 1 );
            } );
}


void PANEL_SETUP_TRACKS_AND_VIAS::OnAddViaSizesClick( wxCommandEvent& event )
{
    m_viaSizesGrid->OnAddRow(
            [&]() -> std::pair<int, int>
            {
                AppendViaSize( 0, 0 );
                return { m_viaSizesGrid->GetNumberRows() - 1, VIA_SIZE_COL };
            } );
}


void PANEL_SETUP_TRACKS_AND_VIAS::OnRemoveViaSizesClick( wxCommandEvent& event )
{
    m_viaSizesGrid->OnDeleteRows(
            [&]( int row )
            {
                m_viaSizesGrid->DeleteRows( row, 1 );
            } );
}


void PANEL_SETUP_TRACKS_AND_VIAS::OnAddDiffPairsClick( wxCommandEvent& event )
{
    m_diffPairsGrid->OnAddRow(
            [&]() -> std::pair<int, int>
            {
                AppendDiffPairs( 0, 0, 0 );
                return { m_diffPairsGrid->GetNumberRows() - 1, DP_WIDTH_COL };
            } );
}


void PANEL_SETUP_TRACKS_AND_VIAS::OnRemoveDiffPairsClick( wxCommandEvent& event )
{
    m_diffPairsGrid->OnDeleteRows(
            [&]( int row )
            {
                m_diffPairsGrid->DeleteRows( row, 1 );
            } );
}


void PANEL_SETUP_TRACKS_AND_VIAS::ImportSettingsFrom( BOARD* aBoard )
{
    commitPendingChanges( true );

    // Note: do not change the board, as we need to get the current nets from it for
    // netclass memberships.  All the netclass definitions and dimension lists are in
    // the BOARD_DESIGN_SETTINGS.

    BOARD_DESIGN_SETTINGS* savedSettings = m_BrdSettings;

    m_BrdSettings = &aBoard->GetDesignSettings();
    TransferDataToWindow();

    m_BrdSettings = savedSettings;
}

