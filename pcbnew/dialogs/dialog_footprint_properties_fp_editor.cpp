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

#include <3d_rendering/opengl/3d_model.h>
#include <3d_viewer/eda_3d_viewer_frame.h>
#include <bitmaps.h>
#include <board_commit.h>
#include <board_design_settings.h>
#include <confirm.h>
#include <dialog_footprint_properties_fp_editor.h>
#include <dialogs/dialog_text_entry.h>
#include <dialogs/panel_preview_3d_model.h>
#include <embedded_files.h>
#include <filename_resolver.h>
#include <footprint.h>
#include <footprint_edit_frame.h>
#include <footprint_editor_settings.h>
#include <grid_layer_box_helpers.h>
#include <kiplatform/ui.h>
#include <panel_embedded_files.h>
#include <panel_fp_properties_3d_model.h>
#include <pgm_base.h>
#include <settings/settings_manager.h>
#include <tool/tool_manager.h>
#include <tools/pcb_selection_tool.h>
#include <validators.h>
#include <widgets/grid_text_button_helpers.h>
#include <widgets/std_bitmap_button.h>
#include <widgets/text_ctrl_eval.h>
#include <widgets/wx_grid.h>

#include <fp_lib_table.h>
#include <project_pcb.h>
#include <kidialog.h>

PRIVATE_LAYERS_GRID_TABLE::PRIVATE_LAYERS_GRID_TABLE( PCB_BASE_FRAME* aFrame ) :
        m_frame( aFrame )
{
    m_layerColAttr = new wxGridCellAttr;
    m_layerColAttr->SetRenderer( new GRID_CELL_LAYER_RENDERER( m_frame ) );

    LSET forbiddenLayers = LSET::AllCuMask() | LSET::AllTechMask();
    forbiddenLayers.set( Edge_Cuts );
    forbiddenLayers.set( Margin );
    m_layerColAttr->SetEditor( new GRID_CELL_LAYER_SELECTOR( m_frame, forbiddenLayers, true ) );
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
                                                    wxGridCellAttr::wxAttrKind aKind  )
{
    m_layerColAttr->IncRef();
    return enhanceAttr( m_layerColAttr, aRow, aCol, aKind );
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
        m_initialized( false ),
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

    // Create the extra panels.
    m_embeddedFiles = new PANEL_EMBEDDED_FILES( m_NoteBook, m_footprint );
    m_3dPanel = new PANEL_FP_PROPERTIES_3D_MODEL( m_frame, m_footprint, this, m_embeddedFiles, m_NoteBook );

    m_NoteBook->AddPage( m_3dPanel, _("3D Models"), false );
    m_NoteBook->AddPage( m_embeddedFiles, _( "Embedded Files" ) );

    m_fields = new PCB_FIELDS_GRID_TABLE( m_frame, this, { m_embeddedFiles->GetLocalFiles() } );
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

    m_itemsGrid->SetTable( m_fields );
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
    m_bpAdd->SetBitmap( KiBitmapBundle( BITMAPS::small_plus ) );
    m_bpDelete->SetBitmap( KiBitmapBundle( BITMAPS::small_trash ) );
    m_bpAddLayer->SetBitmap( KiBitmapBundle( BITMAPS::small_plus ) );
    m_bpDeleteLayer->SetBitmap( KiBitmapBundle( BITMAPS::small_trash ) );
    m_bpAddPadGroup->SetBitmap( KiBitmapBundle( BITMAPS::small_plus ) );
    m_bpRemovePadGroup->SetBitmap( KiBitmapBundle( BITMAPS::small_trash ) );

    SetupStandardButtons();

    finishDialogSettings();
    SetEvtHandlerEnabled( true );
}


DIALOG_FOOTPRINT_PROPERTIES_FP_EDITOR::~DIALOG_FOOTPRINT_PROPERTIES_FP_EDITOR()
{
    m_frame->GetSettings()->m_FootprintTextShownColumns = m_itemsGrid->GetShownColumnsAsString();

    // Prevents crash bug in wxGrid's d'tor
    m_itemsGrid->DestroyTable( m_fields );
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
        m_fields->push_back( *field );

    // Notify the grid
    wxGridTableMessage tmsg( m_fields, wxGRIDTABLE_NOTIFY_ROWS_APPENDED,
                             m_fields->GetNumberRows() );
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

    if( m_footprint->GetLocalClearance().has_value() )
        m_netClearance.SetValue( m_footprint->GetLocalClearance().value() );
    else
        m_netClearance.SetValue( wxEmptyString );

    if( m_footprint->GetLocalSolderMaskMargin().has_value() )
        m_solderMask.SetValue( m_footprint->GetLocalSolderMaskMargin().value() );
    else
        m_solderMask.SetValue( wxEmptyString );

    if( m_footprint->GetLocalSolderPasteMargin().has_value() )
        m_solderPaste.SetValue( m_footprint->GetLocalSolderPasteMargin().value() );
    else
        m_solderPaste.SetValue( wxEmptyString );

    if( m_footprint->GetLocalSolderPasteMarginRatio().has_value() )
        m_solderPasteRatio.SetDoubleValue( m_footprint->GetLocalSolderPasteMarginRatio().value() * 100.0 );
    else
        m_solderPasteRatio.SetValue( wxEmptyString );

    m_allowBridges->SetValue( m_footprint->GetAttributes() & FP_ALLOW_SOLDERMASK_BRIDGES );

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

        if( col == PFC_LAYER )  // This one's a drop-down.  Check all possible values.
        {
            BOARD* board = m_footprint->GetBoard();

            for( PCB_LAYER_ID layer : board->GetEnabledLayers().Seq() )
                col_size = std::max( col_size, GetTextExtent( board->GetLayerName( layer ) ).x );

            // Swatch and gaps:
            col_size += KiROUND( 14 * GetDPIScaleFactor() ) + 12;
        }

        if( m_itemsGrid->IsColShown( col ) )
            m_itemsGrid->SetColSize( col, col_size );
    }

    m_itemsGrid->SetRowLabelSize( 0 );

    Layout();
    adjustGridColumns();
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
    FP_LIB_TABLE* tbl = PROJECT_PCB::PcbFootprintLibs( &m_frame->Prj() );

    if( aFootprintName != originalFPName && tbl->FootprintExists( libraryName, aFootprintName ) )
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
            m_itemsGrid->SetCellValue( i, PFC_THICKNESS,
                                       m_frame->StringFromValue( maxPenWidth, true ) );

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

    return true;
}


bool DIALOG_FOOTPRINT_PROPERTIES_FP_EDITOR::TransferDataFromWindow()
{
    if( !m_itemsGrid->CommitPendingChanges()
            || !m_privateLayersGrid->CommitPendingChanges()
            || !m_padGroupsGrid->CommitPendingChanges() )
    {
        return false;
    }

    KIGFX::PCB_VIEW* view = m_frame->GetCanvas()->GetView();
    BOARD_COMMIT     commit( m_frame );
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
    for( PCB_FIELD& m_field : *m_fields)
    {
        const wxString& name = m_field.GetText();

        if( name.StartsWith( FILEEXT::KiCadUriPrefix ) )
            files.insert( name );
    }

    // Find any files referenced in the old fields that are not in the new fields
    for( PCB_FIELD* field : m_footprint->GetFields() )
    {
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

    std::vector<PCB_FIELD*> items_to_remove;
    size_t                  i = 0;

    for( PCB_FIELD* field : m_footprint->GetFields() )
    {
        // copy grid table entries till we run out, then delete any remaining texts
        if( i < m_fields->size() )
            *field = m_fields->at( i++ );
        else
            items_to_remove.push_back( field );
    }

    // Remove text items:
    PCB_SELECTION_TOOL* selTool = m_frame->GetToolManager()->GetTool<PCB_SELECTION_TOOL>();

    for( PCB_TEXT* item : items_to_remove )
    {
        selTool->RemoveItemFromSel( item );
        view->Remove( item );
        item->DeleteStructure();
    }

    // if there are still grid table entries, create new fields for them
    while( i < m_fields->size() )
        view->Add( m_footprint->AddField( m_fields->at( i++ ) ) );

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

    // Initialize mask clearances
    if( m_netClearance.IsNull() )
        m_footprint->SetLocalClearance( {} );
    else
        m_footprint->SetLocalClearance( m_netClearance.GetValue() );

    if( m_solderMask.IsNull() )
        m_footprint->SetLocalSolderMaskMargin( {} );
    else
        m_footprint->SetLocalSolderMaskMargin( m_solderMask.GetValue() );

    if( m_solderPaste.IsNull() )
        m_footprint->SetLocalSolderPasteMargin( {} );
    else
        m_footprint->SetLocalSolderPasteMargin( m_solderPaste.GetValue() );

    if( m_solderPasteRatio.IsNull() )
        m_footprint->SetLocalSolderPasteMarginRatio( {} );
    else
        m_footprint->SetLocalSolderPasteMarginRatio( m_solderPasteRatio.GetDoubleValue() / 100.0 );

    switch( m_ZoneConnectionChoice->GetSelection() )
    {
    default:
    case 0:  m_footprint->SetLocalZoneConnection( ZONE_CONNECTION::INHERITED ); break;
    case 1:  m_footprint->SetLocalZoneConnection( ZONE_CONNECTION::FULL );      break;
    case 2:  m_footprint->SetLocalZoneConnection( ZONE_CONNECTION::THERMAL );   break;
    case 3:  m_footprint->SetLocalZoneConnection( ZONE_CONNECTION::NONE );      break;
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

    commit.Push( _( "Edit Footprint Properties" ) );

    return true;
}


void DIALOG_FOOTPRINT_PROPERTIES_FP_EDITOR::OnAddField( wxCommandEvent& event )
{
    if( !m_itemsGrid->CommitPendingChanges() )
        return;

    const BOARD_DESIGN_SETTINGS& dsnSettings = m_frame->GetDesignSettings();

    PCB_FIELD newField( m_footprint, m_footprint->GetNextFieldId(),
                        GetUserFieldName( m_fields->GetNumberRows(), DO_TRANSLATE ) );

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

    m_itemsGrid->SetFocus();
    m_itemsGrid->MakeCellVisible( (int) m_fields->size() - 1, 0 );
    m_itemsGrid->SetGridCursor( (int) m_fields->size() - 1, 0 );

    m_itemsGrid->EnableCellEditControl( true );
    m_itemsGrid->ShowCellEditControl();

    OnModify();
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
        if( row < m_fields->GetMandatoryRowCount() )
        {
            DisplayError( this, wxString::Format( _( "The first %d fields are mandatory." ),
                                                  m_fields->GetMandatoryRowCount() ) );
            return;
        }
    }

    m_itemsGrid->CommitPendingChanges( true /* quiet mode */ );

    // Reverse sort so deleting a row doesn't change the indexes of the other rows.
    selectedRows.Sort( []( int* first, int* second )
                       {
                           return *second - *first;
                       } );

    for( int row : selectedRows )
    {
        m_itemsGrid->ClearSelection();
        m_fields->erase( m_fields->begin() + row );

        // notify the grid
        wxGridTableMessage msg( m_fields, wxGRIDTABLE_NOTIFY_ROWS_DELETED, row, 1 );
        m_itemsGrid->ProcessTableMessage( msg );

        if( m_itemsGrid->GetNumberRows() > 0 )
        {
            m_itemsGrid->MakeCellVisible( std::max( 0, row-1 ), m_itemsGrid->GetGridCursorCol() );
            m_itemsGrid->SetGridCursor( std::max( 0, row-1 ), m_itemsGrid->GetGridCursorCol() );
        }
    }

    OnModify();
}


void DIALOG_FOOTPRINT_PROPERTIES_FP_EDITOR::OnAddLayer( wxCommandEvent& event )
{
    if( !m_privateLayersGrid->CommitPendingChanges() )
        return;

    PCB_LAYER_ID nextLayer = User_1;

    while( alg::contains( *m_privateLayers, nextLayer ) && nextLayer < User_45 )
        nextLayer = ToLAYER_ID( nextLayer + 2 );

    m_privateLayers->push_back( nextLayer );

    // notify the grid
    wxGridTableMessage msg( m_privateLayers, wxGRIDTABLE_NOTIFY_ROWS_APPENDED, 1 );
    m_privateLayersGrid->ProcessTableMessage( msg );

    m_privateLayersGrid->SetFocus();
    m_privateLayersGrid->MakeCellVisible( (int) m_privateLayers->size() - 1, 0 );
    m_privateLayersGrid->SetGridCursor( (int) m_privateLayers->size() - 1, 0 );

    OnModify();
}


void DIALOG_FOOTPRINT_PROPERTIES_FP_EDITOR::OnDeleteLayer( wxCommandEvent& event )
{
    if( !m_privateLayersGrid->CommitPendingChanges() )
        return;

    int curRow = m_privateLayersGrid->GetGridCursorRow();

    if( curRow < 0 )
        return;

    m_privateLayersGrid->ClearSelection();
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

    OnModify();
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

    OnModify();
}


void DIALOG_FOOTPRINT_PROPERTIES_FP_EDITOR::OnRemovePadGroup( wxCommandEvent& event )
{
    if( !m_padGroupsGrid->CommitPendingChanges() )
        return;

    wxArrayInt selectedRows = m_padGroupsGrid->GetSelectedRows();
    int        curRow = m_padGroupsGrid->GetGridCursorRow();

    if( selectedRows.empty() && curRow >= 0 && curRow < m_padGroupsGrid->GetNumberRows() )
        selectedRows.Add( curRow );

    for( int ii = (int) selectedRows.Count() - 1; ii >= 0; --ii )
    {
        int row = selectedRows.Item( ii );
        m_padGroupsGrid->DeleteRows( row, 1 );
        curRow = std::min( curRow, row );
    }

    curRow = std::max( 0, curRow - 1 );
    m_padGroupsGrid->MakeCellVisible( curRow, m_padGroupsGrid->GetGridCursorCol() );
    m_padGroupsGrid->SetGridCursor( curRow, m_padGroupsGrid->GetGridCursorCol() );

    OnModify();
}


void DIALOG_FOOTPRINT_PROPERTIES_FP_EDITOR::adjustGridColumns()
{
    // Account for scroll bars
    int itemsWidth = KIPLATFORM::UI::GetUnobscuredSize( m_itemsGrid ).x;

    itemsWidth -= m_itemsGrid->GetRowLabelSize();

    for( int i = 0; i < m_itemsGrid->GetNumberCols(); i++ )
    {
        if( i == 1 )
            continue;

        itemsWidth -= m_itemsGrid->GetColSize( i );
    }

    m_itemsGrid->SetColSize(
            1, std::max( itemsWidth, m_itemsGrid->GetVisibleWidth( 0, true, false ) ) );

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

        if( !( m_delayedFocusColumn == 0 && m_delayedFocusRow < m_fields->GetMandatoryRowCount() ) )
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


void DIALOG_FOOTPRINT_PROPERTIES_FP_EDITOR::OnPageChanging( wxNotebookEvent& aEvent )
{
    if( !m_itemsGrid->CommitPendingChanges() )
        aEvent.Veto();

    if( !m_privateLayersGrid->CommitPendingChanges() )
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
