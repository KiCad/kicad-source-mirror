/**
 * @file librairi.cpp
 * @brief Manage module (footprint) libraries.
 */

#include <fctsys.h>
#include <appl_wxstruct.h>
#include <class_drawpanel.h>
#include <confirm.h>
#include <kicad_string.h>
#include <gestfich.h>
#include <wxPcbStruct.h>
#include <dialog_helpers.h>
#include <richio.h>
#include <filter_reader.h>
#include <pcbcommon.h>
#include <macros.h>

#include <class_board.h>
#include <class_module.h>

#include <pcbnew.h>
#include <class_footprint_library.h>
#include <module_editor_frame.h>


/*
 * Module library header format:
 * Array LIBRARY HEADER-datetime
 * $INDEX
 * List of modules names (1 name per line)
 * $EndIndex
 * List of descriptions of Modules
 * $EndLIBRARY
 */
#define BACKUP_EXT                 wxT( "bak" )
#define FILETMP_EXT                wxT( "$$$" )
#define EXPORT_IMPORT_LASTPATH_KEY wxT( "import_last_path" )

const wxString        ModExportFileExtension( wxT( "emp" ) );

static const wxString ModExportFileWildcard( _( "KiCad foot print export files (*.emp)|*.emp" ) );
static const wxString ModImportFileWildcard( _( "GPcb foot print files (*)|*" ) );


MODULE* FOOTPRINT_EDIT_FRAME::Import_Module()
{
    char*     Line;
    FILE*     file;
    MODULE*   module = NULL;
    bool      Footprint_Is_GPCB_Format = false;

    wxString  LastOpenedPathForLoading;
    wxConfig* Config = wxGetApp().GetSettings();

    if( Config )
        Config->Read( EXPORT_IMPORT_LASTPATH_KEY, &LastOpenedPathForLoading );

    wxString importWildCard = ModExportFileWildcard + wxT("|") + ModImportFileWildcard;
    wxFileDialog dlg( this, _( "Import Footprint Module" ),
                      LastOpenedPathForLoading, wxEmptyString,
                      importWildCard, wxFD_OPEN | wxFD_FILE_MUST_EXIST );

    if( dlg.ShowModal() == wxID_CANCEL )
        return NULL;

    file = wxFopen( dlg.GetPath(), wxT( "rt" ) );

    if( file == NULL )
    {
        wxString msg;
        msg.Printf( _( "File <%s> not found" ), GetChars( dlg.GetPath() ) );
        DisplayError( this, msg );
        return NULL;
    }

    FILE_LINE_READER fileReader( file, dlg.GetPath() );

    FILTER_READER reader( fileReader );

    if( Config )    // Save file path
    {
        LastOpenedPathForLoading = wxPathOnly( dlg.GetPath() );
        Config->Write( EXPORT_IMPORT_LASTPATH_KEY, LastOpenedPathForLoading );
    }

    // Switch the locale to standard C (needed to print floating point numbers like 1.3)
    SetLocaleTo_C_standard();

    // Read header and test file type
    reader.ReadLine();
    Line = reader.Line();

    if( strnicmp( Line, FOOTPRINT_LIBRARY_HEADER, FOOTPRINT_LIBRARY_HEADER_CNT ) != 0 )
    {
        if( strnicmp( Line, "Element", 7 ) == 0 )
        {
            Footprint_Is_GPCB_Format = true;
        }
        else
        {
            DisplayError( this, _( "Not a module file" ) );
            return NULL;
        }
    }

    // Read file: Search the description starting line (skip lib header)
    if( !Footprint_Is_GPCB_Format )
    {
        while( reader.ReadLine() )
        {
            if( strnicmp( Line, "$MODULE", 7 ) == 0 )
                break;
        }
    }

    module = new MODULE( GetBoard() );

    if( Footprint_Is_GPCB_Format )
    {
        module->Read_GPCB_Descr( dlg.GetPath() );
    }
    else
    {
        module->ReadDescr( &reader );
    }

    SetLocaleTo_Default();       // revert to the current locale

    // Insert footprint in list
    GetBoard()->Add( module );

    // Display info :
    module->DisplayInfo( this );
    PlaceModule( module, NULL );
    GetBoard()->m_Status_Pcb = 0;
    GetBoard()->BuildListOfNets();

    return module;
}


void FOOTPRINT_EDIT_FRAME::Export_Module( MODULE* aModule, bool aCreateSysLib )
{
    wxFileName fn;
    FILE*      file;
    wxString   msg, path, title, wildcard;
    wxConfig*  Config = wxGetApp().GetSettings();

    if( aModule == NULL )
        return;

    fn.SetName( aModule->m_LibRef );
    fn.SetExt( aCreateSysLib ? ModuleFileExtension : ModExportFileExtension );

    if( aCreateSysLib )
        path = wxGetApp().ReturnLastVisitedLibraryPath();
    else if( Config )
        Config->Read( EXPORT_IMPORT_LASTPATH_KEY, &path );

    fn.SetPath( path );
    title    = aCreateSysLib ? _( "Create New Library" ) : _( "Export Module" );
    wildcard = aCreateSysLib ?  ModuleFileWildcard : ModExportFileWildcard;
    wxFileDialog dlg( this, msg, fn.GetPath(), fn.GetFullName(), wildcard,
                      wxFD_SAVE | wxFD_OVERWRITE_PROMPT );

    if( dlg.ShowModal() == wxID_CANCEL )
        return;

    fn = dlg.GetPath();
    wxGetApp().SaveLastVisitedLibraryPath( fn.GetPath() );

    if( ( file = wxFopen( fn.GetFullPath(), wxT( "wt" ) ) ) == NULL )
    {
        msg.Printf( _( "Unable to create <%s>" ), GetChars( fn.GetFullPath() ) );
        DisplayError( this, msg );
        return;
    }

    if( !aCreateSysLib && Config )  // Save file path
    {
        Config->Write( EXPORT_IMPORT_LASTPATH_KEY, fn.GetPath() );
    }

    // Switch the locale to standard C (needed to read/write floating point numbers like 1.3)
    SetLocaleTo_C_standard();

    FOOTPRINT_LIBRARY libexport( file );
    libexport.WriteHeader();
    libexport.m_List.Add(aModule->m_LibRef);
    libexport.WriteSectionIndex();

    GetBoard()->m_Modules->Save( file );

    libexport.WriteEndOfFile();
    fclose( file );

    SetLocaleTo_Default();       // revert to the current locale

    msg.Printf( _( "Module exported in file <%s>" ), GetChars( fn.GetFullPath() ) );
    DisplayInfoMessage( this, msg );
}


void FOOTPRINT_EDIT_FRAME::Delete_Module_In_Library( const wxString& aLibname )
{
    wxFileName newFileName;
    wxFileName oldFileName;
    int        LineNum = 0;
    char       Line[1024], Name[256];
    FILE*      out_file, * lib_module;
    wxString   CmpName, msg;

    CmpName = Select_1_Module_From_List( this, aLibname, wxEmptyString, wxEmptyString );

    if( CmpName == wxEmptyString )
        return;

    // Confirmation
    msg.Printf( _( "Ok to delete module %s in library %s" ),
                GetChars( CmpName ), GetChars( aLibname ) );

    if( !IsOK( this, msg ) )
        return;

    oldFileName = aLibname;

    if( ( lib_module = wxFopen( oldFileName.GetFullPath(), wxT( "rt" ) ) )  == NULL )
    {
        wxString msg;
        msg.Printf( _( "Library <%s> not found" ), GetChars(oldFileName.GetFullPath() ) );
        DisplayError( NULL, msg );
        return;
    }


    FOOTPRINT_LIBRARY input_lib( lib_module );

    // Read header.
    if( ! input_lib.IsLibrary() )
    {
        fclose( lib_module );
        wxString msg;
        msg.Printf( _( "<%s> is not a valid footprint library file" ),
                    GetChars( oldFileName.GetFullPath() ) );
        DisplayError( NULL, msg );
        return;
    }

    // Read module names.
    input_lib.RebuildIndex();
    bool found = input_lib.FindInList( CmpName );

    if( !found )
    {
        fclose( lib_module );
        msg.Printf( _( "Module [%s] not found" ), GetChars( CmpName ) );
        DisplayError( NULL, msg );
        return;
    }

    // Create new library.
    newFileName = oldFileName;
    newFileName.SetExt( FILETMP_EXT );

    if( ( out_file = wxFopen( newFileName.GetFullPath(), wxT( "wt" ) ) ) == NULL )
    {
        fclose( lib_module );
        msg.Printf( _( "Unable to create %s" ), GetChars( newFileName.GetFullPath() ) );
        DisplayError( NULL, msg );
        return;
    }

    wxBeginBusyCursor();

    FOOTPRINT_LIBRARY output_lib( out_file );
    output_lib.m_List = input_lib.m_List;

    output_lib.WriteHeader();
    output_lib.RemoveFromList( CmpName );
    output_lib.SortList();
    output_lib.WriteSectionIndex();

    // Copy modules.
    rewind( lib_module );
    LineNum = input_lib.m_LineNum;

    bool copylines = false;
    while( GetLine( lib_module, Line, &LineNum ) )
    {
        StrPurge( Line );

        if( strnicmp( Line, "$MODULE", 7 ) == 0 )
        {
            copylines = true;
            sscanf( Line + 7, " %s", Name );
            msg = FROM_UTF8( Name );

            if( msg.CmpNoCase( CmpName ) == 0 )
            {
                // Delete old module (i.e. do not copy description to out_file).
                while( GetLine( lib_module, Line, &LineNum ) )
                {
                    if( strnicmp( Line, "$EndMODULE", 9 ) == 0 )
                        break;
                }

                continue;
            }
        }

        if( copylines )
            fprintf( out_file, "%s\n", Line );
    }

    fclose( lib_module );
    fclose( out_file );

    wxEndBusyCursor();

    // The old library file is renamed .bak
    wxFileName backupFileName = oldFileName;
    backupFileName.SetExt( BACKUP_EXT );

    if( backupFileName.FileExists() )
        wxRemoveFile( backupFileName.GetFullPath() );

    if( !wxRenameFile( oldFileName.GetFullPath(), backupFileName.GetFullPath() ) )
    {
        msg.Printf( _( "Could not create library back up file <%s>." ),
                    GetChars( backupFileName.GetFullName() ) );
        DisplayError( this, msg );
        return;
    }

    // The temporary file is renamed as the previous library.
    if( !wxRenameFile( newFileName.GetFullPath(), oldFileName.GetFullPath() ) )
    {
        msg.Printf( _("Could not create temporary library file <%s>."),
                    GetChars( oldFileName.GetFullName() ) );
        DisplayError( this, msg );
        return;
    }

    msg.Printf( _( "Component %s deleted in library %s" ), GetChars( CmpName ),
                GetChars( oldFileName.GetFullPath() ) );
    SetStatusText( msg );
}


/* Save modules in a library:
 * param aNewModulesOnly:
 *              true : save modules not already existing in this lib
 *              false: save all modules
 */
void PCB_EDIT_FRAME::ArchiveModulesOnBoard( const wxString& aLibName, bool aNewModulesOnly )
{
    wxString fileName = aLibName;
    wxString path;

    if( GetBoard()->m_Modules == NULL )
    {
        DisplayInfoMessage( this, _( "No modules to archive!" ) );
        return;
    }

    path = wxGetApp().ReturnLastVisitedLibraryPath();

    if( aLibName.IsEmpty() )
    {
        wxFileDialog dlg( this, _( "Library" ), path,
                          wxEmptyString, ModuleFileWildcard,
                          wxFD_SAVE );

        if( dlg.ShowModal() == wxID_CANCEL )
            return;

        fileName = dlg.GetPath();
    }

    wxFileName fn( fileName );
    wxGetApp().SaveLastVisitedLibraryPath( fn.GetPath() );
    bool       file_exists = wxFileExists( fileName );

    if( !aNewModulesOnly && file_exists )
    {
        wxString msg;
        msg.Printf( _( "File %s exists, OK to replace ?" ), GetChars( fileName ) );

        if( !IsOK( this, msg ) )
            return;
    }

    m_canvas->SetAbortRequest( false );

    // Create a new, empty library if no old lib, or if archive all modules
    if( !aNewModulesOnly || !file_exists )
    {
        FILE* lib_module;

        if( ( lib_module = wxFopen( fileName, wxT( "w+t" ) ) )  == NULL )
        {
            wxString msg;
            msg.Printf( _( "Unable to create <%s>" ), GetChars(fileName) );
            DisplayError( this, msg );
            return;
        }

        FOOTPRINT_LIBRARY new_lib( lib_module );
        new_lib.WriteHeader();
        new_lib.WriteSectionIndex();
        new_lib.WriteEndOfFile();
        fclose( lib_module );
    }

    MODULE*  module = (MODULE*) GetBoard()->m_Modules;
    for( int ii = 1; module != NULL; ii++, module = module->Next() )
    {
        // Save footprints in default orientation (0.0) and default layer (FRONT layer)
        int orient = module->GetOrientation();
        if ( orient != 0 )
            module->SetOrientation( 0 );
        int layer = module->GetLayer();
        if(layer != LAYER_N_FRONT )
            module->Flip( module->m_Pos );

        bool success = Save_Module_In_Library( fileName, module,
                                    aNewModulesOnly ? false : true, false );
        // Restore previous orientation and/or side
        if(layer != module->GetLayer() )
            module->Flip( module->m_Pos );
        if ( orient != 0 )
            module->SetOrientation( orient );

        if( !success )
            break;

        // Check for request to stop backup (ESCAPE key actuated)
        if( m_canvas->GetAbortRequest() )
            break;
    }
}


bool PCB_BASE_FRAME::Save_Module_In_Library( const wxString& aLibName,
                                             MODULE*         aModule,
                                             bool            aOverwrite,
                                             bool            aDisplayDialog )
{
    wxFileName oldFileName;
    wxFileName newFileName;
    int        LineNum = 0, tmp;
    char       Name[256], Line[1024];
    wxString   Name_Cmp;
    wxString   msg;
    FILE*      lib_module, * dest;

    if( aModule == NULL )
        return false;

    aModule->DisplayInfo( this );

    newFileName = aLibName;

    if( !newFileName.FileExists( aLibName ) )
    {
        msg.Printf( _( "Library <%s> not found." ), GetChars( aLibName ) );
        DisplayError( this, msg );
        return false;
    }

    if( !IsWritable( newFileName ) )
        return false;

    // Ask for the footprint name in lib
    Name_Cmp = aModule->m_LibRef;

    if( aDisplayDialog )
    {
        wxTextEntryDialog dlg( this, _( "Name:" ), _( "Save module" ), Name_Cmp );

        if( dlg.ShowModal() != wxID_OK )
            return false; // canceled by user

        Name_Cmp = dlg.GetValue();
        Name_Cmp.Trim( true );
        Name_Cmp.Trim( false );

        if( Name_Cmp.IsEmpty() )
            return false;

        aModule->m_LibRef = Name_Cmp;
    }

    // Ensure this footprint has a libname
    if( Name_Cmp.IsEmpty() )
    {
        Name_Cmp = wxT("noname");
        aModule->m_LibRef = Name_Cmp;
    }

    if( ( lib_module = wxFopen( aLibName, wxT( "rt" ) ) ) == NULL )
    {
        msg.Printf( _( "Unable to open <%s>" ), GetChars( aLibName ) );
        DisplayError( this, msg );
        return false;
    }

    // Read library file
    FOOTPRINT_LIBRARY input_lib( lib_module );

    if( ! input_lib.IsLibrary() )
    {
        fclose( lib_module );
        msg.Printf( _( "File <%s> is not an Eeschema library" ), GetChars( aLibName ) );
        DisplayError( this, msg );
        return false;
    }

    // Read footprints in lib: - search for an existing footprint
    input_lib.RebuildIndex();
    bool module_exists = input_lib.FindInList( Name_Cmp );

    if( module_exists )
    {
        // an existing footprint is found in current lib
        if( aDisplayDialog )
        {
            msg = _( "Module exists\n Line: " );
            msg << LineNum;
            SetStatusText( msg );
        }

        if( !aOverwrite )    // Do not save the given footprint: an old one exists
        {
            fclose( lib_module );
            return true;
        }
    }

    // Creates the new library

    newFileName.SetExt( FILETMP_EXT );

    if( ( dest = wxFopen( newFileName.GetFullPath(), wxT( "w+t" ) ) )  == NULL )
    {
        fclose( lib_module );
        msg.Printf( _( "Unable to create <%s>" ), GetChars( newFileName.GetFullPath() ) );
        DisplayError( this, msg );
        return false;
    }

    wxBeginBusyCursor();

    // Switch the locale to standard C (needed to print floating point numbers like 1.3)
    SetLocaleTo_C_standard();

    FOOTPRINT_LIBRARY output_lib( dest );
    output_lib.m_List = input_lib.m_List;

    if( ! module_exists )
        output_lib.m_List.Add( Name_Cmp );

    output_lib.SortList();

    // Create the library header with a new date
    output_lib.WriteHeader();
    output_lib.WriteSectionIndex();

    LineNum = 0;
    rewind( lib_module);

    // Copy footprints, until the old footprint to delete
    bool skip_header = true;

    while( GetLine( lib_module, Line, &LineNum ) )
    {
        StrPurge( Line );
        if( strnicmp( Line, "$EndLIBRARY", 8 ) == 0 )
            continue;

        // Search for the beginning of module section:
        if( skip_header )
        {
            if(  strnicmp( Line, "$MODULE", 7 ) == 0 )
                skip_header = false;
            else
                continue;
        }

        if( strnicmp( Line, "$MODULE", 7 ) == 0 )
        {
            sscanf( Line + 7, " %s", Name );
            msg = FROM_UTF8( Name );

            if( msg.CmpNoCase( Name_Cmp ) == 0 )
            {
                // skip old footprint descr (delete from the lib)
                while( GetLine( lib_module, Line, &LineNum ) )
                {
                    if( strnicmp( Line, "$EndMODULE", 9 ) == 0 )
                        break;
                }

                continue;
            }
        }

        fprintf( dest, "%s\n", Line );
    }

    // Write the new footprint ( append it to the list of footprint )
    tmp = aModule->GetTimeStamp();
    aModule->SetTimeStamp( 0 );
    aModule->Save( dest );
    aModule->SetTimeStamp( tmp );

    output_lib.WriteEndOfFile();

    fclose( dest );
    fclose( lib_module );
    SetLocaleTo_Default();       // revert to the current locale

    wxEndBusyCursor();

    // The old library file is renamed .bak
    oldFileName = aLibName;
    oldFileName.SetExt( BACKUP_EXT );

    if( oldFileName.FileExists() )
        wxRemoveFile( oldFileName.GetFullPath() );

    if( !wxRenameFile( aLibName, oldFileName.GetFullPath() ) )
    {
        msg.Printf( _( "Could not create library back up file <%s>." ),
                      GetChars( oldFileName.GetFullName() ) );
        DisplayError( this, msg );
    }

    // The new library file is renamed
    if( !wxRenameFile( newFileName.GetFullPath(), aLibName ) )
    {
        msg.Printf( _( "Could not create temporary library file <%s>." ),
                      GetChars( aLibName ) );
        DisplayError( this, msg );
        return false;
    }

    if( aDisplayDialog )
    {
        wxString fmt = module_exists ?
            _( "Component [%s] replaced in <%s>" ) :
            _( "Component [%s] added in  <%s>" );
        msg.Printf( fmt, GetChars( Name_Cmp ), GetChars( aLibName ) );
        SetStatusText( msg );
    }

    return true;
}


MODULE* PCB_BASE_FRAME::Create_1_Module( const wxString& aModuleName )
{
    MODULE*  Module;
    wxString moduleName;
    wxPoint  newpos;

    moduleName = aModuleName;

    // Ask for the new module reference
    if( moduleName.IsEmpty() )
    {
        wxTextEntryDialog dlg( this, _( "Module Reference:" ),
                               _( "Module Creation" ), moduleName );

        if( dlg.ShowModal() != wxID_OK )
            return NULL;    //Aborted by user

        moduleName = dlg.GetValue();
    }

    moduleName.Trim( true );
    moduleName.Trim( false );

    if( moduleName.IsEmpty( ) )
    {
        DisplayInfoMessage( this, _( "No reference, aborted" ) );
        return NULL;
    }

    // Creates the new module and add it to the head of the linked list of modules
    Module = new MODULE( GetBoard() );

    GetBoard()->Add( Module );

    // Update parameters: position, timestamp ...
    newpos = GetScreen()->GetCrossHairPosition();
    Module->SetPosition( newpos );
    Module->m_LastEdit_Time = time( NULL );

    // Update its name in lib
    Module->m_LibRef = moduleName;

    // Update reference:
    Module->m_Reference->m_Text = moduleName;
    Module->m_Reference->SetThickness( GetDesignSettings().m_ModuleTextWidth );
    Module->m_Reference->SetSize( GetDesignSettings().m_ModuleTextSize );

    // Set the value field to a default value
    Module->m_Value->m_Text = wxT( "VAL**" );
    Module->m_Value->SetThickness( GetDesignSettings().m_ModuleTextWidth );
    Module->m_Value->SetSize( GetDesignSettings().m_ModuleTextSize );

    Module->SetPosition( wxPoint( 0, 0 ) );

    Module->DisplayInfo( this );
    return Module;
}


void FOOTPRINT_EDIT_FRAME::Select_Active_Library()
{
    wxString msg;

    if( g_LibraryNames.GetCount() == 0 )
        return;

    EDA_LIST_DIALOG dlg( this, _( "Select Active Library:" ), g_LibraryNames, m_CurrentLib );

    if( dlg.ShowModal() != wxID_OK )
        return;

    wxFileName fileName = wxFileName( wxEmptyString, dlg.GetTextSelection(), ModuleFileExtension );
    fileName = wxGetApp().FindLibraryPath( fileName );

    if( fileName.IsOk() && fileName.FileExists() )
    {
        m_CurrentLib = dlg.GetTextSelection();
    }
    else
    {
        msg.Printf( _( "The footprint library <%s> could not be found in any of the search paths." ),
                    GetChars( dlg.GetTextSelection() ) );
        DisplayError( this, msg );
        m_CurrentLib.Empty();
    }

    UpdateTitle();
}


int FOOTPRINT_EDIT_FRAME::CreateLibrary( const wxString& aLibName )
{
    FILE*    lib_module;
    wxString msg;
    wxFileName fileName = aLibName;

    if( fileName.FileExists() )
    {
        msg.Printf( _( "Library <%s> already exists." ), GetChars( fileName.GetFullPath() ) );
        DisplayError( this, msg );
        return 0;
    }

    if( !IsWritable( fileName ) )
        return 0;

    if( ( lib_module = wxFopen( fileName.GetFullPath(), wxT( "wt" ) ) )  == NULL )
    {
        msg.Printf( _( "Unable to create library <%s>" ), GetChars( fileName.GetFullPath() ) );
        DisplayError( this, msg );
        return -1;
    }

    FOOTPRINT_LIBRARY new_lib( lib_module );
    new_lib.WriteHeader();
    new_lib.WriteSectionIndex();
    new_lib.WriteEndOfFile();
    fclose( lib_module );

    return 1;
}
