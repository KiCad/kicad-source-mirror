/***********************************/
/* eeschema.cpp - module principal */
/***********************************/

#include "fctsys.h"
#include "appl_wxstruct.h"
#include "common.h"
#include "class_drawpanel.h"
#include "confirm.h"
#include "gestfich.h"
#include "program.h"
#include "general.h"
#include "bitmaps.h"
#include "eda_dde.h"

#include "libcmp.h"
#include "protos.h"

#include <wx/snglinst.h>


// Global variables

LibraryStruct* g_LibraryList;    // All part libs are saved here.

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
SCH_ITEM* g_BlockSaveDataList; // List of items to paste (Created by Block Save)

// Gestion d'options
bool      g_HVLines = true;   // Bool: force H or V directions (Wires, Bus ..)

int       g_PlotPSColorOpt;    // True = plot postcript color (see plotps.cpp)

struct EESchemaVariables g_EESchemaVar;

/* Variables globales pour Libview */
wxString  g_CurrentViewLibraryName;   /* nom de la librairie en cours d'examen */
wxString  g_CurrentViewComponentName; /* nom du le composant en cours d'examen */
int       g_ViewConvert;              /* Vue normal / convert */
int       g_ViewUnit;                 /* part a afficher (A, B ..) */

/* Variables globales pour Schematic Edit */
int       g_DefaultTextLabelSize = DEFAULT_SIZE_TEXT;

/* Variables globales pour LibEdit */
int       g_LastTextSize = DEFAULT_SIZE_TEXT;
int       g_LastTextOrient = TEXT_ORIENT_HORIZ;

bool      g_FlDrawSpecificUnit = FALSE;
bool      g_FlDrawSpecificConvert = TRUE;

int       g_PlotFormat;                  /* flag = TYPE_HPGL, TYPE_PS... */
int       g_PlotMargin;                  /* Marge pour traces du cartouche */
float     g_PlotScaleX;
float     g_PlotScaleY; /* coeff d'echelle de trace en unites table tracante */

HPGL_Pen_Descr_Struct g_HPGL_Pen_Descr;

//SCH_SCREEN * ScreenSch;
DrawSheetStruct* g_RootSheet = NULL;
SCH_SCREEN*      g_ScreenLib = NULL;

wxString   g_NetCmpExtBuffer( wxT( "cmp" ) );
wxString   g_SymbolExtBuffer( wxT( "sym" ) );

const wxString CompLibFileExtension( wxT( "lib" ) );

const wxString CompLibFileWildcard( wxT( "Kicad component library file (*.lib)|*.lib" ) );

wxString   g_SimulatorCommandLine;  // ligne de commande pour l'appel au simulateur (gnucap, spice..)
wxString   g_NetListerCommandLine;  // ligne de commande pour l'appel au simulateur (gnucap, spice..)

LayerStruct g_LayerDescr;            /* couleurs des couches  */

/* bool: TRUE si edition des pins pin a pin au lieu */
bool g_EditPinByPinIsOn = FALSE;

int g_LibSymbolDefaultLineWidth; /* default line width  (in EESCHEMA units) used when creating a new graphic item in libedit : 0 = default */
int g_DrawMinimunLineWidth;      /* Minimum line (in EESCHEMA units) thickness used to draw items on screen; 0 = single pixel line width */
int g_PlotLine_Width;            /* Minimum line (in EESCHEMA units) thickness used to Plot/Print items */
// Color to draw selected items
int g_ItemSelectetColor = BROWN;
// Color to draw items flagged invisible, in libedit (they are insisible in eeschema
int g_InvisibleItemColor = DARKGRAY;

/* Variables used by LibEdit */
LibEDA_BaseStruct* LibItemToRepeat = NULL; /* pointer on a graphic item than
                                            * can be duplicated by the Ins key
                                            * (usually the last created item */
LibraryStruct* CurrentLib = NULL;          /* Current opened library */
EDA_LibComponentStruct* CurrentLibEntry = NULL;     /* Current component */
LibEDA_BaseStruct* CurrentDrawItem = NULL;     /* current edited item */

// Current selected alias (for components which have aliases)
wxString CurrentAliasName;

// True if the current component has a "De Morgan" representation
bool g_AsDeMorgan;
int CurrentUnit = 1;
int CurrentConvert = 1;

/* Library (name) containing the last component find by FindLibPart() */
wxString FindLibName;

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
    GetSettings();
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
