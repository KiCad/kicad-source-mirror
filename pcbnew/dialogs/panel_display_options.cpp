/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015 Jean-Pierre Charras, jean-pierre.charras at wanadoo.fr
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <pgm_base.h>
#include <settings/settings_manager.h>
#include <pcbnew_settings.h>
#include <config_map.h>
#include <panel_display_options.h>
#include <widgets/paged_dialog.h>
#include <dialogs/panel_gal_options.h>
#include <widgets/std_bitmap_button.h>
#include <grid_tricks.h>
#include <board_design_settings.h>
#include <grid_layer_box_helpers.h>
#include <footprint_editor_settings.h>


static const UTIL::CFG_MAP<TRACK_CLEARANCE_MODE> clearanceModeMap =
{
    { SHOW_WITH_VIA_WHILE_ROUTING,             2 },     // Default
    { DO_NOT_SHOW_CLEARANCE,                   0 },
    { SHOW_WHILE_ROUTING,                      1 },
    { SHOW_WITH_VIA_WHILE_ROUTING_OR_DRAGGING, 3 },
    { SHOW_WITH_VIA_ALWAYS,                    4 },
};


class LAYER_NAMES_GRID_TABLE : public WX_GRID_TABLE_BASE
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


PANEL_DISPLAY_OPTIONS::PANEL_DISPLAY_OPTIONS( wxWindow* aParent, APP_SETTINGS_BASE* aAppSettings ) :
        PANEL_DISPLAY_OPTIONS_BASE( aParent ),
        m_isPCBEdit( dynamic_cast<PCBNEW_SETTINGS*>( aAppSettings ) != nullptr )
{
    m_galOptsPanel = new PANEL_GAL_OPTIONS( this, aAppSettings );
    m_galOptionsSizer->Add( m_galOptsPanel, 1, wxEXPAND|wxRIGHT, 5 );

    m_optionsBook->SetSelection( m_isPCBEdit ? 1 : 0 );

    m_layerNameitemsGrid->SetTable( new LAYER_NAMES_GRID_TABLE(), true );
    m_layerNameitemsGrid->PushEventHandler( new GRID_TRICKS( m_layerNameitemsGrid ) );
    m_layerNameitemsGrid->SetSelectionMode( wxGrid::wxGridSelectRows );

    wxGridCellAttr* attr = new wxGridCellAttr;
    attr->SetRenderer( new GRID_CELL_LAYER_RENDERER( nullptr ) );
    LSET forbiddenLayers = LSET::AllCuMask() | LSET::AllTechMask();
    forbiddenLayers.set( Edge_Cuts );
    forbiddenLayers.set( Margin );
    attr->SetEditor( new GRID_CELL_LAYER_SELECTOR( nullptr, forbiddenLayers ) );
    m_layerNameitemsGrid->SetColAttr( 0, attr );

    m_bpAddLayer->SetBitmap( KiBitmapBundle( BITMAPS::small_plus ) );
    m_bpDeleteLayer->SetBitmap( KiBitmapBundle( BITMAPS::small_trash ) );

    // I wish I knew why these were needed here and not anywhere else.  Something to do with being
    // inside a notebook page that starts off hidden?  Anyway: no hacky code -> no worky.
    m_bpAddLayer->SetSize( m_bpAddLayer->GetMinSize() );
    m_bpDeleteLayer->SetSize( m_bpDeleteLayer->GetMinSize() );
    Layout();
}


PANEL_DISPLAY_OPTIONS::~PANEL_DISPLAY_OPTIONS()
{
    // destroy GRID_TRICKS before grids.
    m_layerNameitemsGrid->PopEventHandler( true );
}


void PANEL_DISPLAY_OPTIONS::loadFPSettings( const FOOTPRINT_EDITOR_SETTINGS* aCfg )
{
    wxGridTableBase* table = m_layerNameitemsGrid->GetTable();

    for( const auto& [canonicalName, userName] : aCfg->m_DesignSettings.m_UserLayerNames )
    {
        wxString orig_name = canonicalName;
        int layer = LSET::NameToLayer( orig_name );

        if( !IsUserLayer( static_cast<PCB_LAYER_ID>( layer ) ) )
            continue;

        int row = m_layerNameitemsGrid->GetNumberRows();
        table->AppendRows( 1 );
        table->SetValueAsLong( row, 0, layer );
        table->SetValue( row, 1, userName );
    }

    Layout();
}


void PANEL_DISPLAY_OPTIONS::loadPCBSettings( PCBNEW_SETTINGS* aCfg )
{
    int i = UTIL::GetConfigForVal( clearanceModeMap, aCfg->m_Display.m_TrackClearance );
    m_OptDisplayTracksClearance->SetSelection( i );

    m_OptDisplayPadClearence->SetValue( aCfg->m_Display.m_PadClearance );
    m_OptUseViaColorForNormalTHPadstacks->SetValue( aCfg->m_Display.m_UseViaColorForNormalTHPadstacks );
    m_OptDisplayPadNumber->SetValue( aCfg->m_ViewersDisplay.m_DisplayPadNumbers );
    m_ShowNetNamesOption->SetSelection( aCfg->m_Display.m_NetNames );
    m_checkForceShowFieldsWhenFPSelected->SetValue( aCfg->m_Display.m_ForceShowFieldsWhenFPSelected );
    m_live3Drefresh->SetValue( aCfg->m_Display.m_Live3DRefresh );
    m_checkCrossProbeOnSelection->SetValue( aCfg->m_CrossProbing.on_selection );
    m_checkCrossProbeCenter->SetValue( aCfg->m_CrossProbing.center_on_items );
    m_checkCrossProbeZoom->SetValue( aCfg->m_CrossProbing.zoom_to_fit );
    m_checkCrossProbeAutoHighlight->SetValue( aCfg->m_CrossProbing.auto_highlight );
    m_checkCrossProbeFlash->SetValue( aCfg->m_CrossProbing.flash_selection );
}


bool PANEL_DISPLAY_OPTIONS::Show( bool aShow )
{
    bool retVal = wxPanel::Show( aShow );

    if( aShow )
    {
        // These *should* work in the constructor, and indeed they do if this panel is the
        // first displayed.  However, on OSX 3.0.5 (at least), if another panel is displayed
        // first then the icons will be blank unless they're set here.
        m_bpAddLayer->SetBitmap( KiBitmapBundle( BITMAPS::small_plus ) );
        m_bpDeleteLayer->SetBitmap( KiBitmapBundle( BITMAPS::small_trash ) );
        Layout();
    }

    return retVal;
}


bool PANEL_DISPLAY_OPTIONS::TransferDataToWindow()
{
    if( m_isPCBEdit )
        loadPCBSettings( GetAppSettings<PCBNEW_SETTINGS>( "pcbnew" ) );
    else
        loadFPSettings( GetAppSettings<FOOTPRINT_EDITOR_SETTINGS>( "fpedit" ) );

    m_galOptsPanel->TransferDataToWindow();

    return true;
}


int PANEL_DISPLAY_OPTIONS::getNextAvailableLayer() const
{
    std::set<int> usedLayers;

    for( int i = 0; i < m_layerNameitemsGrid->GetNumberRows(); ++i )
        usedLayers.insert( (int) m_layerNameitemsGrid->GetTable()->GetValueAsLong( i, 0 ) );

    for( int ii = User_1; ii < User_45; ++ii )
    {
        if( !usedLayers.contains( ii ) )
            return ii;
    }

    return -1;
}


void PANEL_DISPLAY_OPTIONS::onLayerChange( wxGridEvent& event )
{
    wxGridTableBase* table = m_layerNameitemsGrid->GetTable();

    if( event.GetCol() == 0 )
    {
        int layer = static_cast<int>( table->GetValueAsLong( event.GetRow(), 0 ) );

        for( int i = 0; i < m_layerNameitemsGrid->GetNumberRows(); ++i )
        {
            if( i != event.GetRow() && table->GetValueAsLong( i, 0 ) == layer )
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


void PANEL_DISPLAY_OPTIONS::OnAddLayerItem( wxCommandEvent& event )
{
    m_layerNameitemsGrid->OnAddRow(
            [&]() -> std::pair<int, int>
            {
                m_layerNameitemsGrid->GetTable()->AppendRows( 1 );
                return { m_layerNameitemsGrid->GetNumberRows() - 1, -1 };
            } );
}


void PANEL_DISPLAY_OPTIONS::OnDeleteLayerItem( wxCommandEvent& event )
{
    m_layerNameitemsGrid->OnDeleteRows(
            [&]( int row )
            {
                m_layerNameitemsGrid->GetTable()->DeleteRows( row, 1 );
            } );
}


/*
 * Update variables with new options
 */
bool PANEL_DISPLAY_OPTIONS::TransferDataFromWindow()
{
    if( !m_layerNameitemsGrid->CommitPendingChanges() )
        return false;

    m_galOptsPanel->TransferDataFromWindow();

    if( m_isPCBEdit )
    {
        if( PCBNEW_SETTINGS* cfg = GetAppSettings<PCBNEW_SETTINGS>( "pcbnew" ) )
        {
            int i = m_OptDisplayTracksClearance->GetSelection();
            cfg->m_Display.m_TrackClearance = UTIL::GetValFromConfig( clearanceModeMap, i );

            cfg->m_Display.m_PadClearance = m_OptDisplayPadClearence->GetValue();
            cfg->m_Display.m_UseViaColorForNormalTHPadstacks = m_OptUseViaColorForNormalTHPadstacks->GetValue();
            cfg->m_ViewersDisplay.m_DisplayPadNumbers = m_OptDisplayPadNumber->GetValue();
            cfg->m_Display.m_NetNames = m_ShowNetNamesOption->GetSelection();
            cfg->m_Display.m_ForceShowFieldsWhenFPSelected = m_checkForceShowFieldsWhenFPSelected->GetValue();
            cfg->m_Display.m_Live3DRefresh = m_live3Drefresh->GetValue();
            cfg->m_CrossProbing.on_selection = m_checkCrossProbeOnSelection->GetValue();
            cfg->m_CrossProbing.center_on_items = m_checkCrossProbeCenter->GetValue();
            cfg->m_CrossProbing.zoom_to_fit = m_checkCrossProbeZoom->GetValue();
            cfg->m_CrossProbing.auto_highlight = m_checkCrossProbeAutoHighlight->GetValue();
            cfg->m_CrossProbing.flash_selection = m_checkCrossProbeFlash->GetValue();
        }
    }
    else
    {
        if( FOOTPRINT_EDITOR_SETTINGS* cfg = GetAppSettings<FOOTPRINT_EDITOR_SETTINGS>( "fpedit" ) )
        {
            cfg->m_DesignSettings.m_UserLayerNames.clear();
            wxGridTableBase* table = m_layerNameitemsGrid->GetTable();

            for( int i = 0; i < m_layerNameitemsGrid->GetNumberRows(); ++i )
            {
                PCB_LAYER_ID layer = static_cast<PCB_LAYER_ID>( table->GetValueAsLong( i, 0 ) );
                wxString     orig_name = LSET::Name( static_cast<PCB_LAYER_ID>( layer ) );
                wxString     name = table->GetValue( i, 1 );

                if( layer >= 0 && IsUserLayer( layer ) && !name.IsEmpty() )
                    cfg->m_DesignSettings.m_UserLayerNames.emplace( orig_name.ToStdString(), name );
            }
        }
    }

    return true;
}


void PANEL_DISPLAY_OPTIONS::ResetPanel()
{
    if( m_isPCBEdit )
    {
        PCBNEW_SETTINGS cfg;
        cfg.Load();             // Loading without a file will init to defaults

        loadPCBSettings( &cfg );
        m_galOptsPanel->ResetPanel( &cfg );
    }
    else
    {
        FOOTPRINT_EDITOR_SETTINGS cfg;
        cfg.Load();             // Loading without a file will init to defaults

        loadFPSettings( &cfg );
        m_galOptsPanel->ResetPanel( &cfg );
    }
}


