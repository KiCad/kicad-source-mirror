/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 1992-2019 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <pcbnew.h>
#include <board_design_settings.h>
#include <widgets/paged_dialog.h>
#include <footprint_edit_frame.h>
#include <widgets/wx_grid.h>
#include <grid_tricks.h>

#include <panel_modedit_defaults.h>


// Columns of layer classes grid
enum
{
    COL_LINE_THICKNESS = 0,
    COL_TEXT_WIDTH,
    COL_TEXT_HEIGHT,
    COL_TEXT_THICKNESS,
    COL_TEXT_ITALIC
};

enum
{
    ROW_SILK = 0,
    ROW_COPPER,
    ROW_EDGES,
    ROW_COURTYARD,
    ROW_OTHERS,

    ROW_COUNT
};


PANEL_MODEDIT_DEFAULTS::PANEL_MODEDIT_DEFAULTS( FOOTPRINT_EDIT_FRAME* aFrame, PAGED_DIALOG* aParent) :
        PANEL_MODEDIT_DEFAULTS_BASE( aParent->GetTreebook() ),
        m_brdSettings( aFrame->GetDesignSettings() ),
        m_frame( aFrame ),
        m_Parent( aParent )
{
    m_grid->SetDefaultRowSize( m_grid->GetDefaultRowSize() + 4 );

    // Work around a bug in wxWidgets where it fails to recalculate the grid height
    // after changing the default row size
    m_grid->AppendRows( 1 );
    m_grid->DeleteRows( m_grid->GetNumberRows() - 1, 1 );

    m_grid->PushEventHandler( new GRID_TRICKS( m_grid ) );

    wxFont infoFont = wxSystemSettings::GetFont( wxSYS_DEFAULT_GUI_FONT );
    infoFont.SetSymbolicSize( wxFONTSIZE_SMALL );
    m_staticTextInfo->SetFont( infoFont );
}


PANEL_MODEDIT_DEFAULTS::~PANEL_MODEDIT_DEFAULTS()
{
    // destroy GRID_TRICKS before m_grid.
    m_grid->PopEventHandler( true );
}


bool PANEL_MODEDIT_DEFAULTS::TransferDataToWindow()
{
    wxColour disabledColour = wxSystemSettings::GetColour( wxSYS_COLOUR_BACKGROUND );

#define SET_MILS_CELL( row, col, val ) \
    m_grid->SetCellValue( row, col, StringFromValue( m_frame->GetUserUnits(), val, true, true ) )

#define DISABLE_CELL( row, col ) \
    m_grid->SetReadOnly( row, col ); m_grid->SetCellBackgroundColour( row, col, disabledColour );

    for( int i = 0; i < ROW_COUNT; ++i )
    {
        SET_MILS_CELL( i, COL_LINE_THICKNESS, m_brdSettings.m_LineThickness[ i ] );

        if( i == ROW_EDGES || i == ROW_COURTYARD )
        {
            DISABLE_CELL( i, COL_TEXT_WIDTH );
            DISABLE_CELL( i, COL_TEXT_HEIGHT );
            DISABLE_CELL( i, COL_TEXT_THICKNESS );
            DISABLE_CELL( i, COL_TEXT_ITALIC );
        }
        else
        {
            SET_MILS_CELL( i, COL_TEXT_WIDTH, m_brdSettings.m_TextSize[ i ].x );
            SET_MILS_CELL( i, COL_TEXT_HEIGHT, m_brdSettings.m_TextSize[ i ].y );
            SET_MILS_CELL( i, COL_TEXT_THICKNESS, m_brdSettings.m_TextThickness[ i ] );
            m_grid->SetCellValue( i, COL_TEXT_ITALIC, m_brdSettings.m_TextItalic[ i ] ? "1" : "" );

            auto attr = new wxGridCellAttr;
            attr->SetRenderer( new wxGridCellBoolRenderer() );
            attr->SetReadOnly();    // not really; we delegate interactivity to GRID_TRICKS
            attr->SetAlignment( wxALIGN_CENTER, wxALIGN_BOTTOM );
            m_grid->SetAttr( i, COL_TEXT_ITALIC, attr );
        }
    }

    // Footprint defaults
    m_textCtrlRefText->SetValue( m_brdSettings.m_RefDefaultText );
    m_choiceLayerReference->SetSelection( m_brdSettings.m_RefDefaultlayer == F_SilkS ? 0 : 1 );
    m_choiceVisibleReference->SetSelection( m_brdSettings.m_RefDefaultVisibility ? 0 : 1 );

    m_textCtrlValueText->SetValue( m_brdSettings.m_ValueDefaultText );
    m_choiceLayerValue->SetSelection( m_brdSettings.m_ValueDefaultlayer == F_SilkS ? 0 : 1 );
    m_choiceVisibleValue->SetSelection( m_brdSettings.m_ValueDefaultVisibility ? 0 : 1 );

    for( int col = 0; col < m_grid->GetNumberCols(); col++ )
    {
        // Set the minimal width to the column label size.
        m_grid->SetColMinimalWidth( col, m_grid->GetVisibleWidth( col, true, false, false ) );

        // Set the width to see the full contents
        if( m_grid->IsColShown( col ) )
            m_grid->SetColSize( col, m_grid->GetVisibleWidth( col, true, true, true ) );
    }

    m_grid->SetRowLabelSize( m_grid->GetVisibleWidth( -1, true, true, true ) );

    Layout();

    return true;
}


int PANEL_MODEDIT_DEFAULTS::getGridValue( int aRow, int aCol )
{
    return ValueFromString( m_frame->GetUserUnits(), m_grid->GetCellValue( aRow, aCol ), true );
}


bool PANEL_MODEDIT_DEFAULTS::validateData()
{
    if( !m_grid->CommitPendingChanges() )
        return false;

    // Test text parameters.
    for( int row : { ROW_SILK, ROW_COPPER, ROW_OTHERS } )
    {
        int textSize = std::min( getGridValue( row, COL_TEXT_WIDTH ),
                                 getGridValue( row, COL_TEXT_HEIGHT ) );

        if( getGridValue( row, COL_TEXT_THICKNESS ) > textSize / 4 )
        {
            wxString msg = _( "Text will not be readable with a thickness greater than\n"
                              "1/4 its width or height." );
            m_Parent->SetError( msg, this, m_grid, row, COL_TEXT_THICKNESS );
            return false;
        }
    }

    return true;
}


bool PANEL_MODEDIT_DEFAULTS::TransferDataFromWindow()
{
    if( !validateData() )
        return false;

    for( int i = 0; i < ROW_COUNT; ++i )
    {
        m_brdSettings.m_LineThickness[ i ] = getGridValue( i, COL_LINE_THICKNESS );

        if( i == ROW_EDGES || i == ROW_COURTYARD )
            continue;

        m_brdSettings.m_TextSize[ i ] =
                wxSize( getGridValue( i, COL_TEXT_WIDTH ), getGridValue( i, COL_TEXT_HEIGHT ) );
        m_brdSettings.m_TextThickness[ i ] = getGridValue( i, COL_TEXT_THICKNESS );
        m_brdSettings.m_TextItalic[ i ] =
                wxGridCellBoolEditor::IsTrueValue( m_grid->GetCellValue( i, COL_TEXT_ITALIC ) );
    }

    m_frame->SetDesignSettings( m_brdSettings );

    return true;
}
