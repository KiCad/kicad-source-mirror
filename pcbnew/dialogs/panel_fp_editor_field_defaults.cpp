/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 1992-2024 KiCad Developers, see AUTHORS.txt for contributors.
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

#include "panel_fp_editor_field_defaults.h"

#include <pgm_base.h>
#include <settings/settings_manager.h>
#include <footprint_editor_settings.h>
#include <template_fieldnames.h>
#include <widgets/std_bitmap_button.h>
#include <grid_tricks.h>
#include <eda_text.h>
#include <grid_layer_box_helpers.h>
#include <bitmaps.h>
#include <confirm.h>


class TEXT_ITEMS_GRID_TABLE : public wxGridTableBase
{
    bool                        m_forFieldProps;
    std::vector<TEXT_ITEM_INFO> m_items;

public:
    TEXT_ITEMS_GRID_TABLE( bool aForFieldProps ) : m_forFieldProps( aForFieldProps ) {}

    int GetNumberRows() override { return m_items.size(); }
    int GetNumberCols() override { return 3; }

    wxString GetColLabelValue( int aCol ) override
    {
        switch( aCol )
        {
        case 0: return m_forFieldProps ? _( "Value" ) : _( "Text Items" );
        case 1: return _( "Show" );
        case 2: return _( "Layer" );
        default: return wxEmptyString;
        }
    }

    wxString GetRowLabelValue( int aRow ) override
    {
        switch( aRow )
        {
        case 0: return _( "Reference designator" );
        case 1: return _( "Value" );
        default: return wxEmptyString;
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

    wxString GetValue( int row, int col ) override { return m_items[row].m_Text; }
    void     SetValue( int row, int col, const wxString& value ) override
    {
        if( col == 0 )
            m_items[row].m_Text = value;
    }

    bool GetValueAsBool( int row, int col ) override { return m_items[row].m_Visible; }
    void SetValueAsBool( int row, int col, bool value ) override
    {
        if( col == 1 )
            m_items[row].m_Visible = value;
    }

    long GetValueAsLong( int row, int col ) override { return m_items[row].m_Layer; }
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
            wxGridTableMessage msg( this, wxGRIDTABLE_NOTIFY_ROWS_APPENDED, aNumRows );
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


static FOOTPRINT_EDITOR_SETTINGS& GetPgmSettings()
{
    SETTINGS_MANAGER& mgr = Pgm().GetSettingsManager();
    return *mgr.GetAppSettings<FOOTPRINT_EDITOR_SETTINGS>( "fpedit" );
}


PANEL_FP_EDITOR_FIELD_DEFAULTS::PANEL_FP_EDITOR_FIELD_DEFAULTS( wxWindow*       aParent,
                                                                UNITS_PROVIDER* aUnitsProvider ) :
        PANEL_FP_EDITOR_FIELD_DEFAULTS_BASE( aParent ), m_unitProvider( aUnitsProvider ),
        m_designSettings( GetPgmSettings().m_DesignSettings )
{
    m_fieldPropsGrid->SetDefaultRowSize( m_fieldPropsGrid->GetDefaultRowSize() + 4 );

    m_fieldPropsGrid->SetTable( new TEXT_ITEMS_GRID_TABLE( true ), true );
    m_fieldPropsGrid->PushEventHandler( new GRID_TRICKS( m_fieldPropsGrid ) );
    m_fieldPropsGrid->SetSelectionMode( wxGrid::wxGridSelectRows );

    wxGridCellAttr* attr = new wxGridCellAttr;
    attr->SetRenderer( new wxGridCellBoolRenderer() );
    attr->SetReadOnly(); // not really; we delegate interactivity to GRID_TRICKS
    attr->SetAlignment( wxALIGN_CENTER, wxALIGN_CENTER );
    m_fieldPropsGrid->SetColAttr( 1, attr );

    attr = new wxGridCellAttr;
    attr->SetRenderer( new GRID_CELL_LAYER_RENDERER( nullptr ) );
    attr->SetEditor( new GRID_CELL_LAYER_SELECTOR( nullptr, {} ) );
    m_fieldPropsGrid->SetColAttr( 2, attr );

    m_textItemsGrid->SetDefaultRowSize( m_textItemsGrid->GetDefaultRowSize() + 4 );

    m_textItemsGrid->SetTable( new TEXT_ITEMS_GRID_TABLE( false ), true );
    m_textItemsGrid->PushEventHandler( new GRID_TRICKS( m_textItemsGrid ) );
    m_textItemsGrid->SetSelectionMode( wxGrid::wxGridSelectRows );

    attr = new wxGridCellAttr;
    attr->SetRenderer( new wxGridCellBoolRenderer() );
    attr->SetReadOnly(); // not really; we delegate interactivity to GRID_TRICKS
    attr->SetAlignment( wxALIGN_CENTER, wxALIGN_CENTER );
    m_textItemsGrid->SetColAttr( 1, attr );

    attr = new wxGridCellAttr;
    attr->SetRenderer( new GRID_CELL_LAYER_RENDERER( nullptr ) );
    attr->SetEditor( new GRID_CELL_LAYER_SELECTOR( nullptr, {} ) );
    m_textItemsGrid->SetColAttr( 2, attr );
}


PANEL_FP_EDITOR_FIELD_DEFAULTS::~PANEL_FP_EDITOR_FIELD_DEFAULTS()
{
    // destroy GRID_TRICKS before grids.
    m_fieldPropsGrid->PopEventHandler( true );
    m_textItemsGrid->PopEventHandler( true );
}


void PANEL_FP_EDITOR_FIELD_DEFAULTS::loadFPSettings( const FOOTPRINT_EDITOR_SETTINGS* aCfg )
{
    // Footprint defaults
    m_fieldPropsGrid->GetTable()->DeleteRows( 0, m_textItemsGrid->GetNumberRows() );
    m_fieldPropsGrid->GetTable()->AppendRows( 2 );

    for( int i : { REFERENCE_FIELD, VALUE_FIELD } )
    {
        TEXT_ITEM_INFO item = aCfg->m_DesignSettings.m_DefaultFPTextItems[i];

        m_fieldPropsGrid->GetTable()->SetValue( i, 0, item.m_Text );
        m_fieldPropsGrid->GetTable()->SetValueAsBool( i, 1, item.m_Visible );
        m_fieldPropsGrid->GetTable()->SetValueAsLong( i, 2, item.m_Layer );
    }

    m_textItemsGrid->GetTable()->DeleteRows( 0, m_textItemsGrid->GetNumberRows() );
    m_textItemsGrid->GetTable()->AppendRows( aCfg->m_DesignSettings.m_DefaultFPTextItems.size()
                                             - 2 );

    for( int i = 2; i < (int) aCfg->m_DesignSettings.m_DefaultFPTextItems.size(); ++i )
    {
        TEXT_ITEM_INFO item = aCfg->m_DesignSettings.m_DefaultFPTextItems[i];

        m_textItemsGrid->GetTable()->SetValue( i - 2, 0, item.m_Text );
        m_textItemsGrid->GetTable()->SetValueAsBool( i - 2, 1, item.m_Visible );
        m_textItemsGrid->GetTable()->SetValueAsLong( i - 2, 2, item.m_Layer );
    }

    Layout();
}


bool PANEL_FP_EDITOR_FIELD_DEFAULTS::TransferDataToWindow()
{
    const FOOTPRINT_EDITOR_SETTINGS& cfg = GetPgmSettings();

    loadFPSettings( &cfg );

    return true;
}


bool PANEL_FP_EDITOR_FIELD_DEFAULTS::Show( bool aShow )
{
    bool retVal = wxPanel::Show( aShow );

    if( aShow )
    {
        // These *should* work in the constructor, and indeed they do if this panel is the
        // first displayed.  However, on OSX 3.0.5 (at least), if another panel is displayed
        // first then the icons will be blank unless they're set here.
        m_bpAdd->SetBitmap( KiBitmapBundle( BITMAPS::small_plus ) );
        m_bpDelete->SetBitmap( KiBitmapBundle( BITMAPS::small_trash ) );
    }

    return retVal;
}


bool PANEL_FP_EDITOR_FIELD_DEFAULTS::TransferDataFromWindow()
{
    if( !m_textItemsGrid->CommitPendingChanges() )
        return false;

    BOARD_DESIGN_SETTINGS& cfg = m_designSettings;

    // Footprint defaults
    cfg.m_DefaultFPTextItems.clear();

    wxGridTableBase* table = m_fieldPropsGrid->GetTable();

    for( int i : { REFERENCE_FIELD, VALUE_FIELD } )
    {
        wxString text = table->GetValue( i, 0 );
        bool     visible = table->GetValueAsBool( i, 1 );
        int      layer = (int) table->GetValueAsLong( i, 2 );

        cfg.m_DefaultFPTextItems.emplace_back( text, visible, layer );
    }

    table = m_textItemsGrid->GetTable();

    for( int i = 0; i < m_textItemsGrid->GetNumberRows(); ++i )
    {
        wxString text = table->GetValue( i, 0 );
        bool     visible = table->GetValueAsBool( i, 1 );
        int      layer = (int) table->GetValueAsLong( i, 2 );

        cfg.m_DefaultFPTextItems.emplace_back( text, visible, layer );
    }

    return true;
}


void PANEL_FP_EDITOR_FIELD_DEFAULTS::OnAddTextItem( wxCommandEvent& event )
{
    if( !m_textItemsGrid->CommitPendingChanges() )
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


void PANEL_FP_EDITOR_FIELD_DEFAULTS::OnDeleteTextItem( wxCommandEvent& event )
{
    wxArrayInt selectedRows = m_textItemsGrid->GetSelectedRows();

    if( selectedRows.empty() && m_textItemsGrid->GetGridCursorRow() >= 0 )
        selectedRows.push_back( m_textItemsGrid->GetGridCursorRow() );

    if( selectedRows.empty() )
        return;

    if( !m_textItemsGrid->CommitPendingChanges() )
        return;

    // Reverse sort so deleting a row doesn't change the indexes of the other rows.
    selectedRows.Sort(
            []( int* first, int* second )
            {
                return *second - *first;
            } );

    for( int row : selectedRows )
    {
        m_textItemsGrid->GetTable()->DeleteRows( row, 1 );

        if( m_textItemsGrid->GetNumberRows() > 0 )
        {
            m_textItemsGrid->MakeCellVisible( std::max( 0, row - 1 ),
                                              m_textItemsGrid->GetGridCursorCol() );
            m_textItemsGrid->SetGridCursor( std::max( 0, row - 1 ),
                                            m_textItemsGrid->GetGridCursorCol() );
        }
    }
}


void PANEL_FP_EDITOR_FIELD_DEFAULTS::ResetPanel()
{
    FOOTPRINT_EDITOR_SETTINGS cfg;
    cfg.Load(); // Loading without a file will init to defaults

    loadFPSettings( &cfg );
}
