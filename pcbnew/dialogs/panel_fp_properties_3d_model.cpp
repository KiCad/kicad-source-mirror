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

#include <panel_fp_properties_3d_model.h>

#include <3d_viewer/eda_3d_viewer_frame.h>
#include <env_vars.h>
#include <bitmaps.h>
#include <widgets/grid_icon_text_helpers.h>
#include <widgets/grid_text_button_helpers.h>
#include <widgets/wx_grid.h>
#include <widgets/std_bitmap_button.h>
#include <footprint.h>
#include <fp_lib_table.h>
#include <footprint_edit_frame.h>
#include <footprint_editor_settings.h>
#include <dialog_footprint_properties_fp_editor.h>
#include "filename_resolver.h"
#include <pgm_base.h>
#include <kiplatform/ui.h>
#include "dialogs/panel_preview_3d_model.h"
#include "dialogs/3d_cache_dialogs.h"
#include <settings/settings_manager.h>
#include <wx/defs.h>
#include <project_pcb.h>

enum MODELS_TABLE_COLUMNS
{
    COL_PROBLEM  = 0,
    COL_FILENAME = 1,
    COL_SHOWN    = 2
};

wxDEFINE_EVENT( wxCUSTOM_PANEL_SHOWN_EVENT, wxCommandEvent );

PANEL_FP_PROPERTIES_3D_MODEL::PANEL_FP_PROPERTIES_3D_MODEL(
        PCB_BASE_EDIT_FRAME* aFrame, FOOTPRINT* aFootprint, DIALOG_SHIM* aDialogParent,
        wxWindow* aParent, wxWindowID aId, const wxPoint& aPos, const wxSize& aSize, long aStyle,
        const wxString& aName ) :
    PANEL_FP_PROPERTIES_3D_MODEL_BASE( aParent, aId, aPos, aSize, aStyle, aName ),
    m_parentDialog( aDialogParent ),
    m_frame( aFrame ),
    m_footprint( aFootprint ),
    m_inSelect( false )
{
    m_modelsGrid->SetDefaultRowSize( m_modelsGrid->GetDefaultRowSize() + 4 );

    GRID_TRICKS* trick = new GRID_TRICKS( m_modelsGrid, [this]( wxCommandEvent& aEvent )
                                                        {
                                                            OnAdd3DRow( aEvent );
                                                        } );
    trick->SetTooltipEnable( COL_PROBLEM );

    m_modelsGrid->PushEventHandler( trick );

    // Get the last 3D directory
    PCBNEW_SETTINGS* cfg = Pgm().GetSettingsManager().GetAppSettings<PCBNEW_SETTINGS>();

    if( cfg->m_lastFootprint3dDir.IsEmpty() )
    {
        wxGetEnv( ENV_VAR::GetVersionedEnvVarName( wxS( "3DMODEL_DIR" ) ),
                  &cfg->m_lastFootprint3dDir );
    }

    // Icon showing warning/error information
    wxGridCellAttr* attr = new wxGridCellAttr;
    attr->SetReadOnly();
    m_modelsGrid->SetColAttr( COL_PROBLEM, attr );

    // Filename
    attr = new wxGridCellAttr;
    attr->SetEditor( new GRID_CELL_PATH_EDITOR( m_parentDialog, m_modelsGrid, &cfg->m_lastFootprint3dDir,
                                                wxT( "*.*" ), true, m_frame->Prj().GetProjectPath() ) );
    m_modelsGrid->SetColAttr( COL_FILENAME, attr );

    // Show checkbox
    attr = new wxGridCellAttr;
    attr->SetRenderer( new wxGridCellBoolRenderer() );
    attr->SetReadOnly();    // not really; we delegate interactivity to GRID_TRICKS
    attr->SetAlignment( wxALIGN_CENTER, wxALIGN_CENTER );
    m_modelsGrid->SetColAttr( COL_SHOWN, attr );
    m_modelsGrid->SetWindowStyleFlag( m_modelsGrid->GetWindowStyle() & ~wxHSCROLL );

    PROJECT_PCB::Get3DCacheManager( &m_frame->Prj() )->GetResolver()->SetProgramBase( &Pgm() );

    m_previewPane = new PANEL_PREVIEW_3D_MODEL( this, m_frame, m_footprint, &m_shapes3D_list );

    m_LowerSizer3D->Add( m_previewPane, 1, wxEXPAND, 5 );

    // Configure button logos
    m_button3DShapeAdd->SetBitmap( KiBitmapBundle( BITMAPS::small_plus ) );
    m_button3DShapeBrowse->SetBitmap( KiBitmapBundle( BITMAPS::small_folder ) );
    m_button3DShapeRemove->SetBitmap( KiBitmapBundle( BITMAPS::small_trash ) );

    Bind( wxEVT_SHOW, &PANEL_FP_PROPERTIES_3D_MODEL::onShowEvent, this );
    m_parentDialog->Bind( wxEVT_ACTIVATE, &PANEL_FP_PROPERTIES_3D_MODEL::onDialogActivateEvent,
                          this );
}


PANEL_FP_PROPERTIES_3D_MODEL::~PANEL_FP_PROPERTIES_3D_MODEL()
{
    // Delete the GRID_TRICKS.
    m_modelsGrid->PopEventHandler( true );

    // Unbind OnShowEvent to prevent unnecessary event handling.
    Unbind( wxEVT_SHOW, &PANEL_FP_PROPERTIES_3D_MODEL::onShowEvent, this );

    // free the memory used by all models, otherwise models which were
    // browsed but not used would consume memory
    PROJECT_PCB::Get3DCacheManager( &m_frame->Prj() )->FlushCache( false );

    delete m_previewPane;
}


bool PANEL_FP_PROPERTIES_3D_MODEL::TransferDataToWindow()
{
    ReloadModelsFromFootprint();
    return true;
}


bool PANEL_FP_PROPERTIES_3D_MODEL::TransferDataFromWindow()
{
    // Only commit changes in the editor, not the models
    // The container dialog is responsible for moving the new models into
    // the footprint inside a commit.
    if( !m_modelsGrid->CommitPendingChanges() )
        return false;

    return true;
}


void PANEL_FP_PROPERTIES_3D_MODEL::ReloadModelsFromFootprint()
{
    wxString default_path;
    wxGetEnv( ENV_VAR::GetVersionedEnvVarName( wxS( "3DMODEL_DIR" ) ), &default_path );

#ifdef __WINDOWS__
    default_path.Replace( wxT( "/" ), wxT( "\\" ) );
#endif

    m_shapes3D_list.clear();
    m_modelsGrid->ClearRows();

    wxString origPath, alias, shortPath;
    FILENAME_RESOLVER* res = PROJECT_PCB::Get3DCacheManager( &m_frame->Prj() )->GetResolver();

    for( const FP_3DMODEL& model : m_footprint->Models() )
    {
        m_shapes3D_list.push_back( model );
        origPath = model.m_Filename;

        if( res && res->SplitAlias( origPath, alias, shortPath ) )
            origPath = alias + wxT( ":" ) + shortPath;

        m_modelsGrid->AppendRows( 1 );
        int row = m_modelsGrid->GetNumberRows() - 1;
        m_modelsGrid->SetCellValue( row, COL_FILENAME, origPath );
        m_modelsGrid->SetCellValue( row, COL_SHOWN, model.m_Show ? wxT( "1" ) : wxT( "0" ) );

        // Must be after the filename is set
        updateValidateStatus( row );
    }

    select3DModel( 0 );

    m_previewPane->UpdateDummyFootprint();
    m_modelsGrid->SetColSize( COL_SHOWN, m_modelsGrid->GetVisibleWidth( COL_SHOWN, true, false ) );

    Layout();
}


void PANEL_FP_PROPERTIES_3D_MODEL::select3DModel( int aModelIdx )
{
    m_inSelect = true;

    aModelIdx = std::max( 0, aModelIdx );
    aModelIdx = std::min( aModelIdx, m_modelsGrid->GetNumberRows() - 1 );

    if( m_modelsGrid->GetNumberRows() )
    {
        m_modelsGrid->SelectRow( aModelIdx );
        m_modelsGrid->SetGridCursor( aModelIdx, COL_FILENAME );
    }

    m_previewPane->SetSelectedModel( aModelIdx );

    m_inSelect = false;
}


void PANEL_FP_PROPERTIES_3D_MODEL::On3DModelSelected( wxGridEvent& aEvent )
{
    if( !m_inSelect )
        select3DModel( aEvent.GetRow() );
}


void PANEL_FP_PROPERTIES_3D_MODEL::On3DModelCellChanged( wxGridEvent& aEvent )
{
    if( aEvent.GetCol() == COL_FILENAME )
    {
        bool               hasAlias = false;
        FILENAME_RESOLVER* res = PROJECT_PCB::Get3DCacheManager( &m_frame->Prj() )->GetResolver();
        wxString           filename = m_modelsGrid->GetCellValue( aEvent.GetRow(), COL_FILENAME );

        // Perform cleanup and validation on the filename if it isn't empty
        if( !filename.empty() )
        {
            filename.Replace( wxT( "\n" ), wxT( "" ) );
            filename.Replace( wxT( "\r" ), wxT( "" ) );
            filename.Replace( wxT( "\t" ), wxT( "" ) );

            res->ValidateFileName( filename, hasAlias );

            // If the user has specified an alias in the name then prepend ':'
            if( hasAlias )
                filename.insert( 0, wxT( ":" ) );

#ifdef __WINDOWS__
            // In KiCad files, filenames and paths are stored using Unix notation
            filename.Replace( wxT( "\\" ), wxT( "/" ) );
#endif

            // Update the grid with the modified filename
            m_modelsGrid->SetCellValue( aEvent.GetRow(), COL_FILENAME, filename );
        }

        // Save the filename in the 3D shapes table
        m_shapes3D_list[ aEvent.GetRow() ].m_Filename = filename;

        // Update the validation status
        updateValidateStatus( aEvent.GetRow() );
    }
    else if( aEvent.GetCol() == COL_SHOWN )
    {
        wxString showValue = m_modelsGrid->GetCellValue( aEvent.GetRow(), COL_SHOWN );

        m_shapes3D_list[ aEvent.GetRow() ].m_Show = ( showValue == wxT( "1" ) );
    }

    m_previewPane->UpdateDummyFootprint();
}


void PANEL_FP_PROPERTIES_3D_MODEL::OnRemove3DModel( wxCommandEvent&  )
{
    if( !m_modelsGrid->CommitPendingChanges() )
        return;

    int idx = m_modelsGrid->GetGridCursorRow();

    if( idx >= 0 && m_modelsGrid->GetNumberRows() && !m_shapes3D_list.empty() )
    {
        // Don't allow selection until we call select3DModel(), below.  Otherwise wxWidgets
        // has a tendency to get its knickers in a knot....
        m_inSelect = true;

        m_shapes3D_list.erase( m_shapes3D_list.begin() + idx );
        m_modelsGrid->DeleteRows( idx );

        select3DModel( idx );       // will clamp idx within bounds
        m_previewPane->UpdateDummyFootprint();
    }
}


void PANEL_FP_PROPERTIES_3D_MODEL::OnAdd3DModel( wxCommandEvent&  )
{
    if( !m_modelsGrid->CommitPendingChanges() )
        return;

    int selected = m_modelsGrid->GetGridCursorRow();

    PROJECT&   prj = m_frame->Prj();
    FP_3DMODEL model;

    wxString initialpath = prj.GetRString( PROJECT::VIEWER_3D_PATH );
    wxString sidx = prj.GetRString( PROJECT::VIEWER_3D_FILTER_INDEX );
    int      filter = 0;

    // If the PROJECT::VIEWER_3D_PATH hasn't been set yet, use the KICAD7_3DMODEL_DIR environment
    // variable and fall back to the project path if necessary.
    if( initialpath.IsEmpty() )
    {
        if( !wxGetEnv( ENV_VAR::GetVersionedEnvVarName( wxS( "3DMODEL_DIR" ) ), &initialpath )
            || initialpath.IsEmpty() )
        {
            initialpath = prj.GetProjectPath();
        }
    }

    if( !sidx.empty() )
    {
        long tmp;
        sidx.ToLong( &tmp );

        if( tmp > 0 && tmp <= INT_MAX )
            filter = (int) tmp;
    }

    if( !S3D::Select3DModel( m_parentDialog, PROJECT_PCB::Get3DCacheManager( &m_frame->Prj() ), initialpath, filter, &model )
        || model.m_Filename.empty() )
    {
        if( selected >= 0 )
        {
            select3DModel( selected );
            updateValidateStatus( selected );
        }

        return;
    }

    prj.SetRString( PROJECT::VIEWER_3D_PATH, initialpath );
    sidx = wxString::Format( wxT( "%i" ), filter );
    prj.SetRString( PROJECT::VIEWER_3D_FILTER_INDEX, sidx );
    FILENAME_RESOLVER* res = PROJECT_PCB::Get3DCacheManager( &m_frame->Prj() )->GetResolver();
    wxString alias;
    wxString shortPath;
    wxString filename = model.m_Filename;

    if( res && res->SplitAlias( filename, alias, shortPath ) )
        filename = alias + wxT( ":" ) + shortPath;

#ifdef __WINDOWS__
    // In KiCad files, filenames and paths are stored using Unix notation
    model.m_Filename.Replace( wxT( "\\" ), wxT( "/" ) );
#endif

    model.m_Show = true;
    m_shapes3D_list.push_back( model );

    int idx = m_modelsGrid->GetNumberRows();
    m_modelsGrid->AppendRows( 1 );
    m_modelsGrid->SetCellValue( idx, COL_FILENAME, filename );
    m_modelsGrid->SetCellValue( idx, COL_SHOWN, wxT( "1" ) );

    select3DModel( idx );
    updateValidateStatus( idx );

    m_previewPane->UpdateDummyFootprint();
}


void PANEL_FP_PROPERTIES_3D_MODEL::OnAdd3DRow( wxCommandEvent&  )
{
    if( !m_modelsGrid->CommitPendingChanges() )
        return;

    FP_3DMODEL model;

    model.m_Show = true;
    m_shapes3D_list.push_back( model );

    int row = m_modelsGrid->GetNumberRows();
    m_modelsGrid->AppendRows( 1 );
    m_modelsGrid->SetCellValue( row, COL_SHOWN, wxT( "1" ) );
    m_modelsGrid->SetCellValue( row, COL_PROBLEM, "" );

    select3DModel( row );

    m_modelsGrid->SetFocus();
    m_modelsGrid->MakeCellVisible( row, COL_FILENAME );
    m_modelsGrid->SetGridCursor( row, COL_FILENAME );

    m_modelsGrid->EnableCellEditControl( true );
    m_modelsGrid->ShowCellEditControl();

    updateValidateStatus( row );
}


void PANEL_FP_PROPERTIES_3D_MODEL::updateValidateStatus( int aRow )
{
    int icon = 0;
    wxString errStr;

    switch( validateModelExists( m_modelsGrid->GetCellValue( aRow, COL_FILENAME) ) )
    {
        case MODEL_VALIDATE_ERRORS::MODEL_NO_ERROR:
            icon   = 0;
            errStr = "";
            break;

        case MODEL_VALIDATE_ERRORS::NO_FILENAME:
            icon   = wxICON_WARNING;
            errStr = _( "No filename entered" );
            break;

        case MODEL_VALIDATE_ERRORS::ILLEGAL_FILENAME:
            icon   = wxICON_ERROR;
            errStr = _( "Illegal filename" );
            break;

        case MODEL_VALIDATE_ERRORS::RESOLVE_FAIL:
            icon   = wxICON_ERROR;
            errStr = _( "File not found" );
            break;

        case MODEL_VALIDATE_ERRORS::OPEN_FAIL:
            icon   = wxICON_ERROR;
            errStr = _( "Unable to open file" );
            break;

        default:
            icon   = wxICON_ERROR;
            errStr = _( "Unknown error" );
            break;
    }

    m_modelsGrid->SetCellValue( aRow, COL_PROBLEM, errStr );
    m_modelsGrid->SetCellRenderer( aRow, COL_PROBLEM,
                                   new GRID_CELL_STATUS_ICON_RENDERER( icon ) );
}


MODEL_VALIDATE_ERRORS PANEL_FP_PROPERTIES_3D_MODEL::validateModelExists( const wxString& aFilename )
{
    if( aFilename.empty() )
        return MODEL_VALIDATE_ERRORS::NO_FILENAME;

    bool               hasAlias = false;
    FILENAME_RESOLVER* resolv = PROJECT_PCB::Get3DFilenameResolver( &m_frame->Prj() );

    if( !resolv )
        return MODEL_VALIDATE_ERRORS::RESOLVE_FAIL;

    if( !resolv->ValidateFileName( aFilename, hasAlias ) )
        return MODEL_VALIDATE_ERRORS::ILLEGAL_FILENAME;

    wxString libraryName = m_footprint->GetFPID().GetLibNickname();
    const FP_LIB_TABLE_ROW* fpRow = nullptr;
    try
    {
        fpRow = PROJECT_PCB::PcbFootprintLibs( &m_frame->Prj() )->FindRow( libraryName, false );
    }
    catch( ... )
    {
        // if libraryName is not found in table, do nothing
    }

    wxString footprintBasePath = wxEmptyString;

    if( fpRow )
        footprintBasePath = fpRow->GetFullURI( true );

    wxString fullPath = resolv->ResolvePath( aFilename, footprintBasePath );

    if( fullPath.IsEmpty() )
        return MODEL_VALIDATE_ERRORS::RESOLVE_FAIL;

    if( !wxFileName::IsFileReadable( fullPath ) )
        return MODEL_VALIDATE_ERRORS::OPEN_FAIL;

    return MODEL_VALIDATE_ERRORS::MODEL_NO_ERROR;
}


void PANEL_FP_PROPERTIES_3D_MODEL::Cfg3DPath( wxCommandEvent& event )
{
    if( S3D::Configure3DPaths( this, PROJECT_PCB::Get3DCacheManager( &m_frame->Prj() )->GetResolver() ) )
        m_previewPane->UpdateDummyFootprint();
}


void PANEL_FP_PROPERTIES_3D_MODEL::AdjustGridColumnWidths()
{
    // Account for scroll bars
    int modelsWidth = KIPLATFORM::UI::GetUnobscuredSize( m_modelsGrid ).x;

    int width = modelsWidth - m_modelsGrid->GetColSize( COL_SHOWN )
                - m_modelsGrid->GetColSize( COL_PROBLEM );

    if( width > 0 )
        m_modelsGrid->SetColSize( COL_FILENAME, width );
}


void PANEL_FP_PROPERTIES_3D_MODEL::OnGridSize( wxSizeEvent& event )
{
    AdjustGridColumnWidths();

    event.Skip();
}


void PANEL_FP_PROPERTIES_3D_MODEL::OnUpdateUI( wxUpdateUIEvent& event )
{
    m_button3DShapeRemove->Enable( m_modelsGrid->GetNumberRows() > 0 );
}


void PANEL_FP_PROPERTIES_3D_MODEL::onModify()
{
    if( DIALOG_SHIM* dlg = dynamic_cast<DIALOG_SHIM*>( wxGetTopLevelParent( this ) ) )
        dlg->OnModify();
}


void PANEL_FP_PROPERTIES_3D_MODEL::onShowEvent( wxShowEvent& aEvent )
{
    postCustomPanelShownEventWithPredicate( static_cast<int>( aEvent.IsShown() ) );
    aEvent.Skip();
}


void PANEL_FP_PROPERTIES_3D_MODEL::onDialogActivateEvent( wxActivateEvent& aEvent )
{
    postCustomPanelShownEventWithPredicate( aEvent.GetActive()
                                            && m_previewPane->IsShownOnScreen() );
    aEvent.Skip();
}


void PANEL_FP_PROPERTIES_3D_MODEL::postCustomPanelShownEventWithPredicate( bool predicate )
{
    wxCommandEvent event( wxCUSTOM_PANEL_SHOWN_EVENT );
    event.SetEventObject( m_previewPane );
    event.SetInt( static_cast<int>( predicate ) );
    m_previewPane->ProcessWindowEvent( event );
}
