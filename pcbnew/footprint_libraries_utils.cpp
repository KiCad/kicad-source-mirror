/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 1992-2023 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <wx/ffile.h>
#include <pgm_base.h>
#include <kiface_base.h>
#include <confirm.h>
#include <string_utils.h>
#include <macros.h>
#include <pcb_edit_frame.h>
#include <eda_list_dialog.h>
#include <filter_reader.h>
#include <fp_lib_table.h>
#include <validators.h>
#include <dialogs/dialog_text_entry.h>
#include <tool/tool_manager.h>
#include <tools/pcb_actions.h>
#include <tools/board_editor_control.h>
#include <tools/pad_tool.h>
#include <footprint.h>
#include <zone.h>
#include <pcb_group.h>
#include <board_commit.h>
#include <footprint_edit_frame.h>
#include <wildcards_and_files_ext.h>
#include <pcb_io/kicad_legacy/pcb_io_kicad_legacy.h>
#include <pcb_io/kicad_sexpr/pcb_io_kicad_sexpr.h>
#include <env_paths.h>
#include <paths.h>
#include <settings/settings_manager.h>
#include <project_pcb.h>
#include <project/project_file.h>
#include <footprint_editor_settings.h>
#include <footprint_viewer_frame.h>
#include <wx/choicdlg.h>
#include <wx/filedlg.h>
#include <wx/fswatcher.h>


// unique, "file local" translations:


static const wxString INFO_LEGACY_LIB_WARN_EDIT(
        _(  "Writing/modifying legacy libraries (.mod files) is not allowed\n"\
            "Please save the current library to the new .pretty format\n"\
            "and update your footprint lib table\n"\
            "to save your footprint (a .kicad_mod file) in the .pretty library folder" ) );

static const wxString INFO_LEGACY_LIB_WARN_DELETE(
        _(  "Modifying legacy libraries (.mod files) is not allowed\n"\
            "Please save the current library under the new .pretty format\n"\
            "and update your footprint lib table\n"\
            "before deleting a footprint" ) );


FOOTPRINT* FOOTPRINT_EDIT_FRAME::ImportFootprint( const wxString& aName )
{
    wxFileName fn;

    if( !aName.empty() )
    {
        fn = aName;
    }
    else
    {
        // Prompt the user for a footprint file to open.
        static int               lastFilterIndex = 0; // To store the last choice during a session.
        wxString                 fileFiltersStr;
        std::vector<std::string> allExtensions;
        std::set<wxString>       allWildcardsSet;

        for( const auto& plugin : PCB_IO_MGR::PLUGIN_REGISTRY::Instance()->AllPlugins() )
        {
            IO_RELEASER<PCB_IO> pi( plugin.m_createFunc() );

            if( !pi )
                continue;

            const IO_BASE::IO_FILE_DESC& desc = pi->GetLibraryFileDesc();

            if( !desc )
                continue;

            if( !fileFiltersStr.IsEmpty() )
                fileFiltersStr += wxChar( '|' );

            fileFiltersStr += desc.FileFilter();

            for( const std::string& ext : desc.m_FileExtensions )
            {
                allExtensions.emplace_back( ext );
                allWildcardsSet.insert( wxT( "*." ) + formatWildcardExt( ext ) + wxT( ";" ) );
            }
        }

        wxString allWildcardsStr;
        for( const wxString& wildcard : allWildcardsSet )
            allWildcardsStr << wildcard;

        fileFiltersStr = _( "All supported formats" ) + wxT( "|" ) + allWildcardsStr + wxT( "|" )
                         + fileFiltersStr;

        wxFileDialog dlg( this, _( "Import Footprint" ), m_mruPath, wxEmptyString, fileFiltersStr,
                          wxFD_OPEN | wxFD_FILE_MUST_EXIST );

        dlg.SetFilterIndex( lastFilterIndex );

        if( dlg.ShowModal() == wxID_CANCEL )
            return nullptr;

        lastFilterIndex = dlg.GetFilterIndex();

        fn = dlg.GetPath();
    }

    if( !fn.IsOk() )
        return nullptr;

    if( !wxFileExists( fn.GetFullPath() ) )
    {
        wxString msg = wxString::Format( _( "File '%s' not found." ), fn.GetFullPath() );
        DisplayError( this, msg );
        return nullptr;
    }

    m_mruPath = fn.GetPath();

    PCB_IO_MGR::PCB_FILE_T fileType = PCB_IO_MGR::FILE_TYPE_NONE;

    for( const auto& plugin : PCB_IO_MGR::PLUGIN_REGISTRY::Instance()->AllPlugins() )
    {
        IO_RELEASER<PCB_IO> pi( plugin.m_createFunc() );

        if( !pi )
            continue;

        if( pi->GetLibraryFileDesc().m_FileExtensions.empty() )
            continue;

        if( pi->CanReadFootprint( fn.GetFullPath() ) )
        {
            fileType = plugin.m_type;
            break;
        }
    }

    if( fileType == PCB_IO_MGR::FILE_TYPE_NONE )
    {
        DisplayError( this, _( "Not a footprint file." ) );
        return nullptr;
    }

    FOOTPRINT* footprint = nullptr;
    wxString   footprintName;

    try
    {
        IO_RELEASER<PCB_IO> pi( PCB_IO_MGR::PluginFind( fileType ) );

        footprint = pi->ImportFootprint( fn.GetFullPath(), footprintName);

        if( !footprint )
        {
            wxString msg = wxString::Format( _( "Unable to load footprint '%s' from '%s'" ),
                                             footprintName, fn.GetFullPath() );
            DisplayError( this, msg );
            return nullptr;
        }
    }
    catch( const IO_ERROR& ioe )
    {
        DisplayError( this, ioe.What() );

        // if the footprint is not loaded, exit.
        // However, even if an error happens, it can be loaded, because in KICAD and GPCB format,
        // a fp library is a set of separate files, and the error(s) are not necessary when
        // reading the selected file

        if( !footprint )
            return nullptr;
    }

    footprint->SetFPID( LIB_ID( wxEmptyString, footprintName ) );

    // Insert footprint in list
    AddFootprintToBoard( footprint );

    // Display info :
    SetMsgPanel( footprint );
    PlaceFootprint( footprint );

    footprint->SetPosition( VECTOR2I( 0, 0 ) );

    GetBoard()->BuildListOfNets();
    UpdateView();

    return footprint;
}


void FOOTPRINT_EDIT_FRAME::ExportFootprint( FOOTPRINT* aFootprint )
{
    wxFileName fn;
    FOOTPRINT_EDITOR_SETTINGS* cfg = GetSettings();

    if( !aFootprint )
        return;

    fn.SetName( aFootprint->GetFPID().GetLibItemName() );

    wxString wildcard = FILEEXT::KiCadFootprintLibFileWildcard();

    fn.SetExt( FILEEXT::KiCadFootprintFileExtension );

    if( !cfg->m_LastExportPath.empty() )
        fn.SetPath( cfg->m_LastExportPath );
    else
        fn.SetPath( m_mruPath );

    wxFileDialog dlg( this, _( "Export Footprint" ), fn.GetPath(), fn.GetFullName(),
                      wildcard, wxFD_SAVE | wxFD_OVERWRITE_PROMPT );

    if( dlg.ShowModal() == wxID_CANCEL )
        return;

    fn = EnsureFileExtension( dlg.GetPath(), FILEEXT::KiCadFootprintFileExtension );
    cfg->m_LastExportPath = fn.GetPath();

    try
    {
        // Export as *.kicad_pcb format, using a strategy which is specifically chosen
        // as an example on how it could also be used to send it to the system clipboard.

        PCB_IO_KICAD_SEXPR  pcb_io(CTL_FOR_LIBRARY );

        /*  This footprint should *already* be "normalized" in a way such that
            orientation is zero, etc., since it came from the Footprint Editor.

            aFootprint->SetParent( 0 );
            aFootprint->SetOrientation( 0 );
        */

        pcb_io.Format( aFootprint );

        FILE* fp = wxFopen( dlg.GetPath(), wxT( "wt" ) );

        if( fp == nullptr )
        {
            DisplayErrorMessage( this, wxString::Format( _( "Insufficient permissions to write file '%s'." ),
                                            dlg.GetPath() ) );
            return;
        }

        fprintf( fp, "%s", pcb_io.GetStringOutput( false ).c_str() );
        fclose( fp );
    }
    catch( const IO_ERROR& ioe )
    {
        DisplayError( this, ioe.What() );
        return;
    }

    wxString msg = wxString::Format( _( "Footprint exported to file '%s'." ), dlg.GetPath() );
    DisplayInfoMessage( this, msg );
}


wxString PCB_BASE_EDIT_FRAME::CreateNewProjectLibrary( const wxString& aLibName,
                                                       const wxString& aProposedName )
{
    return createNewLibrary( aLibName, aProposedName, PROJECT_PCB::PcbFootprintLibs( &Prj() ) );
}


wxString PCB_BASE_EDIT_FRAME::CreateNewLibrary( const wxString& aLibName,
                                                const wxString& aProposedName )
{
    FP_LIB_TABLE* table  = selectLibTable();

    return createNewLibrary( aLibName, aProposedName, table );
}


wxString PCB_BASE_EDIT_FRAME::createNewLibrary( const wxString& aLibName,
                                                const wxString& aProposedName,
                                                FP_LIB_TABLE* aTable )
{
    // Kicad cannot write legacy format libraries, only .pretty new format because the legacy
    // format cannot handle current features.
    // The footprint library is actually a directory.

    if( aTable == nullptr )
        return wxEmptyString;

    wxString   initialPath = aProposedName.IsEmpty() ? Prj().GetProjectPath() : aProposedName;
    wxFileName fn;
    bool       doAdd = false;
    bool       isGlobal = ( aTable == &GFootprintTable );

    if( aLibName.IsEmpty() )
    {
        fn = initialPath;

        if( !LibraryFileBrowser( false, fn, FILEEXT::KiCadFootprintLibPathWildcard(),
                                 FILEEXT::KiCadFootprintLibPathExtension, false, isGlobal,
                                 PATHS::GetDefaultUserFootprintsPath() ) )
        {
            return wxEmptyString;
        }

        doAdd = true;
    }
    else
    {
        fn = EnsureFileExtension( aLibName, FILEEXT::KiCadFootprintLibPathExtension );

        if( !fn.IsAbsolute() )
        {
            fn.SetName( aLibName );
            fn.MakeAbsolute( initialPath );
        }
    }

    // We can save fp libs only using PCB_IO_MGR::KICAD_SEXP format (.pretty libraries)
    PCB_IO_MGR::PCB_FILE_T piType  = PCB_IO_MGR::KICAD_SEXP;
    wxString           libPath = fn.GetFullPath();

    try
    {
        IO_RELEASER<PCB_IO> pi( PCB_IO_MGR::PluginFind( piType ) );

        bool writable = false;
        bool exists   = false;

        try
        {
            writable = pi->IsLibraryWritable( libPath );
            exists   = true;    // no exception was thrown, lib must exist.
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
        AddLibrary( libPath, aTable );

    return libPath;
}


FP_LIB_TABLE* PCB_BASE_EDIT_FRAME::selectLibTable( bool aOptional )
{
    // If no project is loaded, always work with the global table
    if( Prj().IsNullProject() )
    {
        FP_LIB_TABLE* ret = &GFootprintTable;

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
    case 0: return &GFootprintTable;
    case 1: return PROJECT_PCB::PcbFootprintLibs( &Prj() );
    default: return nullptr;
    }
}


bool PCB_BASE_EDIT_FRAME::AddLibrary( const wxString& aFilename, FP_LIB_TABLE* aTable )
{
    if( aTable == nullptr )
        aTable = selectLibTable();

    if( aTable == nullptr )
        return wxEmptyString;

    bool isGlobal = ( aTable == &GFootprintTable );

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

    PCB_IO_MGR::PCB_FILE_T lib_type = PCB_IO_MGR::GuessPluginTypeFromLibPath( libPath );

    if( lib_type == PCB_IO_MGR::FILE_TYPE_NONE )
        lib_type = PCB_IO_MGR::KICAD_SEXP;

    wxString type = PCB_IO_MGR::ShowType( lib_type );

    // KiCad lib is our default guess.  So it might not have the .pretty extension
    // In this case, the extension is part of the library name
    if( lib_type == PCB_IO_MGR::KICAD_SEXP
        && fn.GetExt() != FILEEXT::KiCadFootprintLibPathExtension )
        libName = fn.GetFullName();

    // try to use path normalized to an environmental variable or project path
    wxString normalizedPath = NormalizePath( libPath, &Pgm().GetLocalEnvVariables(), &Prj() );

    try
    {
        FP_LIB_TABLE_ROW* row = new FP_LIB_TABLE_ROW( libName, normalizedPath, type, wxEmptyString );
        aTable->InsertRow( row );

        if( isGlobal )
            GFootprintTable.Save( FP_LIB_TABLE::GetGlobalTableFileName() );
        else
            PROJECT_PCB::PcbFootprintLibs( &Prj() )->Save( Prj().FootprintLibTblName() );
    }
    catch( const IO_ERROR& ioe )
    {
        DisplayError( this, ioe.What() );
        return false;
    }

    auto editor = (FOOTPRINT_EDIT_FRAME*) Kiway().Player( FRAME_FOOTPRINT_EDITOR, false );

    if( editor )
    {
        LIB_ID libID( libName, wxEmptyString );
        editor->SyncLibraryTree( true );
        editor->FocusOnLibID( libID );
    }

    auto viewer = (FOOTPRINT_VIEWER_FRAME*) Kiway().Player( FRAME_FOOTPRINT_VIEWER, false );

    if( viewer )
        viewer->ReCreateLibraryList();

    return true;
}


bool FOOTPRINT_EDIT_FRAME::DeleteFootprintFromLibrary( const LIB_ID& aFPID, bool aConfirm )
{
    if( !aFPID.IsValid() )
        return false;

    wxString nickname = aFPID.GetLibNickname();
    wxString fpname = aFPID.GetLibItemName();

    // Legacy libraries are readable, but modifying legacy format is not allowed
    // So prompt the user if he try to delete a footprint from a legacy lib
    wxString libfullname = PROJECT_PCB::PcbFootprintLibs( &Prj() )->FindRow( nickname )->GetFullURI();

    if( PCB_IO_MGR::GuessPluginTypeFromLibPath( libfullname ) == PCB_IO_MGR::LEGACY )
    {
        DisplayInfoMessage( this, INFO_LEGACY_LIB_WARN_DELETE );
        return false;
    }

    if( !PROJECT_PCB::PcbFootprintLibs( &Prj() )->IsFootprintLibWritable( nickname ) )
    {
        wxString msg = wxString::Format( _( "Library '%s' is read only." ), nickname );
        ShowInfoBarError( msg );
        return false;
    }

    // Confirmation
    wxString msg = wxString::Format( _( "Delete footprint '%s' from library '%s'?" ),
                                     fpname.GetData(),
                                     nickname.GetData() );

    if( aConfirm && !IsOK( this, msg ) )
        return false;

    try
    {
        PROJECT_PCB::PcbFootprintLibs( &Prj() )->FootprintDelete( nickname, fpname );
    }
    catch( const IO_ERROR& ioe )
    {
        DisplayError( this, ioe.What() );
        return false;
    }

    msg.Printf( _( "Footprint '%s' deleted from library '%s'" ),
                fpname.GetData(),
                nickname.GetData() );

    SetStatusText( msg );

    return true;
}


void PCB_EDIT_FRAME::ExportFootprintsToLibrary( bool aStoreInNewLib, const wxString& aLibName,
                                                wxString* aLibPath )
{
    if( GetBoard()->GetFirstFootprint() == nullptr )
    {
        DisplayInfoMessage( this, _( "No footprints to export!" ) );
        return;
    }

    wxString footprintName;

    auto resetReference =
            []( FOOTPRINT* aFootprint )
            {
                aFootprint->SetReference( "REF**" );
            };

    auto resetGroup =
            []( FOOTPRINT* aFootprint )
            {
                if( PCB_GROUP* parentGroup = aFootprint->GetParentGroup() )
                    parentGroup->RemoveItem( aFootprint );
            };

    auto resetZones =
            []( FOOTPRINT* aFootprint )
            {
                for( ZONE* zone : aFootprint->Zones() )
                    zone->Move( -aFootprint->GetPosition() );
            };

    if( !aStoreInNewLib )
    {
        // The footprints are saved in an existing .pretty library in the fp lib table
        PROJECT& prj = Prj();
        wxString last_nickname = prj.GetRString( PROJECT::PCB_LIB_NICKNAME );
        wxString nickname = SelectLibrary( last_nickname );

        if( !nickname )     // Aborted
            return;

        bool map = IsOK( this, wxString::Format( _( "Update footprints on board to refer to %s?" ),
                                                 nickname ) );

        prj.SetRString( PROJECT::PCB_LIB_NICKNAME, nickname );

        for( FOOTPRINT* footprint : GetBoard()->Footprints() )
        {
            try
            {
                FP_LIB_TABLE* tbl = PROJECT_PCB::PcbFootprintLibs( &prj );

                if( !footprint->GetFPID().GetLibItemName().empty() )    // Handle old boards.
                {
                    FOOTPRINT* fpCopy = static_cast<FOOTPRINT*>( footprint->Duplicate() );

                    // Reset reference designator and group membership before saving
                    resetReference( fpCopy );
                    resetGroup( fpCopy );
                    resetZones( fpCopy );

                    tbl->FootprintSave( nickname, fpCopy, true );

                    delete fpCopy;
                }
            }
            catch( const IO_ERROR& ioe )
            {
                DisplayError( this, ioe.What() );
            }

            if( map )
            {
                LIB_ID id = footprint->GetFPID();
                id.SetLibNickname( nickname );
                footprint->SetFPID( id );
            }
        }
    }
    else
    {
        // The footprints are saved in a new .pretty library.
        // If this library already exists, all previous footprints will be deleted
        wxString libPath = CreateNewLibrary( aLibName );

        if( libPath.IsEmpty() )     // Aborted
            return;

        if( aLibPath )
            *aLibPath = libPath;

        wxString libNickname;
        bool     map = IsOK( this, _( "Update footprints on board to refer to new library?" ) );

        if( map )
        {
            const LIB_TABLE_ROW* row = PROJECT_PCB::PcbFootprintLibs( &Prj() )->FindRowByURI( libPath );

            if( row )
                libNickname = row->GetNickName();
        }

        PCB_IO_MGR::PCB_FILE_T piType = PCB_IO_MGR::KICAD_SEXP;
        IO_RELEASER<PCB_IO>    pi( PCB_IO_MGR::PluginFind( piType ) );

        for( FOOTPRINT* footprint : GetBoard()->Footprints() )
        {
            try
            {
                if( !footprint->GetFPID().GetLibItemName().empty() )    // Handle old boards.
                {
                    FOOTPRINT* fpCopy = static_cast<FOOTPRINT*>( footprint->Duplicate() );

                    // Reset reference designator and group membership before saving
                    resetReference( fpCopy );
                    resetGroup( fpCopy );
                    resetZones( fpCopy );

                    pi->FootprintSave( libPath, fpCopy );

                    delete fpCopy;
                }
            }
            catch( const IO_ERROR& ioe )
            {
                DisplayError( this, ioe.What() );
            }

            if( map )
            {
                LIB_ID id = footprint->GetFPID();
                id.SetLibNickname( libNickname );
                footprint->SetFPID( id );
            }
        }
    }
}


bool FOOTPRINT_EDIT_FRAME::SaveFootprint( FOOTPRINT* aFootprint )
{
    if( !aFootprint )      // Happen if no footprint loaded
        return false;

    PAD_TOOL* padTool = m_toolManager->GetTool<PAD_TOOL>();

    if( padTool->InPadEditMode() )
        m_toolManager->RunAction( PCB_ACTIONS::recombinePad );

    wxString libraryName = aFootprint->GetFPID().GetLibNickname();
    wxString footprintName = aFootprint->GetFPID().GetLibItemName();
    bool     nameChanged = m_footprintNameWhenLoaded != footprintName;

    if( aFootprint->GetLink() != niluuid )
    {
        if( SaveFootprintToBoard( false ) )
        {
            m_footprintNameWhenLoaded = footprintName;
            return true;
        }
        else
        {
            return false;
        }
    }
    else if( libraryName.IsEmpty() || footprintName.IsEmpty() )
    {
        if( SaveFootprintAs( aFootprint ) )
        {
            m_footprintNameWhenLoaded = footprintName;
            SyncLibraryTree( true );
            return true;
        }
        else
        {
            return false;
        }
    }

    FP_LIB_TABLE* tbl = PROJECT_PCB::PcbFootprintLibs( &Prj() );

    // Legacy libraries are readable, but modifying legacy format is not allowed
    // So prompt the user if he try to add/replace a footprint in a legacy lib
    wxString libfullname;

    try
    {
        libfullname = tbl->FindRow( libraryName )->GetFullURI();
    }
    catch( IO_ERROR& error )
    {
        DisplayInfoMessage( this, error.What() );
        return false;
    }

    if( PCB_IO_MGR::GuessPluginTypeFromLibPath( libfullname ) == PCB_IO_MGR::LEGACY )
    {
        DisplayInfoMessage( this, INFO_LEGACY_LIB_WARN_EDIT );
        return false;
    }

    if( nameChanged )
    {
        LIB_ID oldFPID( libraryName, m_footprintNameWhenLoaded );
        DeleteFootprintFromLibrary( oldFPID, false );
    }

    if( !SaveFootprintInLibrary( aFootprint, libraryName ) )
        return false;

    if( nameChanged )
    {
        m_footprintNameWhenLoaded = footprintName;
        SyncLibraryTree( true );
    }

    return true;
}


bool FOOTPRINT_EDIT_FRAME::DuplicateFootprint( FOOTPRINT* aFootprint )
{
    LIB_ID     fpID = aFootprint->GetFPID();
    wxString   libraryName = fpID.GetLibNickname();
    wxString   footprintName = fpID.GetLibItemName();

    // Legacy libraries are readable, but modifying legacy format is not allowed
    // So prompt the user if he try to add/replace a footprint in a legacy lib
    wxString libFullName = PROJECT_PCB::PcbFootprintLibs( &Prj() )->FindRow( libraryName )->GetFullURI();

    if( PCB_IO_MGR::GuessPluginTypeFromLibPath( libFullName ) == PCB_IO_MGR::LEGACY )
    {
        DisplayInfoMessage( this, INFO_LEGACY_LIB_WARN_EDIT );
        return false;
    }

    FP_LIB_TABLE* tbl = PROJECT_PCB::PcbFootprintLibs( &Prj() );
    int           i = 1;
    wxString      newName = footprintName;

    // Append a number to the name until the name is unique in the library.
    while( tbl->FootprintExists( libraryName, newName ) )
        newName.Printf( "%s_%d", footprintName, i++ );

    aFootprint->SetFPID( LIB_ID( libraryName, newName ) );

    if( aFootprint->GetValue() == footprintName )
        aFootprint->SetValue( newName );

    return SaveFootprintInLibrary( aFootprint, libraryName );
}


bool FOOTPRINT_EDIT_FRAME::SaveFootprintInLibrary( FOOTPRINT* aFootprint,
                                                   const wxString& aLibraryName )
{
    try
    {
        aFootprint->SetFPID( LIB_ID( wxEmptyString, aFootprint->GetFPID().GetLibItemName() ) );

        PROJECT_PCB::PcbFootprintLibs( &Prj() )->FootprintSave( aLibraryName, aFootprint );

        aFootprint->SetFPID( LIB_ID( aLibraryName, aFootprint->GetFPID().GetLibItemName() ) );

        if( aFootprint == GetBoard()->GetFirstFootprint() )
            setFPWatcher( aFootprint );

        return true;
    }
    catch( const IO_ERROR& ioe )
    {
        DisplayError( this, ioe.What() );

        aFootprint->SetFPID( LIB_ID( aLibraryName, aFootprint->GetFPID().GetLibItemName() ) );
        return false;
    }
}


bool FOOTPRINT_EDIT_FRAME::SaveFootprintToBoard( bool aAddNew )
{
    // update footprint in the current board,
    // not just add it to the board with total disregard for the netlist...
    PCB_EDIT_FRAME* pcbframe = (PCB_EDIT_FRAME*) Kiway().Player( FRAME_PCB_EDITOR, false );

    if( pcbframe == nullptr )       // happens when the board editor is not active (or closed)
    {
        ShowInfoBarError( _( "No board currently open." ) );
        return false;
    }

    BOARD*     mainpcb  = pcbframe->GetBoard();
    FOOTPRINT* sourceFootprint  = nullptr;
    FOOTPRINT* editorFootprint = GetBoard()->GetFirstFootprint();

    // Search the old footprint (source) if exists
    // Because this source could be deleted when editing the main board...
    if( editorFootprint->GetLink() != niluuid )        // this is not a new footprint ...
    {
        sourceFootprint = nullptr;

        for( FOOTPRINT* candidate : mainpcb->Footprints() )
        {
            if( editorFootprint->GetLink() == candidate->m_Uuid )
            {
                sourceFootprint = candidate;
                break;
            }
        }
    }

    if( !aAddNew && sourceFootprint == nullptr )    // source not found
    {
        DisplayError( this, _( "Unable to find the footprint on the main board.\nCannot save." ) );
        return false;
    }

    TOOL_MANAGER* pcb_ToolManager = pcbframe->GetToolManager();

    if( aAddNew && pcb_ToolManager->GetTool<BOARD_EDITOR_CONTROL>()->PlacingFootprint() )
    {
        DisplayError( this, _( "Previous footprint placement still in progress." ) );
        return false;
    }

    m_toolManager->RunAction( PCB_ACTIONS::selectionClear );
    BOARD_COMMIT commit( pcbframe );

    // Create a copy for the board, first using Clone() to keep existing Uuids, and then either
    // resetting the uuids to the board values or assigning new Uuids.
    FOOTPRINT* newFootprint = static_cast<FOOTPRINT*>( editorFootprint->Clone() );
    newFootprint->SetParent( mainpcb );
    newFootprint->SetLink( niluuid );

    auto fixUuid =
            [&]( KIID& aUuid )
            {
                if( editorFootprint->GetLink() != niluuid && m_boardFootprintUuids.count( aUuid ) )
                    aUuid = m_boardFootprintUuids[ aUuid ];
                else
                    aUuid = KIID();
            };

    fixUuid( const_cast<KIID&>( newFootprint->m_Uuid ) );

    newFootprint->RunOnDescendants(
            [&]( BOARD_ITEM* aChild )
            {
                fixUuid( const_cast<KIID&>( aChild->m_Uuid ) );
            } );

    BOARD_DESIGN_SETTINGS& bds = m_pcb->GetDesignSettings();

    newFootprint->ApplyDefaultSettings( *m_pcb, bds.m_StyleFPFields, bds.m_StyleFPText,
                                        bds.m_StyleFPShapes );

    if( sourceFootprint )         // this is an update command
    {
        // In the main board the new footprint replaces the old one (pos, orient, ref, value,
        // connections and properties are kept) and the sourceFootprint (old footprint) is
        // deleted
        pcbframe->ExchangeFootprint( sourceFootprint, newFootprint, commit );
        commit.Push( _( "Update Footprint" ) );
    }
    else        // This is an insert command
    {
        KIGFX::VIEW_CONTROLS* viewControls = pcbframe->GetCanvas()->GetViewControls();
        VECTOR2D              cursorPos = viewControls->GetCursorPosition();

        commit.Add( newFootprint );
        viewControls->SetCrossHairCursorPosition( VECTOR2D( 0, 0 ), false );
        pcbframe->PlaceFootprint( newFootprint );
        newFootprint->SetPosition( VECTOR2I( 0, 0 ) );
        viewControls->SetCrossHairCursorPosition( cursorPos, false );
        const_cast<KIID&>( newFootprint->m_Uuid ) = KIID();
        commit.Push( _( "Insert Footprint" ) );

        pcbframe->Raise();
        pcb_ToolManager->RunAction( PCB_ACTIONS::placeFootprint, newFootprint );
    }

    newFootprint->ClearFlags();

    return true;
}


static int ID_MAKE_NEW_LIBRARY = 4173;


class SAVE_AS_DIALOG : public EDA_LIST_DIALOG
{
public:
    SAVE_AS_DIALOG( FOOTPRINT_EDIT_FRAME* aParent, const wxString& aFootprintName,
                    const wxString& aLibraryPreselect,
                    std::function<bool( wxString libName, wxString fpName )> aValidator ) :
            EDA_LIST_DIALOG( aParent, _( "Save Footprint As" ), false ),
            m_validator( std::move( aValidator ) )
    {
        COMMON_SETTINGS*           cfg = Pgm().GetCommonSettings();
        PROJECT_FILE&              project = aParent->Prj().GetProjectFile();
        FP_LIB_TABLE*              tbl = PROJECT_PCB::PcbFootprintLibs( &aParent->Prj() );
        std::vector<wxString>      nicknames = tbl->GetLogicalLibs();
        wxArrayString              headers;
        std::vector<wxArrayString> itemsToDisplay;

        headers.Add( _( "Nickname" ) );
        headers.Add( _( "Description" ) );

        for( const wxString& nickname : nicknames )
        {
            if( alg::contains( project.m_PinnedFootprintLibs, nickname )
                    || alg::contains( cfg->m_Session.pinned_fp_libs, nickname ) )
            {
                wxArrayString item;

                item.Add( LIB_TREE_MODEL_ADAPTER::GetPinningSymbol() + nickname );
                item.Add( tbl->GetDescription( nickname ) );
                itemsToDisplay.push_back( item );
            }
        }

        for( const wxString& nickname : nicknames )
        {
            if( !alg::contains( project.m_PinnedFootprintLibs, nickname )
                    && !alg::contains( cfg->m_Session.pinned_fp_libs, nickname ) )
            {
                wxArrayString item;

                item.Add( nickname );
                item.Add( tbl->GetDescription( nickname ) );
                itemsToDisplay.push_back( item );
            }
        }
        initDialog( headers, itemsToDisplay, aLibraryPreselect );

        SetListLabel( _( "Save in library:" ) );
        SetOKLabel( _( "Save" ) );

        wxBoxSizer* bNameSizer = new wxBoxSizer( wxHORIZONTAL );

        wxStaticText* label = new wxStaticText( this, wxID_ANY, _( "Name:" ) );
        bNameSizer->Add( label, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM|wxLEFT, 5 );

        m_fpNameCtrl = new wxTextCtrl( this, wxID_ANY, aFootprintName );
        bNameSizer->Add( m_fpNameCtrl, 1, wxALIGN_CENTER_VERTICAL|wxALL, 5 );

        wxTextValidator nameValidator( wxFILTER_EXCLUDE_CHAR_LIST );
        nameValidator.SetCharExcludes( FOOTPRINT::StringLibNameInvalidChars( false ) );
        m_fpNameCtrl->SetValidator( nameValidator );

        wxButton* newLibraryButton = new wxButton( this, ID_MAKE_NEW_LIBRARY, _( "New Library..." ) );
        m_ButtonsSizer->Prepend( 80, 20 );
        m_ButtonsSizer->Prepend( newLibraryButton, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT, 10 );

        GetSizer()->Prepend( bNameSizer, 0, wxEXPAND|wxTOP|wxLEFT|wxRIGHT, 5 );

        Bind( wxEVT_BUTTON,
                [this]( wxCommandEvent& )
                {
                    EndModal( ID_MAKE_NEW_LIBRARY );
                }, ID_MAKE_NEW_LIBRARY );

        // Move nameTextCtrl to the head of the tab-order
        if( GetChildren().DeleteObject( m_fpNameCtrl ) )
            GetChildren().Insert( m_fpNameCtrl );

        SetInitialFocus( m_fpNameCtrl );

        SetupStandardButtons();

        Layout();
        GetSizer()->Fit( this );

        Centre();
    }

    wxString GetFPName()
    {
        wxString footprintName = m_fpNameCtrl->GetValue();
        footprintName.Trim( true );
        footprintName.Trim( false );
        return footprintName;
    }

protected:
    bool TransferDataFromWindow() override
    {
        return m_validator( GetTextSelection(), GetFPName() );
    }

private:
    wxTextCtrl*                                              m_fpNameCtrl;
    std::function<bool( wxString libName, wxString fpName )> m_validator;
};


bool FOOTPRINT_EDIT_FRAME::SaveFootprintAs( FOOTPRINT* aFootprint )
{
    if( aFootprint == nullptr )
        return false;

    FP_LIB_TABLE* tbl = PROJECT_PCB::PcbFootprintLibs( &Prj() );

    SetMsgPanel( aFootprint );

    wxString libraryName = aFootprint->GetFPID().GetLibNickname();
    wxString footprintName = aFootprint->GetFPID().GetLibItemName();
    bool     updateValue = aFootprint->GetValue() == footprintName;
    bool     done = false;
    bool     footprintExists = false;

    while( !done )
    {
        SAVE_AS_DIALOG dlg( this, footprintName, libraryName,
                [&]( const wxString& newLib, const wxString& newName )
                {
                    if( newLib.IsEmpty() )
                    {
                        wxMessageBox( _( "A library must be specified." ) );
                        return false;
                    }

                    if( newName.IsEmpty() )
                    {
                        wxMessageBox( _( "Footprint must have a name." ) );
                        return false;
                    }

                    // Legacy libraries are readable, but modifying legacy format is not allowed
                    // So prompt the user if he try to add/replace a footprint in a legacy lib
                    const FP_LIB_TABLE_ROW* row = PROJECT_PCB::PcbFootprintLibs( &Prj() )->FindRow( newLib );
                    wxString                libPath = row->GetFullURI();
                    PCB_IO_MGR::PCB_FILE_T  piType = PCB_IO_MGR::GuessPluginTypeFromLibPath( libPath );

                    if( piType == PCB_IO_MGR::LEGACY )
                    {
                        DisplayInfoMessage( this, INFO_LEGACY_LIB_WARN_EDIT );
                        return false;
                    }

                    footprintExists = tbl->FootprintExists( newLib, newName );

                    if( footprintExists )
                    {
                        wxString msg = wxString::Format( _( "Footprint %s already exists in %s." ),
                                                         newName,
                                                         newLib );

                        KIDIALOG errorDlg( this, msg, _( "Confirmation" ),
                                           wxOK | wxCANCEL | wxICON_WARNING );
                        errorDlg.SetOKLabel( _( "Overwrite" ) );

                        return errorDlg.ShowModal() == wxID_OK;
                    }

                    return true;
                } );

        int ret = dlg.ShowModal();

        if( ret == wxID_CANCEL )
        {
            return false;
        }
        else if( ret == wxID_OK )
        {
            footprintName = dlg.GetFPName();
            libraryName = dlg.GetTextSelection();
            done = true;
        }
        else if( ret == ID_MAKE_NEW_LIBRARY )
        {
            wxFileName newLibrary( CreateNewLibrary() );
            libraryName = newLibrary.GetName();
        }
    }

    aFootprint->SetFPID( LIB_ID( libraryName, footprintName ) );

    if( updateValue )
        aFootprint->SetValue( footprintName );

    if( !SaveFootprintInLibrary( aFootprint, libraryName ) )
        return false;

    // Once saved-as a board footprint is no longer a board footprint
    aFootprint->SetLink( niluuid );

    wxString fmt = footprintExists ? _( "Footprint '%s' replaced in '%s'" )
                                   : _( "Footprint '%s' added to '%s'" );

    wxString msg = wxString::Format( fmt, footprintName.GetData(), libraryName.GetData() );
    SetStatusText( msg );
    UpdateTitle();
    ReCreateHToolbar();

    return true;
}


bool FOOTPRINT_EDIT_FRAME::RevertFootprint()
{
    if( GetScreen()->IsContentModified() && m_originalFootprintCopy )
    {
        wxString msg = wxString::Format( _( "Revert '%s' to last version saved?" ),
                                         GetLoadedFPID().GetLibItemName().wx_str() );

        if( ConfirmRevertDialog( this, msg ) )
        {
            Clear_Pcb( false );
            AddFootprintToBoard( static_cast<FOOTPRINT*>( m_originalFootprintCopy->Clone() ) );

            Zoom_Automatique( false );

            Update3DView( true, true );

            ClearUndoRedoList();
            GetScreen()->SetContentModified( false );

            UpdateView();
            GetCanvas()->Refresh();

            return true;
        }
    }

    return false;
}


class NEW_FP_DIALOG : public WX_TEXT_ENTRY_DIALOG
{
public:
    NEW_FP_DIALOG( PCB_BASE_FRAME* aParent, const wxString& aName, int aFootprintType,
                   std::function<bool( wxString newName )> aValidator ) :
            WX_TEXT_ENTRY_DIALOG( aParent, _( "Enter footprint name:" ), _( "New Footprint" ),
                                  aName, _( "Footprint type:" ),
                                  { _( "Through hole" ), _( "SMD" ), _( "Other" ) },
                                  aFootprintType ),
            m_validator( std::move( aValidator ) )
    { }

    wxString GetFPName()
    {
        wxString name = m_textCtrl->GetValue();
        name.Trim( true ).Trim( false );
        return name;
    }

protected:
    bool TransferDataFromWindow() override
    {
        return m_validator( GetFPName() );
    }

private:
    std::function<bool( wxString newName )> m_validator;
};


FOOTPRINT* PCB_BASE_FRAME::CreateNewFootprint( const wxString& aFootprintName,
                                               const wxString& aLibName, bool aQuiet )
{
    FP_LIB_TABLE* tbl = PROJECT_PCB::PcbFootprintLibs( &Prj() );
    wxString      footprintName = aFootprintName;
    wxString      msg;

    // Static to store user preference for a session
    static int footprintType = 1;
    int footprintTranslated = FP_SMD;

    // Ask for the new footprint name
    if( footprintName.IsEmpty() && !aQuiet )
    {
        NEW_FP_DIALOG dlg( this, footprintName, footprintType,
                [&]( wxString newName )
                {
                    if( newName.IsEmpty() )
                    {
                        wxMessageBox( _( "Footprint must have a name." ) );
                        return false;
                    }

                    if( !aLibName.IsEmpty() && tbl->FootprintExists( aLibName, newName ) )
                    {
                        msg = wxString::Format( _( "Footprint '%s' already exists in library '%s'." ),
                                                newName, aLibName );

                        KIDIALOG errorDlg( this, msg, _( "Confirmation" ),
                                           wxOK | wxCANCEL | wxICON_WARNING );
                        errorDlg.SetOKLabel( _( "Overwrite" ) );

                        return errorDlg.ShowModal() == wxID_OK;
                    }

                    return true;
                } );

        dlg.SetTextValidator( FOOTPRINT_NAME_VALIDATOR( &footprintName ) );

        if( dlg.ShowModal() != wxID_OK )
            return nullptr;    //Aborted by user

        footprintName = dlg.GetFPName();
        footprintType = dlg.GetChoice();

        switch( footprintType )
        {
        case 0:  footprintTranslated = FP_THROUGH_HOLE; break;
        case 1:  footprintTranslated = FP_SMD;          break;
        default: footprintTranslated = 0;               break;
        }
    }

    // Creates the new footprint and add it to the head of the linked list of footprints
    FOOTPRINT* footprint = new FOOTPRINT( GetBoard() );

    // Update its name in lib
    footprint->SetFPID( LIB_ID( wxEmptyString, footprintName ) );

    footprint->SetAttributes( footprintTranslated );

    PCB_LAYER_ID txt_layer;
    VECTOR2I     default_pos;
    BOARD_DESIGN_SETTINGS& settings = GetDesignSettings();

    footprint->Reference().SetText( settings.m_DefaultFPTextItems[0].m_Text );
    footprint->Reference().SetVisible( settings.m_DefaultFPTextItems[0].m_Visible );
    txt_layer = (PCB_LAYER_ID) settings.m_DefaultFPTextItems[0].m_Layer;
    footprint->Reference().SetLayer( txt_layer );
    default_pos.y -= settings.GetTextSize( txt_layer ).y / 2;
    footprint->Reference().SetPosition( default_pos );
    default_pos.y += settings.GetTextSize( txt_layer ).y;

    footprint->Value().SetText( settings.m_DefaultFPTextItems[1].m_Text );
    footprint->Value().SetVisible( settings.m_DefaultFPTextItems[1].m_Visible );
    txt_layer = (PCB_LAYER_ID) settings.m_DefaultFPTextItems[1].m_Layer;
    footprint->Value().SetLayer( txt_layer );
    default_pos.y += settings.GetTextSize( txt_layer ).y / 2;
    footprint->Value().SetPosition( default_pos );
    default_pos.y += settings.GetTextSize( txt_layer ).y;

    for( size_t i = 2; i < settings.m_DefaultFPTextItems.size(); ++i )
    {
        PCB_TEXT* textItem = new PCB_TEXT( footprint );
        textItem->SetText( settings.m_DefaultFPTextItems[i].m_Text );
        textItem->SetVisible( settings.m_DefaultFPTextItems[i].m_Visible );
        txt_layer = (PCB_LAYER_ID) settings.m_DefaultFPTextItems[i].m_Layer;
        textItem->SetLayer( txt_layer );
        default_pos.y += settings.GetTextSize( txt_layer ).y / 2;
        textItem->SetPosition( default_pos );
        default_pos.y += settings.GetTextSize( txt_layer ).y;
        footprint->GraphicalItems().push_back( textItem );
    }

    if( footprint->GetReference().IsEmpty() )
        footprint->SetReference( footprintName );

    if( footprint->GetValue().IsEmpty() )
        footprint->SetValue( footprintName );

    footprint->RunOnDescendants(
            [&]( BOARD_ITEM* aChild )
            {
                if( aChild->Type() == PCB_FIELD_T || aChild->Type() == PCB_TEXT_T )
                {
                    PCB_TEXT*    textItem = static_cast<PCB_TEXT*>( aChild );
                    PCB_LAYER_ID layer = textItem->GetLayer();

                    textItem->SetTextThickness( settings.GetTextThickness( layer ) );
                    textItem->SetTextSize( settings.GetTextSize( layer ) );
                    textItem->SetItalic( settings.GetTextItalic( layer ) );
                    textItem->SetKeepUpright( settings.GetTextUpright( layer ) );
                }
            } );

    SetMsgPanel( footprint );
    return footprint;
}


wxString PCB_BASE_FRAME::SelectLibrary( const wxString& aNicknameExisting )
{
    wxArrayString headers;

    headers.Add( _( "Nickname" ) );
    headers.Add( _( "Description" ) );

    COMMON_SETTINGS*             cfg = Pgm().GetCommonSettings();
    PROJECT_FILE&                project = Kiway().Prj().GetProjectFile();
    FP_LIB_TABLE*                fptbl = PROJECT_PCB::PcbFootprintLibs( &Prj() );
    std::vector< wxArrayString > itemsToDisplay;
    std::vector< wxString >      nicknames = fptbl->GetLogicalLibs();

    for( const wxString& nickname : nicknames )
    {
        if( alg::contains( project.m_PinnedFootprintLibs, nickname )
                || alg::contains( cfg->m_Session.pinned_fp_libs, nickname ) )
        {
            wxArrayString item;

            item.Add( LIB_TREE_MODEL_ADAPTER::GetPinningSymbol() + nickname );
            item.Add( fptbl->GetDescription( nickname ) );
            itemsToDisplay.push_back( item );
        }
    }

    for( const wxString& nickname : nicknames )
    {
        if( !alg::contains( project.m_PinnedFootprintLibs, nickname )
                && !alg::contains( cfg->m_Session.pinned_fp_libs, nickname ) )
        {
            wxArrayString item;

            item.Add( nickname );
            item.Add( fptbl->GetDescription( nickname ) );
            itemsToDisplay.push_back( item );
        }
    }

    EDA_LIST_DIALOG dlg( this, _( "Select Library" ), headers, itemsToDisplay, aNicknameExisting,
                         false );

    if( dlg.ShowModal() != wxID_OK )
        return wxEmptyString;

    return dlg.GetTextSelection();
}
