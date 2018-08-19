/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2018 KiCad Developers, see change_log.txt for contributors.
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
#include <class_drawpanel.h>
#include <base_units.h>
#include <pcb_edit_frame.h>
#include <board_design_settings.h>
#include <widgets/wx_grid.h>

#include <panel_setup_tracks_and_vias.h>


PANEL_SETUP_TRACKS_AND_VIAS::PANEL_SETUP_TRACKS_AND_VIAS(
                                          PAGED_DIALOG* aParent, PCB_EDIT_FRAME* aFrame,
                                          PANEL_SETUP_FEATURE_CONSTRAINTS* aConstraintsPanel ) :
    PANEL_SETUP_TRACKS_AND_VIAS_BASE( aParent->GetTreebook() )
{
    m_Parent = aParent;
    m_Frame = aFrame;
    m_Pcb = m_Frame->GetBoard();
    m_BrdSettings = &m_Pcb->GetDesignSettings();
    m_ConstraintsPanel = aConstraintsPanel;

    // Membership combobox editors require a bit more room, so increase the row size of
    // all our grids for consistency
    m_trackWidthsGrid->SetDefaultRowSize( m_trackWidthsGrid->GetDefaultRowSize() + 4 );
    m_viaSizesGrid->SetDefaultRowSize(    m_viaSizesGrid->GetDefaultRowSize()    + 4 );
    m_diffPairsGrid->SetDefaultRowSize(   m_diffPairsGrid->GetDefaultRowSize()   + 4 );
}


bool PANEL_SETUP_TRACKS_AND_VIAS::TransferDataToWindow()
{
#define SETCELL( grid, row, col, val ) \
    grid->SetCellValue( row, col, StringFromValue( m_Frame->GetUserUnits(), val, true, true ) )

    m_trackWidthsGrid->ClearGrid();
    m_viaSizesGrid->ClearGrid();
    m_diffPairsGrid->ClearGrid();

    // Skip the first item, which is the current netclass value
    for( unsigned ii = 1; ii < m_BrdSettings->m_TrackWidthList.size(); ii++ )
    {
        SETCELL( m_trackWidthsGrid, ii-1, 0, m_BrdSettings->m_TrackWidthList[ii] );
    }

    // Skip the first item, which is the current netclass value
    for( unsigned ii = 1; ii < m_BrdSettings->m_ViasDimensionsList.size(); ii++ )
    {
        SETCELL( m_viaSizesGrid, ii-1, 0, m_BrdSettings->m_ViasDimensionsList[ii].m_Diameter );

        if( m_BrdSettings->m_ViasDimensionsList[ii].m_Drill > 0 )
            SETCELL( m_viaSizesGrid, ii-1, 1, m_BrdSettings->m_ViasDimensionsList[ii].m_Drill );
    }

    // Skip the first item, which is the current netclass value
    for( unsigned ii = 1; ii < m_BrdSettings->m_DiffPairDimensionsList.size(); ii++ )
    {
        SETCELL( m_diffPairsGrid, ii-1, 0, m_BrdSettings->m_DiffPairDimensionsList[ii].m_Width );

        if( m_BrdSettings->m_DiffPairDimensionsList[ii].m_Gap > 0 )
            SETCELL( m_diffPairsGrid, ii-1, 1, m_BrdSettings->m_DiffPairDimensionsList[ii].m_Gap );

        if( m_BrdSettings->m_DiffPairDimensionsList[ii].m_ViaGap > 0 )
            SETCELL( m_diffPairsGrid, ii-1, 2, m_BrdSettings->m_DiffPairDimensionsList[ii].m_ViaGap );
    }

    return true;
}


bool PANEL_SETUP_TRACKS_AND_VIAS::TransferDataFromWindow()
{
    if( !validateData() )
        return false;

    wxString                         msg;
    std::vector<int>                 trackWidths;
    std::vector<VIA_DIMENSION>       vias;
    std::vector<DIFF_PAIR_DIMENSION> diffPairs;

    for( int row = 0; row < m_trackWidthsGrid->GetNumberRows();  ++row )
    {
        msg = m_trackWidthsGrid->GetCellValue( row, 0 );

        if( !msg.IsEmpty() )
            trackWidths.push_back( ValueFromString( m_Frame->GetUserUnits(), msg, true ) );
    }

    for( int row = 0; row < m_viaSizesGrid->GetNumberRows();  ++row )
    {
        msg = m_viaSizesGrid->GetCellValue( row, 0 );

        if( !msg.IsEmpty() )
        {
            VIA_DIMENSION via_dim;
            via_dim.m_Diameter = ValueFromString( m_Frame->GetUserUnits(), msg, true );

            msg = m_viaSizesGrid->GetCellValue( row, 1 );

            if( !msg.IsEmpty() )
                via_dim.m_Drill = ValueFromString( m_Frame->GetUserUnits(), msg, true );

            vias.push_back( via_dim );
        }
    }

    for( int row = 0; row < m_viaSizesGrid->GetNumberRows();  ++row )
    {
        msg = m_diffPairsGrid->GetCellValue( row, 0 );

        if( !msg.IsEmpty() )
        {
            DIFF_PAIR_DIMENSION diffPair_dim;
            diffPair_dim.m_Width = ValueFromString( m_Frame->GetUserUnits(), msg, true );

            msg = m_diffPairsGrid->GetCellValue( row, 1 );
            diffPair_dim.m_Gap = ValueFromString( m_Frame->GetUserUnits(), msg, true );

            msg = m_diffPairsGrid->GetCellValue( row, 2 );

            if( !msg.IsEmpty() )
                diffPair_dim.m_ViaGap = ValueFromString( m_Frame->GetUserUnits(), msg, true );

            diffPairs.push_back( diffPair_dim );
        }
    }

    // Sort lists by increasing value
    sort( trackWidths.begin(), trackWidths.end() );
    sort( vias.begin(), vias.end() );
    sort( diffPairs.begin(), diffPairs.end() );

    trackWidths.insert( trackWidths.begin(), m_BrdSettings->m_TrackWidthList[ 0 ] );
    m_BrdSettings->m_TrackWidthList = trackWidths;

    vias.insert( vias.begin(), m_BrdSettings->m_ViasDimensionsList[ 0 ] );
    m_BrdSettings->m_ViasDimensionsList = vias;

    diffPairs.insert( diffPairs.begin(), m_BrdSettings->m_DiffPairDimensionsList[ 0 ] );
    m_BrdSettings->m_DiffPairDimensionsList = diffPairs;

    return true;
}


bool PANEL_SETUP_TRACKS_AND_VIAS::validateData()
{
    if( !m_trackWidthsGrid->CommitPendingChanges()
            || !m_viaSizesGrid->CommitPendingChanges()
            || !m_diffPairsGrid->CommitPendingChanges() )
        return false;

    wxString msg;
    int minViaDia = m_ConstraintsPanel->m_viaMinSize.GetValue();
    int minViaDrill = m_ConstraintsPanel->m_viaMinDrill.GetValue();
    int minTrackWidth = m_ConstraintsPanel->m_trackMinWidth.GetValue();

    // Test tracks
    for( int row = 0; row < m_trackWidthsGrid->GetNumberRows();  ++row )
    {
        wxString tvalue = m_trackWidthsGrid->GetCellValue( row, 0 );

        if( tvalue.IsEmpty() )
            continue;

        if( ValueFromString( m_Frame->GetUserUnits(), tvalue ) < minTrackWidth )
        {
            msg.Printf( _( "Track width less than minimum track width (%s)." ),
                        StringFromValue( m_Frame->GetUserUnits(), minTrackWidth, true, true ) );
            m_Parent->SetError( msg, this, m_trackWidthsGrid, row, 0 );
            return false;
        }
    }

    // Test vias
    for( int row = 0; row < m_viaSizesGrid->GetNumberRows();  ++row )
    {
        wxString viaDia = m_viaSizesGrid->GetCellValue( row, 0 );

        if( viaDia.IsEmpty() )
            continue;

        if( ValueFromString( m_Frame->GetUserUnits(), viaDia ) < minViaDia )
        {
            msg.Printf( _( "Via diameter less than minimum via diameter (%s)." ),
                        StringFromValue( m_Frame->GetUserUnits(), minViaDia, true, true ) );
            m_Parent->SetError( msg, this, m_viaSizesGrid, row, 0 );
            return false;
        }

        wxString viaDrill = m_viaSizesGrid->GetCellValue( row, 1 );

        if( viaDrill.IsEmpty() )
        {
            msg = _( "No via drill defined." );
            m_Parent->SetError( msg, this, m_viaSizesGrid, row, 1 );
            return false;
        }

        if( ValueFromString( m_Frame->GetUserUnits(), viaDrill ) < minViaDrill )
        {
            msg.Printf( _( "Via drill less than minimum via drill (%s)." ),
                        StringFromValue( m_Frame->GetUserUnits(), minViaDrill, true, true ) );
            m_Parent->SetError( msg, this, m_viaSizesGrid, row, 1 );
            return false;
        }

        if( ValueFromString( m_Frame->GetUserUnits(), viaDrill )
                >= ValueFromString( m_Frame->GetUserUnits(), viaDia ) )
        {
            msg = _( "Via drill larger than via diameter." );
            m_Parent->SetError( msg, this, m_viaSizesGrid, row, 1 );
            return false;
        }
    }

    return true;
}


void PANEL_SETUP_TRACKS_AND_VIAS::ImportSettingsFrom( BOARD* aBoard )
{
    m_trackWidthsGrid->CommitPendingChanges( true );
    m_viaSizesGrid->CommitPendingChanges( true );
    m_diffPairsGrid->CommitPendingChanges( true );

    // Note: do not change the board, as we need to get the current nets from it for
    // netclass memberships.  All the netclass definitions and dimension lists are in
    // the BOARD_DESIGN_SETTINGS.

    BOARD_DESIGN_SETTINGS* savedSettings = m_BrdSettings;

    m_BrdSettings = &aBoard->GetDesignSettings();
    TransferDataToWindow();

    m_BrdSettings = savedSettings;
}


