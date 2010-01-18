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
#include "id.h"

#include "build_version.h"

#include "protos.h"

bool Drc_On = true;
bool g_AutoDeleteOldTrack = true;
bool g_No_Via_Route;
bool g_Drag_Pistes_On;
bool g_Show_Ratsnest;
bool g_Show_Module_Ratsnest;
bool g_Show_Pads_Module_in_Move = true;
bool g_Raccord_45_Auto = true;
bool Track_45_Only;
bool Segments_45_Only;
bool g_TwoSegmentTrackBuild = true;
bool g_HightLigt_Status;
extern PARAM_CFG_BASE* ParamCfgList[];

int Angle_Rot_Module;
int ModuleSegmentWidth;
int ModuleTextWidth;
int Route_Layer_TOP;
int Route_Layer_BOTTOM;
int g_MaxLinksShowed;
int g_MagneticPadOption = capture_cursor_in_track_tool;
int g_MagneticTrackOption = capture_cursor_in_track_tool;
int g_HightLigth_NetCode = -1;

wxSize ModuleTextSize;       /* Default footprint texts size */
wxPoint g_Offset_Module;     /* Offset de trace du modul en depl */
wxString g_Current_PadName;  // Last used pad name (pad num)

// Wildcard for footprint libraries filesnames
const wxString g_FootprintLibFileWildcard( wxT( "Kicad footprint library file (*.mod)|*.mod" ) );

/* Name of the document footprint list
 * usually located in share/modules/footprints_doc
 * this is of the responsability to users to create this file
 * if they want to have a list of footprints
 */
wxString g_DocModulesFileName = wxT("footprints_doc/footprints.pdf");

IMPLEMENT_APP( WinEDA_App )

/* MacOSX: Needed for file association
 * http://wiki.wxwidgets.org/WxMac-specific_topics 
 */
void WinEDA_App::MacOpenFile(const wxString &fileName) {
    WinEDA_PcbFrame * frame = ((WinEDA_PcbFrame*) GetTopWindow());;
    frame->LoadOnePcbFile( fileName, FALSE );
}


/****************************/
bool WinEDA_App::OnInit()
/****************************/
{
    /* WXMAC application specific */
#ifdef __WXMAC__
//	wxApp::SetExitOnFrameDelete(false);
//	wxApp::s_macAboutMenuItemId = ID_KICAD_ABOUT;
	wxApp::s_macPreferencesMenuItemId = ID_OPTIONS_SETUP;
#endif /* __WXMAC__ */


    wxFileName fn;
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
    GetSettings(reopenLastUsedDirectory);

    if( argc > 1 )
    {
        fn = argv[1];

        if( fn.GetExt() != BoardFileExtension )
        {
            wxLogDebug( wxT( "PcbNew file <%s> has the wrong extension.\
Changing extension to .brd." ),
                        GetChars( fn.GetFullPath() ) );
            fn.SetExt( BoardFileExtension );
        }

        if( fn.IsOk() && fn.DirExists() )
            wxSetWorkingDirectory( fn.GetPath() );
    }

    wxGetApp().ReadCurrentSetupValues( ParamCfgList );
    g_DrawBgColor = BLACK;
    Read_Hotkey_Config( frame, false );  /* Must be called before creating the
                                          * main frame in order to display the
                                          * real hotkeys in menus or tool tips */


    frame = new WinEDA_PcbFrame( NULL, wxT( "PcbNew" ),
                                 wxPoint( 0, 0 ), wxSize( 600, 400 ) );
    frame->SetTitle( GetTitle() + wxT( " " ) + GetBuildVersion() );
    ActiveScreen = ScreenPcb;

    SetTopWindow( frame );
    frame->Show( true );

    if( CreateServer( frame, KICAD_PCB_PORT_SERVICE_NUMBER ) )
    {
        SetupServerFunction( RemoteCommand );
    }

    frame->Read_Config( fn.GetFullPath() );

    frame->Zoom_Automatique( true );

    /* Load file specified in the command line. */
    if( fn.IsOk() )
    {
        frame->LoadOnePcbFile( fn.GetFullPath(), FALSE );
        // update the layer names in the listbox
        frame->ReCreateLayerBox( NULL );
    }

    return true;
}
