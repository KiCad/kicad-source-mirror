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
#include <board_commit.h>
#include <design_block.h>
#include <design_block_library_adapter.h>
#include <footprint.h>
#include <pad.h>
#include <pcb_group.h>
#include <widgets/pcb_design_block_pane.h>
#include <pcb_edit_frame.h>
#include <pcb_io/pcb_io.h>
#include <pcb_io/pcb_io_mgr.h>
#include <wx/choicdlg.h>
#include <wx/msgdlg.h>
#include <wx/textdlg.h>
#include <wildcards_and_files_ext.h>
#include <paths.h>
#include <env_paths.h>
#include <common.h>
#include <confirm.h>
#include <kidialog.h>
#include <locale_io.h>
#include <netinfo.h>
#include <tool/tool_manager.h>
#include <tools/pcb_selection_tool.h>
#include <dialogs/dialog_design_block_properties.h>
#include <json_common.h>

bool checkOverwriteDb( wxWindow* aFrame, wxString& libname, wxString& newName )
{
    wxString msg = wxString::Format( _( "Design block '%s' already exists in library '%s'." ), newName.GetData(),
                                     libname.GetData() );

    if( OKOrCancelDialog( aFrame, _( "Confirmation" ), msg, _( "Overwrite existing design block?" ), _( "Overwrite" ) )
        != wxID_OK )
    {
        return false;
    }

    return true;
}


bool checkOverwriteDbLayout( wxWindow* aFrame, const LIB_ID& aLibId )
{
    wxString msg = wxString::Format( _( "Design block '%s' already has a layout." ), aLibId.GetUniStringLibItemName() );

    if( OKOrCancelDialog( aFrame, _( "Confirmation" ), msg, _( "Overwrite existing layout?" ), _( "Overwrite" ) )
        != wxID_OK )
    {
        return false;
    }

    return true;
}


bool PCB_EDIT_FRAME::saveBoardAsFile( BOARD* aBoard, const wxString& aFileName, bool aHeadless )
{
    // Ensure the "C" locale is temporary set, before saving any file
    // It also avoid wxWidget alerts about locale issues, later, when using Python 3
    LOCALE_IO dummy;

    wxFileName pcbFileName( aFileName );

    if( !IsWritable( pcbFileName ) )
    {
        if( !aHeadless )
        {
            DisplayError( this, wxString::Format( _( "Insufficient permissions to write file '%s'." ),
                                                  pcbFileName.GetFullPath() ) );
        }
        return false;
    }

    try
    {
        IO_RELEASER<PCB_IO> pi( PCB_IO_MGR::PluginFind( PCB_IO_MGR::KICAD_SEXP ) );

        wxASSERT( pcbFileName.IsAbsolute() );

        pi->SaveBoard( pcbFileName.GetFullPath(), aBoard, nullptr );
    }
    catch( const IO_ERROR& ioe )
    {
        if( !aHeadless )
        {
            DisplayError( this, wxString::Format( _( "Error saving board file '%s'.\n%s" ), pcbFileName.GetFullPath(),
                                                  ioe.What() ) );
        }

        return false;
    }

    return true;
}


bool PCB_EDIT_FRAME::SaveBoardAsDesignBlock( const wxString& aLibraryName )
{
    // Make sure the user has selected a library to save into
    if( m_designBlocksPane->GetSelectedLibId().GetLibNickname().empty() )
    {
        DisplayErrorMessage( this, _( "Please select a library to save the design block to." ) );
        return false;
    }

    DESIGN_BLOCK blk;
    wxFileName   fn = wxFileNameFromPath( GetBoard()->GetFileName() );

    blk.SetLibId( LIB_ID( aLibraryName, fn.GetName() ) );

    DIALOG_DESIGN_BLOCK_PROPERTIES dlg( this, &blk );

    if( dlg.ShowModal() != wxID_OK )
        return false;

    wxString libName = blk.GetLibId().GetLibNickname();
    wxString newName = blk.GetLibId().GetLibItemName();

    if( Prj().DesignBlockLibs()->DesignBlockExists( libName, newName ) && !checkOverwriteDb( this, libName, newName ) )
    {
        return false;
    }

    // Save a temporary copy of the schematic file, as the plugin is just going to move it
    wxString tempFile = wxFileName::CreateTempFileName( "design_block" );

    if( !SavePcbCopy( tempFile, false, false ) )
    {
        DisplayErrorMessage( this, _( "Error saving temporary board file to create design block." ) );
        wxRemoveFile( tempFile );
        return false;
    }

    blk.SetBoardFile( tempFile );

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


bool PCB_EDIT_FRAME::SaveBoardToDesignBlock( const LIB_ID& aLibId )
{
    // Make sure the user has selected a library to save into
    if( m_designBlocksPane->GetSelectedLibId().GetLibNickname().empty() )
    {
        DisplayErrorMessage( this, _( "Please select a library to save the design block to." ) );
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

    if( !blk->GetBoardFile().IsEmpty() && !checkOverwriteDbLayout( this, aLibId ) )
        return false;

    // Save a temporary copy of the schematic file, as the plugin is just going to move it
    wxString tempFile = wxFileName::CreateTempFileName( "design_block" );

    if( !SavePcbCopy( tempFile, false, false ) )
    {
        DisplayErrorMessage( this, _( "Error saving temporary board file to create design block." ) );
        wxRemoveFile( tempFile );
        return false;
    }

    blk->SetBoardFile( tempFile );

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


bool PCB_EDIT_FRAME::saveSelectionToDesignBlock( const wxString& aNickname, PCB_SELECTION& aSelection,
                                                 DESIGN_BLOCK& aBlock )
{
    // Create a temporary board
    BOARD* tempBoard = new BOARD();
    tempBoard->SetDesignSettings( GetBoard()->GetDesignSettings() );
    tempBoard->SetProject( &Prj(), true );
    tempBoard->SynchronizeProperties();

    // For copying net info of selected items into the new board
    auto addNetIfNeeded =
            [&]( EDA_ITEM* aItem )
            {
                BOARD_CONNECTED_ITEM* cItem = dynamic_cast<BOARD_CONNECTED_ITEM*>( aItem );

                if( cItem )
                {
                    NETINFO_ITEM* netinfo = cItem->GetNet();

                    if( netinfo )
                    {
                        NETINFO_ITEM* existingInfo = tempBoard->FindNet( netinfo->GetNetname() );

                        // If the net has already been added to the new board, update our info to match
                        if( existingInfo )
                            cItem->SetNet( existingInfo );
                        else
                        {
                            NETINFO_ITEM* newNet = new NETINFO_ITEM( tempBoard, netinfo->GetNetname() );
                            tempBoard->Add( newNet );
                            cItem->SetNet( newNet );
                        }
                    }
                }
            };

   auto cloneAndAdd =
            [&] ( EDA_ITEM* aItem )
            {
                if( !aItem->IsBOARD_ITEM() )
                    return static_cast<BOARD_ITEM*>( nullptr );

                BOARD_ITEM* copy = static_cast<BOARD_ITEM*>( aItem->Clone() );
                tempBoard->Add( copy, ADD_MODE::APPEND, false );
                return copy;
            };

    // Copy the selected items to the temporary board
    for( EDA_ITEM* item : aSelection )
    {
        BOARD_ITEM* copy = cloneAndAdd( item );

        if( !copy )
            continue;

        copy->SetParentGroup( nullptr );

        if( copy->Type() == PCB_FOOTPRINT_T )
        {
            static_cast<FOOTPRINT*>( copy )->RunOnChildren( addNetIfNeeded, RECURSE_MODE::NO_RECURSE );
        }
        else if( copy->Type() == PCB_GROUP_T || copy->Type() == PCB_GENERATOR_T )
        {
            PCB_GROUP* group = static_cast<PCB_GROUP*>( item );

            // Groups also need their children copied
            group->RunOnChildren( cloneAndAdd, RECURSE_MODE::RECURSE );
            group->RunOnChildren( addNetIfNeeded, RECURSE_MODE::RECURSE );
        }
        else
            addNetIfNeeded( copy );
    }

    wxString tempFile = wxFileName::CreateTempFileName( "design_block" );

    if( !saveBoardAsFile( tempBoard, tempFile, false ) )
    {
        DisplayErrorMessage( this, _( "Error saving temporary board file to create design block." ) );
        wxRemoveFile( tempFile );
        return false;
    }

    aBlock.SetBoardFile( tempFile );

    bool success = false;

    try
    {
        success = Prj().DesignBlockLibs()->SaveDesignBlock( aNickname, &aBlock )
                == DESIGN_BLOCK_LIBRARY_ADAPTER::SAVE_OK;
    }
    catch( const IO_ERROR& ioe )
    {
        DisplayError( this, ioe.What() );
    }

    // Clean up the temporary file
    wxRemoveFile( tempFile );

    m_designBlocksPane->RefreshLibs();
    m_designBlocksPane->SelectLibId( aBlock.GetLibId() );

    return success;
}


bool PCB_EDIT_FRAME::SaveSelectionAsDesignBlock( const wxString& aLibraryName )
{
    // Make sure the user has selected a library to save into
    if( m_designBlocksPane->GetSelectedLibId().GetLibNickname().empty() )
    {
        DisplayErrorMessage( this, _( "Please select a library to save the design block to." ) );
        return false;
    }

    // Get all selected items
    PCB_SELECTION selection = m_toolManager->GetTool<PCB_SELECTION_TOOL>()->GetSelection();

    if( selection.Empty() )
    {
        DisplayErrorMessage( this, _( "Please select some items to save as a design block." ) );
        return false;
    }

    DESIGN_BLOCK blk;
    wxFileName   fn = wxFileNameFromPath( GetBoard()->GetFileName() );

    blk.SetLibId( LIB_ID( aLibraryName, fn.GetName() ) );

    DIALOG_DESIGN_BLOCK_PROPERTIES dlg( this, &blk );

    if( dlg.ShowModal() != wxID_OK )
        return false;

    wxString libName = blk.GetLibId().GetLibNickname();
    wxString newName = blk.GetLibId().GetLibItemName();

    if( Prj().DesignBlockLibs()->DesignBlockExists( libName, newName ) && !checkOverwriteDb( this, libName, newName ) )
    {
        return false;
    }

    return saveSelectionToDesignBlock( libName, selection, blk );
}


bool PCB_EDIT_FRAME::SaveSelectionToDesignBlock( const LIB_ID& aLibId )
{
    // Make sure the user has selected a library to save into
    if( !aLibId.IsValid() )
    {
        DisplayErrorMessage( this, _( "Please select a library to save the design block to." ) );
        return false;
    }

    // Get all selected items
    PCB_SELECTION selection = m_toolManager->GetTool<PCB_SELECTION_TOOL>()->GetSelection();

    if( selection.Empty() )
    {
        DisplayErrorMessage( this, _( "Please select some items to save as a design block." ) );
        return false;
    }

    // If we have a single group, we want to strip the group and select the children
    PCB_GROUP* group = nullptr;

    if( selection.Size() == 1 )
    {
        EDA_ITEM* item = selection.Front();

        if( item->Type() == PCB_GROUP_T || item->Type() == PCB_GENERATOR_T )
        {
            group = static_cast<PCB_GROUP*>( item );

            selection.Remove( item );

            // Don't recurse; if we have a group of groups the user probably intends the inner groups to be saved
            group->RunOnChildren( [&]( EDA_ITEM* aItem ) { selection.Add( aItem ); }, RECURSE_MODE::NO_RECURSE );
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

    if( !blk->GetBoardFile().IsEmpty() && !checkOverwriteDbLayout( this, aLibId ) )
        return false;

    if( !saveSelectionToDesignBlock( aLibId.GetLibNickname(), selection, *blk ) )
        return false;

    // If we had a group, we need to reselect it
    if( group )
    {
        selection.Clear();
        selection.Add( group );

        // If we didn't have a design block link before, add one for convenience
        if( !group->HasDesignBlockLink() )
        {
            BOARD_COMMIT commit( m_toolManager );

            commit.Modify( group, nullptr, RECURSE_MODE::NO_RECURSE );
            group->SetDesignBlockLibId( aLibId );

            commit.Push( "Set Group Design Block Link" );
        }
    }

    return true;
}
