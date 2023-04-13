/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2016 Mario Luzeiro <mrluzeiro@ua.pt>
 * Copyright (C) 2018 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2015 Dick Hollenbeck, dick@softplc.com
 * Copyright (C) 2004-2023 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <footprint.h>
#include <confirm.h>
#include <dialogs/dialog_text_entry.h>
#include <filename_resolver.h>
#include <pcb_edit_frame.h>
#include <pcbnew_settings.h>
#include <pgm_base.h>
#include <pcbnew.h>
#include <kiplatform/ui.h>
#include <widgets/grid_text_button_helpers.h>
#include <widgets/text_ctrl_eval.h>
#include <widgets/std_bitmap_button.h>
#include <settings/settings_manager.h>
#include <panel_fp_properties_3d_model.h>
#include <dialogs/3d_cache_dialogs.h>
#include <dialogs/panel_preview_3d_model.h>
#include <dialog_footprint_properties.h>


int DIALOG_FOOTPRINT_PROPERTIES::m_page = 0;     // remember the last open page during session


DIALOG_FOOTPRINT_PROPERTIES::DIALOG_FOOTPRINT_PROPERTIES( PCB_EDIT_FRAME* aParent,
                                                          FOOTPRINT* aFootprint ) :
        DIALOG_FOOTPRINT_PROPERTIES_BASE( aParent ),
        m_frame( aParent ),
        m_footprint( aFootprint ),
        m_posX( aParent, m_XPosLabel, m_ModPositionX, m_XPosUnit ),
        m_posY( aParent, m_YPosLabel, m_ModPositionY, m_YPosUnit ),
        m_orientation( aParent, m_orientationLabel, m_orientationCtrl, nullptr ),
        m_netClearance( aParent, m_NetClearanceLabel, m_NetClearanceCtrl, m_NetClearanceUnits ),
        m_solderMask( aParent, m_SolderMaskMarginLabel, m_SolderMaskMarginCtrl,
                      m_SolderMaskMarginUnits ),
        m_solderPaste( aParent, m_SolderPasteMarginLabel, m_SolderPasteMarginCtrl,
                       m_SolderPasteMarginUnits ),
        m_solderPasteRatio( aParent, m_PasteMarginRatioLabel, m_PasteMarginRatioCtrl,
                            m_PasteMarginRatioUnits ),
        m_returnValue( FP_PROPS_CANCEL ),
        m_initialized( false ),
        m_gridSize( 0, 0 ),
        m_lastRequestedSize( 0, 0 )
{
    // Create the 3D models page
    m_3dPanel = new PANEL_FP_PROPERTIES_3D_MODEL( m_frame, m_footprint, this, m_NoteBook );
    m_NoteBook->AddPage( m_3dPanel, _("3D Models"), false );

    // Configure display origin transforms
    m_posX.SetCoordType( ORIGIN_TRANSFORMS::ABS_X_COORD );
    m_posY.SetCoordType( ORIGIN_TRANSFORMS::ABS_Y_COORD );

    m_texts = new FP_TEXT_GRID_TABLE( m_frame );

    m_delayedErrorMessage = wxEmptyString;
    m_delayedFocusGrid = nullptr;
    m_delayedFocusRow = -1;
    m_delayedFocusColumn = -1;
    m_initialFocus = false;

    // Give an icon
    wxIcon  icon;
    icon.CopyFromBitmap( KiBitmap( BITMAPS::icon_modedit ) );
    SetIcon( icon );

    // Give a bit more room for combobox editors
    m_itemsGrid->SetDefaultRowSize( m_itemsGrid->GetDefaultRowSize() + 4 );

    m_itemsGrid->SetTable( m_texts );
    m_itemsGrid->PushEventHandler( new GRID_TRICKS( m_itemsGrid ) );

    // Show/hide text item columns according to the user's preference
    if( PCBNEW_SETTINGS* cfg = m_frame->GetPcbNewSettings() )
        m_itemsGrid->ShowHideColumns( cfg->m_FootprintTextShownColumns );

    m_orientation.SetUnits( EDA_UNITS::DEGREES );
    m_orientation.SetPrecision( 3 );

    // Set predefined rotations in combo dropdown, according to the locale floating point
    // separator notation
    double rot_list[] = { 0.0, 90.0, -90.0, 180.0 };

    for( size_t ii = 0; ii < m_orientationCtrl->GetCount() && ii < 4; ++ii )
        m_orientationCtrl->SetString( ii, wxString::Format( "%.1f", rot_list[ii] ) );

    // Set font size for items showing long strings:
    wxFont infoFont = KIUI::GetInfoFont( this );
    m_libraryIDLabel->SetFont( infoFont );
    m_tcLibraryID->SetFont( infoFont );

    infoFont.SetStyle( wxFONTSTYLE_ITALIC );
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

    m_solderPaste.SetNegativeZero();

    m_solderPasteRatio.SetUnits( EDA_UNITS::PERCENT );
    m_solderPasteRatio.SetNegativeZero();

    // Configure button logos
    m_bpAdd->SetBitmap( KiBitmap( BITMAPS::small_plus ) );
    m_bpDelete->SetBitmap( KiBitmap( BITMAPS::small_trash ) );

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
        m_noCourtyards,
        m_cbDNP,
      	m_NetClearanceCtrl,
        m_SolderMaskMarginCtrl,
      	m_allowSolderMaskBridges,
        m_SolderPasteMarginCtrl,
      	m_PasteMarginRatioCtrl,
        m_ZoneConnectionChoice
    };

    SetupStandardButtons();

    finishDialogSettings();
    m_initialized = true;
}


DIALOG_FOOTPRINT_PROPERTIES::~DIALOG_FOOTPRINT_PROPERTIES()
{
    if( PCBNEW_SETTINGS* cfg = m_frame->GetPcbNewSettings() )
        cfg->m_FootprintTextShownColumns = m_itemsGrid->GetShownColumnsAsString();

    // Prevents crash bug in wxGrid's d'tor
    m_itemsGrid->DestroyTable( m_texts );

    // Delete the GRID_TRICKS.
    m_itemsGrid->PopEventHandler( true );

    // free the memory used by all models, otherwise models which were
    // browsed but not used would consume memory
    Prj().Get3DCacheManager()->FlushCache( false );

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

    // Footprint Texts
    m_texts->push_back( m_footprint->Reference() );
    m_texts->push_back( m_footprint->Value() );

    for( BOARD_ITEM* item : m_footprint->GraphicalItems() )
    {
        if( PCB_TEXT* textItem = dynamic_cast<PCB_TEXT*>( item ) )
            m_texts->push_back( *textItem );
    }

    // notify the grid
    wxGridTableMessage tmsg( m_texts, wxGRIDTABLE_NOTIFY_ROWS_APPENDED, m_texts->GetNumberRows() );
    m_itemsGrid->ProcessTableMessage( tmsg );

    // Footprint Properties

    m_posX.SetValue( m_footprint->GetPosition().x );
    m_posY.SetValue( m_footprint->GetPosition().y );

    m_BoardSideCtrl->SetSelection( (m_footprint->GetLayer() == B_Cu) ? 1 : 0 );

    EDA_ANGLE orientation = m_footprint->GetOrientation();
    m_orientation.SetAngleValue( orientation.Normalize180() );

    m_cbLocked->SetValue( m_footprint->IsLocked() );
    m_cbLocked->SetToolTip( _( "Locked footprints cannot be freely moved and oriented on the "
                               "canvas and can only be selected when the 'Locked items' checkbox "
                               "is enabled in the selection filter." ) );

    if( m_footprint->GetAttributes() & FP_THROUGH_HOLE )
        m_componentType->SetSelection( 0 );
    else if( m_footprint->GetAttributes() & FP_SMD )
        m_componentType->SetSelection( 1 );
    else
        m_componentType->SetSelection( 2 );

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
    m_allowSolderMaskBridges->SetValue( m_footprint->GetAttributes() & FP_ALLOW_SOLDERMASK_BRIDGES );

    switch( m_footprint->GetZoneConnection() )
    {
    default:
    case ZONE_CONNECTION::INHERITED: m_ZoneConnectionChoice->SetSelection( 0 ); break;
    case ZONE_CONNECTION::FULL:      m_ZoneConnectionChoice->SetSelection( 1 ); break;
    case ZONE_CONNECTION::THERMAL:   m_ZoneConnectionChoice->SetSelection( 2 ); break;
    case ZONE_CONNECTION::NONE:      m_ZoneConnectionChoice->SetSelection( 3 ); break;
    }

    // Show the footprint's FPID.
    m_tcLibraryID->SetValue( m_footprint->GetFPID().Format() );

    for( int col = 0; col < m_itemsGrid->GetNumberCols(); col++ )
    {
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

    m_itemsGrid->SetRowLabelSize( m_itemsGrid->GetVisibleWidth( -1, false, true, true ) );

    Layout();
    adjustGridColumns();

    return true;
}


bool DIALOG_FOOTPRINT_PROPERTIES::Validate()
{
    if( !m_itemsGrid->CommitPendingChanges() )
        return false;

    if( !DIALOG_SHIM::Validate() )
        return false;

    // Validate texts.
    for( size_t i = 0; i < m_texts->size(); ++i )
    {
        PCB_TEXT& text = m_texts->at( i );

        if( i >= 2 )
        {
            if( text.GetText().IsEmpty() )
            {
                if( m_NoteBook->GetSelection() != 0 )
                    m_NoteBook->SetSelection( 0 );

                m_delayedFocusGrid = m_itemsGrid;
                m_delayedErrorMessage = _( "Text items must have some content." );
                m_delayedFocusColumn = FPT_TEXT;
                m_delayedFocusRow = i;

                return false;
            }
        }

        int width = m_frame->ValueFromString( m_itemsGrid->GetCellValue( i, FPT_WIDTH ) );

        if( width < TEXTS_MIN_SIZE )
        {
            wxString min = m_frame->StringFromValue( TEXTS_MIN_SIZE, true );

            m_itemsGrid->SetCellValue( i, FPT_WIDTH, min );

            m_delayedFocusGrid = m_itemsGrid;
            m_delayedErrorMessage = wxString::Format( _( "Text width must be at least %s." ), min );
            m_delayedFocusColumn = FPT_WIDTH;
            m_delayedFocusRow = i;

            return false;
        }
        else if( width > TEXTS_MAX_SIZE )
        {
            wxString max = m_frame->StringFromValue( TEXTS_MAX_SIZE, true );

            m_itemsGrid->SetCellValue( i, FPT_WIDTH, max );

            m_delayedFocusGrid = m_itemsGrid;
            m_delayedErrorMessage = wxString::Format( _( "Text width must be at most %s." ), max );
            m_delayedFocusColumn = FPT_WIDTH;
            m_delayedFocusRow = i;

            return false;
        }

        int height = m_frame->ValueFromString( m_itemsGrid->GetCellValue( i, FPT_HEIGHT ) );

        if( height < TEXTS_MIN_SIZE )
        {
            wxString min = m_frame->StringFromValue( TEXTS_MIN_SIZE, true );

            m_itemsGrid->SetCellValue( i, FPT_HEIGHT, min );

            m_delayedFocusGrid = m_itemsGrid;
            m_delayedErrorMessage = wxString::Format( _( "Text height must be at least %s." ), min );
            m_delayedFocusColumn = FPT_HEIGHT;
            m_delayedFocusRow = i;

            return false;
        }
        else if( height > TEXTS_MAX_SIZE )
        {
            wxString max = m_frame->StringFromValue( TEXTS_MAX_SIZE, true );

            m_itemsGrid->SetCellValue( i, FPT_HEIGHT, max );

            m_delayedFocusGrid = m_itemsGrid;
            m_delayedErrorMessage = wxString::Format( _( "Text height must be at most %s." ), max );
            m_delayedFocusColumn = FPT_HEIGHT;
            m_delayedFocusRow = i;

            return false;
        }

        // Test for acceptable values for thickness and size and clamp if fails
        int maxPenWidth = Clamp_Text_PenSize( text.GetTextThickness(), text.GetTextSize() );

        if( text.GetTextThickness() > maxPenWidth )
        {
            wxString clamped = m_frame->StringFromValue( maxPenWidth, true );

            m_itemsGrid->SetCellValue( i, FPT_THICKNESS, clamped );

            m_delayedFocusGrid = m_itemsGrid;
            m_delayedErrorMessage = wxString::Format( _( "Text thickness is too large for the "
                                                         "text size.\n"
                                                         "It will be clamped at %s." ),
                                                      clamped );
            m_delayedFocusColumn = FPT_THICKNESS;
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

    // This only commits the editor, model updating is done below so it is inside
    // the commit
    if( !m_3dPanel->TransferDataFromWindow() )
        return false;

    auto view = m_frame->GetCanvas()->GetView();
    BOARD_COMMIT commit( m_frame );
    commit.Modify( m_footprint );

    // copy reference and value
    m_footprint->Reference() = m_texts->at( 0 );
    m_footprint->Value() = m_texts->at( 1 );

    size_t i = 2;

    for( BOARD_ITEM* item : m_footprint->GraphicalItems() )
    {
        if( PCB_TEXT* textItem = dynamic_cast<PCB_TEXT*>( item ) )
        {
            // copy grid table entries till we run out, then delete any remaining texts
            if( i < m_texts->size() )
                *textItem = m_texts->at( i++ );
            else
                textItem->DeleteStructure();
        }
    }

    // if there are still grid table entries, create new texts for them
    while( i < m_texts->size() )
    {
        PCB_TEXT* newText = new PCB_TEXT( m_texts->at( i++ ) );
        m_footprint->Add( newText, ADD_MODE::APPEND );
        view->Add( newText );
    }

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

    if( m_noCourtyards->GetValue() )
        attributes |= FP_ALLOW_MISSING_COURTYARD;

    if( m_allowSolderMaskBridges->GetValue() )
        attributes |= FP_ALLOW_SOLDERMASK_BRIDGES;

    m_footprint->SetAttributes( attributes );

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
        change_layer = true;

    if( change_layer )
        m_footprint->Flip( m_footprint->GetPosition(), m_frame->GetPcbNewSettings()->m_FlipLeftRight );

    // Copy the models from the panel to the footprint
    std::vector<FP_3DMODEL>& panelList = m_3dPanel->GetModelList();
    std::vector<FP_3DMODEL>* fpList    = &m_footprint->Models();
    fpList->clear();
    fpList->insert( fpList->end(), panelList.begin(), panelList.end() );

    // This is a simple edit, we must create an undo entry
    if( m_footprint->GetEditFlags() == 0 )    // i.e. not edited, or moved
        commit.Push( _( "Modify footprint properties" ) );

    m_returnValue = FP_PROPS_OK;
    return true;
}


void DIALOG_FOOTPRINT_PROPERTIES::OnAddField( wxCommandEvent&  )
{
    if( !m_itemsGrid->CommitPendingChanges() )
        return;

    const BOARD_DESIGN_SETTINGS& dsnSettings = m_frame->GetDesignSettings();
    PCB_TEXT                     textItem( m_footprint, PCB_TEXT::TEXT_is_DIVERS );

    // Set active layer if legal; otherwise copy layer from previous text item
    if( LSET::AllTechMask().test( m_frame->GetActiveLayer() ) )
        textItem.SetLayer( m_frame->GetActiveLayer() );
    else
        textItem.SetLayer( m_texts->at( m_texts->size() - 1 ).GetLayer() );

    textItem.SetTextSize( dsnSettings.GetTextSize( textItem.GetLayer() ) );
    textItem.SetTextThickness( dsnSettings.GetTextThickness( textItem.GetLayer() ) );
    textItem.SetItalic( dsnSettings.GetTextItalic( textItem.GetLayer() ) );
    textItem.SetKeepUpright( dsnSettings.GetTextUpright( textItem.GetLayer() ) );
    textItem.SetMirrored( IsBackLayer( textItem.GetLayer() ) );

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


void DIALOG_FOOTPRINT_PROPERTIES::OnDeleteField( wxCommandEvent&  )
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


void DIALOG_FOOTPRINT_PROPERTIES::adjustGridColumns()
{
    // Account for scroll bars
    int itemsWidth = KIPLATFORM::UI::GetUnobscuredSize( m_itemsGrid ).x;

    itemsWidth -= m_itemsGrid->GetRowLabelSize();

    for( int i = 1; i < m_itemsGrid->GetNumberCols(); i++ )
        itemsWidth -= m_itemsGrid->GetColSize( i );

    m_itemsGrid->SetColSize( 0, std::max( itemsWidth,
                                          m_itemsGrid->GetVisibleWidth( 0, true, false ) ) );

    // Update the width of the 3D panel
    m_3dPanel->AdjustGridColumnWidths();
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
            grid->EnableCellEditControl( true );
            grid->ShowCellEditControl();

            if( grid == m_itemsGrid && row == 0 && col == 0 )
            {
                auto referenceEditor = grid->GetCellEditor( 0, 0 );

                if( auto textEntry = dynamic_cast<wxTextEntry*>( referenceEditor->GetControl() ) )
                    KIUI::SelectReferenceNumber( textEntry );

                referenceEditor->DecRef();
            }
        }
        m_initialFocus = false;
    }
}


void DIALOG_FOOTPRINT_PROPERTIES::OnGridSize( wxSizeEvent& aEvent )
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


void DIALOG_FOOTPRINT_PROPERTIES::OnPageChange( wxNotebookEvent& aEvent )
{
    int page = aEvent.GetSelection();

    // Shouldn't be necessary, but is on at least OSX
    if( page >= 0 )
        m_NoteBook->ChangeSelection( (unsigned) page );
}


