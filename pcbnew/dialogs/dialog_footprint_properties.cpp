/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2016 Mario Luzeiro <mrluzeiro@ua.pt>
 * Copyright (C) 2018 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2015 Dick Hollenbeck, dick@softplc.com
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

#include <3d_viewer/eda_3d_viewer_frame.h>
#include <bitmaps.h>
#include <board_commit.h>
#include <board_design_settings.h>
#include <board.h>
#include <footprint.h>
#include <confirm.h>
#include <dialogs/dialog_text_entry.h>
#include <filename_resolver.h>
#include <pcb_edit_frame.h>
#include <pcbnew_settings.h>
#include <pgm_base.h>
#include <project_pcb.h>
#include <kiplatform/ui.h>
#include <widgets/grid_text_button_helpers.h>
#include <widgets/text_ctrl_eval.h>
#include <widgets/std_bitmap_button.h>
#include <settings/settings_manager.h>
#include <panel_embedded_files.h>
#include <panel_fp_properties_3d_model.h>
#include <dialogs/panel_preview_3d_model.h>
#include <dialog_footprint_properties.h>
#include <tool/tool_manager.h>
#include <tools/pcb_selection_tool.h>


int DIALOG_FOOTPRINT_PROPERTIES::m_page = 0;     // remember the last open page during session


DIALOG_FOOTPRINT_PROPERTIES::DIALOG_FOOTPRINT_PROPERTIES( PCB_EDIT_FRAME* aParent, FOOTPRINT* aFootprint ) :
        DIALOG_FOOTPRINT_PROPERTIES_BASE( aParent ),
        m_frame( aParent ),
        m_footprint( aFootprint ),
        m_posX( aParent, m_XPosLabel, m_ModPositionX, m_XPosUnit ),
        m_posY( aParent, m_YPosLabel, m_ModPositionY, m_YPosUnit ),
        m_orientation( aParent, m_orientationLabel, m_orientationCtrl, nullptr ),
        m_netClearance( aParent, m_NetClearanceLabel, m_NetClearanceCtrl, m_NetClearanceUnits ),
        m_solderMask( aParent, m_SolderMaskMarginLabel, m_SolderMaskMarginCtrl, m_SolderMaskMarginUnits ),
        m_solderPaste( aParent, m_SolderPasteMarginLabel, m_SolderPasteMarginCtrl, m_SolderPasteMarginUnits ),
        m_returnValue( FP_PROPS_CANCEL ),
        m_initialized( false )
{
    // Create the extra panels.  Embedded files is referenced by the 3D model panel.
    m_embeddedFiles = new PANEL_EMBEDDED_FILES( m_NoteBook, m_footprint );
    m_3dPanel = new PANEL_FP_PROPERTIES_3D_MODEL( m_frame, m_footprint, this, m_embeddedFiles, m_NoteBook );

    m_NoteBook->AddPage( m_3dPanel, _("3D Models"), false );
    m_NoteBook->AddPage( m_embeddedFiles, _( "Embedded Files" ) );

    // Configure display origin transforms
    m_posX.SetCoordType( ORIGIN_TRANSFORMS::ABS_X_COORD );
    m_posY.SetCoordType( ORIGIN_TRANSFORMS::ABS_Y_COORD );

    std::vector<EMBEDDED_FILES*> embeddedFilesStack;
    embeddedFilesStack.push_back( m_embeddedFiles->GetLocalFiles() );
    embeddedFilesStack.push_back( m_frame->GetBoard()->GetEmbeddedFiles() );

    m_fields = new PCB_FIELDS_GRID_TABLE( m_frame, this, std::move( embeddedFilesStack ) );

    m_delayedErrorMessage = wxEmptyString;
    m_delayedFocusGrid = nullptr;
    m_delayedFocusRow = -1;
    m_delayedFocusColumn = -1;
    m_initialFocus = false;

    // Give an icon
    wxIcon  icon;
    icon.CopyFromBitmap( KiBitmap( BITMAPS::icon_modedit ) );
    SetIcon( icon );

    m_itemsGrid->SetTable( m_fields );
    m_itemsGrid->PushEventHandler( new GRID_TRICKS( m_itemsGrid ) );
    m_itemsGrid->SetupColumnAutosizer( PFC_VALUE );
    m_itemsGrid->ShowHideColumns( "0 1 2 3 4 5 7" );

    m_orientation.SetUnits( EDA_UNITS::DEGREES );
    m_orientation.SetPrecision( 3 );

    // Set predefined rotations in combo dropdown, according to the locale floating point
    // separator notation
    double rot_list[] = { 0.0, 90.0, -90.0, 180.0 };

    for( size_t ii = 0; ii < m_orientationCtrl->GetCount() && ii < 4; ++ii )
        m_orientationCtrl->SetString( ii, wxString::Format( "%.1f", rot_list[ii] ) );

    // Set font size for items showing long strings:
    wxFont infoFont = KIUI::GetSmallInfoFont( this );
    m_libraryIDLabel->SetFont( infoFont );
    m_tcLibraryID->SetFont( infoFont );

    infoFont = KIUI::GetInfoFont( this ).Italic();
    m_staticTextInfoCopper->SetFont( infoFont );
    m_staticTextInfoPaste->SetFont( infoFont );

    m_NoteBook->SetSelection( m_page );

    if( m_page == 0 )
    {
        m_delayedFocusGrid = m_itemsGrid;
        m_delayedFocusRow = 0;
        m_delayedFocusColumn = 0;
    }
    else if( m_page == 1 )
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

    m_tcLibraryID->SetBackgroundColour( KIPLATFORM::UI::GetDialogBGColour() );

    // We can't set the tab order through wxWidgets due to shortcomings in their mnemonics
    // implementation on MSW
    m_tabOrder = {
        m_itemsGrid,
        m_ModPositionX,
        m_ModPositionY,
        m_orientationCtrl,
        m_BoardSideCtrl,
        m_cbLocked,
        m_componentType,
        m_boardOnly,
        m_excludeFromPosFiles,
        m_excludeFromBOM,
        m_cbDNP,
      	m_NetClearanceCtrl,
        m_SolderMaskMarginCtrl,
      	m_allowSolderMaskBridges,
        m_SolderPasteMarginCtrl,
      	m_PasteMarginRatioCtrl,
        m_ZoneConnectionChoice
    };

    SetupStandardButtons();

    // The 3D model tab was added after the base dtor.  The final dialog size needs to be set
    // accordingly.
	SetSizer( m_GeneralBoxSizer );
	Layout();
	m_GeneralBoxSizer->Fit( this );

    finishDialogSettings();
}


DIALOG_FOOTPRINT_PROPERTIES::~DIALOG_FOOTPRINT_PROPERTIES()
{
    // Prevents crash bug in wxGrid's d'tor
    m_itemsGrid->DestroyTable( m_fields );

    // Delete the GRID_TRICKS.
    m_itemsGrid->PopEventHandler( true );

    // free the memory used by all models, otherwise models which were
    // browsed but not used would consume memory
    PROJECT_PCB::Get3DCacheManager( &Prj() )->FlushCache( false );

    // the GL canvas has to be visible before it is destroyed
    m_page = m_NoteBook->GetSelection();
    m_NoteBook->SetSelection( 1 );
}


void DIALOG_FOOTPRINT_PROPERTIES::EditFootprint( wxCommandEvent&  )
{
    if( TransferDataFromWindow() )
    {
        m_returnValue = FP_PROPS_EDIT_BOARD_FP;
        Close();
    }
}


void DIALOG_FOOTPRINT_PROPERTIES::EditLibraryFootprint( wxCommandEvent&  )
{
    if( TransferDataFromWindow() )
    {
        m_returnValue = FP_PROPS_EDIT_LIBRARY_FP;
        Close();
    }
}


void DIALOG_FOOTPRINT_PROPERTIES::UpdateFootprint( wxCommandEvent&  )
{
    if( TransferDataFromWindow() )
    {
        m_returnValue = FP_PROPS_UPDATE_FP;
        Close();
    }
}


void DIALOG_FOOTPRINT_PROPERTIES::ChangeFootprint( wxCommandEvent&  )
{
    if( TransferDataFromWindow() )
    {
        m_returnValue = FP_PROPS_CHANGE_FP;
        Close();
    }
}


bool DIALOG_FOOTPRINT_PROPERTIES::TransferDataToWindow()
{
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
    for( PCB_FIELD* srcField : m_footprint->GetFields() )
    {
        wxCHECK2( srcField, continue );

        PCB_FIELD field( *srcField );
        field.SetText( m_footprint->GetBoard()->ConvertKIIDsToCrossReferences( field.GetText() ) );

        m_fields->push_back( field );
    }

    // notify the grid
    wxGridTableMessage tmsg( m_fields, wxGRIDTABLE_NOTIFY_ROWS_APPENDED, m_fields->GetNumberRows() );
    m_itemsGrid->ProcessTableMessage( tmsg );

    // Footprint Properties

    m_posX.SetValue( m_footprint->GetPosition().x );
    m_posY.SetValue( m_footprint->GetPosition().y );

    m_BoardSideCtrl->SetSelection( (m_footprint->GetLayer() == B_Cu) ? 1 : 0 );

    EDA_ANGLE orientation = m_footprint->GetOrientation();
    m_orientation.SetAngleValue( orientation.Normalize180() );

    m_cbLocked->SetValue( m_footprint->IsLocked() );
    m_cbLocked->SetToolTip( _( "Locked footprints cannot be freely moved or oriented on the canvas and can only be "
                               "selected when the 'Locked items' checkbox is checked in the selection filter." ) );

    if( m_footprint->GetAttributes() & FP_THROUGH_HOLE )
        m_componentType->SetSelection( 0 );
    else if( m_footprint->GetAttributes() & FP_SMD )
        m_componentType->SetSelection( 1 );
    else
        m_componentType->SetSelection( 2 );

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

    m_allowSolderMaskBridges->SetValue( m_footprint->AllowSolderMaskBridges() );

    switch( m_footprint->GetLocalZoneConnection() )
    {
    default:
    case ZONE_CONNECTION::INHERITED: m_ZoneConnectionChoice->SetSelection( 0 ); break;
    case ZONE_CONNECTION::FULL:      m_ZoneConnectionChoice->SetSelection( 1 ); break;
    case ZONE_CONNECTION::THERMAL:   m_ZoneConnectionChoice->SetSelection( 2 ); break;
    case ZONE_CONNECTION::NONE:      m_ZoneConnectionChoice->SetSelection( 3 ); break;
    }

    wxString jumperGroups;

    if( m_footprint->GetDuplicatePadNumbersAreJumpers() )
        jumperGroups = _( "all pads with duplicate numbers" );

    for( const std::set<wxString>& group : m_footprint->JumperPadGroups() )
    {
        wxString groupTxt;

        for( const wxString& pinNumber : group )
        {
            if( !groupTxt.IsEmpty() )
                groupTxt << ", ";

            groupTxt << pinNumber;
        }

        if( !jumperGroups.IsEmpty() )
            jumperGroups << ";  ";

        jumperGroups << "(" << groupTxt << ")";
    }

    if( jumperGroups.IsEmpty() )
    {
        m_jumperGroupsText->SetLabel( _( "none" ) );
        m_jumperGroupsText->SetFont( KIUI::GetControlFont( m_jumperGroupsText ).Italic() );
    }
    else
    {
        m_jumperGroupsText->SetLabel( jumperGroups );
    }

    // Show the footprint's FPID.
    m_tcLibraryID->SetValue( m_footprint->GetFPID().Format() );

    for( int col = 0; col < m_itemsGrid->GetNumberCols(); col++ )
    {
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


bool DIALOG_FOOTPRINT_PROPERTIES::Validate()
{
    if( !m_itemsGrid->CommitPendingChanges() )
        return false;

    if( !DIALOG_SHIM::Validate() )
        return false;

    // Validate texts.
    for( size_t i = 0; i < m_fields->size(); ++i )
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
        int width = m_frame->ValueFromString( m_itemsGrid->GetCellValue( i, PFC_WIDTH ) );
        int height = m_frame->ValueFromString( m_itemsGrid->GetCellValue( i, PFC_HEIGHT ) );

        if( width < minSize )
        {
            wxString min = m_frame->StringFromValue( minSize, true );

            m_itemsGrid->SetCellValue( i, PFC_WIDTH, min );

            m_delayedFocusGrid = m_itemsGrid;
            m_delayedErrorMessage = wxString::Format( _( "Text width must be at least %s." ), min );
            m_delayedFocusColumn = PFC_WIDTH;
            m_delayedFocusRow = i;

            return false;
        }
        else if( width > maxSize )
        {
            wxString max = m_frame->StringFromValue( maxSize, true );

            m_itemsGrid->SetCellValue( i, PFC_WIDTH, max );

            m_delayedFocusGrid = m_itemsGrid;
            m_delayedErrorMessage = wxString::Format( _( "Text width must be at most %s." ), max );
            m_delayedFocusColumn = PFC_WIDTH;
            m_delayedFocusRow = i;

            return false;
        }

        if( height < minSize )
        {
            wxString min = m_frame->StringFromValue( minSize, true );

            m_itemsGrid->SetCellValue( i, PFC_HEIGHT, min );

            m_delayedFocusGrid = m_itemsGrid;
            m_delayedErrorMessage = wxString::Format( _( "Text height must be at least %s." ), min );
            m_delayedFocusColumn = PFC_HEIGHT;
            m_delayedFocusRow = i;

            return false;
        }
        else if( height > maxSize )
        {
            wxString max = m_frame->StringFromValue( maxSize, true );

            m_itemsGrid->SetCellValue( i, PFC_HEIGHT, max );

            m_delayedFocusGrid = m_itemsGrid;
            m_delayedErrorMessage = wxString::Format( _( "Text height must be at most %s." ), max );
            m_delayedFocusColumn = PFC_HEIGHT;
            m_delayedFocusRow = i;

            return false;
        }

        // Test for acceptable values for thickness and size and clamp if fails
        int maxPenWidth = ClampTextPenSize( field.GetTextThickness(), field.GetTextSize() );

        if( field.GetTextThickness() > maxPenWidth )
        {
            wxString clamped = m_frame->StringFromValue( maxPenWidth, true );

            m_itemsGrid->SetCellValue( i, PFC_THICKNESS, clamped );

            m_delayedFocusGrid = m_itemsGrid;
            m_delayedErrorMessage = wxString::Format( _( "Text thickness is too large for the text size.\n"
                                                         "It will be clamped at %s." ),
                                                      clamped );
            m_delayedFocusColumn = PFC_THICKNESS;
            m_delayedFocusRow = i;

            return false;
        }
    }

    if( !m_netClearance.Validate( 0, INT_MAX ) )
        return false;

    return true;
}


bool DIALOG_FOOTPRINT_PROPERTIES::TransferDataFromWindow()
{
    if( !Validate() )
        return false;

    if( !m_itemsGrid->CommitPendingChanges() )
        return false;

    KIGFX::PCB_VIEW*    view = m_frame->GetCanvas()->GetView();
    PCB_SELECTION_TOOL* selectionTool = m_frame->GetToolManager()->GetTool<PCB_SELECTION_TOOL>();
    BOARD_COMMIT        commit( m_frame );
    commit.Modify( m_footprint );

    // Make sure this happens inside a commit to capture any changed files
    if( !m_3dPanel->TransferDataFromWindow() )
        return false;

    if( !m_embeddedFiles->TransferDataFromWindow() )
        return false;

    // Clear out embedded files that are no longer in use
    std::set<wxString> files;
    std::set<wxString> files_to_delete;

    // Get the new files from the footprint fields
    for( PCB_FIELD& field : *m_fields )
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

    // Update fields
    for( PCB_FIELD* existing : m_footprint->GetFields() )
        delete existing;

    m_footprint->GetFields().clear();

    int ordinal = 42;   // Arbitrarily larger than any mandatory FIELD_T ids.

    for( PCB_FIELD& field : *m_fields )
    {
        PCB_FIELD* newField = field.CloneField();
        newField->SetText( commit.GetBoard()->ConvertCrossReferencesToKIIDs( field.GetText() ) );

        if( !field.IsMandatory() )
            newField->SetOrdinal( ordinal++ );

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

    // Initialize masks clearances
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

    // Set Footprint Position
    VECTOR2I pos( m_posX.GetValue(), m_posY.GetValue() );
    m_footprint->SetPosition( pos );
    m_footprint->SetLocked( m_cbLocked->GetValue() );

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

    m_footprint->SetAllowSolderMaskBridges( m_allowSolderMaskBridges->GetValue() );

    EDA_ANGLE orient = m_orientation.GetAngleValue().Normalize();

    if( m_footprint->GetOrientation() != orient )
        m_footprint->Rotate( m_footprint->GetPosition(), orient - m_footprint->GetOrientation() );

    // Set component side, that also have effect on the fields positions on board
    bool change_layer = false;

    if( m_BoardSideCtrl->GetSelection() == 0 )     // layer req = COMPONENT
    {
        if( m_footprint->GetLayer() == B_Cu )
            change_layer = true;
    }
    else if( m_footprint->GetLayer() == F_Cu )
    {
        change_layer = true;
    }

    if( change_layer )
        m_footprint->Flip( m_footprint->GetPosition(), m_frame->GetPcbNewSettings()->m_FlipDirection );

    // Copy the models from the panel to the footprint
    std::vector<FP_3DMODEL>& panelList = m_3dPanel->GetModelList();
    std::vector<FP_3DMODEL>* fpList    = &m_footprint->Models();
    fpList->clear();
    fpList->insert( fpList->end(), panelList.begin(), panelList.end() );

    // This is a simple edit, we must create an undo entry
    if( m_footprint->GetEditFlags() == 0 )    // i.e. not edited, or moved
        commit.Push( _( "Edit Footprint Properties" ) );

    m_returnValue = FP_PROPS_OK;
    return true;
}


void DIALOG_FOOTPRINT_PROPERTIES::OnAddField( wxCommandEvent&  )
{
    m_itemsGrid->OnAddRow(
            [&]() -> std::pair<int, int>
            {
                PCB_FIELD newField( m_footprint, FIELD_T::USER, GetUserFieldName( m_fields->size(), DO_TRANSLATE ) );

                newField.SetVisible( false );
                newField.SetLayer( m_footprint->GetLayer() == F_Cu ? F_Fab : B_Fab );
                newField.SetFPRelativePosition( { 0, 0 } );
                newField.StyleFromSettings( m_frame->GetDesignSettings(), true );

                m_fields->push_back( newField );

                // notify the grid
                wxGridTableMessage msg( m_fields, wxGRIDTABLE_NOTIFY_ROWS_APPENDED, 1 );
                m_itemsGrid->ProcessTableMessage( msg );
                OnModify();

                return { m_fields->size() - 1, PFC_NAME };
            } );
}


void DIALOG_FOOTPRINT_PROPERTIES::OnDeleteField( wxCommandEvent&  )
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


void DIALOG_FOOTPRINT_PROPERTIES::OnUpdateUI( wxUpdateUIEvent&  )
{
    if( !m_initialized )
        return;

    // Handle a grid error.  This is delayed to OnUpdateUI so that we can change focus
    // even when the original validation was triggered from a killFocus event, and so
    // that the corresponding notebook page can be shown in the background when triggered
    // from an OK.
    if( m_delayedFocusRow >= 0 )
    {
        // We will re-enter this routine if an error dialog is displayed, so make sure we
        // zero out our member variables first.
        wxGrid*  grid = m_delayedFocusGrid;
        int      row = m_delayedFocusRow;
        int      col = m_delayedFocusColumn;
        wxString msg = m_delayedErrorMessage;

        m_delayedFocusGrid = nullptr;
        m_delayedFocusRow = -1;
        m_delayedFocusColumn = -1;
        m_delayedErrorMessage = wxEmptyString;

        if( !msg.IsEmpty() )
        {
            // Do not use DisplayErrorMessage(); it screws up window order on Mac
            DisplayError( nullptr, msg );
        }

        grid->SetFocus();
        grid->MakeCellVisible( row, col );

        // Selecting the first grid item only makes sense for the
        // items grid
        if( !m_initialFocus || grid == m_itemsGrid )
        {
            grid->SetGridCursor( row, col );

            if( !( col == 0 && row < m_fields->GetMandatoryRowCount() ) )
                grid->EnableCellEditControl( true );

            grid->ShowCellEditControl();

            if( grid == m_itemsGrid && row == 0 && col == 0 )
            {
                wxGridCellEditor* referenceEditor = grid->GetCellEditor( 0, 0 );

                if( wxTextEntry* textEntry = dynamic_cast<wxTextEntry*>( referenceEditor->GetControl() ) )
                    KIUI::SelectReferenceNumber( textEntry );

                referenceEditor->DecRef();
            }
        }

        m_initialFocus = false;
    }
}


void DIALOG_FOOTPRINT_PROPERTIES::OnPageChanging( wxNotebookEvent& aEvent )
{
    if( !m_itemsGrid->CommitPendingChanges() )
        aEvent.Veto();
}


void DIALOG_FOOTPRINT_PROPERTIES::OnCheckBox( wxCommandEvent& event )
{
    if( m_initialized )
        OnModify();
}


void DIALOG_FOOTPRINT_PROPERTIES::OnCombobox( wxCommandEvent& event )
{
    if( m_initialized )
        OnModify();
}


void DIALOG_FOOTPRINT_PROPERTIES::OnText( wxCommandEvent& event )
{
    if( m_initialized )
        OnModify();
}


void DIALOG_FOOTPRINT_PROPERTIES::OnChoice( wxCommandEvent& event )
{
    if( m_initialized )
        OnModify();
}
