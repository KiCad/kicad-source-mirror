/*************************/
/* PCBNEW: main program  */
/*************************/

#include "fctsys.h"
#include "appl_wxstruct.h"
#include "confirm.h"
#include "macros.h"
#include "class_drawpanel.h"
#include "wxPcbStruct.h"
#include "eda_dde.h"
#include "pcbcommon.h"
#include "colors_selection.h"
#include "gr_basic.h"

#include <wx/file.h>
#include <wx/snglinst.h>

#include "pcbnew.h"
#include "protos.h"
#include "hotkeys.h"


// Colors for layers and items
COLORS_DESIGN_SETTINGS g_ColorsSettings;
int g_DrawDefaultLineThickness = 60; /* Default line thickness in PCBNEW units used to
                                      * draw/plot items having a
                                      * default thickness line value (Frame references)
                                      * (i.e. = 0 ). 0 = single pixel line width */

bool           Drc_On = true;
bool           g_AutoDeleteOldTrack = true;
bool           g_Drag_Pistes_On;
bool           g_Show_Module_Ratsnest;
bool           g_Show_Pads_Module_in_Move = true;
bool           g_Raccord_45_Auto = true;
bool 	       g_Alternate_Track_Posture = false;
bool           g_Track_45_Only_Allowed = true;  // True to allow horiz, vert. and 45deg only tracks
bool           Segments_45_Only;                // True to allow horiz, vert. and 45deg only graphic segments
bool           g_TwoSegmentTrackBuild = true;

wxSize         g_ModuleTextSize;      /* Default footprint texts size */
int            g_ModuleSegmentWidth;
int            g_ModuleTextWidth;
int            Route_Layer_TOP;
int            Route_Layer_BOTTOM;
int            g_MaxLinksShowed;
int            g_MagneticPadOption   = capture_cursor_in_track_tool;
int            g_MagneticTrackOption = capture_cursor_in_track_tool;

wxPoint        g_Offset_Module;     /* Offset de trace du modul en depl */

// Wildcard for footprint libraries filesnames
const wxString g_FootprintLibFileWildcard( wxT( "Kicad footprint library file (*.mod)|*.mod" ) );

/* Name of the document footprint list
 * usually located in share/modules/footprints_doc
 * this is of the responsibility to users to create this file
 * if they want to have a list of footprints
 */
wxString g_DocModulesFileName = wxT( "footprints_doc/footprints.pdf" );

IMPLEMENT_APP( EDA_APP )

/* MacOSX: Needed for file association
 * http://wiki.wxwidgets.org/WxMac-specific_topics
 */
void EDA_APP::MacOpenFile( const wxString& fileName )
{
    wxFileName      filename = fileName;
    PCB_EDIT_FRAME* frame    = ( (PCB_EDIT_FRAME*) GetTopWindow() );

    if( !filename.FileExists() )
        return;

    frame->LoadOnePcbFile( fileName, false );
}


bool EDA_APP::OnInit()
{
    wxFileName      fn;
    PCB_EDIT_FRAME* frame = NULL;

    InitEDA_Appl( wxT( "PCBnew" ), APP_PCBNEW_T );

    if( m_Checker && m_Checker->IsAnotherRunning() )
    {
        if( !IsOK( NULL, _( "PCBnew is already running, Continue?" ) ) )
            return false;
    }

    ScreenPcb = new PCB_SCREEN();

    // read current setup and reopen last directory if no filename to open in command line
    bool reopenLastUsedDirectory = argc == 1;
    GetSettings( reopenLastUsedDirectory );

    if( argc > 1 )
    {
        fn = argv[1];

        if( fn.GetExt() != PcbFileExtension )
        {
            wxLogDebug( wxT( "PCBNew file <%s> has the wrong extension.  \
Changing extension to .brd." ), GetChars( fn.GetFullPath() ) );
            fn.SetExt( PcbFileExtension );
        }

        if( fn.IsOk() && fn.DirExists() )
            wxSetWorkingDirectory( fn.GetPath() );
    }

    g_DrawBgColor = BLACK;

    /* Must be called before creating the main frame in order to
     * display the real hotkeys in menus or tool tips */
    ReadHotkeyConfig( wxT("PcbFrame"), g_Board_Editor_Hokeys_Descr );

    frame = new PCB_EDIT_FRAME( NULL, wxT( "PCBNew" ), wxPoint( 0, 0 ), wxSize( 600, 400 ) );
    frame->UpdateTitle();

    SetTopWindow( frame );
    frame->Show( true );

    if( CreateServer( frame, KICAD_PCB_PORT_SERVICE_NUMBER ) )
    {
        SetupServerFunction( RemoteCommand );
    }

    frame->Zoom_Automatique( true );
    frame->GetScreen()->m_FirstRedraw = false;

    /* Load file specified in the command line. */
    if( fn.IsOk() )
    {
        /* Note the first time PCBnew is called after creating a new project
         * the board file may not exists
         * So we load settings only
         */
        if( fn.FileExists() )
        {
            frame->LoadOnePcbFile( fn.GetFullPath() );
        }
        else
        {   // File does not exists: prepare an empty board
            wxSetWorkingDirectory( fn.GetPath() );
            frame->GetScreen()->SetFileName( fn.GetFullPath( wxPATH_UNIX ) );
            frame->UpdateTitle();
            frame->UpdateFileHistory( frame->GetScreen()->GetFileName() );
            frame->OnModify();          // Ready to save the new empty board
            g_SaveTime = time( NULL );  // Init the time out to save the board

            wxString msg;
            msg.Printf( _( "File <%s> does not exist.\nThis is normal for a new project" ),
                        GetChars( frame->GetScreen()->GetFileName() ) );
            wxMessageBox( msg );
        }
    }

    frame->LoadProjectSettings( fn.GetFullPath() );
    // update the layer names in the listbox
    frame->ReCreateLayerBox( NULL );

    /* For an obscure reason the focus is lost after loading a board file
     * when starting (i.e. only at this point)
     * (seems due to the recreation of the layer manager after loading the file)
     * give focus to main window and Drawpanel
     * must be done for these 2 windows (for an obscure reason ...)
     * Linux specific
     * This is more a workaround than a fix.
     */
    frame->SetFocus();
    frame->DrawPanel->SetFocus();

    return true;
}
