/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017 Oliver Walters
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

#include <advanced_config.h>
#include <common.h>
#include <base_units.h>
#include <bitmaps.h>
#include <confirm.h>
#include <eda_doc.h>
#include <wildcards_and_files_ext.h>
#include <schematic_settings.h>
#include <general.h>
#include <grid_tricks.h>
#include <string_utils.h>
#include <kiface_base.h>
#include <sch_edit_frame.h>
#include <sch_reference_list.h>
#include <tools/sch_editor_control.h>
#include <kiplatform/ui.h>
#include <widgets/grid_text_button_helpers.h>
#include <widgets/grid_text_helpers.h>
#include <widgets/bitmap_button.h>
#include <widgets/std_bitmap_button.h>
#include <widgets/wx_grid.h>
#include <widgets/grid_checkbox.h>
#include <wx/debug.h>
#include <wx/ffile.h>
#include <wx/grid.h>
#include <wx/textdlg.h>
#include <wx/filedlg.h>
#include <wx/msgdlg.h>
#include <dialogs/eda_view_switcher.h>
#include "dialog_symbol_fields_table.h"
#include <fields_data_model.h>
#include <eda_list_dialog.h>
#include <project_sch.h>
#include <jobs/job_export_sch_bom.h>

wxDEFINE_EVENT( EDA_EVT_CLOSE_DIALOG_SYMBOL_FIELDS_TABLE, wxCommandEvent );

#ifdef __WXMAC__
#define COLUMN_MARGIN 4
#else
#define COLUMN_MARGIN 15
#endif

using SCOPE = FIELDS_EDITOR_GRID_DATA_MODEL::SCOPE;


enum
{
    MYID_SELECT_FOOTPRINT = GRIDTRICKS_FIRST_CLIENT_ID,
    MYID_SHOW_DATASHEET
};

class VIEW_CONTROLS_GRID_TRICKS : public GRID_TRICKS
{
public:
    VIEW_CONTROLS_GRID_TRICKS( WX_GRID* aGrid ) :
            GRID_TRICKS( aGrid )
    {}

protected:
    void doPopupSelection( wxCommandEvent& event ) override
    {
        if( event.GetId() >= GRIDTRICKS_FIRST_SHOWHIDE )
            m_grid->PostSizeEvent();

        GRID_TRICKS::doPopupSelection( event );
    }
};


class FIELDS_EDITOR_GRID_TRICKS : public GRID_TRICKS
{
public:
    FIELDS_EDITOR_GRID_TRICKS( DIALOG_SYMBOL_FIELDS_TABLE* aParent, WX_GRID* aGrid,
                               VIEW_CONTROLS_GRID_DATA_MODEL* aViewFieldsData,
                               FIELDS_EDITOR_GRID_DATA_MODEL* aDataModel, EMBEDDED_FILES* aFiles ) :
            GRID_TRICKS( aGrid ),
            m_dlg( aParent ),
            m_viewControlsDataModel( aViewFieldsData ),
            m_dataModel( aDataModel ),
            m_files( aFiles )
    {}

protected:
    void showPopupMenu( wxMenu& menu, wxGridEvent& aEvent ) override
    {
        int col = m_grid->GetGridCursorCol();

        if( m_dataModel->GetColFieldName( col ) == GetCanonicalFieldName( FIELD_T::FOOTPRINT ) )
        {
            menu.Append( MYID_SELECT_FOOTPRINT, _( "Select Footprint..." ), _( "Browse for footprint" ) );
            menu.AppendSeparator();
        }
        else if( m_dataModel->GetColFieldName( col ) == GetCanonicalFieldName( FIELD_T::DATASHEET ) )
        {
            menu.Append( MYID_SHOW_DATASHEET, _( "Show Datasheet" ), _( "Show datasheet in browser" ) );
            menu.AppendSeparator();
        }

        GRID_TRICKS::showPopupMenu( menu, aEvent );
    }

    void doPopupSelection( wxCommandEvent& event ) override
    {
        int row = m_grid->GetGridCursorRow();
        int col = m_grid->GetGridCursorCol();

        if( event.GetId() == MYID_SELECT_FOOTPRINT )
        {
            // pick a footprint using the footprint picker.
            wxString fpid = m_grid->GetCellValue( row, col );

            if( KIWAY_PLAYER* frame = m_dlg->Kiway().Player( FRAME_FOOTPRINT_CHOOSER, true, m_dlg ) )
            {
                if( frame->ShowModal( &fpid, m_dlg ) )
                    m_grid->SetCellValue( row, col, fpid );

                frame->Destroy();
            }
        }
        else if (event.GetId() == MYID_SHOW_DATASHEET )
        {
            wxString datasheet_uri = m_grid->GetCellValue( row, col );
            GetAssociatedDocument( m_dlg, datasheet_uri, &m_dlg->Prj(), PROJECT_SCH::SchSearchS( &m_dlg->Prj() ),
                                   { m_files } );
        }
        else if( event.GetId() >= GRIDTRICKS_FIRST_SHOWHIDE )
        {
            if( !m_grid->CommitPendingChanges( false ) )
                return;

            // Pop-up column order is the order of the shown fields, not the viewControls order
            col = event.GetId() - GRIDTRICKS_FIRST_SHOWHIDE;

            bool show = !m_dataModel->GetShowColumn( col );

            m_dlg->ShowHideColumn( col, show );

            wxString fieldName = m_dataModel->GetColFieldName( col );

            for( row = 0; row < m_viewControlsDataModel->GetNumberRows(); row++ )
            {
                if( m_viewControlsDataModel->GetCanonicalFieldName( row ) == fieldName )
                    m_viewControlsDataModel->SetValueAsBool( row, SHOW_FIELD_COLUMN, show );
            }

            if( m_viewControlsDataModel->GetView() )
                m_viewControlsDataModel->GetView()->ForceRefresh();
        }
        else
        {
            GRID_TRICKS::doPopupSelection( event );
        }
    }

private:
    DIALOG_SYMBOL_FIELDS_TABLE*    m_dlg;
    VIEW_CONTROLS_GRID_DATA_MODEL* m_viewControlsDataModel;
    FIELDS_EDITOR_GRID_DATA_MODEL* m_dataModel;
    EMBEDDED_FILES*                m_files;
};


DIALOG_SYMBOL_FIELDS_TABLE::DIALOG_SYMBOL_FIELDS_TABLE( SCH_EDIT_FRAME* parent, JOB_EXPORT_SCH_BOM* aJob ) :
        DIALOG_SYMBOL_FIELDS_TABLE_BASE( parent ),
        m_currentBomPreset( nullptr ),
        m_lastSelectedBomPreset( nullptr ),
        m_parent( parent ),
        m_viewControlsDataModel( nullptr ),
        m_dataModel( nullptr ),
        m_schSettings( parent->Schematic().Settings() ),
        m_job( aJob )
{
    // Get all symbols from the list of schematic sheets
    m_parent->Schematic().Hierarchy().GetSymbols( m_symbolsList, false );

    m_bRefresh->SetBitmap( KiBitmapBundle( BITMAPS::small_refresh ) );
    m_bMenu->SetBitmap( KiBitmapBundle( BITMAPS::config ) );
    m_bRefreshPreview->SetBitmap( KiBitmapBundle( BITMAPS::small_refresh ) );
    m_browseButton->SetBitmap( KiBitmapBundle( BITMAPS::small_folder ) );

    m_addFieldButton->SetBitmap( KiBitmapBundle( BITMAPS::small_plus ) );
    m_removeFieldButton->SetBitmap( KiBitmapBundle( BITMAPS::small_trash ) );
    m_renameFieldButton->SetBitmap( KiBitmapBundle( BITMAPS::small_edit ) );

    m_addVariantButton->SetBitmap( KiBitmapBundle( BITMAPS::small_plus ) );
    m_deleteVariantButton->SetBitmap( KiBitmapBundle( BITMAPS::small_trash ) );
    m_renameVariantButton->SetBitmap( KiBitmapBundle( BITMAPS::small_edit ) );
    m_copyVariantButton->SetBitmap( KiBitmapBundle( BITMAPS::copy ) );

    m_sidebarButton->SetBitmap( KiBitmapBundle( BITMAPS::left ) );

    m_viewControlsDataModel = new VIEW_CONTROLS_GRID_DATA_MODEL( true );

    m_viewControlsGrid->UseNativeColHeader( true );
    m_viewControlsGrid->SetTable( m_viewControlsDataModel, true );

    // must be done after SetTable(), which appears to re-set it
    m_viewControlsGrid->SetSelectionMode( wxGrid::wxGridSelectCells );

    // add Cut, Copy, and Paste to wxGrid
    m_viewControlsGrid->PushEventHandler( new VIEW_CONTROLS_GRID_TRICKS( m_viewControlsGrid ) );

    wxGridCellAttr* attr = new wxGridCellAttr;
    attr->SetReadOnly( true );
    m_viewControlsDataModel->SetColAttr( attr, DISPLAY_NAME_COLUMN );

    attr = new wxGridCellAttr;
    attr->SetRenderer( new wxGridCellBoolRenderer() );
    attr->SetReadOnly();    // not really; we delegate interactivity to GRID_TRICKS
    attr->SetAlignment( wxALIGN_CENTER, wxALIGN_CENTER );
    m_viewControlsDataModel->SetColAttr( attr, SHOW_FIELD_COLUMN );

    attr = new wxGridCellAttr;
    attr->SetRenderer( new wxGridCellBoolRenderer() );
    attr->SetReadOnly();    // not really; we delegate interactivity to GRID_TRICKS
    attr->SetAlignment( wxALIGN_CENTER, wxALIGN_CENTER );
    m_viewControlsDataModel->SetColAttr( attr, GROUP_BY_COLUMN );

    // Compress the view controls grid.  (We want it to look different from the fields grid.)
    m_viewControlsGrid->SetDefaultRowSize( m_viewControlsGrid->GetDefaultRowSize() - FromDIP( 4 ) );

    m_filter->SetDescriptiveText( _( "Filter" ) );

    attr = new wxGridCellAttr;
    attr->SetEditor( new GRID_CELL_URL_EDITOR( this, PROJECT_SCH::SchSearchS( &Prj() ), { &m_parent->Schematic() } ) );
    m_dataModel = new FIELDS_EDITOR_GRID_DATA_MODEL( m_symbolsList, attr );

    m_grid->UseNativeColHeader( true );
    m_grid->SetTable( m_dataModel, true );

    // must be done after SetTable(), which appears to re-set it
    m_grid->SetSelectionMode( wxGrid::wxGridSelectCells );

    // add Cut, Copy, and Paste to wxGrid
    m_grid->PushEventHandler( new FIELDS_EDITOR_GRID_TRICKS( this, m_grid, m_viewControlsDataModel, m_dataModel,
                                                             &m_parent->Schematic() ) );

    m_variantListBox->Set( parent->Schematic().GetVariantNamesForUI() );

    if( !m_parent->Schematic().GetCurrentVariant().IsEmpty() )
    {
        int toSelect = m_variantListBox->FindString( m_parent->Schematic().GetCurrentVariant() );

        if( toSelect == wxNOT_FOUND )
            m_variantListBox->SetSelection( 0 );
        else
            m_variantListBox->SetSelection( toSelect );
    }
    else
    {
        m_variantListBox->SetSelection( 0 );
    }

    updateVariantButtonStates();

    if( m_job )
        SetTitle( m_job->GetSettingsDialogTitle() );

    // DIALOG_SHIM needs a unique hash_key because classname will be the same for both job and
    // non-job versions (which have different sizes).
    m_hash_key = TO_UTF8( GetTitle() );

    // Set the current variant for highlighting variant-specific field values
    m_dataModel->SetCurrentVariant( m_parent->Schematic().GetCurrentVariant() );

    SetInitialFocus( m_grid );
    m_grid->ClearSelection();

    SetupStandardButtons();

    finishDialogSettings();

    SetSize( wxSize( horizPixelsFromDU( 600 ), vertPixelsFromDU( 300 ) ) );

    EESCHEMA_SETTINGS::PANEL_SYMBOL_FIELDS_TABLE& cfg = m_parent->eeconfig()->m_FieldEditorPanel;

    m_viewControlsGrid->ShowHideColumns( "0 1 2 3" );

    CallAfter( [this, cfg]()
               {
                   if( cfg.sidebar_collapsed )
                       m_splitterMainWindow->Unsplit( m_leftPanel );
                   else
                       m_splitterMainWindow->SetSashPosition( cfg.sash_pos );

                   setSideBarButtonLook( cfg.sidebar_collapsed );

                   m_splitter_left->SetSashPosition( cfg.variant_sash_pos );
               } );

    if( m_job )
        m_outputFileName->SetValue( m_job->GetConfiguredOutputPath() );
    else
        m_outputFileName->SetValue( m_schSettings.m_BomExportFileName );

    Center();

    // Connect Events
    m_grid->Bind( wxEVT_GRID_COL_SORT, &DIALOG_SYMBOL_FIELDS_TABLE::OnColSort, this );
    m_grid->Bind( wxEVT_GRID_COL_MOVE, &DIALOG_SYMBOL_FIELDS_TABLE::OnColMove, this );
    m_cbBomPresets->Bind( wxEVT_CHOICE, &DIALOG_SYMBOL_FIELDS_TABLE::onBomPresetChanged, this );
    m_cbBomFmtPresets->Bind( wxEVT_CHOICE, &DIALOG_SYMBOL_FIELDS_TABLE::onBomFmtPresetChanged, this );
    m_viewControlsGrid->Bind( wxEVT_GRID_CELL_CHANGED, &DIALOG_SYMBOL_FIELDS_TABLE::OnViewControlsCellChanged, this );

    if( !m_job )
    {
        // Start listening for schematic changes
        m_parent->Schematic().AddListener( this );
    }
    else
    {
        // Don't allow editing
        m_grid->EnableEditing( false );
        m_buttonApply->Hide();
        m_buttonExport->Hide();
    }
}


DIALOG_SYMBOL_FIELDS_TABLE::~DIALOG_SYMBOL_FIELDS_TABLE()
{
    savePresetsToSchematic();
    m_schSettings.m_BomExportFileName = m_outputFileName->GetValue();

    EESCHEMA_SETTINGS::PANEL_SYMBOL_FIELDS_TABLE& cfg = m_parent->eeconfig()->m_FieldEditorPanel;

    if( !cfg.sidebar_collapsed )
        cfg.sash_pos = m_splitterMainWindow->GetSashPosition();

    cfg.variant_sash_pos = m_splitter_left->GetSashPosition();

    for( int i = 0; i < m_grid->GetNumberCols(); i++ )
    {
        if( m_grid->IsColShown( i ) )
        {
            std::string fieldName( m_dataModel->GetColFieldName( i ).ToUTF8() );
            cfg.field_widths[fieldName] = m_grid->GetColSize( i );
        }
    }

    // Disconnect Events
    m_grid->Unbind( wxEVT_GRID_COL_SORT, &DIALOG_SYMBOL_FIELDS_TABLE::OnColSort, this );
    m_grid->Unbind( wxEVT_GRID_COL_SORT, &DIALOG_SYMBOL_FIELDS_TABLE::OnColMove, this );
    m_cbBomPresets->Unbind( wxEVT_CHOICE, &DIALOG_SYMBOL_FIELDS_TABLE::onBomPresetChanged, this );
    m_cbBomFmtPresets->Unbind( wxEVT_CHOICE, &DIALOG_SYMBOL_FIELDS_TABLE::onBomFmtPresetChanged, this );
    m_viewControlsGrid->Unbind( wxEVT_GRID_CELL_CHANGED, &DIALOG_SYMBOL_FIELDS_TABLE::OnViewControlsCellChanged, this );

    // Delete the GRID_TRICKS.
    m_viewControlsGrid->PopEventHandler( true );
    m_grid->PopEventHandler( true );

    // we gave ownership of m_viewControlsDataModel & m_dataModel to the wxGrids...
}


void DIALOG_SYMBOL_FIELDS_TABLE::setSideBarButtonLook( bool aIsLeftPanelCollapsed )
{
    // Set bitmap and tooltip according to left panel visibility

    if( aIsLeftPanelCollapsed )
    {
        m_sidebarButton->SetBitmap( KiBitmapBundle( BITMAPS::right ) );
        m_sidebarButton->SetToolTip( _( "Expand left panel" ) );
    }
    else
    {
        m_sidebarButton->SetBitmap( KiBitmapBundle( BITMAPS::left ) );
        m_sidebarButton->SetToolTip( _( "Collapse left panel" ) );
    }
}


void DIALOG_SYMBOL_FIELDS_TABLE::SetupColumnProperties( int aCol )
{
    wxGridCellAttr* attr = new wxGridCellAttr;
    attr->SetReadOnly( false );

    // Set some column types to specific editors
    if( m_dataModel->ColIsReference( aCol ) )
    {
        attr->SetReadOnly();
        attr->SetRenderer( new GRID_CELL_TEXT_RENDERER() );
        m_dataModel->SetColAttr( attr, aCol );
    }
    else if( m_dataModel->GetColFieldName( aCol ) == GetCanonicalFieldName( FIELD_T::FOOTPRINT ) )
    {
        attr->SetEditor( new GRID_CELL_FPID_EDITOR( this, wxEmptyString ) );
        m_dataModel->SetColAttr( attr, aCol );
    }
    else if( m_dataModel->GetColFieldName( aCol ) == GetCanonicalFieldName( FIELD_T::DATASHEET ) )
    {
        // set datasheet column viewer button
        attr->SetEditor( new GRID_CELL_URL_EDITOR( this, PROJECT_SCH::SchSearchS( &Prj() ),
                                                   { &m_parent->Schematic() } ) );
        m_dataModel->SetColAttr( attr, aCol );
    }
    else if( m_dataModel->ColIsQuantity( aCol ) || m_dataModel->ColIsItemNumber( aCol ) )
    {
        attr->SetReadOnly();
        attr->SetAlignment( wxALIGN_RIGHT, wxALIGN_CENTER );
        attr->SetRenderer( new wxGridCellNumberRenderer() );
        m_dataModel->SetColAttr( attr, aCol );
    }
    else if( m_dataModel->ColIsAttribute( aCol ) )
    {
        attr->SetAlignment( wxALIGN_CENTER, wxALIGN_CENTER );
        attr->SetRenderer( new GRID_CELL_CHECKBOX_RENDERER() );
        attr->SetReadOnly();    // not really; we delegate interactivity to GRID_TRICKS
        m_dataModel->SetColAttr( attr, aCol );
    }
    else if( IsGeneratedField( m_dataModel->GetColFieldName( aCol ) ) )
    {
        attr->SetReadOnly();
        m_dataModel->SetColAttr( attr, aCol );
    }
    else
    {
        attr->SetRenderer( new GRID_CELL_TEXT_RENDERER() );
        attr->SetEditor( m_grid->GetDefaultEditor() );
        m_dataModel->SetColAttr( attr, aCol );
    }
}


void DIALOG_SYMBOL_FIELDS_TABLE::SetupAllColumnProperties()
{
    EESCHEMA_SETTINGS* cfg = m_parent->eeconfig();
    wxSize defaultDlgSize = ConvertDialogToPixels( wxSize( 600, 300 ) );

    // Restore column sorting order and widths
    m_grid->AutoSizeColumns( false );
    int  sortCol = 0;
    bool sortAscending = true;

    for( int col = 0; col < m_grid->GetNumberCols(); ++col )
    {
        SetupColumnProperties( col );

        if( col == m_dataModel->GetSortCol() )
        {
            sortCol = col;
            sortAscending = m_dataModel->GetSortAsc();
        }
    }

    // sync m_grid's column visibilities to Show checkboxes in m_viewControlsGrid
    for( int i = 0; i < m_viewControlsDataModel->GetNumberRows(); ++i )
    {
        int col = m_dataModel->GetFieldNameCol( m_viewControlsDataModel->GetCanonicalFieldName( i ) );

        if( col == -1 )
            continue;

        bool show = m_viewControlsDataModel->GetValueAsBool( i, SHOW_FIELD_COLUMN );
        m_dataModel->SetShowColumn( col, show );

        if( show )
        {
            m_grid->ShowCol( col );

            std::string key( m_dataModel->GetColFieldName( col ).ToUTF8() );

            if( cfg->m_FieldEditorPanel.field_widths.count( key )
                && ( cfg->m_FieldEditorPanel.field_widths.at( key ) > 0 ) )
            {
                m_grid->SetColSize( col, cfg->m_FieldEditorPanel.field_widths.at( key ) );
            }
            else
            {
                int textWidth = m_dataModel->GetDataWidth( col ) + COLUMN_MARGIN;
                int maxWidth = defaultDlgSize.x / 3;

                m_grid->SetColSize( col, std::clamp( textWidth, 100, maxWidth ) );
            }
        }
        else
        {
            m_grid->HideCol( col );
        }
    }

    m_dataModel->SetSorting( sortCol, sortAscending );
    m_grid->SetSortingColumn( sortCol, sortAscending );
}


bool DIALOG_SYMBOL_FIELDS_TABLE::TransferDataToWindow()
{
    if( !wxDialog::TransferDataToWindow() )
        return false;

    LoadFieldNames();   // loads rows into m_viewControlsDataModel and columns into m_dataModel

    // Load our BOM view presets
    SetUserBomPresets( m_schSettings.m_BomPresets );

    BOM_PRESET preset = m_schSettings.m_BomSettings;

    if( m_job )
    {
        preset.name = m_job->m_bomPresetName;
        preset.excludeDNP = m_job->m_excludeDNP;
        preset.filterString = m_job->m_filterString;
        preset.sortAsc = m_job->m_sortAsc;
        preset.sortField = m_job->m_sortField;
        preset.groupSymbols = ( m_job->m_fieldsGroupBy.size() > 0 );

        preset.fieldsOrdered.clear();

        size_t i = 0;

        for( const wxString& fieldName : m_job->m_fieldsOrdered )
        {
            BOM_FIELD field;
            field.name = fieldName;
            field.show = !fieldName.StartsWith( wxT( "__" ), &field.name );
            field.groupBy = alg::contains( m_job->m_fieldsGroupBy, field.name );

            if( ( m_job->m_fieldsLabels.size() > i ) && !m_job->m_fieldsLabels[i].IsEmpty() )
                field.label = m_job->m_fieldsLabels[i];
            else if( IsGeneratedField( field.name ) )
                field.label = GetGeneratedFieldDisplayName( field.name );
            else
                field.label = field.name;

            preset.fieldsOrdered.emplace_back( field );
            i++;
        }
    }

    ApplyBomPreset( preset );
    syncBomPresetSelection();

    // Load BOM export format presets
    SetUserBomFmtPresets( m_schSettings.m_BomFmtPresets );
    BOM_FMT_PRESET fmtPreset = m_schSettings.m_BomFmtSettings;

    if( m_job )
    {
        fmtPreset.name = m_job->m_bomFmtPresetName;
        fmtPreset.fieldDelimiter = m_job->m_fieldDelimiter;
        fmtPreset.keepLineBreaks = m_job->m_keepLineBreaks;
        fmtPreset.keepTabs = m_job->m_keepTabs;
        fmtPreset.refDelimiter = m_job->m_refDelimiter;
        fmtPreset.refRangeDelimiter = m_job->m_refRangeDelimiter;
        fmtPreset.stringDelimiter = m_job->m_stringDelimiter;
    }

    ApplyBomFmtPreset( fmtPreset );
    syncBomFmtPresetSelection();

    TOOL_MANAGER*       toolMgr = m_parent->GetToolManager();
    SCH_SELECTION_TOOL* selectionTool = toolMgr->GetTool<SCH_SELECTION_TOOL>();
    SCH_SELECTION&      selection = selectionTool->GetSelection();
    SCH_SYMBOL*         symbol = nullptr;

    m_dataModel->SetGroupingEnabled( m_groupSymbolsBox->GetValue() );

    wxCommandEvent dummy;
    OnScope( dummy );

    if( selection.GetSize() == 1 )
    {
        EDA_ITEM* item = selection.Front();

        if( item->Type() == SCH_SYMBOL_T )
            symbol = (SCH_SYMBOL*) item;
        else if( item->GetParent() && item->GetParent()->Type() == SCH_SYMBOL_T )
            symbol = (SCH_SYMBOL*) item->GetParent();
    }

    if( symbol )
    {
        for( int row = 0; row < m_dataModel->GetNumberRows(); ++row )
        {
            std::vector<SCH_REFERENCE> references = m_dataModel->GetRowReferences( row );
            bool                       found = false;

            for( const SCH_REFERENCE& ref : references )
            {
                if( ref.GetSymbol() == symbol )
                {
                    found = true;
                    break;
                }
            }

            if( found )
            {
                // Find the value column and the reference column if they're shown
                int valueCol = -1;
                int refCol = -1;
                int anyCol = -1;

                for( int col = 0; col < m_dataModel->GetNumberCols(); col++ )
                {
                    if( m_dataModel->ColIsValue( col ) )
                        valueCol = col;
                    else if( m_dataModel->ColIsReference( col ) )
                        refCol = col;
                    else if( anyCol == -1 && m_dataModel->GetShowColumn( col ) )
                        anyCol = col;
                }

                if( valueCol != -1 && m_dataModel->GetShowColumn( valueCol ) )
                    m_grid->GoToCell( row, valueCol );
                else if( refCol != -1 && m_dataModel->GetShowColumn( refCol ) )
                    m_grid->GoToCell( row, refCol );
                else if( anyCol != -1 )
                    m_grid->GoToCell( row, anyCol );

                break;
            }
        }
    }

    // We don't want table range selection events to happen until we've loaded the data or we
    // we'll clear our selection as the grid is built before the code above can get the
    // user's current selection.
    EnableSelectionEvents();

    return true;
}


bool DIALOG_SYMBOL_FIELDS_TABLE::TransferDataFromWindow()
{
    if( !m_grid->CommitPendingChanges() )
        return false;

    if( !wxDialog::TransferDataFromWindow() )
        return false;

    if( m_job )
    {
        // and exit, don't even dream of saving changes from the data model
        return true;
    }

    SCH_COMMIT     commit( m_parent );
    SCH_SHEET_PATH currentSheet = m_parent->GetCurrentSheet();
    wxString       currentVariant = m_parent->Schematic().GetCurrentVariant();

    m_dataModel->ApplyData( commit, m_schSettings.m_TemplateFieldNames, currentVariant );

    if( !commit.Empty() )
    {
        commit.Push( wxS( "Symbol Fields Table Edit" ) );  // Push clears the commit buffer.
        m_parent->OnModify();
    }

    // Reset the view to where we left the user
    m_parent->SetCurrentSheet( currentSheet );
    m_parent->SyncView();
    m_parent->Refresh();

    return true;
}


void DIALOG_SYMBOL_FIELDS_TABLE::AddField( const wxString& aFieldName, const wxString& aLabelValue,
                                           bool show, bool groupBy, bool addedByUser )
{
    // Users can add fields with variable names that match the special names in the grid,
    // e.g. ${QUANTITY} so make sure we don't add them twice
    for( int row = 0; row < m_viewControlsDataModel->GetNumberRows(); row++ )
    {
        if( m_viewControlsDataModel->GetCanonicalFieldName( row ) == aFieldName )
            return;
    }

    m_dataModel->AddColumn( aFieldName, aLabelValue, addedByUser, m_parent->Schematic().GetCurrentVariant() );

    wxGridTableMessage msg( m_dataModel, wxGRIDTABLE_NOTIFY_COLS_APPENDED, 1 );
    m_grid->ProcessTableMessage( msg );

    m_viewControlsGrid->OnAddRow(
            [&]() -> std::pair<int, int>
            {
                m_viewControlsDataModel->AppendRow( aFieldName, aLabelValue, show, groupBy );

                return { m_viewControlsDataModel->GetNumberRows() - 1, -1 };
            } );
}


void DIALOG_SYMBOL_FIELDS_TABLE::LoadFieldNames()
{
    auto addMandatoryField =
            [&]( FIELD_T fieldId, bool show, bool groupBy )
            {
                m_mandatoryFieldListIndexes[fieldId] = m_viewControlsDataModel->GetNumberRows();

                AddField( GetCanonicalFieldName( fieldId ), GetDefaultFieldName( fieldId, DO_TRANSLATE ),
                          show, groupBy );
            };

    // Add mandatory fields first            show   groupBy
    addMandatoryField( FIELD_T::REFERENCE,   true,   true   );
    addMandatoryField( FIELD_T::VALUE,       true,   true   );
    addMandatoryField( FIELD_T::FOOTPRINT,   true,   true   );
    addMandatoryField( FIELD_T::DATASHEET,   true,   false  );
    addMandatoryField( FIELD_T::DESCRIPTION, false,  false  );

    // Generated fields present only in the fields table
    AddField( FIELDS_EDITOR_GRID_DATA_MODEL::QUANTITY_VARIABLE, _( "Qty" ), true, false );
    AddField( FIELDS_EDITOR_GRID_DATA_MODEL::ITEM_NUMBER_VARIABLE, _( "#" ), true, false );

    // User fields next
    std::set<wxString> userFieldNames;

    for( int ii = 0; ii < (int) m_symbolsList.GetCount(); ++ii )
    {
        SCH_SYMBOL* symbol = m_symbolsList[ii].GetSymbol();

        for( const SCH_FIELD& field : symbol->GetFields() )
        {
            if( !field.IsMandatory() && !field.IsPrivate() )
                userFieldNames.insert( field.GetName() );
        }
    }

    for( const wxString& fieldName : userFieldNames )
        AddField( fieldName, GetGeneratedFieldDisplayName( fieldName ), true, false );

    // Add any templateFieldNames which aren't already present in the userFieldNames
    for( const TEMPLATE_FIELDNAME& tfn : m_schSettings.m_TemplateFieldNames.GetTemplateFieldNames() )
    {
        if( userFieldNames.count( tfn.m_Name ) == 0 )
            AddField( tfn.m_Name, GetGeneratedFieldDisplayName( tfn.m_Name ), false, false );
    }
}


void DIALOG_SYMBOL_FIELDS_TABLE::OnAddField( wxCommandEvent& event )
{
    wxTextEntryDialog dlg( this, _( "New field name:" ), _( "Add Field" ) );

    if( dlg.ShowModal() != wxID_OK )
        return;

    wxString fieldName = dlg.GetValue();

    if( fieldName.IsEmpty() )
    {
        DisplayError( this, _( "Field must have a name." ) );
        return;
    }

    for( int i = 0; i < m_dataModel->GetNumberCols(); ++i )
    {
        if( fieldName == m_dataModel->GetColFieldName( i ) )
        {
            DisplayError( this, wxString::Format( _( "Field name '%s' already in use." ), fieldName ) );
            return;
        }
    }

    AddField( fieldName, GetGeneratedFieldDisplayName( fieldName ), true, false, true );

    SetupColumnProperties( m_dataModel->GetColsCount() - 1 );

    syncBomPresetSelection();
    OnModify();
}


void DIALOG_SYMBOL_FIELDS_TABLE::OnRemoveField( wxCommandEvent& event )
{
    m_viewControlsGrid->OnDeleteRows(
            [&]( int row )
            {
                for( FIELD_T id : MANDATORY_FIELDS )
                {
                    if( m_mandatoryFieldListIndexes[id] == row )
                    {
                        DisplayError( this, wxString::Format( _( "The first %d fields are mandatory." ),
                                                              (int) m_mandatoryFieldListIndexes.size() ) );
                        return false;
                    }
                }

                return IsOK( this, wxString::Format( _( "Are you sure you want to remove the field '%s'?" ),
                                                     m_viewControlsDataModel->GetValue( row, DISPLAY_NAME_COLUMN ) ) );
            },
            [&]( int row )
            {
                wxString fieldName = m_viewControlsDataModel->GetCanonicalFieldName( row );
                int col = m_dataModel->GetFieldNameCol( fieldName );

                if( col != -1 )
                    m_dataModel->RemoveColumn( col );

                m_viewControlsDataModel->DeleteRow( row );

                syncBomPresetSelection();
                OnModify();
            } );
}


void DIALOG_SYMBOL_FIELDS_TABLE::OnRenameField( wxCommandEvent& event )
{
    wxArrayInt selectedRows = m_viewControlsGrid->GetSelectedRows();

    if( selectedRows.empty() && m_viewControlsGrid->GetGridCursorRow() >= 0 )
        selectedRows.push_back( m_viewControlsGrid->GetGridCursorRow() );

    if( selectedRows.empty() )
        return;

    int row = selectedRows[0];

    for( FIELD_T id : MANDATORY_FIELDS )
    {
        if( m_mandatoryFieldListIndexes[id] == row )
        {
            DisplayError( this, wxString::Format( _( "The first %d fields are mandatory and names cannot be changed." ),
                                                  (int) m_mandatoryFieldListIndexes.size() ) );
            return;
        }
    }

    wxString fieldName = m_viewControlsDataModel->GetCanonicalFieldName( row );
    wxString label = m_viewControlsDataModel->GetValue( row, LABEL_COLUMN );
    bool     labelIsAutogenerated = label.IsSameAs( GetGeneratedFieldDisplayName( fieldName ) );

    int col = m_dataModel->GetFieldNameCol( fieldName );
    wxCHECK_RET( col != -1, wxS( "Existing field name missing from data model" ) );

    wxTextEntryDialog dlg( this, _( "New field name:" ), _( "Rename Field" ), fieldName );

    if( dlg.ShowModal() != wxID_OK )
         return;

    wxString newFieldName = dlg.GetValue();

    // No change, no-op
    if( newFieldName == fieldName )
         return;

    // New field name already exists
    if( m_dataModel->GetFieldNameCol( newFieldName ) != -1 )
    {
         wxString confirm_msg = wxString::Format( _( "Field name %s already exists." ), newFieldName );
         DisplayError( this, confirm_msg );
         return;
    }

    m_dataModel->RenameColumn( col, newFieldName );
    m_viewControlsDataModel->SetCanonicalFieldName( row, newFieldName );
    m_viewControlsDataModel->SetValue( row, DISPLAY_NAME_COLUMN, newFieldName );

    if( labelIsAutogenerated )
    {
        m_viewControlsDataModel->SetValue( row, LABEL_COLUMN, GetGeneratedFieldDisplayName( newFieldName ) );
        wxGridEvent evt( m_viewControlsGrid->GetId(), wxEVT_GRID_CELL_CHANGED, m_viewControlsGrid, row, LABEL_COLUMN );
        OnViewControlsCellChanged( evt );
    }

    syncBomPresetSelection();
    OnModify();
}


void DIALOG_SYMBOL_FIELDS_TABLE::OnFilterText( wxCommandEvent& aEvent )
{
    m_dataModel->SetFilter( m_filter->GetValue() );
    m_dataModel->RebuildRows();
    m_grid->ForceRefresh();

    syncBomPresetSelection();
}


void DIALOG_SYMBOL_FIELDS_TABLE::OnFilterMouseMoved( wxMouseEvent& aEvent )
{
#if defined( __WXOSX__ ) // Doesn't work properly on other ports
    wxPoint pos = aEvent.GetPosition();
    wxRect  ctrlRect = m_filter->GetScreenRect();
    int     buttonWidth = ctrlRect.GetHeight();         // Presume buttons are square

    // TODO: restore cursor when mouse leaves the filter field (or is it a MSW bug?)
    if( m_filter->IsSearchButtonVisible() && pos.x < buttonWidth )
        SetCursor( wxCURSOR_ARROW );
    else if( m_filter->IsCancelButtonVisible() && pos.x > ctrlRect.GetWidth() - buttonWidth )
        SetCursor( wxCURSOR_ARROW );
    else
        SetCursor( wxCURSOR_IBEAM );
#endif
}


void DIALOG_SYMBOL_FIELDS_TABLE::setScope( SCOPE aScope )
{
    m_dataModel->SetPath( m_parent->GetCurrentSheet() );
    m_dataModel->SetScope( aScope );
    m_dataModel->RebuildRows();
}


void DIALOG_SYMBOL_FIELDS_TABLE::OnScope( wxCommandEvent& aEvent )
{
    switch( aEvent.GetSelection() )
    {
    case 0: setScope( SCOPE::SCOPE_ALL );             break;
    case 1: setScope( SCOPE::SCOPE_SHEET );           break;
    case 2: setScope( SCOPE::SCOPE_SHEET_RECURSIVE ); break;
    }
}


void DIALOG_SYMBOL_FIELDS_TABLE::OnGroupSymbolsToggled( wxCommandEvent& event )
{
    m_dataModel->SetGroupingEnabled( m_groupSymbolsBox->GetValue() );
    m_dataModel->RebuildRows();
    m_grid->ForceRefresh();

    syncBomPresetSelection();
}


void DIALOG_SYMBOL_FIELDS_TABLE::OnMenu( wxCommandEvent& event )
{
    EESCHEMA_SETTINGS::PANEL_SYMBOL_FIELDS_TABLE& cfg = m_parent->eeconfig()->m_FieldEditorPanel;

    // Build a pop menu:
    wxMenu menu;

    menu.Append( 4204, _( "Include 'DNP' Symbols" ),
                 _( "Show symbols marked 'DNP' in the table.  This setting also controls whether or not 'DNP' "
                    "symbols are included on export." ),
                 wxITEM_CHECK );
    menu.Check( 4204, !m_dataModel->GetExcludeDNP() );

    menu.Append( 4205, _( "Include 'Exclude from BOM' Symbols" ),
                 _( "Show symbols marked 'Exclude from BOM' in the table.  Symbols marked 'Exclude from BOM' "
                    "are never included on export." ),
                 wxITEM_CHECK );
    menu.Check( 4205, m_dataModel->GetIncludeExcludedFromBOM() );

    menu.AppendSeparator();

    menu.Append( 4206, _( "Highlight on Cross-probe" ),
                 _( "Highlight corresponding item on canvas when it is selected in the table" ),
                 wxITEM_CHECK );
    menu.Check( 4206, cfg.selection_mode == 0 );

    menu.Append( 4207, _( "Select on Cross-probe" ),
                 _( "Select corresponding item on canvas when it is selected in the table" ),
                 wxITEM_CHECK );
    menu.Check( 4207, cfg.selection_mode == 1 );

    // menu_id is the selected submenu id from the popup menu or wxID_NONE
    int menu_id = m_bMenu->GetPopupMenuSelectionFromUser( menu );

    if( menu_id == 0 || menu_id == 4204 )
    {
        m_dataModel->SetExcludeDNP( !m_dataModel->GetExcludeDNP() );
        m_dataModel->RebuildRows();
        m_grid->ForceRefresh();

        syncBomPresetSelection();
    }
    else if( menu_id == 1 || menu_id == 4205 )
    {
        m_dataModel->SetIncludeExcludedFromBOM( !m_dataModel->GetIncludeExcludedFromBOM() );
        m_dataModel->RebuildRows();
        m_grid->ForceRefresh();

        syncBomPresetSelection();
    }
    else if( menu_id == 3 || menu_id == 4206 )
    {
        if( cfg.selection_mode != 0 )
            cfg.selection_mode = 0;
        else
            cfg.selection_mode = 2;
    }
    else if( menu_id == 4 || menu_id == 4207 )
    {
        if( cfg.selection_mode != 1 )
            cfg.selection_mode = 1;
        else
            cfg.selection_mode = 2;
    }
}


void DIALOG_SYMBOL_FIELDS_TABLE::OnColSort( wxGridEvent& aEvent )
{
    int         sortCol = aEvent.GetCol();
    std::string key( m_dataModel->GetColFieldName( sortCol ).ToUTF8() );
    bool        ascending;

    // Don't sort by item number, it is generated by the sort
    if( m_dataModel->ColIsItemNumber( sortCol ) )
    {
        aEvent.Veto();
        return;
    }

    // This is bonkers, but wxWidgets doesn't tell us ascending/descending in the event, and
    // if we ask it will give us pre-event info.
    if( m_grid->IsSortingBy( sortCol ) )
    {
        // same column; invert ascending
        ascending = !m_grid->IsSortOrderAscending();
    }
    else
    {
        // different column; start with ascending
        ascending = true;
    }

    m_dataModel->SetSorting( sortCol, ascending );
    m_dataModel->RebuildRows();
    m_grid->ForceRefresh();

    syncBomPresetSelection();
}


void DIALOG_SYMBOL_FIELDS_TABLE::OnColMove( wxGridEvent& aEvent )
{
    int origPos = aEvent.GetCol();

    // Save column widths since the setup function uses the saved config values
    EESCHEMA_SETTINGS* cfg = m_parent->eeconfig();

    for( int i = 0; i < m_grid->GetNumberCols(); i++ )
    {
        if( m_grid->IsColShown( i ) )
        {
            std::string fieldName( m_dataModel->GetColFieldName( i ).ToUTF8() );
            cfg->m_FieldEditorPanel.field_widths[fieldName] = m_grid->GetColSize( i );
        }
    }

    CallAfter(
            [origPos, this]()
            {
                int newPos = m_grid->GetColPos( origPos );

#ifdef __WXMAC__
                if( newPos < origPos )
                    newPos += 1;
#endif

                m_dataModel->MoveColumn( origPos, newPos );

                // "Unmove" the column since we've moved the column internally
                m_grid->ResetColPos();

                // We need to reset all the column attr's to the correct column order
                SetupAllColumnProperties();

                m_grid->ForceRefresh();
            } );

    syncBomPresetSelection();
}


void DIALOG_SYMBOL_FIELDS_TABLE::ShowHideColumn( int aCol, bool aShow )
{
    if( aShow )
        m_grid->ShowCol( aCol );
    else
        m_grid->HideCol( aCol );

    m_dataModel->SetShowColumn( aCol, aShow );

    syncBomPresetSelection();

    if( m_nbPages->GetSelection() == 1 )
        PreviewRefresh();
    else
        m_grid->ForceRefresh();

    OnModify();
}


void DIALOG_SYMBOL_FIELDS_TABLE::OnViewControlsCellChanged( wxGridEvent& aEvent )
{
    int row = aEvent.GetRow();

    wxCHECK( row < m_viewControlsGrid->GetNumberRows(), /* void */ );

    switch( aEvent.GetCol() )
    {
    case LABEL_COLUMN:
    {
        wxString label = m_viewControlsDataModel->GetValue( row, LABEL_COLUMN );
        wxString fieldName = m_viewControlsDataModel->GetCanonicalFieldName( row );
        int      dataCol = m_dataModel->GetFieldNameCol( fieldName );

        if( dataCol != -1 )
        {
            m_dataModel->SetColLabelValue( dataCol, label );
            m_grid->SetColLabelValue( dataCol, label );

            if( m_nbPages->GetSelection() == 1 )
                PreviewRefresh();
            else
                m_grid->ForceRefresh();

            syncBomPresetSelection();
            OnModify();
        }

        break;
    }

    case SHOW_FIELD_COLUMN:
    {
        wxString fieldName = m_viewControlsDataModel->GetCanonicalFieldName( row );
        bool     value = m_viewControlsDataModel->GetValueAsBool( row, SHOW_FIELD_COLUMN );
        int      dataCol = m_dataModel->GetFieldNameCol( fieldName );

        if( dataCol != -1 )
            ShowHideColumn( dataCol, value );

        break;
    }

    case GROUP_BY_COLUMN:
    {
        wxString fieldName = m_viewControlsDataModel->GetCanonicalFieldName( row );
        bool     value = m_viewControlsDataModel->GetValueAsBool( row, GROUP_BY_COLUMN );
        int      dataCol = m_dataModel->GetFieldNameCol( fieldName );

        if( m_dataModel->ColIsQuantity( dataCol ) && value )
        {
            DisplayError( this, _( "The Quantity column cannot be grouped by." ) );

            value = false;
            m_viewControlsDataModel->SetValueAsBool( row, GROUP_BY_COLUMN, value );
            break;
        }

        if( m_dataModel->ColIsItemNumber( dataCol ) && value )
        {
            DisplayError( this, _( "The Item Number column cannot be grouped by." ) );

            value = false;
            m_viewControlsDataModel->SetValueAsBool( row, GROUP_BY_COLUMN, value );
            break;
        }

        m_dataModel->SetGroupColumn( dataCol, value );
        m_dataModel->RebuildRows();

        if( m_nbPages->GetSelection() == 1 )
            PreviewRefresh();
        else
            m_grid->ForceRefresh();

        syncBomPresetSelection();
        OnModify();
        break;
    }

    default:
        break;
    }
}


void DIALOG_SYMBOL_FIELDS_TABLE::OnTableValueChanged( wxGridEvent& aEvent )
{
    m_grid->ForceRefresh();
}


void DIALOG_SYMBOL_FIELDS_TABLE::OnTableColSize( wxGridSizeEvent& aEvent )
{
    aEvent.Skip();

    m_grid->ForceRefresh();
}


void DIALOG_SYMBOL_FIELDS_TABLE::OnRegroupSymbols( wxCommandEvent& aEvent )
{
    m_dataModel->RebuildRows();
    m_grid->ForceRefresh();
}


void DIALOG_SYMBOL_FIELDS_TABLE::OnTableCellClick( wxGridEvent& event )
{
    if( m_dataModel->IsExpanderColumn( event.GetCol() ) )
    {
        m_grid->ClearSelection();

        m_dataModel->ExpandCollapseRow( event.GetRow() );
        m_grid->SetGridCursor( event.GetRow(), event.GetCol() );
    }
    else
    {
        event.Skip();
    }
}


void DIALOG_SYMBOL_FIELDS_TABLE::OnTableRangeSelected( wxGridRangeSelectEvent& aEvent )
{
    EESCHEMA_SETTINGS::PANEL_SYMBOL_FIELDS_TABLE& cfg = m_parent->eeconfig()->m_FieldEditorPanel;

    // Cross-probing should only work in Edit page
    if( m_nbPages->GetSelection() != 0 )
        return;

    // Multi-select can grab the rows that are expanded child refs, and also the row
    // containing the list of all child refs. Make sure we add refs/symbols uniquely
    std::set<SCH_REFERENCE> refs;
    std::set<SCH_ITEM*>     symbols;

    // This handler handles selecting and deselecting
    if( aEvent.Selecting() )
    {
        for( int i = aEvent.GetTopRow(); i <= aEvent.GetBottomRow(); i++ )
        {
            for( const SCH_REFERENCE& ref : m_dataModel->GetRowReferences( i ) )
                refs.insert( ref );
        }

        for( const SCH_REFERENCE& ref : refs )
            symbols.insert( ref.GetSymbol() );
    }

    if( cfg.selection_mode == 0 )
    {
        SCH_EDITOR_CONTROL* editor = m_parent->GetToolManager()->GetTool<SCH_EDITOR_CONTROL>();

        if( refs.size() > 0 )
        {
            // Use of full path based on UUID allows select of not yet annotated or duplicated
            // symbols
            wxString symbol_path = refs.begin()->GetFullPath();

            // Focus only handles one item at this time
            editor->FindSymbolAndItem( &symbol_path, nullptr, true, HIGHLIGHT_SYMBOL, wxEmptyString );
        }
        else
        {
            m_parent->ClearFocus();
        }
    }
    else if( cfg.selection_mode == 1 )
    {
        SCH_SELECTION_TOOL*    selTool = m_parent->GetToolManager()->GetTool<SCH_SELECTION_TOOL>();
        std::vector<SCH_ITEM*> items( symbols.begin(), symbols.end() );

        if( refs.size() > 0 )
            selTool->SyncSelection( refs.begin()->GetSheetPath(), nullptr, items );
        else
            selTool->ClearSelection();
    }
}


void DIALOG_SYMBOL_FIELDS_TABLE::OnSizeViewControlsGrid( wxSizeEvent& event )
{
    const wxString& showColLabel = m_viewControlsGrid->GetColLabelValue( SHOW_FIELD_COLUMN );
    const wxString& groupByColLabel = m_viewControlsGrid->GetColLabelValue( GROUP_BY_COLUMN );
    int             showColWidth = KIUI::GetTextSize( showColLabel, m_viewControlsGrid ).x + COLUMN_MARGIN;
    int             groupByColWidth = KIUI::GetTextSize( groupByColLabel, m_viewControlsGrid ).x + COLUMN_MARGIN;
    int             remainingWidth = m_viewControlsGrid->GetSize().GetX() - showColWidth - groupByColWidth;

    m_viewControlsGrid->SetColSize( showColWidth, SHOW_FIELD_COLUMN );
    m_viewControlsGrid->SetColSize( groupByColWidth, GROUP_BY_COLUMN );

    if( m_viewControlsGrid->IsColShown( DISPLAY_NAME_COLUMN ) && m_viewControlsGrid->IsColShown( LABEL_COLUMN ) )
    {
        m_viewControlsGrid->SetColSize( DISPLAY_NAME_COLUMN, std::max( remainingWidth / 2, 60 ) );
        m_viewControlsGrid->SetColSize( LABEL_COLUMN, std::max( remainingWidth - ( remainingWidth / 2 ), 60 ) );
    }
    else if( m_viewControlsGrid->IsColShown( DISPLAY_NAME_COLUMN ) )
    {
        m_viewControlsGrid->SetColSize( DISPLAY_NAME_COLUMN, std::max( remainingWidth, 60 ) );
    }
    else if( m_viewControlsGrid->IsColShown( LABEL_COLUMN ) )
    {
        m_viewControlsGrid->SetColSize( LABEL_COLUMN, std::max( remainingWidth, 60 ) );
    }

    event.Skip();
}


void DIALOG_SYMBOL_FIELDS_TABLE::OnSaveAndContinue( wxCommandEvent& aEvent )
{
    if( TransferDataFromWindow() )
    {
        m_parent->SaveProject();
        ClearModify();
    }
}


void DIALOG_SYMBOL_FIELDS_TABLE::OnPageChanged( wxNotebookEvent& event )
{
    if( m_dataModel->GetColsCount() )
        PreviewRefresh();
}


void DIALOG_SYMBOL_FIELDS_TABLE::OnPreviewRefresh( wxCommandEvent& event )
{
    PreviewRefresh();
    syncBomFmtPresetSelection();
}


void DIALOG_SYMBOL_FIELDS_TABLE::PreviewRefresh()
{
    bool saveIncludeExcudedFromBOM = m_dataModel->GetIncludeExcludedFromBOM();

    m_dataModel->SetIncludeExcludedFromBOM( false );
    m_dataModel->RebuildRows();

    m_textOutput->SetValue( m_dataModel->Export( GetCurrentBomFmtSettings() ) );

    if( saveIncludeExcudedFromBOM )
    {
        m_dataModel->SetIncludeExcludedFromBOM( true );
        m_dataModel->RebuildRows();
    }
}


BOM_FMT_PRESET DIALOG_SYMBOL_FIELDS_TABLE::GetCurrentBomFmtSettings()
{
    BOM_FMT_PRESET current;

    current.name = m_cbBomFmtPresets->GetStringSelection();
    current.fieldDelimiter = m_textFieldDelimiter->GetValue();
    current.stringDelimiter = m_textStringDelimiter->GetValue();
    current.refDelimiter = m_textRefDelimiter->GetValue();
    current.refRangeDelimiter = m_textRefRangeDelimiter->GetValue();
    current.keepTabs = m_checkKeepTabs->GetValue();
    current.keepLineBreaks = m_checkKeepLineBreaks->GetValue();

    return current;
}


void DIALOG_SYMBOL_FIELDS_TABLE::ShowEditTab()
{
    m_nbPages->SetSelection( 0 );
}


void DIALOG_SYMBOL_FIELDS_TABLE::ShowExportTab()
{
    m_nbPages->SetSelection( 1 );
}


void DIALOG_SYMBOL_FIELDS_TABLE::OnOutputFileBrowseClicked( wxCommandEvent& event )
{
    // Build the absolute path of current output directory to preselect it in the file browser.
    wxString path = ExpandEnvVarSubstitutions( m_outputFileName->GetValue(), &Prj() );
    path = Prj().AbsolutePath( path );


    // Calculate the export filename
    wxFileName fn( Prj().AbsolutePath( m_parent->Schematic().GetFileName() ) );
    fn.SetExt( FILEEXT::CsvFileExtension );

    wxFileDialog saveDlg( this, _( "Bill of Materials Output File" ), path, fn.GetFullName(),
                          FILEEXT::CsvFileWildcard(), wxFD_SAVE | wxFD_OVERWRITE_PROMPT );

    if( saveDlg.ShowModal() == wxID_CANCEL )
        return;


    wxFileName file = wxFileName( saveDlg.GetPath() );
    wxString   defaultPath = fn.GetPathWithSep();

    if( IsOK( this, wxString::Format( _( "Do you want to use a path relative to\n'%s'?" ), defaultPath ) ) )
    {
        if( !file.MakeRelativeTo( defaultPath ) )
        {
            DisplayErrorMessage( this, _( "Cannot make path relative (target volume different from schematic "
                                          "file volume)!" ) );
        }
    }

    m_outputFileName->SetValue( file.GetFullPath() );
}


void DIALOG_SYMBOL_FIELDS_TABLE::OnSidebarToggle( wxCommandEvent& event )
{
    EESCHEMA_SETTINGS::PANEL_SYMBOL_FIELDS_TABLE& cfg = m_parent->eeconfig()->m_FieldEditorPanel;

    if( cfg.sidebar_collapsed )
    {
        cfg.sidebar_collapsed = false;
        m_splitterMainWindow->SplitVertically( m_leftPanel, m_rightPanel, cfg.sash_pos );
    }
    else
    {
        cfg.sash_pos = m_splitterMainWindow->GetSashPosition();

        cfg.sidebar_collapsed = true;
        m_splitterMainWindow->Unsplit( m_leftPanel );
    }

    setSideBarButtonLook( cfg.sidebar_collapsed );
}


void DIALOG_SYMBOL_FIELDS_TABLE::OnExport( wxCommandEvent& aEvent )
{
    if( m_dataModel->IsEdited() )
    {
        if( OKOrCancelDialog( nullptr, _( "Unsaved data" ),
                              _( "Changes have not yet been saved. Export unsaved data?" ), "",
                              _( "OK" ), _( "Cancel" ) )
            == wxID_CANCEL )
        {
            return;
        }
    }

    // Create output directory if it does not exist (also transform it in absolute form).
    // Bail if it fails.

    std::function<bool( wxString* )> textResolver =
            [&]( wxString* token ) -> bool
            {
                SCHEMATIC& schematic = m_parent->Schematic();

                // Handles m_board->GetTitleBlock() *and* m_board->GetProject()
                return schematic.ResolveTextVar( &schematic.CurrentSheet(), token, 0 );
            };

    wxString path = m_outputFileName->GetValue();

    if( path.IsEmpty() )
    {
        DisplayError( this, _( "No output file specified in Export tab." ) );
        return;
    }

    path = ExpandTextVars( path, &textResolver );
    path = ExpandEnvVarSubstitutions( path, nullptr );

    wxFileName outputFile = wxFileName::FileName( path );
    wxString msg;

    if( !EnsureFileDirectoryExists( &outputFile, Prj().AbsolutePath( m_parent->Schematic().GetFileName() ),
                                    &NULL_REPORTER::GetInstance() ) )
    {
        msg.Printf( _( "Could not open/create path '%s'." ), outputFile.GetPath() );
        DisplayError( this, msg );
        return;
    }

    wxFFile out( outputFile.GetFullPath(), "wb" );

    if( !out.IsOpened() )
    {
        msg.Printf( _( "Could not create BOM output '%s'." ), outputFile.GetFullPath() );
        DisplayError( this, msg );
        return;
    }

    PreviewRefresh();

    if( !out.Write( m_textOutput->GetValue() ) )
    {
        msg.Printf( _( "Could not write BOM output '%s'." ), outputFile.GetFullPath() );
        DisplayError( this, msg );
        return;
    }

    // close the file before we tell the user it's done with the info modal :workflow meme:
    out.Close();
    msg.Printf( _( "Wrote BOM output to '%s'" ), outputFile.GetFullPath() );
    DisplayInfoMessage( this, msg );
}


void DIALOG_SYMBOL_FIELDS_TABLE::OnCancel( wxCommandEvent& aEvent )
{
    if( m_job )
        EndModal( wxID_CANCEL );
    else
        Close();
}


void DIALOG_SYMBOL_FIELDS_TABLE::OnOk( wxCommandEvent& aEvent )
{
    TransferDataFromWindow();

    if( m_job )
    {
        m_job->SetConfiguredOutputPath( m_outputFileName->GetValue() );

        if( m_currentBomFmtPreset )
            m_job->m_bomFmtPresetName = m_currentBomFmtPreset->name;
        else
            m_job->m_bomFmtPresetName = wxEmptyString;

        if( m_currentBomPreset )
            m_job->m_bomPresetName = m_currentBomPreset->name;
        else
            m_job->m_bomPresetName = wxEmptyString;

        BOM_FMT_PRESET fmtSettings = GetCurrentBomFmtSettings();
        m_job->m_fieldDelimiter = fmtSettings.fieldDelimiter;
        m_job->m_stringDelimiter = fmtSettings.stringDelimiter;
        m_job->m_refDelimiter = fmtSettings.refDelimiter;
        m_job->m_refRangeDelimiter = fmtSettings.refRangeDelimiter;
        m_job->m_keepTabs = fmtSettings.keepTabs;
        m_job->m_keepLineBreaks = fmtSettings.keepLineBreaks;

        BOM_PRESET presetFields = m_dataModel->GetBomSettings();
        m_job->m_sortAsc = presetFields.sortAsc;
        m_job->m_excludeDNP = presetFields.excludeDNP;
        m_job->m_filterString = presetFields.filterString;
        m_job->m_sortField = presetFields.sortField;

        m_job->m_fieldsOrdered.clear();
        m_job->m_fieldsLabels.clear();
        m_job->m_fieldsGroupBy.clear();

        for( const BOM_FIELD& modelField : m_dataModel->GetFieldsOrdered() )
        {
            if( modelField.show )
                m_job->m_fieldsOrdered.emplace_back( modelField.name );
            else
                m_job->m_fieldsOrdered.emplace_back( wxT( "__" ) + modelField.name );

            m_job->m_fieldsLabels.emplace_back( modelField.label );

            if( modelField.groupBy )
                m_job->m_fieldsGroupBy.emplace_back( modelField.name );
        }

        wxString selectedVariant = getSelectedVariant();

        if( !selectedVariant.IsEmpty() )
            m_job->m_variantNames.push_back( selectedVariant );

        EndModal( wxID_OK );
    }
    else
    {
        Close();
    }
}


void DIALOG_SYMBOL_FIELDS_TABLE::OnClose( wxCloseEvent& aEvent )
{
    if( m_job )
    {
        aEvent.Skip();
        return;
    }

    m_grid->CommitPendingChanges( true );

    if( m_dataModel->IsEdited() && aEvent.CanVeto() )
    {
        if( !HandleUnsavedChanges( this, _( "Save changes?" ),
                                   [&]() -> bool
                                   {
                                       return TransferDataFromWindow();
                                   } ) )
        {
            aEvent.Veto();
            return;
        }
    }

    // Stop listening to schematic events
    m_parent->Schematic().RemoveListener( this );
    m_parent->ClearFocus();

    wxCommandEvent* evt = new wxCommandEvent( EDA_EVT_CLOSE_DIALOG_SYMBOL_FIELDS_TABLE, wxID_ANY );

    if( wxWindow* parent = GetParent() )
        wxQueueEvent( parent, evt );
}


std::vector<BOM_PRESET> DIALOG_SYMBOL_FIELDS_TABLE::GetUserBomPresets() const
{
    std::vector<BOM_PRESET> ret;

    for( const std::pair<const wxString, BOM_PRESET>& pair : m_bomPresets )
    {
        if( !pair.second.readOnly )
            ret.emplace_back( pair.second );
    }

    return ret;
}


void DIALOG_SYMBOL_FIELDS_TABLE::SetUserBomPresets( std::vector<BOM_PRESET>& aPresetList )
{
    // Reset to defaults
    loadDefaultBomPresets();

    for( const BOM_PRESET& preset : aPresetList )
    {
        if( m_bomPresets.count( preset.name ) )
            continue;

        m_bomPresets[preset.name] = preset;

        m_bomPresetMRU.Add( preset.name );
    }

    rebuildBomPresetsWidget();
}


void DIALOG_SYMBOL_FIELDS_TABLE::ApplyBomPreset( const wxString& aPresetName )
{
    updateBomPresetSelection( aPresetName );

    wxCommandEvent dummy;
    onBomPresetChanged( dummy );
}


void DIALOG_SYMBOL_FIELDS_TABLE::ApplyBomPreset( const BOM_PRESET& aPreset )
{
    if( m_bomPresets.count( aPreset.name ) )
        m_currentBomPreset = &m_bomPresets[aPreset.name];
    else
        m_currentBomPreset = nullptr;

    if( m_currentBomPreset && !m_currentBomPreset->readOnly )
        m_lastSelectedBomPreset = m_currentBomPreset;
    else
        m_lastSelectedBomPreset = nullptr;

    updateBomPresetSelection( aPreset.name );
    doApplyBomPreset( aPreset );
}


void DIALOG_SYMBOL_FIELDS_TABLE::loadDefaultBomPresets()
{
    m_bomPresets.clear();
    m_bomPresetMRU.clear();

    // Load the read-only defaults
    for( const BOM_PRESET& preset : BOM_PRESET::BuiltInPresets() )
    {
        m_bomPresets[preset.name] = preset;
        m_bomPresets[preset.name].readOnly = true;

        m_bomPresetMRU.Add( preset.name );
    }
}


void DIALOG_SYMBOL_FIELDS_TABLE::rebuildBomPresetsWidget()
{
    m_cbBomPresets->Clear();

    int idx = 0;
    int default_idx = 0;

    for( const auto& [presetName, preset] : m_bomPresets )
    {
        m_cbBomPresets->Append( wxGetTranslation( presetName ), (void*) &preset );

        if( presetName == BOM_PRESET::DefaultEditing().name )
            default_idx = idx;

        idx++;
    }

    m_cbBomPresets->Append( wxT( "---" ) );
    m_cbBomPresets->Append( _( "Save preset..." ) );
    m_cbBomPresets->Append( _( "Delete preset..." ) );

    // At least the built-in presets should always be present
    wxASSERT( !m_bomPresets.empty() );

    m_cbBomPresets->SetSelection( default_idx );
    m_currentBomPreset = static_cast<BOM_PRESET*>( m_cbBomPresets->GetClientData( default_idx ) );
}


void DIALOG_SYMBOL_FIELDS_TABLE::syncBomPresetSelection()
{
    BOM_PRESET current = m_dataModel->GetBomSettings();

    auto it = std::find_if( m_bomPresets.begin(), m_bomPresets.end(),
            [&]( const std::pair<const wxString, BOM_PRESET>& aPair )
            {
                const BOM_PRESET& preset = aPair.second;

                // Check the simple settings first
                if( !( preset.sortAsc == current.sortAsc
                       && preset.filterString == current.filterString
                       && preset.groupSymbols == current.groupSymbols
                       && preset.excludeDNP == current.excludeDNP
                       && preset.includeExcludedFromBOM == current.includeExcludedFromBOM ) )
                {
                    return false;
                }

                // We should compare preset.name and current.name.  Unfortunately current.name is
                // empty because m_dataModel->GetBomSettings() does not store the .name member.
                // So use sortField member as a (not very efficient) auxiliary filter.
                // As a further complication, sortField can be translated in m_bomPresets list, so
                // current.sortField needs to be translated.
                // Probably this not efficient and error prone test should be removed (JPC).
                if( preset.sortField != wxGetTranslation( current.sortField ) )
                    return false;

                // Only compare shown or grouped fields
                std::vector<BOM_FIELD> A, B;

                for( const BOM_FIELD& field : preset.fieldsOrdered )
                {
                    if( field.show || field.groupBy )
                        A.emplace_back( field );
                }

                for( const BOM_FIELD& field : current.fieldsOrdered )
                {
                    if( field.show || field.groupBy )
                        B.emplace_back( field );
                }

                return A == B;
            } );

    if( it != m_bomPresets.end() )
    {
        // Select the right m_cbBomPresets item.
        // but these items are translated if they are predefined items.
        bool     do_translate = it->second.readOnly;
        wxString text = do_translate ? wxGetTranslation( it->first ) : it->first;
        m_cbBomPresets->SetStringSelection( text );
    }
    else
    {
        m_cbBomPresets->SetSelection( m_cbBomPresets->GetCount() - 3 ); // separator
    }

    m_currentBomPreset = static_cast<BOM_PRESET*>( m_cbBomPresets->GetClientData( m_cbBomPresets->GetSelection() ) );
}


void DIALOG_SYMBOL_FIELDS_TABLE::updateBomPresetSelection( const wxString& aName )
{
    // Look at m_userBomPresets to know if aName is a read only preset, or a user preset.
    // Read-only presets have translated names in UI, so we have to use a translated name
    // in UI selection.  But for a user preset name we search for the untranslated aName.
    wxString ui_label = aName;

    for( const auto& [presetName, preset] : m_bomPresets )
    {
        if( presetName == aName )
        {
            if( preset.readOnly == true )
                ui_label = wxGetTranslation( aName );

            break;
        }
    }

    int idx = m_cbBomPresets->FindString( ui_label );

    if( idx >= 0 && m_cbBomPresets->GetSelection() != idx )
    {
        m_cbBomPresets->SetSelection( idx );
        m_currentBomPreset = static_cast<BOM_PRESET*>( m_cbBomPresets->GetClientData( idx ) );
    }
    else if( idx < 0 )
    {
        m_cbBomPresets->SetSelection( m_cbBomPresets->GetCount() - 3 ); // separator
    }
}


void DIALOG_SYMBOL_FIELDS_TABLE::onBomPresetChanged( wxCommandEvent& aEvent )
{
    int count = m_cbBomPresets->GetCount();
    int index = m_cbBomPresets->GetSelection();

    auto resetSelection =
            [&]()
            {
                if( m_currentBomPreset )
                    m_cbBomPresets->SetStringSelection( m_currentBomPreset->name );
                else
                    m_cbBomPresets->SetSelection( m_cbBomPresets->GetCount() - 3 );
            };

    if( index == count - 3 )
    {
        // Separator: reject the selection
        resetSelection();
        return;
    }
    else if( index == count - 2 )
    {
        // Save current state to new preset
        wxString name;

        if( m_lastSelectedBomPreset )
            name = m_lastSelectedBomPreset->name;

        wxTextEntryDialog dlg( this, _( "BOM preset name:" ), _( "Save BOM Preset" ), name );

        if( dlg.ShowModal() != wxID_OK )
        {
            resetSelection();
            return;
        }

        name = dlg.GetValue();
        bool exists = m_bomPresets.count( name );

        if( !exists )
        {
            m_bomPresets[name] = m_dataModel->GetBomSettings();
            m_bomPresets[name].readOnly = false;
            m_bomPresets[name].name = name;
        }

        BOM_PRESET* preset = &m_bomPresets[name];

        if( !exists )
        {
            index = m_cbBomPresets->Insert( name, index - 1, static_cast<void*>( preset ) );
        }
        else if( preset->readOnly )
        {
            wxMessageBox( _( "Default presets cannot be modified.\nPlease use a different name." ),
                          _( "Error" ), wxOK | wxICON_ERROR, this );
            resetSelection();
            return;
        }
        else
        {
            // Ask the user if they want to overwrite the existing preset
            if( !IsOK( this, _( "Overwrite existing preset?" ) ) )
            {
                resetSelection();
                return;
            }

            *preset = m_dataModel->GetBomSettings();
            preset->name = name;

            index = m_cbBomPresets->FindString( name );

            if( m_bomPresetMRU.Index( name ) != wxNOT_FOUND )
                m_bomPresetMRU.Remove( name );
        }

        m_currentBomPreset = preset;
        m_cbBomPresets->SetSelection( index );
        m_bomPresetMRU.Insert( name, 0 );

        return;
    }
    else if( index == count - 1 )
    {
        // Delete a preset
        wxArrayString              headers;
        std::vector<wxArrayString> items;

        headers.Add( _( "Presets" ) );

        for( const auto& [name, preset] : m_bomPresets )
        {
            if( !preset.readOnly )
            {
                wxArrayString item;
                item.Add( name );
                items.emplace_back( item );
            }
        }

        EDA_LIST_DIALOG dlg( this, _( "Delete Preset" ), headers, items );
        dlg.SetListLabel( _( "Select preset:" ) );

        if( dlg.ShowModal() == wxID_OK )
        {
            wxString presetName = dlg.GetTextSelection();
            int      idx = m_cbBomPresets->FindString( presetName );

            if( idx != wxNOT_FOUND )
            {
                m_bomPresets.erase( presetName );

                m_cbBomPresets->Delete( idx );
                m_currentBomPreset = nullptr;
            }

            if( m_bomPresetMRU.Index( presetName ) != wxNOT_FOUND )
                m_bomPresetMRU.Remove( presetName );
        }

        resetSelection();
        return;
    }

    BOM_PRESET* preset = static_cast<BOM_PRESET*>( m_cbBomPresets->GetClientData( index ) );
    m_currentBomPreset = preset;

    m_lastSelectedBomPreset = ( !preset || preset->readOnly ) ? nullptr : preset;

    if( preset )
    {
        doApplyBomPreset( *preset );
        syncBomPresetSelection();
        m_currentBomPreset = preset;

        if( !m_currentBomPreset->name.IsEmpty() )
        {
            if( m_bomPresetMRU.Index( preset->name ) != wxNOT_FOUND )
                m_bomPresetMRU.Remove( preset->name );

            m_bomPresetMRU.Insert( preset->name, 0 );
        }
    }
}


void DIALOG_SYMBOL_FIELDS_TABLE::doApplyBomPreset( const BOM_PRESET& aPreset )
{
    // Disable rebuilds while we're applying the preset otherwise we'll be
    // rebuilding the model constantly while firing off wx events
    m_dataModel->DisableRebuilds();

    // Basically, we apply the BOM preset to the data model and then
    // update our UI to reflect resulting the data model state, not the preset.
    m_dataModel->ApplyBomPreset( aPreset, m_parent->Schematic().GetCurrentVariant() );

    // BOM Presets can add, but not remove, columns, so make sure the view controls
    // grid has all of them before starting
    for( int i = 0; i < m_dataModel->GetColsCount(); i++ )
    {
        const wxString& fieldName( m_dataModel->GetColFieldName( i ) );
        bool            found = false;

        for( int j = 0; j < m_viewControlsDataModel->GetNumberRows(); j++ )
        {
            if( m_viewControlsDataModel->GetCanonicalFieldName( j ) == fieldName )
            {
                found = true;
                break;
            }
        }

        // Properties like label, etc. will be added in the next loop
        if( !found )
            AddField( fieldName, GetGeneratedFieldDisplayName( fieldName ), false, false );
    }

    // Sync all fields
    for( int i = 0; i < m_viewControlsDataModel->GetNumberRows(); i++ )
    {
        const wxString& fieldName( m_viewControlsDataModel->GetCanonicalFieldName( i ) );
        int             col = m_dataModel->GetFieldNameCol( fieldName );

        if( col == -1 )
        {
            wxASSERT_MSG( true, "Fields control has a field not found in the data model." );
            continue;
        }

        EESCHEMA_SETTINGS* cfg = m_parent->eeconfig();
        std::string        fieldNameStr( fieldName.ToUTF8() );

        // Set column labels
        const wxString& label = m_dataModel->GetColLabelValue( col );
        m_viewControlsDataModel->SetValue( i, LABEL_COLUMN, label );
        m_grid->SetColLabelValue( col, label );

        if( cfg->m_FieldEditorPanel.field_widths.count( fieldNameStr ) )
            m_grid->SetColSize( col, cfg->m_FieldEditorPanel.field_widths.at( fieldNameStr ) );

        // Set shown columns
        bool show = m_dataModel->GetShowColumn( col );
        m_viewControlsDataModel->SetValueAsBool( i, SHOW_FIELD_COLUMN, show );

        if( show )
            m_grid->ShowCol( col );
        else
            m_grid->HideCol( col );

        // Set grouped columns
        bool groupBy = m_dataModel->GetGroupColumn( col );
        m_viewControlsDataModel->SetValueAsBool( i, GROUP_BY_COLUMN, groupBy );
    }

    m_grid->SetSortingColumn( m_dataModel->GetSortCol(), m_dataModel->GetSortAsc() );
    m_groupSymbolsBox->SetValue( m_dataModel->GetGroupingEnabled() );
    m_filter->ChangeValue( m_dataModel->GetFilter() );

    SetupAllColumnProperties();

    // This will rebuild all rows and columns in the model such that the order
    // and labels are right, then we refresh the shown grid data to match
    m_dataModel->EnableRebuilds();
    m_dataModel->RebuildRows();

    if( m_nbPages->GetSelection() == 1 )
        PreviewRefresh();
    else
        m_grid->ForceRefresh();
}


std::vector<BOM_FMT_PRESET> DIALOG_SYMBOL_FIELDS_TABLE::GetUserBomFmtPresets() const
{
    std::vector<BOM_FMT_PRESET> ret;

    for( const auto& [name, preset] : m_bomFmtPresets )
    {
        if( !preset.readOnly )
            ret.emplace_back( preset );
    }

    return ret;
}


void DIALOG_SYMBOL_FIELDS_TABLE::SetUserBomFmtPresets( std::vector<BOM_FMT_PRESET>& aPresetList )
{
    // Reset to defaults
    loadDefaultBomFmtPresets();

    for( const BOM_FMT_PRESET& preset : aPresetList )
    {
        if( m_bomFmtPresets.count( preset.name ) )
            continue;

        m_bomFmtPresets[preset.name] = preset;

        m_bomFmtPresetMRU.Add( preset.name );
    }

    rebuildBomFmtPresetsWidget();
}


void DIALOG_SYMBOL_FIELDS_TABLE::ApplyBomFmtPreset( const wxString& aPresetName )
{
    updateBomFmtPresetSelection( aPresetName );

    wxCommandEvent dummy;
    onBomFmtPresetChanged( dummy );
}


void DIALOG_SYMBOL_FIELDS_TABLE::ApplyBomFmtPreset( const BOM_FMT_PRESET& aPreset )
{
    m_currentBomFmtPreset = nullptr;
    m_lastSelectedBomFmtPreset = nullptr;

    if( m_bomFmtPresets.count( aPreset.name ) )
        m_currentBomFmtPreset = &m_bomFmtPresets[aPreset.name];

    if( m_currentBomFmtPreset && !m_currentBomFmtPreset->readOnly )
        m_lastSelectedBomFmtPreset =m_currentBomFmtPreset;

    updateBomFmtPresetSelection( aPreset.name );
    doApplyBomFmtPreset( aPreset );
}


void DIALOG_SYMBOL_FIELDS_TABLE::loadDefaultBomFmtPresets()
{
    m_bomFmtPresets.clear();
    m_bomFmtPresetMRU.clear();

    // Load the read-only defaults
    for( const BOM_FMT_PRESET& preset : BOM_FMT_PRESET::BuiltInPresets() )
    {
        m_bomFmtPresets[preset.name] = preset;
        m_bomFmtPresets[preset.name].readOnly = true;

        m_bomFmtPresetMRU.Add( preset.name );
    }
}


void DIALOG_SYMBOL_FIELDS_TABLE::rebuildBomFmtPresetsWidget()
{
    m_cbBomFmtPresets->Clear();

    int idx = 0;
    int default_idx = 0;

    for( const auto& [presetName, preset] : m_bomFmtPresets )
    {
        m_cbBomFmtPresets->Append( wxGetTranslation( presetName ), (void*) &preset );

        if( presetName == BOM_FMT_PRESET::CSV().name )
            default_idx = idx;

        idx++;
    }

    m_cbBomFmtPresets->Append( wxT( "---" ) );
    m_cbBomFmtPresets->Append( _( "Save preset..." ) );
    m_cbBomFmtPresets->Append( _( "Delete preset..." ) );

    // At least the built-in presets should always be present
    wxASSERT( !m_bomFmtPresets.empty() );

    m_cbBomFmtPresets->SetSelection( default_idx );
    m_currentBomFmtPreset = static_cast<BOM_FMT_PRESET*>( m_cbBomFmtPresets->GetClientData( default_idx ) );
}


void DIALOG_SYMBOL_FIELDS_TABLE::syncBomFmtPresetSelection()
{
    BOM_FMT_PRESET current = GetCurrentBomFmtSettings();

    auto it = std::find_if( m_bomFmtPresets.begin(), m_bomFmtPresets.end(),
                            [&]( const std::pair<const wxString, BOM_FMT_PRESET>& aPair )
                            {
                                return ( aPair.second.fieldDelimiter == current.fieldDelimiter
                                         && aPair.second.stringDelimiter == current.stringDelimiter
                                         && aPair.second.refDelimiter == current.refDelimiter
                                         && aPair.second.refRangeDelimiter == current.refRangeDelimiter
                                         && aPair.second.keepTabs == current.keepTabs
                                         && aPair.second.keepLineBreaks == current.keepLineBreaks );
                            } );

    if( it != m_bomFmtPresets.end() )
    {
        // Select the right m_cbBomFmtPresets item.
        // but these items are translated if they are predefined items.
        bool     do_translate = it->second.readOnly;
        wxString text = do_translate ? wxGetTranslation( it->first ) : it->first;

        m_cbBomFmtPresets->SetStringSelection( text );
    }
    else
    {
        m_cbBomFmtPresets->SetSelection( m_cbBomFmtPresets->GetCount() - 3 ); // separator
    }

    int idx = m_cbBomFmtPresets->GetSelection();
    m_currentBomFmtPreset = static_cast<BOM_FMT_PRESET*>( m_cbBomFmtPresets->GetClientData( idx ) );
}


void DIALOG_SYMBOL_FIELDS_TABLE::updateBomFmtPresetSelection( const wxString& aName )
{
    // look at m_userBomFmtPresets to know if aName is a read only preset, or a user preset.
    // Read only presets have translated names in UI, so we have to use a translated name in UI selection.
    // But for a user preset name we should search for aName (not translated)
    wxString ui_label = aName;

    for( const auto& [presetName, preset] : m_bomFmtPresets )
    {
        if( presetName == aName )
        {
            if( preset.readOnly )
                ui_label = wxGetTranslation( aName );

            break;
        }
    }

    int idx = m_cbBomFmtPresets->FindString( ui_label );

    if( idx >= 0 && m_cbBomFmtPresets->GetSelection() != idx )
    {
        m_cbBomFmtPresets->SetSelection( idx );
        m_currentBomFmtPreset = static_cast<BOM_FMT_PRESET*>( m_cbBomFmtPresets->GetClientData( idx ) );
    }
    else if( idx < 0 )
    {
        m_cbBomFmtPresets->SetSelection( m_cbBomFmtPresets->GetCount() - 3 ); // separator
    }
}


void DIALOG_SYMBOL_FIELDS_TABLE::onBomFmtPresetChanged( wxCommandEvent& aEvent )
{
    int count = m_cbBomFmtPresets->GetCount();
    int index = m_cbBomFmtPresets->GetSelection();

    auto resetSelection =
            [&]()
            {
                if( m_currentBomFmtPreset )
                    m_cbBomFmtPresets->SetStringSelection( m_currentBomFmtPreset->name );
                else
                    m_cbBomFmtPresets->SetSelection( m_cbBomFmtPresets->GetCount() - 3 );
            };

    if( index == count - 3 )
    {
        // Separator: reject the selection
        resetSelection();
        return;
    }
    else if( index == count - 2 )
    {
        // Save current state to new preset
        wxString name;

        if( m_lastSelectedBomFmtPreset )
            name = m_lastSelectedBomFmtPreset->name;

        wxTextEntryDialog dlg( this, _( "BOM preset name:" ), _( "Save BOM Preset" ), name );

        if( dlg.ShowModal() != wxID_OK )
        {
            resetSelection();
            return;
        }

        name = dlg.GetValue();
        bool exists = m_bomFmtPresets.count( name );

        if( !exists )
        {
            m_bomFmtPresets[name] = GetCurrentBomFmtSettings();
            m_bomFmtPresets[name].readOnly = false;
            m_bomFmtPresets[name].name = name;
        }

        BOM_FMT_PRESET* preset = &m_bomFmtPresets[name];

        if( !exists )
        {
            index = m_cbBomFmtPresets->Insert( name, index - 1, static_cast<void*>( preset ) );
        }
        else if( preset->readOnly )
        {
            wxMessageBox( _( "Default presets cannot be modified.\nPlease use a different name." ),
                          _( "Error" ), wxOK | wxICON_ERROR, this );
            resetSelection();
            return;
        }
        else
        {
            // Ask the user if they want to overwrite the existing preset
            if( !IsOK( this, _( "Overwrite existing preset?" ) ) )
            {
                resetSelection();
                return;
            }

            *preset = GetCurrentBomFmtSettings();
            preset->name = name;

            index = m_cbBomFmtPresets->FindString( name );

            if( m_bomFmtPresetMRU.Index( name ) != wxNOT_FOUND )
                m_bomFmtPresetMRU.Remove( name );
        }

        m_currentBomFmtPreset = preset;
        m_cbBomFmtPresets->SetSelection( index );
        m_bomFmtPresetMRU.Insert( name, 0 );

        return;
    }
    else if( index == count - 1 )
    {
        // Delete a preset
        wxArrayString              headers;
        std::vector<wxArrayString> items;

        headers.Add( _( "Presets" ) );

        for( std::pair<const wxString, BOM_FMT_PRESET>& pair : m_bomFmtPresets )
        {
            if( !pair.second.readOnly )
            {
                wxArrayString item;
                item.Add( pair.first );
                items.emplace_back( item );
            }
        }

        EDA_LIST_DIALOG dlg( this, _( "Delete Preset" ), headers, items );
        dlg.SetListLabel( _( "Select preset:" ) );

        if( dlg.ShowModal() == wxID_OK )
        {
            wxString presetName = dlg.GetTextSelection();
            int      idx = m_cbBomFmtPresets->FindString( presetName );

            if( idx != wxNOT_FOUND )
            {
                m_bomFmtPresets.erase( presetName );

                m_cbBomFmtPresets->Delete( idx );
                m_currentBomFmtPreset = nullptr;
            }

            if( m_bomFmtPresetMRU.Index( presetName ) != wxNOT_FOUND )
                m_bomFmtPresetMRU.Remove( presetName );
        }

        resetSelection();
        return;
    }

    auto* preset = static_cast<BOM_FMT_PRESET*>( m_cbBomFmtPresets->GetClientData( index ) );
    m_currentBomFmtPreset = preset;

    m_lastSelectedBomFmtPreset = ( !preset || preset->readOnly ) ? nullptr : preset;

    if( preset )
    {
        doApplyBomFmtPreset( *preset );
        syncBomFmtPresetSelection();
        m_currentBomFmtPreset = preset;

        if( !m_currentBomFmtPreset->name.IsEmpty() )
        {
            if( m_bomFmtPresetMRU.Index( preset->name ) != wxNOT_FOUND )
                m_bomFmtPresetMRU.Remove( preset->name );

            m_bomFmtPresetMRU.Insert( preset->name, 0 );
        }
    }
}


void DIALOG_SYMBOL_FIELDS_TABLE::doApplyBomFmtPreset( const BOM_FMT_PRESET& aPreset )
{
    m_textFieldDelimiter->ChangeValue( aPreset.fieldDelimiter );
    m_textStringDelimiter->ChangeValue( aPreset.stringDelimiter );
    m_textRefDelimiter->ChangeValue( aPreset.refDelimiter );
    m_textRefRangeDelimiter->ChangeValue( aPreset.refRangeDelimiter );
    m_checkKeepTabs->SetValue( aPreset.keepTabs );
    m_checkKeepLineBreaks->SetValue( aPreset.keepLineBreaks );

    // Refresh the preview if that's the current page
    if( m_nbPages->GetSelection() == 1 )
        PreviewRefresh();
}


void DIALOG_SYMBOL_FIELDS_TABLE::savePresetsToSchematic()
{
    bool modified = false;

    // Save our BOM presets
    std::vector<BOM_PRESET> presets;

    for( const auto& [name, preset] : m_bomPresets )
    {
        if( !preset.readOnly )
            presets.emplace_back( preset );
    }

    if( m_schSettings.m_BomPresets != presets )
    {
        modified = true;
        m_schSettings.m_BomPresets = presets;
    }

    if( m_schSettings.m_BomSettings != m_dataModel->GetBomSettings() && !m_job )
    {
        modified = true;
        m_schSettings.m_BomSettings = m_dataModel->GetBomSettings();
    }

    // Save our BOM Format presets
    std::vector<BOM_FMT_PRESET> fmts;

    for( const auto& [name, preset] : m_bomFmtPresets )
    {
        if( !preset.readOnly )
            fmts.emplace_back( preset );
    }

    if( m_schSettings.m_BomFmtPresets != fmts )
    {
        modified = true;
        m_schSettings.m_BomFmtPresets = fmts;
    }

    if( m_schSettings.m_BomFmtSettings != GetCurrentBomFmtSettings() && !m_job )
    {
        modified = true;
        m_schSettings.m_BomFmtSettings = GetCurrentBomFmtSettings();
    }

    if( modified )
        m_parent->OnModify();
}


void DIALOG_SYMBOL_FIELDS_TABLE::OnSchItemsAdded( SCHEMATIC& aSch, std::vector<SCH_ITEM*>& aSchItem )
{
    SCH_REFERENCE_LIST allRefs;
    m_parent->Schematic().Hierarchy().GetSymbols( allRefs );

    for( SCH_ITEM* item : aSchItem )
    {
        if( item->Type() == SCH_SYMBOL_T )
        {
            SCH_SYMBOL* symbol = static_cast<SCH_SYMBOL*>( item );

            // Don't add power symbols
            if( !symbol->IsMissingLibSymbol() && symbol->IsPower() )
                continue;

            // Add all fields again in case this symbol has a new one
            for( SCH_FIELD& field : symbol->GetFields() )
                AddField( field.GetCanonicalName(), field.GetName(), true, false, true );

            m_dataModel->AddReferences( getSymbolReferences( symbol, allRefs ) );
        }
        else if( item->Type() == SCH_SHEET_T )
        {
            std::set<SCH_SYMBOL*> symbols;
            SCH_REFERENCE_LIST refs = getSheetSymbolReferences( *static_cast<SCH_SHEET*>( item ) );

            for( SCH_REFERENCE& ref : refs )
                symbols.insert( ref.GetSymbol() );

            for( SCH_SYMBOL* symbol : symbols )
            {
                // Add all fields again in case this symbol has a new one
                for( SCH_FIELD& field : symbol->GetFields() )
                    AddField( field.GetCanonicalName(), field.GetName(), true, false, true );
            }

            m_dataModel->AddReferences( refs );
        }
    }

    DisableSelectionEvents();
    m_dataModel->RebuildRows();
    EnableSelectionEvents();
}


void DIALOG_SYMBOL_FIELDS_TABLE::OnSchItemsRemoved( SCHEMATIC& aSch, std::vector<SCH_ITEM*>& aSchItem )
{
    for( SCH_ITEM* item : aSchItem )
    {
        if( item->Type() == SCH_SYMBOL_T )
            m_dataModel->RemoveSymbol( *static_cast<SCH_SYMBOL*>( item ) );
        else if( item->Type() == SCH_SHEET_T )
            m_dataModel->RemoveReferences( getSheetSymbolReferences( *static_cast<SCH_SHEET*>( item ) ) );
    }

    DisableSelectionEvents();
    m_dataModel->RebuildRows();
    EnableSelectionEvents();
}


void DIALOG_SYMBOL_FIELDS_TABLE::OnSchItemsChanged( SCHEMATIC& aSch, std::vector<SCH_ITEM*>& aSchItem )
{
    SCH_REFERENCE_LIST allRefs;
    m_parent->Schematic().Hierarchy().GetSymbols( allRefs );

    for( SCH_ITEM* item : aSchItem )
    {
        if( item->Type() == SCH_SYMBOL_T )
        {
            SCH_SYMBOL* symbol = static_cast<SCH_SYMBOL*>( item );

            // Don't add power symbols
            if( !symbol->IsMissingLibSymbol() && symbol->IsPower() )
                continue;

            // Add all fields again in case this symbol has a new one
            for( SCH_FIELD& field : symbol->GetFields() )
                AddField( field.GetCanonicalName(), field.GetName(), true, false, true );

            m_dataModel->UpdateReferences( getSymbolReferences( symbol, allRefs ),
                                           m_parent->Schematic().GetCurrentVariant() );
        }
        else if( item->Type() == SCH_SHEET_T )
        {
            std::set<SCH_SYMBOL*> symbols;
            SCH_REFERENCE_LIST refs = getSheetSymbolReferences( *static_cast<SCH_SHEET*>( item ) );

            for( SCH_REFERENCE& ref : refs )
                symbols.insert( ref.GetSymbol() );

            for( SCH_SYMBOL* symbol : symbols )
            {
                // Add all fields again in case this symbol has a new one
                for( SCH_FIELD& field : symbol->GetFields() )
                    AddField( field.GetCanonicalName(), field.GetName(), true, false, true );
            }

            m_dataModel->UpdateReferences( refs, m_parent->Schematic().GetCurrentVariant() );
        }
    }

    DisableSelectionEvents();
    m_dataModel->RebuildRows();
    EnableSelectionEvents();
}


void DIALOG_SYMBOL_FIELDS_TABLE::OnSchSheetChanged( SCHEMATIC& aSch )
{
    m_dataModel->SetPath( aSch.CurrentSheet() );

    if( m_dataModel->GetScope() != FIELDS_EDITOR_GRID_DATA_MODEL::SCOPE::SCOPE_ALL )
    {
        DisableSelectionEvents();
        m_dataModel->RebuildRows();
        EnableSelectionEvents();
    }
}


void DIALOG_SYMBOL_FIELDS_TABLE::EnableSelectionEvents()
{
    m_grid->Connect( wxEVT_GRID_RANGE_SELECTED,
                     wxGridRangeSelectEventHandler( DIALOG_SYMBOL_FIELDS_TABLE::OnTableRangeSelected ),
                     nullptr, this );
}


void DIALOG_SYMBOL_FIELDS_TABLE::DisableSelectionEvents()
{
    m_grid->Disconnect( wxEVT_GRID_RANGE_SELECTED,
                        wxGridRangeSelectEventHandler( DIALOG_SYMBOL_FIELDS_TABLE::OnTableRangeSelected ),
                        nullptr, this );
}


SCH_REFERENCE_LIST DIALOG_SYMBOL_FIELDS_TABLE::getSymbolReferences( SCH_SYMBOL* aSymbol,
                                                                    SCH_REFERENCE_LIST& aCachedRefs )
{
    SCH_REFERENCE_LIST symbolRefs;

    for( size_t i = 0; i < aCachedRefs.GetCount(); i++ )
    {
        SCH_REFERENCE& ref = aCachedRefs[i];

        if( ref.GetSymbol() == aSymbol )
        {
            ref.Split(); // Figures out if we are annotated or not
            symbolRefs.AddItem( ref );
        }
    }

    return symbolRefs;
}


SCH_REFERENCE_LIST DIALOG_SYMBOL_FIELDS_TABLE::getSheetSymbolReferences( SCH_SHEET& aSheet )
{
    SCH_SHEET_LIST     allSheets = m_parent->Schematic().Hierarchy();
    SCH_REFERENCE_LIST sheetRefs;

    // We need to operate on all instances of the sheet
    for( const SCH_SHEET_INSTANCE& instance : aSheet.GetInstances() )
    {
        // For every sheet instance we need to get the current schematic sheet
        // instance that matches that particular sheet path from the root
        for( SCH_SHEET_PATH& basePath : allSheets )
        {
            if( basePath.Path() == instance.m_Path )
            {
                SCH_SHEET_PATH sheetPath = basePath;
                sheetPath.push_back( &aSheet );

                // Create a list of all sheets in this path, starting with the path
                // of the sheet that we just deleted, then all of its subsheets
                SCH_SHEET_LIST subSheets;
                subSheets.push_back( sheetPath );
                allSheets.GetSheetsWithinPath( subSheets, sheetPath );

                subSheets.GetSymbolsWithinPath( sheetRefs, sheetPath, false, false );
                break;
            }
        }
    }

    for( SCH_REFERENCE& ref : sheetRefs )
        ref.Split();

    return sheetRefs;
}


void DIALOG_SYMBOL_FIELDS_TABLE::onAddVariant( wxCommandEvent& aEvent )
{
    if( !m_parent->ShowAddVariantDialog() )
        return;

    wxArrayString ctrlContents;
    ctrlContents.Add( GetDefaultVariantName() );

    for( const wxString& variant : m_parent->Schematic().GetVariantNames() )
        ctrlContents.Add( variant );

    ctrlContents.Sort( SortVariantNames );
    m_variantListBox->Set( ctrlContents );

    wxString currentVariant = m_parent->Schematic().GetCurrentVariant();
    int      newSelection = m_variantListBox->FindString(
                currentVariant.IsEmpty() ? GetDefaultVariantName() : currentVariant );

    if( newSelection != wxNOT_FOUND )
        m_variantListBox->SetSelection( newSelection );

    updateVariantButtonStates();
}


void DIALOG_SYMBOL_FIELDS_TABLE::onDeleteVariant( wxCommandEvent& aEvent )
{
    int selection = m_variantListBox->GetSelection();

    // An empty or default selection cannot be deleted.
    if( ( selection == wxNOT_FOUND ) || ( selection == 0 ) )
    {
        m_parent->GetInfoBar()->ShowMessageFor( _( "Cannot delete the default variant." ),
                                                 10000, wxICON_ERROR );
        return;
    }

    wxString variantName = m_variantListBox->GetString( selection );
    m_variantListBox->Delete( selection );
    m_parent->Schematic().DeleteVariant( variantName );

    int newSelection = std::max( 0, selection - 1 );
    m_variantListBox->SetSelection( newSelection );

    wxString selectedVariant = getSelectedVariant();
    m_parent->SetCurrentVariant( selectedVariant );

    if( m_grid->CommitPendingChanges( true ) )
    {
        m_dataModel->SetCurrentVariant( selectedVariant );
        m_dataModel->UpdateReferences( m_dataModel->GetReferenceList(), selectedVariant );
        m_dataModel->RebuildRows();

        if( m_nbPages->GetSelection() == 1 )
            PreviewRefresh();
        else
            m_grid->ForceRefresh();
    }

    updateVariantButtonStates();
    m_parent->UpdateVariantSelectionCtrl( m_parent->Schematic().GetVariantNamesForUI() );
}


void DIALOG_SYMBOL_FIELDS_TABLE::onRenameVariant( wxCommandEvent& aEvent )
{
    int selection = m_variantListBox->GetSelection();

    // An empty or default selection cannot be renamed.
    if( ( selection == wxNOT_FOUND ) || ( selection == 0 ) )
    {
        m_parent->GetInfoBar()->ShowMessageFor( _( "Cannot rename the default variant." ),
                                                 10000, wxICON_ERROR );
        return;
    }

    wxString oldVariantName = m_variantListBox->GetString( selection );

    wxTextEntryDialog dlg( this, _( "Enter new variant name:" ), _( "Rename Variant" ),
                           oldVariantName, wxOK | wxCANCEL | wxCENTER );

    if( dlg.ShowModal() == wxID_CANCEL )
        return;

    wxString newVariantName = dlg.GetValue().Trim().Trim( false );

    // Empty name is not allowed.
    if( newVariantName.IsEmpty() )
    {
        m_parent->GetInfoBar()->ShowMessageFor( _( "Variant name cannot be empty." ),
                                                 10000, wxICON_ERROR );
        return;
    }

    // Reserved name is not allowed (case-insensitive).
    if( newVariantName.CmpNoCase( GetDefaultVariantName() ) == 0 )
    {
        m_parent->GetInfoBar()->ShowMessageFor( wxString::Format( _( "'%s' is a reserved variant name." ),
                                                                  GetDefaultVariantName() ),
                                                10000, wxICON_ERROR );
        return;
    }

    // Same name (exact match) - nothing to do
    if( newVariantName == oldVariantName )
        return;

    // Duplicate name is not allowed (case-insensitive).
    for( const wxString& existingName : m_parent->Schematic().GetVariantNames() )
    {
        if( existingName.CmpNoCase( newVariantName ) == 0
            && existingName.CmpNoCase( oldVariantName ) != 0 )
        {
            m_parent->GetInfoBar()->ShowMessageFor( wxString::Format( _( "Variant '%s' already exists." ),
                                                                      existingName ),
                                                    0000, wxICON_ERROR );
            return;
        }
    }

    m_parent->Schematic().RenameVariant( oldVariantName, newVariantName );

    wxArrayString ctrlContents = m_variantListBox->GetStrings();
    ctrlContents.Remove( oldVariantName );
    ctrlContents.Add( newVariantName );
    ctrlContents.Sort( SortVariantNames );
    m_variantListBox->Set( ctrlContents );

    int newSelection = m_variantListBox->FindString( newVariantName );

    if( newSelection != wxNOT_FOUND )
        m_variantListBox->SetSelection( newSelection );

    updateVariantButtonStates();
    m_parent->UpdateVariantSelectionCtrl( m_parent->Schematic().GetVariantNamesForUI() );
}


void DIALOG_SYMBOL_FIELDS_TABLE::onCopyVariant( wxCommandEvent& aEvent )
{
    int selection = m_variantListBox->GetSelection();

    // An empty or default selection cannot be copied.
    if( ( selection == wxNOT_FOUND ) || ( selection == 0 ) )
    {
        m_parent->GetInfoBar()->ShowMessageFor( _( "Cannot copy the default variant." ),
                                                 10000, wxICON_ERROR );
        return;
    }

    wxString sourceVariantName = m_variantListBox->GetString( selection );

    wxTextEntryDialog dlg( this, _( "Enter name for the copied variant:" ), _( "Copy Variant" ),
                           sourceVariantName + wxS( "_copy" ), wxOK | wxCANCEL | wxCENTER );

    if( dlg.ShowModal() == wxID_CANCEL )
        return;

    wxString newVariantName = dlg.GetValue().Trim().Trim( false );

    // Empty name is not allowed.
    if( newVariantName.IsEmpty() )
    {
        m_parent->GetInfoBar()->ShowMessageFor( _( "Variant name cannot be empty." ),
                                                 10000, wxICON_ERROR );
        return;
    }

    // Duplicate name is not allowed.
    if( m_variantListBox->FindString( newVariantName ) != wxNOT_FOUND )
    {
        m_parent->GetInfoBar()->ShowMessageFor( wxString::Format( _( "Variant '%s' already exists." ),
                                                                  newVariantName ),
                                                10000, wxICON_ERROR );
        return;
    }

    m_parent->Schematic().CopyVariant( sourceVariantName, newVariantName );

    wxArrayString ctrlContents = m_variantListBox->GetStrings();
    ctrlContents.Add( newVariantName );
    ctrlContents.Sort( SortVariantNames );
    m_variantListBox->Set( ctrlContents );

    int newSelection = m_variantListBox->FindString( newVariantName );

    if( newSelection != wxNOT_FOUND )
        m_variantListBox->SetSelection( newSelection );

    updateVariantButtonStates();
    m_parent->UpdateVariantSelectionCtrl( m_parent->Schematic().GetVariantNamesForUI() );
}


void DIALOG_SYMBOL_FIELDS_TABLE::onVariantSelectionChange( wxCommandEvent& aEvent )
{
    wxString currentVariant;
    wxString selectedVariant = getSelectedVariant();

    updateVariantButtonStates();

    if( m_parent )
    {
        currentVariant = m_parent->Schematic().GetCurrentVariant();

        if( currentVariant != selectedVariant )
            m_parent->SetCurrentVariant( selectedVariant );
    }

    if( currentVariant != selectedVariant )
    {
        if( m_grid->CommitPendingChanges( true ) )
        {
            // Update the data model's current variant for field highlighting
            m_dataModel->SetCurrentVariant( selectedVariant );

            m_dataModel->UpdateReferences( m_dataModel->GetReferenceList(), selectedVariant );
            m_dataModel->RebuildRows();

            if( m_nbPages->GetSelection() == 1 )
                PreviewRefresh();
            else
                m_grid->ForceRefresh();

            syncBomFmtPresetSelection();
        }
        else
        {
        }
    }
}


void DIALOG_SYMBOL_FIELDS_TABLE::updateVariantButtonStates()
{
    int selection = m_variantListBox->GetSelection();

    // Copy, rename, and delete are only enabled for non-default variant selections
    bool canModify = ( selection != wxNOT_FOUND ) && ( selection != 0 );

    m_copyVariantButton->Enable( canModify );
    m_renameVariantButton->Enable( canModify );
    m_deleteVariantButton->Enable( canModify );
}


wxString DIALOG_SYMBOL_FIELDS_TABLE::getSelectedVariant() const
{
    wxString retv;

    int selection = m_variantListBox->GetSelection();

    if( ( selection == wxNOT_FOUND ) || ( m_variantListBox->GetString( selection ) == GetDefaultVariantName() ) )
        return retv;

    return m_variantListBox->GetString( selection );
}
