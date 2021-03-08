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

#include <widgets/paged_dialog.h>
#include <footprint_edit_frame.h>
#include <footprint_editor_settings.h>
#include <widgets/wx_grid.h>
#include <grid_tricks.h>

#include <panel_fp_editor_defaults.h>
#include <grid_layer_box_helpers.h>
#include <bitmaps.h>

class TEXT_ITEMS_GRID_TABLE : public wxGridTableBase
{
    std::vector<TEXT_ITEM_INFO> m_items;

public:
    TEXT_ITEMS_GRID_TABLE()
    { }

    int GetNumberRows() override { return m_items.size(); }
    int GetNumberCols() override { return 3; }

    wxString GetColLabelValue( int aCol ) override
    {
        switch( aCol )
        {
        case 0: return _( "Text Items" );
        case 1: return _( "Show" );
        case 2: return _( "Layer" );
        default: return wxEmptyString;
        }
    }

    wxString GetRowLabelValue( int aRow ) override
    {
        switch( aRow )
        {
        case 0:   return _( "Reference designator" );
        case 1:   return _( "Value" );
        default:  return wxEmptyString;
        }
    }

    bool CanGetValueAs( int aRow, int aCol, const wxString& aTypeName ) override
    {
        switch( aCol )
        {
        case 0: return aTypeName == wxGRID_VALUE_STRING;
        case 1: return aTypeName == wxGRID_VALUE_BOOL;
        case 2: return aTypeName == wxGRID_VALUE_NUMBER;
        default: wxFAIL; return false;
        }
    }

    bool CanSetValueAs( int aRow, int aCol, const wxString& aTypeName ) override
    {
        return CanGetValueAs( aRow, aCol, aTypeName );
    }

    wxString GetValue( int row, int col ) override
    {
        return m_items[row].m_Text;
    }
    void SetValue( int row, int col, const wxString& value ) override
    {
        if( col == 0 )
            m_items[row].m_Text = value;
    }

    bool GetValueAsBool( int row, int col ) override
    {
        return m_items[row].m_Visible;
    }
    void SetValueAsBool( int row, int col, bool value ) override
    {
        if( col == 1 )
            m_items[row].m_Visible = value;
    }

    long GetValueAsLong( int row, int col ) override
    {
        return m_items[row].m_Layer;
    }
    void SetValueAsLong( int row, int col, long value ) override
    {
        if( col == 2 )
            m_items[row].m_Layer = (int) value;
    }

    bool AppendRows( size_t aNumRows = 1 ) override
    {
        for( size_t i = 0; i < aNumRows; ++i )
            m_items.emplace_back( wxT( "" ), true, F_SilkS );

        if( GetView() )
        {
            wxGridTableMessage msg( this,  wxGRIDTABLE_NOTIFY_ROWS_APPENDED, aNumRows );
            GetView()->ProcessTableMessage( msg );
        }

        return true;
    }

    bool DeleteRows( size_t aPos, size_t aNumRows ) override
    {
        // aPos may be a large positive, e.g. size_t(-1), and the sum of
        // aPos+aNumRows may wrap here, so both ends of the range are tested.
        if( aPos < m_items.size() && aPos + aNumRows <= m_items.size() )
        {
            m_items.erase( m_items.begin() + aPos, m_items.begin() + aPos + aNumRows );

            if( GetView() )
            {
                wxGridTableMessage msg( this, wxGRIDTABLE_NOTIFY_ROWS_DELETED, aPos, aNumRows );
                GetView()->ProcessTableMessage( msg );
            }
            return true;
        }

        return false;
    }
};


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
    ROW_FAB,
    ROW_OTHERS,

    ROW_COUNT
};


PANEL_FP_EDITOR_DEFAULTS::PANEL_FP_EDITOR_DEFAULTS( FOOTPRINT_EDIT_FRAME* aFrame,
                                                    PAGED_DIALOG* aParent) :
        PANEL_FP_EDITOR_DEFAULTS_BASE( aParent->GetTreebook() ),
        m_brdSettings( aFrame->GetDesignSettings() ),
        m_frame( aFrame ),
        m_parent( aParent )
{
    m_textItemsGrid->SetDefaultRowSize( m_textItemsGrid->GetDefaultRowSize() + 4 );
    m_layerClassesGrid->SetDefaultRowSize( m_layerClassesGrid->GetDefaultRowSize() + 4 );

    m_textItemsGrid->SetTable( new TEXT_ITEMS_GRID_TABLE(), true );

    wxGridCellAttr* attr = new wxGridCellAttr;
    attr->SetRenderer( new wxGridCellBoolRenderer() );
    attr->SetReadOnly();    // not really; we delegate interactivity to GRID_TRICKS
    m_textItemsGrid->SetColAttr( 1, attr );

    attr = new wxGridCellAttr;
    attr->SetRenderer( new GRID_CELL_LAYER_RENDERER( m_frame ) );
    attr->SetEditor( new GRID_CELL_LAYER_SELECTOR( m_frame, {} ) );
    m_textItemsGrid->SetColAttr( 2, attr );

    // Work around a bug in wxWidgets where it fails to recalculate the grid height
    // after changing the default row size
    m_layerClassesGrid->AppendRows( 1 );
    m_layerClassesGrid->DeleteRows( m_layerClassesGrid->GetNumberRows() - 1, 1 );

    m_layerClassesGrid->PushEventHandler( new GRID_TRICKS( m_layerClassesGrid ) );

    wxFont infoFont = wxSystemSettings::GetFont( wxSYS_DEFAULT_GUI_FONT );
    infoFont.SetSymbolicSize( wxFONTSIZE_SMALL );
    m_staticTextInfo->SetFont( infoFont );
}


PANEL_FP_EDITOR_DEFAULTS::~PANEL_FP_EDITOR_DEFAULTS()
{
    // destroy GRID_TRICKS before m_layerClassesGrid.
    m_layerClassesGrid->PopEventHandler( true );
}


bool PANEL_FP_EDITOR_DEFAULTS::TransferDataToWindow()
{
    wxColour disabledColour = wxSystemSettings::GetColour( wxSYS_COLOUR_BACKGROUND );

#define SET_MILS_CELL( row, col, val ) \
    m_layerClassesGrid->SetCellValue( row, col, StringFromValue( m_frame->GetUserUnits(), val, true ) )

#define DISABLE_CELL( row, col ) \
    m_layerClassesGrid->SetReadOnly( row, col ); m_layerClassesGrid->SetCellBackgroundColour( row, col, disabledColour );

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
            m_layerClassesGrid->SetCellValue( i, COL_TEXT_ITALIC, m_brdSettings.m_TextItalic[ i ] ? "1" : "" );

            auto attr = new wxGridCellAttr;
            attr->SetRenderer( new wxGridCellBoolRenderer() );
            attr->SetReadOnly();    // not really; we delegate interactivity to GRID_TRICKS
            attr->SetAlignment( wxALIGN_CENTER, wxALIGN_CENTER );
            m_layerClassesGrid->SetAttr( i, COL_TEXT_ITALIC, attr );
        }
    }

    // Footprint defaults
    m_textItemsGrid->GetTable()->AppendRows( m_brdSettings.m_DefaultFPTextItems.size() );

    for( size_t i = 0; i < m_brdSettings.m_DefaultFPTextItems.size(); ++i )
    {
        TEXT_ITEM_INFO item = m_brdSettings.m_DefaultFPTextItems[i];

        m_textItemsGrid->GetTable()->SetValue( i, 0, item.m_Text );
        m_textItemsGrid->GetTable()->SetValueAsBool( i, 1, item.m_Visible );
        m_textItemsGrid->GetTable()->SetValueAsLong( i, 2, item.m_Layer );
    }

    for( int col = 0; col < m_layerClassesGrid->GetNumberCols(); col++ )
    {
        // Set the minimal width to the column label size.
        m_layerClassesGrid->SetColMinimalWidth( col, m_layerClassesGrid->GetVisibleWidth( col, true, false, false ) );

        // Set the width to see the full contents
        if( m_layerClassesGrid->IsColShown( col ) )
            m_layerClassesGrid->SetColSize( col, m_layerClassesGrid->GetVisibleWidth( col, true, true, true ) );
    }

    m_layerClassesGrid->SetRowLabelSize( m_layerClassesGrid->GetVisibleWidth( -1, true, true, true ) );

    Layout();

    return true;
}


bool PANEL_FP_EDITOR_DEFAULTS::Show( bool aShow )
{
    bool retVal = wxPanel::Show( aShow );

    if( aShow )
    {
        // These *should* work in the constructor, and indeed they do if this panel is the
        // first displayed.  However, on OSX 3.0.5 (at least), if another panel is displayed
        // first then the icons will be blank unless they're set here.
        m_bpAdd->SetBitmap( KiBitmap( BITMAPS::small_plus ) );
        m_bpDelete->SetBitmap( KiBitmap( BITMAPS::small_trash ) );
    }

    if( aShow && m_firstShow )
    {
        m_layerClassesGrid->SetColSize( 0, m_layerClassesGrid->GetColSize( 0 ) + 1 );
        m_firstShow = false;
    }

    return retVal;
}


int PANEL_FP_EDITOR_DEFAULTS::getGridValue( int aRow, int aCol )
{
    return ValueFromString( m_frame->GetUserUnits(),
                            m_layerClassesGrid->GetCellValue( aRow, aCol ) );
}


bool PANEL_FP_EDITOR_DEFAULTS::validateData()
{
    if( !m_textItemsGrid->CommitPendingChanges() || !m_layerClassesGrid->CommitPendingChanges() )
        return false;

    // Test text parameters.
    for( int row : { ROW_SILK, ROW_COPPER, ROW_FAB, ROW_OTHERS } )
    {
        int textSize = std::min( getGridValue( row, COL_TEXT_WIDTH ),
                                 getGridValue( row, COL_TEXT_HEIGHT ) );

        if( getGridValue( row, COL_TEXT_THICKNESS ) > textSize / 4 )
        {
            wxString msg = _( "Text will not be readable with a thickness greater than\n"
                              "1/4 its width or height." );
            m_parent->SetError( msg, this, m_layerClassesGrid, row, COL_TEXT_THICKNESS );
            return false;
        }
    }

    return true;
}


bool PANEL_FP_EDITOR_DEFAULTS::TransferDataFromWindow()
{
    if( !validateData() )
        return false;

    for( int i = 0; i < ROW_COUNT; ++i )
    {
        m_brdSettings.m_LineThickness[ i ] = getGridValue( i, COL_LINE_THICKNESS );

        if( i == ROW_EDGES || i == ROW_COURTYARD )
            continue;

        m_brdSettings.m_TextSize[ i ] = wxSize( getGridValue( i, COL_TEXT_WIDTH ),
                                                getGridValue( i, COL_TEXT_HEIGHT ) );
        m_brdSettings.m_TextThickness[ i ] = getGridValue( i, COL_TEXT_THICKNESS );

        wxString msg = m_layerClassesGrid->GetCellValue( i, COL_TEXT_ITALIC );
        m_brdSettings.m_TextItalic[ i ] = wxGridCellBoolEditor::IsTrueValue( msg );
    }

    // Footprint defaults
    wxGridTableBase* table = m_textItemsGrid->GetTable();
    m_brdSettings.m_DefaultFPTextItems.clear();

    for( int i = 0; i < m_textItemsGrid->GetNumberRows(); ++i )
    {
        wxString text = table->GetValue( i, 0 );
        bool     visible = table->GetValueAsBool( i, 1 );
        int      layer = (int) table->GetValueAsLong( i, 2 );

        m_brdSettings.m_DefaultFPTextItems.emplace_back( text, visible, layer );
    }

    m_frame->GetDesignSettings() = m_brdSettings;

    if( FOOTPRINT_EDITOR_SETTINGS* cfg = m_frame->GetSettings() )
        cfg->m_DesignSettings = m_brdSettings;

    return true;
}


void PANEL_FP_EDITOR_DEFAULTS::OnAddTextItem( wxCommandEvent& event )
{
    if( !m_textItemsGrid->CommitPendingChanges() || !m_layerClassesGrid->CommitPendingChanges() )
        return;

    wxGridTableBase* table = m_textItemsGrid->GetTable();

    int newRow = m_textItemsGrid->GetNumberRows();
    table->AppendRows( 1 );
    table->SetValueAsBool( newRow, 1, table->GetValueAsBool( newRow - 1, 1 ) );
    table->SetValueAsLong( newRow, 2, table->GetValueAsLong( newRow - 1, 2 ) );

    m_textItemsGrid->MakeCellVisible( newRow, 0 );
    m_textItemsGrid->SetGridCursor( newRow, 0 );

    m_textItemsGrid->EnableCellEditControl( true );
    m_textItemsGrid->ShowCellEditControl();
}


void PANEL_FP_EDITOR_DEFAULTS::OnDeleteTextItem( wxCommandEvent& event )
{
    if( !m_textItemsGrid->CommitPendingChanges() || !m_layerClassesGrid->CommitPendingChanges() )
        return;

    int curRow = m_textItemsGrid->GetGridCursorRow();

    if( curRow < 2 )    // First two rows are required
        return;

    m_textItemsGrid->GetTable()->DeleteRows( curRow, 1 );

    curRow = std::max( 0, curRow - 1 );
    m_textItemsGrid->MakeCellVisible( curRow, m_textItemsGrid->GetGridCursorCol() );
    m_textItemsGrid->SetGridCursor( curRow, m_textItemsGrid->GetGridCursorCol() );
}


