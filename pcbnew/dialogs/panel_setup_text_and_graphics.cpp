/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2018-2021 KiCad Developers, see change_log.txt for contributors.
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

#include <base_units.h>
#include <pcb_edit_frame.h>
#include <board_design_settings.h>
#include <widgets/wx_grid.h>
#include <grid_tricks.h>
#include <panel_setup_text_and_graphics.h>


// Columns of layer classes grid
enum
{
    COL_LINE_THICKNESS = 0,
    COL_TEXT_WIDTH,
    COL_TEXT_HEIGHT,
    COL_TEXT_THICKNESS,
    COL_TEXT_ITALIC,
    COL_TEXT_UPRIGHT
};

enum
{
    ROW_SILK = 0,
    ROW_COPPER,
    ROW_EDGES,
    ROW_COURTYARD,
    ROW_FAB,
    ROW_OTHERS,

    ROW_COUNT
};


PANEL_SETUP_TEXT_AND_GRAPHICS::PANEL_SETUP_TEXT_AND_GRAPHICS( PAGED_DIALOG* aParent,
                                                              PCB_EDIT_FRAME* aFrame ) :
        PANEL_SETUP_TEXT_AND_GRAPHICS_BASE( aParent->GetTreebook() ),
        m_arrowLength( aFrame, m_lblArrowLength, m_dimensionArrowLength, m_arrowLengthUnits ),
        m_extensionOffset( aFrame, m_lblExtensionOffset, m_dimensionExtensionOffset,
                           m_dimensionExtensionOffsetUnits )
{
    m_Parent = aParent;
    m_Frame = aFrame;
    m_BrdSettings = &m_Frame->GetBoard()->GetDesignSettings();

    m_grid->SetDefaultRowSize( m_grid->GetDefaultRowSize() + 4 );

    // Work around a bug in wxWidgets where it fails to recalculate the grid height
    // after changing the default row size
    m_grid->AppendRows( 1 );
    m_grid->DeleteRows( m_grid->GetNumberRows() - 1, 1 );

    m_grid->PushEventHandler( new GRID_TRICKS( m_grid ) );
}


PANEL_SETUP_TEXT_AND_GRAPHICS::~PANEL_SETUP_TEXT_AND_GRAPHICS()
{
    // destroy GRID_TRICKS before m_grid.
    m_grid->PopEventHandler( true );
}


bool PANEL_SETUP_TEXT_AND_GRAPHICS::TransferDataToWindow()
{
    wxColour disabledColour = wxSystemSettings::GetColour( wxSYS_COLOUR_BACKGROUND );

#define SET_MILS_CELL( row, col, val ) \
    m_grid->SetCellValue( row, col, StringFromValue( m_Frame->GetUserUnits(), val, true ) )

#define DISABLE_CELL( row, col ) \
    m_grid->SetReadOnly( row, col ); m_grid->SetCellBackgroundColour( row, col, disabledColour );

    for( int i = 0; i < ROW_COUNT; ++i )
    {
        SET_MILS_CELL( i, COL_LINE_THICKNESS, m_BrdSettings->m_LineThickness[ i ] );

        if( i == ROW_EDGES || i == ROW_COURTYARD )
        {
            DISABLE_CELL( i, COL_TEXT_WIDTH );
            DISABLE_CELL( i, COL_TEXT_HEIGHT );
            DISABLE_CELL( i, COL_TEXT_THICKNESS );
            DISABLE_CELL( i, COL_TEXT_ITALIC );
            DISABLE_CELL( i, COL_TEXT_UPRIGHT );
        }
        else
        {
            SET_MILS_CELL( i, COL_TEXT_WIDTH, m_BrdSettings->m_TextSize[ i ].x );
            SET_MILS_CELL( i, COL_TEXT_HEIGHT, m_BrdSettings->m_TextSize[ i ].y );
            SET_MILS_CELL( i, COL_TEXT_THICKNESS, m_BrdSettings->m_TextThickness[ i ] );
            m_grid->SetCellValue( i, COL_TEXT_ITALIC, m_BrdSettings->m_TextItalic[ i ] ? "1" : "" );
            m_grid->SetCellValue( i, COL_TEXT_UPRIGHT, m_BrdSettings->m_TextUpright[ i ] ? "1" : "" );

            auto attr = new wxGridCellAttr;
            attr->SetRenderer( new wxGridCellBoolRenderer() );
            attr->SetReadOnly();    // not really; we delegate interactivity to GRID_TRICKS
            attr->SetAlignment( wxALIGN_CENTER, wxALIGN_CENTER );
            m_grid->SetAttr( i, COL_TEXT_ITALIC, attr );

            attr = new wxGridCellAttr;
            attr->SetRenderer( new wxGridCellBoolRenderer() );
            attr->SetReadOnly();    // not really; we delegate interactivity to GRID_TRICKS
            attr->SetAlignment( wxALIGN_CENTER, wxALIGN_CENTER );
            m_grid->SetAttr( i, COL_TEXT_UPRIGHT, attr );
        }
    }

    // Work around an issue where wxWidgets doesn't calculate the row width on its own
    for( int col = 0; col < m_grid->GetNumberCols(); col++ )
        m_grid->SetColMinimalWidth( col, m_grid->GetVisibleWidth( col, true, true, false ) );

    m_grid->SetRowLabelSize( m_grid->GetVisibleWidth( -1, true, true, true ) );

    Layout();

    wxASSERT_MSG( m_BrdSettings->m_DimensionPrecision <= 4, "Unhandled dimension precision!" );

    int mode = static_cast<int>( m_BrdSettings->m_DimensionUnitsMode );
    m_dimensionUnits->SetSelection( mode );

    int format = static_cast<int>( m_BrdSettings->m_DimensionUnitsFormat );
    m_dimensionUnitsFormat->SetSelection( format );

    m_dimensionPrecision->SetSelection( m_BrdSettings->m_DimensionPrecision );
    m_dimensionSuppressZeroes->SetValue( m_BrdSettings->m_DimensionSuppressZeroes );

    int position = static_cast<int>( m_BrdSettings->m_DimensionTextPosition );
    m_dimensionTextPositionMode->SetSelection( position );

    m_dimensionTextKeepAligned->SetValue( m_BrdSettings->m_DimensionKeepTextAligned );
    m_arrowLength.SetValue( m_BrdSettings->m_DimensionArrowLength );
    m_extensionOffset.SetValue( m_BrdSettings->m_DimensionExtensionOffset );

    return true;
}


int PANEL_SETUP_TEXT_AND_GRAPHICS::getGridValue( int aRow, int aCol )
{
    return ValueFromString( m_Frame->GetUserUnits(), m_grid->GetCellValue( aRow, aCol ) );
}


bool PANEL_SETUP_TEXT_AND_GRAPHICS::TransferDataFromWindow()
{
    if( !m_grid->CommitPendingChanges() )
        return false;

    for( int i = 0; i < ROW_COUNT; ++i )
    {
        m_BrdSettings->m_LineThickness[ i ] = getGridValue( i, COL_LINE_THICKNESS );

        if( i == ROW_EDGES || i == ROW_COURTYARD )
            continue;

        m_BrdSettings->m_TextSize[ i ] = wxSize( getGridValue( i, COL_TEXT_WIDTH ),
                                                 getGridValue( i, COL_TEXT_HEIGHT ) );
        m_BrdSettings->m_TextThickness[ i ] = getGridValue( i, COL_TEXT_THICKNESS );
        m_BrdSettings->m_TextItalic[ i ] =
                wxGridCellBoolEditor::IsTrueValue( m_grid->GetCellValue( i, COL_TEXT_ITALIC ) );
        m_BrdSettings->m_TextUpright[ i ] =
                wxGridCellBoolEditor::IsTrueValue( m_grid->GetCellValue( i, COL_TEXT_UPRIGHT ) );
    }

    // These are all stored in project file, not board, so no need for OnModify()

    int mode                                  = m_dimensionUnits->GetSelection();
    m_BrdSettings->m_DimensionUnitsMode       = static_cast<DIM_UNITS_MODE>( mode );
    int format                                = m_dimensionUnitsFormat->GetSelection();
    m_BrdSettings->m_DimensionUnitsFormat     = static_cast<DIM_UNITS_FORMAT>( format );
    m_BrdSettings->m_DimensionPrecision       = m_dimensionPrecision->GetSelection();
    m_BrdSettings->m_DimensionSuppressZeroes  = m_dimensionSuppressZeroes->GetValue();
    int position                              = m_dimensionTextPositionMode->GetSelection();
    m_BrdSettings->m_DimensionTextPosition    = static_cast<DIM_TEXT_POSITION>( position );
    m_BrdSettings->m_DimensionKeepTextAligned = m_dimensionTextKeepAligned->GetValue();
    m_BrdSettings->m_DimensionArrowLength     = m_arrowLength.GetValue();
    m_BrdSettings->m_DimensionExtensionOffset = m_extensionOffset.GetValue();

    return true;
}


void PANEL_SETUP_TEXT_AND_GRAPHICS::ImportSettingsFrom( BOARD* aBoard )
{
    if( !m_grid->CommitPendingChanges() )
        return;

    BOARD_DESIGN_SETTINGS* savedSettings = m_BrdSettings;

    m_BrdSettings = &aBoard->GetDesignSettings();
    TransferDataToWindow();

    m_BrdSettings = savedSettings;
}
