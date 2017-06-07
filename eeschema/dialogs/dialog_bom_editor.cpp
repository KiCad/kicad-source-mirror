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

#include "dialog_bom_editor.h"
#include <bom_table_model.h>

/* BOM Table Colours */

// Create and show BOM editor
void InvokeDialogCreateBOMEditor( SCH_EDIT_FRAME* aCaller )
{
    DIALOG_BOM_EDITOR dlg( aCaller );
    dlg.ShowQuasiModal();
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

    auto sortColumn = m_columnListCtrl->AppendToggleColumn(
                                        _( "Sort" ),
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

    sortColumn->SetWidth( wxCOL_WIDTH_AUTOSIZE );
    sortColumn->SetResizeable( true );

    m_columnListCtrl->Update();

    // Read all components
    LoadComponents();

    LoadColumnNames();
    ReloadColumns();

    m_bom->ReloadTable();

    Update();

    m_bomView->Update();

    // Set default column widths
    for( unsigned int ii = 0; ii < m_bomView->GetColumnCount(); ii++ )
    {
        auto col = m_bomView->GetColumn( ii );

        if( !col )
            continue;

        col->SetWidth( wxCOL_WIDTH_AUTOSIZE );
        col->SetResizeable( true );
    }

    Layout();
    GetSizer()->SetSizeHints( this );
    Centre();
}


DIALOG_BOM_EDITOR::~DIALOG_BOM_EDITOR()
{
    // Nothing to do.
}


void DIALOG_BOM_EDITOR::OnCloseButton( wxCommandEvent& event )
{
    // DIALOG_BOM_EDITOR::OnDialogClosed() will be called,
    // when closing this dialog.
    // The default wxID_CANCEL handler is not suitable for us,
    // because it calls DIALOG_SHIM::EndQuasiModal() without calling
    // DIALOG_BOM_EDITOR::OnDialogClosed()
    Close();
}


bool DIALOG_BOM_EDITOR::CanCloseDialog()
{
    if( !m_bom->HaveFieldsChanged() )
        return true;

    int result = DisplayExitDialog( this, _( "Changes exist in component table" ) );

    switch( result )
    {
    case wxID_CANCEL:
       return false;

    case wxID_NO:
       break;

    case wxID_YES:
       ApplyAllChanges();
       break;
    }

    return true;
}


void DIALOG_BOM_EDITOR::OnDialogClosed( wxCloseEvent& event )
{
    if( !CanCloseDialog() )
    {
        event.Veto();
    }
    else
        // Mandatory to call DIALOG_SHIM::OnCloseWindow( wxCloseEvent& aEvent )
        // and actually close the dialog
        event.Skip();
}


/* Struct for keeping track of schematic sheet changes
 * Stores:
 * SHEET_PATH - Schematic to apply changes to
 * PICKED_ITEMS_LIST - List of changes to apply
 */

typedef struct
{
    SCH_SHEET_PATH path;
    PICKED_ITEMS_LIST items;
} SheetUndoList;

void DIALOG_BOM_EDITOR::ApplyAllChanges()
{
    if( !m_bom->HaveFieldsChanged() )
        return;

     /**
     * As we may be saving changes across multiple sheets,
     * we need to first determine which changes need to be made to which sheet.
     * To this end, we perform the following:
     * 1. Save the "path" of the currently displayed sheet
     * 2. Create a MAP of <SheetPath:ChangeList> changes that need to be made
     * 3. Push UNDO actions to appropriate sheets
     * 4. Perform all the update actions
     * 5. Reset the view to the current sheet
     */

    auto currentSheet = m_parent->GetCurrentSheet();

    //! Create a map of changes required for each sheet
    std::map<wxString, SheetUndoList> undoSheetMap;

    // List of components that have changed
    auto changed = m_bom->GetChangedComponents();

    ITEM_PICKER picker;

    // Iterate through each of the components that were changed
    for( auto ref : changed )
    {
        // Extract the SCH_COMPONENT* object
        auto cmp = ref.GetComp();

        wxString path = ref.GetSheetPath().Path();

        // Push the component into the picker list
        picker = ITEM_PICKER( cmp, UR_CHANGED );
        picker.SetFlags( cmp->GetFlags() );

        /*
         * If there is not currently an undo list for the given sheet,
         * create an empty one
         */

        if( undoSheetMap.count( path ) == 0 )
        {
            SheetUndoList newList;

            newList.path = ref.GetSheetPath();

            undoSheetMap[path] = newList;
        }

        auto& pickerList = undoSheetMap[path];

        pickerList.items.PushItem( picker );
    }

    // Iterate through each sheet that needs updating
    for( auto it = undoSheetMap.begin(); it != undoSheetMap.end(); ++it )
    {
        auto undo = it->second;

        m_parent->SetCurrentSheet( undo.path );
        m_parent->SaveCopyInUndoList( undo.items, UR_CHANGED );
        m_parent->OnModify();
    }

    // Make all component changes
    m_bom->ApplyFieldChanges();

    // Redraw the current sheet and mark as dirty
    m_parent->Refresh();
    m_parent->OnModify();

    // Reset the view to where we left the user
    m_parent->SetCurrentSheet(currentSheet);

    // Instruct the table to set the current values as the new backup values
    m_bom->SetBackupPoint();
}


/**
 * Update the window title to reflect the contents of the table
 */
void DIALOG_BOM_EDITOR::UpdateTitle()
{
    wxString title;


    if( m_bom->GetColumnGrouping() )
    {
        title.Printf ( _( "Component table - %u components in %u groups" ),
                       m_bom->ComponentCount(),
                       (unsigned int) m_bom->Groups.size() );
    }
    else
        title.Printf ( _( "Component table - %u components" ),
                       m_bom->ComponentCount() );

    unsigned int count = m_bom->CountChangedComponents();

    if( count > 0 )
        title += wxString::Format( _( " - %u changed" ), count );

    // Update title only if it has changed, to avoid flicker created by
    // useless update, for instance when moving the mouse, because UpdateTitle()
    // is called by a wxUpdateUIEvent:
    if( GetTitle() != title )
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
    m_bom->SetComponents( refs, m_parent->GetTemplateFieldNames() );
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
        data.push_back( wxVariant( col->IsUsedToSort() ) ); // Column is used to sort

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
    case 2: // Column used to sort
        bomColumn->SetUsedToSort( bValue );
        m_bom->ReloadTable();
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

    bool changes = m_bom->HaveFieldsChanged();

    m_applyChangesButton->Enable( changes );
    m_revertChangesButton->Enable( changes );

    UpdateTitle();
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

void DIALOG_BOM_EDITOR::OnApplyFieldChanges( wxCommandEvent& event )
{
    ApplyAllChanges();
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

// Called when a cell is left-clicked
void DIALOG_BOM_EDITOR::OnTableItemActivated( wxDataViewEvent& event )
{
    /* TODO
     * - Focus on component selected in SCH_FRAME
     */

    event.Skip();
}


// Called when a cell is right-clicked
void DIALOG_BOM_EDITOR::OnTableItemContextMenu( wxDataViewEvent& event )
{
    /* TODO
     * - Display contect menu
     * - Option to revert local changes if changes have been made
     * - Option to select footprint if FOOTPRINT column selected
     */

    event.Skip();
}
