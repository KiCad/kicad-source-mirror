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
#include <design_block_library_adapter.h>
#include <sch_design_block_pane.h>
#include <sch_edit_frame.h>
#include <sch_group.h>
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
#include <sch_selection_tool.h>
#include <dialogs/dialog_design_block_properties.h>
#include <json_common.h>

bool checkOverwriteDb( wxWindow* aFrame, wxString& libname, wxString& newName )
{
    wxString msg = wxString::Format( _( "Design block '%s' already exists in library '%s'." ),
                                     newName.GetData(),
                                     libname.GetData() );

    if( OKOrCancelDialog( aFrame, _( "Confirmation" ), msg, _( "Overwrite existing design block?" ), _( "Overwrite" ) )
        != wxID_OK )
    {
        return false;
    }

    return true;
}


bool checkOverwriteDbSchematic( wxWindow* aFrame, const LIB_ID& aLibId )
{
    wxString msg = wxString::Format( _( "Design block '%s' already has a schematic." ),
                                     aLibId.GetUniStringLibItemName() );

    if( OKOrCancelDialog( aFrame, _( "Confirmation" ), msg, _( "Overwrite existing schematic?" ), _( "Overwrite" ) )
        != wxID_OK )
    {
        return false;
    }

    return true;
}


bool SCH_EDIT_FRAME::SaveSheetAsDesignBlock( const wxString& aLibraryName, SCH_SHEET_PATH& aSheetPath )
{
    // Make sure the user has selected a library to save into
    if( m_designBlocksPane->GetSelectedLibId().GetLibNickname().empty() )
    {
        DisplayErrorMessage( this, _( "Please select a library to save the design block to." ) );
        return false;
    }

    // Just block all attempts to create design blocks with nested sheets at this point
    std::vector<SCH_ITEM*> sheets;
    aSheetPath.LastScreen()->GetSheets( &sheets );

    if( !sheets.empty() )
    {
        DisplayErrorMessage( this, _( "Design blocks with nested sheets are not supported." ) );
        return false;
    }

    DESIGN_BLOCK blk;
    wxFileName   fn = wxFileNameFromPath( aSheetPath.Last()->GetName() );

    blk.SetLibId( LIB_ID( aLibraryName, fn.GetName() ) );

    // Copy all fields from the sheet to the design block
    for( SCH_FIELD& field : aSheetPath.Last()->GetFields() )
    {
        if( field.GetId() == FIELD_T::SHEET_NAME || field.GetId() == FIELD_T::SHEET_FILENAME )
            continue;

        blk.GetFields()[field.GetCanonicalName()] = field.GetText();
    }

    DIALOG_DESIGN_BLOCK_PROPERTIES dlg( this, &blk );

    if( dlg.ShowModal() != wxID_OK )
        return false;

    wxString libName = blk.GetLibId().GetLibNickname();
    wxString newName = blk.GetLibId().GetLibItemName();

    if( Prj().DesignBlockLibs()->DesignBlockExists( libName, newName ) && !checkOverwriteDb( this, libName, newName ) )
        return false;

    // Save a temporary copy of the schematic file, as the plugin is just going to move it
    wxString tempFile = wxFileName::CreateTempFileName( "design_block" );

    if( !saveSchematicFile( aSheetPath.Last(), tempFile ) )
    {
        DisplayErrorMessage( this, _( "Error saving temporary schematic file to create design block." ) );
        wxRemoveFile( tempFile );
        return false;
    }

    blk.SetSchematicFile( tempFile );

    bool success = false;

    try
    {
        success = Prj().DesignBlockLibs()->SaveDesignBlock( aLibraryName, &blk )
                  == DESIGN_BLOCK_LIBRARY_ADAPTER::SAVE_OK;
    }
    catch( const IO_ERROR& ioe )
    {
        DisplayError( this, ioe.What() );
    }

    // Clean up the temporary file
    wxRemoveFile( tempFile );

    m_designBlocksPane->RefreshLibs();
    m_designBlocksPane->SelectLibId( blk.GetLibId() );

    return success;
}


bool SCH_EDIT_FRAME::SaveSheetToDesignBlock( const LIB_ID& aLibId, SCH_SHEET_PATH& aSheetPath )
{
    // Make sure the user has selected a library to save into
    if( !Prj().DesignBlockLibs()->DesignBlockExists( aLibId.GetLibNickname(), aLibId.GetLibItemName() ) )
    {
        DisplayErrorMessage( this, _( "Please select a design block to save the schematic to." ) );
        return false;
    }

    // Just block all attempts to create design blocks with nested sheets at this point
    std::vector<SCH_ITEM*> sheets;
    aSheetPath.LastScreen()->GetSheets( &sheets );

    if( !sheets.empty() )
    {
        DisplayErrorMessage( this, _( "Design blocks with nested sheets are not supported." ) );
        return false;
    }

    std::unique_ptr<DESIGN_BLOCK> blk;

    try
    {
        blk.reset( Prj().DesignBlockLibs()->LoadDesignBlock( aLibId.GetLibNickname(), aLibId.GetLibItemName() ) );
    }
    catch( const IO_ERROR& ioe )
    {
        DisplayError( this, ioe.What() );
        return false;
    }

    if( !blk->GetSchematicFile().IsEmpty() && !checkOverwriteDbSchematic( this, aLibId ) )
        return false;

    // Copy all fields from the sheet to the design block.
    // Note: this will overwrite any existing fields in the design block, but
    // will leave extra fields not in this source sheet alone.
    for( SCH_FIELD& field : aSheetPath.Last()->GetFields() )
    {
        if( field.GetId() == FIELD_T::SHEET_NAME || field.GetId() == FIELD_T::SHEET_FILENAME )
            continue;

        blk->GetFields()[field.GetCanonicalName()] = field.GetText();
    }

    DIALOG_DESIGN_BLOCK_PROPERTIES dlg( this, blk.get(), true );

    if( dlg.ShowModal() != wxID_OK )
        return false;

    // Save a temporary copy of the schematic file, as the plugin is just going to move it
    wxString tempFile = wxFileName::CreateTempFileName( "design_block" );
    if( !saveSchematicFile( aSheetPath.Last(), tempFile ) )
    {
        DisplayErrorMessage( this, _( "Error saving temporary schematic file to create design block." ) );
        wxRemoveFile( tempFile );
        return false;
    }

    blk->SetSchematicFile( tempFile );

    bool success = false;

    try
    {
        success = Prj().DesignBlockLibs()->SaveDesignBlock( aLibId.GetLibNickname(), blk.get() )
                  == DESIGN_BLOCK_LIBRARY_ADAPTER::SAVE_OK;
    }
    catch( const IO_ERROR& ioe )
    {
        DisplayError( this, ioe.What() );
    }

    // Clean up the temporary file
    wxRemoveFile( tempFile );

    m_designBlocksPane->RefreshLibs();
    m_designBlocksPane->SelectLibId( blk->GetLibId() );

    return success;
}


bool SCH_EDIT_FRAME::SaveSelectionAsDesignBlock( const wxString& aLibraryName )
{
    // Get all selected items
    SCH_SELECTION selection = m_toolManager->GetTool<SCH_SELECTION_TOOL>()->GetSelection();

    if( selection.Empty() )
    {
        DisplayErrorMessage( this, _( "Please select some items to save as a design block." ) );
        return false;
    }

    // Make sure the user has selected a library to save into
    if( m_designBlocksPane->GetSelectedLibId().GetLibNickname().empty() )
    {
        DisplayErrorMessage( this, _( "Please select a library to save the design block to." ) );
        return false;
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
        {
            DisplayErrorMessage( this, _( "Design blocks with nested sheets are not supported." ) );
        }

        return false;
    }

    DESIGN_BLOCK blk;
    SCH_GROUP*   group = nullptr;

    if( selection.Size() == 1 && selection.HasType( SCH_GROUP_T ) )
        group = static_cast<SCH_GROUP*>( selection.Front() );

    if( group && !group->GetName().IsEmpty() )
        // If the user has selected a single group, they probably want the design block named after the group
        blk.SetLibId( LIB_ID( aLibraryName, group->GetName() ) );
    else
    {
        // Otherwise, use the current screen name
        wxFileName fn = wxFileNameFromPath( GetScreen()->GetFileName() );
        blk.SetLibId( LIB_ID( aLibraryName, fn.GetName() ) );
    }

    DIALOG_DESIGN_BLOCK_PROPERTIES dlg( this, &blk );

    if( dlg.ShowModal() != wxID_OK )
        return false;

    wxString libName = blk.GetLibId().GetLibNickname();
    wxString newName = blk.GetLibId().GetLibItemName();

    if( Prj().DesignBlockLibs()->DesignBlockExists( libName, newName ) && !checkOverwriteDb( this, libName, newName ) )
        return false;

    // Create a temporary screen
    SCH_SCREEN* tempScreen = new SCH_SCREEN( m_schematic );

    // If we have a single group, we want to strip the group and select the children
    if( group )
    {
        selection.Remove( group );

        // Don't recurse; if we have a group of groups the user probably intends the inner groups to be saved
        group->RunOnChildren(
                [&]( EDA_ITEM* aItem )
                {
                    selection.Add( aItem );
                },
                RECURSE_MODE::NO_RECURSE );
    }

    // Copy the selected items to the temporary screen
    for( EDA_ITEM* item : selection )
    {
        // We need to deep copy since selections of groups will not have the children
        if( item->Type() == SCH_GROUP_T )
        {
            SCH_GROUP* clonedGroup = static_cast<SCH_GROUP*>( item )->DeepClone();

            tempScreen->Append( clonedGroup );

            clonedGroup->RunOnChildren(
                                        [&]( EDA_ITEM* aItem )
                                        {
                                            tempScreen->Append( static_cast<SCH_ITEM*>( aItem ) );
                                        },
                                        RECURSE_MODE::RECURSE );
        }
        else
        {
            EDA_ITEM* copy = item->Clone();
            tempScreen->Append( static_cast<SCH_ITEM*>( copy ) );
        }
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
        return false;
    }

    blk.SetSchematicFile( tempFile );

    bool success = false;

    try
    {
        success = Prj().DesignBlockLibs()->SaveDesignBlock( aLibraryName, &blk )
                  == DESIGN_BLOCK_LIBRARY_ADAPTER::SAVE_OK;
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

    return success;
}


bool SCH_EDIT_FRAME::SaveSelectionToDesignBlock( const LIB_ID& aLibId )
{
    // Get all selected items
    SCH_SELECTION selection = m_toolManager->GetTool<SCH_SELECTION_TOOL>()->GetSelection();

    if( selection.Empty() )
    {
        DisplayErrorMessage( this, _( "Please select some items to save as a design block." ) );
        return false;
    }

    // Make sure the user has selected a library to save into
    if( !Prj().DesignBlockLibs()->DesignBlockExists( aLibId.GetLibNickname(), aLibId.GetLibItemName() ) )
    {
        DisplayErrorMessage( this, _( "Please select a design block to save the schematic to." ) );
        return false;
    }

    // Just block all attempts to create design blocks with nested sheets at this point
    if( selection.HasType( SCH_SHEET_T ) )
    {
        if( selection.Size() == 1 )
        {
            SCH_SHEET*     sheet = static_cast<SCH_SHEET*>( selection.Front() );
            SCH_SHEET_PATH curPath = GetCurrentSheet();

            curPath.push_back( sheet );
            SaveSheetToDesignBlock( aLibId, curPath );
        }
        else
        {
            DisplayErrorMessage( this, _( "Design blocks with nested sheets are not supported." ) );
        }

        return false;
    }

    // If we have a single group, we want to strip the group and select the children
    SCH_GROUP* group = nullptr;

    if( selection.Size() == 1 )
    {
        EDA_ITEM* item = selection.Front();

        if( item->Type() == SCH_GROUP_T )
        {
            group = static_cast<SCH_GROUP*>( item );

            selection.Remove( group );

            // Don't recurse; if we have a group of groups the user probably intends the inner groups to be saved
            group->RunOnChildren( [&]( EDA_ITEM* aItem )
                                  {
                                      selection.Add( aItem );
                                  },
                                  RECURSE_MODE::NO_RECURSE );
        }
    }

    std::unique_ptr<DESIGN_BLOCK> blk;

    try
    {
        blk.reset( Prj().DesignBlockLibs()->LoadDesignBlock( aLibId.GetLibNickname(), aLibId.GetLibItemName() ) );
    }
    catch( const IO_ERROR& ioe )
    {
        DisplayError( this, ioe.What() );
        return false;
    }

    if( !blk->GetSchematicFile().IsEmpty() && !checkOverwriteDbSchematic( this, aLibId ) )
        return false;

    // Create a temporary screen
    SCH_SCREEN* tempScreen = new SCH_SCREEN( m_schematic );

    auto cloneAndAdd =
            [&] ( EDA_ITEM* aItem ) -> SCH_ITEM*
            {
                if( !aItem->IsSCH_ITEM() )
                    return nullptr;

                SCH_ITEM* copy = static_cast<SCH_ITEM*>( aItem->Clone() );
                tempScreen->Append( static_cast<SCH_ITEM*>( copy ) );
                return copy;
            };

    // Copy the selected items to the temporary board
    for( EDA_ITEM* item : selection )
    {
        // Remove parent group membership since we strip the first group layer
        if( SCH_ITEM* copy = cloneAndAdd( item ) )
            copy->SetParentGroup( nullptr );

        if( item->Type() == SCH_GROUP_T )
        {
            SCH_GROUP* innerGroup = static_cast<SCH_GROUP*>( item );

            // Groups also need their children copied
            innerGroup->RunOnChildren( cloneAndAdd, RECURSE_MODE::RECURSE );
        }
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
        return false;
    }

    blk->SetSchematicFile( tempFile );

    bool success = false;

    try
    {
        success = Prj().DesignBlockLibs()->SaveDesignBlock( aLibId.GetLibNickname(), blk.get() )
                  == DESIGN_BLOCK_LIBRARY_ADAPTER::SAVE_OK;

        // If we had a group, we need to reselect it
        if( group )
        {
            selection.Clear();
            selection.Add( group );

            // If we didn't have a design block link before, add one for convenience
            if( !group->HasDesignBlockLink() )
            {
                SCH_COMMIT commit( m_toolManager );

                commit.Modify( group, GetScreen() );
                group->SetDesignBlockLibId( aLibId );

                commit.Push( "Set Group Design Block Link" );
            }
        }
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
    m_designBlocksPane->SelectLibId( blk->GetLibId() );

    return success;
}
