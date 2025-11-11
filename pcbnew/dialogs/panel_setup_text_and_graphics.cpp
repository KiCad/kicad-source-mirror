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

#include "panel_setup_text_and_graphics.h"

#include <board_design_settings.h>
#include <dialogs/panel_setup_dimensions.h>
#include <pcb_edit_frame.h>
#include <grid_tricks.h>
#include <eda_text.h>

#include <kidialog.h>


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


PANEL_SETUP_TEXT_AND_GRAPHICS::PANEL_SETUP_TEXT_AND_GRAPHICS( wxWindow* aParentWindow, PCB_EDIT_FRAME* aFrame,
                                                              BOARD_DESIGN_SETTINGS* aBrdSettings) :
        PANEL_SETUP_TEXT_AND_GRAPHICS_BASE( aParentWindow ),
        m_Frame( aFrame ),
        m_BrdSettings( aBrdSettings )
{
    m_grid->SetUnitsProvider( m_Frame );
    m_grid->SetAutoEvalCols( { COL_LINE_THICKNESS,
                               COL_TEXT_WIDTH,
                               COL_TEXT_HEIGHT,
                               COL_TEXT_THICKNESS } );
    m_grid->SetUseNativeColLabels();

    // Work around a bug in wxWidgets where it fails to recalculate the grid height
    // after changing the default row size
    m_grid->AppendRows( 1 );
    m_grid->DeleteRows( m_grid->GetNumberRows() - 1, 1 );

    // Gives a suitable size to m_grid columns
    // The default wxWidget col size is not very good
    // Calculate a min best size to handle longest usual numeric values:
    int min_best_width = m_grid->GetTextExtent( wxT( "555,555555 mils" ) ).x;

    for( int i = 0; i < m_grid->GetNumberCols(); ++i )
    {
        // We calculate the column min size only from texts sizes, not using the initial col width
        // as this initial width is sometimes strange depending on the language (wxGrid bug?)
        int min_width =  m_grid->GetVisibleWidth( i );

        m_grid->SetColMinimalWidth( i, min_width );

        // We use a "best size" >= min_best_width
        m_grid->SetColSize( i, std::max( min_width, min_best_width ) );
    }

    m_grid->PushEventHandler( new GRID_TRICKS( m_grid ) );

	m_mainSizer->Fit( this );

    m_Frame->Bind( EDA_EVT_UNITS_CHANGED, &PANEL_SETUP_TEXT_AND_GRAPHICS::onUnitsChanged, this );
}


PANEL_SETUP_TEXT_AND_GRAPHICS::~PANEL_SETUP_TEXT_AND_GRAPHICS()
{
    // destroy GRID_TRICKS before m_grid.
    m_grid->PopEventHandler( true );

    m_Frame->Unbind( EDA_EVT_UNITS_CHANGED, &PANEL_SETUP_TEXT_AND_GRAPHICS::onUnitsChanged, this );
}


void PANEL_SETUP_TEXT_AND_GRAPHICS::onUnitsChanged( wxCommandEvent& aEvent )
{
    BOARD_DESIGN_SETTINGS  tempBDS( nullptr, "dummy" );
    BOARD_DESIGN_SETTINGS* saveBDS = m_BrdSettings;

    m_BrdSettings = &tempBDS;       // No, address of stack var does not escape function

    TransferDataFromWindow();
    TransferDataToWindow();

    m_BrdSettings = saveBDS;

    aEvent.Skip();
}


bool PANEL_SETUP_TEXT_AND_GRAPHICS::TransferDataToWindow()
{
    wxColour disabledColour = wxSystemSettings::GetColour( wxSYS_COLOUR_FRAMEBK );

#define SET_MILS_CELL( row, col, val ) \
    m_grid->SetCellValue( row, col, m_Frame->StringFromValue( val, true ) )

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
            m_grid->SetCellValue( i, COL_TEXT_ITALIC, m_BrdSettings->m_TextItalic[ i ] ? wxT( "1" )
                                                                                       : wxT( "" ) );
            m_grid->SetCellValue( i, COL_TEXT_UPRIGHT, m_BrdSettings->m_TextUpright[ i ] ? wxT( "1" )
                                                                                         : wxT( "" ) );

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
        m_grid->SetColMinimalWidth( col, m_grid->GetVisibleWidth( col ) );

    m_grid->SetRowLabelSize( m_grid->GetVisibleWidth( -1, true, true, true ) );

    return true;
}


bool PANEL_SETUP_TEXT_AND_GRAPHICS::TransferDataFromWindow()
{
    if( !m_grid->CommitPendingChanges() )
        return false;

    // A minimal value for sizes and thickness
    const int minWidth = pcbIUScale.mmToIU( MINIMUM_LINE_WIDTH_MM );
    const int maxWidth = pcbIUScale.mmToIU( MAXIMUM_LINE_WIDTH_MM );
    const int minSize  = pcbIUScale.mmToIU( TEXT_MIN_SIZE_MM );
    const int maxSize = pcbIUScale.mmToIU( TEXT_MAX_SIZE_MM );
    wxString errorsMsg;
    UNITS_PROVIDER* unitProvider = m_Frame;

    for( int i = 0; i < ROW_COUNT; ++i )
    {
        bool badParam = false;

        int lineWidth =  m_grid->GetUnitValue( i, COL_LINE_THICKNESS );

        if( lineWidth < minWidth || lineWidth > maxWidth )
        {
            if( !errorsMsg.IsEmpty() )
                errorsMsg += wxT( "\n\n" );

            errorsMsg += wxString::Format( _( "%s: Incorrect line width.\n"
                                              "It must be between %s and %s" ),
                                            m_grid->GetRowLabelValue( i ),
                                            unitProvider->StringFromValue( minWidth , true),
                                            unitProvider->StringFromValue( maxWidth , true) );
            badParam = true;
        }

        if( !badParam )
            m_BrdSettings->m_LineThickness[ i ] = lineWidth;

        if( i == ROW_EDGES || i == ROW_COURTYARD )
            continue;

        badParam = false;
        int textWidth =  m_grid->GetUnitValue( i, COL_TEXT_WIDTH );
        int textHeight = m_grid->GetUnitValue( i, COL_TEXT_HEIGHT );
        int textThickness = m_grid->GetUnitValue( i, COL_TEXT_THICKNESS );

        if( textWidth < minSize || textHeight < minSize || textWidth > maxSize || textHeight > maxSize )
        {
            if( !errorsMsg.IsEmpty() )
                errorsMsg += wxT( "\n\n" );

            errorsMsg += wxString::Format( _( "%s: Text size is incorrect.\n"
                                              "Size must be between %s and %s" ),
                                            m_grid->GetRowLabelValue( i ),
                                            unitProvider->StringFromValue( minSize , true),
                                            unitProvider->StringFromValue( maxSize , true) );
            badParam = true;
        }

        // Text thickness cannot be > text size /4 to be readable
        int textMaxThickness = std::min( maxWidth, std::min( textWidth, textHeight ) /4);

        if( !badParam && ( textThickness < minWidth || textThickness > textMaxThickness ) )
        {
            if( !errorsMsg.IsEmpty() )
                errorsMsg += wxT( "\n\n" );

            if( textThickness > textMaxThickness )
            {
                errorsMsg += wxString::Format( _( "%s: Text thickness is too large.\n"
                                                  "It will be truncated to %s" ),
                                                m_grid->GetRowLabelValue( i ),
                                                unitProvider->StringFromValue( textMaxThickness , true) );
            }
            else if( textThickness < minWidth )
            {
                errorsMsg += wxString::Format( _( "%s: Text thickness is too small.\n"
                                                  "It will be truncated to %s" ),
                                                m_grid->GetRowLabelValue( i ),
                                                unitProvider->StringFromValue( minWidth , true ) );
            }

            textThickness = std::min( textThickness, textMaxThickness );
            textThickness = std::max( textThickness, minWidth );
            SET_MILS_CELL( i, COL_TEXT_THICKNESS, textThickness );
        }

        if( !badParam )
        {
            m_BrdSettings->m_TextSize[ i ] = VECTOR2I( textWidth, textHeight );
            m_BrdSettings->m_TextThickness[ i ] = textThickness;
        }

        m_BrdSettings->m_TextItalic[ i ] =
                wxGridCellBoolEditor::IsTrueValue( m_grid->GetCellValue( i, COL_TEXT_ITALIC ) );
        m_BrdSettings->m_TextUpright[ i ] =
                wxGridCellBoolEditor::IsTrueValue( m_grid->GetCellValue( i, COL_TEXT_UPRIGHT ) );
    }

    if( errorsMsg.IsEmpty() )
        return true;

    KIDIALOG dlg( wxGetTopLevelParent( m_grid ), errorsMsg, KIDIALOG::KD_ERROR, _( "Parameter error" ) );
    dlg.ShowModal();

    return false;
}


bool PANEL_SETUP_TEXT_AND_GRAPHICS::CommitPendingChanges()
{
    return m_grid->CommitPendingChanges();
}