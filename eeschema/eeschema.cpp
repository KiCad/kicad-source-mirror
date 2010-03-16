/***********************************/
/* eeschema.cpp - module principal */
/***********************************/

#include "fctsys.h"
#include "appl_wxstruct.h"
#include "common.h"
#include "class_drawpanel.h"
#include "confirm.h"
#include "gestfich.h"
#include "bitmaps.h"
#include "eda_dde.h"
#include "id.h"

#include "program.h"
#include "general.h"
#include "protos.h"

#include <wx/snglinst.h>


// Global variables

int       g_OptNetListUseNames; /* TRUE to use names rather than net
                                 * The numbers (PSPICE netlist only) */
SCH_ITEM* g_ItemToRepeat;       /* Pointer to the last structure
                                 * for duplicatation by the repeat command.
                                 * (NULL if no struct exists) */
wxSize    g_RepeatStep;
int       g_RepeatDeltaLabel;

SCH_ITEM* g_ItemToUndoCopy;     /* copy of last modified schematic item
                                 * before it is modified (used for undo
                                 * managing to restore old values ) */

/* Block operation (copy, paste) */
BLOCK_SELECTOR           g_BlockSaveDataList;   // List of items to paste
                                                // (Created by Block Save)

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

// command line to call the simulator (gnucap, spice..)
wxString    g_SimulatorCommandLine;

// command line to call the simulator net lister (gnucap, spice..)
wxString    g_NetListerCommandLine;

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

int DefaultTransformMatrix[2][2] = { { 1, 0 }, { 0, -1 } };


/************************************/
/* Called to initialize the program */
/************************************/

// Create a new application object: this macro will allow wxWindows to create
// the application object during program execution (it's better than using a
// static object for many reasons) and also declares the accessor function
// wxGetApp() which will return the reference of the right type (i.e. MyApp and
// not wxApp)
IMPLEMENT_APP( WinEDA_App )

/* MacOSX: Needed for file association
 * http://wiki.wxwidgets.org/WxMac-specific_topics
 */
void WinEDA_App::MacOpenFile(const wxString &fileName) {
    wxFileName    filename = fileName;
    WinEDA_SchematicFrame * frame = ((WinEDA_SchematicFrame*) GetTopWindow());

    if(!filename.FileExists())
        return;

    frame->LoadOneEEProject( fileName, false );
}


bool WinEDA_App::OnInit()
{
    /* WXMAC application specific */
#ifdef __WXMAC__
//	wxApp::SetExitOnFrameDelete(false);
//	wxApp::s_macAboutMenuItemId = ID_KICAD_ABOUT;
	wxApp::s_macPreferencesMenuItemId = ID_OPTIONS_SETUP;
#endif /* __WXMAC__ */

    wxFileName             filename;
    WinEDA_SchematicFrame* frame = NULL;

    g_DebugLevel = 0;   // Debug level */

    InitEDA_Appl( wxT( "EESchema" ), APP_TYPE_EESCHEMA );

    if( m_Checker && m_Checker->IsAnotherRunning() )
    {
        if( !IsOK( NULL, _( "Eeschema is already running, Continue?" ) ) )
            return false;
    }

    if( argc > 1 )
        filename = argv[1];

    /* init EESCHEMA */
    SeedLayers();

    // read current setup and reopen last directory if no filename to open in
    // command line
    bool reopenLastUsedDirectory = argc == 1;
    GetSettings( reopenLastUsedDirectory );

    Read_Hotkey_Config( frame, false );   /* Must be called before creating
                                           * the main frame  in order to
                                           * display the real hotkeys in menus
                                           * or tool tips */

    // Create main frame (schematic frame) :
    frame = new WinEDA_SchematicFrame( NULL, wxT( "EESchema" ),
                                       wxPoint( 0, 0 ), wxSize( 600, 400 ) );

    SetTopWindow( frame );
    frame->Show( TRUE );

    if( CreateServer( frame, KICAD_SCH_PORT_SERVICE_NUMBER ) )
    {
        // RemoteCommand is in controle.cpp and is called when PCBNEW
        // sends EESCHEMA a command
        SetupServerFunction( RemoteCommand );
    }

    ActiveScreen = frame->GetScreen();
    frame->Zoom_Automatique( TRUE );

    /* Load file specified in the command line. */
    if( filename.IsOk() )
    {
        if( filename.GetExt() != SchematicFileExtension )
            filename.SetExt( SchematicFileExtension );
        wxSetWorkingDirectory( filename.GetPath() );
        if( frame->DrawPanel
            && frame->LoadOneEEProject( filename.GetFullPath(), false ) <= 0 )
            frame->DrawPanel->Refresh( true );
    }
    else
    {
        // Read a default config file if no file to load.
        frame->LoadProjectFile( wxEmptyString, TRUE );
        if( frame->DrawPanel )
            frame->DrawPanel->Refresh( TRUE );
    }

    return TRUE;
}
