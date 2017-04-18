/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017 Oliver Walters
 * Copyright (C) 2017 KiCad Developers, see AUTHORS.txt for contributors.
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


#include <wx/colour.h>
#include <wx/msgdlg.h>
#include <wx/progdlg.h>
#include <wx/file.h>
#include <wx/filename.h>

#include <confirm.h>

#include <build_version.h>
#include <general.h>
#include <class_library.h>

#include <bom_exporter.h>

#include "dialog_bom_editor.h"
#include <bom_table_model.h>

/* BOM Table Colours */

// Create and show BOM editor
int InvokeDialogCreateBOMEditor( SCH_EDIT_FRAME* aCaller )
{
    DIALOG_BOM_EDITOR dlg( aCaller );
    return dlg.ShowModal();
}

DIALOG_BOM_EDITOR::DIALOG_BOM_EDITOR( SCH_EDIT_FRAME* parent ) :
        DIALOG_BOM_EDITOR_BASE( parent ),
        m_parent( parent )
{
    m_bom = BOM_TABLE_MODEL::Create();

    m_columnListCtrl->DeleteAllItems();
    m_columnListCtrl->ClearColumns();

    auto nameColumn = m_columnListCtrl->AppendTextColumn( _( "Field" ) );

    auto showColumn = m_columnListCtrl->AppendToggleColumn(
                                        _( "Show" ),
                                        wxDATAVIEW_CELL_ACTIVATABLE,
                                        100 );

    // Resize the columns appropriately
    m_columnListCtrl->Update();

    showColumn->SetWidth( wxCOL_WIDTH_AUTOSIZE );
    showColumn->SetMinWidth( showColumn->GetWidth() );
    showColumn->SetResizeable( false );

    m_columnListCtrl->Update();

    nameColumn->SetWidth( wxCOL_WIDTH_AUTOSIZE );
    nameColumn->SetResizeable( true );

    // Read all components
    LoadComponents();

    LoadColumnNames();
    ReloadColumns();

    m_bom->ReloadTable();

    Update();
}

DIALOG_BOM_EDITOR::~DIALOG_BOM_EDITOR()
{
    //TODO
}

/**
 * When the component table dialog is closed,
 * work out if we need to save any changed.
 * If so, capture those changes and push them to the undo stack.
 */
bool DIALOG_BOM_EDITOR::TransferDataFromWindow()
{
    bool saveChanges = false;

    // If there are changed values, warn the user first
    if( m_bom->HaveFieldsChanged() )
    {
        int result = DisplayExitDialog( this, _( "Changes exist in component table" ) );

        switch( result )
        {
        // Save and exit
        case wxID_YES:
            saveChanges = true;
            break;
        // Cancel (do not exit)
        case wxID_CANCEL:
            return false;
        // Do not save, exit
        default:
            return true;
        }
    }

    if( saveChanges )
    {
        /** Create a list of picked items for undo
         * PICKED_ITEMS_LIST contains multiple ITEM_PICKER instances
         * Each ITEM_PICKER contains a component and a command
         */

        auto pickerList = PICKED_ITEMS_LIST();

        // List of components that have changed
        auto changed = m_bom->GetChangedComponents();

        ITEM_PICKER picker;

        for( auto cmp : changed )
        {
            // Push the component into the picker list
            picker = ITEM_PICKER( cmp, UR_CHANGED );
            picker.SetFlags( cmp->GetFlags() );
            //picker.SetLink( DuplicateStruct( cmp, true ) );

            pickerList.PushItem( picker );
        }

        if( pickerList.GetCount() > 0 )
        {
            m_parent->SaveCopyInUndoList( pickerList, UR_CHANGED );
            m_bom->ApplyFieldChanges();
            m_parent->Refresh();
        }

        m_parent->OnModify();
    }

    return true;
}

/**
 * Update the window title to reflect the contents of the table
 */
void DIALOG_BOM_EDITOR::UpdateTitle()
{
    wxString title = _( "Component table" ) + wxString( " - " );

    title += wxString::Format( "%u %s",
                              m_bom->ComponentCount(),
                              _( "components" ) );

    if( m_bom->GetColumnGrouping() )
    {
        title += wxString::Format( " %s %u %s",
                              _( "in" ),
                              (unsigned int) m_bom->Groups.size(),
                              _( "groups" ) );
    }

    unsigned int count = m_bom->CountChangedComponents();

    if( count > 0 )
    {
        title += wxString::Format( " - %u %s",
                                   count,
                                   _( "changed" ) );
    }

    SetTitle( title );
}

/**
 * Load component data from the entire schematic set
 */
void DIALOG_BOM_EDITOR::LoadComponents()
{
    if( !m_parent ) return;

    // List of component objects
    SCH_REFERENCE_LIST refs;

    // Generate a list of schematic sheets
    SCH_SHEET_LIST sheets( g_RootSheet );
    sheets.GetComponents( m_parent->Prj().SchLibs(), refs, false );

    // Pass the references through to the model
    m_bom->SetComponents( refs );
}

/**
 * Display list of columns (fields)
 */
void DIALOG_BOM_EDITOR::LoadColumnNames()
{
    m_columnListCtrl->DeleteAllItems();

    wxVector< wxVariant > data;

    for( auto* col : m_bom->ColumnList.Columns )
    {
        if( nullptr == col )
            continue;

        data.clear();

        data.push_back( wxVariant( col->Title() ) );        // Column title      (string)
        data.push_back( wxVariant( col->IsVisible() ) );    // Column visibility (bool)

        m_columnListCtrl->AppendItem( data );
    }
}

void DIALOG_BOM_EDITOR::ReloadColumns()
{
    m_bom->AttachTo( m_bomView );

    UpdateTitle();
}

void DIALOG_BOM_EDITOR::OnColumnItemToggled( wxDataViewEvent& event )
{
    wxDataViewItem item = event.GetItem();

    int row = m_columnListCtrl->ItemToRow( item );

    int col = event.GetColumn();

    if( row == wxNOT_FOUND || row < 0 || row >= (int) m_bom->ColumnCount() ) return;

    BOM_COLUMN* bomColumn = m_bom->ColumnList.GetColumnByIndex( row );

    if( nullptr == bomColumn ) return;

    bool bValue = m_columnListCtrl->GetToggleValue( row, col );

    switch ( col )
    {
    default:
        break;
    case 1: // Column visibility
        bomColumn->SetVisible( bValue );

        // Insert a new column
        if( bValue )
        {
            m_bom->AddColumn( bomColumn );
        }
        else
        {
            m_bom->RemoveColumn( bomColumn );
        }
        break;
    }
}

/**
 * Called when the "Group Components" toggle is pressed
 */
void DIALOG_BOM_EDITOR::OnGroupComponentsToggled( wxCommandEvent& event )
{
    bool group = m_groupComponentsBox->GetValue();

    m_bom->SetColumnGrouping( group );
    m_bom->ReloadTable();

    Update();
}

void DIALOG_BOM_EDITOR::OnUpdateUI( wxUpdateUIEvent& event )
{
    m_regroupComponentsButton->Enable( m_bom->GetColumnGrouping() );

    m_reloadTableButton->Enable( m_bom->HaveFieldsChanged() );

    UpdateTitle();
}

/**
 * Called when the "Export BOM" button is pressed
 * Extract row data from the component table,
 * and export it to a BOM file
 */
void DIALOG_BOM_EDITOR::OnExportBOM( wxCommandEvent& event )
{
    // Allowable BOM file formats
    static const wxString wildcard = _( "BOM Files" ) + wxString( " *.csv, *.tsv, *.html)|*.csv;*.tsv;*.htm;*.html" );

    wxFileDialog bomFileDialog(this, _("Select BOM file"),
                Prj().GetProjectPath(),
                wxEmptyString,
                wildcard,
                wxFD_SAVE | wxFD_OVERWRITE_PROMPT );


    if( bomFileDialog.ShowModal() == wxID_CANCEL )
    {
        return;
    }

    // Ensure the component groups are correct
    m_bom->ReloadTable();

    wxString msg;

    wxFileName filename = bomFileDialog.GetPath();

    // Ensure correct file format
    BOM_FILE_WRITER* writer;

    wxString fn = filename.GetFullPath().Lower();

    // CSV File
    if( fn.EndsWith( ".csv" ) )
    {
        writer = new BOM_CSV_WRITER();
    }
    // TSV file
    else if( fn.EndsWith( ".tsv" ) )
    {
        writer = new BOM_CSV_WRITER( '\t' );
    }
    // HTML file
    else if( fn.EndsWith( ".html" ) || fn.EndsWith( ".htm" ) )
    {
        writer = new BOM_HTML_WRITER();
    }
    // Unknown file!
    else
    {
        msg.Printf("%s:\n%s",
                   _( "Unsupported file type" ),
                   filename.GetExt() );

        wxMessageBox( msg );
        return;
    }

    // Set export preferences
    writer->IncludeExtraData( m_includeProjectData->GetValue() );
    writer->ShowRowNumbers( m_showRowNumbers->GetValue() );

    // Project information
    writer->SetKicadVersion( GetBuildVersion() );

    // Extract sheet info from top-level sheet
    if( g_RootSheet )
    {
        const TITLE_BLOCK& tb = g_RootSheet->GetScreen()->GetTitleBlock();

        writer->SetSchematicDate( tb.GetDate() );
        writer->SetSchematicVersion( tb.GetRevision() );
        writer->SetSchematicTitle( tb.GetTitle() );
    }

    std::vector<BOM_COLUMN*> columns;
    wxArrayString headings;

    // Extract the visible column data
    for( auto column : m_bom->ColumnList.Columns )
    {
        if( column && column->IsVisible() )
        {
            columns.push_back( column );
            headings.push_back( column->Title() );
        }
    }

    writer->SetHeader( headings );

    // Extract the row data
    for( unsigned int row=0; row<m_bom->GroupCount(); row++ )
    {
        writer->AddLine( m_bom->GetRowData( row, columns ) );
    }

    writer->SetGroupCount( m_bom->GroupCount() );
    writer->SetComponentCount( m_bom->ComponentCount() );

    // Open the BOM file for writing
    wxFile bomFile( filename.GetFullPath(), wxFile::write );

    if( bomFile.IsOpened() )
    {
        if( !writer->WriteToFile( bomFile ) )
        {
            msg.Printf( "%s:\n%s",
                        _( "Error writing BOM file" ),
                        filename.GetFullPath() );
        }

        bomFile.Close();
    }
    else
    {
        msg.Printf( "%s:\n%s",
                    _( "Error opening BOM file" ),
                    filename.GetFullPath() );

        wxMessageBox( msg );
    }
}

void DIALOG_BOM_EDITOR::OnTableValueChanged( wxDataViewEvent& event )
{
    Update();
}

void DIALOG_BOM_EDITOR::OnRegroupComponents( wxCommandEvent& event )
{
    m_bom->ReloadTable();
    Update();
}

void DIALOG_BOM_EDITOR::OnRevertFieldChanges( wxCommandEvent& event )
{
    if( m_bom->HaveFieldsChanged() )
    {
        if( IsOK( this, _( "Revert all component table changes?" ) ) )
        {
            m_bom->RevertFieldChanges();
            Update();
        }
    }
}
