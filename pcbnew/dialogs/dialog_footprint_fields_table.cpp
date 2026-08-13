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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <advanced_config.h>
#include <common.h>
#include <base_units.h>
#include <bitmaps.h>
#include <confirm.h>
#include <eda_doc.h>
#include <wildcards_and_files_ext.h>
#include <pcbnew_settings.h>
#include <board_design_settings.h>
#include <grid_tricks.h>
#include <string_utils.h>
#include <template_fieldnames.h>
#include <kiface_base.h>
#include <pcb_edit_frame.h>
#include <widgets/wx_infobar.h>
#include <tools/board_editor_control.h>
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
#include "dialog_footprint_fields_table.h"
#include <footprint_fields_data_model.h>
#include <board_commit.h>
#include <project_pcb.h>
#include <jobs/job_export_bom.h>
#include <tool/tool_manager.h>
#include <tools/pcb_actions.h>
#include <tools/pcb_selection_tool.h>

wxDEFINE_EVENT( EDA_EVT_CLOSE_DIALOG_FOOTPRINT_FIELDS_TABLE, wxCommandEvent );

#ifdef __WXMAC__
#define COLUMN_MARGIN 4
#else
#define COLUMN_MARGIN 15
#endif

using SCOPE = FOOTPRINT_FIELDS_EDITOR_GRID_DATA_MODEL::SCOPE;


enum
{
    MYID_SELECT_FOOTPRINT = GRIDTRICKS_FIRST_CLIENT_ID,
    MYID_SHOW_DATASHEET,
    MYID_SET_VARIANT_FOOTPRINT,
    MYID_CLEAR_VARIANT_FOOTPRINT
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
    FIELDS_EDITOR_GRID_TRICKS( DIALOG_FOOTPRINT_FIELDS_TABLE* aParent, WX_GRID* aGrid,
                               VIEW_CONTROLS_GRID_DATA_MODEL*           aViewFieldsData,
                               FOOTPRINT_FIELDS_EDITOR_GRID_DATA_MODEL* aDataModel, EMBEDDED_FILES* aFiles ) :
            GRID_TRICKS( aGrid ),
            m_dlg( aParent ),
            m_viewControlsDataModel( aViewFieldsData ),
            m_dataModel( aDataModel ),
            m_files( aFiles )
    {}

protected:
    bool toggleCell( int aRow, int aCol, bool aPreserveSelection = false ) override
    {
        if( !m_grid->IsEditable() || m_dataModel->IsCellReadOnly( aRow, aCol ) )
            return false;

        return GRID_TRICKS::toggleCell( aRow, aCol, aPreserveSelection );
    }

    void showPopupMenu( wxMenu& menu, wxGridEvent& aEvent ) override
    {
        int col = m_grid->GetGridCursorCol();

        if( m_dataModel->GetColFieldName( col ) == GetCanonicalFieldName( FIELD_T::DATASHEET ) )
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

        if( event.GetId() == MYID_SHOW_DATASHEET )
        {
            wxString datasheet_uri = m_grid->GetCellValue( row, col );
            GetAssociatedDocument( m_dlg, datasheet_uri, &m_dlg->Prj(), nullptr, { m_files } );
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
    DIALOG_FOOTPRINT_FIELDS_TABLE*           m_dlg;
    VIEW_CONTROLS_GRID_DATA_MODEL*           m_viewControlsDataModel;
    FOOTPRINT_FIELDS_EDITOR_GRID_DATA_MODEL* m_dataModel;
    EMBEDDED_FILES*                          m_files;
};


DIALOG_FOOTPRINT_FIELDS_TABLE::DIALOG_FOOTPRINT_FIELDS_TABLE( PCB_EDIT_FRAME* parent, JOB_EXPORT_BOM* aJob ) :
        DIALOG_FIELDS_TABLE( parent, parent->GetPcbNewSettings()->m_FieldEditorPanel ),
        m_parent( parent ),
        m_boardSettings( parent->GetBoard()->GetDesignSettings() ),
        m_job( aJob )
{
    // Get all footprints from the list of board sheets
    for( FOOTPRINT* fp : m_parent->GetBoard()->Footprints() )
    {
        m_footprintsList.emplace_back( *fp );
    }

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
    m_editVariantDescButton->SetBitmap( KiBitmapBundle( BITMAPS::text ) );

    m_sidebarButton->SetBitmap( KiBitmapBundle( BITMAPS::left ) );

    // Do not OptOut the notebook. That would also exclude its child controls such as the
    // scope selector from being persisted. The active page is forced by the opening tool.

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
    attr->SetReadOnly(); // not really; we delegate interactivity to GRID_TRICKS
    attr->SetAlignment( wxALIGN_CENTER, wxALIGN_CENTER );
    m_viewControlsDataModel->SetColAttr( attr, SHOW_FIELD_COLUMN );

    attr = new wxGridCellAttr;
    attr->SetRenderer( new wxGridCellBoolRenderer() );
    attr->SetReadOnly(); // not really; we delegate interactivity to GRID_TRICKS
    attr->SetAlignment( wxALIGN_CENTER, wxALIGN_CENTER );
    m_viewControlsDataModel->SetColAttr( attr, GROUP_BY_COLUMN );

    // Compress the view controls grid.  (We want it to look different from the fields grid.)
    m_viewControlsGrid->SetDefaultRowSize( m_viewControlsGrid->GetDefaultRowSize() - FromDIP( 4 ) );

    m_filter->SetDescriptiveText( _( "Filter" ) );

    attr = new wxGridCellAttr;
    attr->SetEditor( new GRID_CELL_URL_EDITOR( this, nullptr, { m_parent->GetBoard() } ) );
    m_dataModel = new FOOTPRINT_FIELDS_EDITOR_GRID_DATA_MODEL( m_footprintsList, attr );

    m_grid->UseNativeColHeader( true );
    m_grid->SetTable( m_dataModel, true );

    // The field-list grid regroups its rows, so the dialog's position-based Ctrl+Z would shift
    // values onto the wrong field.
    ExcludeFromControlUndoRedo( m_viewControlsGrid );

    // must be done after SetTable(), which appears to re-set it
    m_grid->SetSelectionMode( wxGrid::wxGridSelectCells );

    // add Cut, Copy, and Paste to wxGrid
    m_grid->PushEventHandler( new FIELDS_EDITOR_GRID_TRICKS( this, m_grid, m_viewControlsDataModel, m_dataModel,
                                                             m_parent->GetBoard()->GetEmbeddedFiles() ) );

    m_variantListBox->Set( parent->GetBoard()->GetVariantNamesForUI() );

    // A job keeps its own variant, otherwise follow the board.
    wxString variantToSelect;

    if( m_job )
        variantToSelect = m_job->GetSelectedVariant();
    else
        variantToSelect = m_parent->GetBoard()->GetCurrentVariant();

    if( !variantToSelect.IsEmpty() )
    {
        int toSelect = m_variantListBox->FindString( variantToSelect );

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
    else
        SetTitle( _( "Footprint Fields Table" ) );

    m_buttonApply->SetLabel( _( "Apply, Save Board && Continue" ) );

    // DIALOG_SHIM needs a unique hash_key because classname will be the same for both job and
    // non-job versions (which have different sizes).
    m_hash_key = TO_UTF8( GetTitle() );

    // Set the current variant for highlighting variant-specific field values
    m_dataModel->SetCurrentVariant( resolveVariant() );

    // Disable variant editing controls in for footprint fields table, variants come only from the schematic
    m_addVariantButton->Hide();
    m_copyVariantButton->Hide();
    m_renameVariantButton->Hide();
    m_editVariantDescButton->Hide();
    m_deleteVariantButton->Hide();

    SetInitialFocus( m_grid );
    m_grid->ClearSelection();

    SetupStandardButtons();

    finishDialogSettings();

    SetSize( GetDefaultDialogSize() );

    RestorePanelLayout();

    OptOut( m_outputFileName );

    if( m_job )
        m_outputFileName->SetValue( m_job->GetConfiguredOutputPath() );
    else
        m_outputFileName->SetValue( m_boardSettings.m_BomExportFileName );

    Center();

    // Connect Events
    m_grid->Bind( wxEVT_GRID_COL_SORT, &DIALOG_FOOTPRINT_FIELDS_TABLE::OnColSort, this );
    m_grid->Bind( wxEVT_GRID_COL_MOVE, &DIALOG_FOOTPRINT_FIELDS_TABLE::OnColMove, this );
    m_grid->GetGridWindow()->Bind( wxEVT_MOTION, &DIALOG_FOOTPRINT_FIELDS_TABLE::OnGridMouseMove, this );
    m_cbBomPresets->Bind( wxEVT_CHOICE, &DIALOG_FOOTPRINT_FIELDS_TABLE::onBomPresetChanged, this );
    m_cbBomFmtPresets->Bind( wxEVT_CHOICE, &DIALOG_FOOTPRINT_FIELDS_TABLE::onBomFmtPresetChanged, this );
    m_viewControlsGrid->Bind( wxEVT_GRID_CELL_CHANGED, &DIALOG_FOOTPRINT_FIELDS_TABLE::OnViewControlsCellChanged, this );

    if( !m_job )
    {
        // Start listening for board changes
        m_parent->GetBoard()->AddListener( this );
        m_parent->Bind( EDA_EVT_PCB_LAST_SCH_SHEET_CHANGED,
                        &DIALOG_FOOTPRINT_FIELDS_TABLE::OnCurrentSchematicSheetChanged, this );
    }
    else
    {
        // Don't allow editing
        m_grid->EnableEditing( false );
        m_buttonApply->Hide();
        m_buttonExport->Hide();
    }
}


DIALOG_FOOTPRINT_FIELDS_TABLE::~DIALOG_FOOTPRINT_FIELDS_TABLE()
{
    if( !m_job )
    {
        m_parent->Unbind( EDA_EVT_PCB_LAST_SCH_SHEET_CHANGED,
                          &DIALOG_FOOTPRINT_FIELDS_TABLE::OnCurrentSchematicSheetChanged, this );
    }

    savePresetsToBoard();

    SavePanelLayout();

    FIELDS_TABLE_SETTINGS& cfg = GetPanelSettings();

    for( int i = 0; i < m_grid->GetNumberCols(); i++ )
    {
        if( m_grid->IsColShown( i ) )
        {
            std::string fieldName( m_dataModel->GetColFieldName( i ).ToUTF8() );
            cfg.field_widths[fieldName] = m_grid->GetColSize( i );
        }
    }

    // Disconnect Events
    m_grid->GetGridWindow()->Unbind( wxEVT_MOTION, &DIALOG_FOOTPRINT_FIELDS_TABLE::OnGridMouseMove, this );
    m_grid->Unbind( wxEVT_GRID_COL_SORT, &DIALOG_FOOTPRINT_FIELDS_TABLE::OnColSort, this );
    m_grid->Unbind( wxEVT_GRID_COL_MOVE, &DIALOG_FOOTPRINT_FIELDS_TABLE::OnColMove, this );
    m_cbBomPresets->Unbind( wxEVT_CHOICE, &DIALOG_FOOTPRINT_FIELDS_TABLE::onBomPresetChanged, this );
    m_cbBomFmtPresets->Unbind( wxEVT_CHOICE, &DIALOG_FOOTPRINT_FIELDS_TABLE::onBomFmtPresetChanged, this );
    m_viewControlsGrid->Unbind( wxEVT_GRID_CELL_CHANGED, &DIALOG_FOOTPRINT_FIELDS_TABLE::OnViewControlsCellChanged, this );

    // Delete the GRID_TRICKS.
    m_viewControlsGrid->PopEventHandler( true );
    m_grid->PopEventHandler( true );

    // we gave ownership of m_viewControlsDataModel & m_dataModel to the wxGrids...
}


void DIALOG_FOOTPRINT_FIELDS_TABLE::SetupColumnProperties( int aCol )
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
        attr->SetEditor( new GRID_CELL_URL_EDITOR( this, nullptr, { m_parent->GetBoard() } ) );
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
        attr->SetReadOnly(); // not really; we delegate interactivity to GRID_TRICKS
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


void DIALOG_FOOTPRINT_FIELDS_TABLE::SetupAllColumnProperties()
{
    FIELDS_TABLE_SETTINGS& cfg = GetPanelSettings();
    wxSize                 defaultDlgSize = GetDefaultDialogSize();

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

            if( cfg.field_widths.count( key ) && ( cfg.field_widths.at( key ) > 0 ) )
            {
                m_grid->SetColSize( col, cfg.field_widths.at( key ) );
            }
            else
            {
                int textWidth = m_dataModel->GetColDataWidth( col ) + COLUMN_MARGIN;
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


bool DIALOG_FOOTPRINT_FIELDS_TABLE::TransferDataToWindow()
{
    if( !wxDialog::TransferDataToWindow() )
        return false;

    LoadFieldNames(); // loads rows into m_viewControlsDataModel and columns into m_dataModel

    // Load our BOM view presets
    SetUserBomPresets( m_boardSettings.m_BomPresets );

    BOM_PRESET preset = m_boardSettings.m_BomSettings;

    if( m_job )
    {
        preset.name = m_job->m_bomPresetName;
        preset.excludeDNP = m_job->m_excludeDNP;
        preset.filterString = m_job->m_filterString;
        preset.sortAsc = m_job->m_sortAsc;
        preset.sortField = m_job->m_sortField;
        preset.groupSymbols = m_job->m_groupSymbols;

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
    SetUserBomFmtPresets( m_boardSettings.m_BomFmtPresets );
    BOM_FMT_PRESET fmtPreset = m_boardSettings.m_BomFmtSettings;

    if( m_job )
    {
        fmtPreset.name = m_job->m_bomFmtPresetName;
        fmtPreset.fieldDelimiter = m_job->m_fieldDelimiter;
        fmtPreset.keepLineBreaks = m_job->m_keepLineBreaks;
        fmtPreset.keepTabs = m_job->m_keepTabs;
        fmtPreset.includeByteOrderMark = m_job->m_includeByteOrderMark;
        fmtPreset.refDelimiter = m_job->m_refDelimiter;
        fmtPreset.refRangeDelimiter = m_job->m_refRangeDelimiter;
        fmtPreset.stringDelimiter = m_job->m_stringDelimiter;
    }

    ApplyBomFmtPreset( fmtPreset );
    syncBomFmtPresetSelection();

    if( !m_job )
        m_outputFileName->SetValue( m_boardSettings.m_BomExportFileName );

    TOOL_MANAGER*       toolMgr = m_parent->GetToolManager();
    PCB_SELECTION_TOOL* selectionTool = toolMgr->GetTool<PCB_SELECTION_TOOL>();
    PCB_SELECTION&      selection = selectionTool->GetSelection();
    FOOTPRINT*          footprint = nullptr;

    m_dataModel->SetGroupingEnabled( m_groupSymbolsBox->GetValue() );

    setScope( static_cast<SCOPE>( m_scope->GetSelection() ) );

    if( selection.GetSize() == 1 )
    {
        EDA_ITEM* item = selection.Front();

        if( item->Type() == PCB_FOOTPRINT_T )
            footprint = (FOOTPRINT*) item;
        else if( item->GetParent() && item->GetParent()->Type() == PCB_FOOTPRINT_T )
            footprint = (FOOTPRINT*) item->GetParent();
    }

    if( footprint )
    {
        for( int row = 0; row < m_dataModel->GetNumberRows(); ++row )
        {
            std::vector<FOOTPRINT_REF> references = m_dataModel->GetRowReferences( row );
            bool                       found = false;

            for( const FOOTPRINT_REF& ref : references )
            {
                if( &ref.GetFootprint() == footprint )
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


bool DIALOG_FOOTPRINT_FIELDS_TABLE::TransferDataFromWindow()
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

    BOARD_COMMIT commit( m_parent );
    wxString     currentVariant = m_parent->GetBoard()->GetCurrentVariant();

    // TODO: board settings don't have template field names, ideally we would sync these from schematic
    // or more likely move them up the project level since they should be the same for symbols/footprints.
    TEMPLATES notImplemented;
    m_dataModel->ApplyData( commit, notImplemented, currentVariant );

    if( !commit.Empty() )
    {
        commit.Push( wxS( "Footprint Fields Table Edit" ) ); // Push clears the commit buffer.
        m_parent->OnModify();
    }

    // Reset the view to where we left the user
    m_parent->Refresh();

    return true;
}


void DIALOG_FOOTPRINT_FIELDS_TABLE::AddField( const wxString& aFieldName, const wxString& aLabelValue, bool show,
                                              bool groupBy, bool addedByUser )
{
    // Users can add fields with variable names that match the special names in the grid,
    // e.g. ${QUANTITY} so make sure we don't add them twice
    for( int row = 0; row < m_viewControlsDataModel->GetNumberRows(); row++ )
    {
        if( FieldNamesAreDuplicates( m_viewControlsDataModel->GetCanonicalFieldName( row ), aFieldName ) )
        {
            return;
        }
    }

    m_dataModel->AddColumn( aFieldName, aLabelValue, addedByUser );

    wxGridTableMessage msg( m_dataModel, wxGRIDTABLE_NOTIFY_COLS_APPENDED, 1 );
    m_grid->ProcessTableMessage( msg );

    m_viewControlsGrid->OnAddRow(
            [&]() -> std::pair<int, int>
            {
                m_viewControlsDataModel->AppendRow( aFieldName, aLabelValue, show, groupBy );

                return { m_viewControlsDataModel->GetNumberRows() - 1, -1 };
            } );
}


void DIALOG_FOOTPRINT_FIELDS_TABLE::LoadFieldNames()
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
    AddField( FOOTPRINT_FIELDS_EDITOR_GRID_DATA_MODEL::QUANTITY_VARIABLE, _( "Qty" ), true, false );
    AddField( FOOTPRINT_FIELDS_EDITOR_GRID_DATA_MODEL::ITEM_NUMBER_VARIABLE, _( "#" ), true, false );

    // User field names are stored and matched case-sensitively (see issue #24021), so each
    // distinct name gets its own column rather than collapsing case variants together.
    std::set<wxString> userFieldNames;

    for( int ii = 0; ii < (int) m_footprintsList.size(); ++ii )
    {
        FOOTPRINT& footprint = m_footprintsList[ii].GetFootprint();

        for( const PCB_FIELD* field : footprint.GetFields() )
        {
            if( !field->IsMandatory() && !field->IsPrivate() )
                userFieldNames.insert( field->GetName() );
        }
    }

    for( const wxString& fieldName : userFieldNames )
        AddField( fieldName, GetGeneratedFieldDisplayName( fieldName ), true, false );

    // Add any templateFieldNames which aren't already present.
    // TODO: no template fieldnames in board settings
    TEMPLATES notImplemented;
    for( const TEMPLATE_FIELDNAME& tfn : notImplemented.GetTemplateFieldNames() )
    {
        if( userFieldNames.count( tfn.m_Name ) == 0 )
            AddField( tfn.m_Name, GetGeneratedFieldDisplayName( tfn.m_Name ), false, false );
    }
}


void DIALOG_FOOTPRINT_FIELDS_TABLE::OnAddField( wxCommandEvent& event )
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
        if( FieldNamesAreDuplicates( fieldName, m_dataModel->GetColFieldName( i ) ) )
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


void DIALOG_FOOTPRINT_FIELDS_TABLE::OnRemoveField( wxCommandEvent& event )
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
                int      col = m_dataModel->GetFieldNameCol( fieldName );

                if( col != -1 )
                    m_dataModel->RemoveColumn( col );

                m_viewControlsDataModel->DeleteRow( row );

                syncBomPresetSelection();
                OnModify();
            } );
}


void DIALOG_FOOTPRINT_FIELDS_TABLE::OnRenameField( wxCommandEvent& event )
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


void DIALOG_FOOTPRINT_FIELDS_TABLE::OnFilterText( wxCommandEvent& aEvent )
{
    m_dataModel->SetFilter( m_filter->GetValue() );
    m_dataModel->RebuildRows();
    m_grid->ForceRefresh();

    syncBomPresetSelection();
}


void DIALOG_FOOTPRINT_FIELDS_TABLE::setScope( SCOPE aScope )
{
    m_dataModel->SetPath( m_parent->GetLastSchematicSheetPath() );
    m_dataModel->SetScope( aScope );
    m_dataModel->RebuildRows();
}


void DIALOG_FOOTPRINT_FIELDS_TABLE::OnScope( wxCommandEvent& aEvent )
{
    switch( aEvent.GetSelection() )
    {
    case 0: setScope( SCOPE::SCOPE_ALL );             break;
    case 1: setScope( SCOPE::SCOPE_SHEET );           break;
    case 2: setScope( SCOPE::SCOPE_SHEET_RECURSIVE ); break;
    }
}


void DIALOG_FOOTPRINT_FIELDS_TABLE::OnGroupSymbolsToggled( wxCommandEvent& event )
{
    m_dataModel->SetGroupingEnabled( m_groupSymbolsBox->GetValue() );
    m_dataModel->RebuildRows();
    m_grid->ForceRefresh();

    syncBomPresetSelection();
}


void DIALOG_FOOTPRINT_FIELDS_TABLE::OnMenu( wxCommandEvent& event )
{
    FIELDS_TABLE_SETTINGS& cfg = GetPanelSettings();

    // Build a pop menu:
    wxMenu menu;

    menu.Append( 4204, _( "Include 'DNP' Footprints" ),
                 _( "Show footprints marked 'DNP' in the table.  This setting also controls whether or not 'DNP' "
                    "footprints are included on export." ),
                 wxITEM_CHECK );
    menu.Check( 4204, !m_dataModel->GetExcludeDNP() );

    menu.Append( 4205, _( "Include 'Exclude from BOM' Footprints" ),
                 _( "Show footprints marked 'Exclude from BOM' in the table.  Footprints marked 'Exclude from BOM' "
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


void DIALOG_FOOTPRINT_FIELDS_TABLE::OnColSort( wxGridEvent& aEvent )
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


void DIALOG_FOOTPRINT_FIELDS_TABLE::OnColMove( wxGridEvent& aEvent )
{
    int origPos = aEvent.GetCol();

    // Save column widths since the setup function uses the saved config values
    FIELDS_TABLE_SETTINGS& cfg = GetPanelSettings();

    for( int i = 0; i < m_grid->GetNumberCols(); i++ )
    {
        if( m_grid->IsColShown( i ) )
        {
            std::string fieldName( m_dataModel->GetColFieldName( i ).ToUTF8() );
            cfg.field_widths[fieldName] = m_grid->GetColSize( i );
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


void DIALOG_FOOTPRINT_FIELDS_TABLE::ShowHideColumn( int aCol, bool aShow )
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


void DIALOG_FOOTPRINT_FIELDS_TABLE::OnViewControlsCellChanged( wxGridEvent& aEvent )
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


void DIALOG_FOOTPRINT_FIELDS_TABLE::OnRegroupSymbols( wxCommandEvent& aEvent )
{
    m_dataModel->RebuildRows();
    m_grid->ForceRefresh();
}


void DIALOG_FOOTPRINT_FIELDS_TABLE::OnTableCellClick( wxGridEvent& event )
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


void DIALOG_FOOTPRINT_FIELDS_TABLE::OnGridMouseMove( wxMouseEvent& aEvent )
{
    aEvent.Skip();

    wxPoint pos = aEvent.GetPosition();
    int     ux, uy;
    m_grid->CalcUnscrolledPosition( pos.x, pos.y, &ux, &uy );
    int row = m_grid->YToRow( uy );
    int col = m_grid->XToCol( ux );


    if( row == wxNOT_FOUND || col == wxNOT_FOUND )
    {
        m_grid->GetGridWindow()->UnsetToolTip();
        return;
    }

    wxString rawValue = m_dataModel->GetValue( row, col );

    if( rawValue.Contains( wxT( "${" ) ) )
    {
        m_grid->GetGridWindow()->SetToolTip( rawValue );
    }
    else
    {
        m_grid->GetGridWindow()->UnsetToolTip();
    }
}


void DIALOG_FOOTPRINT_FIELDS_TABLE::OnTableRangeSelected( wxGridRangeSelectEvent& aEvent )
{
    FIELDS_TABLE_SETTINGS& cfg = GetPanelSettings();

    // Cross-probing should only work in Edit page
    if( m_nbPages->GetSelection() != 0 )
        return;

    // Multi-select can grab the rows that are expanded child refs, and also the row
    // containing the list of all child refs. Make sure we add refs/footprints uniquely
    std::set<BOARD_ITEM*> footprints;

    // This handler handles selecting and deselecting
    if( aEvent.Selecting() )
    {
        for( int i = aEvent.GetTopRow(); i <= aEvent.GetBottomRow(); i++ )
        {
            for( const FOOTPRINT_REF& ref : m_dataModel->GetRowReferences( i ) )
                footprints.insert( &ref.GetFootprint() );
        }
    }

    std::vector<BOARD_ITEM*> focusItems( footprints.begin(), footprints.end() );

    if( cfg.selection_mode == 0 )
    {
        m_parent->FocusOnItems( focusItems );
    }
    else if( cfg.selection_mode == 1 )
    {
        m_parent->GetToolManager()->RunAction( PCB_ACTIONS::syncSelection, &focusItems );
    }
}


void DIALOG_FOOTPRINT_FIELDS_TABLE::OnSaveAndContinue( wxCommandEvent& aEvent )
{
    if( TransferDataFromWindow() )
    {
        m_boardSettings.m_BomExportFileName = m_outputFileName->GetValue();
        m_parent->SaveBoard();
        ClearModify();
    }
}


void DIALOG_FOOTPRINT_FIELDS_TABLE::OnPageChanged( wxNotebookEvent& event )
{
    if( m_dataModel->GetColsCount() )
        PreviewRefresh();
}


void DIALOG_FOOTPRINT_FIELDS_TABLE::OnPreviewRefresh( wxCommandEvent& event )
{
    PreviewRefresh();
    syncBomFmtPresetSelection();
}


void DIALOG_FOOTPRINT_FIELDS_TABLE::PreviewRefresh()
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


void DIALOG_FOOTPRINT_FIELDS_TABLE::OnOutputFileBrowseClicked( wxCommandEvent& event )
{
    // Build the absolute path of current output directory to preselect it in the file browser.
    wxString path = ExpandEnvVarSubstitutions( m_outputFileName->GetValue(), &Prj() );
    path = Prj().AbsolutePath( path );


    // Calculate the export filename
    wxFileName fn( Prj().AbsolutePath( m_parent->GetBoard()->GetFileName() ) );
    fn.SetExt( FILEEXT::CsvFileExtension );

    wxFileDialog saveDlg( this, _( "Bill of Materials Output File" ), path, fn.GetFullName(),
                          FILEEXT::CsvFileWildcard(), wxFD_SAVE | wxFD_OVERWRITE_PROMPT );

    KIPLATFORM::UI::AllowNetworkFileSystems( &saveDlg );

    if( saveDlg.ShowModal() == wxID_CANCEL )
        return;


    wxFileName file = wxFileName( saveDlg.GetPath() );
    wxString   defaultPath = fn.GetPathWithSep();

    if( IsOK( this, wxString::Format( _( "Do you want to use a path relative to\n'%s'?" ), defaultPath ) ) )
    {
        if( !file.MakeRelativeTo( defaultPath ) )
        {
            DisplayErrorMessage( this, _( "Cannot make path relative (target volume different from board "
                                          "file volume)!" ) );
        }
    }

    m_outputFileName->SetValue( file.GetFullPath() );
}


void DIALOG_FOOTPRINT_FIELDS_TABLE::OnExport( wxCommandEvent& aEvent )
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
                BOARD* board = m_parent->GetBoard();

                // Handles m_board->GetTitleBlock() *and* m_board->GetProject()
                return board->ResolveTextVar( token, 0 );
            };

    wxString path = m_outputFileName->GetValue();

    if( path.IsEmpty() )
    {
        // Match the behaviour of other exporters and default to <board>.csv in the project
        // directory when the user leaves the field blank.
        path = GetDefaultBomFileName( m_parent->GetBoard()->GetFileName() );

        if( path.IsEmpty() )
        {
            DisplayError( this, _( "No output file specified in Export tab." ) );
            return;
        }

        m_outputFileName->SetValue( path );
    }

    path = ExpandTextVars( NormalizeFilePathForTextVars( path ), &textResolver );
    path = ExpandEnvVarSubstitutions( path, &Prj() );

    wxFileName outputFile = wxFileName::FileName( path );
    wxString   msg;

    if( !EnsureFileDirectoryExists( &outputFile, Prj().AbsolutePath( m_parent->GetBoard()->GetFileName() ),
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

    if( m_boardSettings.m_BomExportFileName != m_outputFileName->GetValue() )
    {
        m_boardSettings.m_BomExportFileName = m_outputFileName->GetValue();
        m_parent->OnModify();
    }

    msg.Printf( _( "Wrote BOM output to '%s'" ), outputFile.GetFullPath() );
    DisplayInfoMessage( this, msg );
}


void DIALOG_FOOTPRINT_FIELDS_TABLE::OnCancel( wxCommandEvent& aEvent )
{
    if( m_job )
    {
        EndModal( wxID_CANCEL );
    }
    else
    {
        // Discard any unsaved edit in the output filename field
        m_outputFileName->SetValue( m_boardSettings.m_BomExportFileName );
        Close();
    }
}


void DIALOG_FOOTPRINT_FIELDS_TABLE::OnOk( wxCommandEvent& aEvent )
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
        m_job->m_includeByteOrderMark = fmtSettings.includeByteOrderMark;

        BOM_PRESET presetFields = m_dataModel->GetBomSettings();
        m_job->m_sortAsc = presetFields.sortAsc;
        m_job->m_excludeDNP = presetFields.excludeDNP;
        m_job->m_filterString = presetFields.filterString;
        m_job->m_sortField = presetFields.sortField;
        m_job->m_groupSymbols = presetFields.groupSymbols;

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

        m_job->SetSelectedVariant( getSelectedVariant() );

        EndModal( wxID_OK );
    }
    else
    {
        if( m_boardSettings.m_BomExportFileName != m_outputFileName->GetValue() )
        {
            m_boardSettings.m_BomExportFileName = m_outputFileName->GetValue();
            m_parent->OnModify();
        }

        Close();
    }
}


void DIALOG_FOOTPRINT_FIELDS_TABLE::OnClose( wxCloseEvent& aEvent )
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

    // Stop listening to board events
    m_parent->GetBoard()->RemoveListener( this );
    m_parent->ClearFocus();

    wxCommandEvent* evt = new wxCommandEvent( EDA_EVT_CLOSE_DIALOG_FOOTPRINT_FIELDS_TABLE, wxID_ANY );

    if( wxWindow* parent = GetParent() )
        wxQueueEvent( parent, evt );
}


void DIALOG_FOOTPRINT_FIELDS_TABLE::doApplyBomPreset( const BOM_PRESET& aPreset )
{
    // Disable rebuilds while we're applying the preset otherwise we'll be
    // rebuilding the model constantly while firing off wx events
    m_dataModel->DisableRebuilds();

    // Basically, we apply the BOM preset to the data model and then
    // update our UI to reflect resulting the data model state, not the preset.
    m_dataModel->SetCurrentVariant( resolveVariant() );
    m_dataModel->ApplyBomPreset( aPreset );

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

        FIELDS_TABLE_SETTINGS& cfg = GetPanelSettings();
        std::string            fieldNameStr( fieldName.ToUTF8() );

        // Set column labels
        const wxString& label = m_dataModel->GetColLabelValue( col );
        m_viewControlsDataModel->SetValue( i, LABEL_COLUMN, label );
        m_grid->SetColLabelValue( col, label );

        if( cfg.field_widths.count( fieldNameStr ) )
            m_grid->SetColSize( col, cfg.field_widths.at( fieldNameStr ) );

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


void DIALOG_FOOTPRINT_FIELDS_TABLE::doApplyBomFmtPreset( const BOM_FMT_PRESET& aPreset )
{
    m_textFieldDelimiter->ChangeValue( aPreset.fieldDelimiter );
    m_textStringDelimiter->ChangeValue( aPreset.stringDelimiter );
    m_textRefDelimiter->ChangeValue( aPreset.refDelimiter );
    m_textRefRangeDelimiter->ChangeValue( aPreset.refRangeDelimiter );
    m_checkKeepTabs->SetValue( aPreset.keepTabs );
    m_checkKeepLineBreaks->SetValue( aPreset.keepLineBreaks );
    m_checkIncludeByteOrderMark->SetValue( aPreset.includeByteOrderMark );

    // Refresh the preview if that's the current page
    if( m_nbPages->GetSelection() == 1 )
        PreviewRefresh();
}


BOM_PRESET DIALOG_FOOTPRINT_FIELDS_TABLE::getDataModelBomPreset()
{
    return m_dataModel->GetBomSettings();
}


void DIALOG_FOOTPRINT_FIELDS_TABLE::savePresetsToBoard()
{
    bool modified = false;

    // Save our BOM presets
    std::vector<BOM_PRESET> presets;

    for( const auto& [name, preset] : m_bomPresets )
    {
        if( !preset.readOnly )
            presets.emplace_back( preset );
    }

    if( m_boardSettings.m_BomPresets != presets )
    {
        modified = true;
        m_boardSettings.m_BomPresets = presets;
    }

    if( m_boardSettings.m_BomSettings != m_dataModel->GetBomSettings() && !m_job )
    {
        modified = true;
        m_boardSettings.m_BomSettings = m_dataModel->GetBomSettings();
    }

    // Save our BOM Format presets
    std::vector<BOM_FMT_PRESET> fmts;

    for( const auto& [name, preset] : m_bomFmtPresets )
    {
        if( !preset.readOnly )
            fmts.emplace_back( preset );
    }

    if( m_boardSettings.m_BomFmtPresets != fmts )
    {
        modified = true;
        m_boardSettings.m_BomFmtPresets = fmts;
    }

    if( m_boardSettings.m_BomFmtSettings != GetCurrentBomFmtSettings() && !m_job )
    {
        modified = true;
        m_boardSettings.m_BomFmtSettings = GetCurrentBomFmtSettings();
    }

    if( modified )
        m_parent->OnModify();
}


void DIALOG_FOOTPRINT_FIELDS_TABLE::OnBoardItemsAdded( BOARD& aPcb, std::vector<BOARD_ITEM*>& aPcbItem )
{
    FOOTPRINT_REFERENCE_LIST addedRefs;

    for( BOARD_ITEM* item : aPcbItem )
    {
        if( item->Type() == PCB_FOOTPRINT_T )
            addedRefs.push_back( FOOTPRINT_REF( *static_cast<FOOTPRINT*>( item ) ) );
    }

    if( addedRefs.empty() )
        return;

    std::set<KIID> savedSelection = SaveGridSelection();
    DisableSelectionEvents();

    for( FOOTPRINT_REF& ref : addedRefs )
    {
        // Add all fields again in case this footprint has a new one
        for( PCB_FIELD* field : ref.GetFootprint().GetFields() )
        {
            if( !field->IsMandatory() && !field->IsPrivate() )
                AddField( field->GetCanonicalName(), field->GetName(), true, false, true );
        }
    }


    m_dataModel->AddReferences( addedRefs );
    m_dataModel->RebuildRows();

    RestoreGridSelection( savedSelection );
    EnableSelectionEvents();
}


void DIALOG_FOOTPRINT_FIELDS_TABLE::OnBoardItemsRemoved( BOARD& aPcb, std::vector<BOARD_ITEM*>& aPcbItem )
{
    std::set<KIID> savedSelection = SaveGridSelection();

    for( BOARD_ITEM* item : aPcbItem )
    {
        if( item->Type() == PCB_FOOTPRINT_T )
            m_dataModel->RemoveFootprint( FOOTPRINT_REF( *static_cast<FOOTPRINT*>( item ) ) );
    }

    DisableSelectionEvents();
    m_dataModel->RebuildRows();
    RestoreGridSelection( savedSelection );
    EnableSelectionEvents();
}


void DIALOG_FOOTPRINT_FIELDS_TABLE::OnBoardItemsChanged( BOARD& aPcb, std::vector<BOARD_ITEM*>& aPcbItem )
{
    FOOTPRINT_REFERENCE_LIST changedRefs;

    for( BOARD_ITEM* item : aPcbItem )
    {
        if( item->Type() == PCB_FOOTPRINT_T )
            changedRefs.push_back( FOOTPRINT_REF( *static_cast<FOOTPRINT*>( item ) ) );
    }

    if( changedRefs.empty() )
        return;

    std::set<KIID> savedSelection = SaveGridSelection();
    DisableSelectionEvents();

    for( FOOTPRINT_REF& ref : changedRefs )
    {
        // Add all fields again in case this footprint has a new one
        for( PCB_FIELD* field : ref.GetFootprint().GetFields() )
        {
            if( !field->IsMandatory() && !field->IsPrivate() )
                AddField( field->GetCanonicalName(), field->GetName(), true, false, true );
        }
    }


    m_dataModel->UpdateReferences( changedRefs );
    m_dataModel->RebuildRows();

    RestoreGridSelection( savedSelection );
    EnableSelectionEvents();
}


void DIALOG_FOOTPRINT_FIELDS_TABLE::OnCurrentSchematicSheetChanged( wxCommandEvent& aEvent )
{
    wxUnusedVar( aEvent );

    m_dataModel->SetPath( m_parent->GetLastSchematicSheetPath() );

    if( m_dataModel->GetScope() != FOOTPRINT_FIELDS_EDITOR_GRID_DATA_MODEL::SCOPE::SCOPE_ALL )
    {
        std::set<KIID> savedSelection = SaveGridSelection();

        DisableSelectionEvents();
        m_dataModel->RebuildRows();
        RestoreGridSelection( savedSelection );
        EnableSelectionEvents();
    }
}


void DIALOG_FOOTPRINT_FIELDS_TABLE::EnableSelectionEvents()
{
    m_grid->Connect( wxEVT_GRID_RANGE_SELECTED,
                     wxGridRangeSelectEventHandler( DIALOG_FOOTPRINT_FIELDS_TABLE::OnTableRangeSelected ),
                     nullptr, this );
}


void DIALOG_FOOTPRINT_FIELDS_TABLE::DisableSelectionEvents()
{
    m_grid->Disconnect( wxEVT_GRID_RANGE_SELECTED,
                        wxGridRangeSelectEventHandler( DIALOG_FOOTPRINT_FIELDS_TABLE::OnTableRangeSelected ),
                        nullptr, this );
}


std::set<KIID> DIALOG_FOOTPRINT_FIELDS_TABLE::SaveGridSelection()
{
    std::set<KIID> selectedFullPaths;

    wxGridCellCoordsArray topLeft = m_grid->GetSelectionBlockTopLeft();
    wxGridCellCoordsArray bottomRight = m_grid->GetSelectionBlockBottomRight();

    for( size_t i = 0; i < topLeft.size(); ++i )
    {
        for( int row = topLeft[i].GetRow(); row <= bottomRight[i].GetRow(); ++row )
        {
            for( const FOOTPRINT_REF& ref : m_dataModel->GetRowReferences( row ) )
                selectedFullPaths.insert( ref.GetFootprint().m_Uuid );
        }
    }

    wxArrayInt selectedRows = m_grid->GetSelectedRows();

    for( int row : selectedRows )
    {
        for( const FOOTPRINT_REF& ref : m_dataModel->GetRowReferences( row ) )
            selectedFullPaths.insert( ref.GetFootprint().m_Uuid );
    }

    int cursorRow = m_grid->GetGridCursorRow();

    if( cursorRow >= 0 && selectedFullPaths.empty() )
    {
        for( const FOOTPRINT_REF& ref : m_dataModel->GetRowReferences( cursorRow ) )
            selectedFullPaths.insert( ref.GetFootprint().m_Uuid );
    }

    return selectedFullPaths;
}


void DIALOG_FOOTPRINT_FIELDS_TABLE::RestoreGridSelection( const std::set<KIID>& aKIIDs )
{
    if( aKIIDs.empty() )
        return;

    m_grid->ClearSelection();

    bool firstSelection = true;

    for( int row = 0; row < m_dataModel->GetNumberRows(); ++row )
    {
        std::vector<FOOTPRINT_REF> refs = m_dataModel->GetRowReferences( row );

        for( const FOOTPRINT_REF& ref : refs )
        {
            if( aKIIDs.count( ref.GetFootprint().m_Uuid ) )
            {
                m_grid->SelectRow( row, true );

                if( firstSelection )
                {
                    m_grid->SetGridCursor( row, m_grid->GetGridCursorCol() );
                    firstSelection = false;
                }

                break;
            }
        }
    }
}


void DIALOG_FOOTPRINT_FIELDS_TABLE::onAddVariant( wxCommandEvent& aEvent )
{
    return;
}


void DIALOG_FOOTPRINT_FIELDS_TABLE::onDeleteVariant( wxCommandEvent& aEvent )
{
    return;
}


void DIALOG_FOOTPRINT_FIELDS_TABLE::onRenameVariant( wxCommandEvent& aEvent )
{
    return;
}


void DIALOG_FOOTPRINT_FIELDS_TABLE::onCopyVariant( wxCommandEvent& aEvent )
{
    return;
}


void DIALOG_FOOTPRINT_FIELDS_TABLE::onEditVariantDescription( wxCommandEvent& aEvent )
{
    return;
}


void DIALOG_FOOTPRINT_FIELDS_TABLE::onVariantSelectionChange( wxCommandEvent& aEvent )
{
    wxString currentVariant;
    wxString selectedVariant = getSelectedVariant();

    updateVariantButtonStates();

    if( m_job )
    {
        m_grid->CommitPendingChanges( true );

        if( m_parent )
            m_parent->SetCurrentVariant( selectedVariant );

        m_dataModel->SetCurrentVariant( selectedVariant );
        m_dataModel->UpdateReferences( m_dataModel->GetReferenceList() );
        m_dataModel->RebuildRows();

        if( m_nbPages->GetSelection() == 1 )
            PreviewRefresh();
        else
            m_grid->ForceRefresh();

        syncBomFmtPresetSelection();
        return;
    }

    if( m_parent )
    {
        currentVariant = m_parent->GetBoard()->GetCurrentVariant();

        if( currentVariant != selectedVariant )
            m_parent->SetCurrentVariant( selectedVariant );
    }

    // TODO: this is probably the wrong method in both the symbol and footprint fields tables,
    // changing the variant should ask the user whether to apply the changes to the board.
    // The rest of the time in the fields table, no data is pushed to the sch/board until the user
    // explicity clicks Apply or Ok.
    if( currentVariant != selectedVariant )
    {
        m_grid->CommitPendingChanges( true );

        BOARD_COMMIT commit( m_parent );

        TEMPLATES notImplemented;
        m_dataModel->ApplyData( commit, notImplemented, currentVariant );

        if( !commit.Empty() )
        {
            commit.Push( wxS( "Footprint Fields Table Edit" ) ); // Push clears the commit buffer.
            m_parent->OnModify();
        }

        // Update the data model's current variant for field highlighting
        m_dataModel->SetCurrentVariant( selectedVariant );
        m_dataModel->UpdateReferences( m_dataModel->GetReferenceList() );
        m_dataModel->RebuildRows();

        if( m_nbPages->GetSelection() == 1 )
            PreviewRefresh();
        else
            m_grid->ForceRefresh();

        syncBomFmtPresetSelection();
    }
}


void DIALOG_FOOTPRINT_FIELDS_TABLE::updateVariantButtonStates()
{
    // All variant modifications are disabled for the footprint fields table
    m_addVariantButton->Enable( false );
    m_copyVariantButton->Enable( false );
    m_renameVariantButton->Enable( false );
    m_editVariantDescButton->Enable( false );
    m_deleteVariantButton->Enable( false );
}


wxString DIALOG_FOOTPRINT_FIELDS_TABLE::getSelectedVariant() const
{
    wxString retv;

    int selection = m_variantListBox->GetSelection();

    if( ( selection == wxNOT_FOUND ) || ( m_variantListBox->GetString( selection ) == GetDefaultVariantName() ) )
        return retv;

    return m_variantListBox->GetString( selection );
}


wxString DIALOG_FOOTPRINT_FIELDS_TABLE::resolveVariant() const
{
    // A job keeps its own variant, otherwise follow the board.
    if( m_job )
        return getSelectedVariant();

    return m_parent->GetBoard()->GetCurrentVariant();
}
