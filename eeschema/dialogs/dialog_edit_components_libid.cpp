/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright 2017 Jean-Pierre Charras, jp.charras@wanadoo.fr
 * Copyright 1992-2018 KiCad Developers, see AUTHORS.txt for contributors.
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
 * @brief Dialog to remap library id of components to another library id
 */


#include <fctsys.h>
#include <sch_edit_frame.h>
#include <sch_draw_panel.h>
#include <sch_component.h>
#include <sch_reference_list.h>
#include <pgm_base.h>
#include <symbol_lib_table.h>
#include <widgets/wx_grid.h>

#include <dialog_edit_components_libid_base.h>
#include <wx/tokenzr.h>
#include <grid_tricks.h>
#include <widgets/grid_text_button_helpers.h>

#define COL_REFS 0
#define COL_CURR_LIBID 1
#define COL_NEW_LIBID 2

// a re-implementation of wxGridCellAutoWrapStringRenderer to allow workaround to autorowsize bug
class GRIDCELL_AUTOWRAP_STRINGRENDERER : public wxGridCellAutoWrapStringRenderer
{
public:
    int GetHeight( wxDC& aDC, wxGrid* aGrid, int aRow, int aCol );

    wxGridCellRenderer *Clone() const override
    { return new GRIDCELL_AUTOWRAP_STRINGRENDERER; }

private:
    // HELPER ROUTINES UNCHANGED FROM wxWidgets IMPLEMENTATION

    wxArrayString GetTextLines( wxGrid& grid,
                                wxDC& dc,
                                const wxGridCellAttr& attr,
                                const wxRect& rect,
                                int row, int col);

    // Helper methods of GetTextLines()

    // Break a single logical line of text into several physical lines, all of
    // which are added to the lines array. The lines are broken at maxWidth and
    // the dc is used for measuring text extent only.
    void BreakLine(wxDC& dc,
                   const wxString& logicalLine,
                   wxCoord maxWidth,
                   wxArrayString& lines);

    // Break a word, which is supposed to be wider than maxWidth, into several
    // lines, which are added to lines array and the last, incomplete, of which
    // is returned in line output parameter.
    //
    // Returns the width of the last line.
    wxCoord BreakWord(wxDC& dc,
                      const wxString& word,
                      wxCoord maxWidth,
                      wxArrayString& lines,
                      wxString& line);
};


// PRIVATE METHOD UNCHANGED FROM wxWidgets IMPLEMENTATION
wxArrayString
GRIDCELL_AUTOWRAP_STRINGRENDERER::GetTextLines(wxGrid& grid,
                                               wxDC& dc,
                                               const wxGridCellAttr& attr,
                                               const wxRect& rect,
                                               int row, int col)
{
    dc.SetFont(attr.GetFont());
    const wxCoord maxWidth = rect.GetWidth();

    // Transform logical lines into physical ones, wrapping the longer ones.
    const wxArrayString
            logicalLines = wxSplit(grid.GetCellValue(row, col), '\n', '\0');

    // Trying to do anything if the column is hidden anyhow doesn't make sense
    // and we run into problems in BreakLine() in this case.
    if ( maxWidth <= 0 )
        return logicalLines;

    wxArrayString physicalLines;
    for ( wxArrayString::const_iterator it = logicalLines.begin();
          it != logicalLines.end();
          ++it )
    {
        const wxString& line = *it;

        if ( dc.GetTextExtent(line).x > maxWidth )
        {
            // Line does not fit, break it up.
            BreakLine(dc, line, maxWidth, physicalLines);
        }
        else // The entire line fits as is
        {
            physicalLines.push_back(line);
        }
    }

    return physicalLines;
}


// PRIVATE METHOD UNCHANGED FROM wxWidgets IMPLEMENTATION
void
GRIDCELL_AUTOWRAP_STRINGRENDERER::BreakLine(wxDC& dc,
                                            const wxString& logicalLine,
                                            wxCoord maxWidth,
                                            wxArrayString& lines)
{
    wxCoord lineWidth = 0;
    wxString line;

    // For each word
    wxStringTokenizer wordTokenizer(logicalLine, wxS(" \t"), wxTOKEN_RET_DELIMS);
    while ( wordTokenizer.HasMoreTokens() )
    {
        const wxString word = wordTokenizer.GetNextToken();
        const wxCoord wordWidth = dc.GetTextExtent(word).x;
        if ( lineWidth + wordWidth < maxWidth )
        {
            // Word fits, just add it to this line.
            line += word;
            lineWidth += wordWidth;
        }
        else
        {
            // Word does not fit, check whether the word is itself wider that
            // available width
            if ( wordWidth < maxWidth )
            {
                // Word can fit in a new line, put it at the beginning
                // of the new line.
                lines.push_back(line);
                line = word;
                lineWidth = wordWidth;
            }
            else // Word cannot fit in available width at all.
            {
                if ( !line.empty() )
                {
                    lines.push_back(line);
                    line.clear();
                    lineWidth = 0;
                }

                // Break it up in several lines.
                lineWidth = BreakWord(dc, word, maxWidth, lines, line);
            }
        }
    }

    if ( !line.empty() )
        lines.push_back(line);
}


// PRIVATE METHOD UNCHANGED FROM wxWidgets IMPLEMENTATION
wxCoord
GRIDCELL_AUTOWRAP_STRINGRENDERER::BreakWord(wxDC& dc,
                                            const wxString& word,
                                            wxCoord maxWidth,
                                            wxArrayString& lines,
                                            wxString& line)
{
    wxArrayInt widths;
    dc.GetPartialTextExtents(word, widths);

    // TODO: Use binary search to find the first element > maxWidth.
    const unsigned count = widths.size();
    unsigned n;
    for ( n = 0; n < count; n++ )
    {
        if ( widths[n] > maxWidth )
            break;
    }

    if ( n == 0 )
    {
        // This is a degenerate case: the first character of the word is
        // already wider than the available space, so we just can't show it
        // completely and have to put the first character in this line.
        n = 1;
    }

    lines.push_back(word.substr(0, n));

    // Check if the remainder of the string fits in one line.
    //
    // Unfortunately we can't use the existing partial text extents as the
    // extent of the remainder may be different when it's rendered in a
    // separate line instead of as part of the same one, so we have to
    // recompute it.
    const wxString rest = word.substr(n);
    const wxCoord restWidth = dc.GetTextExtent(rest).x;
    if ( restWidth <= maxWidth )
    {
        line = rest;
        return restWidth;
    }

    // Break the rest of the word into lines.
    //
    // TODO: Perhaps avoid recursion? The code is simpler like this but using a
    // loop in this function would probably be more efficient.
    return BreakWord(dc, rest, maxWidth, lines, line);
}


#define GRID_CELL_MARGIN   4

int GRIDCELL_AUTOWRAP_STRINGRENDERER::GetHeight( wxDC& aDC, wxGrid* aGrid, int aRow, int aCol )
{
    wxGridCellAttr* attr = aGrid->GetOrCreateCellAttr( aRow, aCol );
    wxRect rect;

    aDC.SetFont( attr->GetFont() );
    rect.SetWidth( aGrid->GetColSize( aCol ) - ( 2 * GRID_CELL_MARGIN ) );

    const size_t numLines = GetTextLines( *aGrid, aDC, *attr, rect, aRow, aCol ).size();
    const int textHeight = numLines * aDC.GetCharHeight();

    attr->DecRef();

    return textHeight + ( 2 * GRID_CELL_MARGIN );
}


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
        return m_Component->GetLibId().GetUniStringLibId();
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
 *  to move a symbol from a symbol library to another symbol library
 *  to change the nickname of a library
 *  globally replace the symbol used by a group of components by another symbol.
 */
class DIALOG_EDIT_COMPONENTS_LIBID : public DIALOG_EDIT_COMPONENTS_LIBID_BASE
{
public:
    DIALOG_EDIT_COMPONENTS_LIBID( SCH_EDIT_FRAME* aParent );
    ~DIALOG_EDIT_COMPONENTS_LIBID() override;

    bool IsSchematicModified() { return m_isModified; }

private:
    SCH_EDIT_FRAME*  m_parent;
    bool             m_isModified;          // set to true if the schematic is modified
    std::vector<int> m_OrphansRowIndexes;   // list of rows containing orphan lib_id

    std::vector<CMP_CANDIDATE> m_components;

    GRIDCELL_AUTOWRAP_STRINGRENDERER* m_autoWrapRenderer;

    void initDlg();

    /**
     * Add a new row (new entry) in m_grid.
     * @param aMarkRow = true to use bold/italic font in column COL_CURR_LIBID
     * @param aReferences is the value of cell( aRowId, COL_REFS)
     * @param aStrLibId is the value of cell( aRowId, COL_CURR_LIBID)
     */
    void AddRowToGrid( bool aMarkRow, const wxString& aReferences, const wxString& aStrLibId );

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

        // Just skipping the event doesn't work after the library browser was run
        if( IsQuasiModal() )
            EndQuasiModal( wxID_CANCEL );
        else
            event.Skip();
    }

    // Undo all changes, and clear the list of new lib_ids
	void onUndoChangesButton( wxCommandEvent& event ) override;

    // Try to find a candidate for non existing symbols
	void onClickOrphansButton( wxCommandEvent& event ) override;

    // UI event, to enable/disable buttons
	void updateUIChangesButton( wxUpdateUIEvent& event ) override
    {
        m_buttonUndo->Enable( m_isModified );
    }

    // Automatically called when click on OK button
    bool TransferDataFromWindow() override;

    void AdjustGridColumns( int aWidth );

    void OnSizeGrid( wxSizeEvent& event ) override;
};


DIALOG_EDIT_COMPONENTS_LIBID::DIALOG_EDIT_COMPONENTS_LIBID( SCH_EDIT_FRAME* aParent )
    :DIALOG_EDIT_COMPONENTS_LIBID_BASE( aParent )
{
    m_parent = aParent;
    m_autoWrapRenderer = new GRIDCELL_AUTOWRAP_STRINGRENDERER;

    m_grid->PushEventHandler( new GRID_TRICKS( m_grid ) );

    initDlg();

    FinishDialogSettings();
}


DIALOG_EDIT_COMPONENTS_LIBID::~DIALOG_EDIT_COMPONENTS_LIBID()
{
    // Delete the GRID_TRICKS.
    m_grid->PopEventHandler( true );

    m_autoWrapRenderer->DecRef();
}


// A sort compare function to sort components list by LIB_ID and then reference
static bool sort_by_libid( const CMP_CANDIDATE& cmp1, const CMP_CANDIDATE& cmp2 )
{
    if( cmp1.m_Component->GetLibId() == cmp2.m_Component->GetLibId() )
        return cmp1.m_Reference.Cmp( cmp2.m_Reference ) < 0;

    return cmp1.m_Component->GetLibId() < cmp2.m_Component->GetLibId();
}


void DIALOG_EDIT_COMPONENTS_LIBID::initDlg()
{
    // Clear the FormBuilder rows
    m_grid->DeleteRows( 0, m_grid->GetNumberRows() );

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
            AddRowToGrid( mark_cell, refs, last_str_libid );

            // prepare next entry
            mark_cell = cmp->m_IsOrphan;
            last_str_libid = str_libid;
            refs.Empty();
            row++;
        }

        if( !refs.IsEmpty() )
            refs += wxT( ", " );

        refs += cmp->GetSchematicReference();
        cmp->m_Row = row;
    }

    // Add last component group:
    AddRowToGrid( mark_cell, refs, last_str_libid );

    // Allows only the selection by row
    m_grid->SetSelectionMode( wxGrid::wxGridSelectRows );

    m_buttonOrphanItems->Enable( m_OrphansRowIndexes.size() > 0 );
    Layout();
}


void DIALOG_EDIT_COMPONENTS_LIBID::AddRowToGrid( bool aMarkRow, const wxString& aReferences,
                                                 const wxString& aStrLibId )
{
    int row = m_grid->GetNumberRows();

    if( aMarkRow )      // a orphan component exists, set m_AsOrphanCmp as true
        m_OrphansRowIndexes.push_back( row );

    m_grid->AppendRows( 1 );

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

    m_grid->SetCellRenderer( row, COL_REFS, m_autoWrapRenderer->Clone() );

    // wxWidgets' AutoRowHeight fails when used with wxGridCellAutoWrapStringRenderer
    // (fixed in 2014, but didn't get in to wxWidgets 3.0.2)
    wxClientDC dc( this );
    m_grid->SetRowSize( row, m_autoWrapRenderer->GetHeight( dc, m_grid, row, COL_REFS ) );

    // set new libid column browse button
    wxGridCellAttr* attr = new wxGridCellAttr;
    attr->SetEditor( new GRID_CELL_SYMBOL_ID_EDITOR( this, aStrLibId ) );
    m_grid->SetAttr( row, COL_NEW_LIBID, attr );
}


bool DIALOG_EDIT_COMPONENTS_LIBID::validateLibIds()
{
    if( !m_grid->CommitPendingChanges() )
        return false;

    int row_max = m_grid->GetNumberRows() - 1;

    for( int row = 0; row <= row_max; row++ )
    {
        wxString new_libid = m_grid->GetCellValue( row, COL_NEW_LIBID );

        if( new_libid.IsEmpty() )
            continue;

        // a new lib id is found. validate this new value
        LIB_ID id;
        id.Parse( new_libid, LIB_ID::ID_SCH );

        if( !id.IsValid() )
        {
            wxString msg;
            msg.Printf( _( "Symbol library identifier \"%s\" is not valid." ), new_libid );
            wxMessageBox( msg );

            m_grid->SetFocus();
            m_grid->MakeCellVisible( row, COL_NEW_LIBID );
            m_grid->SetGridCursor( row, COL_NEW_LIBID );

            m_grid->EnableCellEditControl( true );
            m_grid->ShowCellEditControl();

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

        LIB_ID curr_libid;
        curr_libid.Parse( orphanLibid, LIB_ID::ID_SCH, true );
        wxString symbName = curr_libid.GetLibItemName();
        // number of full LIB_ID candidates (because we search for a symbol name
        // inside all avaiable libraries, perhaps the same symbol name can be found
        // in more than one library, giving ambiguity
        int libIdCandidateCount = 0;
        candidateSymbNames.Clear();

        // now try to find a candidate
        for( auto &lib : libs )
        {
            aliasNames.Clear();

            try
            {
                Prj().SchSymbolLibTable()->EnumerateSymbolLib( lib, aliasNames );
            }
            catch( const IO_ERROR& ) {}   // ignore, it is handled below

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
        wxMessageBox( wxString::Format( _( "%u link(s) mapped, %u not found" ),
                                        fixesCount,
                                        (unsigned) m_OrphansRowIndexes.size() - fixesCount ) );
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
                m_frame->SelectComponentFromLibrary( NULL, dummy, true, 0, 0, false );
#else
    // Use library viewer to choose a symbol
    LIB_ID aPreselectedLibid;
    wxString current = m_grid->GetCellValue( aRow, COL_NEW_LIBID );

    if( current.IsEmpty() )
        current = m_grid->GetCellValue( aRow, COL_CURR_LIBID );

    if( !current.IsEmpty() )
        aPreselectedLibid.Parse( current, LIB_ID::ID_SCH, true );

    SCH_BASE_FRAME::COMPONENT_SELECTION sel =
            m_parent->SelectComponentFromLibBrowser( this, NULL, aPreselectedLibid, 0, 0 );
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

        // A new lib id is found and was already validated.
        LIB_ID id;
        id.Parse( new_libid, LIB_ID::ID_SCH, true );

        for( CMP_CANDIDATE& cmp : m_components )
        {
            if( cmp.m_Row != row )
                continue;

            SCH_FIELD* value = cmp.m_Component->GetField( VALUE );

            // If value is a proxy for the itemName then make sure it gets updated
            if( cmp.m_Component->GetLibId().GetLibItemName().wx_str() == value->GetText() )
                value->SetText( id.GetLibItemName().wx_str() );

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
            id.Parse( cmp.m_InitialLibId, LIB_ID::ID_SCH, true );

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


void DIALOG_EDIT_COMPONENTS_LIBID::AdjustGridColumns( int aWidth )
{
    // Account for scroll bars
    aWidth -= ( m_grid->GetSize().x - m_grid->GetClientSize().x );

    int colWidth = aWidth / 3;

    m_grid->SetColSize( COL_REFS, colWidth );
    aWidth -= colWidth;

    colWidth = 0;
    for( int row = 0; row < m_grid->GetNumberRows(); ++row )
    {
        wxString cellValue = m_grid->GetCellValue( row, COL_CURR_LIBID );
        colWidth = std::max( colWidth, GetTextSize( cellValue, m_grid ).x );
    }

    colWidth += 20;
    m_grid->SetColSize( COL_CURR_LIBID, colWidth );
    aWidth -= colWidth;

    colWidth = 0;
    for( int row = 0; row < m_grid->GetNumberRows(); ++row )
    {
        wxString cellValue = m_grid->GetCellValue( row, COL_NEW_LIBID );
        colWidth = std::max( colWidth, GetTextSize( cellValue, m_grid ).x );
    }

    colWidth += 20;
    m_grid->SetColSize( COL_NEW_LIBID, std::max( colWidth, aWidth ) );
}


void DIALOG_EDIT_COMPONENTS_LIBID::OnSizeGrid( wxSizeEvent& event )
{
    AdjustGridColumns( event.GetSize().GetX() );

    wxClientDC dc( this );

    // wxWidgets' AutoRowHeight fails when used with wxGridCellAutoWrapStringRenderer
    for( int row = 0; row < m_grid->GetNumberRows(); ++row )
        m_grid->SetRowSize( row, m_autoWrapRenderer->GetHeight( dc, m_grid, row, COL_REFS ) );

    event.Skip();
}


bool InvokeDialogEditComponentsLibId( SCH_EDIT_FRAME* aCaller )
{
    // This dialog itself subsequently can invoke a KIWAY_PLAYER as a quasimodal
    // frame. Therefore this dialog as a modal frame parent, MUST be run under
    // quasimodal mode for the quasimodal frame support to work.  So don't use
    // the QUASIMODAL macros here.
    DIALOG_EDIT_COMPONENTS_LIBID dlg( aCaller );
    // DO NOT use ShowModal() here, otherwise the library browser will not work
    // properly.
    dlg.ShowQuasiModal();

    return dlg.IsSchematicModified();
}
