/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 1992-2024 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <design_block_pane.h>
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

DESIGN_BLOCK_LIB_TABLE* SCH_EDIT_FRAME::selectDesignBlockLibTable( bool aOptional )
{
    // If no project is loaded, always work with the global table
    if( Prj().IsNullProject() )
    {
        DESIGN_BLOCK_LIB_TABLE* ret = &DESIGN_BLOCK_LIB_TABLE::GetGlobalLibTable();

        if( aOptional )
        {
            wxMessageDialog dlg( this, _( "Add the library to the global library table?" ),
                                 _( "Add To Global Library Table" ), wxYES_NO );

            if( dlg.ShowModal() != wxID_OK )
                ret = nullptr;
        }

        return ret;
    }

    wxArrayString libTableNames;
    libTableNames.Add( _( "Global" ) );
    libTableNames.Add( _( "Project" ) );

    wxSingleChoiceDialog dlg( this, _( "Choose the Library Table to add the library to:" ),
                              _( "Add To Library Table" ), libTableNames );

    if( aOptional )
    {
        dlg.FindWindow( wxID_CANCEL )->SetLabel( _( "Skip" ) );
        dlg.FindWindow( wxID_OK )->SetLabel( _( "Add" ) );
    }

    if( dlg.ShowModal() != wxID_OK )
        return nullptr;

    switch( dlg.GetSelection() )
    {
    case 0: return &DESIGN_BLOCK_LIB_TABLE::GetGlobalLibTable();
    case 1: return Prj().DesignBlockLibs();
    default: return nullptr;
    }
}


wxString SCH_EDIT_FRAME::CreateNewDesignBlockLibrary( const wxString& aLibName,
                                                      const wxString& aProposedName )
{
    return createNewDesignBlockLibrary( aLibName, aProposedName, selectDesignBlockLibTable() );
}


wxString SCH_EDIT_FRAME::createNewDesignBlockLibrary( const wxString&         aLibName,
                                                      const wxString&         aProposedName,
                                                      DESIGN_BLOCK_LIB_TABLE* aTable )
{
    if( aTable == nullptr )
        return wxEmptyString;

    wxFileName fn;
    bool       doAdd = false;
    bool       isGlobal = ( aTable == &DESIGN_BLOCK_LIB_TABLE::GetGlobalLibTable() );
    wxString   initialPath = aProposedName;

    if( initialPath.IsEmpty() )
        initialPath = isGlobal ? PATHS::GetDefaultUserDesignBlocksPath() : Prj().GetProjectPath();

    if( aLibName.IsEmpty() )
    {
        fn = initialPath;

        if( !LibraryFileBrowser( false, fn, FILEEXT::KiCadDesignBlockLibPathWildcard(),
                                 FILEEXT::KiCadDesignBlockLibPathExtension, false, isGlobal,
                                 initialPath ) )
        {
            return wxEmptyString;
        }

        doAdd = true;
    }
    else
    {
        fn = EnsureFileExtension( aLibName, FILEEXT::KiCadDesignBlockLibPathExtension );

        if( !fn.IsAbsolute() )
        {
            fn.SetName( aLibName );
            fn.MakeAbsolute( initialPath );
        }
    }

    // We can save libs only using DESIGN_BLOCK_IO_MGR::KICAD_SEXP format (.pretty libraries)
    DESIGN_BLOCK_IO_MGR::DESIGN_BLOCK_FILE_T piType = DESIGN_BLOCK_IO_MGR::KICAD_SEXP;
    wxString                                 libPath = fn.GetFullPath();

    try
    {
        IO_RELEASER<DESIGN_BLOCK_IO> pi( DESIGN_BLOCK_IO_MGR::FindPlugin( piType ) );

        bool writable = false;
        bool exists   = false;

        try
        {
            writable = pi->IsLibraryWritable( libPath );
            exists   = fn.Exists();
        }
        catch( const IO_ERROR& )
        {
            // best efforts....
        }

        if( exists )
        {
            if( !writable )
            {
                wxString msg = wxString::Format( _( "Library %s is read only." ), libPath );
                ShowInfoBarError( msg );
                return wxEmptyString;
            }
            else
            {
                wxString msg = wxString::Format( _( "Library %s already exists." ), libPath );
                KIDIALOG dlg( this, msg, _( "Confirmation" ), wxOK | wxCANCEL | wxICON_WARNING );
                dlg.SetOKLabel( _( "Overwrite" ) );
                dlg.DoNotShowCheckbox( __FILE__, __LINE__ );

                if( dlg.ShowModal() == wxID_CANCEL )
                    return wxEmptyString;

                pi->DeleteLibrary( libPath );
            }
        }

        pi->CreateLibrary( libPath );
    }
    catch( const IO_ERROR& ioe )
    {
        DisplayError( this, ioe.What() );
        return wxEmptyString;
    }

    if( doAdd )
        AddDesignBlockLibrary( libPath, aTable );

    return libPath;
}


bool SCH_EDIT_FRAME::AddDesignBlockLibrary( const wxString&         aFilename,
                                            DESIGN_BLOCK_LIB_TABLE* aTable )
{
    if( aTable == nullptr )
        aTable = selectDesignBlockLibTable();

    if( aTable == nullptr )
        return wxEmptyString;

    bool isGlobal = ( aTable == &DESIGN_BLOCK_LIB_TABLE::GetGlobalLibTable() );

    wxFileName fn( aFilename );

    if( aFilename.IsEmpty() )
    {
        if( !LibraryFileBrowser( true, fn, FILEEXT::KiCadFootprintLibPathWildcard(),
                                 FILEEXT::KiCadFootprintLibPathExtension, true, isGlobal,
                                 PATHS::GetDefaultUserFootprintsPath() ) )
        {
            return false;
        }
    }

    wxString libPath = fn.GetFullPath();
    wxString libName = fn.GetName();

    if( libName.IsEmpty() )
        return false;

    // Open a dialog to ask for a description
    wxString description = wxGetTextFromUser( _( "Enter a description for the library:" ),
                                              _( "Library Description" ), wxEmptyString, this );

    DESIGN_BLOCK_IO_MGR::DESIGN_BLOCK_FILE_T lib_type =
            DESIGN_BLOCK_IO_MGR::GuessPluginTypeFromLibPath( libPath );

    if( lib_type == DESIGN_BLOCK_IO_MGR::FILE_TYPE_NONE )
        lib_type = DESIGN_BLOCK_IO_MGR::KICAD_SEXP;

    wxString type = DESIGN_BLOCK_IO_MGR::ShowType( lib_type );

    // KiCad lib is our default guess.  So it might not have the .blocks extension
    // In this case, the extension is part of the library name
    if( lib_type == DESIGN_BLOCK_IO_MGR::KICAD_SEXP
        && fn.GetExt() != FILEEXT::KiCadDesignBlockLibPathExtension )
        libName = fn.GetFullName();

    // try to use path normalized to an environmental variable or project path
    wxString normalizedPath = NormalizePath( libPath, &Pgm().GetLocalEnvVariables(), &Prj() );

    try
    {
        DESIGN_BLOCK_LIB_TABLE_ROW* row = new DESIGN_BLOCK_LIB_TABLE_ROW(
                libName, normalizedPath, type, wxEmptyString, description );
        aTable->InsertRow( row );

        if( isGlobal )
            DESIGN_BLOCK_LIB_TABLE::GetGlobalLibTable().Save(
                    DESIGN_BLOCK_LIB_TABLE::GetGlobalTableFileName() );
        else
            Prj().DesignBlockLibs()->Save( Prj().DesignBlockLibTblName() );
    }
    catch( const IO_ERROR& ioe )
    {
        DisplayError( this, ioe.What() );
        return false;
    }

    LIB_ID libID( libName, wxEmptyString );
    m_designBlocksPane->RefreshLibs();
    m_designBlocksPane->SelectLibId( libID );

    return true;
}


void SCH_EDIT_FRAME::SaveSheetAsDesignBlock( const wxString& aLibraryName )
{
    // Make sure the user has selected a library to save into
    if( m_designBlocksPane->GetSelectedLibId().GetLibNickname().empty() )
    {
        DisplayError( this, _( "Please select a library to save the design block to." ) );
        return;
    }

    // Just block all attempts to create design blocks with nested sheets at this point
    std::vector<SCH_ITEM*> sheets;
    GetScreen()->GetSheets( &sheets );

    if( !sheets.empty() )
    {
        DisplayError( this, _( "Design blocks with nested sheets are not supported." ) );
        return;
    }

    // Ask the user for the design block name
    wxFileName fn = wxFileNameFromPath( GetScreen()->GetFileName() );

    wxString name = wxGetTextFromUser( _( "Enter a name for the design block:" ),
                                       _( "Design Block Name" ), fn.GetName(), this );

    if( name.IsEmpty() )
        return;

    // Save a temporary copy of the schematic file, as the plugin is just going to move it
    wxString tempFile = wxFileName::CreateTempFileName( "design_block" );
    if( !saveSchematicFile( GetCurrentSheet().Last(), tempFile ) )
    {
        DisplayError( this, _( "Error saving temporary schematic file to create design block." ) );
        wxRemoveFile( tempFile );
        return;
    }


    DESIGN_BLOCK blk;
    blk.SetSchematicFile( tempFile );
    blk.SetLibId( LIB_ID( aLibraryName, name ) );

    try
    {
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
        DisplayError( this, _( "Please select a library to save the design block to." ) );
        return;
    }

    // Get all selected items
    EE_SELECTION selection = m_toolManager->GetTool<EE_SELECTION_TOOL>()->GetSelection();

    if( selection.Empty() )
    {
        DisplayError( this, _( "Please select some items to save as a design block." ) );
        return;
    }

    // Just block all attempts to create design blocks with nested sheets at this point
    if( selection.HasType( SCH_SHEET_T ) )
    {
        DisplayError( this, _( "Design blocks with nested sheets are not supported." ) );
        return;
    }

    // Ask the user for the design block name
    wxString name = wxGetTextFromUser( _( "Enter a name for the design block:" ),
                                       _( "Design Block Name" ), wxEmptyString, this );

    if( name.IsEmpty() )
        return;

    // Create a temperorary screen
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
        DisplayError( this, _( "Error saving temporary schematic file to create design block." ) );
        wxRemoveFile( tempFile );
        return;
    }

    // Create a design block
    DESIGN_BLOCK blk;
    blk.SetSchematicFile( tempFile );
    blk.SetLibId( LIB_ID( aLibraryName, name ) );

    try
    {
        // Actually save it to disk
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


bool SCH_EDIT_FRAME::DeleteDesignBlockLibrary( const wxString& aLibName, bool aConfirm )
{
    if( aLibName.IsEmpty() )
    {
        DisplayError( this, _( "Please select a library to delete." ) );
        return false;
    }

    if( !Prj().DesignBlockLibs()->IsDesignBlockLibWritable( aLibName ) )
    {
        wxString msg = wxString::Format( _( "Library '%s' is read only." ), aLibName );
        ShowInfoBarError( msg );
        return false;
    }

    // Confirmation
    wxString msg = wxString::Format( _( "Delete design block library '%s' from disk? This will "
                                        "delete all design blocks within the library." ),
                                     aLibName.GetData() );

    if( aConfirm && !IsOK( this, msg ) )
        return false;

    try
    {
        Prj().DesignBlockLibs()->DesignBlockLibDelete( aLibName );
    }
    catch( const IO_ERROR& ioe )
    {
        DisplayError( this, ioe.What() );
        return false;
    }

    msg.Printf( _( "Design block library '%s' deleted" ), aLibName.GetData() );
    SetStatusText( msg );

    m_designBlocksPane->RefreshLibs();

    return true;
}


bool SCH_EDIT_FRAME::DeleteDesignBlockFromLibrary( const LIB_ID& aLibId, bool aConfirm )
{
    if( !aLibId.IsValid() )
        return false;

    wxString libname = aLibId.GetLibNickname();
    wxString dbname = aLibId.GetLibItemName();

    if( !Prj().DesignBlockLibs()->IsDesignBlockLibWritable( libname ) )
    {
        wxString msg = wxString::Format( _( "Library '%s' is read only." ), libname );
        ShowInfoBarError( msg );
        return false;
    }

    // Confirmation
    wxString msg = wxString::Format( _( "Delete design block '%s' in library '%s' from disk?" ),
                                     dbname.GetData(), libname.GetData() );

    if( aConfirm && !IsOK( this, msg ) )
        return false;

    try
    {
        Prj().DesignBlockLibs()->DesignBlockDelete( libname, dbname );
    }
    catch( const IO_ERROR& ioe )
    {
        DisplayError( this, ioe.What() );
        return false;
    }

    msg.Printf( _( "Design block '%s' deleted from library '%s'" ), dbname.GetData(),
                libname.GetData() );

    SetStatusText( msg );

    m_designBlocksPane->RefreshLibs();

    return true;
}


DESIGN_BLOCK* SchGetDesignBlock( const LIB_ID& aLibId, DESIGN_BLOCK_LIB_TABLE* aLibTable,
                                 wxWindow* aParent, bool aShowErrorMsg )
{
    wxCHECK_MSG( aLibTable, nullptr, wxS( "Invalid design block library table." ) );

    DESIGN_BLOCK* designBlock = nullptr;

    try
    {
        designBlock = aLibTable->DesignBlockLoadWithOptionalNickname( aLibId, true );
    }
    catch( const IO_ERROR& ioe )
    {
        if( aShowErrorMsg )
        {
            wxString msg = wxString::Format( _( "Error loading designblock %s from library '%s'." ),
                                             aLibId.GetLibItemName().wx_str(),
                                             aLibId.GetLibNickname().wx_str() );
            DisplayErrorMessage( aParent, msg, ioe.What() );
        }
    }

    return designBlock;
}


DESIGN_BLOCK* SCH_EDIT_FRAME::GetDesignBlock( const LIB_ID& aLibId, bool aUseCacheLib,
                                              bool aShowErrorMsg )
{
    return SchGetDesignBlock( aLibId, Prj().DesignBlockLibs(), this, aShowErrorMsg );
}


void SCH_EDIT_FRAME::UpdateDesignBlockOptions()
{
    m_designBlocksPane->UpdateCheckboxes();
}
