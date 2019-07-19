/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 1992-2018 KiCad Developers, see AUTHORS.txt for contributors.
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
 * @file librairi.cpp
 * @brief Manage module (footprint) libraries.
 */

#include <wx/ffile.h>
#include <wx/stdpaths.h>
#include <fctsys.h>
#include <pgm_base.h>
#include <kiface_i.h>
#include <confirm.h>
#include <kicad_string.h>
#include <gestfich.h>
#include <pcb_edit_frame.h>
#include <dialog_helpers.h>
#include <filter_reader.h>
#include <macros.h>
#include <fp_lib_table.h>
#include <validators.h>
#include <dialog_text_entry.h>
#include <tool/tool_manager.h>
#include <tools/pcb_actions.h>
#include <class_board.h>
#include <class_module.h>
#include <board_commit.h>
#include <pcbnew.h>
#include <footprint_edit_frame.h>
#include <wildcards_and_files_ext.h>
#include <kicad_plugin.h>
#include <legacy_plugin.h>
#include <env_paths.h>
#include "footprint_viewer_frame.h"


// unique, "file local" translations:

#define FMT_OK_DELETE       _( "OK to delete footprint \"%s\" in library \"%s\"" )
#define FMT_IMPORT_MODULE   _( "Import Footprint" )
#define FMT_FILE_NOT_FOUND  _( "File \"%s\" not found" )
#define FMT_NOT_MODULE      _( "Not a footprint file" )
#define FMT_MOD_NOT_FOUND   _( "Unable to find or load footprint \"%s\" from lib path \"%s\"" )
#define FMT_LIB_READ_ONLY   _( "Library \"%s\" is read only, not writable" )

#define FMT_EXPORT_MODULE   _( "Export Footprint" )
#define FMT_SAVE_MODULE     _( "Save Footprint" )
#define FMT_MOD_REF         _( "Enter footprint name:" )
#define FMT_EXPORTED        _( "Footprint exported to file \"%s\"" )
#define FMT_MOD_DELETED     _( "Footprint \"%s\" deleted from library \"%s\"" )
#define FMT_MOD_CREATE      _( "New Footprint" )

#define FMT_NO_REF_ABORTED  _( "No footprint name defined." )
#define FMT_SELECT_LIB      _( "Select Library" )

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


#define EXPORT_IMPORT_LASTPATH_KEY wxT( "import_last_path" )


/**
 * Prompt the user for a module file to open.
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

    wxFileDialog dlg( aParent, FMT_IMPORT_MODULE, aLastPath, wxEmptyString, wildCard,
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
 * @param aName - wxString to receive the module name iff type is LEGACY
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

    if( !strncasecmp( line, "(module", strlen( "(module" ) ) )
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
static MODULE* parse_module_with_plugin(
        const wxFileName& aFileName, IO_MGR::PCB_FILE_T aFileType,
        const wxString& aName )
{
    wxString path;

    switch( aFileType )
    {
    case IO_MGR::GEDA_PCB:
        path = aFileName.GetPath();
        break;
    case IO_MGR::LEGACY:
        path = aFileName.GetFullPath();
        break;
    default:
        wxFAIL_MSG( wxT( "unexpected IO_MGR::PCB_FILE_T" ) );
    }

    PLUGIN::RELEASER pi( IO_MGR::PluginFind( aFileType ) );

    return pi->FootprintLoad( path, aName );
}


/**
 * Parse a KICAD footprint.
 * @param aFileName - file name to parse
 */
static MODULE* parse_module_kicad( const wxFileName& aFileName )
{
    wxString fcontents;
    PCB_IO   pcb_io;
    wxFFile  f( aFileName.GetFullPath() );

    if( !f.IsOpened() )
        return NULL;

    f.ReadAll( &fcontents );

    return dynamic_cast<MODULE*>( pcb_io.Parse( fcontents ) );
}


/**
 * Try to load a footprint, returning NULL if the file couldn't be accessed.
 * @param aFileName - file name to load
 * @param aFileType - type of the file to load
 * @param aName - footprint name
 */
MODULE* try_load_footprint( const wxFileName& aFileName, IO_MGR::PCB_FILE_T aFileType,
        const wxString& aName )
{
    MODULE* module;

    switch( aFileType )
    {
    case IO_MGR::GEDA_PCB:
    case IO_MGR::LEGACY:
        module = parse_module_with_plugin( aFileName, aFileType, aName );
        break;

    case IO_MGR::KICAD_SEXP:
        module = parse_module_kicad( aFileName );
        break;

    default:
        wxFAIL_MSG( wxT( "unexpected IO_MGR::PCB_FILE_T" ) );
        module = NULL;
    }

    return module;
}


MODULE* FOOTPRINT_EDIT_FRAME::Import_Module( const wxString& aName )
{
    wxString        lastOpenedPathForLoading = m_mruPath;
    wxConfigBase*   cfg = Kiface().KifaceSettings();

    if( cfg )
        cfg->Read( EXPORT_IMPORT_LASTPATH_KEY, &lastOpenedPathForLoading );

    wxFileName fn;

    if( aName != wxT("") )
        fn = aName;
    else
        fn = getFootprintFilenameFromUser( this, lastOpenedPathForLoading );

    if( !fn.IsOk() )
        return NULL;

    FILE* fp = wxFopen( fn.GetFullPath(), wxT( "rt" ) );

    if( !fp )
    {
        wxString msg = wxString::Format( FMT_FILE_NOT_FOUND, GetChars( fn.GetFullPath() ) );
        DisplayError( this, msg );
        return NULL;
    }

    if( cfg )    // Save file path
    {
        lastOpenedPathForLoading = fn.GetPath();
        cfg->Write( EXPORT_IMPORT_LASTPATH_KEY, lastOpenedPathForLoading );
    }

    wxString    moduleName;
    IO_MGR::PCB_FILE_T fileType = detect_file_type( fp, fn.GetFullPath(), &moduleName );

    if( fileType == IO_MGR::FILE_TYPE_NONE )
    {
        DisplayError( this, FMT_NOT_MODULE );
        return NULL;
    }

    MODULE*    module = NULL;

    try
    {
        module = try_load_footprint( fn, fileType, moduleName );

        if( !module )
        {
            wxString msg = wxString::Format(
                    FMT_MOD_NOT_FOUND, GetChars( moduleName ), GetChars( fn.GetFullPath() ) );
            DisplayError( this, msg );
            return NULL;
        }
    }
    catch( const IO_ERROR& ioe )
    {
        DisplayError( this, ioe.What() );

        // if the footprint is not loaded, exit.
        // However, even if an error happens, it can be loaded, because in KICAD and GPCB format,
        // a fp library is a set of separate files, and the error(s) are not necessary when
        // reading the selected file

        if( !module )
            return NULL;
    }

    module->SetFPID( LIB_ID( wxEmptyString, moduleName ) );

    // Insert footprint in list
    AddModuleToBoard( module );

    // Display info :
    SetMsgPanel( module );
    PlaceModule( module );

    module->SetPosition( wxPoint( 0, 0 ) );

    GetBoard()->BuildListOfNets();
    updateView();

    return module;
}


void FOOTPRINT_EDIT_FRAME::Export_Module( MODULE* aModule )
{
    wxFileName      fn;
    wxConfigBase*   cfg = Kiface().KifaceSettings();

    if( !aModule )
        return;

    fn.SetName( aModule->GetFPID().GetLibItemName() );

    wxString    wildcard = KiCadFootprintLibFileWildcard();

    fn.SetExt( KiCadFootprintFileExtension );

    if( cfg )
    {
        wxString    path;
        cfg->Read( EXPORT_IMPORT_LASTPATH_KEY, &path, m_mruPath );
        fn.SetPath( path );
    }

    wxFileDialog dlg( this, FMT_EXPORT_MODULE, fn.GetPath(), fn.GetFullName(),
                      wildcard, wxFD_SAVE | wxFD_OVERWRITE_PROMPT );

    if( dlg.ShowModal() == wxID_CANCEL )
        return;

    fn = dlg.GetPath();

    if( cfg )  // Save file path
    {
        cfg->Write( EXPORT_IMPORT_LASTPATH_KEY, fn.GetPath() );
    }

    try
    {
        // Export as *.kicad_pcb format, using a strategy which is specifically chosen
        // as an example on how it could also be used to send it to the system clipboard.

        PCB_IO  pcb_io( CTL_FOR_LIBRARY );

        /*  This module should *already* be "normalized" in a way such that
            orientation is zero, etc., since it came from module editor.

            module->SetTimeStamp( 0 );
            module->SetParent( 0 );
            module->SetOrientation( 0 );
        */

        pcb_io.Format( aModule );

        FILE* fp = wxFopen( dlg.GetPath(), wxT( "wt" ) );

        if( fp == NULL )
        {
            wxMessageBox( wxString::Format(
                          _( "Unable to create or write file \"%s\"" ),
                         GetChars( dlg.GetPath() ) ) );
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

    wxString msg = wxString::Format( FMT_EXPORTED, GetChars( dlg.GetPath() ) );
    DisplayInfoMessage( this, msg );
}


wxString PCB_BASE_EDIT_FRAME::CreateNewLibrary(const wxString& aLibName )
{
    // Kicad cannot write legacy format libraries, only .pretty new format
    // because the legacy format cannot handle current features.
    // The footprint library is actually a directory

    wxString initialPath = wxPathOnly( Prj().GetProjectFullName() );
    wxFileName fn;
    bool       doAdd = false;

    if( aLibName.IsEmpty() )
    {
        fn = initialPath;

        if( !LibraryFileBrowser( false, fn,
                                 KiCadFootprintLibPathWildcard(), KiCadFootprintLibPathExtension) )
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
    IO_MGR::PCB_FILE_T  piType = IO_MGR::KICAD_SEXP;
    wxString libPath = fn.GetFullPath();

    try
    {
        PLUGIN::RELEASER  pi( IO_MGR::PluginFind( piType ) );

        bool    writable = false;
        bool    exists   = false;

        try
        {
            writable = pi->IsFootprintLibWritable( libPath );
            exists   = true;    // no exception was thrown, lib must exist.
        }
        catch( const IO_ERROR& )
        { }

        if( exists )
        {
            if( !writable )
            {
                wxString msg = wxString::Format( FMT_LIB_READ_ONLY, libPath );
                DisplayError( this, msg );
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
        AddLibrary( libPath );

    return libPath;
}


bool PCB_BASE_EDIT_FRAME::AddLibrary( const wxString& aFilename )
{
    wxFileName fn( aFilename );

    if( aFilename.IsEmpty() )
    {
        if( !LibraryFileBrowser( true, fn,
                                 KiCadFootprintLibPathWildcard(), KiCadFootprintLibPathExtension,
                                 true ) )
        {
            return false;
        }
    }

    wxString libPath = fn.GetFullPath();
    wxString libName = fn.GetName();

    if( libName.IsEmpty() )
        return false;

    bool          saveInGlobalTable = false;
    bool          saveInProjectTable = false;
    wxArrayString libTableNames;

    libTableNames.Add( _( "Global" ) );
    libTableNames.Add( _( "Project" ) );

    switch( SelectSingleOption( this, _( "Select Library Table" ),
                                _( "Choose the Library Table to add the library to:" ),
                                libTableNames ) )
    {
    case 0:  saveInGlobalTable = true;  break;
    case 1:  saveInProjectTable = true; break;
    default: return false;
    }

    wxString type = IO_MGR::ShowType( IO_MGR::GuessPluginTypeFromLibPath( libPath ) );

    // try to use path normalized to an environmental variable or project path
    wxString normalizedPath = NormalizePath( libPath, &Pgm().GetLocalEnvVariables(), &Prj() );

    if( normalizedPath.IsEmpty() )
        normalizedPath = libPath;

    try
    {
        if( saveInGlobalTable )
        {
            auto row = new FP_LIB_TABLE_ROW( libName, normalizedPath, type, wxEmptyString );
            GFootprintTable.InsertRow( row );
            GFootprintTable.Save( FP_LIB_TABLE::GetGlobalTableFileName() );
        }
        else if( saveInProjectTable )
        {
            auto row = new FP_LIB_TABLE_ROW( libName, normalizedPath, type, wxEmptyString );
            Prj().PcbFootprintLibs()->InsertRow( row );
            Prj().PcbFootprintLibs()->Save( Prj().FootprintLibTblName() );
        }
    }
    catch( const IO_ERROR& ioe )
    {
        DisplayError( this, ioe.What() );
        return false;
    }

    auto editor = (FOOTPRINT_EDIT_FRAME*) Kiway().Player( FRAME_PCB_MODULE_EDITOR, false );

    if( editor )
    {
        LIB_ID libID( libName, wxEmptyString );
        editor->SyncLibraryTree( true );
        editor->FocusOnLibID( libID );
    }

    auto viewer = (FOOTPRINT_VIEWER_FRAME*) Kiway().Player( FRAME_PCB_MODULE_VIEWER, false );

    if( viewer )
        viewer->ReCreateLibraryList();

    return true;
}


bool FOOTPRINT_EDIT_FRAME::DeleteModuleFromLibrary( const LIB_ID& aFPID, bool aConfirm )
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
        wxString msg = wxString::Format( _( "Library \"%s\" is read only" ), nickname );
        DisplayError( this, msg );
        return false;
    }

    // Confirmation
    wxString msg = wxString::Format( FMT_OK_DELETE, fpname.GetData(), nickname.GetData() );

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

    msg.Printf( FMT_MOD_DELETED, fpname.GetData(), nickname.GetData() );

    SetStatusText( msg );

    return true;
}


void PCB_EDIT_FRAME::ArchiveModulesOnBoard( bool aStoreInNewLib, const wxString& aLibName,
                                            wxString* aLibPath )
{
    if( GetBoard()->GetFirstModule() == NULL )
    {
        DisplayInfoMessage( this, _( "No footprints to archive!" ) );
        return;
    }

    wxString footprintName;

    if( !aStoreInNewLib )
    {
        // The footprints are saved in an existing .pretty library in the fp lib table
        PROJECT&        prj = Prj();
        wxString last_nickname = prj.GetRString( PROJECT::PCB_LIB_NICKNAME );
        wxString nickname = SelectLibrary( last_nickname );

        if( !nickname )     // Aborted
            return;

        prj.SetRString( PROJECT::PCB_LIB_NICKNAME, nickname );

        try
        {
            FP_LIB_TABLE* tbl = prj.PcbFootprintLibs();

            for( auto curr_fp : GetBoard()->Modules() )
            {
                if( !curr_fp->GetFPID().GetLibItemName().empty() )   // Can happen with old boards.
                    tbl->FootprintSave( nickname, curr_fp, false );
            }
        }
        catch( const IO_ERROR& ioe )
        {
            DisplayError( this, ioe.What() );
        }
    }
    else
    {
        // The footprints are saved in a new .pretty library.
        // If this library already exists, all previous footprints will be deleted
        wxString libPath = CreateNewLibrary( aLibName );

        if( libPath.IsEmpty() )     // Aborted
            return;


        if( aLibPath ) *aLibPath = libPath;

        IO_MGR::PCB_FILE_T  piType = IO_MGR::KICAD_SEXP;
        PLUGIN::RELEASER  pi( IO_MGR::PluginFind( piType ) );

        for( auto curr_fp : GetBoard()->Modules() )
        {
            try
            {
                if( !curr_fp->GetFPID().GetLibItemName().empty() )   // Can happen with old boards.
                    pi->FootprintSave( libPath, curr_fp );
            }
            catch( const IO_ERROR& ioe )
            {
                DisplayError( this, ioe.What() );
            }
        }
    }
}


bool FOOTPRINT_EDIT_FRAME::SaveFootprint( MODULE* aModule )
{
    if( !aModule )      // Happen if no footprint loaded
        return false;

    wxString libraryName = aModule->GetFPID().GetLibNickname();
    wxString footprintName = aModule->GetFPID().GetLibItemName();
    bool nameChanged = m_footprintNameWhenLoaded != footprintName;

    if( aModule->GetLink() )
    {
        if( SaveFootprintToBoard( false ) )
        {
            m_footprintNameWhenLoaded = footprintName;
            return true;
        }
        else
            return false;
    }
    else if( libraryName.IsEmpty() || footprintName.IsEmpty() )
    {
        if( SaveFootprintAs( aModule ) )
        {
            m_footprintNameWhenLoaded = footprintName;
            SyncLibraryTree( true );
            return true;
        }
        else
            return false;
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
        DeleteModuleFromLibrary( oldFPID, false );
    }

    if( !SaveFootprintInLibrary( aModule, libraryName ) )
        return false;

    if( nameChanged )
    {
        m_footprintNameWhenLoaded = footprintName;
        SyncLibraryTree( true );
    }

    return true;
}


bool FOOTPRINT_EDIT_FRAME::SaveFootprintInLibrary( MODULE* aModule, const wxString& aLibraryName )
{
    try
    {
        aModule->SetFPID( LIB_ID( wxEmptyString, aModule->GetFPID().GetLibItemName() ) );

        Prj().PcbFootprintLibs()->FootprintSave( aLibraryName, aModule );

        aModule->SetFPID( LIB_ID( aLibraryName, aModule->GetFPID().GetLibItemName() ) );
        return true;
    }
    catch( const IO_ERROR& ioe )
    {
        DisplayError( this, ioe.What() );

        aModule->SetFPID( LIB_ID( aLibraryName, aModule->GetFPID().GetLibItemName() ) );
        return false;
    }
}


bool FOOTPRINT_EDIT_FRAME::SaveFootprintToBoard( bool aAddNew )
{
    // update module in the current board,
    // not just add it to the board with total disregard for the netlist...
    PCB_EDIT_FRAME* pcbframe = (PCB_EDIT_FRAME*) Kiway().Player( FRAME_PCB, false );

    if( pcbframe == NULL )      // happens when the board editor is not active (or closed)
    {
        DisplayErrorMessage( this, _("No board currently open." ) );
        return false;
    }

    BOARD*  mainpcb  = pcbframe->GetBoard();
    MODULE* source_module  = NULL;
    MODULE* module_in_edit = GetBoard()->GetFirstModule();

    // Search the old module (source) if exists
    // Because this source could be deleted when editing the main board...
    if( module_in_edit->GetLink() )        // this is not a new module ...
    {
        source_module = nullptr;

        for( auto mod : mainpcb->Modules() )
        {
            if( module_in_edit->GetLink() == mod->GetTimeStamp() )
            {
                source_module = mod;
                break;
            }
        }
    }

    if( !aAddNew && source_module == NULL ) // source not found
    {
        DisplayError( this, _( "Unable to find the footprint on the main board.\nCannot save." ) );
        return false;
    }

    if( aAddNew && source_module != NULL )
    {
        DisplayError( this, _( "Footprint already exists on board." ) );
        return false;
    }

    m_toolManager->RunAction( PCB_ACTIONS::selectionClear, true );
    pcbframe->GetToolManager()->RunAction( PCB_ACTIONS::selectionClear, true );
    BOARD_COMMIT commit( pcbframe );

    // Create the "new" module
    MODULE* newmodule = new MODULE( *module_in_edit );
    newmodule->SetParent( mainpcb );
    newmodule->SetLink( 0 );

    if( source_module )         // this is an update command
    {
        // In the main board,
        // the new module replace the old module (pos, orient, ref, value
        // and connexions are kept)
        // and the source_module (old module) is deleted
        pcbframe->Exchange_Module( source_module, newmodule, commit );
        newmodule->SetTimeStamp( module_in_edit->GetLink() );
        commit.Push( wxT( "Update module" ) );
    }
    else        // This is an insert command
    {
        KIGFX::VIEW_CONTROLS* viewControls = pcbframe->GetCanvas()->GetViewControls();
        VECTOR2D              cursorPos = viewControls->GetCursorPosition();

        commit.Add( newmodule );
        viewControls->SetCrossHairCursorPosition( VECTOR2D( 0, 0 ), false );
        pcbframe->PlaceModule( newmodule );
        newmodule->SetPosition( wxPoint( 0, 0 ) );
        viewControls->SetCrossHairCursorPosition( cursorPos, false );
        newmodule->SetTimeStamp( GetNewTimeStamp() );
        commit.Push( wxT( "Insert module" ) );

        pcbframe->Raise();
        pcbframe->GetToolManager()->RunAction( PCB_ACTIONS::placeModule, true, newmodule );
    }

    newmodule->ClearFlags();

    return true;
}


bool FOOTPRINT_EDIT_FRAME::SaveFootprintAs( MODULE* aModule )
{
    if( aModule == NULL )
        return false;

    FP_LIB_TABLE* tbl = Prj().PcbFootprintLibs();

    SetMsgPanel( aModule );

    wxString libraryName = aModule->GetFPID().GetLibNickname();
    wxString footprintName = aModule->GetFPID().GetLibItemName();
    bool updateValue = ( aModule->GetValue() == footprintName );

    wxArrayString              headers;
    std::vector<wxArrayString> itemsToDisplay;
    std::vector<wxString>      nicknames = tbl->GetLogicalLibs();

    headers.Add( _( "Nickname" ) );
    headers.Add( _( "Description" ) );

    for( unsigned i = 0; i < nicknames.size(); i++ )
    {
        wxArrayString item;
        item.Add( nicknames[i] );
        item.Add( tbl->GetDescription( nicknames[i] ) );
        itemsToDisplay.push_back( item );
    }

    EDA_LIST_DIALOG dlg( this, FMT_SAVE_MODULE, headers, itemsToDisplay, libraryName,
                         nullptr, nullptr, /* sort */ false, /* show headers */ false );
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
    nameValidator.SetCharExcludes( MODULE::StringLibNameInvalidChars( false ) );
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
        DisplayError( NULL, _( "No library specified.  Footprint could not be saved." ) );
        return false;
    }

    footprintName = nameTextCtrl->GetValue();
    footprintName.Trim( true );
    footprintName.Trim( false );

    if( footprintName.IsEmpty() )
    {
        DisplayError( NULL, _( "No footprint name specified.  Footprint could not be saved." ) );
        return false;
    }

    aModule->SetFPID( LIB_ID( libraryName, footprintName ) );

    if( updateValue )
        aModule->SetValue( footprintName );

    // Legacy libraries are readable, but modifying legacy format is not allowed
    // So prompt the user if he try to add/replace a footprint in a legacy lib
    wxString    libfullname = Prj().PcbFootprintLibs()->FindRow( libraryName )->GetFullURI();
    IO_MGR::PCB_FILE_T  piType = IO_MGR::GuessPluginTypeFromLibPath( libfullname );

    if( piType == IO_MGR::LEGACY )
    {
        DisplayInfoMessage( this, INFO_LEGACY_LIB_WARN_EDIT );
        return false;
    }

    bool module_exists = tbl->FootprintExists( libraryName, footprintName );

    if( module_exists )
    {
        wxString msg = wxString::Format( _( "Footprint %s already exists in %s." ),
                                         footprintName,
                                         libraryName );
        KIDIALOG chkdlg( this, msg, _( "Confirmation" ), wxOK | wxCANCEL | wxICON_WARNING );
        chkdlg.SetOKLabel( _( "Overwrite" ) );

        if( chkdlg.ShowModal() == wxID_CANCEL )
            return false;
    }

    if( !SaveFootprintInLibrary( aModule, libraryName ) )
        return false;

    // Once saved-as a board footprint is no longer a board footprint
    aModule->SetLink( 0 );

    wxString fmt = module_exists ? _( "Component \"%s\" replaced in \"%s\"" ) :
                                   _( "Component \"%s\" added in  \"%s\"" );

    wxString msg = wxString::Format( fmt, footprintName.GetData(), libraryName.GetData() );
    SetStatusText( msg );
    updateTitle();
    ReCreateHToolbar();

    return true;
}


bool FOOTPRINT_EDIT_FRAME::RevertFootprint()
{
    if( GetScreen()->IsModify() && m_revertModule )
    {
        wxString msg = wxString::Format( _( "Revert \"%s\" to last version saved?" ),
                                         GetChars( GetLoadedFPID().GetLibItemName() ) );

        if( ConfirmRevertDialog( this, msg ) )
        {
            Clear_Pcb( false );
            AddModuleToBoard( (MODULE*) m_revertModule->Clone() );

            Zoom_Automatique( false );

            Update3DView( true );

            GetScreen()->ClearUndoRedoList();
            GetScreen()->ClrModify();

            updateView();
            GetCanvas()->Refresh();

            return true;
        }
    }

    return false;
}


MODULE* PCB_BASE_FRAME::CreateNewModule( const wxString& aModuleName )
{
    // Creates a new footprint at position 0,0 which contains the minimal items:
    // the reference and the value.
    //   Value : initialized to the footprint name.
    //           put on fab layer (front side)
    //   Reference : initialized to a default value (REF**).
    //               put on silkscreen layer (front side)

    wxString moduleName = aModuleName;

    // Ask for the new module name
    if( moduleName.IsEmpty() )
    {
        WX_TEXT_ENTRY_DIALOG dlg( this, FMT_MOD_REF, FMT_MOD_CREATE, moduleName );
        dlg.SetTextValidator( FILE_NAME_CHAR_VALIDATOR( &moduleName ) );

        if( dlg.ShowModal() != wxID_OK )
            return NULL;    //Aborted by user
    }

    moduleName.Trim( true );
    moduleName.Trim( false );

    if( moduleName.IsEmpty() )
    {
        DisplayInfoMessage( this, FMT_NO_REF_ABORTED );
        return NULL;
    }

    // Creates the new module and add it to the head of the linked list of modules
    MODULE* module = new MODULE( GetBoard() );

    // Update parameters: timestamp ...
    module->SetLastEditTime();

    // Update its name in lib
    module->SetFPID( LIB_ID( wxEmptyString, moduleName ) );

    wxPoint default_pos;
    BOARD_DESIGN_SETTINGS& settings = GetDesignSettings();

    // Update reference:
    if( settings.m_RefDefaultText.IsEmpty() )
        module->SetReference( moduleName );
    else
        module->SetReference( settings.m_RefDefaultText );

    PCB_LAYER_ID layer = ToLAYER_ID( settings.m_RefDefaultlayer );
    module->Reference().SetThickness( settings.GetTextThickness( layer ) );
    module->Reference().SetTextSize( settings.GetTextSize( layer ) );
    module->Reference().SetItalic( settings.GetTextItalic( layer ) );
    module->Reference().SetKeepUpright( settings.GetTextUpright( layer ) );
    default_pos.y = GetDesignSettings().GetTextSize( layer ).y / 2;
    module->Reference().SetPosition( default_pos );
    module->Reference().SetLayer( layer );
    module->Reference().SetVisible( settings.m_RefDefaultVisibility );

    // Set the value field to a default value
    if( settings.m_ValueDefaultText.IsEmpty() )
        module->SetValue( moduleName );
    else
        module->SetValue( settings.m_ValueDefaultText );

    layer = ToLAYER_ID( settings.m_ValueDefaultlayer );
    module->Value().SetThickness( settings.GetTextThickness( layer ) );
    module->Value().SetTextSize( settings.GetTextSize( layer ) );
    module->Value().SetItalic( settings.GetTextItalic( layer ) );
    module->Value().SetKeepUpright( settings.GetTextUpright( layer ) );
    default_pos.y = -default_pos.y;
    module->Value().SetPosition( default_pos );
    module->Value().SetLayer( layer );
    module->Value().SetVisible( settings.m_ValueDefaultVisibility );

    SetMsgPanel( module );
    return module;
}


wxString PCB_BASE_FRAME::SelectLibrary( const wxString& aNicknameExisting )
{
    wxArrayString headers;

    headers.Add( _( "Nickname" ) );
    headers.Add( _( "Description" ) );

    FP_LIB_TABLE*   fptbl = Prj().PcbFootprintLibs();

    std::vector< wxArrayString > itemsToDisplay;
    std::vector< wxString >      nicknames = fptbl->GetLogicalLibs();

    for( unsigned i = 0; i < nicknames.size(); i++ )
    {
        wxArrayString item;

        item.Add( nicknames[i] );
        item.Add( fptbl->GetDescription( nicknames[i] ) );

        itemsToDisplay.push_back( item );
    }

    EDA_LIST_DIALOG dlg( this, FMT_SELECT_LIB, headers, itemsToDisplay, aNicknameExisting );

    if( dlg.ShowModal() != wxID_OK )
        return wxEmptyString;

    wxString nickname = dlg.GetTextSelection();

    wxLogDebug( wxT( "Chose footprint library \"%s\"." ), GetChars( nickname ) );

    return nickname;
}
