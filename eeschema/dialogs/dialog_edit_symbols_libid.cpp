/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright 2017 Jean-Pierre Charras, jp.charras@wanadoo.fr
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

/**
 * @file eeschema/dialogs/dialog_edit_symbols_libid.cpp
 * @brief Dialog to remap library id of symbols to another library id
 */


#include <confirm.h>
#include <sch_commit.h>
#include <sch_edit_frame.h>
#include <sch_symbol.h>
#include <sch_reference_list.h>
#include <schematic.h>
#include <libraries/symbol_library_adapter.h>
#include <trace_helpers.h>
#include <widgets/wx_grid.h>

#include <dialog_edit_symbols_libid_base.h>
#include <wx/tokenzr.h>
#include <wx/choicdlg.h>
#include <wx/dcclient.h>
#include <wx/msgdlg.h>
#include <grid_tricks.h>
#include <widgets/grid_text_button_helpers.h>
#include <kiplatform/ui.h>
#include <string_utils.h>
#include <project_sch.h>


#define COL_REFS 0
#define COL_CURR_LIBID 1
#define COL_NEW_LIBID 2

// a re-implementation of wxGridCellAutoWrapStringRenderer to allow workaround to autorowsize bug
class GRIDCELL_AUTOWRAP_STRINGRENDERER : public wxGridCellAutoWrapStringRenderer
{
public:
    int GetHeight( wxDC& aDC, wxGrid* aGrid, int aRow, int aCol );

    wxGridCellRenderer *Clone() const override
    {
        return new GRIDCELL_AUTOWRAP_STRINGRENDERER;
    }

private:
    // HELPER ROUTINES UNCHANGED FROM wxWidgets IMPLEMENTATION

    wxArrayString GetTextLines( wxGrid& grid, wxDC& dc, const wxGridCellAttr& attr,
                                const wxRect& rect, int row, int col );

    // Helper methods of GetTextLines()

    // Break a single logical line of text into several physical lines, all of
    // which are added to the lines array. The lines are broken at maxWidth and
    // the dc is used for measuring text extent only.
    void BreakLine( wxDC& dc, const wxString& logicalLine, wxCoord maxWidth, wxArrayString& lines );

    // Break a word, which is supposed to be wider than maxWidth, into several
    // lines, which are added to lines array and the last, incomplete, of which
    // is returned in line output parameter.
    //
    // Returns the width of the last line.
    wxCoord BreakWord( wxDC& dc, const wxString& word, wxCoord maxWidth, wxArrayString& lines,
                       wxString& line );
};


// PRIVATE METHOD UNCHANGED FROM wxWidgets IMPLEMENTATION
wxArrayString GRIDCELL_AUTOWRAP_STRINGRENDERER::GetTextLines( wxGrid& grid, wxDC& dc,
                                                              const wxGridCellAttr& attr,
                                                              const wxRect& rect, int row, int col )
{
    dc.SetFont( attr.GetFont() );
    const wxCoord maxWidth = rect.GetWidth();

    // Transform logical lines into physical ones, wrapping the longer ones.
    const wxArrayString logicalLines = wxSplit( grid.GetCellValue( row, col ), '\n', '\0' );

    // Trying to do anything if the column is hidden anyhow doesn't make sense
    // and we run into problems in BreakLine() in this case.
    if( maxWidth <= 0 )
        return logicalLines;

    wxArrayString physicalLines;

    for( const wxString& line : logicalLines )
    {
        if( dc.GetTextExtent( line ).x > maxWidth )
        {
            // Line does not fit, break it up.
            BreakLine( dc, line, maxWidth, physicalLines );
        }
        else // The entire line fits as is
        {
            physicalLines.push_back( line );
        }
    }

    return physicalLines;
}


// PRIVATE METHOD UNCHANGED FROM wxWidgets IMPLEMENTATION
void GRIDCELL_AUTOWRAP_STRINGRENDERER::BreakLine( wxDC& dc, const wxString& logicalLine,
                                                  wxCoord maxWidth, wxArrayString& lines )
{
    wxCoord  lineWidth = 0;
    wxString line;

    // For each word
    wxStringTokenizer wordTokenizer( logicalLine, " \t", wxTOKEN_RET_DELIMS );

    while( wordTokenizer.HasMoreTokens() )
    {
        const wxString word = wordTokenizer.GetNextToken();
        const wxCoord  wordWidth = dc.GetTextExtent( word ).x;

        if( lineWidth + wordWidth < maxWidth )
        {
            // Word fits, just add it to this line.
            line += word;
            lineWidth += wordWidth;
        }
        else
        {
            // Word does not fit, check whether the word is itself wider that
            // available width
            if( wordWidth < maxWidth )
            {
                // Word can fit in a new line, put it at the beginning
                // of the new line.
                lines.push_back( line );
                line = word;
                lineWidth = wordWidth;
            }
            else // Word cannot fit in available width at all.
            {
                if( !line.empty() )
                {
                    lines.push_back( line );
                    line.clear();
                    lineWidth = 0;
                }

                // Break it up in several lines.
                lineWidth = BreakWord( dc, word, maxWidth, lines, line );
            }
        }
    }

    if( !line.empty() )
        lines.push_back( line );
}


// PRIVATE METHOD UNCHANGED FROM wxWidgets IMPLEMENTATION
wxCoord GRIDCELL_AUTOWRAP_STRINGRENDERER::BreakWord( wxDC& dc, const wxString& word,
                                                     wxCoord maxWidth, wxArrayString& lines,
                                                     wxString& line )
{
    wxArrayInt widths;
    dc.GetPartialTextExtents( word, widths );

    // TODO: Use binary search to find the first element > maxWidth.
    const unsigned count = widths.size();
    unsigned       n;

    for( n = 0; n < count; n++ )
    {
        if( widths[n] > maxWidth )
            break;
    }

    if( n == 0 )
    {
        // This is a degenerate case: the first character of the word is
        // already wider than the available space, so we just can't show it
        // completely and have to put the first character in this line.
        n = 1;
    }

    lines.push_back( word.substr( 0, n ) );

    // Check if the remainder of the string fits in one line.
    //
    // Unfortunately we can't use the existing partial text extents as the
    // extent of the remainder may be different when it's rendered in a
    // separate line instead of as part of the same one, so we have to
    // recompute it.
    const wxString rest = word.substr( n );
    const wxCoord  restWidth = dc.GetTextExtent( rest ).x;

    if( restWidth <= maxWidth )
    {
        line = rest;
        return restWidth;
    }

    // Break the rest of the word into lines.
    //
    // TODO: Perhaps avoid recursion? The code is simpler like this but using a
    // loop in this function would probably be more efficient.
    return BreakWord( dc, rest, maxWidth, lines, line );
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


/**
 * A helper to handle symbols to edit.
 */
class SYMBOL_CANDIDATE
{
public:
    SYMBOL_CANDIDATE( SCH_SYMBOL* aSymbol )
    {
        m_Symbol = aSymbol;
        m_InitialLibId = m_Symbol->GetLibId().Format().wx_str();
        m_Row = -1;
        m_IsOrphan = false;
        m_Screen = nullptr;
    }

    // Return a string like mylib:symbol_name from the #LIB_ID of the symbol.
    wxString GetStringLibId()
    {
        return m_Symbol->GetLibId().GetUniStringLibId();
    }

    SCH_SYMBOL* m_Symbol;       // the schematic symbol
    int         m_Row;          // the row index in m_grid
    SCH_SCREEN* m_Screen;       // the screen where m_Symbol lives
    wxString    m_Reference;    // the schematic reference, only to display it in list
    wxString    m_InitialLibId; // the Lib Id of the symbol before any change.
    bool        m_IsOrphan;     // true if a symbol has no corresponding symbol found in libs.
};


/**
 * Dialog to globally edit the #LIB_ID of groups if symbols having the same initial LIB_ID.
 *
 * This is useful when you want to:
 *  * move a symbol from a symbol library to another symbol library.
 *  * change the nickname of a library.
 *  * globally replace the symbol used by a group of symbols by another symbol.
 */
class DIALOG_EDIT_SYMBOLS_LIBID : public DIALOG_EDIT_SYMBOLS_LIBID_BASE
{
public:
    DIALOG_EDIT_SYMBOLS_LIBID( SCH_EDIT_FRAME* aParent );
    ~DIALOG_EDIT_SYMBOLS_LIBID() override;

    SCH_EDIT_FRAME* GetParent();

    bool IsSchematicModified() { return m_isModified; }

private:
    void initDlg();

    /**
     * Add a new row (new entry) in m_grid.
     *
     * @param aMarkRow set to true to use bold/italic font in column COL_CURR_LIBID.
     * @param aReferences is the value of cell( aRowId, COL_REFS).
     * @param aStrLibId is the value of cell( aRowId, COL_CURR_LIBID).
     */
    void AddRowToGrid( bool aMarkRow, const wxString& aReferences, const wxString& aStrLibId );

    /// returns true if all new lib id are valid
    bool validateLibIds();

    /**
     * Run the lib browser and set the selected #LIB_ID for \a aRow.
     *
     * @param aRow is the row to edit.
     * @return false if the command was aborted.
     */
    bool setLibIdByBrowser( int aRow );

    // Event handlers

    // called on a right click or a left double click:
	void onCellBrowseLib( wxGridEvent& event ) override;

    // Cancel all changes, and close the dialog
	void onCancel( wxCommandEvent& event ) override
    {
        // Just skipping the event doesn't work after the library browser was run
        if( IsQuasiModal() )
            EndQuasiModal( wxID_CANCEL );
        else
            event.Skip();
    }

    // Try to find a candidate for non existing symbols
	void onClickOrphansButton( wxCommandEvent& event ) override;

    // Automatically called when click on OK button
    bool TransferDataFromWindow() override;

    void AdjustGridColumns();

    void OnSizeGrid( wxSizeEvent& event ) override;

    bool             m_isModified;          // set to true if the schematic is modified
    std::vector<int> m_OrphansRowIndexes;   // list of rows containing orphan lib_id

    std::vector<SYMBOL_CANDIDATE>     m_symbols;

    GRIDCELL_AUTOWRAP_STRINGRENDERER* m_autoWrapRenderer;
};


DIALOG_EDIT_SYMBOLS_LIBID::DIALOG_EDIT_SYMBOLS_LIBID( SCH_EDIT_FRAME* aParent ) :
        DIALOG_EDIT_SYMBOLS_LIBID_BASE( aParent )
{
    m_autoWrapRenderer = new GRIDCELL_AUTOWRAP_STRINGRENDERER;

    m_grid->PushEventHandler( new GRID_TRICKS( m_grid ) );

    initDlg();

    finishDialogSettings();
}


DIALOG_EDIT_SYMBOLS_LIBID::~DIALOG_EDIT_SYMBOLS_LIBID()
{
    // Delete the GRID_TRICKS.
    m_grid->PopEventHandler( true );

    m_autoWrapRenderer->DecRef();
}


// A sort compare function to sort symbols list by LIB_ID and then reference.
static bool sort_by_libid( const SYMBOL_CANDIDATE& candidate1, const SYMBOL_CANDIDATE& candidate2 )
{
    if( candidate1.m_Symbol->GetLibId() == candidate2.m_Symbol->GetLibId() )
        return candidate1.m_Reference.Cmp( candidate2.m_Reference ) < 0;

    return candidate1.m_Symbol->GetLibId() < candidate2.m_Symbol->GetLibId();
}


void DIALOG_EDIT_SYMBOLS_LIBID::initDlg()
{
    // Clear the FormBuilder rows
    m_grid->ClearRows();

    m_isModified = false;

    // This option build the full symbol list.
    // In complex hierarchies, the same symbol is in fact duplicated, but
    // it is listed with different references (one by sheet instance)
    // the list is larger and looks like it contains all symbols.
    SCH_REFERENCE_LIST references;

    // build the full list of symbols including symbol having no symbol in loaded libs
    // (orphan symbols)
    GetParent()->Schematic().Hierarchy().GetSymbols( references,
                                                     true /* include power symbols */,
                                                     true /* include orphan symbols */ );

    for( unsigned ii = 0; ii < references.GetCount(); ii++ )
    {
        SCH_REFERENCE& item = references[ii];
        SYMBOL_CANDIDATE candidate( item.GetSymbol() );
        candidate.m_Screen = item.GetSheetPath().LastScreen();
        SCH_SHEET_PATH sheetpath = item.GetSheetPath();
        candidate.m_Reference = candidate.m_Symbol->GetRef( &sheetpath );
        int unitcount = candidate.m_Symbol->GetUnitCount();
        candidate.m_IsOrphan = ( unitcount == 0 );
        m_symbols.push_back( candidate );
    }

    if( m_symbols.size() == 0 )
        return;

    // now sort by lib id to create groups of items having the same lib id
    std::sort( m_symbols.begin(), m_symbols.end(), sort_by_libid );

    // Now, fill m_grid
    wxString last_str_libid = m_symbols.front().GetStringLibId();
    int row = 0;
    wxString refs;
    wxString last_ref;
    bool mark_cell = m_symbols.front().m_IsOrphan;

    for( SYMBOL_CANDIDATE& symbol : m_symbols )
    {
        wxString str_libid = symbol.GetStringLibId();

        if( last_str_libid != str_libid )
        {
            // Add last group to grid
            AddRowToGrid( mark_cell, refs, last_str_libid );

            // prepare next entry
            mark_cell = symbol.m_IsOrphan;
            last_str_libid = str_libid;
            refs.Empty();
            row++;
        }
        else if( symbol.m_Reference == last_ref )
        {
            symbol.m_Row = row;
            continue;
        }

        last_ref = symbol.m_Reference;

        if( !refs.IsEmpty() )
            refs += wxT( ", " );

        refs += symbol.m_Reference;
        symbol.m_Row = row;
    }

    // Add last symbol group:
    AddRowToGrid( mark_cell, refs, last_str_libid );

    // Allows only the selection by row
    m_grid->SetSelectionMode( wxGrid::wxGridSelectRows );

    m_buttonOrphanItems->Enable( m_OrphansRowIndexes.size() > 0 );
    Layout();
}


SCH_EDIT_FRAME* DIALOG_EDIT_SYMBOLS_LIBID::GetParent()
{
    return dynamic_cast<SCH_EDIT_FRAME*>( wxDialog::GetParent() );
}


void DIALOG_EDIT_SYMBOLS_LIBID::AddRowToGrid( bool aMarkRow, const wxString& aReferences,
                                              const wxString& aStrLibId )
{
    int row = m_grid->GetNumberRows();

    if( aMarkRow )      // An orphaned symbol exists, set m_AsOrphanCmp as true.
        m_OrphansRowIndexes.push_back( row );

    m_grid->AppendRows( 1 );

    m_grid->SetCellValue( row, COL_REFS, UnescapeString( aReferences ) );
    m_grid->SetReadOnly( row, COL_REFS );

    m_grid->SetCellValue( row, COL_CURR_LIBID, UnescapeString( aStrLibId ) );
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
    attr->SetEditor( new GRID_CELL_SYMBOL_ID_EDITOR( this, UnescapeString( aStrLibId ) ) );
    m_grid->SetAttr( row, COL_NEW_LIBID, attr );
}


wxString getLibIdValue( const WX_GRID* aGrid, int aRow, int aCol )
{
    wxString rawValue = aGrid->GetCellValue( aRow, aCol );

    if( rawValue.IsEmpty() )
        return rawValue;

    wxString itemName;
    wxString libName = rawValue.BeforeFirst( ':', &itemName );

    return EscapeString( libName, CTX_LIBID ) + ':' + EscapeString( itemName, CTX_LIBID );
}


bool DIALOG_EDIT_SYMBOLS_LIBID::validateLibIds()
{
    if( !m_grid->CommitPendingChanges() )
        return false;

    int row_max = m_grid->GetNumberRows() - 1;

    for( int row = 0; row <= row_max; row++ )
    {
        wxString new_libid = getLibIdValue( m_grid, row, COL_NEW_LIBID );

        if( new_libid.IsEmpty() )
            continue;

        // a new lib id is found. validate this new value
        LIB_ID id;
        id.Parse( new_libid );

        if( !id.IsValid() )
        {
            wxString msg;
            msg.Printf( _( "Symbol library identifier %s is not valid." ), new_libid );
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


void DIALOG_EDIT_SYMBOLS_LIBID::onCellBrowseLib( wxGridEvent& event )
{
    int row = event.GetRow();
    m_grid->SelectRow( row );   // only for user, to show the selected line

    setLibIdByBrowser( row );

}


void DIALOG_EDIT_SYMBOLS_LIBID::onClickOrphansButton( wxCommandEvent& event )
{
    std::vector<wxString> libs = PROJECT_SCH::SymbolLibAdapter( &Prj() )->GetLibraryNames();
    std::vector<wxString> aliasNames;
    wxArrayString candidateSymbNames;

    unsigned fixesCount = 0;

    // Try to find a candidate for non existing symbols in any loaded library
    for( int orphanRow : m_OrphansRowIndexes )
    {
        wxString orphanLibid = getLibIdValue( m_grid, orphanRow, COL_CURR_LIBID );
        int grid_row_idx = orphanRow; //row index in m_grid for the current item

        LIB_ID curr_libid;
        curr_libid.Parse( orphanLibid, true );
        wxString symbolName = curr_libid.GetLibItemName();

        // number of full LIB_ID candidates (because we search for a symbol name
        // inside all available libraries, perhaps the same symbol name can be found
        // in more than one library, giving ambiguity
        int libIdCandidateCount = 0;
        candidateSymbNames.Clear();

        // now try to find a candidate
        for( const wxString &lib : libs )
        {
            aliasNames.clear();

            try
            {
                aliasNames = PROJECT_SCH::SymbolLibAdapter( &Prj() )->GetSymbolNames( lib );
            }
            catch( const IO_ERROR& ) {}   // ignore, it is handled below

            if( aliasNames.empty() )
                continue;

            // Find a symbol name in symbols inside this library:
            if( auto it = std::ranges::find( aliasNames, symbolName ); it != aliasNames.end() )
            {
                // a candidate is found!
                libIdCandidateCount++;
                wxString newLibid = lib + ':' + symbolName;

                // Uses the first found. Most of time, it is alone.
                // Others will be stored in a candidate list
                if( libIdCandidateCount <= 1 )
                {
                    m_grid->SetCellValue( grid_row_idx, COL_NEW_LIBID, UnescapeString( newLibid ) );
                    candidateSymbNames.Add( m_grid->GetCellValue( grid_row_idx, COL_NEW_LIBID ) );
                    fixesCount++;
                }
                else    // Store other candidates for later selection
                {
                    candidateSymbNames.Add( UnescapeString( newLibid ) );
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
                                       wxString::Format( _( "Candidates count %d " ),
                                                         libIdCandidateCount ),
                                       candidateSymbNames );

            if( dlg.ShowModal() == wxID_OK )
                m_grid->SetCellValue( grid_row_idx, COL_NEW_LIBID, dlg.GetStringSelection() );
        }
    }

    if( fixesCount < m_OrphansRowIndexes.size() )   // Not all orphan symbols are fixed.
    {
        wxMessageBox( wxString::Format( _( "%u link(s) mapped, %u not found" ),
                                        fixesCount,
                                        (unsigned) m_OrphansRowIndexes.size() - fixesCount ) );
    }
    else
    {
        wxMessageBox( wxString::Format( _( "All %u link(s) resolved" ), fixesCount ) );
    }
}


bool DIALOG_EDIT_SYMBOLS_LIBID::setLibIdByBrowser( int aRow )
{
    // Use library viewer to choose a symbol
    std::vector<PICKED_SYMBOL> dummyHistory;
    std::vector<PICKED_SYMBOL> dummyAlreadyPlaced;
    LIB_ID                     preselected;
    wxString                   current = getLibIdValue( m_grid, aRow, COL_NEW_LIBID );

    if( current.IsEmpty() )
        current = getLibIdValue( m_grid, aRow, COL_CURR_LIBID );

    if( !current.IsEmpty() )
        preselected.Parse( current, true );

    PICKED_SYMBOL sel = GetParent()->PickSymbolFromLibrary(
            nullptr, dummyHistory, dummyAlreadyPlaced, false, &preselected, false );

    if( sel.LibId.empty() )     // command aborted
        return false;

    if( !sel.LibId.IsValid() )  // Should not occur
    {
        wxMessageBox( _( "Invalid symbol library identifier" ) );
        return false;
    }

    wxString new_libid;
    new_libid = sel.LibId.Format().wx_str();

    m_grid->SetCellValue( aRow, COL_NEW_LIBID, UnescapeString( new_libid ) );

    return true;
}


bool DIALOG_EDIT_SYMBOLS_LIBID::TransferDataFromWindow()
{
    if( !validateLibIds() )
        return false;

    SCH_COMMIT commit( GetParent() );

    auto getName = []( const LIB_ID& aLibId )
            {
                return UnescapeString( aLibId.GetLibItemName().wx_str() );
            };

    int row_max = m_grid->GetNumberRows() - 1;

    for( int row = 0; row <= row_max; row++ )
    {
        wxString new_libid = getLibIdValue( m_grid, row, COL_NEW_LIBID );

        if( new_libid.IsEmpty() || new_libid == getLibIdValue( m_grid, row, COL_CURR_LIBID ) )
            continue;

        // A new lib id is found and was already validated.
        LIB_ID id;
        id.Parse( new_libid, true );

        for( SYMBOL_CANDIDATE& candidate : m_symbols )
        {
            if( candidate.m_Row != row )
                continue;

            LIB_SYMBOL* symbol = nullptr;

            try
            {
                symbol = PROJECT_SCH::SymbolLibAdapter( &Prj() )->LoadSymbol( id );
            }
            catch( const IO_ERROR& ioe )
            {
                wxString msg;

                msg.Printf( _( "Error loading symbol %s from library %s.\n\n%s" ),
                            id.GetLibItemName().wx_str(),
                            id.GetLibNickname().wx_str(),
                            ioe.What() );

                DisplayErrorMessage( this, msg );
            }

            if( symbol == nullptr )
                continue;

            commit.Modify( candidate.m_Symbol, candidate.m_Screen );
            m_isModified = true;

            candidate.m_Screen->Remove( candidate.m_Symbol );
            SCH_FIELD* value = candidate.m_Symbol->GetField( FIELD_T::VALUE );

            // If value is a proxy for the itemName then make sure it gets updated
            if( getName( candidate.m_Symbol->GetLibId() ) == value->GetText() )
                candidate.m_Symbol->SetValueFieldText( getName( id ) );

            candidate.m_Symbol->SetLibId( id );
            candidate.m_Symbol->SetLibSymbol( symbol->Flatten().release() );
            candidate.m_Screen->Append( candidate.m_Symbol );
            candidate.m_Screen->SetContentModified();

            if ( m_checkBoxUpdateFields->IsChecked() )
            {
                candidate.m_Symbol->UpdateFields( nullptr,
                                                  false, /* update style */
                                                  false, /* update ref */
                                                  false, /* update other fields */
                                                  false, /* reset ref */
                                                  true   /* reset other fields */ );
            }
        }
    }

    if( m_isModified )
        commit.Push( wxS( "Change Symbol Library Indentifier" ) );

    return true;
}


void DIALOG_EDIT_SYMBOLS_LIBID::AdjustGridColumns()
{
    // Account for scroll bars
    int width = KIPLATFORM::UI::GetUnobscuredSize( m_grid ).x;

    int colWidth = width / 3;

    m_grid->SetColSize( COL_REFS, colWidth );
    width -= colWidth;

    colWidth = 0;

    for( int row = 0; row < m_grid->GetNumberRows(); ++row )
    {
        wxString cellValue = m_grid->GetCellValue( row, COL_CURR_LIBID );
        colWidth           = std::max( colWidth, KIUI::GetTextSize( cellValue, m_grid ).x );
    }

    colWidth += 20;
    m_grid->SetColSize( COL_CURR_LIBID, colWidth );
    width -= colWidth;

    colWidth = 0;

    for( int row = 0; row < m_grid->GetNumberRows(); ++row )
    {
        wxString cellValue = m_grid->GetCellValue( row, COL_NEW_LIBID );
        colWidth           = std::max( colWidth, KIUI::GetTextSize( cellValue, m_grid ).x );
    }

    colWidth += 20;
    m_grid->SetColSize( COL_NEW_LIBID, std::max( colWidth, width ) );
}


void DIALOG_EDIT_SYMBOLS_LIBID::OnSizeGrid( wxSizeEvent& event )
{
    AdjustGridColumns();

    wxClientDC dc( this );

    // wxWidgets' AutoRowHeight fails when used with wxGridCellAutoWrapStringRenderer
    for( int row = 0; row < m_grid->GetNumberRows(); ++row )
        m_grid->SetRowSize( row, m_autoWrapRenderer->GetHeight( dc, m_grid, row, COL_REFS ) );

    event.Skip();
}


bool InvokeDialogEditSymbolsLibId( SCH_EDIT_FRAME* aCaller )
{
    // This dialog itself subsequently can invoke a KIWAY_PLAYER as a quasimodal
    // frame. Therefore this dialog as a modal frame parent, MUST be run under
    // quasimodal mode for the quasimodal frame support to work.  So don't use
    // the QUASIMODAL macros here.
    DIALOG_EDIT_SYMBOLS_LIBID dlg( aCaller );

    // DO NOT use ShowModal() here, otherwise the library browser will not work properly.
    dlg.ShowQuasiModal();

    return dlg.IsSchematicModified();
}
