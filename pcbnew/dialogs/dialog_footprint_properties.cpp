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

#include <3d_viewer/eda_3d_viewer.h>
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

#include "3d_cache/dialogs/3d_cache_dialogs.h"
#include "3d_cache/dialogs/panel_preview_3d_model.h"

#include <dialog_footprint_properties.h>


int DIALOG_FOOTPRINT_PROPERTIES::m_page = 0;     // remember the last open page during session


DIALOG_FOOTPRINT_PROPERTIES::DIALOG_FOOTPRINT_PROPERTIES( PCB_EDIT_FRAME* aParent,
                                                          FOOTPRINT* aFootprint ) :
        DIALOG_FOOTPRINT_PROPERTIES_BASE( aParent ),
        m_posX( aParent, m_XPosLabel, m_ModPositionX, m_XPosUnit ),
        m_posY( aParent, m_YPosLabel, m_ModPositionY, m_YPosUnit ),
        m_orientValidator( 3, &m_orientValue ),
        m_netClearance( aParent, m_NetClearanceLabel, m_NetClearanceCtrl, m_NetClearanceUnits ),
        m_solderMask( aParent, m_SolderMaskMarginLabel, m_SolderMaskMarginCtrl, m_SolderMaskMarginUnits ),
        m_solderPaste( aParent, m_SolderPasteMarginLabel, m_SolderPasteMarginCtrl, m_SolderPasteMarginUnits ),
        m_initialFocus( true ),
        m_inSelect( false )
{
    m_frame     = aParent;
    m_footprint = aFootprint;
    m_returnValue = FP_PROPS_CANCEL;

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

    // Give an icon
    wxIcon  icon;
    icon.CopyFromBitmap( KiBitmap( BITMAPS::icon_modedit ) );
    SetIcon( icon );

    // Give a bit more room for combobox editors
    m_itemsGrid->SetDefaultRowSize( m_itemsGrid->GetDefaultRowSize() + 4 );
    m_modelsGrid->SetDefaultRowSize( m_modelsGrid->GetDefaultRowSize() + 4 );

    m_itemsGrid->SetTable( m_texts );
    m_itemsGrid->PushEventHandler( new GRID_TRICKS( m_itemsGrid ) );
    m_modelsGrid->PushEventHandler( new GRID_TRICKS( m_modelsGrid ) );

    PCBNEW_SETTINGS* cfg = m_frame->GetPcbNewSettings();

    // Show/hide text item columns according to the user's preference
    m_itemsGrid->ShowHideColumns( cfg->m_FootprintTextShownColumns );

    // Set up the 3D models grid
    // Path selector
    if( cfg->m_lastFootprint3dDir.IsEmpty() )
    {
        wxGetEnv( KICAD6_3DMODEL_DIR, &cfg->m_lastFootprint3dDir );
    }

    wxGridCellAttr* attr = new wxGridCellAttr;
    attr->SetEditor( new GRID_CELL_PATH_EDITOR( this, m_modelsGrid, &cfg->m_lastFootprint3dDir,
                                                "*.*", true, Prj().GetProjectPath() ) );
    m_modelsGrid->SetColAttr( 0, attr );

    // Show checkbox
    attr = new wxGridCellAttr;
    attr->SetRenderer( new wxGridCellBoolRenderer() );
    attr->SetReadOnly();    // not really; we delegate interactivity to GRID_TRICKS
    attr->SetAlignment( wxALIGN_CENTER, wxALIGN_CENTER );
    m_modelsGrid->SetColAttr( 1, attr );
    m_modelsGrid->SetWindowStyleFlag( m_modelsGrid->GetWindowStyle() & ~wxHSCROLL );
    m_modelsGrid->SetSelectionMode( wxGrid::wxGridSelectRows );

    m_orientValidator.SetRange( -360.0, 360.0 );
    m_OrientValueCtrl->SetValidator( m_orientValidator );
    m_orientValidator.SetWindow( m_OrientValueCtrl );

    aParent->Prj().Get3DCacheManager()->GetResolver()->SetProgramBase( &Pgm() );

    m_previewPane = new PANEL_PREVIEW_3D_MODEL( m_Panel3D, m_frame, m_footprint, &m_shapes3D_list );

    bLowerSizer3D->Add( m_previewPane, 1, wxEXPAND, 5 );

    // Set font size for items showing long strings:
    wxFont infoFont = KIUI::GetInfoFont();
#if __WXMAC__
    m_allow90Label->SetFont( infoFont );
    m_allow180Label->SetFont( infoFont );
#endif
    m_staticTextInfoCopper->SetFont( infoFont );
    m_staticTextInfoPaste->SetFont( infoFont );

    m_libraryIDLabel->SetFont( infoFont );
    m_tcLibraryID->SetFont( infoFont );

    infoFont.SetStyle( wxFONTSTYLE_ITALIC );
    m_staticTextInfoValNeg->SetFont( infoFont );
    m_staticTextInfoValPos->SetFont( infoFont );

    m_NoteBook->SetSelection( m_page );

    if( m_page == 0 )
    {
        m_delayedFocusGrid = m_itemsGrid;
        m_delayedFocusRow = 0;
        m_delayedFocusColumn = 0;
    }
    else if ( m_page == 1 )
    {
        SetInitialFocus( m_NetClearanceCtrl );
    }
    else
    {
        m_delayedFocusGrid = m_modelsGrid;
        m_delayedFocusRow = 0;
        m_delayedFocusColumn = 0;
    }

    m_sdbSizerStdButtonsOK->SetDefault();

    m_orientValue = 0;

    // Configure button logos
    m_bpAdd->SetBitmap( KiBitmap( BITMAPS::small_plus ) );
    m_bpDelete->SetBitmap( KiBitmap( BITMAPS::small_trash ) );
    m_buttonAdd->SetBitmap( KiBitmap( BITMAPS::small_plus ) );
    m_buttonBrowse->SetBitmap( KiBitmap( BITMAPS::small_folder ) );
    m_buttonRemove->SetBitmap( KiBitmap( BITMAPS::small_trash ) );

    finishDialogSettings();
}


DIALOG_FOOTPRINT_PROPERTIES::~DIALOG_FOOTPRINT_PROPERTIES()
{
    m_frame->GetPcbNewSettings()->m_FootprintTextShownColumns =
            m_itemsGrid->GetShownColumns().ToStdString();

    // Prevents crash bug in wxGrid's d'tor
    m_itemsGrid->DestroyTable( m_texts );

    // Delete the GRID_TRICKS.
    m_itemsGrid->PopEventHandler( true );
    m_modelsGrid->PopEventHandler( true );

    // free the memory used by all models, otherwise models which were
    // browsed but not used would consume memory
    Prj().Get3DCacheManager()->FlushCache( false );

    // the GL canvas has to be visible before it is destroyed
    m_page = m_NoteBook->GetSelection();
    m_NoteBook->SetSelection( 1 );

    delete m_previewPane;
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

    if( !m_Panel3D->TransferDataToWindow() )
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

    // Prefer "-0" to "0" for normally negative values
    if( m_footprint->GetLocalSolderPasteMargin() == 0 )
        m_SolderPasteMarginCtrl->SetValue( wxT( "-" ) + m_SolderPasteMarginCtrl->GetValue() );

    // Add solder paste margin ratio in percent
    // for the usual default value 0.0, display -0.0 (or -0,0 in some countries)
    wxString msg;
    msg.Printf( wxT( "%f" ), m_footprint->GetLocalSolderPasteMarginRatio() * 100.0 );

    if( m_footprint->GetLocalSolderPasteMarginRatio() == 0.0 &&
        msg[0] == '0')  // Sometimes Printf adds a sign if the value is very small (0.0)
        m_SolderPasteMarginRatioCtrl->SetValue( wxT("-") + msg );
    else
        m_SolderPasteMarginRatioCtrl->SetValue( msg );

    switch( m_footprint->GetZoneConnection() )
    {
    default:
    case ZONE_CONNECTION::INHERITED: m_ZoneConnectionChoice->SetSelection( 0 ); break;
    case ZONE_CONNECTION::FULL:      m_ZoneConnectionChoice->SetSelection( 1 ); break;
    case ZONE_CONNECTION::THERMAL:   m_ZoneConnectionChoice->SetSelection( 2 ); break;
    case ZONE_CONNECTION::NONE:      m_ZoneConnectionChoice->SetSelection( 3 ); break;
    }

    // 3D Settings
    m_shapes3D_list.clear();
    m_modelsGrid->DeleteRows( 0, m_modelsGrid->GetNumberRows() );

    wxString origPath, alias, shortPath;
    FILENAME_RESOLVER* res = Prj().Get3DCacheManager()->GetResolver();

    for( const FP_3DMODEL& model : m_footprint->Models() )
    {
        m_shapes3D_list.push_back( model );
        origPath = model.m_Filename;

        if( res && res->SplitAlias( origPath, alias, shortPath ) )
            origPath = alias + wxT( ":" ) + shortPath;

        m_modelsGrid->AppendRows( 1 );
        int row = m_modelsGrid->GetNumberRows() - 1;
        m_modelsGrid->SetCellValue( row, 0, origPath );
        m_modelsGrid->SetCellValue( row, 1, model.m_Show ? wxT( "1" ) : wxT( "0" ) );
    }

    select3DModel( 0 );   // will clamp idx within bounds
    m_previewPane->UpdateDummyFootprint();

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
    m_modelsGrid->SetColSize( 1, m_modelsGrid->GetVisibleWidth( 1, true, false, false ) );

    Layout();
    adjustGridColumns( m_itemsGrid->GetRect().GetWidth() );

    return true;
}


void DIALOG_FOOTPRINT_PROPERTIES::select3DModel( int aModelIdx )
{
    m_inSelect = true;

    aModelIdx = std::max( 0, aModelIdx );
    aModelIdx = std::min( aModelIdx, m_modelsGrid->GetNumberRows() - 1 );

    if( m_modelsGrid->GetNumberRows() )
    {
        m_modelsGrid->SelectRow( aModelIdx );
        m_modelsGrid->SetGridCursor( aModelIdx, 0 );
    }

    m_previewPane->SetSelectedModel( aModelIdx );

    m_inSelect = false;
}


void DIALOG_FOOTPRINT_PROPERTIES::On3DModelSelected( wxGridEvent& aEvent )
{
    if( !m_inSelect )
        select3DModel( aEvent.GetRow() );
}


void DIALOG_FOOTPRINT_PROPERTIES::On3DModelCellChanged( wxGridEvent& aEvent )
{
    if( aEvent.GetCol() == 0 )
    {
        bool               hasAlias = false;
        FILENAME_RESOLVER* res = Prj().Get3DCacheManager()->GetResolver();
        wxString           filename = m_modelsGrid->GetCellValue( aEvent.GetRow(), 0 );

        filename.Replace( "\n", "" );
        filename.Replace( "\r", "" );
        filename.Replace( "\t", "" );

        if( filename.empty() || !res->ValidateFileName( filename, hasAlias ) )
        {
            m_delayedErrorMessage = wxString::Format( _( "Invalid filename: %s" ), filename );
            m_delayedFocusGrid = m_modelsGrid;
            m_delayedFocusRow = aEvent.GetRow();
            m_delayedFocusColumn = aEvent.GetCol();
            aEvent.Veto();
        }

        // if the user has specified an alias in the name then prepend ':'
        if( hasAlias )
            filename.insert( 0, wxT( ":" ) );

#ifdef __WINDOWS__
        // In Kicad files, filenames and paths are stored using Unix notation
        filename.Replace( wxT( "\\" ), wxT( "/" ) );
#endif

        m_shapes3D_list[ aEvent.GetRow() ].m_Filename = filename;
        m_modelsGrid->SetCellValue( aEvent.GetRow(), 0, filename );
    }
    else if( aEvent.GetCol() == 1 )
    {
        wxString showValue = m_modelsGrid->GetCellValue( aEvent.GetRow(), 1 );

        m_shapes3D_list[ aEvent.GetRow() ].m_Show = ( showValue == wxT( "1" ) );
    }

    m_previewPane->UpdateDummyFootprint();
}


void DIALOG_FOOTPRINT_PROPERTIES::OnRemove3DModel( wxCommandEvent&  )
{
    m_modelsGrid->CommitPendingChanges( true /* quiet mode */ );

    int idx = m_modelsGrid->GetGridCursorRow();

    if( idx >= 0 && m_modelsGrid->GetNumberRows() && !m_shapes3D_list.empty() )
    {
        m_shapes3D_list.erase( m_shapes3D_list.begin() + idx );
        m_modelsGrid->DeleteRows( idx, 1 );

        select3DModel( idx );       // will clamp idx within bounds
        m_previewPane->UpdateDummyFootprint();
    }
}


void DIALOG_FOOTPRINT_PROPERTIES::OnAdd3DModel( wxCommandEvent&  )
{
    if( !m_modelsGrid->CommitPendingChanges() )
        return;

    int selected = m_modelsGrid->GetGridCursorRow();

    PROJECT&   prj = Prj();
    FP_3DMODEL model;

    wxString initialpath = prj.GetRString( PROJECT::VIEWER_3D_PATH );
    wxString sidx = prj.GetRString( PROJECT::VIEWER_3D_FILTER_INDEX );
    int filter = 0;

    // If the PROJECT::VIEWER_3D_PATH hasn't been set yet, use the KICAD6_3DMODEL_DIR environment
    // variable and fall back to the project path if necessary.
    if( initialpath.IsEmpty() )
    {
        if( !wxGetEnv( "KICAD6_3DMODEL_DIR", &initialpath ) || initialpath.IsEmpty() )
            initialpath = prj.GetProjectPath();
    }

    if( !sidx.empty() )
    {
        long tmp;
        sidx.ToLong( &tmp );

        if( tmp > 0 && tmp <= INT_MAX )
            filter = (int) tmp;
    }

    if( !S3D::Select3DModel( this, Prj().Get3DCacheManager(), initialpath, filter, &model )
        || model.m_Filename.empty() )
    {
        select3DModel( selected );
        return;
    }

    prj.SetRString( PROJECT::VIEWER_3D_PATH, initialpath );
    sidx = wxString::Format( wxT( "%i" ), filter );
    prj.SetRString( PROJECT::VIEWER_3D_FILTER_INDEX, sidx );
    FILENAME_RESOLVER* res = Prj().Get3DCacheManager()->GetResolver();
    wxString alias;
    wxString shortPath;
    wxString filename = model.m_Filename;

    if( res && res->SplitAlias( filename, alias, shortPath ) )
        filename = alias + wxT( ":" ) + shortPath;

#ifdef __WINDOWS__
    // In KiCad files, filenames and paths are stored using Unix notation
    model.m_Filename.Replace( "\\", "/" );
#endif

    model.m_Show = true;
    m_shapes3D_list.push_back( model );

    int idx = m_modelsGrid->GetNumberRows();
    m_modelsGrid->AppendRows( 1 );
    m_modelsGrid->SetCellValue( idx, 0, filename );
    m_modelsGrid->SetCellValue( idx, 1, wxT( "1" ) );

    select3DModel( idx );
    m_previewPane->UpdateDummyFootprint();
}


void DIALOG_FOOTPRINT_PROPERTIES::OnAdd3DRow( wxCommandEvent&  )
{
    if( !m_modelsGrid->CommitPendingChanges() )
        return;

    FP_3DMODEL model;

    model.m_Show = true;
    m_shapes3D_list.push_back( model );

    int row = m_modelsGrid->GetNumberRows();
    m_modelsGrid->AppendRows( 1 );
    m_modelsGrid->SetCellValue( row, 1, wxT( "1" ) );

    select3DModel( row );

    m_modelsGrid->SetFocus();
    m_modelsGrid->MakeCellVisible( row, 0 );
    m_modelsGrid->SetGridCursor( row, 0 );

    m_modelsGrid->EnableCellEditControl( true );
    m_modelsGrid->ShowCellEditControl();
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

    if( !m_modelsGrid->CommitPendingChanges() )
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

    double dtmp = 0.0;
    wxString msg = m_SolderPasteMarginRatioCtrl->GetValue();
    msg.ToDouble( &dtmp );

    // A -50% margin ratio means no paste on a pad, the ratio must be >= -50%
    if( dtmp < -50.0 )
        dtmp = -50.0;
    // A margin ratio is always <= 0
    // 0 means use full pad copper area
    if( dtmp > 0.0 )
        dtmp = 0.0;

    m_footprint->SetLocalSolderPasteMarginRatio( dtmp / 100 );

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

    std::list<FP_3DMODEL>* draw3D = &m_footprint->Models();
    draw3D->clear();
    draw3D->insert( draw3D->end(), m_shapes3D_list.begin(), m_shapes3D_list.end() );

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


void DIALOG_FOOTPRINT_PROPERTIES::Cfg3DPath( wxCommandEvent&  )
{
    if( S3D::Configure3DPaths( this, Prj().Get3DCacheManager()->GetResolver() ) )
        m_previewPane->UpdateDummyFootprint();
}


void DIALOG_FOOTPRINT_PROPERTIES::adjustGridColumns( int aWidth )
{
    // Account for scroll bars
    int itemsWidth = aWidth - ( m_itemsGrid->GetSize().x - m_itemsGrid->GetClientSize().x );
    int modelsWidth = aWidth - ( m_modelsGrid->GetSize().x - m_modelsGrid->GetClientSize().x );

    itemsWidth -= m_itemsGrid->GetRowLabelSize();

    for( int i = 1; i < m_itemsGrid->GetNumberCols(); i++ )
        itemsWidth -= m_itemsGrid->GetColSize( i );

    if( itemsWidth > 0 )
    {
        m_itemsGrid->SetColSize( 0, std::max( itemsWidth,
                m_itemsGrid->GetVisibleWidth( 0, true, false, false ) ) );
    }

    m_modelsGrid->SetColSize( 0, modelsWidth - m_modelsGrid->GetColSize( 1 ) - 6 );
}


void DIALOG_FOOTPRINT_PROPERTIES::OnUpdateUI( wxUpdateUIEvent&  )
{
    if( !m_itemsGrid->IsCellEditControlShown() && !m_modelsGrid->IsCellEditControlShown() )
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

    m_buttonRemove->Enable( m_modelsGrid->GetNumberRows() > 0 );
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
