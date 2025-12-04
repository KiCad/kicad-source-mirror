/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
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

#include <sch_actions.h>
#include <tools/sch_edit_table_tool.h>
#include <dialogs/dialog_table_properties.h>
#include <wx/filedlg.h>
#include <fstream>
#include <sch_sheet_path.h>
#include <wx/msgdlg.h>

SCH_EDIT_TABLE_TOOL::SCH_EDIT_TABLE_TOOL() :
        SCH_TOOL_BASE<SCH_EDIT_FRAME>( "eeschema.TableEditor" )
{
}


bool SCH_EDIT_TABLE_TOOL::Init()
{
    SCH_TOOL_BASE::Init();

    addMenus( m_selectionTool->GetToolMenu().GetMenu() );

    return true;
}


int SCH_EDIT_TABLE_TOOL::EditTable( const TOOL_EVENT& aEvent )
{
    SCH_SELECTION& selection = m_selectionTool->RequestSelection( SCH_COLLECTOR::EditableItems );
    bool           clearSelection = selection.IsHover();
    SCH_TABLE*     parentTable = nullptr;

    for( EDA_ITEM* item : selection.Items() )
    {
        if( item->Type() != SCH_TABLECELL_T )
            return 0;

        SCH_TABLE* table = static_cast<SCH_TABLE*>( item->GetParent() );

        if( !parentTable )
        {
            parentTable = table;
        }
        else if( parentTable != table )
        {
            parentTable = nullptr;
            break;
        }
    }

    if( parentTable )
    {
        DIALOG_TABLE_PROPERTIES dlg( m_frame, parentTable );

        // QuasiModal required for Scintilla auto-complete
        dlg.ShowQuasiModal();
    }

    if( clearSelection )
        m_toolMgr->RunAction( ACTIONS::selectionClear );

    return 0;
}


int SCH_EDIT_TABLE_TOOL::ExportTableToCSV( const TOOL_EVENT& aEvent )
{
    SCH_SELECTION& selection = m_selectionTool->RequestSelection( SCH_COLLECTOR::EditableItems );
    bool           clearSelection = selection.IsHover();
    SCH_TABLE*     parentTable = nullptr;

    // Find the table from the selection
    for( EDA_ITEM* item : selection.Items() )
    {
        if( item->Type() != SCH_TABLECELL_T )
            return 0;

        SCH_TABLE* table = static_cast<SCH_TABLE*>( item->GetParent() );

        if( !parentTable )
        {
            parentTable = table;
        }
        else if( parentTable != table )
        {
            parentTable = nullptr;
            break;
        }
    }

    if( !parentTable )
        return 0;

    // Get current sheet path for variable resolution
    SCH_SHEET_PATH& currentSheet = m_frame->GetCurrentSheet();

    // Show file save dialog
    wxFileDialog saveDialog( m_frame, _( "Export Table to CSV" ),
                            wxEmptyString, wxEmptyString,
                            _( "CSV files (*.csv)|*.csv" ),
                            wxFD_SAVE | wxFD_OVERWRITE_PROMPT );

    if( saveDialog.ShowModal() == wxID_CANCEL )
    {
        if( clearSelection )
            m_toolMgr->RunAction( ACTIONS::selectionClear );
        return 0;
    }

    wxString filePath = saveDialog.GetPath();

    // Ensure .csv extension
    if( !filePath.Lower().EndsWith( ".csv" ) )
        filePath += ".csv";

    // Open file for writing
    std::ofstream outFile( filePath.ToStdString() );

    if( !outFile.is_open() )
    {
        wxMessageBox( wxString::Format( _( "Failed to open file:\n%s" ), filePath ),
                     _( "Export Error" ), wxOK | wxICON_ERROR, m_frame );

        if( clearSelection )
            m_toolMgr->RunAction( ACTIONS::selectionClear );
        return 0;
    }

    // Helper function to escape CSV fields
    auto escapeCSV = []( const wxString& field ) -> wxString
    {
        wxString escaped = field;

        // If field contains comma, quote, or newline, wrap in quotes and escape quotes
        if( escaped.Contains( ',' ) || escaped.Contains( '\"' ) || escaped.Contains( '\n' ) )
        {
            escaped.Replace( "\"", "\"\"" );  // Escape quotes by doubling them
            escaped = "\"" + escaped + "\"";
        }

        return escaped;
    };

    // Export table data
    for( int row = 0; row < parentTable->GetRowCount(); ++row )
    {
        for( int col = 0; col < parentTable->GetColCount(); ++col )
        {
            SCH_TABLECELL* cell = parentTable->GetCell( row, col );

            // Get resolved text (with variables expanded)
            wxString cellText = cell->GetShownText( nullptr, &currentSheet, false, 0 );

            // Write escaped cell text
            outFile << escapeCSV( cellText ).ToStdString();

            // Add comma separator unless it's the last column
            if( col < parentTable->GetColCount() - 1 )
                outFile << ',';
        }

        // End of row
        outFile << '\n';
    }

    outFile.close();

    if( clearSelection )
        m_toolMgr->RunAction( ACTIONS::selectionClear );

    return 0;
}


SCH_TABLECELL* SCH_EDIT_TABLE_TOOL::copyCell( SCH_TABLECELL* aSource )
{
    // Use copy constructor to copy all formatting properties (font, colors, borders, etc.)
    SCH_TABLECELL* cell = new SCH_TABLECELL( *aSource );

    // Generate a new UUID to avoid duplicates (copy constructor preserves the old UUID)
    const_cast<KIID&>( cell->m_Uuid ) = KIID();

    // Clear text content - we only want the formatting, not the content
    cell->SetText( wxEmptyString );

    // Position will be set by the caller, but preserve size from source
    cell->SetStart( aSource->GetStart() );
    cell->SetEnd( aSource->GetEnd() );

    return cell;
}


const SELECTION& SCH_EDIT_TABLE_TOOL::getTableCellSelection()
{
    return m_selectionTool->RequestSelection( { SCH_TABLECELL_T } );
}


void SCH_EDIT_TABLE_TOOL::setTransitions()
{
    Go( &SCH_EDIT_TABLE_TOOL::AddRowAbove, ACTIONS::addRowAbove.MakeEvent() );
    Go( &SCH_EDIT_TABLE_TOOL::AddRowBelow, ACTIONS::addRowBelow.MakeEvent() );

    Go( &SCH_EDIT_TABLE_TOOL::AddColumnBefore, ACTIONS::addColBefore.MakeEvent() );
    Go( &SCH_EDIT_TABLE_TOOL::AddColumnAfter, ACTIONS::addColAfter.MakeEvent() );

    Go( &SCH_EDIT_TABLE_TOOL::DeleteRows, ACTIONS::deleteRows.MakeEvent() );
    Go( &SCH_EDIT_TABLE_TOOL::DeleteColumns, ACTIONS::deleteColumns.MakeEvent() );

    Go( &SCH_EDIT_TABLE_TOOL::MergeCells, ACTIONS::mergeCells.MakeEvent() );
    Go( &SCH_EDIT_TABLE_TOOL::UnmergeCells, ACTIONS::unmergeCells.MakeEvent() );

    Go( &SCH_EDIT_TABLE_TOOL::EditTable, ACTIONS::editTable.MakeEvent() );
    Go( &SCH_EDIT_TABLE_TOOL::ExportTableToCSV, ACTIONS::exportTableCSV.MakeEvent() );
}
