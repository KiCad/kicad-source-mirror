/****************************************/
/* Manage module (footprint) libraries. */
/****************************************/

#include "fctsys.h"
#include "appl_wxstruct.h"
#include "common.h"
#include "class_drawpanel.h"
#include "confirm.h"
#include "kicad_string.h"
#include "gestfich.h"
#include "pcbnew.h"
#include "wxPcbStruct.h"
#include "module_editor_frame.h"
#include "dialog_helpers.h"
#include "richio.h"
#include "filter_reader.h"

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

static const wxString ModExportFileWildcard(
    _( "Kicad foot print export files (*.emp)|*.emp" ) );


static bool CreateDocLibrary( const wxString& LibName );


/*
 * Function Import_Module
 * Read a file containing only one footprint.
 * Used to import (after exporting) a footprint
 * Exported files  have the standard ext .emp
 * This is the same format as .mod files but restricted to only one footprint
 * The import function can also read gpcb footprint file, in Newlib format
 * (One footprint per file, Newlib files have no special ext.)
 */
MODULE* WinEDA_ModuleEditFrame::Import_Module( )
{
    char*     Line;
    FILE*     file;
    MODULE*   module = NULL;
    bool      Footprint_Is_GPCB_Format = false;

    wxString  LastOpenedPathForLoading;
    wxConfig* Config = wxGetApp().m_EDA_Config;

    if( Config )
        Config->Read( EXPORT_IMPORT_LASTPATH_KEY, &LastOpenedPathForLoading );

    wxFileDialog dlg( this, _( "Import Footprint Module" ),
                      LastOpenedPathForLoading, wxEmptyString,
                      ModExportFileWildcard, wxFD_OPEN | wxFD_FILE_MUST_EXIST );

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

    // Switch the locale to standard C (needed to print floating point numbers
    // like 1.3)
    SetLocaleTo_C_standard();

    /* Read header and test file type */
    reader.ReadLine();
    Line = reader.Line();
    if( strnicmp( Line, ENTETE_LIBRAIRIE, L_ENTETE_LIB ) != 0 )
    {
        if( strnicmp( Line, "Element", 7 ) == 0 )
            Footprint_Is_GPCB_Format = true;
        else
        {
            DisplayError( this, _( "Not a module file" ) );
            return NULL;
        }
    }

    /* Read file: Search the description starting line (skip lib header)*/
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

    /* Insert footprint in list*/
    GetBoard()->Add( module );

    /* Display info : */
    module->DisplayInfo( this );
    Place_Module( module, NULL );
    GetBoard()->m_Status_Pcb = 0;
    GetBoard()->m_NetInfo->BuildListOfNets();

    return module;
}


/**
 * Function Export_Module
 * Create a file containing only one footprint.
 * Used to export a footprint
 * Exported files  have the standard ext .emp
 * This is the same format as .mod files but restricted to only one footprint
 * So Create a new lib (which will contains one module) and export a footprint
 * is basically the same thing
 * @param aModule = the module to export
 * @param aCreateSysLib : true = use default lib path to create lib
 *                    false = use current path or last used path to export the footprint
 */
void WinEDA_ModuleEditFrame::Export_Module( MODULE* aModule, bool aCreateSysLib )
{
    wxFileName fn;
    char       Line[1025];
    FILE*      file;
    wxString   msg, path, title, wildcard;
    wxConfig*  Config = wxGetApp().m_EDA_Config;

    if( aModule == NULL )
        return;

    aModule->m_LibRef = aModule->m_Reference->m_Text;
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

    // Switch the locale to standard C (needed to read floating point numbers
    // like 1.3)
    SetLocaleTo_C_standard();

    fprintf( file, "%s  %s\n", ENTETE_LIBRAIRIE, DateAndTime( Line ) );
    fputs( "$INDEX\n", file );

    fprintf( file, "%s\n", CONV_TO_UTF8( aModule->m_LibRef ) );
    fputs( "$EndINDEX\n", file );

    GetBoard()->m_Modules->Save( file );

    fputs( "$EndLIBRARY\n", file );
    fclose( file );

    SetLocaleTo_Default();       // revert to the current locale

    msg.Printf( _( "Module exported in file <%s>" ),
                GetChars( fn.GetFullPath() ) );
    DisplayInfoMessage( this, msg );
}


void WinEDA_ModuleEditFrame::Delete_Module_In_Library( const wxString& aLibname )
{
    wxFileName newFileName;
    wxFileName oldFileName;
    int        ii, NoFound = 1, LineNum = 0;
    char       Line[1024], Name[256];
    FILE*      dest, * lib_module;
    wxString   CmpName, msg;

    CmpName = Select_1_Module_From_List( this,
                                         aLibname,
                                         wxEmptyString,
                                         wxEmptyString );
    if( CmpName == wxEmptyString )
        return;

    /* Confirmation */
    msg.Printf( _( "Ok to delete module %s in library %s" ),
               GetChars( CmpName ), GetChars( aLibname ) );

    if( !IsOK( this, msg ) )
        return;

    oldFileName = aLibname;

    if( ( lib_module = wxFopen( oldFileName.GetFullPath(),
                               wxT( "rt" ) ) )  == NULL )
    {
        wxString msg;
        msg = _( "Library " ) + oldFileName.GetFullPath() + _( " not found" );
        DisplayError( this, msg );
        return;
    }


    /* Read header. */
    GetLine( lib_module, Line, &LineNum );

    if( strnicmp( Line, ENTETE_LIBRAIRIE, L_ENTETE_LIB ) != 0 )
    {
        DisplayError( this, _( "Not a Library file" ) );
        fclose( lib_module );
        return;
    }

    /* Read module names.  */
    while( GetLine( lib_module, Line, &LineNum ) )
    {
        if( strnicmp( Line, "$INDEX", 6 ) == 0 )
        {
            while( GetLine( lib_module, Line, &LineNum ) )
            {
                StrPurge( Line );
                msg = CONV_FROM_UTF8( Line );
                if( CmpName.CmpNoCase( msg ) == 0 ) /* New module? */
                {
                    NoFound = 0; break;
                }
                if( strnicmp( Line, "$EndINDEX", 9 ) == 0 )
                    break;
            }
        }
        if( strnicmp( Line, "$EndINDEX", 9 ) == 0 )
            break;
    }

    if( NoFound )
    {
        fclose( lib_module );
        msg.Printf( _( "Module [%s] not found" ), GetChars( CmpName ) );
        DisplayError( this, msg );
        return;
    }

    /* Create new library. */
    newFileName = oldFileName;
    newFileName.SetExt( FILETMP_EXT );

    if( ( dest = wxFopen( newFileName.GetFullPath(), wxT( "wt" ) ) ) == NULL )
    {
        fclose( lib_module );
        wxString msg;
        msg = _( "Unable to create " ) + newFileName.GetFullPath();
        DisplayError( this, msg );
        return;
    }

    wxBeginBusyCursor();

    /* Create header with new date. */
    fprintf( dest, ENTETE_LIBRAIRIE );
    fprintf( dest, "  %s\n$INDEX\n", DateAndTime( Line ) );

    fseek( lib_module, 0, 0 );
    GetLine( lib_module, Line, &ii );

    while( GetLine( lib_module, Line, &ii ) )
    {
        if( strnicmp( Line, "$M", 2 ) == 0 )
            break;
        if( strnicmp( Line, "$INDEX", 6 ) == 0 )
        {
            while( GetLine( lib_module, Line, &ii ) )
            {
                if( strnicmp( Line, "$EndINDEX", 9 ) == 0 )
                    break;
                StrPurge( Line );
                msg = CONV_FROM_UTF8( Line );
                if( CmpName.CmpNoCase( msg ) != 0 )
                    fprintf( dest, "%s\n", Line );
            }
        }
        if( strnicmp( Line, "$EndINDEX", 9 ) == 0 )
            break;
    }

    fprintf( dest, "$EndINDEX\n" );

    /* Copy modules. */
    while( GetLine( lib_module, Line, &LineNum ) )
    {
        StrPurge( Line );
        if( strnicmp( Line, "$MODULE", 7 ) == 0 )
        {
            sscanf( Line + 7, " %s", Name );
            msg = CONV_FROM_UTF8( Name );
            if( msg.CmpNoCase( CmpName ) == 0 )
            {
                /* Delete old module. */
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

    fclose( lib_module );
    fclose( dest );

    wxEndBusyCursor();

    /* The old library file is renamed .bak */
    wxFileName backupFileName = oldFileName;
    backupFileName.SetExt( BACKUP_EXT );

    if( backupFileName.FileExists() )
        wxRemoveFile( backupFileName.GetFullPath() );

    if( !wxRenameFile( oldFileName.GetFullPath(),
                      backupFileName.GetFullPath() ) )
    {
        DisplayError( this, wxT( "Librairi.cpp: rename .bak err" ) );
        return;
    }

    /* The temporary file is renamed as the previous library. */
    if( !wxRenameFile( newFileName.GetFullPath(), oldFileName.GetFullPath() ) )
    {
        DisplayError( this, wxT( "Librairi.cpp: rename err 2" ) );
        return;
    }

    msg.Printf( _( "Component %s deleted in library %s" ), GetChars( CmpName ),
               GetChars( oldFileName.GetFullPath() ) );
    Affiche_Message( msg );

    CreateDocLibrary( oldFileName.GetFullPath() );
}


/**
 * Function Archive_Modules
 * Save in the library:
 * All new modules (ie modules not found in this lib) (if NewModulesOnly == true)
 * all modules (if NewModulesOnly == false)
 */
void WinEDA_BasePcbFrame::Archive_Modules( const wxString& LibName,
                                           bool            NewModulesOnly )
{
    int      ii, NbModules = 0;
    float    Pas;
    MODULE*  Module;
    wxString fileName = LibName, path;

    if( GetBoard()->m_Modules == NULL )
    {
        DisplayInfoMessage( this, _( " No modules to archive!" ) );
        return;
    }

    path = wxGetApp().ReturnLastVisitedLibraryPath();
    if( LibName.IsEmpty() )
    {
        wxFileDialog dlg( this, _( "Library" ), path,
                          wxEmptyString, ModuleFileWildcard,
                          wxFD_SAVE | wxFD_OVERWRITE_PROMPT );

        if( dlg.ShowModal() == wxID_CANCEL )
            return;

        fileName = dlg.GetPath();
    }

    wxFileName fn( fileName );
    wxGetApp().SaveLastVisitedLibraryPath( fn.GetPath() );
    bool       file_exists = wxFileExists( fileName );

    if( !NewModulesOnly && file_exists )
    {
        wxString msg;
        msg.Printf( _( "File %s exists, OK to replace ?" ),
                    GetChars( fileName ) );
        if( !IsOK( this, msg ) )
            return;
    }

    DrawPanel->m_AbortRequest = false;

    // Create a new, empty library if no old lib, or if archive all modules
    if( !NewModulesOnly || !file_exists )
    {
        FILE* lib_module;
        if( ( lib_module = wxFopen( fileName, wxT( "w+t" ) ) )  == NULL )
        {
            wxString msg = _( "Unable to create " ) + fileName;
            DisplayError( this, msg );
            return;
        }
        char Line[256];
        fprintf( lib_module, "%s  %s\n", ENTETE_LIBRAIRIE, DateAndTime( Line ) );
        fputs( "$INDEX\n", lib_module );
        fputs( "$EndINDEX\n", lib_module );
        fputs( "$EndLIBRARY\n", lib_module );
        fclose( lib_module );
    }

    /* Calculate the number of modules. */
    Module = (MODULE*) GetBoard()->m_Modules;
    for( ; Module != NULL; Module = (MODULE*) Module->Next() )
        NbModules++;

    Pas = (float) 100 / NbModules;
    DisplayActivity( 0, wxEmptyString );

    Module = (MODULE*) GetBoard()->m_Modules;
    for( ii = 1; Module != NULL; ii++, Module = (MODULE*) Module->Next() )
    {
        if( Save_Module_In_Library( fileName, Module,
                                    NewModulesOnly ? false : true,
                                    false, false ) == 0 )
            break;
        DisplayActivity( (int) ( ii * Pas ), wxEmptyString );
        /* Check for request to stop backup (ESCAPE key actuated) */
        if( DrawPanel->m_AbortRequest )
            break;
    }

    CreateDocLibrary( fileName );
}


/**
 * Function Save_Module_In_Library
 *  Save in an existing library a given footprint
 * @param aLibName = name of the library to use
 * @param aModule = the given footprint
 * @param aOverwrite = true to overwrite an existing footprint, false to abort
 * an existing footprint is found
 * @param aDisplayDialog = true to display a dialog to enter or confirm the
 * footprint name
 * @param aCreateDocFile = true to creates the associated doc file
 * @return :  1 if OK, 0 if abort
 */
int WinEDA_BasePcbFrame::Save_Module_In_Library( const wxString& aLibName,
                                                 MODULE*         aModule,
                                                 bool            aOverwrite,
                                                 bool            aDisplayDialog,
                                                 bool            aCreateDocFile )
{
    wxFileName oldFileName;
    wxFileName newFileName;
    int        newmodule, end;
    int        LineNum = 0, tmp;
    char       Name[256], Line[1024];
    wxString   Name_Cmp;
    wxString   msg;
    FILE*      lib_module, * dest;
    bool       added = true;

    aModule->DisplayInfo( this );

    if( !wxFileExists( aLibName ) )
    {
        msg.Printf( _( "Library %s not found" ), GetChars( aLibName ) );
        DisplayError( this, msg );
        return 0;
    }

    /* Ask for the footprint name in lib */
    Name_Cmp = aModule->m_LibRef;

    if( aDisplayDialog )
    {
        wxTextEntryDialog dlg( this, _( "Name:" ), _( "Save module" ), Name_Cmp );
        if( dlg.ShowModal() != wxID_OK )
            return 0; // cancelled by user
        Name_Cmp = dlg.GetValue();
        Name_Cmp.Trim( true );
        Name_Cmp.Trim( false );
        if( Name_Cmp.IsEmpty() )
            return 0;
        aModule->m_LibRef = Name_Cmp;
    }

    if( ( lib_module = wxFopen( aLibName, wxT( "rt" ) ) ) == NULL )
    {
        msg.Printf( _( "Unable to open %s" ), GetChars( aLibName ) );
        DisplayError( this, msg );
        return 0;
    }

    /* Read library file : library header */
    GetLine( lib_module, Line, &LineNum );
    if( strnicmp( Line, ENTETE_LIBRAIRIE, L_ENTETE_LIB ) != 0 )
    {
        fclose( lib_module );
        msg.Printf( _( "File %s is not a eeschema library" ),
                   GetChars( aLibName ) );
        DisplayError( this, msg );
        return 0;
    }

    /* Read footprints in lib: - search for an existing footprint */
    newmodule = 1; end = 0;
    while( !end && GetLine( lib_module, Line, &LineNum ) )
    {
        if( Line[0] != '$' )
            continue;
        if( strncmp( Line + 1, "INDEX", 5 ) != 0 )
            continue;

        while( GetLine( lib_module, Line, &LineNum ) )
        {
            if( strncmp( Line, "$EndINDEX", 9 ) == 0 )
            {
                end = 1; break;
            }

            StrPurge( Line );
            msg = CONV_FROM_UTF8( Line );
            if( Name_Cmp.CmpNoCase( msg ) == 0 ) /* an existing footprint is
                                                  * found */
            {
                added     = false;
                newmodule = 0;
                if( aDisplayDialog )
                {
                    msg = _( "Module exists\n Line: " );
                    msg << LineNum;
                    Affiche_Message( msg );
                }
                if( !aOverwrite )    /* Do not save the given footprint: an old
                                      * one exists */
                {
                    fclose( lib_module );
                    return 1;
                }
                end = 1; break;
            }
        }
    }

    fclose( lib_module );

    /* Creates the new library */

    if( ( lib_module = wxFopen( aLibName, wxT( "rt" ) ) )  == NULL )
    {
        DisplayError( this, wxT( "Librairi.cpp: Error oldlib not found" ) );
        return 0;
    }

    newFileName = aLibName;
    newFileName.SetExt( FILETMP_EXT );

    if( ( dest = wxFopen( newFileName.GetFullPath(), wxT( "w+t" ) ) )  == NULL )
    {
        fclose( lib_module );
        msg = _( "Unable to create " ) + newFileName.GetFullPath();
        DisplayError( this, msg );
        return 0;
    }

    wxBeginBusyCursor();

    // Switch the locale to standard C (needed to print floating point numbers
    // like 1.3)
    SetLocaleTo_C_standard();

    /* Create the library header with a new date */
    fprintf( dest, ENTETE_LIBRAIRIE );
    fprintf( dest, "  %s\n$INDEX\n", DateAndTime( Line ) );

    LineNum = 0;
    GetLine( lib_module, Line, &LineNum );
    while( GetLine( lib_module, Line, &LineNum ) )
    {
        StrPurge( Line );
        if( strnicmp( Line, "$M", 2 ) == 0 )
            break;
        if( strnicmp( Line, "$INDEX", 6 ) == 0 )
        {
            while( GetLine( lib_module, Line, &LineNum ) )
            {
                if( strnicmp( Line, "$EndINDEX", 9 ) == 0 )
                    break;
                fprintf( dest, "%s\n", Line );
            }
        }
        if( newmodule )
            fprintf( dest, "%s\n", CONV_TO_UTF8( Name_Cmp ) );
        if( strnicmp( Line, "$EndINDEX", 0 ) == 0 )
            break;
    }

    fprintf( dest, "$EndINDEX\n" );

    /* Copy footprints, until the old footprint to delete */
    while( GetLine( lib_module, Line, &LineNum ) )
    {
        StrPurge( Line );
        if( strnicmp( Line, "$EndLIBRARY", 8 ) == 0 )
            continue;
        if( strnicmp( Line, "$MODULE", 7 ) == 0 )
        {
            sscanf( Line + 7, " %s", Name );
            msg = CONV_FROM_UTF8( Name );
            if( msg.CmpNoCase( Name_Cmp ) == 0 )
            {
                /* skip old footprint descr (delete from the lib) */
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

    /* Write the new footprint ( append it to the list of footprint ) */
    tmp = aModule->m_TimeStamp; aModule->m_TimeStamp = 0;
    aModule->Save( dest );
    fprintf( dest, "$EndLIBRARY\n" );
    aModule->m_TimeStamp = tmp;

    fclose( dest );
    fclose( lib_module );
    SetLocaleTo_Default();       // revert to the current locale

    wxEndBusyCursor();

    /* The old library file is renamed .bak */
    oldFileName = aLibName;
    oldFileName.SetExt( BACKUP_EXT );

    if( oldFileName.FileExists() )
        wxRemoveFile( oldFileName.GetFullPath() );

    if( !wxRenameFile( aLibName, oldFileName.GetFullPath() ) )
        DisplayError( this, wxT( "Librairi.cpp: rename .bak err" ) );

    /* The new library file is renamed */
    if( !wxRenameFile( newFileName.GetFullPath(), aLibName ) )
    {
        DisplayError( this, wxT( "Librairi.cpp: rename NewLib err" ) );
        return 0;
    }

    /* creates the new .dcm doc file corresponding to the new library */
    if( aCreateDocFile )
        CreateDocLibrary( aLibName );

    if( aDisplayDialog )
    {
        msg  = _( "Component " ); msg += Name_Cmp;
        msg += added ? _( " added in " ) : _( " replaced in " );
        msg += aLibName;
        Affiche_Message( msg );
    }

    return 1;
}


/**
 * Function Create_1_Module
 * Creates a new module or footprint : A new module contains 2 texts :
 *  First = REFERENCE
 *  Second = VALUE: "VAL**"
 * the new module is added to the board module list
 * @param aModuleName = name of the new footprint
 *   (will the component reference in board)
 * @return a pointer to the new module
 */
MODULE* WinEDA_BasePcbFrame::Create_1_Module( const wxString& aModuleName )
{
    MODULE*  Module;
    wxString moduleName;
    wxPoint  newpos;

    moduleName = aModuleName;

    /* Ask for the new module reference */
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

    // Creates the new module and add it to the head of the linked list of
    // modules
    Module = new MODULE( GetBoard() );

    GetBoard()->Add( Module );

    /* Update parameters: position, timestamp ... */
    newpos = GetScreen()->m_Curseur;
    Module->SetPosition( newpos );
    Module->m_LastEdit_Time = time( NULL );

    /* Update its name in lib */
    Module->m_LibRef = moduleName;

    /* Update reference: */
    Module->m_Reference->m_Text = moduleName;
    Module->m_Reference->SetThickness( g_ModuleTextWidth );
    Module->m_Reference->SetSize( g_ModuleTextSize );

    /* Set the value field to a default value */
    Module->m_Value->m_Text = wxT( "VAL**" );
    Module->m_Value->SetThickness( g_ModuleTextWidth );
    Module->m_Value->SetSize( g_ModuleTextSize );

    Module->SetPosition( wxPoint( 0, 0 ) );

    Module->DisplayInfo( this );
    return Module;
}


void WinEDA_ModuleEditFrame::Select_Active_Library()
{
    if( g_LibName_List.GetCount() == 0 )
        return;

    WinEDAListBox dlg( this, _( "Active Lib:" ), g_LibName_List, m_CurrentLib );

    if( dlg.ShowModal() != wxID_OK )
        return;

    m_CurrentLib = dlg.GetTextSelection();

    SetTitle( _( "Module Editor (lib: " ) + m_CurrentLib + wxT( ")" ) );
    return;
}


int WinEDA_ModuleEditFrame::Create_Librairie( const wxString& LibName )
{
    FILE*    lib_module;
    wxString msg;
    char     cbuf[256];

    if( wxFileExists( LibName ) )
    {
        msg = _( "Library exists " ) + LibName;
        DisplayError( this, msg );
        return 0;
    }

    if( ( lib_module = wxFopen( LibName, wxT( "wt" ) ) )  == NULL )
    {
        msg = _( "Unable to create " ) + LibName;
        DisplayError( this, msg );
        return -1;
    }

    /* Write the header of the new library. */
    if( fprintf( lib_module, ENTETE_LIBRAIRIE ) == 0 )
    {
        msg = _( "Create error " ) + LibName;
        DisplayError( this, msg );
        fclose( lib_module );
        return -1;
    }

    fprintf( lib_module, "  %s\n", DateAndTime( cbuf ) );
    fputs( "$INDEX\n", lib_module );
    fputs( "$EndINDEX\n", lib_module );
    fclose( lib_module );

    return 1;
}


/* Synch. Dcm combines a library Libname
 * (Full file name)
 */
static bool CreateDocLibrary( const wxString& LibName )
{
    char       Line[1024];
    char       cbuf[256];
    wxString   Name, Doc, KeyWord;
    wxFileName fn;
    FILE*      LibMod, * LibDoc;

    fn = LibName;
    fn.SetExt( EXT_DOC );

    LibMod = wxFopen( LibName, wxT( "rt" ) );
    if( LibMod == NULL )
        return false;

    /* Read library header. */
    GetLine( LibMod, Line, NULL, sizeof(Line) - 1 );
    if( strnicmp( Line, ENTETE_LIBRAIRIE, L_ENTETE_LIB ) != 0 )
    {
        fclose( LibMod );
        return false;
    }

    LibDoc = wxFopen( fn.GetFullPath(), wxT( "wt" ) );
    if( LibDoc == NULL )
    {
        fclose( LibMod );
        return false;
    }
    fprintf( LibDoc, ENTETE_LIBDOC );
    fprintf( LibDoc, "  %s\n", DateAndTime( cbuf ) );

    /* Read library. */
    Name = Doc = KeyWord = wxEmptyString;
    while( GetLine( LibMod, Line, NULL, sizeof(Line) - 1 ) )
    {
        if( Line[0] != '$' )
            continue;
        if( strnicmp( Line, "$MODULE", 6 ) == 0 )
        {
            while( GetLine( LibMod, Line, NULL, sizeof(Line) - 1 ) )
            {
                if( Line[0] == '$' )
                {
                    if( Line[1] == 'E' )
                        break;
                    if( Line[1] == 'P' ) /* Pad Descr */
                    {
                        while( GetLine( LibMod, Line, NULL, sizeof(Line) - 1 ) )
                        {
                            if( (Line[0] == '$') && (Line[1] == 'E') )
                                break;
                        }
                    }
                }
                if( Line[0] == 'L' ) /* LibName */
                    Name = CONV_FROM_UTF8( StrPurge( Line + 3 ) );

                if( Line[0] == 'K' ) /* KeyWords */
                    KeyWord = CONV_FROM_UTF8( StrPurge( Line + 3 ) );

                if( Line[0] == 'C' ) /* Doc */
                    Doc = CONV_FROM_UTF8( StrPurge( Line + 3 ) );
            }

            /* Generate the module the documentation. */
            if( ( Name != wxEmptyString )
               && ( ( Doc != wxEmptyString ) || ( KeyWord != wxEmptyString ) ) )
            {
                fprintf( LibDoc, "#\n$MODULE %s\n", CONV_TO_UTF8( Name ) );
                fprintf( LibDoc, "Li %s\n", CONV_TO_UTF8( Name ) );
                if( Doc != wxEmptyString )
                    fprintf( LibDoc, "Cd %s\n", CONV_TO_UTF8( Doc ) );

                if( KeyWord  != wxEmptyString )
                    fprintf( LibDoc, "Kw %s\n", CONV_TO_UTF8( KeyWord ) );

                fprintf( LibDoc, "$EndMODULE\n" );
            }
            Name = Doc = KeyWord = wxEmptyString;
        }     /* End read 1 module */

        if( strnicmp( Line, "$INDEX", 6 ) == 0 )
        {
            while( GetLine( LibMod, Line, NULL, sizeof(Line) - 1 ) )
            {
                if( strnicmp( Line, "$EndINDEX", 9 ) == 0 )
                    break;
            }

            /* End read INDEX */
        }
    }

    fclose( LibMod );
    fprintf( LibDoc, "#\n$EndLIBDOC\n" );
    fclose( LibDoc );
    return true;
}
