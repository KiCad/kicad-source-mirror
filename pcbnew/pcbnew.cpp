/*************************/
/* PCBNEW: main program  */
/*************************/

#include "fctsys.h"
#include "appl_wxstruct.h"
#include "confirm.h"

#include <wx/file.h>
#include <wx/snglinst.h>

#include "common.h"
#include "pcbnew.h"
#include "wxPcbStruct.h"
#include "plot_common.h"
#include "gestfich.h"
#include "pcbplot.h"
#include "autorout.h"
#include "cell.h"
#include "worksheet.h"
#include "zones.h"
#include "drag.h"
#include "eda_dde.h"
#include "colors_selection.h"
#include "class_drawpanel.h"

#include "id.h"

#include "build_version.h"

#include "protos.h"

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
bool           g_Track_45_Only_Allowed = true;  // True to allow horiz, vert. and 45deg only tracks
bool           Segments_45_Only;                // True to allow horiz, vert. and 45deg only graphic segments
bool           g_TwoSegmentTrackBuild = true;
bool           g_HighLight_Status;

int            ModuleSegmentWidth;
int            ModuleTextWidth;
int            Route_Layer_TOP;
int            Route_Layer_BOTTOM;
int            g_MaxLinksShowed;
int            g_MagneticPadOption   = capture_cursor_in_track_tool;
int            g_MagneticTrackOption = capture_cursor_in_track_tool;
int            g_HighLight_NetCode   = -1;

wxSize         ModuleTextSize;      /* Default footprint texts size */
wxPoint        g_Offset_Module;     /* Offset de trace du modul en depl */
wxString       g_Current_PadName;   // Last used pad name (pad num)

// Wildcard for footprint libraries filesnames
const wxString g_FootprintLibFileWildcard( wxT( "Kicad footprint library file (*.mod)|*.mod" ) );

/* Name of the document footprint list
 * usually located in share/modules/footprints_doc
 * this is of the responsibility to users to create this file
 * if they want to have a list of footprints
 */
wxString g_DocModulesFileName = wxT( "footprints_doc/footprints.pdf" );

IMPLEMENT_APP( WinEDA_App )

/* MacOSX: Needed for file association
 * http://wiki.wxwidgets.org/WxMac-specific_topics
 */
void WinEDA_App::MacOpenFile( const wxString& fileName )
{
    wxFileName       filename = fileName;
    WinEDA_PcbFrame* frame    = ( (WinEDA_PcbFrame*) GetTopWindow() );

    if( !filename.FileExists() )
        return;

    frame->LoadOnePcbFile( fileName, FALSE );
}


bool WinEDA_App::OnInit()
{
    /* WXMAC application specific */
#ifdef __WXMAC__

//	wxApp::SetExitOnFrameDelete(false);
//	wxApp::s_macAboutMenuItemId = ID_KICAD_ABOUT;
    wxApp::s_macPreferencesMenuItemId = ID_OPTIONS_SETUP;
#endif /* __WXMAC__ */


    wxFileName       fn;
    WinEDA_PcbFrame* frame = NULL;

    InitEDA_Appl( wxT( "PCBnew" ), APP_TYPE_PCBNEW );

    if( m_Checker && m_Checker->IsAnotherRunning() )
    {
        if( !IsOK( NULL, _( "Pcbnew is already running, Continue?" ) ) )
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
            wxLogDebug( wxT( "PcbNew file <%s> has the wrong extension.  \
Changing extension to .brd." ), GetChars( fn.GetFullPath() ) );
            fn.SetExt( PcbFileExtension );
        }

        if( fn.IsOk() && fn.DirExists() )
            wxSetWorkingDirectory( fn.GetPath() );
    }

    g_DrawBgColor = BLACK;
    Read_Hotkey_Config( frame, false );  /* Must be called before creating the
                                          * main frame in order to display the
                                          * real hotkeys in menus or tool tips */


    frame = new WinEDA_PcbFrame( NULL, wxT( "PcbNew" ), wxPoint( 0, 0 ), wxSize( 600, 400 ) );
    frame->SetTitle( GetTitle() + wxT( " " ) + GetBuildVersion() );
    ActiveScreen = ScreenPcb;

    SetTopWindow( frame );
    frame->Show( true );

    if( CreateServer( frame, KICAD_PCB_PORT_SERVICE_NUMBER ) )
    {
        SetupServerFunction( RemoteCommand );
    }

    frame->Zoom_Automatique( true );

    /* Load file specified in the command line. */
    if( fn.IsOk() )
    {
        frame->LoadOnePcbFile( fn.GetFullPath(), FALSE );

        // update the layer names in the listbox
        frame->ReCreateLayerBox( NULL );
    }

    frame->LoadProjectSettings( fn.GetFullPath() );

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
