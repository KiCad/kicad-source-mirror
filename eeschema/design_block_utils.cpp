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
#include <sch_selection_tool.h>
#include <dialogs/dialog_design_block_properties.h>
#include <nlohmann/json.hpp>


bool checkOverwrite( SCH_EDIT_FRAME* aFrame, wxString& libname, wxString& newName )
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
        if( !LibraryFileBrowser( true, fn, FILEEXT::KiCadDesignBlockLibPathWildcard(),
                                 FILEEXT::KiCadDesignBlockLibPathExtension, true, isGlobal,
                                 PATHS::GetDefaultUserDesignBlocksPath() ) )
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

    // KiCad lib is our default guess.  So it might not have the .kicad_blocks extension
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
    std::vector<SCH_FIELD>& shFields = aSheetPath.Last()->GetFields();
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
    SCH_SELECTION selection = m_toolManager->GetTool<SCH_SELECTION_TOOL>()->GetSelection();

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
        {
            DisplayErrorMessage( this, _( "Design blocks with nested sheets are not supported." ) );
        }

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
        DisplayErrorMessage( this,
                             _( "Error saving temporary schematic file to create design block." ) );
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


bool SCH_EDIT_FRAME::DeleteDesignBlockLibrary( const wxString& aLibName, bool aConfirm )
{
    if( aLibName.IsEmpty() )
    {
        DisplayErrorMessage( this, _( "Please select a library to delete." ) );
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


bool SCH_EDIT_FRAME::EditDesignBlockProperties( const LIB_ID& aLibId )
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

    DESIGN_BLOCK* designBlock = GetDesignBlock( aLibId, true, true );

    if( !designBlock )
        return false;

    wxString                       originalName = designBlock->GetLibId().GetLibItemName();
    DIALOG_DESIGN_BLOCK_PROPERTIES dlg( this, designBlock );

    if( dlg.ShowModal() != wxID_OK )
        return false;

    wxString newName = designBlock->GetLibId().GetLibItemName();

    try
    {
        if( originalName != newName )
        {
            if( Prj().DesignBlockLibs()->DesignBlockExists( libname, newName ) )
                if( !checkOverwrite( this, libname, newName ) )
                    return false;

            Prj().DesignBlockLibs()->DesignBlockSave( libname, designBlock );
            Prj().DesignBlockLibs()->DesignBlockDelete( libname, originalName );
        }
        else
        {
            Prj().DesignBlockLibs()->DesignBlockSave( libname, designBlock );
        }
    }
    catch( const IO_ERROR& ioe )
    {
        DisplayError( this, ioe.What() );
        return false;
    }

    m_designBlocksPane->RefreshLibs();
    m_designBlocksPane->SelectLibId( designBlock->GetLibId() );

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
            wxString msg = wxString::Format( _( "Error loading design block %s from "
                                                "library '%s'." ),
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
