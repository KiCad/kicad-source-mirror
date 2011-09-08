/***********************************/
/* eeschema.cpp - module principal */
/***********************************/

#include "fctsys.h"
#include "appl_wxstruct.h"
#include "class_drawpanel.h"
#include "confirm.h"
#include "gestfich.h"
#include "eda_dde.h"
#include "id.h"
#include "class_sch_screen.h"
#include "wxEeschemaStruct.h"

#include "general.h"
#include "protos.h"
#include "hotkeys.h"
#include "dialogs/dialog_color_config.h"
#include "transform.h"

#include <wx/snglinst.h>


// Global variables
bool      g_OptNetListUseNames; /* TRUE to use names rather than net
                                 * The numbers (PSPICE netlist only) */
wxSize    g_RepeatStep;
int       g_RepeatDeltaLabel;

bool                     g_HVLines = true;      // Bool: force H or V
                                                // directions (Wires, Bus ..)

struct EESchemaVariables g_EESchemaVar;

int g_DefaultTextLabelSize = DEFAULT_SIZE_TEXT;

HPGL_Pen_Descr_Struct g_HPGL_Pen_Descr;

SCH_SHEET*     g_RootSheet = NULL;

wxString       g_NetCmpExtBuffer( wxT( "cmp" ) );

const wxString SymbolFileExtension( wxT( "sym" ) );
const wxString CompLibFileExtension( wxT( "lib" ) );

const wxString SymbolFileWildcard( wxT( "Kicad drawing symbol file (*.sym)|*.sym" ) );
const wxString CompLibFileWildcard( wxT( "Kicad component library file (*.lib)|*.lib" ) );

LayerStruct g_LayerDescr;            /* layer colors. */

bool        g_EditPinByPinIsOn = false; /* true to do not synchronize pins
                                         * edition  when they are at the
                                         * same location */

int         g_DrawDefaultLineThickness = 6; /* Default line thickness in
                                             * EESCHEMA units used to
                                             * draw/plot items having a
                                             * default thickness line value
                                             * (i.e. = 0 ). 0 = single pixel
                                             * line width */

// Color to draw selected items
int g_ItemSelectetColor = BROWN;

// Color to draw items flagged invisible, in libedit (they are insisible
// in eeschema
int g_InvisibleItemColor = DARKGRAY;

TRANSFORM DefaultTransform = TRANSFORM( 1, 0, 0, -1 );


/************************************/
/* Called to initialize the program */
/************************************/

// Create a new application object: this macro will allow wxWindows to create
// the application object during program execution (it's better than using a
// static object for many reasons) and also declares the accessor function
// wxGetApp() which will return the reference of the right type (i.e. MyApp and
// not wxApp)
IMPLEMENT_APP( EDA_APP )

/* MacOSX: Needed for file association
 * http://wiki.wxwidgets.org/WxMac-specific_topics
 */
void EDA_APP::MacOpenFile( const wxString &fileName )
{
    wxFileName      filename = fileName;
    SCH_EDIT_FRAME* frame = ((SCH_EDIT_FRAME*) GetTopWindow());

    if( !frame )
        return;

    if( !filename.FileExists() )
        return;

    frame->LoadOneEEProject( fileName, false );
}


bool EDA_APP::OnInit()
{
    wxFileName      filename;
    SCH_EDIT_FRAME* frame = NULL;

    InitEDA_Appl( wxT( "EESchema" ), APP_EESCHEMA_T );

    if( m_Checker && m_Checker->IsAnotherRunning() )
    {
        if( !IsOK( NULL, _( "Eeschema is already running, Continue?" ) ) )
            return false;
    }

    if( argc > 1 )
        filename = argv[1];

    // Init EESchema
    SeedLayers();

    // read current setup and reopen last directory if no filename to open in
    // command line
    bool reopenLastUsedDirectory = argc == 1;
    GetSettings( reopenLastUsedDirectory );

   /* Must be called before creating the main frame in order to
    * display the real hotkeys in menus or tool tips */
    ReadHotkeyConfig( wxT("SchematicFrame"), s_Eeschema_Hokeys_Descr );

    // Create main frame (schematic frame) :
    frame = new SCH_EDIT_FRAME( NULL, wxT( "EESchema" ), wxPoint( 0, 0 ), wxSize( 600, 400 ) );

    SetTopWindow( frame );
    frame->Show( true );

    if( CreateServer( frame, KICAD_SCH_PORT_SERVICE_NUMBER ) )
    {
        // RemoteCommand is in controle.cpp and is called when PCBNEW
        // sends EESCHEMA a command
        SetupServerFunction( RemoteCommand );
    }

    frame->Zoom_Automatique( true );

    /* Load file specified in the command line. */
    if( filename.IsOk() )
    {
        wxLogDebug( wxT( "Loading schematic file " ) + filename.GetFullPath() );

        if( filename.GetExt() != SchematicFileExtension )
            filename.SetExt( SchematicFileExtension );

        wxSetWorkingDirectory( filename.GetPath() );

        if( frame->LoadOneEEProject( filename.GetFullPath(), false ) )
            frame->DrawPanel->Refresh( true );
    }
    else
    {
        // Read a default config file if no file to load.
        frame->LoadProjectFile( wxEmptyString, true );
        frame->DrawPanel->Refresh( true );
    }

    return true;
}
