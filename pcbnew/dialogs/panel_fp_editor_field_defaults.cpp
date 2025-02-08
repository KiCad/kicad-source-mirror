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

#include "panel_fp_editor_field_defaults.h"

#include <pgm_base.h>
#include <settings/settings_manager.h>
#include <footprint_editor_settings.h>
#include <widgets/paged_dialog.h>
#include <template_fieldnames.h>
#include <widgets/std_bitmap_button.h>
#include <grid_tricks.h>
#include <eda_text.h>
#include <grid_layer_box_helpers.h>
#include <bitmaps.h>
#include <confirm.h>


class LAYER_NAMES_GRID_TABLE : public wxGridTableBase
{
    std::vector<TEXT_ITEM_INFO> m_items;

public:
    LAYER_NAMES_GRID_TABLE() {}

    int GetNumberRows() override { return m_items.size(); }
    int GetNumberCols() override { return 2; }

    wxString GetColLabelValue( int aCol ) override
    {
        switch( aCol )
        {
        case 0: return _( "Layer" );
        case 1: return _( "Name" );
        default: return wxEmptyString;
        }
    }

    bool CanGetValueAs( int aRow, int aCol, const wxString& aTypeName ) override
    {
        switch( aCol )
        {
        case 0: return aTypeName == wxGRID_VALUE_NUMBER;
        case 1: return aTypeName == wxGRID_VALUE_STRING;
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
        if( col == 1 )
            m_items[row].m_Text = value;
    }

    long GetValueAsLong( int row, int col ) override { return m_items[row].m_Layer; }
    void SetValueAsLong( int row, int col, long value ) override
    {
        if( col == 0 )
            m_items[row].m_Layer = static_cast<PCB_LAYER_ID>( value );
    }

    bool AppendRows( size_t aNumRows = 1 ) override
    {
        std::set<int> layers;
        int layer = User_1;

        for( const TEXT_ITEM_INFO& item : m_items )
            layers.insert( item.m_Layer );


        for( size_t i = 0; i < aNumRows; ++i )
        {
            while( layers.contains( layer ) )
                layer = layer + 2;

            if( IsUserLayer( static_cast<PCB_LAYER_ID>( layer ) ) )
            {
                layers.insert( layer );
                m_items.emplace_back( wxT( "" ), true, static_cast<PCB_LAYER_ID>( layer ) );
            }
            else
            {
                return false;
            }
        }

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

class TEXT_ITEMS_GRID_TABLE : public wxGridTableBase
{
    bool                        m_forFieldProps;
    std::vector<TEXT_ITEM_INFO> m_items;

public:
    TEXT_ITEMS_GRID_TABLE( bool aForFieldProps ) :
            m_forFieldProps( aForFieldProps )
    {}

    int GetNumberRows() override { return m_items.size(); }
    int GetNumberCols() override { return m_forFieldProps ? 3 : 2; }

    wxString GetColLabelValue( int aCol ) override
    {
        if( m_forFieldProps )
        {
            switch( aCol )
            {
            case 0: return _( "Value" );
            case 1: return _( "Show" );
            case 2: return _( "Layer" );
            default: return wxEmptyString;
            }
        }
        else
        {
            switch( aCol )
            {
            case 0: return _( "Text Items" );
            case 1: return _( "Layer" );
            default: return wxEmptyString;
            }
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
        if( m_forFieldProps )
        {
            switch( aCol )
            {
            case 0: return aTypeName == wxGRID_VALUE_STRING;
            case 1: return aTypeName == wxGRID_VALUE_BOOL;
            case 2: return aTypeName == wxGRID_VALUE_NUMBER;
            default: wxFAIL; return false;
            }
        }
        else
        {
            switch( aCol )
            {
            case 0: return aTypeName == wxGRID_VALUE_STRING;
            case 1: return aTypeName == wxGRID_VALUE_NUMBER;
            default: wxFAIL; return false;
            }
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
            m_items[row].m_Layer = static_cast<PCB_LAYER_ID>( value );
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
    attr->SetRenderer( new GRID_CELL_LAYER_RENDERER( nullptr ) );
    attr->SetEditor( new GRID_CELL_LAYER_SELECTOR( nullptr, {} ) );
    m_textItemsGrid->SetColAttr( 1, attr );

    m_layerNameitemsGrid->SetDefaultRowSize( m_layerNameitemsGrid->GetDefaultRowSize() + 4 );

    m_layerNameitemsGrid->SetTable( new LAYER_NAMES_GRID_TABLE(), true );
    m_layerNameitemsGrid->PushEventHandler( new GRID_TRICKS( m_layerNameitemsGrid ) );
    m_layerNameitemsGrid->SetSelectionMode( wxGrid::wxGridSelectRows );

    attr = new wxGridCellAttr;
    attr->SetRenderer( new GRID_CELL_LAYER_RENDERER( nullptr ) );
    attr->SetEditor( new GRID_CELL_LAYER_SELECTOR( nullptr, LSET::AllTechMask() | LSET::AllCuMask() | Edge_Cuts | Margin ) );
    m_layerNameitemsGrid->SetColAttr( 0, attr );


}


PANEL_FP_EDITOR_FIELD_DEFAULTS::~PANEL_FP_EDITOR_FIELD_DEFAULTS()
{
    // destroy GRID_TRICKS before grids.
    m_fieldPropsGrid->PopEventHandler( true );
    m_textItemsGrid->PopEventHandler( true );
    m_layerNameitemsGrid->PopEventHandler( true );
}


void PANEL_FP_EDITOR_FIELD_DEFAULTS::loadFPSettings( const FOOTPRINT_EDITOR_SETTINGS* aCfg )
{
    // Footprint defaults
    wxGridTableBase* table = m_fieldPropsGrid->GetTable();
    table->DeleteRows( 0, m_fieldPropsGrid->GetNumberRows() );
    table->AppendRows( 2 );

    for( int i : { 0, 1 } )
    {
        TEXT_ITEM_INFO item = aCfg->m_DesignSettings.m_DefaultFPTextItems[i];

        table->SetValue( i, 0, item.m_Text );
        table->SetValueAsBool( i, 1, item.m_Visible );
        table->SetValueAsLong( i, 2, item.m_Layer );
    }

    table = m_textItemsGrid->GetTable();
    table->DeleteRows( 0, m_textItemsGrid->GetNumberRows() );

    // if aCfg->m_DesignSettings.m_DefaultFPTextItems.size() is > 2 (first and second are ref and
    // value), some extra texts must be added to the list of default texts
    int extra_texts_cnt = aCfg->m_DesignSettings.m_DefaultFPTextItems.size() - 2;

    if( extra_texts_cnt > 0 )
        table->AppendRows( extra_texts_cnt );

    for( int i = 2; i < (int) aCfg->m_DesignSettings.m_DefaultFPTextItems.size(); ++i )
    {
        TEXT_ITEM_INFO item = aCfg->m_DesignSettings.m_DefaultFPTextItems[i];

        table->SetValue( i - 2, 0, item.m_Text );
        table->SetValueAsLong( i - 2, 1, item.m_Layer );
    }

    table = m_layerNameitemsGrid->GetTable();

    for( auto& item : aCfg->m_DesignSettings.m_UserLayerNames )
    {
        wxString orig_name = item.first;
        int layer = LSET::NameToLayer( orig_name );

        if( !IsUserLayer( static_cast<PCB_LAYER_ID>( layer ) ) )
            continue;

        int row = m_layerNameitemsGrid->GetNumberRows();
        table->AppendRows( 1 );
        table->SetValueAsLong( row, 0, layer );
        table->SetValue( row, 1, item.second );
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
        m_bpAddLayer->SetBitmap( KiBitmapBundle( BITMAPS::small_plus ) );
        m_bpDeleteLayer->SetBitmap( KiBitmapBundle( BITMAPS::small_trash ) );
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

    for( int i : { 0, 1 } )
    {
        wxString text = table->GetValue( i, 0 );
        bool visible = table->GetValueAsBool( i, 1 );
        PCB_LAYER_ID layer = static_cast<PCB_LAYER_ID>( table->GetValueAsLong( i, 2 ) );

        cfg.m_DefaultFPTextItems.emplace_back( text, visible, layer );
    }

    table = m_textItemsGrid->GetTable();

    for( int i = 0; i < m_textItemsGrid->GetNumberRows(); ++i )
    {
        wxString text = table->GetValue( i, 0 );
        PCB_LAYER_ID layer = static_cast<PCB_LAYER_ID>( table->GetValueAsLong( i, 1 ) );

        cfg.m_DefaultFPTextItems.emplace_back( text, true, layer );
    }

    cfg.m_UserLayerNames.clear();
    table = m_layerNameitemsGrid->GetTable();

    for( int i = 0; i < m_layerNameitemsGrid->GetNumberRows(); ++i )
    {
        PCB_LAYER_ID layer = static_cast<PCB_LAYER_ID>( table->GetValueAsLong( i, 0 ) );
        wxString     orig_name = LSET::Name( static_cast<PCB_LAYER_ID>( layer ) );
        wxString     name = table->GetValue( i, 1 );

        if( layer >= 0 && IsUserLayer( layer ) && !name.IsEmpty() )
            cfg.m_UserLayerNames.emplace( orig_name.ToStdString(), name );
    }

    return true;
}


bool PANEL_FP_EDITOR_FIELD_DEFAULTS::isLayerAvailable( int aLayer ) const
{
    for( int i = 0; i < m_layerNameitemsGrid->GetNumberRows(); ++i )
    {
        if( m_layerNameitemsGrid->GetTable()->GetValueAsLong( i, 0 ) == aLayer )
            return false;
    }

    return true;
}


int PANEL_FP_EDITOR_FIELD_DEFAULTS::getNextAvailableLayer() const
{
    std::set<int> usedLayers;

    for( int i = 0; i < m_layerNameitemsGrid->GetNumberRows(); ++i )
        usedLayers.insert( m_layerNameitemsGrid->GetTable()->GetValueAsLong( i, 0 ) );

    for( int ii = User_1; ii < User_45; ++ii )
    {
        if( !usedLayers.contains( ii ) )
            return ii;
    }

    return -1;
}


void PANEL_FP_EDITOR_FIELD_DEFAULTS::onLayerChange( wxGridEvent& event )
{
    wxGridTableBase* table = m_layerNameitemsGrid->GetTable();

    if( event.GetCol() == 0 )
    {
        int layer = static_cast<int>( table->GetValueAsLong( event.GetRow(), 0 ) );

        for( int i = 0; i < m_layerNameitemsGrid->GetNumberRows(); ++i )
        {
            if( i != event.GetRow()
                && table->GetValueAsLong( i, 0 ) == layer )
            {
                table->SetValueAsLong( event.GetRow(), 0, getNextAvailableLayer() );
                return;
            }
        }
    }

    for( int ii = 0; ii < m_layerNameitemsGrid->GetNumberRows(); ++ii )
    {
        wxString layerName = table->GetValue( ii, 1 );

        if( ii != event.GetRow() && layerName == table->GetValue( event.GetRow(), 1 ) )
        {
            wxString msg = wxString::Format( _( "Layer name %s already in use." ), layerName );
            PAGED_DIALOG::GetDialog( this )->SetError( msg, this, m_layerNameitemsGrid, ii, 1 );
            return;
        }
    }
}


void PANEL_FP_EDITOR_FIELD_DEFAULTS::OnAddTextItem( wxCommandEvent& event )
{
    if( !m_textItemsGrid->CommitPendingChanges() )
        return;

    wxGridTableBase* table = m_textItemsGrid->GetTable();

    int newRow = m_textItemsGrid->GetNumberRows();
    table->AppendRows( 1 );
    table->SetValueAsLong( newRow, 1, table->GetValueAsLong( newRow - 1, 1 ) );

    m_textItemsGrid->MakeCellVisible( newRow, 0 );
    m_textItemsGrid->SetGridCursor( newRow, 0 );

    m_textItemsGrid->EnableCellEditControl( true );
    m_textItemsGrid->ShowCellEditControl();
}


void PANEL_FP_EDITOR_FIELD_DEFAULTS::OnAddLayerItem( wxCommandEvent& event )
{
    if( !m_layerNameitemsGrid->CommitPendingChanges() )
        return;

    wxGridTableBase* table = m_layerNameitemsGrid->GetTable();

    int newRow = m_layerNameitemsGrid->GetNumberRows();
    table->AppendRows( 1 );

    m_layerNameitemsGrid->MakeCellVisible( newRow, 0 );
    m_layerNameitemsGrid->SetGridCursor( newRow, 0 );

    m_layerNameitemsGrid->EnableCellEditControl( true );
    m_layerNameitemsGrid->ShowCellEditControl();
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


void PANEL_FP_EDITOR_FIELD_DEFAULTS::OnDeleteLayerItem( wxCommandEvent& event )
{
    wxArrayInt selectedRows = m_layerNameitemsGrid->GetSelectedRows();

    if( selectedRows.empty() && m_layerNameitemsGrid->GetGridCursorRow() >= 0 )
        selectedRows.push_back( m_layerNameitemsGrid->GetGridCursorRow() );

    if( selectedRows.empty() )
        return;

    if( !m_layerNameitemsGrid->CommitPendingChanges() )
        return;

    // Reverse sort so deleting a row doesn't change the indexes of the other rows.
    selectedRows.Sort(
            []( int* first, int* second )
            {
                return *second - *first;
            } );

    for( int row : selectedRows )
    {
        m_layerNameitemsGrid->GetTable()->DeleteRows( row, 1 );

        if( m_layerNameitemsGrid->GetNumberRows() > 0 )
        {
            m_layerNameitemsGrid->MakeCellVisible( std::max( 0, row - 1 ),
                                                  m_layerNameitemsGrid->GetGridCursorCol() );
            m_layerNameitemsGrid->SetGridCursor( std::max( 0, row - 1 ),
                                                m_layerNameitemsGrid->GetGridCursorCol() );
        }
    }
}


void PANEL_FP_EDITOR_FIELD_DEFAULTS::ResetPanel()
{
    FOOTPRINT_EDITOR_SETTINGS cfg;
    cfg.Load(); // Loading without a file will init to defaults

    loadFPSettings( &cfg );
}
