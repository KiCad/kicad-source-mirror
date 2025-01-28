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
#include <design_block_lib_table.h>
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
            DisplayError( this,
                          wxString::Format( _( "Insufficient permissions to write file '%s'." ),
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
            DisplayError( this, wxString::Format( _( "Error saving board file '%s'.\n%s" ),
                                                  pcbFileName.GetFullPath(), ioe.What() ) );
        }

        return false;
    }

    return true;
}


void PCB_EDIT_FRAME::SaveBoardAsDesignBlock( const wxString& aLibraryName )
{
    // Make sure the user has selected a library to save into
    if( m_designBlocksPane->GetSelectedLibId().GetLibNickname().empty() )
    {
        DisplayErrorMessage( this, _( "Please select a library to save the design block to." ) );
        return;
    }

    DESIGN_BLOCK blk;
    wxFileName   fn = wxFileNameFromPath( GetBoard()->GetFileName() );

    blk.SetLibId( LIB_ID( aLibraryName, fn.GetName() ) );

    DIALOG_DESIGN_BLOCK_PROPERTIES dlg( this, &blk );

    if( dlg.ShowModal() != wxID_OK )
        return;

    // Save a temporary copy of the schematic file, as the plugin is just going to move it
    wxString tempFile = wxFileName::CreateTempFileName( "design_block" );

    if( !SavePcbCopy( tempFile, false, false ) )
    {
        DisplayErrorMessage( this,
                             _( "Error saving temporary board file to create design block." ) );
        wxRemoveFile( tempFile );
        return;
    }

    blk.SetBoardFile( tempFile );

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


void PCB_EDIT_FRAME::SaveSelectionAsDesignBlock( const wxString& aLibraryName )
{
    // Make sure the user has selected a library to save into
    if( m_designBlocksPane->GetSelectedLibId().GetLibNickname().empty() )
    {
        DisplayErrorMessage( this, _( "Please select a library to save the design block to." ) );
        return;
    }

    // Get all selected items
    PCB_SELECTION selection = m_toolManager->GetTool<PCB_SELECTION_TOOL>()->GetSelection();

    if( selection.Empty() )
    {
        DisplayErrorMessage( this, _( "Please select some items to save as a design block." ) );
        return;
    }

    DESIGN_BLOCK blk;
    wxFileName   fn = wxFileNameFromPath( GetBoard()->GetFileName() );

    blk.SetLibId( LIB_ID( aLibraryName, fn.GetName() ) );

    DIALOG_DESIGN_BLOCK_PROPERTIES dlg( this, &blk );

    if( dlg.ShowModal() != wxID_OK )
        return;

    // Create a temporary board
    BOARD* tempBoard = new BOARD();
    tempBoard->SetDesignSettings( GetBoard()->GetDesignSettings() );
    tempBoard->SetProject( &Prj(), true );
    tempBoard->SynchronizeProperties();

    // Copy all net info into the new board
    for( NETINFO_ITEM* netInfo : GetBoard()->GetNetInfo() )
    {
        EDA_ITEM* copy = netInfo->Clone();
        tempBoard->Add( static_cast<BOARD_ITEM*>( copy ), ADD_MODE::APPEND, false );
    }

    // Copy the selected items to the temporary board
    for( EDA_ITEM* item : selection )
    {
        EDA_ITEM* copy = item->Clone();
        tempBoard->Add( static_cast<BOARD_ITEM*>( copy ), ADD_MODE::APPEND, false );
    }

    // Rebuild connectivity, remove any unused nets
    tempBoard->BuildListOfNets();
    tempBoard->BuildConnectivity();
    tempBoard->RemoveUnusedNets( nullptr );

    wxString tempFile = wxFileName::CreateTempFileName( "design_block" );

    if( !saveBoardAsFile( tempBoard, tempFile, false ) )
    {
        DisplayErrorMessage( this,
                             _( "Error saving temporary board file to create design block." ) );
        wxRemoveFile( tempFile );
        return;
    }

    blk.SetBoardFile( tempFile );

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
