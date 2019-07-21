/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019 Alexander Shuklin, jasuramme@gmail.com
 * Copyright (C) 2019 KiCad Developers, see AUTHORS.txt for contributors.
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


#include "dialog_board_statistics.h"


#define COL_FRONT_SIDE 1
#define COL_BOTTOM_SIDE 2
#define COL_TOTAL 3

DIALOG_BOARD_STATISTICS::DIALOG_BOARD_STATISTICS( PCB_EDIT_FRAME* aParentFrame )
        : DIALOG_BOARD_STATISTICS_BASE( aParentFrame )
{
    m_parentFrame = aParentFrame;

    // Remove wxgrid's selection boxes
    m_gridComponents->SetCellHighlightPenWidth( 0 );
    m_gridPads->SetCellHighlightPenWidth( 0 );
    m_gridBoard->SetCellHighlightPenWidth( 0 );
    m_gridComponents->SetColMinimalAcceptableWidth( 80 );
    m_gridPads->SetColMinimalAcceptableWidth( 80 );
    m_gridBoard->SetColMinimalAcceptableWidth( 80 );

    // Make labels for grids
    wxFont headingFont = wxSystemSettings::GetFont( wxSYS_DEFAULT_GUI_FONT );
    headingFont.SetSymbolicSize( wxFONTSIZE_SMALL );
    m_gridComponents->SetCellValue( 0, COL_FRONT_SIDE, _( "Front Side" ) );
    m_gridComponents->SetCellFont( 0, COL_FRONT_SIDE, headingFont );
    m_gridComponents->SetCellValue( 0, COL_BOTTOM_SIDE, _( "Bottom Side" ) );
    m_gridComponents->SetCellFont( 0, COL_BOTTOM_SIDE, headingFont );
    m_gridComponents->SetCellValue( 0, COL_TOTAL, _( "Total" ) );
    m_gridComponents->SetCellFont( 0, COL_TOTAL, headingFont );

    m_gridComponents->SetCellAlignment( 0, 0, wxALIGN_LEFT, wxALIGN_CENTRE );
    m_gridComponents->SetCellAlignment( 1, 0, wxALIGN_LEFT, wxALIGN_CENTRE );
    m_gridComponents->SetCellAlignment( 2, 0, wxALIGN_LEFT, wxALIGN_CENTRE );
    m_gridComponents->SetCellAlignment( 3, 0, wxALIGN_LEFT, wxALIGN_CENTRE );
    m_gridComponents->SetCellAlignment( 4, 0, wxALIGN_LEFT, wxALIGN_CENTRE );

    m_gridPads->SetCellAlignment( 0, 0, wxALIGN_LEFT, wxALIGN_CENTRE );
    m_gridPads->SetCellAlignment( 1, 0, wxALIGN_LEFT, wxALIGN_CENTRE );
    m_gridPads->SetCellAlignment( 2, 0, wxALIGN_LEFT, wxALIGN_CENTRE );
    m_gridPads->SetCellAlignment( 3, 0, wxALIGN_LEFT, wxALIGN_CENTRE );
    m_gridPads->SetCellAlignment( 4, 0, wxALIGN_LEFT, wxALIGN_CENTRE );

    m_gridBoard->SetCellValue( 0, 0, _( "Width:" ) );
    m_gridBoard->SetCellAlignment( 0, 0, wxALIGN_LEFT, wxALIGN_CENTRE );
    m_gridBoard->SetCellValue( 1, 0, _( "Height:" ) );
    m_gridBoard->SetCellAlignment( 1, 0, wxALIGN_LEFT, wxALIGN_CENTRE );
    m_gridBoard->SetCellValue( 2, 0, _( "Area:" ) );
    m_gridBoard->SetCellAlignment( 2, 0, wxALIGN_LEFT, wxALIGN_CENTRE );
}

void DIALOG_BOARD_STATISTICS::refreshItemsTypes()
{
    m_componentsTypes.clear();

    // If you need some more types to be shown, simply add them to the
    // corresponding list
    m_componentsTypes.push_back( componentsType_t( MOD_DEFAULT, _( "THT:" ) ) );
    m_componentsTypes.push_back( componentsType_t( MOD_CMS, _( "SMD:" ) ) );
    m_componentsTypes.push_back( componentsType_t( MOD_VIRTUAL, _( "Virtual:" ) ) );

    m_padsTypes.clear();
    m_padsTypes.push_back( padsType_t( PAD_ATTRIB_STANDARD, _( "Through hole:" ) ) );
    m_padsTypes.push_back( padsType_t( PAD_ATTRIB_SMD, _( "SMD:" ) ) );
    m_padsTypes.push_back( padsType_t( PAD_ATTRIB_CONN, _( "Connector:" ) ) );
    m_padsTypes.push_back( padsType_t( PAD_ATTRIB_HOLE_NOT_PLATED, _( "NPTH:" ) ) );

    // If there not enough rows in grids, append some
    size_t appendRows = m_componentsTypes.size() + 2 - m_gridComponents->GetNumberRows();

    if( appendRows > 0 )
        m_gridComponents->AppendRows( appendRows );

    appendRows = m_padsTypes.size() + 1 - m_gridPads->GetNumberRows();

    if( appendRows > 0 )
        m_gridPads->AppendRows( appendRows );
}

bool DIALOG_BOARD_STATISTICS::TransferDataToWindow()
{
    refreshItemsTypes();
    getDataFromPCB();
    updateWidets();
    Layout();
    FinishDialogSettings();
    return true;
}

void DIALOG_BOARD_STATISTICS::getDataFromPCB()
{
    auto board = m_parentFrame->GetBoard();

    for( MODULE* module : board->Modules() )
    {
        auto& pads = module->Pads();

        // Do not proceed modules with no pads if checkbox checked
        if( m_checkBoxExcludeComponentsNoPins->GetValue() && !pads.size() )
            continue;

        // Go through components types list
        for( auto& type : m_componentsTypes )
        {
            if( module->GetAttributes() == type.attribute )
            {
                if( module->IsFlipped() )
                    type.backSideQty++;
                else
                    type.frontSideQty++;
                break;
            }
        }

        for( auto& pad : pads )
        {
            // Go through pads types list
            for( auto& type : m_padsTypes )
            {
                if( pad->GetAttribute() == type.attribute )
                {
                    type.qty++;
                    break;
                }
            }
        }
    }

    bool           boundingBoxCreated = false; //flag if bounding box initialized
    BOX2I          bbox;
    SHAPE_POLY_SET polySet;
    m_hasOutline = board->GetBoardPolygonOutlines( polySet );

    // If board has no Edge Cuts lines, board->GetBoardPolygonOutlines will
    // return small rectangle, so we double check that
    bool edgeCutsExists = false;
    for( auto& drawing : board->Drawings() )
    {
        if( drawing->GetLayer() == Edge_Cuts )
        {
            edgeCutsExists = true;
            break;
        }
    }

    if( !edgeCutsExists )
        m_hasOutline = false;

    if( m_hasOutline )
    {
        m_boardArea = 0;
        int outlinesCount = polySet.OutlineCount();
        for( int i = 0; i < outlinesCount; i++ )
        {
            auto& outline = polySet.Outline( i );
            m_boardArea += abs( outline.Area() );

            // If checkbox "subtract holes" is checked
            if( m_checkBoxSubtractHoles->GetValue() )
            {
                int holesCount = polySet.HoleCount( i );
                for( int j = 0; j < holesCount; j++ )
                {
                    m_boardArea -= abs( polySet.Hole( i, j ).Area() );
                }
            }

            if( boundingBoxCreated )
            {
                bbox.Merge( outline.BBox() );
            }
            else
            {
                bbox = outline.BBox();
                boundingBoxCreated = true;
            }
        }
        m_boardWidth = bbox.GetWidth();
        m_boardHeight = bbox.GetHeight();
    }
}

void DIALOG_BOARD_STATISTICS::updateWidets()
{
    int totalPads = 0;
    int currentRow = 0;
    for( auto& type : m_padsTypes )
    {
        m_gridPads->SetCellValue( currentRow, 0, type.title );
        m_gridPads->SetCellValue( currentRow, 1, wxString::Format( wxT( "%i " ), type.qty ) );
        totalPads += type.qty;
        currentRow++;
    }
    m_gridPads->SetCellValue( currentRow, 0, _( "Total" ) );
    m_gridPads->SetCellValue( currentRow, 1, wxString::Format( wxT( "%i " ), totalPads ) );

    int totalFront = 0;
    int totalBack = 0;

    // We don't use row 0, as there labels are
    currentRow = 1;
    for( auto& type : m_componentsTypes )
    {
        m_gridComponents->SetCellValue( currentRow, 0, type.title );
        m_gridComponents->SetCellValue(
                currentRow, COL_FRONT_SIDE, wxString::Format( wxT( "%i " ), type.frontSideQty ) );
        m_gridComponents->SetCellValue(
                currentRow, COL_BOTTOM_SIDE, wxString::Format( wxT( "%i " ), type.backSideQty ) );
        m_gridComponents->SetCellValue( currentRow, 3,
                wxString::Format( wxT( "%i " ), type.frontSideQty + type.backSideQty ) );
        totalFront += type.frontSideQty;
        totalBack += type.backSideQty;
        currentRow++;
    }
    m_gridComponents->SetCellValue( currentRow, 0, _( "Total" ) );
    m_gridComponents->SetCellValue( currentRow, COL_FRONT_SIDE,
                                    wxString::Format( wxT( "%i " ), totalFront ) );
    m_gridComponents->SetCellValue( currentRow, COL_BOTTOM_SIDE, wxString::Format( wxT( "%i " ), totalBack ) );
    m_gridComponents->SetCellValue(
            currentRow, COL_TOTAL, wxString::Format( wxT( "%i " ), totalFront + totalBack ) );

    if( m_hasOutline )
    {
        m_gridBoard->SetCellValue( 0, 1,
                MessageTextFromValue( m_parentFrame->GetUserUnits(), m_boardWidth, false ) + " " );
        m_gridBoard->SetCellValue( 1, 1,
                MessageTextFromValue( m_parentFrame->GetUserUnits(), m_boardHeight, false ) + " " );
        if( GetUserUnits() == INCHES )
            m_boardArea /= ( IU_PER_MILS * IU_PER_MILS * 1000000 );
        else
            m_boardArea /= ( IU_PER_MM * IU_PER_MM );
        m_gridBoard->SetCellValue( 2, 1,
                wxString::Format( wxT( "%.3f %s²" ), m_boardArea,
                        GetAbbreviatedUnitsLabel( GetUserUnits(), false ) ) );
    }
    else
    {
        m_gridBoard->SetCellValue( 0, 1, _( "unknown" ) );
        m_gridBoard->SetCellValue( 1, 1, _( "unknown" ) );
        m_gridBoard->SetCellValue( 2, 1, _( "unknown" ) );
    }

    m_gridComponents->AutoSize();
    m_gridPads->AutoSize();
    m_gridBoard->AutoSize();
}

// If any checkbox clicked, we have to refresh dialog data
void DIALOG_BOARD_STATISTICS::checkboxClicked( wxCommandEvent& event )
{
    TransferDataToWindow();
}

DIALOG_BOARD_STATISTICS::~DIALOG_BOARD_STATISTICS()
{
}
