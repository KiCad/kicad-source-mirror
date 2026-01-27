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

#include "panel_fp_user_layer_names.h"

#include <pgm_base.h>
#include <settings/settings_manager.h>
#include <footprint_editor_settings.h>
#include <widgets/paged_dialog.h>
#include <template_fieldnames.h>
#include <widgets/std_bitmap_button.h>
#include <grid_tricks.h>
#include <grid_layer_box_helpers.h>
#include <board_design_settings.h>
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
        case 0:  return _( "Layer" );
        case 1:  return _( "Name" );
        default: return wxEmptyString;
        }
    }

    bool CanGetValueAs( int aRow, int aCol, const wxString& aTypeName ) override
    {
        switch( aCol )
        {
        case 0:  return aTypeName == wxGRID_VALUE_NUMBER;
        case 1:  return aTypeName == wxGRID_VALUE_STRING;
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


static FOOTPRINT_EDITOR_SETTINGS& GetPgmSettings()
{
    return *GetAppSettings<FOOTPRINT_EDITOR_SETTINGS>( "fpedit" );
}


PANEL_FP_USER_LAYER_NAMES::PANEL_FP_USER_LAYER_NAMES( wxWindow* aParent ) :
        PANEL_FP_USER_LAYER_NAMES_BASE( aParent ),
        m_designSettings( GetPgmSettings().m_DesignSettings )
{
    m_layerNamesGrid->SetDefaultRowSize( m_layerNamesGrid->GetDefaultRowSize() + 4 );

    m_layerNamesGrid->SetTable( new LAYER_NAMES_GRID_TABLE(), true );
    m_layerNamesGrid->PushEventHandler( new GRID_TRICKS( m_layerNamesGrid ) );
    m_layerNamesGrid->SetSelectionMode( wxGrid::wxGridSelectRows );

    wxGridCellAttr* attr = new wxGridCellAttr;
    attr->SetRenderer( new GRID_CELL_LAYER_RENDERER( nullptr ) );
    LSET forbiddenLayers = LSET::AllCuMask() | LSET::AllTechMask();
    forbiddenLayers.set( Edge_Cuts );
    forbiddenLayers.set( Margin );
    attr->SetEditor( new GRID_CELL_LAYER_SELECTOR( nullptr, forbiddenLayers ) );
    m_layerNamesGrid->SetColAttr( 0, attr );

    attr = new wxGridCellAttr;
    m_layerNamesGrid->SetColAttr( 1, attr );

    m_bpAdd->SetBitmap( KiBitmapBundle( BITMAPS::small_plus ) );
    m_bpDelete->SetBitmap( KiBitmapBundle( BITMAPS::small_trash ) );
}


PANEL_FP_USER_LAYER_NAMES::~PANEL_FP_USER_LAYER_NAMES()
{
    m_layerNamesGrid->PopEventHandler( true );
}


void PANEL_FP_USER_LAYER_NAMES::loadFPSettings( const FOOTPRINT_EDITOR_SETTINGS* aCfg )
{
    int userLayerCount = aCfg->m_DesignSettings.GetUserDefinedLayerCount();

    if( userLayerCount >= 0 && userLayerCount < (int) m_choiceUserLayers->GetCount() )
        m_choiceUserLayers->SetSelection( userLayerCount );
    else
        m_choiceUserLayers->SetSelection( 0 );

    wxGridTableBase* table = m_layerNamesGrid->GetTable();
    table->DeleteRows( 0, m_layerNamesGrid->GetNumberRows() );

    for( const auto& [canonicalName, userName] : aCfg->m_DesignSettings.m_UserLayerNames )
    {
        wxString orig_name = canonicalName;
        int layer = LSET::NameToLayer( orig_name );

        if( !IsUserLayer( static_cast<PCB_LAYER_ID>( layer ) ) )
            continue;

        int row = m_layerNamesGrid->GetNumberRows();
        table->AppendRows( 1 );
        table->SetValueAsLong( row, 0, layer );
        table->SetValue( row, 1, userName );
    }

    Layout();
}


bool PANEL_FP_USER_LAYER_NAMES::TransferDataToWindow()
{
    const FOOTPRINT_EDITOR_SETTINGS& cfg = GetPgmSettings();

    loadFPSettings( &cfg );

    return true;
}


bool PANEL_FP_USER_LAYER_NAMES::Show( bool aShow )
{
    bool retVal = wxPanel::Show( aShow );

    if( aShow )
    {
        m_bpAdd->SetBitmap( KiBitmapBundle( BITMAPS::small_plus ) );
        m_bpDelete->SetBitmap( KiBitmapBundle( BITMAPS::small_trash ) );
    }

    return retVal;
}


bool PANEL_FP_USER_LAYER_NAMES::TransferDataFromWindow()
{
    if( !m_layerNamesGrid->CommitPendingChanges() )
        return false;

    BOARD_DESIGN_SETTINGS& cfg = m_designSettings;

    int userLayerCount = m_choiceUserLayers->GetSelection();
    cfg.SetUserDefinedLayerCount( userLayerCount );

    cfg.m_UserLayerNames.clear();
    wxGridTableBase* table = m_layerNamesGrid->GetTable();

    for( int i = 0; i < m_layerNamesGrid->GetNumberRows(); ++i )
    {
        PCB_LAYER_ID layer = static_cast<PCB_LAYER_ID>( table->GetValueAsLong( i, 0 ) );
        wxString     orig_name = LSET::Name( static_cast<PCB_LAYER_ID>( layer ) );
        wxString     name = table->GetValue( i, 1 );

        if( layer >= 0 && IsUserLayer( layer ) && !name.IsEmpty() )
            cfg.m_UserLayerNames.emplace( orig_name.ToStdString(), name );
    }

    return true;
}


void PANEL_FP_USER_LAYER_NAMES::onUserLayerCountChange( wxCommandEvent& event )
{
    // Nothing to do here - just update the selection. The value is saved in TransferDataFromWindow.
}


void PANEL_FP_USER_LAYER_NAMES::onLayerChange( wxGridEvent& event )
{
    wxGridTableBase* table = m_layerNamesGrid->GetTable();

    if( event.GetCol() == 0 )
    {
        int layer = static_cast<int>( table->GetValueAsLong( event.GetRow(), 0 ) );

        for( int i = 0; i < m_layerNamesGrid->GetNumberRows(); ++i )
        {
            if( i != event.GetRow() && table->GetValueAsLong( i, 0 ) == layer )
            {
                table->SetValueAsLong( event.GetRow(), 0, getNextAvailableLayer() );
                return;
            }
        }
    }

    for( int ii = 0; ii < m_layerNamesGrid->GetNumberRows(); ++ii )
    {
        wxString layerName = table->GetValue( ii, 1 );

        if( ii != event.GetRow() && layerName == table->GetValue( event.GetRow(), 1 ) )
        {
            wxString msg = wxString::Format( _( "Layer name %s already in use." ), layerName );
            PAGED_DIALOG::GetDialog( this )->SetError( msg, this, m_layerNamesGrid, ii, 1 );
            return;
        }
    }
}


int PANEL_FP_USER_LAYER_NAMES::getNextAvailableLayer() const
{
    std::set<int> usedLayers;

    for( int i = 0; i < m_layerNamesGrid->GetNumberRows(); ++i )
        usedLayers.insert( (int) m_layerNamesGrid->GetTable()->GetValueAsLong( i, 0 ) );

    for( int ii = User_1; ii < User_45; ++ii )
    {
        if( !usedLayers.contains( ii ) )
            return ii;
    }

    return -1;
}


void PANEL_FP_USER_LAYER_NAMES::OnAddLayerItem( wxCommandEvent& event )
{
    m_layerNamesGrid->OnAddRow(
            [&]() -> std::pair<int, int>
            {
                m_layerNamesGrid->GetTable()->AppendRows( 1 );
                return { m_layerNamesGrid->GetNumberRows() - 1, -1 };
            } );
}


void PANEL_FP_USER_LAYER_NAMES::OnDeleteLayerItem( wxCommandEvent& event )
{
    m_layerNamesGrid->OnDeleteRows(
            [&]( int row )
            {
                m_layerNamesGrid->GetTable()->DeleteRows( row, 1 );
            } );
}


void PANEL_FP_USER_LAYER_NAMES::ResetPanel()
{
    FOOTPRINT_EDITOR_SETTINGS cfg;
    cfg.Load();

    loadFPSettings( &cfg );
}
