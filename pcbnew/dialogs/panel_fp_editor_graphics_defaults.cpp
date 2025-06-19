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

#include "panel_fp_editor_graphics_defaults.h"

#include <pgm_base.h>
#include <settings/settings_manager.h>
#include <footprint_editor_settings.h>
#include <grid_tricks.h>
#include <eda_text.h>
#include <panel_setup_dimensions.h>
#include <grid_layer_box_helpers.h>
#include <bitmaps.h>
#include <confirm.h>
#include <kidialog.h>


// Columns of graphics grid
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
    ROW_FAB,
    ROW_OTHERS,

    ROW_COUNT
};


static FOOTPRINT_EDITOR_SETTINGS& GetPgmSettings()
{
    return *GetAppSettings<FOOTPRINT_EDITOR_SETTINGS>( "fpedit" );
}


PANEL_FP_EDITOR_GRAPHICS_DEFAULTS::PANEL_FP_EDITOR_GRAPHICS_DEFAULTS( wxWindow* aParent,
                                                                      UNITS_PROVIDER* aUnitsProvider ) :
        PANEL_FP_EDITOR_GRAPHICS_DEFAULTS_BASE( aParent ),
        m_unitProvider( aUnitsProvider ),
        m_designSettings( GetPgmSettings().m_DesignSettings ),
        m_dimensionsPanel( std::make_unique<PANEL_SETUP_DIMENSIONS>( this, *m_unitProvider, m_designSettings ) )
{
    m_graphicsGrid->SetUnitsProvider( aUnitsProvider );
    m_graphicsGrid->SetAutoEvalCols( { COL_LINE_THICKNESS,
                                       COL_TEXT_WIDTH,
                                       COL_TEXT_HEIGHT,
                                       COL_TEXT_THICKNESS } );

    // Work around a bug in wxWidgets where it fails to recalculate the grid height
    // after changing the default row size
    m_graphicsGrid->AppendRows( 1 );
    m_graphicsGrid->DeleteRows( m_graphicsGrid->GetNumberRows() - 1, 1 );

    m_graphicsGrid->PushEventHandler( new GRID_TRICKS( m_graphicsGrid ) );

    GetSizer()->Add( m_dimensionsPanel.get(), 0, wxEXPAND, 5 );
}


PANEL_FP_EDITOR_GRAPHICS_DEFAULTS::~PANEL_FP_EDITOR_GRAPHICS_DEFAULTS()
{
    // destroy GRID_TRICKS before grids.
    m_graphicsGrid->PopEventHandler( true );
}


void PANEL_FP_EDITOR_GRAPHICS_DEFAULTS::loadFPSettings( const FOOTPRINT_EDITOR_SETTINGS* aCfg )
{
    wxColour disabledColour = wxSystemSettings::GetColour( wxSYS_COLOUR_FRAMEBK );

    auto disableCell = [&]( int row, int col )
    {
        m_graphicsGrid->SetReadOnly( row, col );
        m_graphicsGrid->SetCellBackgroundColour( row, col, disabledColour );
    };

    for( int i = 0; i < ROW_COUNT; ++i )
    {
        m_graphicsGrid->SetUnitValue( i, COL_LINE_THICKNESS,
                                      aCfg->m_DesignSettings.m_LineThickness[i] );

        if( i == ROW_EDGES || i == ROW_COURTYARD )
        {
            disableCell( i, COL_TEXT_WIDTH );
            disableCell( i, COL_TEXT_HEIGHT );
            disableCell( i, COL_TEXT_THICKNESS );
            disableCell( i, COL_TEXT_ITALIC );
        }
        else
        {
            m_graphicsGrid->SetUnitValue( i, COL_TEXT_WIDTH,
                                          aCfg->m_DesignSettings.m_TextSize[i].x );
            m_graphicsGrid->SetUnitValue( i, COL_TEXT_HEIGHT,
                                          aCfg->m_DesignSettings.m_TextSize[i].y );
            m_graphicsGrid->SetUnitValue( i, COL_TEXT_THICKNESS,
                                          aCfg->m_DesignSettings.m_TextThickness[i] );
            m_graphicsGrid->SetCellValue( i, COL_TEXT_ITALIC,
                                          aCfg->m_DesignSettings.m_TextItalic[i] ? wxT( "1" )
                                                                                 : wxT( "" ) );

            auto attr = new wxGridCellAttr;
            attr->SetRenderer( new wxGridCellBoolRenderer() );
            attr->SetReadOnly(); // not really; we delegate interactivity to GRID_TRICKS
            attr->SetAlignment( wxALIGN_CENTER, wxALIGN_CENTER );
            m_graphicsGrid->SetAttr( i, COL_TEXT_ITALIC, attr );
        }
    }

    for( int col = 0; col < m_graphicsGrid->GetNumberCols(); col++ )
    {
        // Set the minimal width to the column label size.
        m_graphicsGrid->SetColMinimalWidth( col, m_graphicsGrid->GetVisibleWidth( col, true, false ) );

        // Set the width to see the full contents
        if( m_graphicsGrid->IsColShown( col ) )
            m_graphicsGrid->SetColSize( col, m_graphicsGrid->GetVisibleWidth( col, true, true, true ) );
    }

    m_graphicsGrid->SetRowLabelSize( m_graphicsGrid->GetVisibleWidth( -1, true, true, true ) );

    m_dimensionsPanel->LoadFromSettings( aCfg->m_DesignSettings );

    Layout();
}


bool PANEL_FP_EDITOR_GRAPHICS_DEFAULTS::TransferDataToWindow()
{
    const FOOTPRINT_EDITOR_SETTINGS& cfg = GetPgmSettings();

    loadFPSettings( &cfg );

    return true;
}


bool PANEL_FP_EDITOR_GRAPHICS_DEFAULTS::Show( bool aShow )
{
    bool retVal = wxPanel::Show( aShow );

    if( aShow && m_firstShow )
    {
        m_graphicsGrid->SetColSize( 0, m_graphicsGrid->GetColSize( 0 ) + 1 );
        m_firstShow = false;
    }

    return retVal;
}


bool PANEL_FP_EDITOR_GRAPHICS_DEFAULTS::TransferDataFromWindow()
{
    if( !m_graphicsGrid->CommitPendingChanges() )
        return false;

    BOARD_DESIGN_SETTINGS& cfg = m_designSettings;

    // A minimal value for sizes and thickness:
    const int minWidth = pcbIUScale.mmToIU( MINIMUM_LINE_WIDTH_MM );
    const int maxWidth = pcbIUScale.mmToIU( MAXIMUM_LINE_WIDTH_MM );
    const int minSize = pcbIUScale.mmToIU( TEXT_MIN_SIZE_MM );
    const int maxSize = pcbIUScale.mmToIU( TEXT_MAX_SIZE_MM );
    wxString  errorsMsg;

    for( int i = 0; i < ROW_COUNT; ++i )
    {
        bool badParam = false;

        int lineWidth = m_graphicsGrid->GetUnitValue( i, COL_LINE_THICKNESS );

        if( lineWidth < minWidth || lineWidth > maxWidth )
        {
            if( !errorsMsg.IsEmpty() )
                errorsMsg += wxT( "\n\n" );

            errorsMsg += wxString::Format( _( "%s: Incorrect line width.\n"
                                              "It must be between %s and %s" ),
                                           m_graphicsGrid->GetRowLabelValue( i ),
                                           m_unitProvider->StringFromValue( minWidth, true ),
                                           m_unitProvider->StringFromValue( maxWidth, true ) );
            badParam = true;
        }

        if( !badParam )
            cfg.m_LineThickness[i] = lineWidth;

        if( i == ROW_EDGES || i == ROW_COURTYARD )
            continue;

        badParam = false;
        int textWidth = m_graphicsGrid->GetUnitValue( i, COL_TEXT_WIDTH );
        int textHeight = m_graphicsGrid->GetUnitValue( i, COL_TEXT_HEIGHT );
        int textThickness = m_graphicsGrid->GetUnitValue( i, COL_TEXT_THICKNESS );

        if( textWidth < minSize || textHeight < minSize || textWidth > maxSize || textHeight > maxSize )
        {
            if( !errorsMsg.IsEmpty() )
                errorsMsg += wxT( "\n\n" );

            errorsMsg += wxString::Format( _( "%s: Text size is incorrect.\n"
                                              "Size must be between %s and %s" ),
                                           m_graphicsGrid->GetRowLabelValue( i ),
                                           m_unitProvider->StringFromValue( minSize, true ),
                                           m_unitProvider->StringFromValue( maxSize, true ) );
            badParam = true;
        }

        // Text thickness cannot be > text size /4 to be readable
        int textMinDim = std::min( textWidth, textHeight );
        int textMaxThickness = std::min( maxWidth, textMinDim / 4 );

        if( !badParam && ( textThickness < minWidth || textThickness > textMaxThickness ) )
        {
            if( !errorsMsg.IsEmpty() )
                errorsMsg += wxT( "\n\n" );

            if( textThickness > textMaxThickness )
            {
                errorsMsg += wxString::Format( _( "%s: Text thickness is too large.\n"
                                                  "It will be truncated to %s" ),
                                               m_graphicsGrid->GetRowLabelValue( i ),
                                               m_unitProvider->StringFromValue( textMaxThickness, true ) );
            }
            else if( textThickness < minWidth )
            {
                errorsMsg += wxString::Format( _( "%s: Text thickness is too small.\n"
                                                  "It will be truncated to %s" ),
                                               m_graphicsGrid->GetRowLabelValue( i ),
                                               m_unitProvider->StringFromValue( minWidth, true ) );
            }

            textThickness = std::min( textThickness, textMaxThickness );
            textThickness = std::max( textThickness, minWidth );
            m_graphicsGrid->SetUnitValue( i, COL_TEXT_THICKNESS, textThickness );
        }

        if( !badParam )
        {
            cfg.m_TextSize[i] = VECTOR2I( textWidth, textHeight );
            cfg.m_TextThickness[i] = textThickness;
        }

        wxString msg = m_graphicsGrid->GetCellValue( i, COL_TEXT_ITALIC );
        cfg.m_TextItalic[i] = wxGridCellBoolEditor::IsTrueValue( msg );
    }

    m_dimensionsPanel->TransferDataFromWindow();

    if( errorsMsg.IsEmpty() )
        return true;

    KIDIALOG dlg( wxGetTopLevelParent( this ), errorsMsg, KIDIALOG::KD_ERROR, _( "Parameter error" ) );
    dlg.ShowModal();

    return false;
}


void PANEL_FP_EDITOR_GRAPHICS_DEFAULTS::ResetPanel()
{
    FOOTPRINT_EDITOR_SETTINGS cfg;
    cfg.Load(); // Loading without a file will init to defaults

    loadFPSettings( &cfg );
}
