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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <panel_fp_properties_3d_model.h>

#include <algorithm>
#include <3d_viewer/eda_3d_viewer_frame.h>
#include <dialogs/dialog_configure_paths.h>
#include <env_vars.h>
#include <bitmaps.h>
#include <widgets/grid_icon_text_helpers.h>
#include <widgets/grid_text_button_helpers.h>
#include <widgets/wx_grid.h>
#include <widgets/std_bitmap_button.h>
#include <board.h>
#include <footprint.h>
#include <footprint_library_adapter.h>
#include <footprint_edit_frame.h>
#include <footprint_editor_settings.h>
#include <dialog_footprint_properties_fp_editor.h>
#include "filename_resolver.h"
#include <pgm_base.h>
#include <kiplatform/ui.h>
#include <dialogs/panel_preview_3d_model.h>
#include <dialogs/native_model_file_picker.h>
#include <dialogs/panel_embedded_files.h>
#include <settings/common_settings.h>
#include <settings/settings_manager.h>
#include <project_pcb.h>
#include <widgets/color_swatch.h>
#include <3d_rendering/3d_placeholder_utils.h>
#include <exporters/step/step_pcb_model.h>
#include <geometry/shape_poly_set.h>

#include <reporter.h>

#include <wx/defs.h>
#include <wx/filedlg.h>
#include <wx/msgdlg.h>
#include <wx/notebook.h>

enum MODELS_TABLE_COLUMNS
{
    COL_PROBLEM  = 0,
    COL_FILENAME = 1,
    COL_SHOWN    = 2
};


wxDEFINE_EVENT( wxCUSTOM_PANEL_SHOWN_EVENT, wxCommandEvent );

PANEL_FP_PROPERTIES_3D_MODEL::PANEL_FP_PROPERTIES_3D_MODEL( PCB_BASE_EDIT_FRAME* aFrame,
                                                            FOOTPRINT* aFootprint,
                                                            DIALOG_SHIM* aDialogParent,
                                                            PANEL_EMBEDDED_FILES* aFilesPanel,
                                                            wxWindow* aParent ) :
        PANEL_FP_PROPERTIES_3D_MODEL_BASE( aParent ),
        m_parentDialog( aDialogParent ),
        m_frame( aFrame ),
        m_footprint( aFootprint ),
        m_filesPanel( aFilesPanel ),
        m_inSelect( false )
{
    m_splitter1->SetSashPosition( FromDIP( m_splitter1->GetSashPosition() ) );
    m_splitter1->SetMinimumPaneSize( FromDIP( m_splitter1->GetMinimumPaneSize() ) );

    GRID_TRICKS* trick = new GRID_TRICKS( m_modelsGrid, [this]( wxCommandEvent& aEvent )
                                                        {
                                                            OnAdd3DRow( aEvent );
                                                        } );
    trick->SetTooltipEnable( COL_PROBLEM );
    m_modelsGrid->PushEventHandler( trick );
    m_modelsGrid->SetupColumnAutosizer( COL_FILENAME );

    // Get the last 3D directory
    PCBNEW_SETTINGS* cfg = GetAppSettings<PCBNEW_SETTINGS>( "pcbnew" );

    if( cfg && cfg->m_LastFootprint3dDir.IsEmpty() )
    {
        wxGetEnv( ENV_VAR::GetVersionedEnvVarName( wxS( "3DMODEL_DIR" ) ),
                  &cfg->m_LastFootprint3dDir );
    }

    // Icon showing warning/error information
    wxGridCellAttr* attr = new wxGridCellAttr;
    attr->SetReadOnly();
    m_modelsGrid->SetColAttr( COL_PROBLEM, attr );

    // Filename
    attr = new wxGridCellAttr;

    attr->SetEditor( new GRID_CELL_RUN_FUNCTION_EDITOR(
                m_parentDialog,
                [this]( int aRow, int )
                {
                    if( aRow < 0 || aRow >= static_cast<int>( m_shapes3D_list.size() ) )
                        return;

                    FP_3DMODEL model = m_shapes3D_list[aRow];

                    if( pick3DModel( m_modelsGrid->GetCellValue( aRow, COL_FILENAME ), model ) )
                    {
                        m_modelsGrid->CallAfter(
                                [this, aRow, model]
                                {
                                    m_modelsGrid->DisableCellEditControl();
                                    set3DModelRow( aRow, model );
                                } );
                    }
                } ) );

    m_modelsGrid->SetColAttr( COL_FILENAME, attr );

    // Show checkbox
    attr = new wxGridCellAttr;
    attr->SetRenderer( new wxGridCellBoolRenderer() );
    attr->SetReadOnly();    // not really; we delegate interactivity to GRID_TRICKS
    attr->SetAlignment( wxALIGN_CENTER, wxALIGN_CENTER );
    m_modelsGrid->SetColAttr( COL_SHOWN, attr );
    m_modelsGrid->SetWindowStyleFlag( m_modelsGrid->GetWindowStyle() & ~wxHSCROLL );

    PROJECT_PCB::Get3DCacheManager( &m_frame->Prj() )->GetResolver()->SetProgramBase( &Pgm() );

    m_previewPane = new PANEL_PREVIEW_3D_MODEL( m_lowerPanel, m_frame, m_footprint, &m_shapes3D_list );

    m_previewPane->SetEmbeddedFilesDelegate( m_filesPanel->GetLocalFiles() );

    m_LowerSizer3D->Add( m_previewPane, 1, wxEXPAND, 5 );

    // Configure button logos
    m_button3DShapeAdd->SetBitmap( KiBitmapBundle( BITMAPS::small_plus ) );
    m_button3DShapeBrowse->SetBitmap( KiBitmapBundle( BITMAPS::small_folder ) );
    m_button3DShapeRemove->SetBitmap( KiBitmapBundle( BITMAPS::small_trash ) );
    updateConfiguredPathChoices();
    KIPLATFORM::UI::EllipsizeChoiceBox( m_configuredPathChoice );

    m_modelsGrid->Bind( wxEVT_GRID_CELL_CHANGING, &PANEL_FP_PROPERTIES_3D_MODEL::on3DModelCellChanging, this );
    Bind( wxEVT_SHOW, &PANEL_FP_PROPERTIES_3D_MODEL::onShowEvent, this );
    Bind( wxEVT_SIZE, &PANEL_FP_PROPERTIES_3D_MODEL::onSize, this );
    aParent->Bind( wxEVT_NOTEBOOK_PAGE_CHANGED, &PANEL_FP_PROPERTIES_3D_MODEL::onNotebookPageChanged, this );
    m_parentDialog->Bind( wxEVT_ACTIVATE, &PANEL_FP_PROPERTIES_3D_MODEL::onDialogActivateEvent, this );

    // Bind extrusion control events to update the 3D preview
    m_componentHeightCtrl->Bind( wxEVT_TEXT, &PANEL_FP_PROPERTIES_3D_MODEL::onExtrusionControlChanged, this );
    m_standoffHeightCtrl->Bind( wxEVT_TEXT, &PANEL_FP_PROPERTIES_3D_MODEL::onExtrusionControlChanged, this );
    m_extrusionLayerChoice->Bind( wxEVT_CHOICE, &PANEL_FP_PROPERTIES_3D_MODEL::onExtrusionControlChanged, this );
    m_extrusionColorSwatch->Bind( COLOR_SWATCH_CHANGED, &PANEL_FP_PROPERTIES_3D_MODEL::onExtrusionColorChanged, this );
    m_extrusionMaterialChoice->Bind( wxEVT_CHOICE, &PANEL_FP_PROPERTIES_3D_MODEL::onExtrusionMaterialChanged, this );
    m_showExtrusionCheckbox->Bind( wxEVT_CHECKBOX, &PANEL_FP_PROPERTIES_3D_MODEL::onExtrusionControlChanged, this );
}


PANEL_FP_PROPERTIES_3D_MODEL::~PANEL_FP_PROPERTIES_3D_MODEL()
{
    // Delete the GRID_TRICKS.
    m_modelsGrid->PopEventHandler( true );

    m_modelsGrid->Unbind( wxEVT_GRID_CELL_CHANGING, &PANEL_FP_PROPERTIES_3D_MODEL::on3DModelCellChanging, this );
    // Unbind OnShowEvent to prevent unnecessary event handling.
    Unbind( wxEVT_SHOW, &PANEL_FP_PROPERTIES_3D_MODEL::onShowEvent, this );
    Unbind( wxEVT_SIZE, &PANEL_FP_PROPERTIES_3D_MODEL::onSize, this );
    GetParent()->Unbind( wxEVT_NOTEBOOK_PAGE_CHANGED, &PANEL_FP_PROPERTIES_3D_MODEL::onNotebookPageChanged, this );

    m_componentHeightCtrl->Unbind( wxEVT_TEXT, &PANEL_FP_PROPERTIES_3D_MODEL::onExtrusionControlChanged, this );
    m_standoffHeightCtrl->Unbind( wxEVT_TEXT, &PANEL_FP_PROPERTIES_3D_MODEL::onExtrusionControlChanged, this );
    m_extrusionLayerChoice->Unbind( wxEVT_CHOICE, &PANEL_FP_PROPERTIES_3D_MODEL::onExtrusionControlChanged, this );
    m_extrusionColorSwatch->Unbind( COLOR_SWATCH_CHANGED, &PANEL_FP_PROPERTIES_3D_MODEL::onExtrusionColorChanged,
                                    this );
    m_extrusionMaterialChoice->Unbind( wxEVT_CHOICE, &PANEL_FP_PROPERTIES_3D_MODEL::onExtrusionMaterialChanged, this );
    m_showExtrusionCheckbox->Unbind( wxEVT_CHECKBOX, &PANEL_FP_PROPERTIES_3D_MODEL::onExtrusionControlChanged, this );

    // free the memory used by all models, otherwise models which were
    // browsed but not used would consume memory
    PROJECT_PCB::Get3DCacheManager( &m_frame->Prj() )->FlushCache( false );

    delete m_previewPane;
}


bool PANEL_FP_PROPERTIES_3D_MODEL::TransferDataToWindow()
{
    ReloadModelsFromFootprint();

    bool hasExtrusion = m_footprint->HasExtrudedBody();
    m_enableExtrusionCheckbox->SetValue( hasExtrusion );

    const EXTRUDED_3D_BODY* body = m_footprint->GetExtrudedBody();
    m_showExtrusionCheckbox->SetValue( body ? body->m_show : true );
    m_componentHeightCtrl->SetValue(
            wxString::Format( wxT( "%.4f" ), body ? pcbIUScale.IUTomm( body->m_height ) : 0.0 ) );
    m_standoffHeightCtrl->SetValue(
            wxString::Format( wxT( "%.4f" ), body ? pcbIUScale.IUTomm( body->m_standoff ) : 0.0 ) );

    m_extrusionLayers = { UNDEFINED_LAYER, F_CrtYd, F_Fab, F_SilkS, UNSELECTED_LAYER };

    m_extrusionLayerChoice->Clear();
    m_extrusionLayerChoice->Append( _( "Auto" ) );
    m_extrusionLayerChoice->Append( _( "Courtyard layer" ) );
    m_extrusionLayerChoice->Append( _( "Fabrication layer" ) );
    m_extrusionLayerChoice->Append( _( "Silkscreen layer" ) );
    m_extrusionLayerChoice->Append( _( "Pin bounding box" ) );

    PCB_LAYER_ID layer = body ? body->m_layer : UNDEFINED_LAYER;
    int          selection = 0; // Auto

    for( size_t i = 0; i < m_extrusionLayers.size(); i++ )
    {
        if( m_extrusionLayers[i] == layer )
        {
            selection = static_cast<int>( i );
            break;
        }
    }

    m_extrusionLayerChoice->SetSelection( selection );

    KIGFX::COLOR4D color = body ? body->m_color : KIGFX::COLOR4D::UNSPECIFIED;
    m_userSetExtrusionColor = ( color != KIGFX::COLOR4D::UNSPECIFIED );

    if( color == KIGFX::COLOR4D::UNSPECIFIED )
        color = EXTRUDED_3D_BODY::GetDefaultColor( body ? body->m_material : EXTRUSION_MATERIAL::PLASTIC );

    m_extrusionColorSwatch->SetSwatchColor( color, false );

    m_extrusionMaterialChoice->SetSelection(
            static_cast<int>( body ? body->m_material : EXTRUSION_MATERIAL::PLASTIC ) );

    updateExtrusionControls();
    updateExtrusionPreview();

    Layout();

    if( GetSizer() )
        GetSizer()->Fit( this );

    return true;
}


bool PANEL_FP_PROPERTIES_3D_MODEL::TransferDataFromWindow()
{
    if( !m_modelsGrid->CommitPendingChanges() )
        return false;

    if( m_enableExtrusionCheckbox->GetValue() )
    {
        double compHeight;
        double standoff;

        if( !readExtrusionHeights( _( "Extruded 3D Body" ), compHeight, standoff ) )
            return false;

        if( compHeight <= 0.0 )
        {
            wxMessageBox( _( "Component height must be greater than zero." ), _( "Extruded 3D Body" ),
                          wxOK | wxICON_WARNING, this );
            return false;
        }

        if( standoff < 0.0 )
            standoff = 0.0;

        if( standoff >= compHeight )
        {
            wxMessageBox( _( "Standoff height must be less than the overall height." ), _( "Extruded 3D Body" ),
                          wxOK | wxICON_WARNING, this );
            return false;
        }

        int sel = m_extrusionLayerChoice->GetSelection();

        EXTRUDED_3D_BODY& body = m_footprint->EnsureExtrudedBody();
        body.m_height = pcbIUScale.mmToIU( compHeight );
        body.m_standoff = pcbIUScale.mmToIU( standoff );
        body.m_layer = m_extrusionLayers[sel];
        body.m_color = m_userSetExtrusionColor ? m_extrusionColorSwatch->GetSwatchColor() : KIGFX::COLOR4D::UNSPECIFIED;
        body.m_material = static_cast<EXTRUSION_MATERIAL>( m_extrusionMaterialChoice->GetSelection() );

        FOOTPRINT* dummyFp = m_previewPane->GetDummyFootprint();

        if( dummyFp && dummyFp->HasExtrudedBody() )
        {
            const EXTRUDED_3D_BODY* dummyBody = dummyFp->GetExtrudedBody();
            body.m_scale = dummyBody->m_scale;
            body.m_rotation = dummyBody->m_rotation;
            body.m_offset = dummyBody->m_offset;
        }

        body.m_show = m_showExtrusionCheckbox->GetValue();
    }
    else
    {
        m_footprint->ClearExtrudedBody();
    }

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
    m_modelsGrid->SetGridWidthsDirty();

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


void PANEL_FP_PROPERTIES_3D_MODEL::cleanupFilename( wxString* aFilename )
{
    if( !aFilename->empty() )
    {
        bool               hasAlias = false;
        FILENAME_RESOLVER* res = PROJECT_PCB::Get3DCacheManager( &m_frame->Prj() )->GetResolver();

        aFilename->Replace( wxT( "\n" ), wxT( "" ) );
        aFilename->Replace( wxT( "\r" ), wxT( "" ) );
        aFilename->Replace( wxT( "\t" ), wxT( "" ) );

        res->ValidateFileName( *aFilename, hasAlias );

        // If the user has specified an alias in the name then prepend ':'
        if( hasAlias )
            aFilename->insert( 0, wxT( ":" ) );

#ifdef __WINDOWS__
        // In KiCad files, filenames and paths are stored using Unix notation
        aFilename->Replace( wxT( "\\" ), wxT( "/" ) );
#endif
    }
}


void PANEL_FP_PROPERTIES_3D_MODEL::on3DModelCellChanging( wxGridEvent& aEvent )
{
    if( aEvent.GetCol() == COL_FILENAME )
        updateValidateStatus( aEvent.GetRow() );
}


void PANEL_FP_PROPERTIES_3D_MODEL::On3DModelCellChanged( wxGridEvent& aEvent )
{
    if( aEvent.GetCol() == COL_FILENAME )
    {
        wxString filename = m_modelsGrid->GetCellValue( aEvent.GetRow(), COL_FILENAME );

        if( !filename.empty() )
        {
            cleanupFilename( &filename );

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
    onModify();
}


void PANEL_FP_PROPERTIES_3D_MODEL::OnRemove3DModel( wxCommandEvent&  )
{
    if( !m_modelsGrid->CommitPendingChanges() )
        return;

    if( !m_modelsGrid->GetNumberRows() || m_shapes3D_list.empty() )
        return;

    wxArrayInt selectedRows = m_modelsGrid->GetSelectedRows();
    wxGridCellCoordsArray selectedCells = m_modelsGrid->GetSelectedCells();
    wxGridCellCoordsArray blockTopLeft = m_modelsGrid->GetSelectionBlockTopLeft();
    wxGridCellCoordsArray blockBottomRight = m_modelsGrid->GetSelectionBlockBottomRight();

    for( unsigned ii = 0; ii < selectedCells.GetCount(); ++ii )
        selectedRows.Add( selectedCells[ii].GetRow() );

    if( !blockTopLeft.IsEmpty() && !blockBottomRight.IsEmpty() )
    {
        for( int row = blockTopLeft[0].GetRow(); row <= blockBottomRight[0].GetRow(); ++row )
            selectedRows.Add( row );
    }

    if( selectedRows.empty() && m_modelsGrid->GetGridCursorRow() >= 0 )
        selectedRows.Add( m_modelsGrid->GetGridCursorRow() );

    if( selectedRows.empty() )
    {
        wxBell();
        return;
    }

    std::sort( selectedRows.begin(), selectedRows.end() );

    int nextSelection = selectedRows.front();
    int lastRow = -1;

    // Don't allow selection until we call select3DModel(), below.  Otherwise wxWidgets
    // has a tendency to get its knickers in a knot....
    m_inSelect = true;

    m_modelsGrid->ClearSelection();

    for( int ii = selectedRows.size() - 1; ii >= 0; --ii )
    {
        int row = selectedRows[ii];

        if( row == lastRow )
            continue;

        lastRow = row;

        if( row < 0 || row >= (int) m_shapes3D_list.size() )
            continue;

        // Not all files are embedded but this will ignore the ones that are not
        m_filesPanel->RemoveEmbeddedFile( m_shapes3D_list[row].m_Filename );
        m_shapes3D_list.erase( m_shapes3D_list.begin() + row );
        m_modelsGrid->DeleteRows( row );
    }

    if( m_modelsGrid->GetNumberRows() > 0 )
        nextSelection = std::min( nextSelection, m_modelsGrid->GetNumberRows() - 1 );
    else
        nextSelection = 0;

    select3DModel( nextSelection );       // will clamp index within bounds
    m_previewPane->UpdateDummyFootprint();
    m_inSelect = false;

    onModify();
}


bool PANEL_FP_PROPERTIES_3D_MODEL::pick3DModel( const wxString& aCurrentValue, FP_3DMODEL& aModel )
{
    PROJECT&           project = m_frame->Prj();
    S3D_CACHE*         cache = PROJECT_PCB::Get3DCacheManager( &project );
    FILENAME_RESOLVER* resolver = cache->GetResolver();
    wxString           directory;
    wxString           filename;

    const wxString embeddedPrefix = wxString::FromUTF8( FILEEXT::KiCadUriPrefix ) + wxS( "://" );

    if( !aCurrentValue.IsEmpty() && !aCurrentValue.StartsWith( embeddedPrefix ) )
    {
        wxString libraryBasePath;

        if( std::optional<LIBRARY_TABLE_ROW*> row =
                    PROJECT_PCB::FootprintLibAdapter( &project )->GetRow( m_footprint->GetFPID().GetLibNickname() ) )
        {
            libraryBasePath = LIBRARY_MANAGER::GetFullURI( *row, true );
        }

        wxString resolved = resolver->ResolvePath( aCurrentValue, libraryBasePath,
                                       { m_filesPanel->GetLocalFiles(), m_frame->GetBoard()->GetEmbeddedFiles() } );
        wxFileName currentFile( resolved );

        if( currentFile.FileExists() )
        {
            directory = currentFile.GetPath();
            filename = currentFile.GetFullName();
        }
    }

    if( directory.IsEmpty() && m_configuredPathChoice->GetSelection() > 0 )
        directory = m_configuredPathChoice->GetStringSelection();

    if( directory.IsEmpty() )
        directory = project.GetRString( PROJECT::VIEWER_3D_PATH );

    PCBNEW_SETTINGS* settings = GetAppSettings<PCBNEW_SETTINGS>( "pcbnew" );

    if( directory.IsEmpty() && settings )
        directory = settings->m_LastFootprint3dDir;

    if( directory.IsEmpty() )
        wxGetEnv( ENV_VAR::GetVersionedEnvVarName( wxS( "3DMODEL_DIR" ) ), &directory );

    if( directory.IsEmpty() )
        directory = project.GetProjectPath();

    wxString wildcard;
    wxString extensions;
    int      filterCount = 0;

    if( const std::list<wxString>* filters = cache->GetFileFilters() )
    {
        for( const wxString& filter : *filters )
        {
            ++filterCount;

            if( !wildcard.IsEmpty() )
                wildcard += '|';

            wildcard += filter;
            wxString extension = filter.AfterLast( '|' );

            if( extension != wxS( "*.*" ) )
            {
                if( !extensions.IsEmpty() )
                    extensions += ';';

                extensions += extension;
            }
        }
    }

    if( !extensions.IsEmpty() )
    {
        wxString fullFilter = wxString::Format( _( "All supported files (%s)" ), extensions );
        fullFilter << '|' << extensions << '|' << wildcard;
        wildcard = fullFilter;
        ++filterCount;
    }

    if( wildcard.IsEmpty() )
        wildcard = wxFileSelectorDefaultWildcardStr;

    COMMON_SETTINGS*                 commonSettings = Pgm().GetCommonSettings();
    NATIVE_MODEL_FILE_PICKER_OPTIONS options;
    options.parent = m_parentDialog;
    options.title = _( "Select 3D Model" );
    options.directory = directory;
    options.filename = filename;
    options.wildcard = wildcard;
    options.style = wxFD_OPEN | wxFD_FILE_MUST_EXIST;
    options.embedFile = commonSettings && commonSettings->m_EmbedFileDefaults.model_3d;

    long filterIndex = 0;
    project.GetRString( PROJECT::VIEWER_3D_FILTER_INDEX ).ToLong( &filterIndex );
    options.filterIndex = filterCount > 0
                                  ? static_cast<int>( std::clamp( filterIndex, 0L,
                                                                  static_cast<long>( filterCount - 1 ) ) )
                                  : 0;

    NATIVE_MODEL_FILE_PICKER_RESULT selected;

    struct MODAL_SCOPE
    {
        explicit MODAL_SCOPE( DIALOG_SHIM* aDialog ) : dialog( aDialog ) { dialog->PrepareForModalSubDialog(); }
        ~MODAL_SCOPE() { dialog->CleanupAfterModalSubDialog(); }
        DIALOG_SHIM* dialog;
    };

    {
        MODAL_SCOPE modalScope( m_parentDialog );
        selected = ShowNativeModelFilePicker( options );
    }

    if( !selected.accepted )
        return false;

    if( commonSettings )
        commonSettings->m_EmbedFileDefaults.model_3d = selected.embedFile;

    wxFileName selectedFile( selected.path );
    selectedFile.Normalize( FN_NORMALIZE_FLAGS | wxPATH_NORM_ENV_VARS );
    project.SetRString( PROJECT::VIEWER_3D_PATH, selectedFile.GetPath() );
    project.SetRString( PROJECT::VIEWER_3D_FILTER_INDEX, wxString::Format( wxS( "%d" ), selected.filterIndex ) );

    if( settings )
        settings->m_LastFootprint3dDir = selectedFile.GetPath();

    if( selected.embedFile )
    {
        if( EMBEDDED_FILES::EMBEDDED_FILE* embedded = m_filesPanel->AddEmbeddedFile( selected.path ) )
            aModel.m_Filename = embedded->GetLink();
        else
        {
            wxMessageBox( _( "Error adding 3D model" ), _( "Error" ), wxICON_ERROR | wxOK, this );
            return false;
        }
    }
    else
    {
        aModel.m_Filename = resolver->ShortenPath( selectedFile.GetFullPath() );
    }

    return true;
}


void PANEL_FP_PROPERTIES_3D_MODEL::set3DModelRow( int aRow, const FP_3DMODEL& aModel )
{
    wxCHECK_RET( aRow >= 0 && aRow < static_cast<int>( m_shapes3D_list.size() ),
                 wxS( "invalid 3D model row" ) );

    m_shapes3D_list[aRow] = aModel;
    wxString displayName = aModel.m_Filename;
    wxString alias;
    wxString shortPath;

    if( FILENAME_RESOLVER* resolver = PROJECT_PCB::Get3DCacheManager( &m_frame->Prj() )->GetResolver();
        resolver && resolver->SplitAlias( displayName, alias, shortPath ) )
    {
        displayName = alias + wxS( ":" ) + shortPath;
    }

    m_modelsGrid->SetCellValue( aRow, COL_FILENAME, displayName );
    m_modelsGrid->SetCellValue( aRow, COL_SHOWN, aModel.m_Show ? wxS( "1" ) : wxS( "0" ) );
    select3DModel( aRow );
    updateValidateStatus( aRow );
    m_previewPane->UpdateDummyFootprint();
    onModify();
}


void PANEL_FP_PROPERTIES_3D_MODEL::updateConfiguredPathChoices()
{
    const wxString previous = m_configuredPathChoice->GetStringSelection();
    m_configuredPathChoice->Clear();
    m_configuredPathChoice->Append( _( "Automatic" ) );

    FILENAME_RESOLVER* resolver = PROJECT_PCB::Get3DCacheManager( &m_frame->Prj() )->GetResolver();

    if( const std::list<SEARCH_PATH>* paths = resolver->GetPaths() )
    {
        for( const SEARCH_PATH& path : *paths )
        {
            if( !path.m_Pathexp.IsEmpty() && m_configuredPathChoice->FindString( path.m_Pathexp ) == wxNOT_FOUND )
                m_configuredPathChoice->Append( path.m_Pathexp );
        }
    }

    const int previousIndex = m_configuredPathChoice->FindString( previous );
    m_configuredPathChoice->SetSelection( previousIndex == wxNOT_FOUND ? 0 : previousIndex );
}


void PANEL_FP_PROPERTIES_3D_MODEL::OnAdd3DModel( wxCommandEvent&  )
{
    if( !m_modelsGrid->CommitPendingChanges() )
        return;

    const int  selected = m_modelsGrid->GetGridCursorRow();
    FP_3DMODEL model;

    if( !pick3DModel( wxEmptyString, model ) )
    {
        if( selected >= 0 )
        {
            select3DModel( selected );
            updateValidateStatus( selected );
        }

        return;
    }

    const int row = m_modelsGrid->GetNumberRows();
    model.m_Show = true;
    m_shapes3D_list.push_back( model );
    m_modelsGrid->AppendRows( 1 );
    set3DModelRow( row, model );
}


void PANEL_FP_PROPERTIES_3D_MODEL::OnAdd3DRow( wxCommandEvent&  )
{
    m_modelsGrid->OnAddRow(
            [&]() -> std::pair<int, int>
            {
                FP_3DMODEL model;

                model.m_Show = true;
                m_shapes3D_list.push_back( model );

                int row = m_modelsGrid->GetNumberRows();
                m_modelsGrid->AppendRows( 1 );
                m_modelsGrid->SetCellValue( row, COL_SHOWN, wxT( "1" ) );
                m_modelsGrid->SetCellValue( row, COL_PROBLEM, "" );

                select3DModel( row );
                updateValidateStatus( row );
                onModify();

                return { row, COL_FILENAME };
            } );
}


void PANEL_FP_PROPERTIES_3D_MODEL::updateValidateStatus( int aRow )
{
    int icon = 0;
    wxString errStr;
    wxString filename = m_modelsGrid->GetCellValue( aRow, COL_FILENAME );

    if( wxGridCellEditor* cellEditor = m_modelsGrid->GetCellEditor( aRow, COL_FILENAME ) )
    {
        if( cellEditor->IsCreated() && cellEditor->GetWindow()->IsShown() )
            filename = cellEditor->GetValue();

        cellEditor->DecRef();
    }

    switch( validateModelExists( filename ) )
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
    m_modelsGrid->SetCellRenderer( aRow, COL_PROBLEM, new GRID_CELL_STATUS_ICON_RENDERER( icon ) );
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
    wxString footprintBasePath = wxEmptyString;

    std::optional<LIBRARY_TABLE_ROW*> fpRow =
                            PROJECT_PCB::FootprintLibAdapter( &m_frame->Prj() )->GetRow( libraryName );
    if( fpRow )
        footprintBasePath = LIBRARY_MANAGER::GetFullURI( *fpRow, true );

    std::vector<const EMBEDDED_FILES*> embeddedFilesStack;
    embeddedFilesStack.push_back( m_filesPanel->GetLocalFiles() );
    embeddedFilesStack.push_back( m_frame->GetBoard()->GetEmbeddedFiles() );

    wxString fullPath = resolv->ResolvePath( aFilename, footprintBasePath, std::move( embeddedFilesStack ) );

    if( fullPath.IsEmpty() )
        return MODEL_VALIDATE_ERRORS::RESOLVE_FAIL;

    if( !wxFileName::IsFileReadable( fullPath ) )
        return MODEL_VALIDATE_ERRORS::OPEN_FAIL;

    return MODEL_VALIDATE_ERRORS::MODEL_NO_ERROR;
}


void PANEL_FP_PROPERTIES_3D_MODEL::Cfg3DPath( wxCommandEvent& event )
{
    DIALOG_CONFIGURE_PATHS dlg( this );

    if( dlg.ShowQuasiModal() == wxID_OK )
    {
        updateConfiguredPathChoices();
        m_previewPane->UpdateDummyFootprint();
    }
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


void PANEL_FP_PROPERTIES_3D_MODEL::onSize( wxSizeEvent& aEvent )
{
    aEvent.Skip();
    setDefaultSashPosition();
}


void PANEL_FP_PROPERTIES_3D_MODEL::setDefaultSashPosition()
{
    const int page = GetSize().y;

    if( m_sashPositioned || page <= 0 || !m_splitter1->IsSplit() )
        return;

    const int rowHeight = m_modelsGrid->GetDefaultRowSize();
    const int chrome = m_modelsGrid->GetColLabelSize() + 4;
    const int minGrid = 3 * rowHeight + chrome;
    const int maxGrid = std::max( minGrid, page / 3 );
    const int grid = std::clamp( m_modelsGrid->GetNumberRows() * rowHeight + chrome, minGrid, maxGrid );

    // Everything else in the upper pane is fixed height, so the sash is that plus the grid.
    m_modelsGrid->SetMinSize( wxSize( -1, minGrid ) );
    const int sash = m_upperPanel->GetSizer()->GetMinSize().y - minGrid + grid;

    // An early layout pass reports a page far shorter than the final one.  Wait for a size
    // that leaves the preview a usable pane before locking the split in.
    if( page - sash < m_splitter1->GetMinimumPaneSize() )
        return;

    m_splitter1->SetSashPosition( sash );
    m_sashPositioned = true;
}


void PANEL_FP_PROPERTIES_3D_MODEL::onDialogActivateEvent( wxActivateEvent& aEvent )
{
    postCustomPanelShownEventWithPredicate( aEvent.GetActive() && m_previewPane->IsShownOnScreen() );
    aEvent.Skip();
}


void PANEL_FP_PROPERTIES_3D_MODEL::onNotebookPageChanged( wxBookCtrlEvent& aEvent )
{
    wxNotebook* notebook = dynamic_cast<wxNotebook*>( aEvent.GetEventObject() );

    if( notebook && notebook == GetParent() )
    {
        // Native notebooks do not consistently emit show events when hiding their pages.
        const int selected = aEvent.GetSelection();
        postCustomPanelShownEventWithPredicate( selected >= 0
                && selected < (int) notebook->GetPageCount() && notebook->GetPage( selected ) == this );
    }

    aEvent.Skip();
}


void PANEL_FP_PROPERTIES_3D_MODEL::postCustomPanelShownEventWithPredicate( bool predicate )
{
    wxCommandEvent event( wxCUSTOM_PANEL_SHOWN_EVENT );
    event.SetEventObject( m_previewPane );
    event.SetInt( static_cast<int>( predicate ) );
    m_previewPane->ProcessWindowEvent( event );
}


void PANEL_FP_PROPERTIES_3D_MODEL::OnEnableExtrusion( wxCommandEvent& event )
{
    updateExtrusionControls();
    updateExtrusionPreview();
    onModify();
}


void PANEL_FP_PROPERTIES_3D_MODEL::onExtrusionControlChanged( wxCommandEvent& event )
{
    updateExtrusionPreview();
    onModify();
    event.Skip();
}


void PANEL_FP_PROPERTIES_3D_MODEL::onExtrusionColorChanged( wxCommandEvent& event )
{
    m_userSetExtrusionColor = true;
    updateExtrusionPreview();
    onModify();
    event.Skip();
}


void PANEL_FP_PROPERTIES_3D_MODEL::updateExtrusionControls()
{
    bool enabled = m_enableExtrusionCheckbox->GetValue();
    m_showExtrusionCheckbox->Enable( enabled );
    m_componentHeightCtrl->Enable( enabled );
    m_standoffHeightCtrl->Enable( enabled );
    m_extrusionLayerChoice->Enable( enabled );
    m_extrusionColorSwatch->Enable( enabled );
    m_extrusionMaterialChoice->Enable( enabled );
    m_buttonExportExtruded->Enable( enabled );

    if( enabled )
    {
        FOOTPRINT* dummyFp = m_previewPane->GetDummyFootprint();

        if( dummyFp )
            m_previewPane->SetExtrusionTransformMode( &dummyFp->EnsureExtrudedBody() );
    }
    else
    {
        m_previewPane->SetExtrusionTransformMode( nullptr );
    }
}


bool PANEL_FP_PROPERTIES_3D_MODEL::readExtrusionHeights( const wxString& aTitle, double& aHeight, double& aStandoff )
{
    wxString heightText = m_componentHeightCtrl->GetValue();
    wxString standoffText = m_standoffHeightCtrl->GetValue();

    // An empty field reads as zero.
    aHeight = 0.0;
    aStandoff = 0.0;

    if( !heightText.IsEmpty() && !heightText.ToDouble( &aHeight ) )
    {
        wxMessageBox( _( "Overall height is not a valid number." ), aTitle, wxOK | wxICON_WARNING, this );
        return false;
    }

    if( !standoffText.IsEmpty() && !standoffText.ToDouble( &aStandoff ) )
    {
        wxMessageBox( _( "Standoff height is not a valid number." ), aTitle, wxOK | wxICON_WARNING, this );
        return false;
    }

    return true;
}


void PANEL_FP_PROPERTIES_3D_MODEL::updateExtrusionPreview()
{
    FOOTPRINT* dummyFp = m_previewPane->GetDummyFootprint();

    if( !dummyFp || m_extrusionLayers.empty() )
        return;

    if( m_enableExtrusionCheckbox->GetValue() && m_showExtrusionCheckbox->GetValue() )
    {
        double compHeight = 0.0;
        double standoff = 0.0;
        m_componentHeightCtrl->GetValue().ToDouble( &compHeight );
        m_standoffHeightCtrl->GetValue().ToDouble( &standoff );

        int sel = m_extrusionLayerChoice->GetSelection();

        EXTRUDED_3D_BODY& body = dummyFp->EnsureExtrudedBody();
        body.m_height = pcbIUScale.mmToIU( compHeight );
        body.m_standoff = pcbIUScale.mmToIU( standoff );
        body.m_layer = m_extrusionLayers[sel];
        body.m_color = m_userSetExtrusionColor ? m_extrusionColorSwatch->GetSwatchColor() : KIGFX::COLOR4D::UNSPECIFIED;
        body.m_material = static_cast<EXTRUSION_MATERIAL>( m_extrusionMaterialChoice->GetSelection() );
    }
    else
    {
        dummyFp->ClearExtrudedBody();
    }

    m_previewPane->UpdateDummyFootprint( true );
}


void PANEL_FP_PROPERTIES_3D_MODEL::onExtrusionMaterialChanged( wxCommandEvent& event )
{
    if( !m_userSetExtrusionColor )
    {
        EXTRUSION_MATERIAL mat = static_cast<EXTRUSION_MATERIAL>( m_extrusionMaterialChoice->GetSelection() );
        m_extrusionColorSwatch->SetSwatchColor( EXTRUDED_3D_BODY::GetDefaultColor( mat ), false );
    }

    updateExtrusionPreview();
    onModify();
    event.Skip();
}


void PANEL_FP_PROPERTIES_3D_MODEL::OnExportExtrudedModel( wxCommandEvent& event )
{
    double height;
    double standoff;

    if( !readExtrusionHeights( _( "Export Extruded Body" ), height, standoff ) )
        return;

    if( height <= 0.0 )
    {
        wxMessageBox( _( "Component height must be greater than zero." ), _( "Export Extruded Body" ),
                      wxOK | wxICON_WARNING, this );
        return;
    }

    int layerSel = m_extrusionLayerChoice->GetSelection();

    SHAPE_POLY_SET outline;
    bool           gotOutline =
            GetExtrusionOutline( m_footprint, outline, m_extrusionLayers[layerSel] ) && outline.OutlineCount() > 0;

    if( !gotOutline )
    {
        wxMessageBox( _( "No extrusion outline could be generated for this footprint." ), _( "Export Extruded Body" ),
                      wxOK | wxICON_WARNING, this );
        return;
    }

    wxString defaultName = m_footprint->GetReference() + wxT( "_extruded" );

    wxFileDialog dlg( this, _( "Export Extruded 3D Body" ), wxEmptyString, defaultName,
                      _( "STEP files" ) + wxT( " (*.step)|*.step|" ) + _( "GLB files" ) + wxT( " (*.glb)|*.glb|" )
                              + _( "STL files" ) + wxT( " (*.stl)|*.stl|" ) + _( "BREP files" )
                              + wxT( " (*.brep)|*.brep" ),
                      wxFD_SAVE | wxFD_OVERWRITE_PROMPT );

    if( dlg.ShowModal() == wxID_CANCEL )
        return;

    wxString path = dlg.GetPath();
    int      filterIdx = dlg.GetFilterIndex();

    bool     bottom = m_footprint->IsFlipped();
    VECTOR2D origin( m_footprint->GetPosition().x, m_footprint->GetPosition().y );

    EXTRUSION_MATERIAL material = static_cast<EXTRUSION_MATERIAL>( m_extrusionMaterialChoice->GetSelection() );

    KIGFX::COLOR4D c = m_extrusionColorSwatch->GetSwatchColor();

    if( c == KIGFX::COLOR4D::UNSPECIFIED )
        c = EXTRUDED_3D_BODY::GetDefaultColor( material );

    uint32_t colorKey = EXTRUDED_3D_BODY::PackColorKey( c );

    NULL_REPORTER  reporter;
    STEP_PCB_MODEL model( wxT( "extruded_body" ), &reporter );

    if( !model.AddExtrudedBody( outline, bottom, standoff, height, origin, colorKey, material,
                                m_footprint->GetReference() ) )
    {
        wxMessageBox( _( "Failed to create extruded body geometry." ), _( "Export Extruded Body" ), wxOK | wxICON_ERROR,
                      this );
        return;
    }

    if( standoff > 0.0 )
        model.AddExtrudedPins( m_footprint, bottom, standoff, origin );

    SHAPE_POLY_SET boardOutline( outline );
    model.CreatePCB( boardOutline, origin, false );

    bool ok = false;

    switch( filterIdx )
    {
    case 0: ok = model.WriteSTEP( path, true, false ); break;
    case 1: ok = model.WriteGLTF( path ); break;
    case 2: ok = model.WriteSTL( path ); break;
    case 3: ok = model.WriteBREP( path ); break;
    }

    if( !ok )
    {
        wxMessageBox( _( "Failed to write file." ), _( "Export Extruded Body" ), wxOK | wxICON_ERROR, this );
    }
}
