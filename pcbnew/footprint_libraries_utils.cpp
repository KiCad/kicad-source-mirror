/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 1992-2021 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <board.h>
#include <footprint.h>
#include <board_commit.h>
#include <footprint_edit_frame.h>
#include <wildcards_and_files_ext.h>
#include <plugins/kicad/kicad_plugin.h>
#include <plugins/legacy/legacy_plugin.h>
#include <env_paths.h>
#include <paths.h>
#include <settings/settings_manager.h>
#include <footprint_editor_settings.h>
#include "footprint_viewer_frame.h"
#include <wx/choicdlg.h>
#include <wx/filedlg.h>


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


/**
 * Prompt the user for a footprint file to open.
 * @param aParent - parent window for the dialog
 * @param aLastPath - last opened path
 */
static wxFileName getFootprintFilenameFromUser( wxWindow* aParent, const wxString& aLastPath )
{
    static int lastFilterIndex = 0;     // To store the last choice during a session.
    wxString wildCard;

    wildCard << KiCadFootprintLibFileWildcard() << wxChar( '|' )
             << ModLegacyExportFileWildcard() << wxChar( '|' )
             << GedaPcbFootprintLibFileWildcard() << wxChar( '|' )
             << AllFilesWildcard();

    wxFileDialog dlg( aParent, _( "Import Footprint" ), aLastPath, wxEmptyString, wildCard,
            wxFD_OPEN | wxFD_FILE_MUST_EXIST );

    dlg.SetFilterIndex( lastFilterIndex );

    if( dlg.ShowModal() == wxID_CANCEL )
        return wxFileName();

    lastFilterIndex = dlg.GetFilterIndex();

    return wxFileName( dlg.GetPath() );
}


/**
 * Read a file to detect the type.
 * @param aFile - open file to be read. File pointer will be closed.
 * @param aFileName - file name to be read
 * @param aName - wxString to receive the footprint name iff type is LEGACY
 */
static IO_MGR::PCB_FILE_T detect_file_type( FILE* aFile, const wxFileName& aFileName,
                                            wxString* aName )
{
    FILE_LINE_READER freader( aFile, aFileName.GetFullPath() );
    WHITESPACE_FILTER_READER reader( freader );
    IO_MGR::PCB_FILE_T file_type;

    wxASSERT( aName );

    reader.ReadLine();
    char* line = reader.Line();

    // first .kicad_mod file versions starts by "(module"
    // recent .kicad_mod file versions starts by "(footprint"
    if( strncasecmp( line, "(module", strlen( "(module" ) ) == 0
        || strncasecmp( line, "(footprint", strlen( "(footprint" ) ) == 0 )
    {
        file_type = IO_MGR::KICAD_SEXP;
        *aName = aFileName.GetName();
    }
    else if( !strncasecmp( line, FOOTPRINT_LIBRARY_HEADER, FOOTPRINT_LIBRARY_HEADER_CNT ) )
    {
        file_type = IO_MGR::LEGACY;

        while( reader.ReadLine() )
        {
            if( !strncasecmp( line, "$MODULE", strlen( "$MODULE" ) ) )
            {
                *aName = FROM_UTF8( StrPurge( line + strlen( "$MODULE" ) ) );
                break;
            }
        }
    }
    else if( !strncasecmp( line, "Element", strlen( "Element" ) ) )
    {
        file_type = IO_MGR::GEDA_PCB;
        *aName = aFileName.GetName();
    }
    else
    {
        file_type = IO_MGR::FILE_TYPE_NONE;
    }

    return file_type;
}


/**
 * Parse a footprint using a PLUGIN.
 * @param aFileName - file name to parse
 * @param aFileType - type of the file
 * @param aName - name of the footprint
 */
static FOOTPRINT* parse_footprint_with_plugin( const wxFileName& aFileName,
                                               IO_MGR::PCB_FILE_T aFileType,
                                               const wxString& aName )
{
    wxString path;

    switch( aFileType )
    {
    case IO_MGR::GEDA_PCB: path = aFileName.GetPath();             break;
    case IO_MGR::LEGACY:   path = aFileName.GetFullPath();         break;
    default: wxFAIL_MSG( wxT( "unexpected IO_MGR::PCB_FILE_T" ) ); break;
    }

    PLUGIN::RELEASER pi( IO_MGR::PluginFind( aFileType ) );

    return pi->FootprintLoad( path, aName );
}


/**
 * Parse a KICAD footprint.
 * @param aFileName - file name to parse
 */
static FOOTPRINT* parse_footprint_kicad( const wxFileName& aFileName )
{
    wxString fcontents;
    PCB_IO   pcb_io;
    wxFFile  f( aFileName.GetFullPath() );

    if( !f.IsOpened() )
        return nullptr;

    f.ReadAll( &fcontents );

    return dynamic_cast<FOOTPRINT*>( pcb_io.Parse( fcontents ) );
}


/**
 * Try to load a footprint, returning nullptr if the file couldn't be accessed.
 * @param aFileName - file name to load
 * @param aFileType - type of the file to load
 * @param aName - footprint name
 */
FOOTPRINT* try_load_footprint( const wxFileName& aFileName, IO_MGR::PCB_FILE_T aFileType,
                               const wxString& aName )
{
    FOOTPRINT* footprint;

    switch( aFileType )
    {
    case IO_MGR::GEDA_PCB:
    case IO_MGR::LEGACY:
        footprint = parse_footprint_with_plugin( aFileName, aFileType, aName );
        break;

    case IO_MGR::KICAD_SEXP:
        footprint = parse_footprint_kicad( aFileName );
        break;

    default:
        wxFAIL_MSG( wxT( "unexpected IO_MGR::PCB_FILE_T" ) );
        footprint = nullptr;
    }

    return footprint;
}


FOOTPRINT* FOOTPRINT_EDIT_FRAME::ImportFootprint( const wxString& aName )
{
    wxString lastOpenedPathForLoading = m_mruPath;
    FOOTPRINT_EDITOR_SETTINGS* cfg    = GetSettings();

    if( !cfg->m_LastImportExportPath.empty() )
        lastOpenedPathForLoading = cfg->m_LastImportExportPath;

    wxFileName fn;

    if( aName != wxT("") )
        fn = aName;
    else
        fn = getFootprintFilenameFromUser( this, lastOpenedPathForLoading );

    if( !fn.IsOk() )
        return nullptr;

    FILE* fp = wxFopen( fn.GetFullPath(), wxT( "rt" ) );

    if( !fp )
    {
        wxString msg = wxString::Format( _( "File '%s' not found." ), fn.GetFullPath() );
        DisplayError( this, msg );
        return nullptr;
    }

    cfg->m_LastImportExportPath = lastOpenedPathForLoading;

    wxString footprintName;
    IO_MGR::PCB_FILE_T fileType = detect_file_type( fp, fn.GetFullPath(), &footprintName );

    if( fileType == IO_MGR::FILE_TYPE_NONE )
    {
        DisplayError( this, _( "Not a footprint file." ) );
        return nullptr;
    }

    FOOTPRINT* footprint = nullptr;

    try
    {
        footprint = try_load_footprint( fn, fileType, footprintName );

        if( !footprint )
        {
            wxString msg = wxString::Format( _( "Unable to load footprint '%s' from '%s'" ),
                                             footprintName,
                                             fn.GetFullPath() );
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

    footprint->SetPosition( wxPoint( 0, 0 ) );

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

    wxString    wildcard = KiCadFootprintLibFileWildcard();

    fn.SetExt( KiCadFootprintFileExtension );

    if( !cfg->m_LastImportExportPath.empty() )
        fn.SetPath( cfg->m_LastImportExportPath );
    else
        fn.SetPath( m_mruPath );

    wxFileDialog dlg( this, _( "Export Footprint" ), fn.GetPath(), fn.GetFullName(),
                      wildcard, wxFD_SAVE | wxFD_OVERWRITE_PROMPT );

    if( dlg.ShowModal() == wxID_CANCEL )
        return;

    fn = dlg.GetPath();
    cfg->m_LastImportExportPath = fn.GetPath();

    try
    {
        // Export as *.kicad_pcb format, using a strategy which is specifically chosen
        // as an example on how it could also be used to send it to the system clipboard.

        PCB_IO  pcb_io( CTL_FOR_LIBRARY );

        /*  This footprint should *already* be "normalized" in a way such that
            orientation is zero, etc., since it came from the Footprint Editor.

            aFootprint->SetParent( 0 );
            aFootprint->SetOrientation( 0 );
        */

        pcb_io.Format( aFootprint );

        FILE* fp = wxFopen( dlg.GetPath(), wxT( "wt" ) );

        if( fp == nullptr )
        {
            wxMessageBox( wxString::Format( _( "Insufficient permissions to write file '%s'." ),
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
    return createNewLibrary( aLibName, aProposedName, Prj().PcbFootprintLibs() );
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

        if( !LibraryFileBrowser( false, fn, KiCadFootprintLibPathWildcard(),
                                 KiCadFootprintLibPathExtension, false, isGlobal,
                                 PATHS::GetDefaultUserFootprintsPath() ) )
        {
            return wxEmptyString;
        }

        doAdd = true;
    }
    else
    {
        fn = aLibName;

        if( !fn.IsAbsolute() )
        {
            fn.SetName( aLibName );
            fn.MakeAbsolute( initialPath );
        }

        // Enforce the .pretty extension:
        fn.SetExt( KiCadFootprintLibPathExtension );
    }

    // We can save fp libs only using IO_MGR::KICAD_SEXP format (.pretty libraries)
    IO_MGR::PCB_FILE_T piType  = IO_MGR::KICAD_SEXP;
    wxString           libPath = fn.GetFullPath();

    try
    {
        PLUGIN::RELEASER pi( IO_MGR::PluginFind( piType ) );

        bool writable = false;
        bool exists   = false;

        try
        {
            writable = pi->IsFootprintLibWritable( libPath );
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

                pi->FootprintLibDelete( libPath );
            }
        }

        pi->FootprintLibCreate( libPath );
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
    case 1: return Prj().PcbFootprintLibs();
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
        if( !LibraryFileBrowser( true, fn, KiCadFootprintLibPathWildcard(),
                                 KiCadFootprintLibPathExtension, true, isGlobal,
                                 PATHS::GetDefaultUserFootprintsPath() ) )
        {
            return false;
        }
    }

    wxString libPath = fn.GetFullPath();
    wxString libName = fn.GetName();

    if( libName.IsEmpty() )
        return false;

    wxString type = IO_MGR::ShowType( IO_MGR::GuessPluginTypeFromLibPath( libPath ) );

    // try to use path normalized to an environmental variable or project path
    wxString normalizedPath = NormalizePath( libPath, &Pgm().GetLocalEnvVariables(), &Prj() );

    try
    {
        FP_LIB_TABLE_ROW* row = new FP_LIB_TABLE_ROW( libName, normalizedPath, type, wxEmptyString );
        aTable->InsertRow( row );

        if( isGlobal )
            GFootprintTable.Save( FP_LIB_TABLE::GetGlobalTableFileName() );
        else
            Prj().PcbFootprintLibs()->Save( Prj().FootprintLibTblName() );
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
    wxString libfullname = Prj().PcbFootprintLibs()->FindRow( nickname )->GetFullURI();

    if( IO_MGR::GuessPluginTypeFromLibPath( libfullname ) == IO_MGR::LEGACY )
    {
        DisplayInfoMessage( this, INFO_LEGACY_LIB_WARN_DELETE );
        return false;
    }

    if( !Prj().PcbFootprintLibs()->IsFootprintLibWritable( nickname ) )
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
        Prj().PcbFootprintLibs()->FootprintDelete( nickname, fpname );
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
                FP_LIB_TABLE* tbl = prj.PcbFootprintLibs();

                if( !footprint->GetFPID().GetLibItemName().empty() )    // Handle old boards.
                {
                    FOOTPRINT* fpCopy = static_cast<FOOTPRINT*>( footprint->Duplicate() );

                    resetReference( fpCopy );
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
            const LIB_TABLE_ROW* row = Prj().PcbFootprintLibs()->FindRowByURI( libPath );

            if( row )
                libNickname = row->GetNickName();
        }

        IO_MGR::PCB_FILE_T piType = IO_MGR::KICAD_SEXP;
        PLUGIN::RELEASER   pi( IO_MGR::PluginFind( piType ) );

        for( FOOTPRINT* footprint : GetBoard()->Footprints() )
        {
            try
            {
                if( !footprint->GetFPID().GetLibItemName().empty() )    // Handle old boards.
                {
                    FOOTPRINT* fpCopy = static_cast<FOOTPRINT*>( footprint->Duplicate() );

                    resetReference( fpCopy );
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

    FP_LIB_TABLE* tbl = Prj().PcbFootprintLibs();

    // Legacy libraries are readable, but modifying legacy format is not allowed
    // So prompt the user if he try to add/replace a footprint in a legacy lib
    wxString libfullname = tbl->FindRow( libraryName )->GetFullURI();

    if( IO_MGR::GuessPluginTypeFromLibPath( libfullname ) == IO_MGR::LEGACY )
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


bool FOOTPRINT_EDIT_FRAME::SaveFootprintInLibrary( FOOTPRINT* aFootprint,
                                                   const wxString& aLibraryName )
{
    try
    {
        aFootprint->SetFPID( LIB_ID( wxEmptyString, aFootprint->GetFPID().GetLibItemName() ) );

        Prj().PcbFootprintLibs()->FootprintSave( aLibraryName, aFootprint );

        aFootprint->SetFPID( LIB_ID( aLibraryName, aFootprint->GetFPID().GetLibItemName() ) );
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
    TOOL_MANAGER*   toolMgr = pcbframe->GetToolManager();

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

    if( aAddNew && toolMgr->GetTool<BOARD_EDITOR_CONTROL>()->PlacingFootprint() )
    {
        DisplayError( this, _( "Previous footprint placement still in progress." ) );
        return false;
    }

    m_toolManager->RunAction( PCB_ACTIONS::selectionClear, true );
    toolMgr->RunAction( PCB_ACTIONS::selectionClear, true );
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
    newFootprint->RunOnChildren(
            [&]( BOARD_ITEM* aChild )
            {
                fixUuid( const_cast<KIID&>( aChild->m_Uuid ) );
            } );

    if( sourceFootprint )         // this is an update command
    {
        // In the main board the new footprint replaces the old one (pos, orient, ref, value,
        // connections and properties are kept) and the sourceFootprint (old footprint) is
        // deleted
        pcbframe->ExchangeFootprint( sourceFootprint, newFootprint, commit );
        commit.Push( wxT( "Update footprint" ) );
    }
    else        // This is an insert command
    {
        KIGFX::VIEW_CONTROLS* viewControls = pcbframe->GetCanvas()->GetViewControls();
        VECTOR2D              cursorPos = viewControls->GetCursorPosition();

        commit.Add( newFootprint );
        viewControls->SetCrossHairCursorPosition( VECTOR2D( 0, 0 ), false );
        pcbframe->PlaceFootprint( newFootprint );
        newFootprint->SetPosition( wxPoint( 0, 0 ) );
        viewControls->SetCrossHairCursorPosition( cursorPos, false );
        const_cast<KIID&>( newFootprint->m_Uuid ) = KIID();
        commit.Push( wxT( "Insert footprint" ) );

        pcbframe->Raise();
        toolMgr->RunAction( PCB_ACTIONS::placeFootprint, true, newFootprint );
    }

    newFootprint->ClearFlags();

    return true;
}


bool FOOTPRINT_EDIT_FRAME::SaveFootprintAs( FOOTPRINT* aFootprint )
{
    if( aFootprint == nullptr )
        return false;

    FP_LIB_TABLE* tbl = Prj().PcbFootprintLibs();

    SetMsgPanel( aFootprint );

    wxString libraryName = aFootprint->GetFPID().GetLibNickname();
    wxString footprintName = aFootprint->GetFPID().GetLibItemName();
    bool     updateValue = aFootprint->GetValue() == footprintName;

    wxArrayString              headers;
    std::vector<wxArrayString> itemsToDisplay;
    std::vector<wxString>      nicknames = tbl->GetLogicalLibs();

    headers.Add( _( "Nickname" ) );
    headers.Add( _( "Description" ) );

    for( const wxString& nickname : nicknames )
    {
        wxArrayString item;
        item.Add( nickname );
        item.Add( tbl->GetDescription( nickname ) );
        itemsToDisplay.push_back( item );
    }

    EDA_LIST_DIALOG dlg( this, _( "Save Footprint As" ), headers, itemsToDisplay, libraryName );
    dlg.SetListLabel( _( "Save in library:" ) );
    dlg.SetOKLabel( _( "Save" ) );

    wxBoxSizer* bNameSizer = new wxBoxSizer( wxHORIZONTAL );

    wxStaticText* label = new wxStaticText( &dlg, wxID_ANY, _( "Name:" ),
                                            wxDefaultPosition, wxDefaultSize, 0 );
    bNameSizer->Add( label, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM|wxLEFT, 5 );

    wxTextCtrl* nameTextCtrl = new wxTextCtrl( &dlg, wxID_ANY, footprintName,
                                               wxDefaultPosition, wxDefaultSize, 0 );
    bNameSizer->Add( nameTextCtrl, 1, wxALIGN_CENTER_VERTICAL|wxALL, 5 );

    wxTextValidator nameValidator( wxFILTER_EXCLUDE_CHAR_LIST );
    nameValidator.SetCharExcludes( FOOTPRINT::StringLibNameInvalidChars( false ) );
    nameTextCtrl->SetValidator( nameValidator );

    wxSizer* mainSizer = dlg.GetSizer();
    mainSizer->Prepend( bNameSizer, 0, wxEXPAND|wxTOP|wxLEFT|wxRIGHT, 5 );

    // Move nameTextCtrl to the head of the tab-order
    if( dlg.GetChildren().DeleteObject( nameTextCtrl ) )
        dlg.GetChildren().Insert( nameTextCtrl );

    dlg.SetInitialFocus( nameTextCtrl );

    dlg.Layout();
    mainSizer->Fit( &dlg );

    if( dlg.ShowModal() != wxID_OK )
        return false;                   // canceled by user

    libraryName = dlg.GetTextSelection();

    if( libraryName.IsEmpty() )
    {
        DisplayError( this, _( "No library specified.  Footprint could not be saved." ) );
        return false;
    }

    footprintName = nameTextCtrl->GetValue();
    footprintName.Trim( true );
    footprintName.Trim( false );

    if( footprintName.IsEmpty() )
    {
        DisplayError( this, _( "No footprint name specified.  Footprint could not be saved." ) );
        return false;
    }

    aFootprint->SetFPID( LIB_ID( libraryName, footprintName ) );

    if( updateValue )
        aFootprint->SetValue( footprintName );

    // Legacy libraries are readable, but modifying legacy format is not allowed
    // So prompt the user if he try to add/replace a footprint in a legacy lib
    wxString    libfullname = Prj().PcbFootprintLibs()->FindRow( libraryName )->GetFullURI();
    IO_MGR::PCB_FILE_T  piType = IO_MGR::GuessPluginTypeFromLibPath( libfullname );

    if( piType == IO_MGR::LEGACY )
    {
        DisplayInfoMessage( this, INFO_LEGACY_LIB_WARN_EDIT );
        return false;
    }

    bool footprintExists = tbl->FootprintExists( libraryName, footprintName );

    if( footprintExists )
    {
        wxString msg = wxString::Format( _( "Footprint %s already exists in %s." ),
                                         footprintName,
                                         libraryName );
        KIDIALOG chkdlg( this, msg, _( "Confirmation" ), wxOK | wxCANCEL | wxICON_WARNING );
        chkdlg.SetOKLabel( _( "Overwrite" ) );

        if( chkdlg.ShowModal() == wxID_CANCEL )
            return false;
    }

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
    if( GetScreen()->IsContentModified() && m_revertModule )
    {
        wxString msg = wxString::Format( _( "Revert '%s' to last version saved?" ),
                                         GetLoadedFPID().GetLibItemName().wx_str() );

        if( ConfirmRevertDialog( this, msg ) )
        {
            Clear_Pcb( false );
            AddFootprintToBoard( static_cast<FOOTPRINT*>( m_revertModule->Clone() ) );

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


FOOTPRINT* PCB_BASE_FRAME::CreateNewFootprint( const wxString& aFootprintName, bool aQuiet )
{
    wxString footprintName = aFootprintName;

    // Static to store user preference for a session
    static int footprintType = 1;
    int footprintTranslated = FP_SMD;

    // Ask for the new footprint name
    if( footprintName.IsEmpty() && !aQuiet )
    {
        WX_TEXT_ENTRY_DIALOG dlg( this, _( "Enter footprint name:" ), _( "New Footprint" ),
                                  footprintName, _( "Footprint type:" ),
                                  { _( "Through hole" ), _( "SMD" ), _( "Other" ) }, footprintType );
        dlg.SetTextValidator( FOOTPRINT_NAME_VALIDATOR( &footprintName ) );

        if( dlg.ShowModal() != wxID_OK )
            return nullptr;    //Aborted by user

        footprintType = dlg.GetChoice();

        switch( footprintType )
        {
        case 0:
            footprintTranslated = FP_THROUGH_HOLE;
            break;
        case 1:
            footprintTranslated = FP_SMD;
            break;
        default:
            footprintTranslated = 0;
        }
    }

    footprintName.Trim( true );
    footprintName.Trim( false );

    if( footprintName.IsEmpty() )
    {
        if( !aQuiet )
            DisplayInfoMessage( this, _( "No footprint name defined." ) );

        return nullptr;
    }

    // Creates the new footprint and add it to the head of the linked list of footprints
    FOOTPRINT* footprint = new FOOTPRINT( GetBoard() );

    // Update parameters: timestamp ...
    footprint->SetLastEditTime();

    // Update its name in lib
    footprint->SetFPID( LIB_ID( wxEmptyString, footprintName ) );

    footprint->SetAttributes( footprintTranslated );

    PCB_LAYER_ID txt_layer;
    wxPoint default_pos;
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
        FP_TEXT* textItem = new FP_TEXT( footprint );
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

    footprint->RunOnChildren(
            [&] ( BOARD_ITEM* aChild )
            {
                if( aChild->Type() == PCB_FP_TEXT_T )
                {
                    FP_TEXT*     textItem = static_cast<FP_TEXT*>( aChild );
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

    FP_LIB_TABLE*   fptbl = Prj().PcbFootprintLibs();

    std::vector< wxArrayString > itemsToDisplay;
    std::vector< wxString >      nicknames = fptbl->GetLogicalLibs();

    for( const wxString& nickname : nicknames )
    {
        wxArrayString item;

        item.Add( nickname );
        item.Add( fptbl->GetDescription( nickname ) );

        itemsToDisplay.push_back( item );
    }

    EDA_LIST_DIALOG dlg( this, _( "Select Library" ), headers, itemsToDisplay, aNicknameExisting );

    if( dlg.ShowModal() != wxID_OK )
        return wxEmptyString;

    return dlg.GetTextSelection();
}
