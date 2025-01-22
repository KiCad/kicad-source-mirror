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

#include <pgm_base.h>
#include <kiway.h>
#include <design_block.h>
#include <design_block_lib_table.h>
#include <sch_design_block_pane.h>
#include <sch_edit_frame.h>
#include <wx/choicdlg.h>
#include <wx/msgdlg.h>
#include <wx/textdlg.h>
#include <wildcards_and_files_ext.h>
#include <paths.h>
#include <env_paths.h>
#include <common.h>
#include <kidialog.h>
#include <confirm.h>
#include <tool/tool_manager.h>
#include <ee_selection_tool.h>
#include <dialogs/dialog_design_block_properties.h>
#include <nlohmann/json.hpp>

bool checkOverwrite( wxWindow* aFrame, wxString& libname, wxString& newName )
{
    wxString msg = wxString::Format( _( "Design block '%s' already exists in library '%s'." ),
                                     newName.GetData(), libname.GetData() );

    if( OKOrCancelDialog( aFrame, _( "Confirmation" ), msg, _( "Overwrite existing design block?" ),
                          _( "Overwrite" ) )
        != wxID_OK )
    {
        return false;
    }

    return true;
}


void SCH_EDIT_FRAME::SaveSheetAsDesignBlock( const wxString& aLibraryName,
                                             SCH_SHEET_PATH& aSheetPath )
{
    // Make sure the user has selected a library to save into
    if( m_designBlocksPane->GetSelectedLibId().GetLibNickname().empty() )
    {
        DisplayErrorMessage( this, _( "Please select a library to save the design block to." ) );
        return;
    }

    // Just block all attempts to create design blocks with nested sheets at this point
    std::vector<SCH_ITEM*> sheets;
    aSheetPath.LastScreen()->GetSheets( &sheets );

    if( !sheets.empty() )
    {
        DisplayErrorMessage( this, _( "Design blocks with nested sheets are not supported." ) );
        return;
    }

    DESIGN_BLOCK blk;
    wxFileName   fn = wxFileNameFromPath( aSheetPath.Last()->GetName() );

    blk.SetLibId( LIB_ID( aLibraryName, fn.GetName() ) );

    // Copy all fields from the sheet to the design block
    std::vector<SCH_FIELD>&                   shFields = aSheetPath.Last()->GetFields();
    nlohmann::ordered_map<wxString, wxString> dbFields;

    for( int i = 0; i < (int) shFields.size(); i++ )
    {
        if( i == SHEETNAME || i == SHEETFILENAME )
            continue;

        dbFields[shFields[i].GetCanonicalName()] = shFields[i].GetText();
    }

    blk.SetFields( dbFields );

    DIALOG_DESIGN_BLOCK_PROPERTIES dlg( this, &blk );

    if( dlg.ShowModal() != wxID_OK )
        return;

    // Save a temporary copy of the schematic file, as the plugin is just going to move it
    wxString tempFile = wxFileName::CreateTempFileName( "design_block" );
    if( !saveSchematicFile( aSheetPath.Last(), tempFile ) )
    {
        DisplayErrorMessage( this, _( "Error saving temporary schematic file to create design block." ) );
        wxRemoveFile( tempFile );
        return;
    }

    blk.SetSchematicFile( tempFile );

    try
    {
        wxString libName = blk.GetLibId().GetLibNickname();
        wxString newName = blk.GetLibId().GetLibItemName();

        if( Prj().DesignBlockLibs()->DesignBlockExists( libName, newName ) )
            if( !checkOverwrite( this, libName, newName ) )
                return;

        Prj().DesignBlockLibs()->DesignBlockSave( aLibraryName, &blk );
    }
    catch( const IO_ERROR& ioe )
    {
        DisplayError( this, ioe.What() );
    }

    // Clean up the temporary file
    wxRemoveFile( tempFile );

    m_designBlocksPane->RefreshLibs();
    m_designBlocksPane->SelectLibId( blk.GetLibId() );
}


void SCH_EDIT_FRAME::SaveSelectionAsDesignBlock( const wxString& aLibraryName )
{
    // Make sure the user has selected a library to save into
    if( m_designBlocksPane->GetSelectedLibId().GetLibNickname().empty() )
    {
        DisplayErrorMessage( this, _( "Please select a library to save the design block to." ) );
        return;
    }

    // Get all selected items
    EE_SELECTION selection = m_toolManager->GetTool<EE_SELECTION_TOOL>()->GetSelection();

    if( selection.Empty() )
    {
        DisplayErrorMessage( this, _( "Please select some items to save as a design block." ) );
        return;
    }

    // Just block all attempts to create design blocks with nested sheets at this point
    if( selection.HasType( SCH_SHEET_T ) )
    {
        if( selection.Size() == 1 )
        {
            SCH_SHEET*     sheet = static_cast<SCH_SHEET*>( selection.Front() );
            SCH_SHEET_PATH curPath = GetCurrentSheet();

            curPath.push_back( sheet );
            SaveSheetAsDesignBlock( aLibraryName, curPath );
        }
        else
            DisplayErrorMessage( this, _( "Design blocks with nested sheets are not supported." ) );

        return;
    }

    DESIGN_BLOCK blk;
    wxFileName   fn = wxFileNameFromPath( GetScreen()->GetFileName() );

    blk.SetLibId( LIB_ID( aLibraryName, fn.GetName() ) );

    DIALOG_DESIGN_BLOCK_PROPERTIES dlg( this, &blk );

    if( dlg.ShowModal() != wxID_OK )
        return;

    // Create a temporary screen
    SCH_SCREEN* tempScreen = new SCH_SCREEN( m_schematic );

    // Copy the selected items to the temporary screen
    for( EDA_ITEM* item : selection )
    {
        EDA_ITEM* copy = item->Clone();
        tempScreen->Append( static_cast<SCH_ITEM*>( copy ) );
    }

    // Create a sheet for the temporary screen
    SCH_SHEET* tempSheet = new SCH_SHEET( m_schematic );
    tempSheet->SetScreen( tempScreen );

    // Save a temporary copy of the schematic file, as the plugin is just going to move it
    wxString tempFile = wxFileName::CreateTempFileName( "design_block" );
    if( !saveSchematicFile( tempSheet, tempFile ) )
    {
        DisplayErrorMessage( this, _( "Error saving temporary schematic file to create design block." ) );
        wxRemoveFile( tempFile );
        return;
    }

    blk.SetSchematicFile( tempFile );

    try
    {
        wxString libName = blk.GetLibId().GetLibNickname();
        wxString newName = blk.GetLibId().GetLibItemName();

        if( Prj().DesignBlockLibs()->DesignBlockExists( libName, newName ) )
            if( !checkOverwrite( this, libName, newName ) )
                return;

        Prj().DesignBlockLibs()->DesignBlockSave( aLibraryName, &blk );
    }
    catch( const IO_ERROR& ioe )
    {
        DisplayError( this, ioe.What() );
    }

    // Clean up the temporaries
    wxRemoveFile( tempFile );
    // This will also delete the screen
    delete tempSheet;

    m_designBlocksPane->RefreshLibs();
    m_designBlocksPane->SelectLibId( blk.GetLibId() );
}
