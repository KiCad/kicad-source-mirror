/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright 2017 Jean-Pierre Charras, jp.charras@wanadoo.fr
 * Copyright 1992-2017 KiCad Developers, see AUTHORS.txt for contributors.
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

/**
 * @file eeschema/dialogs/dialog_edit_components_libid.cpp
 * @brief Dialog to remap library id of components to an other library id
 */


#include <fctsys.h>
#include <dialog_edit_components_libid_base.h>
#include <schframe.h>
#include <class_drawpanel.h>
#include <sch_component.h>
#include <sch_reference_list.h>
#include <pgm_base.h>
#include <symbol_lib_table.h>

#include <wx/choicdlg.h>

#define COL_REFS 0
#define COL_CURR_LIBID 1
#define COL_NEW_LIBID 2

// a helper class to handle components to edit
class CMP_CANDIDATE
{
public:
    SCH_COMPONENT* m_Component; // the schematic component
    int         m_Row;          // the row index in m_grid
    SCH_SCREEN* m_Screen;       // the screen where m_Component lives
    wxString    m_Reference;    // the schematic reference, only to display it in list
    wxString    m_InitialLibId; // the Lib Id of the component before any change
    bool        m_IsOrphan;     // true if a component has no corresponding symbol found in libs

    CMP_CANDIDATE( SCH_COMPONENT* aComponent )
    {
        m_Component = aComponent;
        m_InitialLibId = m_Component->GetLibId().Format();
        m_Row = -1;
        m_IsOrphan = false;
        m_Screen = nullptr;
    }

    // Returns a string like mylib:symbol_name from the LIB_ID of the component
    wxString GetStringLibId()
    {
        return wxString( m_Component->GetLibId().Format().c_str() );
    }

    // Returns a string containing the reference of the component
    wxString GetSchematicReference()
    {
        return m_Reference;
    }
};


/**
 * DIALOG_EDIT_COMPONENTS_LIBID is a dialog to globally edit the LIB_ID of groups if components
 * having the same initial LIB_ID.
 * this is useful when you want:
 *  to move a symbol from a symbol library to an other symbol library
 *  to change the nickname of a library
 *  globally replace the symbol used by a group of components by an other symbol.
 */
class DIALOG_EDIT_COMPONENTS_LIBID : public DIALOG_EDIT_COMPONENTS_LIBID_BASE
{
public:
    DIALOG_EDIT_COMPONENTS_LIBID( SCH_EDIT_FRAME* aParent );

    bool IsSchematicModified() { return m_isModified; }

private:
    SCH_EDIT_FRAME* m_parent;
    bool m_isModified;      // set to true if the schematic is modified
    std::vector<int> m_OrphansRowIndexes;   // list of rows containing orphan lib_id

    std::vector<CMP_CANDIDATE> m_components;

    void initDlg();

    /**
     * Add a new row (new entry) in m_grid.
     * @param aRowId is the row index
     * @param aMarkRow = true to use bold/italic font in column COL_CURR_LIBID
     * @param aReferences is the value of cell( aRowId, COL_REFS)
     * @param aStrLibId is the value of cell( aRowId, COL_CURR_LIBID)
     */
    void AddRowToGrid( int aRowId, bool aMarkRow,
                       const wxString& aReferences, const wxString& aStrLibId );

    /// returns true if all new lib id are valid
    bool validateLibIds();

    /// Reverts all changes already made
    void revertChanges();

    /** run the lib browser and set the selected LIB_ID for row aRow
     * @param aRow is the row to edit
     * @return false if the command was aborted
     */
    bool setLibIdByBrowser( int aRow );

    // Events handlers

    // called on a right click or a left double click:
	void onCellBrowseLib( wxGridEvent& event ) override;

    // Apply changes, but do not close the dialog
	void onApplyButton( wxCommandEvent& event ) override;

    // Cancel all changes, and close the dialog
	void onCancel( wxCommandEvent& event ) override
    {
        if( m_isModified )
            revertChanges();
        event.Skip();
    }

	void onButtonBrowseLibraries( wxCommandEvent& event ) override;

    // Undo all changes, and clear the list of new lib_ids
	void onUndoChangesButton( wxCommandEvent& event ) override;

    // Try to find a candidate for non existing symbols
	void onClickOrphansButton( wxCommandEvent& event ) override;

    // UI event, to enable/disable buttons
	void updateUIChangesButton( wxUpdateUIEvent& event ) override
    {
        m_buttonUndo->Enable( m_isModified );
    }

	void updateUIBrowseButton( wxUpdateUIEvent& event ) override
    {
        wxArrayInt rows = m_grid->GetSelectedRows();
        m_buttonBrowseLibs->Enable( rows.GetCount() == 1 );
    }

    // Automatically called when click on OK button
    bool TransferDataFromWindow() override;
};


DIALOG_EDIT_COMPONENTS_LIBID::DIALOG_EDIT_COMPONENTS_LIBID( SCH_EDIT_FRAME* aParent )
    :DIALOG_EDIT_COMPONENTS_LIBID_BASE( aParent )
{
    m_parent = aParent;
    initDlg();

    // Gives a min size to display m_grid, now it is populated:
    int minwidth = 30   // a margin
                   + m_grid->GetRowLabelSize() + m_grid->GetColSize( COL_REFS )
                   + m_grid->GetColSize( COL_CURR_LIBID ) + m_grid->GetColSize( COL_NEW_LIBID );
    m_panelGrid->SetMinSize( wxSize( minwidth, -1) );
    Layout();

    // Now all widgets have the size fixed, call FinishDialogSettings
    FinishDialogSettings();
}


// A sort compare function to sort components list by LIB_ID
// inside the group of same LIB_ID, sort by reference
static bool sort_by_libid( const CMP_CANDIDATE& cmp1, const CMP_CANDIDATE& cmp2 )
{
    if( cmp1.m_Component->GetLibId() == cmp2.m_Component->GetLibId() )
        return cmp1.m_Reference.Cmp( cmp2.m_Reference ) < 0;

    return cmp1.m_Component->GetLibId() < cmp2.m_Component->GetLibId();
}


void DIALOG_EDIT_COMPONENTS_LIBID::initDlg()
{
    m_isModified = false;

    // Build the component list:
#if 0
    // This option build a component list that works fine to edit LIB_ID fields, but does not display
    // all components in a complex hierarchy.
    // the list is shorter, but can be look like there are missing components in list
    SCH_SCREENS screens;

    for( SCH_SCREEN* screen = screens.GetFirst(); screen; screen = screens.GetNext() )
    {
        for( SCH_ITEM* item = screen->GetDrawItems(); item; item = item->Next() )
        {
            if( item->Type() == SCH_COMPONENT_T )
            {
                CMP_CANDIDATE candidate( static_cast< SCH_COMPONENT* >( item ) );
                candidate.m_Screen = screen;
                candidate.m_Reference = candidate.m_Component->GetField( REFERENCE )->GetFullyQualifiedText();
                m_components.push_back( candidate );
            }
        }
    }
#else
    // This option build the full component list
    // In complex hierarchies, the same component is in fact duplicated, but
    // it is listed with different references (one by sheet instance)
    // the list is larger and looks like it contains all components
    SCH_SHEET_LIST sheets( g_RootSheet );
    SCH_REFERENCE_LIST references;
    // build the full list of components including component having no symbol in loaded libs
    // (orphan components)
    sheets.GetComponents( references, /* include power symbols */true,
                          /* include orphan components */true );

    for( unsigned ii = 0; ii < references.GetCount(); ii++ )
    {
        SCH_REFERENCE& item = references[ii];
        CMP_CANDIDATE candidate( item.GetComp() );
        candidate.m_Screen = item.GetSheetPath().LastScreen();
        SCH_SHEET_PATH sheetpath = item.GetSheetPath();
        candidate.m_Reference = candidate.m_Component->GetRef( &sheetpath );
        // For multi units per package , add unit id.
        // however, there is a problem: the unit id stored is always >= 1
        // and 1 for no multi units.
        // so add unit id only if unit > 1 if the unit count is > 1
        // (can be 0 if the symbol is not found)
        int unit = candidate.m_Component->GetUnitSelection( &sheetpath );
        int unitcount = candidate.m_Component->GetUnitCount();
        candidate.m_IsOrphan = ( unitcount == 0 );

        if( unitcount > 1 || unit > 1 )
        {
            candidate.m_Reference << wxChar( ('A' + unit -1) );
        }

        m_components.push_back( candidate );
    }
#endif

    if( m_components.size() == 0 )
        return;

    // now sort by lib id to create groups of items having the same lib id
    std::sort( m_components.begin(), m_components.end(), sort_by_libid );

    // Now, fill m_grid
    wxString last_str_libid = m_components.front().GetStringLibId();
    int row = 0;
    wxString refs;
    bool mark_cell = m_components.front().m_IsOrphan;
    CMP_CANDIDATE* cmp = nullptr;

    for( unsigned ii = 0; ii < m_components.size(); ii++ )
    {
        cmp = &m_components[ii];

        wxString str_libid = cmp->GetStringLibId();

        if( last_str_libid != str_libid )
        {
            // Add last group to grid
            AddRowToGrid( row, mark_cell, refs, last_str_libid );

            // prepare next entry
            mark_cell = cmp->m_IsOrphan;
            last_str_libid = str_libid;
            refs.Empty();
            row++;
        }

        if( !refs.IsEmpty() )
            refs += " ";

        refs += cmp->GetSchematicReference();
        cmp->m_Row = row;
    }

    // Add last component group:
    AddRowToGrid( row, mark_cell, refs, last_str_libid );

    m_grid->AutoSizeColumn( COL_CURR_LIBID );
    // ensure the column title is correctly displayed
    // (the min size is already fixed by AutoSizeColumn() )
    m_grid->AutoSizeColLabelSize( COL_CURR_LIBID );

    // Gives a similar width to COL_NEW_LIBID because it can contains similar strings
    if( m_grid->GetColSize( COL_CURR_LIBID ) > m_grid->GetColSize( COL_NEW_LIBID ) )
        m_grid->SetColSize( COL_NEW_LIBID, m_grid->GetColSize( COL_CURR_LIBID ) );
    // ensure the column title is correctly displayed
    m_grid->SetColMinimalWidth( COL_NEW_LIBID, m_grid->GetColSize( COL_NEW_LIBID ) );
    m_grid->AutoSizeColLabelSize( COL_NEW_LIBID );

    // Allows only the selection by row
    m_grid->SetSelectionMode( wxGrid::wxGridSelectRows );

    m_buttonOrphanItems->Enable( m_OrphansRowIndexes.size() > 0 );
}


void DIALOG_EDIT_COMPONENTS_LIBID::AddRowToGrid( int aRowId, bool aMarkRow,
                    const wxString& aReferences, const wxString& aStrLibId )
{
    if( aMarkRow )      // a orphan component exists, set m_AsOrphanCmp as true
        m_OrphansRowIndexes.push_back( aRowId );

    int row = aRowId;

    if( m_grid->GetNumberRows() <= row )
        m_grid->AppendRows();

    m_grid->SetCellValue( row, COL_REFS, aReferences );
    m_grid->SetReadOnly( row, COL_REFS );

    m_grid->SetCellValue( row, COL_CURR_LIBID, aStrLibId );
    m_grid->SetReadOnly( row, COL_CURR_LIBID );

    if( aMarkRow ) // A symbol is not existing in libraries: mark the cell
    {
        wxFont font = m_grid->GetDefaultCellFont();
        font.MakeBold();
        font.MakeItalic();
        m_grid->SetCellFont( row, COL_CURR_LIBID, font );
    }

    m_grid->SetCellRenderer( row, COL_REFS, new wxGridCellAutoWrapStringRenderer);
    m_grid->AutoSizeRow( row, false );
}


bool DIALOG_EDIT_COMPONENTS_LIBID::validateLibIds()
{
    int row_max = m_grid->GetNumberRows() - 1;

    for( int row = 0; row <= row_max; row++ )
    {
        wxString new_libid = m_grid->GetCellValue( row, COL_NEW_LIBID );

        if( new_libid.IsEmpty() )
            continue;

        // a new lib id is found. validate this new value
        LIB_ID id;
        id.Parse( new_libid );

        if( !id.IsValid() )
        {
            wxString msg;
            msg.Printf( _( "Symbol library identifier \"%s\" is not valid at row %d!" ), new_libid, row+1 );
            wxMessageBox( msg );
            return false;
        }
    }

    return true;
}


void DIALOG_EDIT_COMPONENTS_LIBID::onApplyButton( wxCommandEvent& event )
{
    if( TransferDataFromWindow() )
        m_parent->GetCanvas()->Refresh();
}


void DIALOG_EDIT_COMPONENTS_LIBID::onUndoChangesButton( wxCommandEvent& event )
{
    revertChanges();

    int row_max = m_grid->GetNumberRows() - 1;

    for( int row = 0; row <= row_max; row++ )
    {
        m_grid->SetCellValue( row, COL_NEW_LIBID, wxEmptyString );
    }

    m_isModified = false;
}


void DIALOG_EDIT_COMPONENTS_LIBID::onCellBrowseLib( wxGridEvent& event )
{
    int row = event.GetRow();
    m_grid->SelectRow( row );   // only for user, to show the selected line

    setLibIdByBrowser( row );

}


void DIALOG_EDIT_COMPONENTS_LIBID::onButtonBrowseLibraries( wxCommandEvent& event )
{
    wxArrayInt rows = m_grid->GetSelectedRows();

    if( rows.GetCount() != 1 )  // Should not occur, because the button is disabled
        return;

    setLibIdByBrowser( rows[0] );
}


void DIALOG_EDIT_COMPONENTS_LIBID::onClickOrphansButton( wxCommandEvent& event )
{
    std::vector< wxString > libs = Prj().SchSymbolLibTable()->GetLogicalLibs();
    wxArrayString aliasNames;
    wxArrayString candidateSymbNames;

    unsigned fixesCount = 0;

    // Try to find a candidate for non existing symbols in any loaded library
    for( unsigned ii = 0; ii < m_OrphansRowIndexes.size(); ii++ )
    {
        wxString orphanLibid = m_grid->GetCellValue( m_OrphansRowIndexes[ii], COL_CURR_LIBID );
        int grid_row_idx = m_OrphansRowIndexes[ii]; //row index in m_grid for the current item

        LIB_ID curr_libid( orphanLibid );
        wxString symbName = curr_libid.GetLibItemName();
        // number of full LIB_ID candidates (because we search for a symbol name
        // inside all avaiable libraries, perhaps the same symbol name can be found
        // in more than one library, giving ambiguity
        int libIdCandidateCount = 0;
        candidateSymbNames.Clear();

        // now try to fin a candidate
        for( auto &lib : libs )
        {
            aliasNames.Clear();

            try
            {
                Prj().SchSymbolLibTable()->EnumerateSymbolLib( lib, aliasNames );
            }
            catch( const IO_ERROR& e ) {}   // ignore, it is handled below

            if( aliasNames.IsEmpty() )
                continue;

            // Find a symbol name in symbols inside this library:
            int index = aliasNames.Index( symbName );

            if( index != wxNOT_FOUND )
            {
                // a candidate is found!
                libIdCandidateCount++;
                wxString newLibid = lib + ':' + symbName;

                // Uses the first found. Most of time, it is alone.
                // Others will be stored in a candidate list
                if( libIdCandidateCount <= 1 )
                {
                    m_grid->SetCellValue( grid_row_idx, COL_NEW_LIBID, newLibid );
                    candidateSymbNames.Add( m_grid->GetCellValue( grid_row_idx, COL_NEW_LIBID ) );
                    fixesCount++;
                }
                else    // Store other candidates for later selection
                {
                    candidateSymbNames.Add( newLibid );
                }
            }
        }

        // If more than one LIB_ID candidate, ask for selection between candidates:
        if( libIdCandidateCount > 1 )
        {
            // Mainly for user: select the row being edited
            m_grid->SelectRow( grid_row_idx );

            wxString msg;
            msg.Printf( _( "Available Candidates for %s " ),
                        m_grid->GetCellValue( grid_row_idx, COL_CURR_LIBID ) );

            wxSingleChoiceDialog dlg ( this, msg,
                wxString::Format( _( "Candidates count %d " ), libIdCandidateCount ),
                candidateSymbNames );

            if( dlg.ShowModal() == wxID_OK )
                m_grid->SetCellValue( grid_row_idx, COL_NEW_LIBID, dlg.GetStringSelection() );
        }
    }

    if( fixesCount < m_OrphansRowIndexes.size() )   // Not all orphan components are fixed
    {
        wxMessageBox( wxString::Format( _( "%u link(s) mapped, %d not found" ),
                      fixesCount, m_OrphansRowIndexes.size() - fixesCount ) );
    }
    else
        wxMessageBox( wxString::Format( _( "All %u link(s) resolved" ), fixesCount ) );
}


bool DIALOG_EDIT_COMPONENTS_LIBID::setLibIdByBrowser( int aRow )
{
#if 0
    // Use dialog symbol selector to choose a symbol
    SCH_BASE_FRAME::HISTORY_LIST dummy;
    SCH_BASE_FRAME::COMPONENT_SELECTION sel =
                m_parent->SelectComponentFromLibrary( NULL, dummy, true, 0, 0 );
#else
    // Use library viewer to choose a symbol
    LIB_ID aPreselectedLibid;
    SCH_BASE_FRAME::COMPONENT_SELECTION sel =
            m_parent->SelectComponentFromLibBrowser( NULL, aPreselectedLibid, 0, 0 );
#endif

    if( sel.LibId.empty() )     // command aborted
        return false;

    if( !sel.LibId.IsValid() )  // Should not occur
    {
        wxMessageBox( _( "Invalid symbol library identifier" ) );
        return false;
    }

    wxString new_libid;
    new_libid = sel.LibId.Format();

    m_grid->SetCellValue( aRow, COL_NEW_LIBID, new_libid );

    return true;
}


bool DIALOG_EDIT_COMPONENTS_LIBID::TransferDataFromWindow()
{
    if( !validateLibIds() )
        return false;

    bool change = false;
    int row_max = m_grid->GetNumberRows() - 1;

    for( int row = 0; row <= row_max; row++ )
    {
        wxString new_libid = m_grid->GetCellValue( row, COL_NEW_LIBID );

        if( new_libid.IsEmpty() || new_libid == m_grid->GetCellValue( row, COL_CURR_LIBID ) )
            continue;

        // a new lib id is found and was already validated.
        // set this new value
        LIB_ID id;
        id.Parse( new_libid );

        for( CMP_CANDIDATE& cmp : m_components )
        {
            if( cmp.m_Row != row )
                continue;

            cmp.m_Component->SetLibId( id );
            change = true;
            cmp.m_Screen->SetModify();
        }
    }

    if( change )
    {
        m_isModified = true;
        SCH_SCREENS schematic;
        schematic.UpdateSymbolLinks( true );
    }

    return true;
}


void DIALOG_EDIT_COMPONENTS_LIBID::revertChanges()
{
    bool change = false;
    int row_max = m_grid->GetNumberRows() - 1;

    for( int row = 0; row <= row_max; row++ )
    {
        for( CMP_CANDIDATE& cmp : m_components )
        {
            if( cmp.m_Row != row )
                continue;

            LIB_ID id;
            id.Parse( cmp.m_InitialLibId );

            if( cmp.m_Component->GetLibId() != id )
            {
                cmp.m_Component->SetLibId( id );
                change = true;
            }
        }
    }

    if( change )
    {
        SCH_SCREENS schematic;
        schematic.UpdateSymbolLinks( true );
        m_parent->GetCanvas()->Refresh();
    }
}


bool InvokeDialogEditComponentsLibId( SCH_EDIT_FRAME* aCaller )
{
    DIALOG_EDIT_COMPONENTS_LIBID dlg( aCaller );
    dlg.ShowModal();

    return dlg.IsSchematicModified();
}