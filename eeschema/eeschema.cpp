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

#include "program.h"
#include "general.h"
#include "protos.h"

#include <wx/snglinst.h>


// Global variables

int       g_OptNetListUseNames; /* TRUE pour utiliser les noms de net plutot que
                                 * les numeros (netlist PSPICE seulement) */
SCH_ITEM* g_ItemToRepeat; /* pointeur sur la derniere structure
                           * dessinee pouvant etre dupliquee par la commande
                           * Repeat ( NULL si aucune struct existe ) */
wxSize    g_RepeatStep;
int       g_RepeatDeltaLabel;

SCH_ITEM* g_ItemToUndoCopy; /* copy of last modified schematic item
                             * before it is modified (used for undo managing
                             * to restore old values ) */

bool      g_LastSearchIsMarker; /* True if last seach is a marker serach
                                 * False for a schematic item search
                                 * Used for hotkey next search */

/* Block operation (copy, paste) */
BLOCK_SELECTOR g_BlockSaveDataList; // List of items to paste (Created by Block Save)

// Gestion d'options
bool      g_HVLines = true;   // Bool: force H or V directions (Wires, Bus ..)

struct EESchemaVariables g_EESchemaVar;

/* Variables globales pour Schematic Edit */
int       g_DefaultTextLabelSize = DEFAULT_SIZE_TEXT;

/* Variables globales pour LibEdit */
int       g_LastTextSize = DEFAULT_SIZE_TEXT;
int       g_LastTextOrient = TEXT_ORIENT_HORIZ;

bool      g_FlDrawSpecificUnit = FALSE;
bool      g_FlDrawSpecificConvert = TRUE;

HPGL_Pen_Descr_Struct g_HPGL_Pen_Descr;

//SCH_SCREEN * ScreenSch;
DrawSheetStruct* g_RootSheet = NULL;

wxString   g_NetCmpExtBuffer( wxT( "cmp" ) );

const wxString SymbolFileExtension( wxT( "sym" ) );
const wxString CompLibFileExtension( wxT( "lib" ) );

const wxString SymbolFileWildcard( wxT( "Kicad drawing symbol file (*.sym)|*.sym" ) );
const wxString CompLibFileWildcard( wxT( "Kicad component library file (*.lib)|*.lib" ) );

wxString   g_SimulatorCommandLine;  // ligne de commande pour l'appel au simulateur (gnucap, spice..)
wxString   g_NetListerCommandLine;  // ligne de commande pour l'appel au simulateur (gnucap, spice..)

LayerStruct g_LayerDescr;            /* couleurs des couches  */

bool g_EditPinByPinIsOn = false;   /* true to do not synchronize pins edition
                                             *  when they are at the same location */

int  g_LibSymbolDefaultLineWidth = 0; /* default line width  (in EESCHEMA units)
                                                    *  used when creating a new graphic item in libedit.
                                                    *  0 = use default line thicknes
                                                    */
int  g_DrawDefaultLineThickness = 6; /* Default line (in EESCHEMA units) thickness
                                                   *  used to draw/plot items having a default thickness line value (i.e. = 0 ).
                                                   *  0 = single pixel line width
                                                   */

// Color to draw selected items
int g_ItemSelectetColor = BROWN;
// Color to draw items flagged invisible, in libedit (they are insisible in eeschema
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

bool WinEDA_App::OnInit()
{
    wxFileName fn;
    WinEDA_SchematicFrame* frame = NULL;

    g_DebugLevel = 0;   // Debug level */

    InitEDA_Appl( wxT( "EESchema" ), APP_TYPE_EESCHEMA );

    if( m_Checker && m_Checker->IsAnotherRunning() )
    {
        if( !IsOK( NULL, _( "Eeschema is already running, Continue?" ) ) )
            return false;
    }

    if( argc > 1 )
        fn = argv[1];

    /* init EESCHEMA */
    SeedLayers();

    // read current setup and reopen last directory if no filename to open in command line
    bool reopenLastUsedDirectory = argc == 1;
    GetSettings(reopenLastUsedDirectory);

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
    if( fn.IsOk() )
    {
        if( fn.GetExt() != SchematicFileExtension )
            fn.SetExt( SchematicFileExtension );
        wxSetWorkingDirectory( fn.GetPath() );
        if( frame->DrawPanel
            && frame->LoadOneEEProject( fn.GetFullPath(), false ) <= 0 )
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
