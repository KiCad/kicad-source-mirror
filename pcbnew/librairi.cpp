/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 1992-2015 KiCad Developers, see change_log.txt for contributors.
 *
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
#include <class_drawpanel.h>
#include <confirm.h>
#include <kicad_string.h>
#include <gestfich.h>
#include <wxPcbStruct.h>
#include <dialog_helpers.h>
#include <filter_reader.h>
#include <macros.h>
#include <fp_lib_table.h>
#include <validators.h>

#include <class_board.h>
#include <class_module.h>

#include <pcbnew.h>
#include <module_editor_frame.h>
#include <wildcards_and_files_ext.h>
#include <kicad_plugin.h>
#include <legacy_plugin.h>

#include <dialog_select_pretty_lib.h>


// unique, "file local" translations:

#define FMT_OK_OVERWRITE    _( "Library '%s' exists, OK to replace ?" )
#define FMT_CREATE_LIB      _( "Create New Library Folder (the .pretty folder is the library)" )
#define FMT_OK_DELETE       _( "OK to delete footprint %s in library '%s'" )
#define FMT_IMPORT_MODULE   _( "Import Footprint" )
#define FMT_FILE_NOT_FOUND  _( "File '%s' not found" )
#define FMT_NOT_MODULE      _( "Not a footprint file" )
#define FMT_MOD_NOT_FOUND   _( "Unable to find or load footprint %s from lib path '%s'" )
#define FMT_BAD_PATH        _( "Unable to find or load footprint from path '%s'" )
#define FMT_BAD_PATHS       _( "The footprint library '%s' could not be found in any of the search paths." )
#define FMT_LIB_READ_ONLY   _( "Library '%s' is read only, not writable" )

#define FMT_EXPORT_MODULE   _( "Export Footprint" )
#define FMT_SAVE_MODULE     _( "Save Footprint" )
#define FMT_MOD_REF         _( "Enter footprint name:" )
#define FMT_EXPORTED        _( "Footprint exported to file '%s'" )
#define FMT_MOD_DELETED     _( "Footprint %s deleted from library '%s'" )
#define FMT_MOD_CREATE      _( "New Footprint" )

#define FMT_MOD_EXISTS      _( "Footprint %s already exists in library '%s'" )
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

static const wxString ModLegacyExportFileWildcard( _( "Legacy foot print export files (*.emp)|*.emp" ) );
static const wxString ModImportFileWildcard( _( "GPcb foot print files (*)|*" ) );


#define EXPORT_IMPORT_LASTPATH_KEY wxT( "import_last_path" )


MODULE* FOOTPRINT_EDIT_FRAME::Import_Module()
{
    // use the clipboard for this in the future?

    // Some day it might be useful save the last library type selected along with the path.
    static int lastFilterIndex = 0;

    wxString        lastOpenedPathForLoading = m_mruPath;
    wxConfigBase*   config = Kiface().KifaceSettings();

    if( config )
        config->Read( EXPORT_IMPORT_LASTPATH_KEY, &lastOpenedPathForLoading );

    wxString wildCard;

    wildCard << wxGetTranslation( KiCadFootprintLibFileWildcard ) << wxChar( '|' )
             << wxGetTranslation( ModLegacyExportFileWildcard ) << wxChar( '|' )
             << wxGetTranslation( ModImportFileWildcard ) << wxChar( '|' )
             << wxGetTranslation( GedaPcbFootprintLibFileWildcard );

    wxFileDialog dlg( this, FMT_IMPORT_MODULE,
                      lastOpenedPathForLoading, wxEmptyString,
                      wildCard, wxFD_OPEN | wxFD_FILE_MUST_EXIST );
    dlg.SetFilterIndex( lastFilterIndex );

    if( dlg.ShowModal() == wxID_CANCEL )
        return NULL;

    lastFilterIndex = dlg.GetFilterIndex();

    FILE* fp = wxFopen( dlg.GetPath(), wxT( "rt" ) );

    if( !fp )
    {
        wxString msg = wxString::Format( FMT_FILE_NOT_FOUND, GetChars( dlg.GetPath() ) );
        DisplayError( this, msg );
        return NULL;
    }

    if( config )    // Save file path
    {
        lastOpenedPathForLoading = wxPathOnly( dlg.GetPath() );
        config->Write( EXPORT_IMPORT_LASTPATH_KEY, lastOpenedPathForLoading );
    }

    wxString    moduleName;

    bool        isGeda   = false;
    bool        isLegacy = false;

    {
        FILE_LINE_READER         freader( fp, dlg.GetPath() );   // I own fp, and will close it.
        WHITESPACE_FILTER_READER reader( freader );              // skip blank lines

        reader.ReadLine();
        char* line = reader.Line();

        if( !strnicmp( line, "(module", 7 ) )
        {
            // isKicad = true;
        }
        else if( !strnicmp( line, FOOTPRINT_LIBRARY_HEADER, FOOTPRINT_LIBRARY_HEADER_CNT ) )
        {
            isLegacy = true;

            while( reader.ReadLine() )
            {
                if( !strnicmp( line, "$MODULE", 7 ) )
                {
                    moduleName = FROM_UTF8( StrPurge( line + sizeof( "$MODULE" ) -1 ) );
                    break;
                }
            }
        }
        else if( !strnicmp( line, "Element", 7 ) )
        {
            isGeda = true;
        }
        else
        {
            DisplayError( this, FMT_NOT_MODULE );
            return NULL;
        }

        // fp is closed here by ~FILE_LINE_READER()
    }

    MODULE*   module;

    if( isGeda )
    {
        try
        {
            wxFileName fn = dlg.GetPath();
            PLUGIN::RELEASER pi( IO_MGR::PluginFind( IO_MGR::GEDA_PCB ) );

            moduleName = fn.GetName();
            module = pi->FootprintLoad( fn.GetPath(), moduleName );

            if( !module )
            {
                wxString msg = wxString::Format(
                    FMT_MOD_NOT_FOUND, GetChars( moduleName ), GetChars( fn.GetPath() ) );

                DisplayError( this, msg );
                return NULL;
            }
        }
        catch( const IO_ERROR& ioe )
        {
            DisplayError( this, ioe.errorText );
            return NULL;
        }
    }
    else if( isLegacy )
    {
        try
        {
            PLUGIN::RELEASER pi( IO_MGR::PluginFind( IO_MGR::LEGACY ) );

            module = pi->FootprintLoad( dlg.GetPath(), moduleName );

            if( !module )
            {
                wxString msg = wxString::Format(
                    FMT_MOD_NOT_FOUND, GetChars( moduleName ), GetChars( dlg.GetPath() ) );

                DisplayError( this, msg );
                return NULL;
            }
        }
        catch( const IO_ERROR& ioe )
        {
            DisplayError( this, ioe.errorText );
            return NULL;
        }
    }
    else    //  if( isKicad )
    {
        try
        {
            // This technique was chosen to create an example of how reading
            // the s-expression format from clipboard could be done.

            wxString    fcontents;
            PCB_IO      pcb_io;
            wxFFile     f( dlg.GetPath() );

            if( !f.IsOpened() )
            {
                wxString msg = wxString::Format( FMT_BAD_PATH, GetChars( dlg.GetPath() ) );

                DisplayError( this, msg );
                return NULL;
            }

            f.ReadAll( &fcontents );

            module = dyn_cast<MODULE*>( pcb_io.Parse( fcontents ) );

            if( !module )
            {
                wxString msg = wxString::Format( FMT_BAD_PATH, GetChars( dlg.GetPath() ) );

                DisplayError( this, msg );
                return NULL;
            }
        }
        catch( const IO_ERROR& ioe )
        {
            DisplayError( this, ioe.errorText );
            return NULL;
        }
    }

    // Insert footprint in list
    GetBoard()->Add( module );

    // Display info :
    SetMsgPanel( module );
    PlaceModule( module, NULL );

    if( IsGalCanvasActive() )
        module->SetPosition( wxPoint( 0, 0 ) );

    GetBoard()->m_Status_Pcb = 0;
    GetBoard()->BuildListOfNets();
    updateView();

    return module;
}


void FOOTPRINT_EDIT_FRAME::Export_Module( MODULE* aModule )
{
    wxFileName      fn;
    wxConfigBase*   config = Kiface().KifaceSettings();

    if( !aModule )
        return;

    fn.SetName( aModule->GetFPID().GetFootprintName() );

    wxString    wildcard = wxGetTranslation( KiCadFootprintLibFileWildcard );

    fn.SetExt( KiCadFootprintFileExtension );

    if( config )
    {
        wxString    path;
        config->Read( EXPORT_IMPORT_LASTPATH_KEY, &path, m_mruPath );
        fn.SetPath( path );
    }

    wxFileDialog dlg( this, FMT_EXPORT_MODULE, fn.GetPath(), fn.GetFullName(),
                      wildcard, wxFD_SAVE | wxFD_OVERWRITE_PROMPT );

    if( dlg.ShowModal() == wxID_CANCEL )
        return;

    fn = dlg.GetPath();

    if( config )  // Save file path
    {
        config->Write( EXPORT_IMPORT_LASTPATH_KEY, fn.GetPath() );
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
        fprintf( fp, "%s", pcb_io.GetStringOutput( false ).c_str() );
        fclose( fp );
    }
    catch( const IO_ERROR& ioe )
    {
        DisplayError( this, ioe.errorText );
        return;
    }

    wxString msg = wxString::Format( FMT_EXPORTED, GetChars( dlg.GetPath() ) );
    DisplayInfoMessage( this, msg );
}

bool FOOTPRINT_EDIT_FRAME::SaveCurrentModule( const wxString* aLibPath )
{
    wxString            libPath = aLibPath ? *aLibPath : getLibPath();

    IO_MGR::PCB_FILE_T  piType = IO_MGR::GuessPluginTypeFromLibPath( libPath );

    // Legacy libraries are readable, but writing legacy format is not allowed
    if( piType == IO_MGR::LEGACY )
    {
        DisplayInfoMessage( this, INFO_LEGACY_LIB_WARN_EDIT );
        return false;
    }

    try
    {
        PLUGIN::RELEASER  pi( IO_MGR::PluginFind( piType ) );

        pi->FootprintSave( libPath, GetBoard()->m_Modules );
    }
    catch( const IO_ERROR& ioe )
    {
        DisplayError( this, ioe.errorText );
        return false;
    }
    return true;
}

wxString PCB_BASE_EDIT_FRAME::CreateNewLibrary()
{
    // Kicad cannot write legacy format libraries, only .pretty new format
    // because the legacy format cannot handle current features.
    // The footprint library is actually a directory

    // prompt user for footprint library name, ending by ".pretty"
    // Because there are constraints for the directory name to create,
    // (the name should have the extension ".pretty", and the folder cannot be inside
    // a footprint library), we do not use the standard wxDirDialog.

    wxString initialPath = wxPathOnly( Prj().GetProjectFullName() );
    DIALOG_SELECT_PRETTY_LIB dlg( this, initialPath );

    if( dlg.ShowModal() != wxID_OK )
        return wxEmptyString;

    wxString libPath = dlg.GetFullPrettyLibName();

    // We can save fp libs only using IO_MGR::KICAD format (.pretty libraries)
    IO_MGR::PCB_FILE_T  piType = IO_MGR::KICAD;

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
        {
            // ignore, original values of 'writable' and 'exists' are accurate.
        }

        if( exists )
        {
            if( !writable )
            {
                wxString msg = wxString::Format( FMT_LIB_READ_ONLY, GetChars( libPath ) );
                DisplayError( this, msg );
                return wxEmptyString;
            }
            else
            {
                wxString msg = wxString::Format( FMT_OK_OVERWRITE, GetChars( libPath ) );

                if( !IsOK( this, msg ) )
                    return wxEmptyString;

                pi->FootprintLibDelete( libPath );
            }
        }

        pi->FootprintLibCreate( libPath );
    }
    catch( const IO_ERROR& ioe )
    {
        DisplayError( this, ioe.errorText );
        return wxEmptyString;
    }

    return libPath;
}


bool FOOTPRINT_EDIT_FRAME::DeleteModuleFromCurrentLibrary()
{
    wxString    nickname = GetCurrentLib();

    // Legacy libraries are readable, but modifying legacy format is not allowed
    // So prompt the user if he try to delete a footprint from a legacy lib
    wxString    libfullname = Prj().PcbFootprintLibs()->FindRow(nickname)->GetFullURI();
    IO_MGR::PCB_FILE_T  piType = IO_MGR::GuessPluginTypeFromLibPath( libfullname );

    if( piType == IO_MGR::LEGACY )
    {
        DisplayInfoMessage( this, INFO_LEGACY_LIB_WARN_DELETE );
        return false;
    }

    if( !Prj().PcbFootprintLibs()->IsFootprintLibWritable( nickname ) )
    {
        wxString msg = wxString::Format(
                _( "Library '%s' is read only" ),
                GetChars( nickname )
                );

        DisplayError( this, msg );
        return false;
    }

    wxString    fpid_txt = PCB_BASE_FRAME::SelectFootprint( this, nickname,
                        wxEmptyString, wxEmptyString, Prj().PcbFootprintLibs() );

    if( !fpid_txt )
        return false;

    FPID        fpid( fpid_txt );
    wxString    fpname = fpid.GetFootprintName();

    // Confirmation
    wxString msg = wxString::Format( FMT_OK_DELETE, fpname.GetData(), nickname.GetData() );

    if( !IsOK( this, msg ) )
        return false;

    try
    {
        Prj().PcbFootprintLibs()->FootprintDelete( nickname, fpname );
    }
    catch( const IO_ERROR& ioe )
    {
        DisplayError( this, ioe.errorText );
        return false;
    }

    msg.Printf( FMT_MOD_DELETED, fpname.GetData(), nickname.GetData() );

    SetStatusText( msg );

    return true;
}


void PCB_EDIT_FRAME::ArchiveModulesOnBoard( bool aStoreInNewLib )
{
    if( GetBoard()->m_Modules == NULL )
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

            for( MODULE* curr_fp = GetBoard()->m_Modules; curr_fp; curr_fp = curr_fp->Next() )
            {
                if( !curr_fp->GetFPID().GetFootprintName().empty() )      // Can happen with old boards.
                    tbl->FootprintSave( nickname, curr_fp, false );
            }
        }
        catch( const IO_ERROR& ioe )
        {
            DisplayError( this, ioe.errorText );
        }
    }
    else
    {
        // The footprints are saved in a new .pretty library.
        // If this library already exists, all previous footprints will be deleted
        wxString libPath = CreateNewLibrary();

        if( libPath.IsEmpty() )     // Aborted
            return;

        IO_MGR::PCB_FILE_T  piType = IO_MGR::KICAD;
        PLUGIN::RELEASER  pi( IO_MGR::PluginFind( piType ) );

        for( MODULE* curr_fp = GetBoard()->m_Modules; curr_fp; curr_fp = curr_fp->Next() )
        {
            try
            {
                if( !curr_fp->GetFPID().GetFootprintName().empty() )      // Can happen with old boards.
                    pi->FootprintSave( libPath, curr_fp );
            }
            catch( const IO_ERROR& ioe )
            {
                DisplayError( this, ioe.errorText );
            }
        }
    }
}


bool FOOTPRINT_EDIT_FRAME::SaveFootprintInLibrary( const wxString& aLibrary,
                                             MODULE*         aModule,
                                             bool            aOverwrite,
                                             bool            aDisplayDialog )
{
    if( aModule == NULL )
        return false;

    SetMsgPanel( aModule );


    // Legacy libraries are readable, but modifying legacy format is not allowed
    // So prompt the user if he try to add/replace a footprint in a legacy lib
    wxString    libfullname = Prj().PcbFootprintLibs()->FindRow( aLibrary )->GetFullURI();
    IO_MGR::PCB_FILE_T  piType = IO_MGR::GuessPluginTypeFromLibPath( libfullname );

    if( piType == IO_MGR::LEGACY )
    {
        DisplayInfoMessage( this, INFO_LEGACY_LIB_WARN_EDIT );
        return false;
    }

    // Ask what to use as the footprint name in the library
    wxString footprintName = aModule->GetFPID().GetFootprintName();

    if( aDisplayDialog )
    {
        wxTextEntryDialog dlg( this, _( "Name:" ), FMT_SAVE_MODULE, footprintName );

        if( dlg.ShowModal() != wxID_OK )
            return false;                   // canceled by user

        footprintName = dlg.GetValue();
        footprintName.Trim( true );
        footprintName.Trim( false );

        if( footprintName.IsEmpty() )
            return false;

        if( ! MODULE::IsLibNameValid( footprintName ) )
        {
            wxString msg = wxString::Format(
                    _("Error:\none of invalid chars '%s' found\nin '%s'" ),
                    MODULE::StringLibNameInvalidChars( true ),
                    GetChars( footprintName ) );

            DisplayError( NULL, msg );
            return false;
        }

        aModule->SetFPID( FPID( footprintName ) );
    }

    // Ensure this footprint has a libname
    if( footprintName.IsEmpty() )
    {
        footprintName = wxT("noname");
        aModule->SetFPID( FPID( footprintName ) );
    }

    bool module_exists = false;

    try
    {
        FP_LIB_TABLE* tbl = Prj().PcbFootprintLibs();

        MODULE* m = tbl->FootprintLoad( aLibrary, footprintName );

        if( m )
        {
            delete m;

            module_exists = true;

            // an existing footprint is found in current lib
            if( aDisplayDialog )
            {
                wxString msg = wxString::Format( FMT_MOD_EXISTS,
                        footprintName.GetData(), aLibrary.GetData() );

                SetStatusText( msg );
            }

            if( !aOverwrite )
            {
                // Do not save the given footprint: an old one exists
                return true;
            }
        }

        // this always overwrites any existing footprint, but should yell on its
        // own if the library or footprint is not writable.
        tbl->FootprintSave( aLibrary, aModule );
    }
    catch( const IO_ERROR& ioe )
    {
        DisplayError( this, ioe.errorText );
        return false;
    }

    if( aDisplayDialog )
    {
        wxString fmt = module_exists ?
            _( "Component [%s] replaced in '%s'" ) :
            _( "Component [%s] added in  '%s'" );

        wxString msg = wxString::Format( fmt, footprintName.GetData(), aLibrary.GetData() );
        SetStatusText( msg );
    }

    return true;
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
        wxTextEntryDialog dlg( this, FMT_MOD_REF, FMT_MOD_CREATE, moduleName );
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

    GetBoard()->Add( module );

    // Update parameters: timestamp ...
    module->SetLastEditTime();

    // Update its name in lib
    module->SetFPID( FPID( moduleName ) );

    wxPoint default_pos;
    BOARD_DESIGN_SETTINGS& settings = GetDesignSettings();

    // Update reference:
    if( settings.m_RefDefaultText.IsEmpty() )
        module->SetReference( moduleName );
    else
        module->SetReference( settings.m_RefDefaultText );

    module->Reference().SetThickness( settings.m_ModuleTextWidth );
    module->Reference().SetSize( settings.m_ModuleTextSize );
    default_pos.y = GetDesignSettings().m_ModuleTextSize.y / 2;
    module->Reference().SetPosition( default_pos );
    module->Reference().SetLayer( ToLAYER_ID( settings.m_RefDefaultlayer ) );
    module->Reference().SetVisible( settings.m_RefDefaultVisibility );

    // Set the value field to a default value
    if( settings.m_ValueDefaultText.IsEmpty() )
        module->SetValue( moduleName );
    else
        module->SetValue( settings.m_ValueDefaultText );

    module->Value().SetThickness( GetDesignSettings().m_ModuleTextWidth );
    module->Value().SetSize( GetDesignSettings().m_ModuleTextSize );
    default_pos.y = -default_pos.y;
    module->Value().SetPosition( default_pos );
    module->Value().SetLayer( ToLAYER_ID( settings.m_ValueDefaultlayer ) );
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

    wxLogDebug( wxT( "Chose footprint library '%s'." ), GetChars( nickname ) );

    return nickname;
}
