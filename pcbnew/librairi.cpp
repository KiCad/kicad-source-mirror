/**
 * @file librairi.cpp
 * @brief Manage module (footprint) libraries.
 */

#include <fctsys.h>
#include <wx/ffile.h>
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
#include <module_editor_frame.h>
#include <wildcards_and_files_ext.h>
#include <kicad_plugin.h>
#include <legacy_plugin.h>


#define BACKUP_EXT                 wxT( "bak" )
#define FILETMP_EXT                wxT( "$$$" )
#define EXPORT_IMPORT_LASTPATH_KEY wxT( "import_last_path" )

const wxString        ModExportFileExtension( wxT( "emp" ) );

static const wxString ModExportFileWildcard( _( "KiCad foot print export files (*.emp)|*.emp" ) );
static const wxString ModImportFileWildcard( _( "GPcb foot print files (*)|*" ) );


MODULE* FOOTPRINT_EDIT_FRAME::Import_Module()
{
    // use the clipboard for this in the future?

    wxString  lastOpenedPathForLoading;
    wxConfig* config = wxGetApp().GetSettings();

    if( config )
        config->Read( EXPORT_IMPORT_LASTPATH_KEY, &lastOpenedPathForLoading );

    wxString importWildCard = ModExportFileWildcard + wxT("|") + ModImportFileWildcard;

    wxFileDialog dlg( this, _( "Import Footprint Module" ),
                      lastOpenedPathForLoading, wxEmptyString,
                      importWildCard, wxFD_OPEN | wxFD_FILE_MUST_EXIST );

    if( dlg.ShowModal() == wxID_CANCEL )
        return NULL;

    FILE* fp = wxFopen( dlg.GetPath(), wxT( "rt" ) );
    if( !fp )
    {
        wxString msg = wxString::Format( _( "File <%s> not found" ), GetChars( dlg.GetPath() ) );
        DisplayError( this, msg );
        return NULL;
    }

    if( config )    // Save file path
    {
        lastOpenedPathForLoading = wxPathOnly( dlg.GetPath() );
        config->Write( EXPORT_IMPORT_LASTPATH_KEY, lastOpenedPathForLoading );
    }

    wxString    moduleName;

    bool    isGeda   = false;
    bool    isLegacy = false;

    {
        FILE_LINE_READER    freader( fp, dlg.GetPath() );   // I own fp, and will close it.
        FILTER_READER       reader( freader );              // skip blank lines

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
            DisplayError( this, _( "Not a module file" ) );
            return NULL;
        }

        // fp is closed here by ~FILE_LINE_READER()
    }

    MODULE*   module;

    if( isGeda )
    {
        LOCALE_IO   toggle;

        // @todo GEDA plugin
        module = new MODULE( GetBoard() );
        module->Read_GPCB_Descr( dlg.GetPath() );
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
                    _( "Unable to find or load footprint '%s' from lib path '%s'" ),
                    GetChars( moduleName ), GetChars( dlg.GetPath() ) );
                DisplayError( this, msg );
                return NULL;
            }
        }
        catch( IO_ERROR ioe )
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
            PCB_IO      pcb_io( CTL_CLIPBOARD );
            wxFFile     f( dlg.GetPath() );

            if( !f.IsOpened() )
            {
                wxString msg = wxString::Format(
                    _( "Unable to find or load footprint from path '%s'" ),
                    GetChars( dlg.GetPath() ) );
                DisplayError( this, msg );
                return NULL;
            }

            f.ReadAll( &fcontents );

            // @todo Fix this. The layernames are missing, and this fails.
            module = dynamic_cast<MODULE*>( pcb_io.Parse( fcontents ) );

            if( !module )
            {
                wxString msg = wxString::Format(
                    _( "Unable to find or load footprint from lib path '%s'" ),
                    GetChars( dlg.GetPath() ) );
                DisplayError( this, msg );
                return NULL;
            }
        }
        catch( IO_ERROR ioe )
        {
            DisplayError( this, ioe.errorText );
            return NULL;
        }
    }

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
    wxString   msg, path, title, wildcard;
    wxConfig*  config = wxGetApp().GetSettings();

    if( aModule == NULL )
        return;

    fn.SetName( aModule->m_LibRef );
    fn.SetExt( aCreateSysLib ? FootprintLibFileExtension : ModExportFileExtension );

    if( aCreateSysLib )
        path = wxGetApp().ReturnLastVisitedLibraryPath();
    else if( config )
        config->Read( EXPORT_IMPORT_LASTPATH_KEY, &path );

    title    = aCreateSysLib ? _( "Create New Library" ) : _( "Export Module" );
    wildcard = aCreateSysLib ?  LegacyFootprintLibFileWildcard : ModExportFileWildcard;

    fn.SetPath( path );

    wxFileDialog dlg( this, msg, fn.GetPath(), fn.GetFullName(),
                      wxGetTranslation( wildcard ),
                      wxFD_SAVE | wxFD_OVERWRITE_PROMPT );

    if( dlg.ShowModal() == wxID_CANCEL )
        return;

    fn = dlg.GetPath();
    wxGetApp().SaveLastVisitedLibraryPath( fn.GetPath() );

    if( !aCreateSysLib && config )  // Save file path
    {
        config->Write( EXPORT_IMPORT_LASTPATH_KEY, fn.GetPath() );
    }

    wxString libPath = fn.GetFullPath();

    try
    {
#if 1   // This *.kicad_mod export works fine.  It is the import which is still broken.
        // The function PCB_PARSER::Parse() fails with due to the m_layerName[] table
        // being empty.

        // @todo, enable this code asap.

        // Export as *.kicad_pcb format, using a strategy which is specifically chosen
        // as an example on how it could also be used to send it to the system clipboard.

        PCB_IO  pcb_io( CTL_CLIPBOARD );

        /*  This module should *already* be "normalized" in a way such that
            orientation is zero, etc., since it came from module editor.

            module->SetTimeStamp( 0 );
            module->SetParent( 0 );
            module->SetOrientation( 0 );
        */

        LOCALE_IO toggle;
        pcb_io.Format( aModule );

        FILE* fp = wxFopen( dlg.GetPath(), wxT( "wt" ) );
        fprintf( fp, "%s", pcb_io.GetStringOutput( false ).c_str() );
        fclose( fp );
#else
        // Use IO_MGR::LEGACY for now, until the IO_MGR::KICAD plugin is ready.
        PLUGIN::RELEASER pi( IO_MGR::PluginFind( IO_MGR::LEGACY ) );

        try
        {
            // try to delete the library whether it exists or not, quietly.
            pi->FootprintLibDelete( libPath );
        }
        catch( IO_ERROR ioe )
        {
            // Ignore this, it will often happen and is not an error because
            // the library may not exist.  If library was in a read only directory,
            // it will still exist as we get to the FootprintLibCreate() below.
        }

        pi->FootprintLibCreate( libPath );
        pi->FootprintSave( libPath, aModule );
#endif

    }
    catch( IO_ERROR ioe )
    {
        DisplayError( this, ioe.errorText );
        return;
    }

    msg.Printf( _( "Module exported in file <%s>" ), libPath.GetData() );
    DisplayInfoMessage( this, msg );
}


void FOOTPRINT_EDIT_FRAME::Delete_Module_In_Library( const wxString& aLibName )
{
    wxString footprintName = Select_1_Module_From_List( this, aLibName, wxEmptyString, wxEmptyString );

    if( footprintName == wxEmptyString )
        return;

    // Confirmation
    wxString msg = wxString::Format( _( "Ok to delete module '%s' in library '%s'" ),
                        footprintName.GetData(), aLibName.GetData() );

    if( !IsOK( this, msg ) )
        return;

    try
    {
        PLUGIN::RELEASER  pi( IO_MGR::PluginFind( IO_MGR::LEGACY ) );

        pi->FootprintDelete( aLibName, footprintName );
    }
    catch( IO_ERROR ioe )
    {
        DisplayError( NULL, ioe.errorText );
        return;
    }

    msg.Printf( _( "Component '%s' deleted from library '%s'" ),
                footprintName.GetData(), aLibName.GetData() );

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

    if( !aLibName )
    {
        wxFileDialog dlg( this, _( "Library" ), path,
                          wxEmptyString,
                          wxGetTranslation( LegacyFootprintLibFileWildcard ),
                          wxFD_SAVE );

        if( dlg.ShowModal() == wxID_CANCEL )
            return;

        fileName = dlg.GetPath();
    }

    wxFileName fn( fileName );
    wxGetApp().SaveLastVisitedLibraryPath( fn.GetPath() );
    bool       lib_exists = wxFileExists( fileName );

    if( !aNewModulesOnly && lib_exists )
    {
        wxString msg;
        msg.Printf( _( "Library %s exists, OK to replace ?" ), GetChars( fileName ) );

        if( !IsOK( this, msg ) )
            return;
    }

    m_canvas->SetAbortRequest( false );

    try
    {
        PLUGIN::RELEASER pi( IO_MGR::PluginFind( IO_MGR::LEGACY ) );

        // Delete old library if we're replacing it entirely.
        if( lib_exists && !aNewModulesOnly )
        {
            pi->FootprintLibDelete( fileName );
            lib_exists = false;
        }

        if( !lib_exists )
        {
            pi->FootprintLibCreate( fileName );
        }

        if( !aNewModulesOnly )
        {
            for( MODULE* m = GetBoard()->m_Modules;  m;  m = m->Next() )
            {
                pi->FootprintSave( fileName, m );
            }
        }
        else
        {
            for( MODULE* m = GetBoard()->m_Modules;  m;  m = m->Next() )
            {
                if( !Save_Module_In_Library( fileName, m, false, false ) )
                    break;

                // Check for request to stop backup (ESCAPE key actuated)
                if( m_canvas->GetAbortRequest() )
                    break;
            }
        }
    }
    catch( IO_ERROR ioe )
    {
        DisplayError( this, ioe.errorText );
        return;
    }
}


bool PCB_BASE_FRAME::Save_Module_In_Library( const wxString& aLibName,
                                             MODULE*         aModule,
                                             bool            aOverwrite,
                                             bool            aDisplayDialog )
{
    if( aModule == NULL )
        return false;

    aModule->DisplayInfo( this );

    if( !wxFileExists( aLibName ) )
    {
        wxString msg = wxString::Format( _( "Library <%s> not found." ),
                            aLibName.GetData() );
        DisplayError( this, msg );
        return false;
    }

    if( !IsWritable( aLibName ) )
        return false;

    // Ask what to use as the footprint name in the library
    wxString footprintName = aModule->GetLibRef();

    if( aDisplayDialog )
    {
        wxTextEntryDialog dlg( this, _( "Name:" ), _( "Save Module" ), footprintName );

        if( dlg.ShowModal() != wxID_OK )
            return false;                   // canceled by user

        footprintName = dlg.GetValue();
        footprintName.Trim( true );
        footprintName.Trim( false );

        if( footprintName.IsEmpty() )
            return false;

        aModule->SetLibRef( footprintName );
    }

    // Ensure this footprint has a libname
    if( footprintName.IsEmpty() )
    {
        footprintName = wxT("noname");
        aModule->SetLibRef( footprintName );
    }

    MODULE*  module_exists = NULL;

    try
    {
        PLUGIN::RELEASER pi( IO_MGR::PluginFind( IO_MGR::LEGACY ) );

        module_exists = pi->FootprintLoad( aLibName, footprintName );

        if( module_exists )
        {
            delete module_exists;

            // an existing footprint is found in current lib
            if( aDisplayDialog )
            {
                wxString msg = wxString::Format(
                    _( "Footprint '%s' already exists in library '%s'" ),
                    footprintName.GetData(), aLibName.GetData() );

                SetStatusText( msg );
            }

            if( !aOverwrite )
            {
                // Do not save the given footprint: an old one exists
                return true;
            }
        }

        // this always overwrites any existing footprint
        pi->FootprintSave( aLibName, aModule );
    }
    catch( IO_ERROR ioe )
    {
        DisplayError( this, ioe.errorText );
        return false;
    }

    if( aDisplayDialog )
    {
        wxString fmt = module_exists ?
            _( "Component [%s] replaced in <%s>" ) :
            _( "Component [%s] added in  <%s>" );

        wxString msg = wxString::Format( fmt, footprintName.GetData(), aLibName.GetData() );
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
    Module->SetLastEditTime();

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
    if( g_LibraryNames.GetCount() == 0 )
        return;

    EDA_LIST_DIALOG dlg( this, _( "Select Active Library:" ), g_LibraryNames, m_CurrentLib );

    if( dlg.ShowModal() != wxID_OK )
        return;

    wxFileName fileName = wxFileName( wxEmptyString, dlg.GetTextSelection(),
                                      FootprintLibFileExtension );
    fileName = wxGetApp().FindLibraryPath( fileName );

    if( fileName.IsOk() && fileName.FileExists() )
    {
        m_CurrentLib = dlg.GetTextSelection();
    }
    else
    {
        wxString msg = wxString::Format(
                        _( "The footprint library <%s> could not be found in any of the search paths." ),
                        GetChars( dlg.GetTextSelection() ) );

        DisplayError( this, msg );
        m_CurrentLib.Empty();
    }

    UpdateTitle();
}


int FOOTPRINT_EDIT_FRAME::CreateLibrary( const wxString& aLibName )
{
    wxFileName fileName = aLibName;

    if( fileName.FileExists() )
    {
        wxString msg = wxString::Format(
            _( "Library <%s> already exists." ),
            aLibName.GetData() );

        DisplayError( this, msg );
        return 0;
    }

    if( !IsWritable( fileName ) )
        return 0;

    try
    {
        PLUGIN::RELEASER pi( IO_MGR::PluginFind( IO_MGR::LEGACY ) );

        pi->FootprintLibCreate( aLibName );
    }
    catch( IO_ERROR ioe )
    {
        DisplayError( this, ioe.errorText );
        return 0;
    }

    return 1;       // remember how many times we succeeded
}
