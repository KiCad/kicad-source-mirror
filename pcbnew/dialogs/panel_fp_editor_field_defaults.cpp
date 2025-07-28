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
        if( col == GetNumberCols() - 1 )    // only last column uses a long value
                                            // (probably useless test)
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
    return *GetAppSettings<FOOTPRINT_EDITOR_SETTINGS>( "fpedit" );
}


PANEL_FP_EDITOR_FIELD_DEFAULTS::PANEL_FP_EDITOR_FIELD_DEFAULTS( wxWindow* aParent ) :
        PANEL_FP_EDITOR_FIELD_DEFAULTS_BASE( aParent ),
        m_designSettings( GetPgmSettings().m_DesignSettings )
{
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

    m_textItemsGrid->SetTable( new TEXT_ITEMS_GRID_TABLE( false ), true );
    m_textItemsGrid->PushEventHandler( new GRID_TRICKS( m_textItemsGrid ) );
    m_textItemsGrid->SetSelectionMode( wxGrid::wxGridSelectRows );

    attr = new wxGridCellAttr;
    attr->SetRenderer( new GRID_CELL_LAYER_RENDERER( nullptr ) );
    attr->SetEditor( new GRID_CELL_LAYER_SELECTOR( nullptr, {} ) );
    m_textItemsGrid->SetColAttr( 1, attr );
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
    wxGridTableBase* table = m_fieldPropsGrid->GetTable();
    table->DeleteRows( 0, m_fieldPropsGrid->GetNumberRows() );
    table->AppendRows( 2 );

    for( int i = 0; i < std::min<int>( 2, (int) aCfg->m_DesignSettings.m_DefaultFPTextItems.size() ); ++i )
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
    int extra_texts_cnt = (int) aCfg->m_DesignSettings.m_DefaultFPTextItems.size() - 2;

    if( extra_texts_cnt > 0 )
        table->AppendRows( extra_texts_cnt );

    for( int i = 2; i < (int) aCfg->m_DesignSettings.m_DefaultFPTextItems.size(); ++i )
    {
        TEXT_ITEM_INFO item = aCfg->m_DesignSettings.m_DefaultFPTextItems[i];

        table->SetValue( i - 2, 0, item.m_Text );
        table->SetValueAsLong( i - 2, 1, item.m_Layer );
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

    return true;
}


void PANEL_FP_EDITOR_FIELD_DEFAULTS::OnAddTextItem( wxCommandEvent& event )
{
    m_textItemsGrid->OnAddRow(
            [&]() -> std::pair<int, int>
            {
                wxGridTableBase* table = m_textItemsGrid->GetTable();

                int newRow = m_textItemsGrid->GetNumberRows();
                table->AppendRows( 1 );

                long defaultBoardLayer = F_SilkS;

                if( newRow > 0 )
                     defaultBoardLayer = table->GetValueAsLong( newRow - 1, 1 );

                table->SetValueAsLong( newRow, 1, defaultBoardLayer );

                return { newRow, 0 };
            } );
}


void PANEL_FP_EDITOR_FIELD_DEFAULTS::OnDeleteTextItem( wxCommandEvent& event )
{
    m_textItemsGrid->OnDeleteRows(
            [&]( int row )
            {
                m_textItemsGrid->GetTable()->DeleteRows( row, 1 );
            } );
}


void PANEL_FP_EDITOR_FIELD_DEFAULTS::ResetPanel()
{
    FOOTPRINT_EDITOR_SETTINGS cfg;
    cfg.Load(); // Loading without a file will init to defaults

    loadFPSettings( &cfg );
}
