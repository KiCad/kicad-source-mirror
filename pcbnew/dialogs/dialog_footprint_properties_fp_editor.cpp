/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2018 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2015 Dick Hollenbeck, dick@softplc.com
 * Copyright (C) 2008 Wayne Stambaugh <stambaughw@gmail.com>
 * Copyright (C) 2004-2022 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <confirm.h>
#include <dialogs/dialog_text_entry.h>
#include <3d_viewer/eda_3d_viewer_frame.h>
#include <validators.h>
#include <board_design_settings.h>
#include <board_commit.h>
#include <bitmaps.h>
#include <kiplatform/ui.h>
#include <widgets/grid_text_button_helpers.h>
#include <widgets/wx_grid.h>
#include <widgets/std_bitmap_button.h>
#include <widgets/text_ctrl_eval.h>
#include <footprint.h>
#include <footprint_edit_frame.h>
#include <footprint_editor_settings.h>
#include <dialog_footprint_properties_fp_editor.h>
#include <panel_fp_properties_3d_model.h>
#include "3d_rendering/opengl/3d_model.h"
#include "filename_resolver.h"
#include <pgm_base.h>
#include <pcbnew.h>
#include "dialogs/panel_preview_3d_model.h"
#include "dialogs/3d_cache_dialogs.h"
#include <settings/settings_manager.h>
#include <tool/tool_manager.h>
#include <tools/pcb_selection_tool.h>
#include <grid_layer_box_helpers.h>

#include <fp_lib_table.h>

PRIVATE_LAYERS_GRID_TABLE::PRIVATE_LAYERS_GRID_TABLE( PCB_BASE_FRAME* aFrame ) :
        m_frame( aFrame )
{
    m_layerColAttr = new wxGridCellAttr;
    m_layerColAttr->SetRenderer( new GRID_CELL_LAYER_RENDERER( m_frame ) );

    LSET forbiddenLayers = LSET::AllCuMask() | LSET::AllTechMask();
    forbiddenLayers.set( Edge_Cuts );
    forbiddenLayers.set( Margin );
    m_layerColAttr->SetEditor( new GRID_CELL_LAYER_SELECTOR( m_frame, forbiddenLayers ) );
}


PRIVATE_LAYERS_GRID_TABLE::~PRIVATE_LAYERS_GRID_TABLE()
{
    m_layerColAttr->DecRef();
}


bool PRIVATE_LAYERS_GRID_TABLE::CanGetValueAs( int aRow, int aCol, const wxString& aTypeName )
{
    return aTypeName == wxGRID_VALUE_NUMBER;
}


bool PRIVATE_LAYERS_GRID_TABLE::CanSetValueAs( int aRow, int aCol, const wxString& aTypeName )
{
    return aTypeName == wxGRID_VALUE_NUMBER;
}


wxGridCellAttr* PRIVATE_LAYERS_GRID_TABLE::GetAttr( int aRow, int aCol,
                                                    wxGridCellAttr::wxAttrKind  )
{
    m_layerColAttr->IncRef();
    return m_layerColAttr;
}


wxString PRIVATE_LAYERS_GRID_TABLE::GetValue( int aRow, int aCol )
{
    return m_frame->GetBoard()->GetLayerName( this->at( (size_t) aRow ) );
}


long PRIVATE_LAYERS_GRID_TABLE::GetValueAsLong( int aRow, int aCol )
{
    return this->at( (size_t) aRow );
}


void PRIVATE_LAYERS_GRID_TABLE::SetValue( int aRow, int aCol, const wxString &aValue )
{
    wxFAIL_MSG( wxString::Format( wxT( "column %d doesn't hold a string value" ), aCol ) );
}


void PRIVATE_LAYERS_GRID_TABLE::SetValueAsLong( int aRow, int aCol, long aValue )
{
    this->at( (size_t) aRow ) = ToLAYER_ID( (int) aValue );
}


// Remember the last open page during session.

NOTEBOOK_PAGES DIALOG_FOOTPRINT_PROPERTIES_FP_EDITOR::m_page = NOTEBOOK_PAGES::PAGE_GENERAL;


DIALOG_FOOTPRINT_PROPERTIES_FP_EDITOR::DIALOG_FOOTPRINT_PROPERTIES_FP_EDITOR(
        FOOTPRINT_EDIT_FRAME* aParent,
        FOOTPRINT* aFootprint ) :
    DIALOG_FOOTPRINT_PROPERTIES_FP_EDITOR_BASE( aParent ),
    m_frame( aParent ),
    m_footprint( aFootprint ),
    m_netClearance( aParent, m_NetClearanceLabel, m_NetClearanceCtrl, m_NetClearanceUnits ),
    m_solderMask( aParent, m_SolderMaskMarginLabel, m_SolderMaskMarginCtrl,
                  m_SolderMaskMarginUnits ),
    m_solderPaste( aParent, m_SolderPasteMarginLabel, m_SolderPasteMarginCtrl,
                   m_SolderPasteMarginUnits ),
    m_solderPasteRatio( aParent, m_PasteMarginRatioLabel, m_PasteMarginRatioCtrl,
                        m_PasteMarginRatioUnits ),
    m_gridSize( 0, 0 ),
    m_lastRequestedSize( 0, 0 )
{
    SetEvtHandlerEnabled( false );
    // Create the 3D models page
    m_3dPanel = new PANEL_FP_PROPERTIES_3D_MODEL( m_frame, m_footprint, this, m_NoteBook );
    m_NoteBook->AddPage( m_3dPanel, _("3D Models"), false );

    m_texts = new FP_TEXT_GRID_TABLE( m_frame );
    m_privateLayers = new PRIVATE_LAYERS_GRID_TABLE( m_frame );

    m_delayedErrorMessage = wxEmptyString;
    m_delayedFocusCtrl = nullptr;
    m_delayedFocusGrid = nullptr;
    m_delayedFocusRow = -1;
    m_delayedFocusColumn = -1;
    m_delayedFocusPage = NOTEBOOK_PAGES::PAGE_UNKNOWN;

    // Give an icon
    wxIcon icon;
    icon.CopyFromBitmap( KiBitmap( BITMAPS::icon_modedit ) );
    SetIcon( icon );

    // Give a bit more room for combobox editors
    m_itemsGrid->SetDefaultRowSize( m_itemsGrid->GetDefaultRowSize() + 4 );
    m_privateLayersGrid->SetDefaultRowSize( m_privateLayersGrid->GetDefaultRowSize() + 4 );

    m_itemsGrid->SetTable( m_texts );
    m_privateLayersGrid->SetTable( m_privateLayers );

    m_itemsGrid->PushEventHandler( new GRID_TRICKS( m_itemsGrid ) );
    m_privateLayersGrid->PushEventHandler( new GRID_TRICKS( m_privateLayersGrid,
                                                            [this]( wxCommandEvent& aEvent )
                                                            {
                                                                OnAddLayer( aEvent );
                                                            } ) );
    m_padGroupsGrid->PushEventHandler( new GRID_TRICKS( m_padGroupsGrid,
                                                        [this]( wxCommandEvent& aEvent )
                                                        {
                                                            OnAddPadGroup( aEvent );
                                                        } ) );

    m_itemsGrid->SetSelectionMode( wxGrid::wxGridSelectRows );
    m_privateLayersGrid->SetSelectionMode( wxGrid::wxGridSelectRows );
    m_padGroupsGrid->SetSelectionMode( wxGrid::wxGridSelectRows );

    // Show/hide columns according to the user's preference
    m_itemsGrid->ShowHideColumns( m_frame->GetSettings()->m_FootprintTextShownColumns );

    m_FootprintNameCtrl->SetValidator( FOOTPRINT_NAME_VALIDATOR() );

    // Set font sizes
    wxFont infoFont = KIUI::GetInfoFont( this );
    infoFont.SetStyle( wxFONTSTYLE_ITALIC );
    m_staticTextInfoCopper->SetFont( infoFont );
    m_staticTextInfoPaste->SetFont( infoFont );

    if( static_cast<int>( m_page ) >= 0 )
        m_NoteBook->SetSelection( (unsigned) m_page );

    if( m_page == NOTEBOOK_PAGES::PAGE_GENERAL )
    {
        m_delayedFocusGrid = m_itemsGrid;
        m_delayedFocusRow = 0;
        m_delayedFocusColumn = 0;
        m_delayedFocusPage = NOTEBOOK_PAGES::PAGE_GENERAL;
    }
    else if( m_page == NOTEBOOK_PAGES::PAGE_CLEARANCES )
    {
        SetInitialFocus( m_NetClearanceCtrl );
    }

    m_solderPaste.SetNegativeZero();

    m_solderPasteRatio.SetUnits( EDA_UNITS::PERCENT );
    m_solderPasteRatio.SetNegativeZero();

    // Configure button logos
    m_bpAdd->SetBitmap( KiBitmap( BITMAPS::small_plus ) );
    m_bpDelete->SetBitmap( KiBitmap( BITMAPS::small_trash ) );
    m_bpAddLayer->SetBitmap( KiBitmap( BITMAPS::small_plus ) );
    m_bpDeleteLayer->SetBitmap( KiBitmap( BITMAPS::small_trash ) );
    m_bpAddPadGroup->SetBitmap( KiBitmap( BITMAPS::small_plus ) );
    m_bpRemovePadGroup->SetBitmap( KiBitmap( BITMAPS::small_trash ) );

    SetupStandardButtons();

    finishDialogSettings();
    SetEvtHandlerEnabled( true );
}


DIALOG_FOOTPRINT_PROPERTIES_FP_EDITOR::~DIALOG_FOOTPRINT_PROPERTIES_FP_EDITOR()
{
    if( FOOTPRINT_EDITOR_SETTINGS* cfg = m_frame->GetSettings() )
        cfg->m_FootprintTextShownColumns = m_itemsGrid->GetShownColumnsAsString();

    // Prevents crash bug in wxGrid's d'tor
    m_itemsGrid->DestroyTable( m_texts );
    m_privateLayersGrid->DestroyTable( m_privateLayers );

    // Delete the GRID_TRICKS.
    m_itemsGrid->PopEventHandler( true );
    m_privateLayersGrid->PopEventHandler( true );
    m_padGroupsGrid->PopEventHandler( true );

    m_page = static_cast<NOTEBOOK_PAGES>( m_NoteBook->GetSelection() );

    // the GL canvas on the 3D models page has to be visible before it is destroyed
    m_NoteBook->SetSelection( static_cast<int>( NOTEBOOK_PAGES::PAGE_3D_MODELS ) );
}


bool DIALOG_FOOTPRINT_PROPERTIES_FP_EDITOR::TransferDataToWindow()
{
    LIB_ID   fpID          = m_footprint->GetFPID();
    wxString footprintName = fpID.GetLibItemName();

    m_FootprintNameCtrl->ChangeValue( footprintName );

    m_DocCtrl->SetValue( m_footprint->GetDescription() );
    m_KeywordCtrl->SetValue( m_footprint->GetKeywords() );

    if( !wxDialog::TransferDataToWindow() )
        return false;

    if( !m_PanelGeneral->TransferDataToWindow() )
        return false;

    // Add the models to the panel
    if( !m_3dPanel->TransferDataToWindow() )
        return false;

    // Footprint Texts
    m_texts->push_back( m_footprint->Reference() );
    m_texts->push_back( m_footprint->Value() );

    for( BOARD_ITEM* item : m_footprint->GraphicalItems() )
    {
        if( PCB_TEXT* textItem = dynamic_cast<PCB_TEXT*>( item) )
            m_texts->push_back( *textItem );
    }

    // Notify the grid
    wxGridTableMessage tmsg( m_texts, wxGRIDTABLE_NOTIFY_ROWS_APPENDED, m_texts->GetNumberRows() );
    m_itemsGrid->ProcessTableMessage( tmsg );

    if( m_footprint->GetAttributes() & FP_THROUGH_HOLE )
        m_componentType->SetSelection( 0 );
    else if( m_footprint->GetAttributes() & FP_SMD )
        m_componentType->SetSelection( 1 );
    else
        m_componentType->SetSelection( 2 );

    // Private layers
    for( PCB_LAYER_ID privateLayer : m_footprint->GetPrivateLayers().UIOrder() )
        m_privateLayers->push_back( privateLayer );

    // Notify the grid
    wxGridTableMessage gridTableMessagesg( m_privateLayers, wxGRIDTABLE_NOTIFY_ROWS_APPENDED,
                                           m_privateLayers->GetNumberRows() );
    m_privateLayersGrid->ProcessTableMessage( gridTableMessagesg );

    m_boardOnly->SetValue( m_footprint->GetAttributes() & FP_BOARD_ONLY );
    m_excludeFromPosFiles->SetValue( m_footprint->GetAttributes() & FP_EXCLUDE_FROM_POS_FILES );
    m_excludeFromBOM->SetValue( m_footprint->GetAttributes() & FP_EXCLUDE_FROM_BOM );
    m_noCourtyards->SetValue( m_footprint->GetAttributes() & FP_ALLOW_MISSING_COURTYARD );
    m_cbDNP->SetValue( m_footprint->GetAttributes() & FP_DNP );

    // Local Clearances

    m_netClearance.SetValue( m_footprint->GetLocalClearance() );
    m_solderMask.SetValue( m_footprint->GetLocalSolderMaskMargin() );
    m_solderPaste.SetValue( m_footprint->GetLocalSolderPasteMargin() );
    m_solderPasteRatio.SetDoubleValue( m_footprint->GetLocalSolderPasteMarginRatio() * 100.0 );
    m_allowBridges->SetValue( m_footprint->GetAttributes() & FP_ALLOW_SOLDERMASK_BRIDGES );

    switch( m_footprint->GetZoneConnection() )
    {
    default:
    case ZONE_CONNECTION::INHERITED: m_ZoneConnectionChoice->SetSelection( 0 ); break;
    case ZONE_CONNECTION::FULL:      m_ZoneConnectionChoice->SetSelection( 1 ); break;
    case ZONE_CONNECTION::THERMAL:   m_ZoneConnectionChoice->SetSelection( 2 ); break;
    case ZONE_CONNECTION::NONE:      m_ZoneConnectionChoice->SetSelection( 3 ); break;
    }

    for( const wxString& group : m_footprint->GetNetTiePadGroups() )
    {
        if( !group.IsEmpty() )
        {
            m_padGroupsGrid->AppendRows( 1 );
            m_padGroupsGrid->SetCellValue( m_padGroupsGrid->GetNumberRows() - 1, 0, group );
        }
    }

    // Items grid
    for( int col = 0; col < m_itemsGrid->GetNumberCols(); col++ )
    {
        // Adjust min size to the column label size
        m_itemsGrid->SetColMinimalWidth( col, m_itemsGrid->GetVisibleWidth( col, true, false ) );
        // Adjust the column size.
        int col_size = m_itemsGrid->GetVisibleWidth( col );

        if( col == FPT_LAYER )  // This one's a drop-down.  Check all possible values.
        {
            BOARD* board = m_footprint->GetBoard();

            for( PCB_LAYER_ID layer : board->GetEnabledLayers().Seq() )
                col_size = std::max( col_size, GetTextExtent( board->GetLayerName( layer ) ).x );

            // And the swatch:
            col_size += 20;
        }

        if( m_itemsGrid->IsColShown( col ) )
            m_itemsGrid->SetColSize( col, col_size );
    }

    m_itemsGrid->SetRowLabelSize( m_itemsGrid->GetVisibleWidth( -1, true, true, true ) );

    Layout();
    adjustGridColumns();

    return true;
}


bool DIALOG_FOOTPRINT_PROPERTIES_FP_EDITOR::checkFootprintName( const wxString& aFootprintName )
{
    if( aFootprintName.IsEmpty() )
    {
        m_delayedErrorMessage = _( "Footprint must have a name." );
        return false;
    }
    else if( !FOOTPRINT::IsLibNameValid( aFootprintName ) )
    {
        m_delayedErrorMessage.Printf( _( "Footprint name may not contain '%s'." ),
                                      FOOTPRINT::StringLibNameInvalidChars( true ) );
        return false;
    }

    return true;
}


bool DIALOG_FOOTPRINT_PROPERTIES_FP_EDITOR::Validate()
{
    if( !m_itemsGrid->CommitPendingChanges() )
        return false;

    if( !DIALOG_SHIM::Validate() )
        return false;

    // First, test for invalid chars in footprint name
    wxString footprintName = m_FootprintNameCtrl->GetValue();

    if( !checkFootprintName( footprintName ) )
    {
        if( m_NoteBook->GetSelection() != 0 )
            m_NoteBook->SetSelection( 0 );

        m_delayedFocusCtrl = m_FootprintNameCtrl;
        m_delayedFocusPage = NOTEBOOK_PAGES::PAGE_GENERAL;

        return false;
    }

    // Check for empty texts.
    for( size_t i = 2; i < m_texts->size(); ++i )
    {
        PCB_TEXT& text = m_texts->at( i );

        if( text.GetText().IsEmpty() )
        {
            if( m_NoteBook->GetSelection() != 0 )
                m_NoteBook->SetSelection( 0 );

            m_delayedErrorMessage = _( "Text items must have some content." );
            m_delayedFocusGrid = m_itemsGrid;
            m_delayedFocusColumn = FPT_TEXT;
            m_delayedFocusRow = i;

            return false;
        }

        if( text.GetTextWidth() < TEXTS_MIN_SIZE || text.GetTextWidth() > TEXTS_MAX_SIZE )
        {
            m_delayedFocusGrid = m_itemsGrid;
            m_delayedErrorMessage = wxString::Format( _( "The text width must be between %s and %s." ),
                                                      m_frame->StringFromValue( TEXTS_MIN_SIZE, true ),
                                                      m_frame->StringFromValue( TEXTS_MAX_SIZE, true ) );
            m_delayedFocusColumn = FPT_WIDTH;
            m_delayedFocusRow = i;

            return false;
        }

        if( text.GetTextHeight() < TEXTS_MIN_SIZE || text.GetTextHeight() > TEXTS_MAX_SIZE )
        {
            m_delayedFocusGrid = m_itemsGrid;
            m_delayedErrorMessage = wxString::Format( _( "The text height must be between %s and %s." ),
                                                      m_frame->StringFromValue( TEXTS_MIN_SIZE, true ),
                                                      m_frame->StringFromValue( TEXTS_MAX_SIZE, true ) );
            m_delayedFocusColumn = FPT_HEIGHT;
            m_delayedFocusRow = i;

            return false;
        }

        // Test for acceptable values for thickness and size and clamp if fails
        int maxPenWidth = Clamp_Text_PenSize( text.GetTextThickness(), text.GetTextSize() );

        if( text.GetTextThickness() > maxPenWidth )
        {
            m_itemsGrid->SetCellValue( i, FPT_THICKNESS,
                                       m_frame->StringFromValue( maxPenWidth, true ) );

            m_delayedFocusGrid = m_itemsGrid;
            m_delayedErrorMessage = _( "The text thickness is too large for the text size.\n"
                                       "It will be clamped." );
            m_delayedFocusColumn = FPT_THICKNESS;
            m_delayedFocusRow = i;

            return false;
        }
    }

    if( !m_netClearance.Validate( 0, INT_MAX ) )
        return false;

    return true;
}


bool DIALOG_FOOTPRINT_PROPERTIES_FP_EDITOR::TransferDataFromWindow()
{
    if( !Validate() )
        return false;

    if( !DIALOG_SHIM::TransferDataFromWindow() )
        return false;

    if( !m_itemsGrid->CommitPendingChanges()
            || !m_privateLayersGrid->CommitPendingChanges()
            || !m_padGroupsGrid->CommitPendingChanges() )
    {
        return false;
    }

    // This only commits the editor, model updating is done below so it is inside
    // the commit
    if( !m_3dPanel->TransferDataFromWindow() )
        return false;

    KIGFX::PCB_VIEW* view = m_frame->GetCanvas()->GetView();
    BOARD_COMMIT     commit( m_frame );
    commit.Modify( m_footprint );

    LIB_ID fpID = m_footprint->GetFPID();
    fpID.SetLibItemName( m_FootprintNameCtrl->GetValue() );
    m_footprint->SetFPID( fpID );

    m_footprint->SetDescription( m_DocCtrl->GetValue() );
    m_footprint->SetKeywords( m_KeywordCtrl->GetValue() );

    // copy reference and value
    m_footprint->Reference() = m_texts->at( 0 );
    m_footprint->Value() = m_texts->at( 1 );

    size_t i = 2;
    std::vector<PCB_TEXT*> items_to_remove;

    for( BOARD_ITEM* item : m_footprint->GraphicalItems() )
    {
        PCB_TEXT* textItem = dynamic_cast<PCB_TEXT*>( item );

        if( textItem )
        {
            // copy grid table entries till we run out, then delete any remaining texts
            if( i < m_texts->size() )
                *textItem = m_texts->at( i++ );
            else    // store this item to remove and delete it later,
                    // after the graphic list is explored:
                items_to_remove.push_back( textItem );
        }
    }

    // Remove text items:
    PCB_SELECTION_TOOL* selTool = m_frame->GetToolManager()->GetTool<PCB_SELECTION_TOOL>();

    for( PCB_TEXT* item: items_to_remove )
    {
        selTool->RemoveItemFromSel( item );
        view->Remove( item );
        item->DeleteStructure();
    }

    // if there are still grid table entries, create new texts for them
    while( i < m_texts->size() )
    {
        PCB_TEXT* newText = new PCB_TEXT( m_texts->at( i++ ) );
        m_footprint->Add( newText, ADD_MODE::APPEND );
        view->Add( newText );
    }

    LSET privateLayers;

    for( PCB_LAYER_ID layer : *m_privateLayers )
        privateLayers.set( layer );

    m_footprint->SetPrivateLayers( privateLayers );

    int attributes = 0;

    switch( m_componentType->GetSelection() )
    {
    case 0:  attributes |= FP_THROUGH_HOLE; break;
    case 1:  attributes |= FP_SMD;          break;
    default:                                break;
    }

    if( m_boardOnly->GetValue() )
        attributes |= FP_BOARD_ONLY;

    if( m_excludeFromPosFiles->GetValue() )
        attributes |= FP_EXCLUDE_FROM_POS_FILES;

    if( m_excludeFromBOM->GetValue() )
        attributes |= FP_EXCLUDE_FROM_BOM;

    if( m_noCourtyards->GetValue() )
        attributes |= FP_ALLOW_MISSING_COURTYARD;

    if( m_cbDNP->GetValue() )
        attributes |= FP_DNP;

    if( m_allowBridges->GetValue() )
        attributes |= FP_ALLOW_SOLDERMASK_BRIDGES;

    m_footprint->SetAttributes( attributes );

    // Initialize masks clearances
    m_footprint->SetLocalClearance( m_netClearance.GetValue() );
    m_footprint->SetLocalSolderMaskMargin( m_solderMask.GetValue() );
    m_footprint->SetLocalSolderPasteMargin( m_solderPaste.GetValue() );
    m_footprint->SetLocalSolderPasteMarginRatio( m_solderPasteRatio.GetDoubleValue() / 100.0 );

    switch( m_ZoneConnectionChoice->GetSelection() )
    {
    default:
    case 0:  m_footprint->SetZoneConnection( ZONE_CONNECTION::INHERITED ); break;
    case 1:  m_footprint->SetZoneConnection( ZONE_CONNECTION::FULL );      break;
    case 2:  m_footprint->SetZoneConnection( ZONE_CONNECTION::THERMAL );   break;
    case 3:  m_footprint->SetZoneConnection( ZONE_CONNECTION::NONE );      break;
    }

    m_footprint->ClearNetTiePadGroups();

    for( int ii = 0; ii < m_padGroupsGrid->GetNumberRows(); ++ii )
    {
        wxString group = m_padGroupsGrid->GetCellValue( ii, 0 );

        if( !group.IsEmpty() )
            m_footprint->AddNetTiePadGroup( group );
    }

    // Copy the models from the panel to the footprint
    std::vector<FP_3DMODEL>& panelList = m_3dPanel->GetModelList();
    std::vector<FP_3DMODEL>* fpList    = &m_footprint->Models();
    fpList->clear();
    fpList->insert( fpList->end(), panelList.begin(), panelList.end() );

    commit.Push( _( "Modify footprint properties" ) );

    return true;
}


static bool footprintIsFromBoard( FOOTPRINT* aFootprint )
{
    return aFootprint->GetLink() != niluuid;
}


void DIALOG_FOOTPRINT_PROPERTIES_FP_EDITOR::OnFootprintNameText( wxCommandEvent& event )
{
    if( !footprintIsFromBoard( m_footprint ) )
    {
        // Currently: nothing to do
    }
}


void DIALOG_FOOTPRINT_PROPERTIES_FP_EDITOR::OnAddField( wxCommandEvent& event )
{
    if( !m_itemsGrid->CommitPendingChanges() )
        return;

    const BOARD_DESIGN_SETTINGS& dsnSettings = m_frame->GetDesignSettings();
    PCB_TEXT                     textItem( m_footprint );

    // Set active layer if legal; otherwise copy layer from previous text item
    if( LSET::AllTechMask().test( m_frame->GetActiveLayer() ) )
        textItem.SetLayer( m_frame->GetActiveLayer() );
    else
        textItem.SetLayer( m_texts->at( m_texts->size() - 1 ).GetLayer() );

    textItem.SetTextSize( dsnSettings.GetTextSize( textItem.GetLayer() ) );
    textItem.SetTextThickness( dsnSettings.GetTextThickness( textItem.GetLayer() ) );
    textItem.SetItalic( dsnSettings.GetTextItalic( textItem.GetLayer() ) );

    m_texts->push_back( textItem );

    // notify the grid
    wxGridTableMessage msg( m_texts, wxGRIDTABLE_NOTIFY_ROWS_APPENDED, 1 );
    m_itemsGrid->ProcessTableMessage( msg );

    m_itemsGrid->SetFocus();
    m_itemsGrid->MakeCellVisible( m_texts->size() - 1, 0 );
    m_itemsGrid->SetGridCursor( m_texts->size() - 1, 0 );

    m_itemsGrid->EnableCellEditControl( true );
    m_itemsGrid->ShowCellEditControl();
}


void DIALOG_FOOTPRINT_PROPERTIES_FP_EDITOR::OnDeleteField( wxCommandEvent& event )
{
    if( !m_itemsGrid->CommitPendingChanges() )
        return;

    wxArrayInt selectedRows = m_itemsGrid->GetSelectedRows();

    if( selectedRows.empty() && m_itemsGrid->GetGridCursorRow() >= 0 )
        selectedRows.push_back( m_itemsGrid->GetGridCursorRow() );

    if( selectedRows.empty() )
        return;

    for( int row : selectedRows )
    {
        if( row < 2 )
        {
            DisplayError( nullptr, _( "Reference and value are mandatory." ) );
            return;
        }
    }

    // Reverse sort so deleting a row doesn't change the indexes of the other rows.
    selectedRows.Sort( []( int* first, int* second ) { return *second - *first; } );

    for( int row : selectedRows )
    {
        m_texts->erase( m_texts->begin() + row );

        // notify the grid
        wxGridTableMessage msg( m_texts, wxGRIDTABLE_NOTIFY_ROWS_DELETED, row, 1 );
        m_itemsGrid->ProcessTableMessage( msg );

        if( m_itemsGrid->GetNumberRows() > 0 )
        {
            m_itemsGrid->MakeCellVisible( std::max( 0, row-1 ), m_itemsGrid->GetGridCursorCol() );
            m_itemsGrid->SetGridCursor( std::max( 0, row-1 ), m_itemsGrid->GetGridCursorCol() );
        }
    }
}


void DIALOG_FOOTPRINT_PROPERTIES_FP_EDITOR::OnAddLayer( wxCommandEvent& event )
{
    if( !m_privateLayersGrid->CommitPendingChanges() )
        return;

    PCB_LAYER_ID nextLayer = User_1;

    while( alg::contains( *m_privateLayers, nextLayer ) && nextLayer < User_9 )
        nextLayer = ToLAYER_ID( nextLayer + 1 );

    m_privateLayers->push_back( nextLayer );

    // notify the grid
    wxGridTableMessage msg( m_privateLayers, wxGRIDTABLE_NOTIFY_ROWS_APPENDED, 1 );
    m_privateLayersGrid->ProcessTableMessage( msg );

    m_privateLayersGrid->SetFocus();
    m_privateLayersGrid->MakeCellVisible( m_privateLayers->size() - 1, 0 );
    m_privateLayersGrid->SetGridCursor( m_privateLayers->size() - 1, 0 );
}


void DIALOG_FOOTPRINT_PROPERTIES_FP_EDITOR::OnDeleteLayer( wxCommandEvent& event )
{
    if( !m_privateLayersGrid->CommitPendingChanges() )
        return;

    int curRow = m_privateLayersGrid->GetGridCursorRow();

    if( curRow < 0 )
        return;

    m_privateLayers->erase( m_privateLayers->begin() + curRow );

    // notify the grid
    wxGridTableMessage msg( m_privateLayers, wxGRIDTABLE_NOTIFY_ROWS_DELETED, curRow, 1 );
    m_privateLayersGrid->ProcessTableMessage( msg );

    if( m_privateLayersGrid->GetNumberRows() > 0 )
    {
        m_privateLayersGrid->MakeCellVisible( std::max( 0, curRow-1 ),
                                               m_privateLayersGrid->GetGridCursorCol() );
        m_privateLayersGrid->SetGridCursor( std::max( 0, curRow-1 ),
                                             m_privateLayersGrid->GetGridCursorCol() );
    }
}


void DIALOG_FOOTPRINT_PROPERTIES_FP_EDITOR::OnAddPadGroup( wxCommandEvent& event )
{
    if( !m_padGroupsGrid->CommitPendingChanges() )
        return;

    m_padGroupsGrid->AppendRows( 1 );

    m_padGroupsGrid->SetFocus();
    m_padGroupsGrid->MakeCellVisible( m_padGroupsGrid->GetNumberRows() - 1, 0 );
    m_padGroupsGrid->SetGridCursor( m_padGroupsGrid->GetNumberRows() - 1, 0 );

    m_padGroupsGrid->EnableCellEditControl( true );
    m_padGroupsGrid->ShowCellEditControl();
}


void DIALOG_FOOTPRINT_PROPERTIES_FP_EDITOR::OnRemovePadGroup( wxCommandEvent& event )
{
    if( !m_padGroupsGrid->CommitPendingChanges() )
        return;

    wxArrayInt selectedRows = m_padGroupsGrid->GetSelectedRows();
    int        curRow = m_padGroupsGrid->GetGridCursorRow();

    if( selectedRows.empty() && curRow >= 0 && curRow < m_padGroupsGrid->GetNumberRows() )
        selectedRows.Add( curRow );

    for( int ii = selectedRows.Count() - 1; ii >= 0; --ii )
    {
        int row = selectedRows.Item( ii );
        m_padGroupsGrid->DeleteRows( row, 1 );
        curRow = std::min( curRow, row );
    }

    curRow = std::max( 0, curRow - 1 );
    m_padGroupsGrid->MakeCellVisible( curRow, m_padGroupsGrid->GetGridCursorCol() );
    m_padGroupsGrid->SetGridCursor( curRow, m_padGroupsGrid->GetGridCursorCol() );
}


void DIALOG_FOOTPRINT_PROPERTIES_FP_EDITOR::adjustGridColumns()
{
    // Account for scroll bars
    int itemsWidth = KIPLATFORM::UI::GetUnobscuredSize( m_itemsGrid ).x;

    itemsWidth -= m_itemsGrid->GetRowLabelSize();

    for( int i = 1; i < m_itemsGrid->GetNumberCols(); i++ )
        itemsWidth -= m_itemsGrid->GetColSize( i );

    m_itemsGrid->SetColSize( 0, std::max( itemsWidth,
                                          m_itemsGrid->GetVisibleWidth( 0, true, false ) ) );

    // Update the width private layers grid
    m_privateLayersGrid->SetColSize( 0, std::max( m_privateLayersGrid->GetClientSize().x,
                                                  m_privateLayersGrid->GetVisibleWidth( 0 ) ) );

    // Update the width net tie pad groups grid
    m_padGroupsGrid->SetColSize( 0, std::max( m_padGroupsGrid->GetClientSize().x,
                                              m_padGroupsGrid->GetVisibleWidth( 0 ) ) );

    // Update the width of the 3D panel
    m_3dPanel->AdjustGridColumnWidths();
}


void DIALOG_FOOTPRINT_PROPERTIES_FP_EDITOR::OnUpdateUI( wxUpdateUIEvent& event )
{
    // Handle a delayed focus.  The delay allows us to:
    // a) change focus when the error was triggered from within a killFocus handler
    // b) show the correct notebook page in the background before the error dialog comes up
    //    when triggered from an OK or a notebook page change

    if( static_cast<int>( m_delayedFocusPage ) >= 0 )
    {
        if( m_NoteBook->GetSelection() != static_cast<int>( m_delayedFocusPage ) )
            m_NoteBook->ChangeSelection( static_cast<int>( m_delayedFocusPage ) );

        m_delayedFocusPage = NOTEBOOK_PAGES::PAGE_UNKNOWN;
    }

    if( !m_delayedErrorMessage.IsEmpty() )
    {
        // We will re-enter this routine when the error dialog is displayed, so make
        // sure we don't keep putting up more dialogs.
        wxString msg = m_delayedErrorMessage;
        m_delayedErrorMessage = wxEmptyString;

        // Do not use DisplayErrorMessage(); it screws up window order on Mac
        DisplayError( nullptr, msg );
    }

    if( m_delayedFocusCtrl )
    {
        m_delayedFocusCtrl->SetFocus();

        if( wxTextEntry* textEntry = dynamic_cast<wxTextEntry*>( m_delayedFocusCtrl ) )
            textEntry->SelectAll();

        m_delayedFocusCtrl = nullptr;
    }
    else if( m_delayedFocusGrid )
    {
        m_delayedFocusGrid->SetFocus();
        m_delayedFocusGrid->MakeCellVisible( m_delayedFocusRow, m_delayedFocusColumn );
        m_delayedFocusGrid->SetGridCursor( m_delayedFocusRow, m_delayedFocusColumn );

        m_delayedFocusGrid->EnableCellEditControl( true );
        m_delayedFocusGrid->ShowCellEditControl();

        m_delayedFocusGrid = nullptr;
        m_delayedFocusRow = -1;
        m_delayedFocusColumn = -1;
    }
}


void DIALOG_FOOTPRINT_PROPERTIES_FP_EDITOR::OnGridSize( wxSizeEvent& aEvent )
{
    wxSize new_size = aEvent.GetSize();

    if( ( !m_itemsGrid->IsCellEditControlShown() || m_lastRequestedSize != new_size )
            && m_gridSize != new_size )
    {
        m_gridSize = new_size;

        // A trick to fix a cosmetic issue: when, in m_itemsGrid, a layer selector widget has
        // the focus (is activated in column 6) when resizing the grid, the widget is not moved.
        // So just change the widget having the focus in this case
        if( m_NoteBook->GetSelection() == 0 && !m_itemsGrid->HasFocus() )
        {
            int col = m_itemsGrid->GetGridCursorCol();

            if( col == 6 )  // a layer selector widget can be activated
                 m_itemsGrid->SetFocus();
        }

        adjustGridColumns();
    }

    // We store this value to check whether the dialog is changing size.  This might indicate
    // that the user is scaling the dialog with an editor shown.  Some editors do not close
    // (at least on GTK) when the user drags a dialog corner
    m_lastRequestedSize = new_size;

    // Always propagate for a grid repaint (needed if the height changes, as well as width)
    aEvent.Skip();
}
