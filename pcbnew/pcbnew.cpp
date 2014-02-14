/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012 Jean-Pierre Charras, jean-pierre.charras@ujf-grenoble.fr
 * Copyright (C) 2012 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 1992-2012 KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

/**
 * @file pcbnew.cpp
 * @brief Pcbnew main program.
 */

#ifdef KICAD_SCRIPTING
#include <python_scripting.h>
#include <pcbnew_scripting_helpers.h>
#endif
#include <fctsys.h>
#include <appl_wxstruct.h>
#include <confirm.h>
#include <macros.h>
#include <class_drawpanel.h>
#include <wxPcbStruct.h>
#include <eda_dde.h>
#include <pcbcommon.h>
#include <colors_selection.h>
#include <gr_basic.h>
#include <3d_viewer.h>
#include <wx/stdpaths.h>

#include <wx/file.h>
#include <wx/snglinst.h>
#include <wx/dir.h>

#include <pcbnew.h>
#include <protos.h>
#include <hotkeys.h>
#include <wildcards_and_files_ext.h>
#include <class_board.h>


// Colors for layers and items
COLORS_DESIGN_SETTINGS g_ColorsSettings;

bool           g_Drc_On = true;
bool           g_AutoDeleteOldTrack = true;
bool           g_Show_Module_Ratsnest;
bool           g_Raccord_45_Auto = true;
bool 	       g_Alternate_Track_Posture = false;
bool           g_Track_45_Only_Allowed = true;  // True to allow horiz, vert. and 45deg only tracks
bool           g_Segments_45_Only;              // True to allow horiz, vert. and 45deg only graphic segments
bool           g_TwoSegmentTrackBuild = true;

LAYER_NUM      g_Route_Layer_TOP;
LAYER_NUM      g_Route_Layer_BOTTOM;
int            g_MaxLinksShowed;
int            g_MagneticPadOption   = capture_cursor_in_track_tool;
int            g_MagneticTrackOption = capture_cursor_in_track_tool;

wxPoint        g_Offset_Module;     /* Distance to offset module trace when moving. */

/* Name of the document footprint list
 * usually located in share/modules/footprints_doc
 * this is of the responsibility to users to create this file
 * if they want to have a list of footprints
 */
wxString      g_DocModulesFileName = wxT( "footprints_doc/footprints.pdf" );

// wxWindow* DoPythonStuff(wxWindow* parent); // declaration

IMPLEMENT_APP( EDA_APP )


/* MacOSX: Needed for file association
 * http://wiki.wxwidgets.org/WxMac-specific_topics
 */
void EDA_APP::MacOpenFile( const wxString& aFileName )
{
    PCB_EDIT_FRAME* frame    = ( (PCB_EDIT_FRAME*) GetTopWindow() );
    wxFileName      filename = aFileName;

    if( !filename.FileExists() )
        return;

    frame->LoadOnePcbFile( aFileName, false );
}


bool EDA_APP::OnInit()
{
    wxFileName      fn;
    PCB_EDIT_FRAME* frame = NULL;
    wxString        msg;

    InitEDA_Appl( wxT( "Pcbnew" ), APP_PCBNEW_T );

#ifdef KICAD_SCRIPTING
    msg.Empty();
#ifdef __WINDOWS__
    // force python environment under Windows:
    const wxString python_us("python27_us");

    // Build our python path inside kicad
    wxString kipython = m_BinDir + python_us;

    // If our python install is existing inside kicad, use it
    if( wxDirExists( kipython ) )
    {
        wxString ppath;
        if( !wxGetEnv( wxT( "PYTHONPATH" ), &ppath ) || !ppath.Contains( python_us ) )
        {
            ppath << kipython << wxT("/pylib;");
            ppath << kipython << wxT("/lib;");
            ppath << kipython << wxT("/dll");
            wxSetEnv( wxT( "PYTHONPATH" ), ppath );
            DBG( std::cout << "set PYTHONPATH to "  << TO_UTF8(ppath) << "\n"; )

            // Add python executable path:
            wxGetEnv( wxT( "PATH" ), &ppath );
            if( !ppath.Contains( python_us ) )
            {
                kipython << wxT(";") << ppath;
                wxSetEnv( wxT( "PATH" ), kipython );
                DBG( std::cout << "set PATH to " << TO_UTF8(kipython) << "\n"; )
            }
        }
    }

    // TODO: make this path definable by the user, and set more than one path
    // (and remove the fixed paths from <src>/scripting/kicadplugins.i)

    // wizard plugins are stored in kicad/bin/plugins.
    // so add this path to python scripting defualt search paths
    // which are ( [KICAD_PATH] is an environment variable to define)
    // [KICAD_PATH]/scripting/plugins
    // Add this default search path:
    msg = wxGetApp().GetExecutablePath() + wxT("scripting/plugins");
#else
    // Add this default search path:
    msg = wxT("/usr/local/kicad/bin/scripting/plugins");

#ifdef  __WXMAC__
    // OSX
    // System Library first
    // User Library then
    // (TODO) Bundle package ? where to place ? Shared Support ?
    msg = wxT("/Library/Application Support/kicad/scripting");
    msg = wxString( wxGetenv("HOME") ) + wxT("/Library/Application Support/kicad/scripting");

    // Get pcbnew.app/Contents directory
    wxFileName bundledir( wxStandardPaths::Get().GetExecutablePath() ) ;
    bundledir.RemoveLastDir();

    // Prepend in PYTHONPATH the content of the bundle libraries !
    wxSetEnv("PYTHONPATH",((wxGetenv("PYTHONPATH") != NULL ) ? (wxString(wxGetenv("PYTHONPATH")) + ":") : wxString("")) + 
                           "/Library/Application Support/kicad/scripting" + ":" +
                            bundledir.GetPath() + "/PlugIns" + ":" +
                            wxString( wxGetenv("HOME") )  + "/Library/Application Support/kicad/scripting" +
                            bundledir.GetPath() +
                            "/Frameworks/wxPython/lib/python2.6/site-packages/wx-3.0-osx_cocoa" 
                          );
#endif
#endif
    // On linux and osx, 2 others paths are
    // [HOME]/.kicad_plugins/
    // [HOME]/.kicad/scripting/plugins/
    if ( !pcbnewInitPythonScripting( TO_UTF8(msg) ) )
    {
        wxMessageBox( wxT( "pcbnewInitPythonScripting() fails" ) );
        return false;
    }
#endif

    if( argc > 1 )
    {
        fn = argv[1];

        // Be sure the filename is absolute, to avoid issues
        // when the filename is relative,
        // for instance when stored in history list without path,
        // and when building the config filename ( which should have a path )
        if( fn.IsRelative() )
            fn.MakeAbsolute();

        if( fn.GetExt() != PcbFileExtension && fn.GetExt() != LegacyPcbFileExtension )
        {
            msg.Printf( _( "Pcbnew file <%s> has a wrong extension.\n"
                           "Changing extension to .%s." ),
                        GetChars( fn.GetFullPath() ),
                        GetChars( PcbFileExtension ) );
            fn.SetExt( PcbFileExtension );
            wxMessageBox( msg );
        }

        if( !wxGetApp().LockFile( fn.GetFullPath() ) )
        {
            DisplayError( NULL, _( "This file is already open." ) );
            return false;
        }
    }

    if( m_Checker && m_Checker->IsAnotherRunning() )
    {
        if( !IsOK( NULL, _( "Pcbnew is already running, Continue?" ) ) )
            return false;
    }

    // read current setup and reopen last directory if no filename to open in command line
    bool reopenLastUsedDirectory = argc == 1;
    GetSettings( reopenLastUsedDirectory );

    if( fn.IsOk() && fn.DirExists() )
        wxSetWorkingDirectory( fn.GetPath() );

    g_DrawBgColor = BLACK;

    /* Must be called before creating the main frame in order to
     * display the real hotkeys in menus or tool tips */
    ReadHotkeyConfig( wxT( "PcbFrame" ), g_Board_Editor_Hokeys_Descr );

    // Set any environment variables before loading FP_LIB_TABLE
    SetFootprintLibTablePath();

    // Set 3D shape path from environment variable KISYS3DMOD
    Set3DShapesPath( wxT(KISYS3DMOD) );

    frame = new PCB_EDIT_FRAME( NULL, wxT( "Pcbnew" ), wxPoint( 0, 0 ), wxSize( 600, 400 ) );

#ifdef KICAD_SCRIPTING
    ScriptingSetPcbEditFrame(frame); /* give the scripting helpers access to our frame */
#endif

    frame->UpdateTitle();

    SetTopWindow( frame );
    frame->Show( true );

    CreateServer( frame, KICAD_PCB_PORT_SERVICE_NUMBER );

    frame->Zoom_Automatique( true );

    // Load config and default values before loading a board file
    // Some will be overwritten after loading the board file
    frame->LoadProjectSettings( fn.GetFullPath() );

    /* Load file specified in the command line. */
    if( fn.IsOk() )
    {
        /* Note the first time Pcbnew is called after creating a new project
         * the board file may not exist so we load settings only.
         * However, because legacy board files are named *.brd,
         * and new files are named *.kicad_pcb,
         * for all previous projects ( before 2012, december 14 ),
         * because KiCad manager ask to load a .kicad_pcb file
         * if this file does not exist, it is certainly useful
         * to test if a legacy file is existing,
         * under the same name, and therefore if the user want to load it
         */
        bool file_exists = false;

        if( fn.FileExists() )
        {
            file_exists = true;
            frame->LoadOnePcbFile( fn.GetFullPath() );
        }
        else if( fn.GetExt() == KiCadPcbFileExtension )
        {
            // Try to find a legacy file with the same name:
            wxFileName fn_legacy = fn;
            fn_legacy.SetExt( LegacyPcbFileExtension );

            if( fn_legacy.FileExists() )
            {
                msg.Printf( _( "File <%s> does not exist.\n"
                               "However a legacy file <%s> exists.\n"
                               "Do you want to load it?\n"
                               "It will be saved under the new file format." ),
                            GetChars( fn.GetFullPath() ),
                            GetChars( fn_legacy.GetFullPath() ) );

                if( IsOK( frame, msg ) )
                {
                    file_exists = true;
                    frame->LoadOnePcbFile( fn_legacy.GetFullPath() );
                    wxString filename = fn.GetFullPath();
                    filename.Replace( WIN_STRING_DIR_SEP, UNIX_STRING_DIR_SEP );
                    frame->GetBoard()->SetFileName( filename );
                    frame->UpdateTitle();
                    frame->OnModify();  // Ready to save the board under the new format
                }
            }
        }

        if( ! file_exists )
        {
            // File does not exists: prepare an empty board
            if( ! fn.GetPath().IsEmpty() )
                wxSetWorkingDirectory( fn.GetPath() );

            frame->GetBoard()->SetFileName( fn.GetFullPath( wxPATH_UNIX ) );
            frame->UpdateTitle();
            frame->UpdateFileHistory( frame->GetBoard()->GetFileName() );
            frame->OnModify();          // Ready to save the new empty board

            msg.Printf( _( "File <%s> does not exist.\nThis is normal for a new project" ),
                        GetChars( frame->GetBoard()->GetFileName() ) );
            wxMessageBox( msg );
        }
    }

    else
        // No file to open: initialize a new empty board
        // using default values for design settings:
        frame->Clear_Pcb( false );

    // update the layer names in the listbox
    frame->ReCreateLayerBox( false );

    /* For an obscure reason the focus is lost after loading a board file
     * when starting (i.e. only at this point)
     * (seems due to the recreation of the layer manager after loading the file)
     * give focus to main window and Drawpanel
     * must be done for these 2 windows (for an obscure reason ...)
     * Linux specific
     * This is more a workaround than a fix.
     */
    frame->SetFocus();
    frame->GetCanvas()->SetFocus();

    return true;
}


#if 0
// for some reason KiCad classes do not implement OnExit
// if I add it in the declaration, I need to fix it in every application
// so for now make a note TODO TODO
// we need to clean up python when the application exits
int EDA_APP::OnExit()
{
    // Restore the thread state and tell Python to cleanup after itself.
    // wxPython will do its own cleanup as part of that process.  This is done
    // in OnExit instead of ~MyApp because OnExit is only called if OnInit is
    // successful.
#if KICAD_SCRIPTING_WXPYTHON
    pcbnewFinishPythonScripting();
#endif
    return 0;
}

#endif
