/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2018 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2015 Dick Hollenbeck, dick@softplc.com
 * Copyright (C) 2008 Wayne Stambaugh <stambaughw@gmail.com>
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

#include <confirm.h>
#include <dialogs/dialog_text_entry.h>
#include <3d_viewer/eda_3d_viewer.h>
#include <validators.h>
#include <board_design_settings.h>
#include <board_commit.h>
#include <bitmaps.h>
#include <widgets/grid_text_button_helpers.h>
#include <widgets/wx_grid.h>
#include <widgets/text_ctrl_eval.h>
#include <footprint.h>
#include <footprint_edit_frame.h>
#include <footprint_editor_settings.h>
#include <dialog_footprint_properties_fp_editor.h>
#include "filename_resolver.h"
#include <pgm_base.h>
#include "3d_cache/dialogs/panel_prev_3d.h"
#include "3d_cache/dialogs/3d_cache_dialogs.h"
#include <settings/settings_manager.h>
#include <tool/tool_manager.h>
#include <tools/pcb_selection_tool.h>

#include <fp_lib_table.h>


int DIALOG_FOOTPRINT_PROPERTIES_FP_EDITOR::m_page = 0;     // remember the last open page during session


DIALOG_FOOTPRINT_PROPERTIES_FP_EDITOR::DIALOG_FOOTPRINT_PROPERTIES_FP_EDITOR( FOOTPRINT_EDIT_FRAME* aParent,
                                                                              FOOTPRINT* aFootprint ) :
    DIALOG_FOOTPRINT_PROPERTIES_FP_EDITOR_BASE( aParent ),
    m_netClearance( aParent, m_NetClearanceLabel, m_NetClearanceCtrl, m_NetClearanceUnits ),
    m_solderMask( aParent, m_SolderMaskMarginLabel, m_SolderMaskMarginCtrl, m_SolderMaskMarginUnits ),
    m_solderPaste( aParent, m_SolderPasteMarginLabel, m_SolderPasteMarginCtrl, m_SolderPasteMarginUnits ),
    m_inSelect( false )
{
    m_frame = aParent;
    m_footprint = aFootprint;

    m_texts = new FP_TEXT_GRID_TABLE( m_units, m_frame );

    m_delayedErrorMessage = wxEmptyString;
    m_delayedFocusCtrl = nullptr;
    m_delayedFocusGrid = nullptr;
    m_delayedFocusRow = -1;
    m_delayedFocusColumn = -1;
    m_delayedFocusPage = -1;

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

    // Show/hide columns according to the user's preference
    m_itemsGrid->ShowHideColumns( m_frame->GetSettings()->m_FootprintTextShownColumns );

    PCBNEW_SETTINGS* cfg = Pgm().GetSettingsManager().GetAppSettings<PCBNEW_SETTINGS>();
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

    aParent->Prj().Get3DCacheManager()->GetResolver()->SetProgramBase( &Pgm() );

    m_PreviewPane = new PANEL_PREV_3D( m_Panel3D, m_frame, m_footprint, &m_shapes3D_list );

    bLowerSizer3D->Add( m_PreviewPane, 1, wxEXPAND, 5 );

    m_FootprintNameCtrl->SetValidator( FOOTPRINT_NAME_VALIDATOR() );

    // Set font sizes
    wxFont infoFont = wxSystemSettings::GetFont( wxSYS_DEFAULT_GUI_FONT );
    infoFont.SetSymbolicSize( wxFONTSIZE_SMALL );
#if __WXMAC__
    m_allow90Label->SetFont( infoFont );
    m_allow180Label->SetFont( infoFont );
#endif
    m_staticTextInfoCopper->SetFont( infoFont );
    m_staticTextInfoPaste->SetFont( infoFont );

    infoFont.SetStyle( wxFONTSTYLE_ITALIC );
    m_staticTextInfoValNeg->SetFont( infoFont );
    m_staticTextInfoValPos->SetFont( infoFont );

    if( m_page >= 0 )
        m_NoteBook->SetSelection( (unsigned) m_page );

    if( m_page == 0 )
    {
        m_delayedFocusGrid = m_itemsGrid;
        m_delayedFocusRow = 0;
        m_delayedFocusColumn = 0;
        m_delayedFocusPage = 0;
    }
    else if ( m_page == 1 )
        SetInitialFocus( m_NetClearanceCtrl );
    else
    {
        m_delayedFocusGrid = m_modelsGrid;
        m_delayedFocusRow = 0;
        m_delayedFocusColumn = 0;
        m_delayedFocusPage = 2;
    }

    m_sdbSizerStdButtonsOK->SetDefault();

    // Configure button logos
    m_bpAdd->SetBitmap( KiBitmap( BITMAPS::small_plus ) );
    m_bpDelete->SetBitmap( KiBitmap( BITMAPS::small_trash ) );
    m_button3DShapeAdd->SetBitmap( KiBitmap( BITMAPS::small_plus ) );
    m_button3DShapeBrowse->SetBitmap( KiBitmap( BITMAPS::small_folder ) );
    m_button3DShapeRemove->SetBitmap( KiBitmap( BITMAPS::small_trash ) );

    // wxFormBuilder doesn't include this event...
    m_itemsGrid->Connect( wxEVT_GRID_CELL_CHANGING, wxGridEventHandler( DIALOG_FOOTPRINT_PROPERTIES_FP_EDITOR::OnGridCellChanging ), NULL, this );

    finishDialogSettings();
}


DIALOG_FOOTPRINT_PROPERTIES_FP_EDITOR::~DIALOG_FOOTPRINT_PROPERTIES_FP_EDITOR()
{
    m_frame->GetSettings()->m_FootprintTextShownColumns =
            m_itemsGrid->GetShownColumns().ToStdString();

    // Prevents crash bug in wxGrid's d'tor
    m_itemsGrid->DestroyTable( m_texts );

    m_itemsGrid->Disconnect( wxEVT_GRID_CELL_CHANGING, wxGridEventHandler( DIALOG_FOOTPRINT_PROPERTIES_FP_EDITOR::OnGridCellChanging ), NULL, this );

    // Delete the GRID_TRICKS.
    m_itemsGrid->PopEventHandler( true );
    m_modelsGrid->PopEventHandler( true );

    // free the memory used by all models, otherwise models which were
    // browsed but not used would consume memory
    Prj().Get3DCacheManager()->FlushCache( false );

    // the GL canvas has to be visible before it is destroyed
    m_page = m_NoteBook->GetSelection();
    m_NoteBook->SetSelection( 1 );

    delete m_PreviewPane;
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

    if( !m_Panel3D->TransferDataToWindow() )
        return false;

    // Module Texts

    m_texts->push_back( m_footprint->Reference() );
    m_texts->push_back( m_footprint->Value() );

    for( auto item : m_footprint->GraphicalItems() )
    {
        auto textItem = dyn_cast<FP_TEXT*>( item );

        if( textItem )
            m_texts->push_back( *textItem );
    }

    // notify the grid
    wxGridTableMessage tmsg( m_texts, wxGRIDTABLE_NOTIFY_ROWS_APPENDED, m_texts->GetNumberRows() );
    m_itemsGrid->ProcessTableMessage( tmsg );

    // Footprint Properties
    m_AutoPlaceCtrl->SetSelection( m_footprint->IsLocked() ? 1 : 0 );
    m_AutoPlaceCtrl->SetItemToolTip( 0, _( "Enable hotkey move commands and Auto Placement" ) );
    m_AutoPlaceCtrl->SetItemToolTip( 1, _( "Disable hotkey move commands and Auto Placement" ) );

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
    case ZONE_CONNECTION::INHERITED:
        m_ZoneConnectionChoice->SetSelection( 0 );
        break;
    case ZONE_CONNECTION::FULL:
        m_ZoneConnectionChoice->SetSelection( 1 );
        break;
    case ZONE_CONNECTION::THERMAL:
        m_ZoneConnectionChoice->SetSelection( 2 );
        break;
    case ZONE_CONNECTION::NONE:
        m_ZoneConnectionChoice->SetSelection( 3 );
        break;
    }

    // 3D Settings

    wxString default_path;
    wxGetEnv( KICAD6_3DMODEL_DIR, &default_path );
#ifdef __WINDOWS__
    default_path.Replace( wxT( "/" ), wxT( "\\" ) );
#endif

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
    m_PreviewPane->UpdateDummyFootprint();

    for( int col = 0; col < m_itemsGrid->GetNumberCols(); col++ )
    {
        // Adjust min size to the column label size
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

    m_itemsGrid->SetRowLabelSize( m_itemsGrid->GetVisibleWidth( -1, true, true, true ) );
    m_modelsGrid->SetColSize( 1, m_modelsGrid->GetVisibleWidth( 1, true, false, false ) );

    Layout();
    adjustGridColumns( m_itemsGrid->GetRect().GetWidth() );

    return true;
}


void DIALOG_FOOTPRINT_PROPERTIES_FP_EDITOR::select3DModel( int aModelIdx )
{
    m_inSelect = true;

    aModelIdx = std::max( 0, aModelIdx );
    aModelIdx = std::min( aModelIdx, m_modelsGrid->GetNumberRows() - 1 );

    if( m_modelsGrid->GetNumberRows() )
    {
        m_modelsGrid->SelectRow( aModelIdx );
        m_modelsGrid->SetGridCursor( aModelIdx, 0 );
    }

    m_PreviewPane->SetSelectedModel( aModelIdx );

    m_inSelect = false;
}


void DIALOG_FOOTPRINT_PROPERTIES_FP_EDITOR::On3DModelSelected( wxGridEvent& aEvent )
{
    if( !m_inSelect )
        select3DModel( aEvent.GetRow() );
}


void DIALOG_FOOTPRINT_PROPERTIES_FP_EDITOR::On3DModelCellChanged( wxGridEvent& aEvent )
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
            m_delayedFocusPage = 2;
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

    m_PreviewPane->UpdateDummyFootprint();
}


void DIALOG_FOOTPRINT_PROPERTIES_FP_EDITOR::OnRemove3DModel( wxCommandEvent&  )
{
    if( !m_modelsGrid->CommitPendingChanges() )
        return;

    int idx = m_modelsGrid->GetGridCursorRow();

    if( idx >= 0 && m_modelsGrid->GetNumberRows() && !m_shapes3D_list.empty() )
    {
        m_shapes3D_list.erase( m_shapes3D_list.begin() + idx );
        m_modelsGrid->DeleteRows( idx );

        select3DModel( idx );       // will clamp idx within bounds
        m_PreviewPane->UpdateDummyFootprint();
    }
}


void DIALOG_FOOTPRINT_PROPERTIES_FP_EDITOR::OnAdd3DModel( wxCommandEvent&  )
{
    if( !m_modelsGrid->CommitPendingChanges() )
        return;

    int selected = m_modelsGrid->GetGridCursorRow();

    PROJECT&   prj = Prj();
    FP_3DMODEL model;

    wxString initialpath = prj.GetRString( PROJECT::VIEWER_3D_PATH );
    wxString sidx = prj.GetRString( PROJECT::VIEWER_3D_FILTER_INDEX );
    int      filter = 0;

    // If the PROJECT::VIEWER_3D_PATH hasn't been set yet, use the KICAD6_3DMODEL_DIR environment
    // varaible and fall back to the project path if necessary.
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
    // In Kicad files, filenames and paths are stored using Unix notation
    model.m_Filename.Replace( "\\", "/" );
#endif

    model.m_Show = true;
    m_shapes3D_list.push_back( model );

    int idx = m_modelsGrid->GetNumberRows();
    m_modelsGrid->AppendRows( 1 );
    m_modelsGrid->SetCellValue( idx, 0, filename );
    m_modelsGrid->SetCellValue( idx, 1, wxT( "1" ) );

    select3DModel( idx );
    m_PreviewPane->UpdateDummyFootprint();
}


void DIALOG_FOOTPRINT_PROPERTIES_FP_EDITOR::OnAdd3DRow( wxCommandEvent&  )
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


bool DIALOG_FOOTPRINT_PROPERTIES_FP_EDITOR::checkFootprintName( const wxString& aFootprintName )
{
    if( aFootprintName.IsEmpty() )
    {
        m_delayedErrorMessage = _( "Footprint must have a name." );
        return false;
    }
    else if( !FOOTPRINT::IsLibNameValid( aFootprintName ) )
    {
        m_delayedErrorMessage.Printf( _( "Footprint name may not contain \"%s\"." ),
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
        m_delayedFocusPage = 0;

        return false;
    }

    // Check for empty texts.
    for( size_t i = 2; i < m_texts->size(); ++i )
    {
        FP_TEXT& text = m_texts->at( i );

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

    if( !m_itemsGrid->CommitPendingChanges() )
        return false;

    if( !m_modelsGrid->CommitPendingChanges() )
        return false;

    auto view = m_frame->GetCanvas()->GetView();
    BOARD_COMMIT commit( m_frame );
    commit.Modify( m_footprint );

    LIB_ID fpID = m_footprint->GetFPID();
    fpID.SetLibItemName( m_FootprintNameCtrl->GetValue(), false );
    m_footprint->SetFPID( fpID );

    m_footprint->SetDescription( m_DocCtrl->GetValue() );
    m_footprint->SetKeywords( m_KeywordCtrl->GetValue() );

    // copy reference and value
    m_footprint->Reference() = m_texts->at( 0 );
    m_footprint->Value() = m_texts->at( 1 );

    size_t i = 2;
    std::vector<FP_TEXT*> items_to_remove;

    for( BOARD_ITEM* item : m_footprint->GraphicalItems() )
    {
        FP_TEXT* textItem = dynamic_cast<FP_TEXT*>( item );

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

    for( FP_TEXT* item: items_to_remove )
    {
        selTool->RemoveItemFromSel( item );
        view->Remove( item );
        item->DeleteStructure();
    }

    // if there are still grid table entries, create new texts for them
    while( i < m_texts->size() )
    {
        FP_TEXT* newText = new FP_TEXT( m_texts->at( i++ ) );
        m_footprint->Add( newText, ADD_MODE::APPEND );
        view->Add( newText );
    }

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

    std::list<FP_3DMODEL>* draw3D  = &m_footprint->Models();
    draw3D->clear();
    draw3D->insert( draw3D->end(), m_shapes3D_list.begin(), m_shapes3D_list.end() );

    commit.Push( _( "Modify footprint properties" ) );

    return true;
}


static bool footprintIsFromBoard( FOOTPRINT* aFootprint )
{
    return aFootprint->GetLink() != niluuid;
}


void DIALOG_FOOTPRINT_PROPERTIES_FP_EDITOR::OnGridCellChanging( wxGridEvent& event )
{
    // Currently: nothing to do
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
    FP_TEXT textItem( m_footprint );

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

    int curRow = m_itemsGrid->GetGridCursorRow();

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


void DIALOG_FOOTPRINT_PROPERTIES_FP_EDITOR::Cfg3DPath( wxCommandEvent& event )
{
    if( S3D::Configure3DPaths( this, Prj().Get3DCacheManager()->GetResolver() ) )
        m_PreviewPane->UpdateDummyFootprint();
}


void DIALOG_FOOTPRINT_PROPERTIES_FP_EDITOR::adjustGridColumns( int aWidth )
{
    // Account for scroll bars
    int itemsWidth = aWidth - ( m_itemsGrid->GetSize().x - m_itemsGrid->GetClientSize().x );
    int modelsWidth = aWidth - ( m_modelsGrid->GetSize().x - m_modelsGrid->GetClientSize().x );

    itemsWidth -= m_itemsGrid->GetRowLabelSize();

    for( int i = 1; i < m_itemsGrid->GetNumberCols(); i++ )
        itemsWidth -= m_itemsGrid->GetColSize( i );

    if( itemsWidth > 0 )
        m_itemsGrid->SetColSize( 0, std::max( itemsWidth,
                m_itemsGrid->GetVisibleWidth( 0, true, false, false ) ) );

    m_modelsGrid->SetColSize( 0, modelsWidth - m_modelsGrid->GetColSize( 1 ) - 5 );
}


void DIALOG_FOOTPRINT_PROPERTIES_FP_EDITOR::OnUpdateUI( wxUpdateUIEvent& event )
{
    if( !m_itemsGrid->IsCellEditControlShown() && !m_modelsGrid->IsCellEditControlShown() )
        adjustGridColumns( m_itemsGrid->GetRect().GetWidth() );

    if( m_itemsGrid->IsCellEditControlShown() )
    {
        // Currently: nonthing to do
    }

    // Handle a delayed focus.  The delay allows us to:
    // a) change focus when the error was triggered from within a killFocus handler
    // b) show the correct notebook page in the background before the error dialog comes up
    //    when triggered from an OK or a notebook page change

    if( m_delayedFocusPage >= 0 )
    {
        if( m_NoteBook->GetSelection() != m_delayedFocusPage )
            m_NoteBook->SetSelection( (unsigned) m_delayedFocusPage );

        m_delayedFocusPage = -1;
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

        if( auto textEntry = dynamic_cast<wxTextEntry*>( m_delayedFocusCtrl ) )
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

    m_button3DShapeRemove->Enable( m_modelsGrid->GetNumberRows() > 0 );
}


void DIALOG_FOOTPRINT_PROPERTIES_FP_EDITOR::OnGridSize( wxSizeEvent& event )
{
    adjustGridColumns( event.GetSize().GetX() );

    event.Skip();
}
