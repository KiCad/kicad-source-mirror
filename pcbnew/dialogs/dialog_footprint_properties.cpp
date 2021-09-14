/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2016 Mario Luzeiro <mrluzeiro@ua.pt>
 * Copyright (C) 2018 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2015 Dick Hollenbeck, dick@softplc.com
 * Copyright (C) 2004-2020 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <validators.h>
#include <widgets/grid_text_button_helpers.h>
#include <widgets/text_ctrl_eval.h>
#include <widgets/wx_grid.h>
#include <settings/settings_manager.h>

#include <panel_fp_properties_3d_model.h>

#include "3d_cache/dialogs/3d_cache_dialogs.h"
#include "3d_cache/dialogs/panel_preview_3d_model.h"

#include <dialog_footprint_properties.h>


int DIALOG_FOOTPRINT_PROPERTIES::m_page = 0;     // remember the last open page during session


DIALOG_FOOTPRINT_PROPERTIES::DIALOG_FOOTPRINT_PROPERTIES( PCB_EDIT_FRAME* aParent,
                                                          FOOTPRINT* aFootprint ) :
        DIALOG_FOOTPRINT_PROPERTIES_BASE( aParent ),
        m_frame( aParent ),
        m_footprint( aFootprint ),
        m_posX( aParent, m_XPosLabel, m_ModPositionX, m_XPosUnit ),
        m_posY( aParent, m_YPosLabel, m_ModPositionY, m_YPosUnit ),
        m_orientValidator( 3, &m_orientValue ),
        m_netClearance( aParent, m_NetClearanceLabel, m_NetClearanceCtrl, m_NetClearanceUnits ),
        m_solderMask( aParent, m_SolderMaskMarginLabel, m_SolderMaskMarginCtrl,
                      m_SolderMaskMarginUnits ),
        m_solderPaste( aParent, m_SolderPasteMarginLabel, m_SolderPasteMarginCtrl,
                       m_SolderPasteMarginUnits ),
        m_solderPasteRatio( aParent, m_PasteMarginRatioLabel, m_PasteMarginRatioCtrl,
                            m_PasteMarginRatioUnits ),
        m_returnValue( FP_PROPS_CANCEL ),
        m_initialized( false )
{
    // Create the 3D models page
    m_3dPanel = new PANEL_FP_PROPERTIES_3D_MODEL( m_frame, m_footprint, this, m_NoteBook );
    m_NoteBook->AddPage( m_3dPanel, _("3D Models"), false );

    // Configure display origin transforms
    m_posX.SetCoordType( ORIGIN_TRANSFORMS::ABS_X_COORD );
    m_posY.SetCoordType( ORIGIN_TRANSFORMS::ABS_Y_COORD );

    for( size_t i = 0; i < m_NoteBook->GetPageCount(); ++i )
   	    m_macHack.push_back( true );

    m_texts = new FP_TEXT_GRID_TABLE( m_units, m_frame );

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

    PCBNEW_SETTINGS* cfg = m_frame->GetPcbNewSettings();

    // Show/hide text item columns according to the user's preference
    m_itemsGrid->ShowHideColumns( cfg->m_FootprintTextShownColumns );

    m_orientValidator.SetRange( -360.0, 360.0 );
    m_OrientValueCtrl->SetValidator( m_orientValidator );
    m_orientValidator.SetWindow( m_OrientValueCtrl );

    // Set font size for items showing long strings:
    wxFont infoFont = KIUI::GetInfoFont( this );
#if __WXMAC__
    m_allow90Label->SetFont( infoFont );
    m_allow180Label->SetFont( infoFont );
#endif
    m_libraryIDLabel->SetFont( infoFont );
    m_tcLibraryID->SetFont( infoFont );

    infoFont.SetStyle( wxFONTSTYLE_ITALIC );
    m_staticTextInfoValNeg->SetFont( infoFont );
    m_staticTextInfoValPos->SetFont( infoFont );
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

    m_sdbSizerStdButtonsOK->SetDefault();

    m_orientValue = 0;

    // Configure button logos
    m_bpAdd->SetBitmap( KiBitmap( BITMAPS::small_plus ) );
    m_bpDelete->SetBitmap( KiBitmap( BITMAPS::small_trash ) );

    finishDialogSettings();
    m_initialized = true;
}


DIALOG_FOOTPRINT_PROPERTIES::~DIALOG_FOOTPRINT_PROPERTIES()
{
    m_frame->GetPcbNewSettings()->m_FootprintTextShownColumns =
            m_itemsGrid->GetShownColumns().ToStdString();

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


void DIALOG_FOOTPRINT_PROPERTIES::FootprintOrientEvent( wxCommandEvent&  )
{
    if( m_Orient0->GetValue() )
        m_orientValue = 0.0;
    else if( m_Orient90->GetValue() )
        m_orientValue = 90.0;
    else if( m_Orient270->GetValue() )
        m_orientValue = 270.0;
    else if( m_Orient180->GetValue() )
        m_orientValue = 180.0;

    updateOrientationControl();
}


void DIALOG_FOOTPRINT_PROPERTIES::OnOtherOrientation( wxCommandEvent& aEvent )
{
    m_OrientOther->SetValue( true );

    aEvent.Skip();
}


bool allPadsLocked( FOOTPRINT* aFootprint )
{
    for( PAD* pad : aFootprint->Pads() )
    {
        if( !pad->IsLocked() )
            return false;
    }

    return true;
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
        FP_TEXT* textItem = dyn_cast<FP_TEXT*>( item );

        if( textItem )
            m_texts->push_back( *textItem );
    }

    // notify the grid
    wxGridTableMessage tmsg( m_texts, wxGRIDTABLE_NOTIFY_ROWS_APPENDED, m_texts->GetNumberRows() );
    m_itemsGrid->ProcessTableMessage( tmsg );

    // Footprint Properties

    m_posX.SetValue( m_footprint->GetPosition().x );
    m_posY.SetValue( m_footprint->GetPosition().y );

    m_BoardSideCtrl->SetSelection( (m_footprint->GetLayer() == B_Cu) ? 1 : 0 );

    m_orientValue = m_footprint->GetOrientation() / 10.0;

    if( m_orientValue == 0.0 )
        m_Orient0->SetValue( true );
    else if( m_orientValue == 90.0 || m_orientValue == -270.0 )
        m_Orient90->SetValue( true );
    else if( m_orientValue == 270.0 || m_orientValue == -90.0 )
        m_Orient270->SetValue( true );
    else if( m_orientValue == 180.0 || m_orientValue == -180.0 )
        m_Orient180->SetValue( true );
    else
        m_OrientOther->SetValue( true );

    updateOrientationControl();

    m_AutoPlaceCtrl->SetSelection( m_footprint->IsLocked() ? 1 : 0 );

    m_AutoPlaceCtrl->SetItemToolTip( 0, _( "Footprint can be freely moved and oriented on the "
                                           "canvas." ) );
    m_AutoPlaceCtrl->SetItemToolTip( 1, _( "Footprint is locked: it cannot be freely moved and "
                                           "oriented on the canvas and can only be selected when "
                                           "the 'Locked items' checkbox is enabled in the "
                                           "selection filter." ) );

    m_CostRot90Ctrl->SetValue( m_footprint->GetPlacementCost90() );
    m_CostRot180Ctrl->SetValue( m_footprint->GetPlacementCost180() );

    if( m_footprint->GetAttributes() & FP_THROUGH_HOLE )
        m_componentType->SetSelection( 0 );
    else if( m_footprint->GetAttributes() & FP_SMD )
        m_componentType->SetSelection( 1 );
    else
        m_componentType->SetSelection( 2 );

    m_boardOnly->SetValue( m_footprint->GetAttributes() & FP_BOARD_ONLY );
    m_excludeFromPosFiles->SetValue( m_footprint->GetAttributes() & FP_EXCLUDE_FROM_POS_FILES );
    m_excludeFromBOM->SetValue( m_footprint->GetAttributes() & FP_EXCLUDE_FROM_BOM );

    // Local Clearances

    m_netClearance.SetValue( m_footprint->GetLocalClearance() );
    m_solderMask.SetValue( m_footprint->GetLocalSolderMaskMargin() );
    m_solderPaste.SetValue( m_footprint->GetLocalSolderPasteMargin() );
    m_solderPasteRatio.SetDoubleValue( m_footprint->GetLocalSolderPasteMarginRatio() * 100.0 );

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
        m_itemsGrid->SetColMinimalWidth( col, m_itemsGrid->GetVisibleWidth( col, true, false,
                                                                            false ) );
        // Adjust the column size.
        int col_size = m_itemsGrid->GetVisibleWidth( col, true, true, false );

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
    adjustGridColumns( m_itemsGrid->GetRect().GetWidth() );

    return true;
}


bool DIALOG_FOOTPRINT_PROPERTIES::Validate()
{
    if( !m_itemsGrid->CommitPendingChanges() )
        return false;

    if( !DIALOG_SHIM::Validate() )
        return false;

    // Check for empty texts.
    for( size_t i = 2; i < m_texts->size(); ++i )
    {
        FP_TEXT& text = m_texts->at( i );

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
        FP_TEXT* textItem = dyn_cast<FP_TEXT*>( item );

        if( textItem )
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
        auto newText = new FP_TEXT( m_texts->at( i++ ) );
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
    wxPoint pos( m_posX.GetValue(), m_posY.GetValue() );
    m_footprint->SetPosition( pos );
    m_footprint->SetLocked( m_AutoPlaceCtrl->GetSelection() == 1 );

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

    m_footprint->SetAttributes( attributes );

    m_footprint->SetPlacementCost90( m_CostRot90Ctrl->GetValue() );
    m_footprint->SetPlacementCost180( m_CostRot180Ctrl->GetValue() );

    // Now, set orientation.  Must be done after other changes because rotation changes field
    // positions on board (so that relative positions are held constant)
    m_orientValidator.TransferFromWindow();

    double orient = m_orientValue * 10;

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
        m_footprint->Flip( m_footprint->GetPosition(), m_frame->Settings().m_FlipLeftRight );

    // Copy the models from the panel to the footprint
    std::vector<FP_3DMODEL>& panelList = m_3dPanel->GetModelList();
    std::list<FP_3DMODEL>*   fpList    = &m_footprint->Models();
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
    FP_TEXT textItem( m_footprint );

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
    m_itemsGrid->CommitPendingChanges( true /* quiet mode */ );

    int curRow   = m_itemsGrid->GetGridCursorRow();

    if( curRow < 0 )
        return;
    else if( curRow < 2 )
    {
        DisplayError( nullptr, _( "Reference and value are mandatory." ) );
        return;
    }

    m_texts->erase( m_texts->begin() + curRow );

    // notify the grid
    wxGridTableMessage msg( m_texts, wxGRIDTABLE_NOTIFY_ROWS_DELETED, curRow, 1 );
    m_itemsGrid->ProcessTableMessage( msg );

    if( m_itemsGrid->GetNumberRows() > 0 )
    {
        m_itemsGrid->MakeCellVisible( std::max( 0, curRow-1 ), m_itemsGrid->GetGridCursorCol() );
        m_itemsGrid->SetGridCursor( std::max( 0, curRow-1 ), m_itemsGrid->GetGridCursorCol() );
    }
}


void DIALOG_FOOTPRINT_PROPERTIES::adjustGridColumns( int aWidth )
{
    // Account for scroll bars
    int itemsWidth = aWidth - ( m_itemsGrid->GetSize().x - m_itemsGrid->GetClientSize().x );

    itemsWidth -= m_itemsGrid->GetRowLabelSize();

    for( int i = 1; i < m_itemsGrid->GetNumberCols(); i++ )
        itemsWidth -= m_itemsGrid->GetColSize( i );

    if( itemsWidth > 0 )
    {
        m_itemsGrid->SetColSize( 0, std::max( itemsWidth,
                m_itemsGrid->GetVisibleWidth( 0, true, false, false ) ) );
    }

    // Update the width of the 3D panel
    m_3dPanel->AdjustGridColumnWidths( aWidth );
}


void DIALOG_FOOTPRINT_PROPERTIES::OnUpdateUI( wxUpdateUIEvent&  )
{
    if( !m_initialized )
        return;

    if( !m_itemsGrid->IsCellEditControlShown() )
        adjustGridColumns( m_itemsGrid->GetRect().GetWidth() );

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
    adjustGridColumns( aEvent.GetSize().GetX() );

    aEvent.Skip();
}


void DIALOG_FOOTPRINT_PROPERTIES::OnPageChange( wxNotebookEvent& aEvent )
{
    int page = aEvent.GetSelection();

    // Shouldn't be necessary, but is on at least OSX
    if( page >= 0 )
        m_NoteBook->ChangeSelection( (unsigned) page );

#ifdef __WXMAC__
    // Work around an OSX bug where the wxGrid children don't get placed correctly until
    // the first resize event
    if( m_macHack[ page ] )
    {
        wxSize pageSize = m_NoteBook->GetPage( page )->GetSize();
        pageSize.x -= 1;

        m_NoteBook->GetPage( page )->SetSize( pageSize );
        m_macHack[ page ] = false;
    }
#endif
}


void DIALOG_FOOTPRINT_PROPERTIES::updateOrientationControl()
{
    KIUI::ValidatorTransferToWindowWithoutEvents( m_orientValidator );
}
