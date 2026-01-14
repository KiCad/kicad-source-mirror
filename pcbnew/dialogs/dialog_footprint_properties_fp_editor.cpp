/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2018 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2015 Dick Hollenbeck, dick@softplc.com
 * Copyright (C) 2008 Wayne Stambaugh <stambaughw@gmail.com>
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

#include "dialog_footprint_properties_fp_editor.h"

#include <wx/debug.h>

#include <3d_rendering/opengl/3d_model.h>
#include <3d_viewer/eda_3d_viewer_frame.h>
#include <bitmaps.h>
#include <board_commit.h>
#include <board_design_settings.h>
#include <confirm.h>

#include <dialogs/dialog_text_entry.h>
#include <dialogs/panel_preview_3d_model.h>
#include <embedded_files.h>
#include <filename_resolver.h>
#include <footprint.h>
#include <footprint_edit_frame.h>
#include <footprint_editor_settings.h>
#include <grid_layer_box_helpers.h>
#include <layer_utils.h>
#include <kiplatform/ui.h>
#include <panel_embedded_files.h>
#include <panel_fp_properties_3d_model.h>
#include <pgm_base.h>
#include <settings/settings_manager.h>
#include <tool/tool_manager.h>
#include <tools/pcb_actions.h>
#include <tools/pcb_selection_tool.h>
#include <validators.h>
#include <widgets/grid_text_button_helpers.h>
#include <widgets/std_bitmap_button.h>
#include <widgets/text_ctrl_eval.h>
#include <widgets/wx_grid.h>

#include <footprint_library_adapter.h>
#include <project_pcb.h>
#include <kidialog.h>


class LAYERS_GRID_TABLE : public WX_GRID_TABLE_BASE, public std::vector<PCB_LAYER_ID>
{
public:
    LAYERS_GRID_TABLE( PCB_BASE_FRAME* aFrame, const LSET& aForbiddenLayers );
    ~LAYERS_GRID_TABLE();

    int GetNumberRows() override { return (int) size(); }
    int GetNumberCols() override { return 1; }

    bool            CanGetValueAs( int aRow, int aCol, const wxString& aTypeName ) override;
    bool            CanSetValueAs( int aRow, int aCol, const wxString& aTypeName ) override;
    wxGridCellAttr* GetAttr( int aRow, int aCol, wxGridCellAttr::wxAttrKind aKind ) override;

    wxString GetValue( int aRow, int aCol ) override;
    long     GetValueAsLong( int aRow, int aCol ) override;

    void SetValue( int aRow, int aCol, const wxString& aValue ) override;
    void SetValueAsLong( int aRow, int aCol, long aValue ) override;

private:
    PCB_BASE_FRAME* m_frame;
    wxGridCellAttr* m_layerColAttr;
};


LAYERS_GRID_TABLE::LAYERS_GRID_TABLE( PCB_BASE_FRAME* aFrame, const LSET& aForbiddenLayers ) :
        m_frame( aFrame )
{
    m_layerColAttr = new wxGridCellAttr;
    m_layerColAttr->SetRenderer( new GRID_CELL_LAYER_RENDERER( m_frame ) );

    m_layerColAttr->SetEditor( new GRID_CELL_LAYER_SELECTOR( m_frame, aForbiddenLayers, true ) );
}


LAYERS_GRID_TABLE::~LAYERS_GRID_TABLE()
{
    m_layerColAttr->DecRef();
}


bool LAYERS_GRID_TABLE::CanGetValueAs( int aRow, int aCol, const wxString& aTypeName )
{
    return aTypeName == wxGRID_VALUE_NUMBER;
}


bool LAYERS_GRID_TABLE::CanSetValueAs( int aRow, int aCol, const wxString& aTypeName )
{
    return aTypeName == wxGRID_VALUE_NUMBER;
}


wxGridCellAttr* LAYERS_GRID_TABLE::GetAttr( int aRow, int aCol, wxGridCellAttr::wxAttrKind aKind )
{
    m_layerColAttr->IncRef();
    return enhanceAttr( m_layerColAttr, aRow, aCol, aKind );
}


wxString LAYERS_GRID_TABLE::GetValue( int aRow, int aCol )
{
    return m_frame->GetBoard()->GetLayerName( this->at( (size_t) aRow ) );
}


long LAYERS_GRID_TABLE::GetValueAsLong( int aRow, int aCol )
{
    return this->at( (size_t) aRow );
}


void LAYERS_GRID_TABLE::SetValue( int aRow, int aCol, const wxString& aValue )
{
    wxFAIL_MSG( wxString::Format( wxT( "column %d doesn't hold a string value" ), aCol ) );
}


void LAYERS_GRID_TABLE::SetValueAsLong( int aRow, int aCol, long aValue )
{
    this->at( (size_t) aRow ) = ToLAYER_ID( (int) aValue );
}


// Remember the last open page during session.

NOTEBOOK_PAGES DIALOG_FOOTPRINT_PROPERTIES_FP_EDITOR::m_page = NOTEBOOK_PAGES::PAGE_GENERAL;


DIALOG_FOOTPRINT_PROPERTIES_FP_EDITOR::DIALOG_FOOTPRINT_PROPERTIES_FP_EDITOR( FOOTPRINT_EDIT_FRAME* aParent,
                                                                              FOOTPRINT* aFootprint ) :
        DIALOG_FOOTPRINT_PROPERTIES_FP_EDITOR_BASE( aParent ),
        m_frame( aParent ),
        m_footprint( aFootprint ),
        m_initialized( false ),
        m_netClearance( aParent, m_NetClearanceLabel, m_NetClearanceCtrl, m_NetClearanceUnits ),
        m_solderMask( aParent, m_SolderMaskMarginLabel, m_SolderMaskMarginCtrl, m_SolderMaskMarginUnits ),
        m_solderPaste( aParent, m_SolderPasteMarginLabel, m_SolderPasteMarginCtrl, m_SolderPasteMarginUnits )
{
    SetEvtHandlerEnabled( false );

    // Create the extra panels.
    m_embeddedFiles = new PANEL_EMBEDDED_FILES( m_NoteBook, m_footprint );
    m_3dPanel = new PANEL_FP_PROPERTIES_3D_MODEL( m_frame, m_footprint, this, m_embeddedFiles, m_NoteBook );

    m_NoteBook->AddPage( m_3dPanel, _("3D Models"), false );
    m_NoteBook->AddPage( m_embeddedFiles, _( "Embedded Files" ) );

    m_fields = new PCB_FIELDS_GRID_TABLE( m_frame, this, { m_embeddedFiles->GetLocalFiles() } );

    {
        LSET forbiddenLayers = LSET::AllCuMask() | LSET::AllTechMask();
        forbiddenLayers.set( Edge_Cuts );
        forbiddenLayers.set( Margin );

        m_privateLayers = new LAYERS_GRID_TABLE( m_frame, forbiddenLayers );
    }

    {
        LSET forbiddenLayers = LSET::AllLayersMask() & ~LSET::UserDefinedLayersMask();
        m_customUserLayers = new LAYERS_GRID_TABLE( m_frame, forbiddenLayers );
    }

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

    m_itemsGrid->SetTable( m_fields );
    m_privateLayersGrid->SetTable( m_privateLayers );
    m_customUserLayersGrid->SetTable( m_customUserLayers );

    m_itemsGrid->PushEventHandler( new GRID_TRICKS( m_itemsGrid ) );
    m_privateLayersGrid->PushEventHandler( new GRID_TRICKS( m_privateLayersGrid,
                                                            [this]( wxCommandEvent& aEvent )
                                                            {
                                                                OnAddPrivateLayer( aEvent );
                                                            } ) );
    m_nettieGroupsGrid->PushEventHandler( new GRID_TRICKS( m_nettieGroupsGrid,
                                                           [this]( wxCommandEvent& aEvent )
                                                           {
                                                               OnAddNettieGroup( aEvent );
                                                           } ) );
    m_jumperGroupsGrid->PushEventHandler( new GRID_TRICKS( m_jumperGroupsGrid,
                                                           [this]( wxCommandEvent& aEvent )
                                                           {
                                                               OnAddJumperGroup( aEvent );
                                                           } ) );
    m_customUserLayersGrid->PushEventHandler( new GRID_TRICKS( m_customUserLayersGrid,
                                                               [this]( wxCommandEvent& aEvent )
                                                               {
                                                                   OnAddCustomLayer( aEvent );
                                                               } ) );

    m_itemsGrid->SetupColumnAutosizer( PFC_VALUE );
    m_privateLayersGrid->SetupColumnAutosizer( 0 );
    m_nettieGroupsGrid->SetupColumnAutosizer( 0 );
    m_jumperGroupsGrid->SetupColumnAutosizer( 0 );
    m_customUserLayersGrid->SetupColumnAutosizer( 0 );

    m_itemsGrid->SetSelectionMode( wxGrid::wxGridSelectRows );
    m_privateLayersGrid->SetSelectionMode( wxGrid::wxGridSelectRows );
    m_nettieGroupsGrid->SetSelectionMode( wxGrid::wxGridSelectRows );
    m_jumperGroupsGrid->SetSelectionMode( wxGrid::wxGridSelectRows );
    m_customUserLayersGrid->SetSelectionMode( wxGrid::wxGridSelectRows );

    m_itemsGrid->ShowHideColumns( "0 1 2 3 4 5 7" );

    m_FootprintNameCtrl->SetValidator( FOOTPRINT_NAME_VALIDATOR() );

    // Set font sizes
    wxFont infoFont = KIUI::GetInfoFont( this ).Italic();
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

    // Update label text and tooltip for combined offset + ratio field
    m_SolderPasteMarginLabel->SetLabel( _( "Solder paste clearance:" ) );
    m_SolderPasteMarginLabel->SetToolTip( _( "Local solder paste clearance for this footprint.\n"
                                             "Enter an absolute value (e.g., -0.1mm), a percentage "
                                             "(e.g., -5%), or both (e.g., -0.1mm - 5%).\n"
                                             "If blank, the global value is used." ) );

    // Hide the old ratio controls - they're no longer needed
    m_PasteMarginRatioLabel->Show( false );
    m_PasteMarginRatioCtrl->Show( false );
    m_PasteMarginRatioUnits->Show( false );

    // Configure button logos
    m_bpAdd->SetBitmap( KiBitmapBundle( BITMAPS::small_plus ) );
    m_bpDelete->SetBitmap( KiBitmapBundle( BITMAPS::small_trash ) );
    m_bpAddPrivateLayer->SetBitmap( KiBitmapBundle( BITMAPS::small_plus ) );
    m_bpDeletePrivateLayer->SetBitmap( KiBitmapBundle( BITMAPS::small_trash ) );
    m_bpAddCustomLayer->SetBitmap( KiBitmapBundle( BITMAPS::small_plus ) );
    m_bpDeleteCustomLayer->SetBitmap( KiBitmapBundle( BITMAPS::small_trash ) );
    m_bpAddNettieGroup->SetBitmap( KiBitmapBundle( BITMAPS::small_plus ) );
    m_bpRemoveNettieGroup->SetBitmap( KiBitmapBundle( BITMAPS::small_trash ) );
    m_bpAddJumperGroup->SetBitmap( KiBitmapBundle( BITMAPS::small_plus ) );
    m_bpRemoveJumperGroup->SetBitmap( KiBitmapBundle( BITMAPS::small_trash ) );

    SetupStandardButtons();

    finishDialogSettings();
    SetEvtHandlerEnabled( true );
}


DIALOG_FOOTPRINT_PROPERTIES_FP_EDITOR::~DIALOG_FOOTPRINT_PROPERTIES_FP_EDITOR()
{
    // Prevents crash bug in wxGrid's d'tor
    m_itemsGrid->DestroyTable( m_fields );
    m_privateLayersGrid->DestroyTable( m_privateLayers );
    m_customUserLayersGrid->DestroyTable( m_customUserLayers );

    // Delete the GRID_TRICKS.
    m_itemsGrid->PopEventHandler( true );
    m_privateLayersGrid->PopEventHandler( true );
    m_nettieGroupsGrid->PopEventHandler( true );
    m_jumperGroupsGrid->PopEventHandler( true );
    m_customUserLayersGrid->PopEventHandler( true );

    m_page = static_cast<NOTEBOOK_PAGES>( m_NoteBook->GetSelection() );

    // the GL canvas on the 3D models page has to be visible before it is destroyed
    m_NoteBook->SetSelection( static_cast<int>( NOTEBOOK_PAGES::PAGE_3D_MODELS ) );
}


bool DIALOG_FOOTPRINT_PROPERTIES_FP_EDITOR::TransferDataToWindow()
{
    LIB_ID   fpID          = m_footprint->GetFPID();
    wxString footprintName = fpID.GetLibItemName();

    m_FootprintNameCtrl->ChangeValue( footprintName );

    m_DocCtrl->SetValue( EscapeString( m_footprint->GetLibDescription(), CTX_LINE ) );
    m_KeywordCtrl->SetValue( m_footprint->GetKeywords() );

    if( !wxDialog::TransferDataToWindow() )
        return false;

    if( !m_PanelGeneral->TransferDataToWindow() )
        return false;

    // Add the models to the panel
    if( !m_3dPanel->TransferDataToWindow() )
        return false;

    if( !m_embeddedFiles->TransferDataToWindow() )
        return false;

    // Footprint Fields
    for( PCB_FIELD* field : m_footprint->GetFields() )
    {
        wxCHECK2( field, continue );

        m_fields->push_back( *field );
    }

    // Notify the grid
    wxGridTableMessage tmsg( m_fields, wxGRIDTABLE_NOTIFY_ROWS_APPENDED, m_fields->GetNumberRows() );
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

    switch( m_footprint->GetStackupMode() )
    {
    case FOOTPRINT_STACKUP::EXPAND_INNER_LAYERS:
    {
        m_cbCustomLayers->SetValue( false );

        m_copperLayerCount->SetSelection( 0 );
        break;
    }
    case FOOTPRINT_STACKUP::CUSTOM_LAYERS:
    {
        m_cbCustomLayers->SetValue( true );

        const LSET& customFpLayers = m_footprint->GetStackupLayers();
        const LSET  customUserLayers = customFpLayers & LSET::UserDefinedLayersMask();

        for( PCB_LAYER_ID customUserLayer : customUserLayers )
        {
            m_customUserLayers->push_back( customUserLayer );
        }

        // Set the number of copper layers
        m_copperLayerCount->SetSelection( ( customFpLayers & LSET::AllCuMask() ).count() / 2 - 1 );
        break;
    }
    }
    setCustomLayerCtrlEnablement();

    // Notify the grid
    {
        wxGridTableMessage gridTableMessagesCustom( m_customUserLayers, wxGRIDTABLE_NOTIFY_ROWS_APPENDED,
                                                    m_customUserLayers->GetNumberRows() );
        m_customUserLayersGrid->ProcessTableMessage( gridTableMessagesCustom );
    }

    m_boardOnly->SetValue( m_footprint->GetAttributes() & FP_BOARD_ONLY );
    m_excludeFromPosFiles->SetValue( m_footprint->GetAttributes() & FP_EXCLUDE_FROM_POS_FILES );
    m_excludeFromBOM->SetValue( m_footprint->GetAttributes() & FP_EXCLUDE_FROM_BOM );
    m_cbDNP->SetValue( m_footprint->GetAttributes() & FP_DNP );

    // Local Clearances

    if( m_footprint->GetLocalClearance().has_value() )
        m_netClearance.SetValue( m_footprint->GetLocalClearance().value() );
    else
        m_netClearance.SetValue( wxEmptyString );

    if( m_footprint->GetLocalSolderMaskMargin().has_value() )
        m_solderMask.SetValue( m_footprint->GetLocalSolderMaskMargin().value() );
    else
        m_solderMask.SetValue( wxEmptyString );

    m_solderPaste.SetOffsetValue( m_footprint->GetLocalSolderPasteMargin() );
    m_solderPaste.SetRatioValue( m_footprint->GetLocalSolderPasteMarginRatio() );

    m_noCourtyards->SetValue( m_footprint->AllowMissingCourtyard() );
    m_allowBridges->SetValue( m_footprint->AllowSolderMaskBridges() );

    switch( m_footprint->GetLocalZoneConnection() )
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
            m_nettieGroupsGrid->AppendRows( 1 );
            m_nettieGroupsGrid->SetCellValue( m_nettieGroupsGrid->GetNumberRows() - 1, 0, group );
        }
    }

    m_cbDuplicatePadsAreJumpers->SetValue( m_footprint->GetDuplicatePadNumbersAreJumpers() );

    for( const std::set<wxString>& group : m_footprint->JumperPadGroups() )
    {
        wxString groupTxt;

        for( const wxString& pinNumber : group )
        {
            if( !groupTxt.IsEmpty() )
                groupTxt << ", ";

            groupTxt << pinNumber;
        }

        m_jumperGroupsGrid->AppendRows( 1 );
        m_jumperGroupsGrid->SetCellValue( m_jumperGroupsGrid->GetNumberRows() - 1, 0, groupTxt );
    }

    // Items grid
    for( int col = 0; col < m_itemsGrid->GetNumberCols(); col++ )
    {
        // Adjust min size to the column label size
        m_itemsGrid->SetColMinimalWidth( col, m_itemsGrid->GetVisibleWidth( col, true, false ) );
        // Adjust the column size.
        int col_size = m_itemsGrid->GetVisibleWidth( col );

        if( col == PFC_LAYER )  // This one's a drop-down.  Check all possible values.
        {
            BOARD* board = m_footprint->GetBoard();

            for( PCB_LAYER_ID layer : board->GetEnabledLayers() )
                col_size = std::max( col_size, GetTextExtent( board->GetLayerName( layer ) ).x );

            // Swatch and gaps:
            col_size += KiROUND( 14 * GetDPIScaleFactor() ) + 12;
        }

        if( m_itemsGrid->IsColShown( col ) )
            m_itemsGrid->SetColSize( col, col_size );
    }

    m_itemsGrid->SetRowLabelSize( 0 );

    Layout();
    m_initialized = true;

    return true;
}


bool DIALOG_FOOTPRINT_PROPERTIES_FP_EDITOR::checkFootprintName( const wxString& aFootprintName,
                                                                LIB_ID* doOverwrite )
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

    LIB_ID        fpID = m_footprint->GetFPID();
    wxString      libraryName = fpID.GetLibNickname();
    wxString      originalFPName = fpID.GetLibItemName();

    FOOTPRINT_LIBRARY_ADAPTER* adapter = PROJECT_PCB::FootprintLibAdapter( &m_frame->Prj() );

    if( aFootprintName != originalFPName && adapter->FootprintExists( libraryName, aFootprintName ) )
    {
        wxString msg = wxString::Format( _( "Footprint '%s' already exists in library '%s'." ),
                                         aFootprintName, libraryName );

        KIDIALOG errorDlg( m_frame, msg, _( "Confirmation" ), wxOK | wxCANCEL | wxICON_WARNING );
        errorDlg.SetOKLabel( _( "Overwrite" ) );

        if( errorDlg.ShowModal() == wxID_OK )
        {
            doOverwrite->SetLibNickname( libraryName );
            doOverwrite->SetLibItemName( aFootprintName );
            return true;
        }
    }

    return true;
}


LSET DIALOG_FOOTPRINT_PROPERTIES_FP_EDITOR::getCustomLayersFromControls() const
{
    LSET userLayers;
    if( m_cbCustomLayers->GetValue() )
    {
        userLayers |= LSET::AllCuMask( ( m_copperLayerCount->GetSelection() + 1 ) * 2 );

        for( PCB_LAYER_ID layer : *m_customUserLayers )
        {
            userLayers.set( layer );
        }
    }
    else
    {
        userLayers |= LSET{ F_Cu, In1_Cu, B_Cu };
        userLayers |= LSET::UserDefinedLayersMask( 4 );
    }

    return userLayers;
}


static LSET GetAllUsedFootprintLayers( const FOOTPRINT& aFootprint )
{
    LSET usedLayers{};
    aFootprint.RunOnChildren(
            [&]( BOARD_ITEM* aSubItem )
            {
                wxCHECK2( aSubItem, /*void*/ );

                switch( aSubItem->Type() )
                {
                case PCB_ZONE_T:
                {
                    ZONE& zone = static_cast<ZONE&>( *aSubItem );
                    usedLayers |= zone.GetLayerSet();
                    break;
                }
                default:
                {
                    usedLayers.set( aSubItem->GetLayer() );
                    break;
                }
                }
            },
            RECURSE_MODE::RECURSE );
    return usedLayers;
}


bool DIALOG_FOOTPRINT_PROPERTIES_FP_EDITOR::Validate()
{
    if( !m_itemsGrid->CommitPendingChanges() )
        return false;

    if( !DIALOG_SHIM::Validate() )
        return false;

    // First, test for invalid chars in footprint name
    wxString footprintName = m_FootprintNameCtrl->GetValue();
    LIB_ID   overwrite;

    if( !checkFootprintName( footprintName, &overwrite ) )
    {
        if( m_NoteBook->GetSelection() != 0 )
            m_NoteBook->SetSelection( 0 );

        m_delayedFocusCtrl = m_FootprintNameCtrl;
        m_delayedFocusPage = NOTEBOOK_PAGES::PAGE_GENERAL;

        return false;
    }

    // Check for valid field text properties
    for( int i = 0; i < (int) m_fields->size(); ++i )
    {
        PCB_FIELD& field = m_fields->at( i );

        // Check for missing field names.
        if( field.GetName( false ).IsEmpty() )
        {
            m_delayedFocusGrid = m_itemsGrid;
            m_delayedErrorMessage = wxString::Format( _( "Fields must have a name." ) );
            m_delayedFocusColumn = PFC_NAME;
            m_delayedFocusRow = i;

            return false;
        }

        int minSize = pcbIUScale.mmToIU( TEXT_MIN_SIZE_MM );
        int maxSize = pcbIUScale.mmToIU( TEXT_MAX_SIZE_MM );

        if( field.GetTextWidth() < minSize || field.GetTextWidth() > maxSize )
        {
            m_delayedFocusGrid = m_itemsGrid;
            m_delayedErrorMessage = wxString::Format( _( "The text width must be between %s and %s." ),
                                                      m_frame->StringFromValue( minSize, true ),
                                                      m_frame->StringFromValue( maxSize, true ) );
            m_delayedFocusColumn = PFC_WIDTH;
            m_delayedFocusRow = i;

            return false;
        }

        if( field.GetTextHeight() < minSize || field.GetTextHeight() > maxSize )
        {
            m_delayedFocusGrid = m_itemsGrid;
            m_delayedErrorMessage = wxString::Format( _( "The text height must be between %s and %s." ),
                                                      m_frame->StringFromValue( minSize, true ),
                                                      m_frame->StringFromValue( maxSize, true ) );
            m_delayedFocusColumn = PFC_HEIGHT;
            m_delayedFocusRow = i;

            return false;
        }

        // Test for acceptable values for thickness and size and clamp if fails
        int maxPenWidth = ClampTextPenSize( field.GetTextThickness(), field.GetTextSize() );

        if( field.GetTextThickness() > maxPenWidth )
        {
            m_itemsGrid->SetCellValue( i, PFC_THICKNESS, m_frame->StringFromValue( maxPenWidth, true ) );

            m_delayedFocusGrid = m_itemsGrid;
            m_delayedErrorMessage = _( "The text thickness is too large for the text size.\n"
                                       "It will be clamped." );
            m_delayedFocusColumn = PFC_THICKNESS;
            m_delayedFocusRow = i;

            return false;
        }
    }

    if( !m_netClearance.Validate( 0, INT_MAX ) )
        return false;

    if( overwrite.IsValid() )
    {
        if( m_frame->DeleteFootprintFromLibrary( overwrite, false /* already confirmed */ ) )
            m_frame->SyncLibraryTree( true );
    }

    // See if there is an object in the footprint that uses a layer that is not in that list
    LSET usedLayers = GetAllUsedFootprintLayers( *m_footprint );

    // Check that the user isn't trying to remove a layer that is used by the footprint
    usedLayers &= ~getCustomLayersFromControls();
    usedLayers &= ~LSET::AllTechMask();
    usedLayers &= ~LSET::UserMask();

    if( usedLayers.any() )
    {
        m_delayedErrorMessage =
                wxString::Format( _( "You are trying to remove layers that are used by the footprint: %s.\n"
                                     "Please remove the objects that use these layers first." ),
                                  LAYER_UTILS::AccumulateNames( usedLayers, m_frame->GetBoard() ) );
        m_delayedFocusGrid = m_customUserLayersGrid;
        m_delayedFocusColumn = 0;
        m_delayedFocusRow = 0;
        m_delayedFocusPage = NOTEBOOK_PAGES::PAGE_LAYERS;
        return false;
    }

    return true;
}


bool DIALOG_FOOTPRINT_PROPERTIES_FP_EDITOR::TransferDataFromWindow()
{
    if( !m_itemsGrid->CommitPendingChanges()
            || !m_privateLayersGrid->CommitPendingChanges()
            || !m_nettieGroupsGrid->CommitPendingChanges()
            || !m_jumperGroupsGrid->CommitPendingChanges()
            || !m_customUserLayersGrid->CommitPendingChanges() )
    {
        return false;
    }

    KIGFX::PCB_VIEW*    view = m_frame->GetCanvas()->GetView();
    PCB_SELECTION_TOOL* selectionTool = m_frame->GetToolManager()->GetTool<PCB_SELECTION_TOOL>();
    BOARD_COMMIT        commit( m_frame );
    commit.Modify( m_footprint );

    // Must be done inside the commit to capture the undo state
    // This will call TransferDataToWindow() on the 3D panel and
    // the embedded files panel.
    if( !DIALOG_SHIM::TransferDataFromWindow() )
        return false;

    // Clear out embedded files that are no longer in use
    std::set<wxString> files;
    std::set<wxString> files_to_delete;

    // Get the new files from the footprint fields
    for( const PCB_FIELD& field : *m_fields )
    {
        if( field.GetText().StartsWith( FILEEXT::KiCadUriPrefix ) )
            files.insert( field.GetText() );
    }

    // Find any files referenced in the old fields that are not in the new fields
    for( PCB_FIELD* field : m_footprint->GetFields() )
    {
        wxCHECK2( field, continue );

        if( field->GetText().StartsWith( FILEEXT::KiCadUriPrefix ) )
        {
            if( files.find( field->GetText() ) == files.end() )
                files_to_delete.insert( field->GetText() );
        }
    }

    for( const wxString& file : files_to_delete )
    {
        wxString name = file.Mid( FILEEXT::KiCadUriPrefix.size() + 3 ); // Skip "kicad-embed://"
        m_footprint->RemoveFile( name );
    }

    LIB_ID fpID = m_footprint->GetFPID();
    fpID.SetLibItemName( m_FootprintNameCtrl->GetValue() );
    m_footprint->SetFPID( fpID );

    m_footprint->SetLibDescription( UnescapeString( m_DocCtrl->GetValue() ) );
    m_footprint->SetKeywords( m_KeywordCtrl->GetValue() );

    // Update fields
    m_frame->GetToolManager()->RunAction( ACTIONS::selectionClear );

    while( !m_footprint->GetFields().empty() )
    {
        PCB_FIELD* existing = m_footprint->GetFields().front();
        view->Remove( existing );
        m_footprint->Remove( existing );
        delete existing;
    }

    for( PCB_FIELD& field : *m_fields )
    {
        PCB_FIELD* newField = field.CloneField();
        m_footprint->Add( newField );
        view->Add( newField );

        if( newField->IsSelected() )
        {
            // The old copy was in the selection list, but this one is not.  Remove the
            // out-of-sync selection flag so we can re-add the field to the selection.
            newField->ClearSelected();
            selectionTool->AddItemToSel( newField, true );
        }
    }

    LSET privateLayers;

    for( PCB_LAYER_ID layer : *m_privateLayers )
        privateLayers.set( layer );

    m_footprint->SetPrivateLayers( privateLayers );

    if( m_cbCustomLayers->GetValue() )
    {
        const LSET customLayers = getCustomLayersFromControls();

        m_footprint->SetStackupMode( FOOTPRINT_STACKUP::CUSTOM_LAYERS );
        m_footprint->SetStackupLayers( std::move( customLayers ) );
    }
    else
    {
        // Just use the default stackup mode
        m_footprint->SetStackupMode( FOOTPRINT_STACKUP::EXPAND_INNER_LAYERS );
    }

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

    if( m_cbDNP->GetValue() )
        attributes |= FP_DNP;

    m_footprint->SetAttributes( attributes );

    m_footprint->SetAllowMissingCourtyard( m_noCourtyards->GetValue() );
    m_footprint->SetAllowSolderMaskBridges( m_allowBridges->GetValue() );

    // Initialize mask clearances
    if( m_netClearance.IsNull() )
        m_footprint->SetLocalClearance( {} );
    else
        m_footprint->SetLocalClearance( m_netClearance.GetValue() );

    if( m_solderMask.IsNull() )
        m_footprint->SetLocalSolderMaskMargin( {} );
    else
        m_footprint->SetLocalSolderMaskMargin( m_solderMask.GetValue() );

    m_footprint->SetLocalSolderPasteMargin( m_solderPaste.GetOffsetValue() );
    m_footprint->SetLocalSolderPasteMarginRatio( m_solderPaste.GetRatioValue() );

    switch( m_ZoneConnectionChoice->GetSelection() )
    {
    default:
    case 0:  m_footprint->SetLocalZoneConnection( ZONE_CONNECTION::INHERITED ); break;
    case 1:  m_footprint->SetLocalZoneConnection( ZONE_CONNECTION::FULL );      break;
    case 2:  m_footprint->SetLocalZoneConnection( ZONE_CONNECTION::THERMAL );   break;
    case 3:  m_footprint->SetLocalZoneConnection( ZONE_CONNECTION::NONE );      break;
    }

    m_footprint->ClearNetTiePadGroups();

    for( int ii = 0; ii < m_nettieGroupsGrid->GetNumberRows(); ++ii )
    {
        wxString group = m_nettieGroupsGrid->GetCellValue( ii, 0 );

        if( !group.IsEmpty() )
            m_footprint->AddNetTiePadGroup( group );
    }

    m_footprint->SetDuplicatePadNumbersAreJumpers( m_cbDuplicatePadsAreJumpers->GetValue() );

    std::vector<std::set<wxString>>& jumpers = m_footprint->JumperPadGroups();
    jumpers.clear();

    for( int ii = 0; ii < m_jumperGroupsGrid->GetNumberRows(); ++ii )
    {
        wxStringTokenizer tokenizer( m_jumperGroupsGrid->GetCellValue( ii, 0 ), ", \t\r\n", wxTOKEN_STRTOK );
        std::set<wxString>& group = jumpers.emplace_back();

        while( tokenizer.HasMoreTokens() )
        {
            if( wxString token = tokenizer.GetNextToken(); !token.IsEmpty() )
                group.insert( token );
        }
    }

    // Copy the models from the panel to the footprint
    std::vector<FP_3DMODEL>& panelList = m_3dPanel->GetModelList();
    std::vector<FP_3DMODEL>* fpList    = &m_footprint->Models();
    fpList->clear();
    fpList->insert( fpList->end(), panelList.begin(), panelList.end() );

    commit.Push( _( "Edit Footprint Properties" ) );

    return true;
}


void DIALOG_FOOTPRINT_PROPERTIES_FP_EDITOR::OnAddField( wxCommandEvent& event )
{
    m_itemsGrid->OnAddRow(
            [&]() -> std::pair<int, int>
            {
                const BOARD_DESIGN_SETTINGS& dsnSettings = m_frame->GetDesignSettings();

                PCB_FIELD newField( m_footprint, FIELD_T::USER, GetUserFieldName( m_fields->size(), DO_TRANSLATE ) );

                // Set active layer if legal; otherwise copy layer from previous text item
                if( LSET::AllTechMask().test( m_frame->GetActiveLayer() ) )
                    newField.SetLayer( m_frame->GetActiveLayer() );
                else
                    newField.SetLayer( m_fields->at( m_fields->size() - 1 ).GetLayer() );

                newField.SetTextSize( dsnSettings.GetTextSize( newField.GetLayer() ) );
                newField.SetTextThickness( dsnSettings.GetTextThickness( newField.GetLayer() ) );
                newField.SetItalic( dsnSettings.GetTextItalic( newField.GetLayer() ) );

                m_fields->push_back( newField );

                // notify the grid
                wxGridTableMessage msg( m_fields, wxGRIDTABLE_NOTIFY_ROWS_APPENDED, 1 );
                m_itemsGrid->ProcessTableMessage( msg );
                OnModify();

                return { m_fields->size() - 1, PFC_NAME };
            } );
}


void DIALOG_FOOTPRINT_PROPERTIES_FP_EDITOR::OnDeleteField( wxCommandEvent& event )
{
    m_itemsGrid->OnDeleteRows(
            [&]( int row )
            {
                if( row < m_fields->GetMandatoryRowCount() )
                {
                    DisplayError( this, wxString::Format( _( "The first %d fields are mandatory." ),
                                                          m_fields->GetMandatoryRowCount() ) );
                    return false;
                }

                return true;
            },
            [&]( int row )
            {
                m_fields->erase( m_fields->begin() + row );

                // notify the grid
                wxGridTableMessage msg( m_fields, wxGRIDTABLE_NOTIFY_ROWS_DELETED, row, 1 );
                m_itemsGrid->ProcessTableMessage( msg );
            } );

    OnModify();
}


void DIALOG_FOOTPRINT_PROPERTIES_FP_EDITOR::onLayerGridRowDelete( WX_GRID& aGrid, LAYERS_GRID_TABLE& aLayerTable,
                                                                  int aRow )
{
    aLayerTable.erase( aLayerTable.begin() + aRow );

    // notify the grid
    wxGridTableMessage msg( &aLayerTable, wxGRIDTABLE_NOTIFY_ROWS_DELETED, aRow, 1 );
    aGrid.ProcessTableMessage( msg );

    OnModify();
}


std::pair<int, int> DIALOG_FOOTPRINT_PROPERTIES_FP_EDITOR::onLayerGridRowAddUserLayer( WX_GRID&           aGrid,
                                                                                       LAYERS_GRID_TABLE& aGridTable )
{
    PCB_LAYER_ID nextLayer = User_1;

    while( alg::contains( aGridTable, nextLayer ) && nextLayer < User_45 )
        nextLayer = ToLAYER_ID( nextLayer + 2 );

    aGridTable.push_back( nextLayer );

    // notify the grid
    wxGridTableMessage msg( &aGridTable, wxGRIDTABLE_NOTIFY_ROWS_APPENDED, 1 );
    aGrid.ProcessTableMessage( msg );
    OnModify();

    return { aGridTable.size() - 1, 0 };
}


void DIALOG_FOOTPRINT_PROPERTIES_FP_EDITOR::OnAddPrivateLayer( wxCommandEvent& event )
{
    m_privateLayersGrid->OnAddRow(
            [&]()
            {
                return onLayerGridRowAddUserLayer( *m_privateLayersGrid, *m_privateLayers );
            } );
}


void DIALOG_FOOTPRINT_PROPERTIES_FP_EDITOR::OnDeletePrivateLayer( wxCommandEvent& event )
{
    m_privateLayersGrid->OnDeleteRows(
            [&]( int row )
            {
                onLayerGridRowDelete( *m_privateLayersGrid, *m_privateLayers, row );
            } );
}


void DIALOG_FOOTPRINT_PROPERTIES_FP_EDITOR::OnUseCustomLayers( wxCommandEvent& event )
{
    setCustomLayerCtrlEnablement();
}


void DIALOG_FOOTPRINT_PROPERTIES_FP_EDITOR::OnAddCustomLayer( wxCommandEvent& event )
{
    m_customUserLayersGrid->OnAddRow(
            [&]()
            {
                return onLayerGridRowAddUserLayer( *m_customUserLayersGrid, *m_customUserLayers );
            } );
}


void DIALOG_FOOTPRINT_PROPERTIES_FP_EDITOR::OnDeleteCustomLayer( wxCommandEvent& event )
{
    m_customUserLayersGrid->OnDeleteRows(
            [&]( int row )
            {
                onLayerGridRowDelete( *m_customUserLayersGrid, *m_customUserLayers, row );
            } );
}


void DIALOG_FOOTPRINT_PROPERTIES_FP_EDITOR::OnAddNettieGroup( wxCommandEvent& event )
{
    onAddGroup( m_nettieGroupsGrid );
}


void DIALOG_FOOTPRINT_PROPERTIES_FP_EDITOR::OnRemoveNettieGroup( wxCommandEvent& event )
{
    onRemoveGroup( m_nettieGroupsGrid );
}


void DIALOG_FOOTPRINT_PROPERTIES_FP_EDITOR::OnAddJumperGroup( wxCommandEvent& event )
{
    onAddGroup( m_jumperGroupsGrid );
}


void DIALOG_FOOTPRINT_PROPERTIES_FP_EDITOR::OnRemoveJumperGroup( wxCommandEvent& event )
{
    onRemoveGroup( m_jumperGroupsGrid );
}


void DIALOG_FOOTPRINT_PROPERTIES_FP_EDITOR::onAddGroup( WX_GRID* aGrid )
{
    aGrid->OnAddRow(
            [&]() -> std::pair<int, int>
            {
                aGrid->AppendRows( 1 );
                OnModify();

                return { aGrid->GetNumberRows() - 1, 0 };
            } );
}


void DIALOG_FOOTPRINT_PROPERTIES_FP_EDITOR::onRemoveGroup( WX_GRID* aGrid )
{
    aGrid->OnDeleteRows(
            [&]( int row )
            {
                aGrid->DeleteRows( row, 1 );
            } );

    OnModify();
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

        if( !( m_delayedFocusColumn == 0 && m_delayedFocusRow < m_fields->GetMandatoryRowCount() ) )
            m_delayedFocusGrid->EnableCellEditControl( true );

        m_delayedFocusGrid->ShowCellEditControl();

        m_delayedFocusGrid = nullptr;
        m_delayedFocusRow = -1;
        m_delayedFocusColumn = -1;
    }
}


void DIALOG_FOOTPRINT_PROPERTIES_FP_EDITOR::setCustomLayerCtrlEnablement()
{
    bool enableCustomCtrls = m_cbCustomLayers->GetValue();

    m_copperLayerCount->Enable( enableCustomCtrls );
    m_customUserLayersGrid->Enable( enableCustomCtrls );
    m_bpAddCustomLayer->Enable( enableCustomCtrls );
    m_bpDeleteCustomLayer->Enable( enableCustomCtrls );
}


void DIALOG_FOOTPRINT_PROPERTIES_FP_EDITOR::OnPageChanging( wxNotebookEvent& aEvent )
{
    if( !m_itemsGrid->CommitPendingChanges() )
        aEvent.Veto();

    if( !m_privateLayersGrid->CommitPendingChanges() )
        aEvent.Veto();

    if( !m_customUserLayersGrid->CommitPendingChanges() )
        aEvent.Veto();
}


void DIALOG_FOOTPRINT_PROPERTIES_FP_EDITOR::OnCheckBox( wxCommandEvent& event )
{
    if( m_initialized )
        OnModify();
}


void DIALOG_FOOTPRINT_PROPERTIES_FP_EDITOR::OnText( wxCommandEvent& event )
{
    if( m_initialized )
        OnModify();
}


void DIALOG_FOOTPRINT_PROPERTIES_FP_EDITOR::OnChoice( wxCommandEvent& event )
{
    if( m_initialized )
        OnModify();
}
