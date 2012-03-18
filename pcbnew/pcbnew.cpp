/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2009 Jean-Pierre Charras, jaen-pierre.charras@gipsa-lab.inpg.com
 * Copyright (C) 1992-2011 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <wx/file.h>
#include <wx/snglinst.h>

#include <pcbnew.h>
#include <protos.h>
#include <hotkeys.h>
#include <wildcards_and_files_ext.h>
#include <class_board.h>

#include <dialogs/dialog_scripting.h>
#include <python_scripting.h>
#include <pcbnew_scripting_helpers.h>

// Colors for layers and items
COLORS_DESIGN_SETTINGS g_ColorsSettings;
int g_DrawDefaultLineThickness = 60; /* Default line thickness in PCnew units used to draw
                                      * or plot items having a default thickness line value
                                      * (Frame references) (i.e. = 0 ). 0 = single pixel line
                                      * width */

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

int            Route_Layer_TOP;
int            Route_Layer_BOTTOM;
int            g_MaxLinksShowed;
int            g_MagneticPadOption   = capture_cursor_in_track_tool;
int            g_MagneticTrackOption = capture_cursor_in_track_tool;

wxPoint        g_Offset_Module;     /* Distance to offset module trace when moving. */

// Wildcard for footprint libraries filesnames
const wxString g_FootprintLibFileWildcard( _( "KiCad footprint library file (*.mod)|*.mod" ) );

/* Name of the document footprint list
 * usually located in share/modules/footprints_doc
 * this is of the responsibility to users to create this file
 * if they want to have a list of footprints
 */
wxString g_DocModulesFileName = wxT( "footprints_doc/footprints.pdf" );

wxArrayString g_LibraryNames;


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

#ifdef KICAD_SCRIPTING    
    pcbnewInitPythonScripting();
#endif    
    

    InitEDA_Appl( wxT( "Pcbnew" ), APP_PCBNEW_T );

    if( m_Checker && m_Checker->IsAnotherRunning() )
    {
        if( !IsOK( NULL, _( "Pcbnew is already running, Continue?" ) ) )
            return false;
    }

    // read current setup and reopen last directory if no filename to open in command line
    bool reopenLastUsedDirectory = argc == 1;
    GetSettings( reopenLastUsedDirectory );

    if( argc > 1 )
    {
        fn = argv[1];

        if( fn.GetExt() != PcbFileExtension )
        {
            wxLogDebug( wxT( "Pcbnew file <%s> has the wrong extension.  \
Changing extension to .brd." ), GetChars( fn.GetFullPath() ) );
            fn.SetExt( PcbFileExtension );
        }

        if( fn.IsOk() && fn.DirExists() )
            wxSetWorkingDirectory( fn.GetPath() );
    }

    g_DrawBgColor = BLACK;

    /* Must be called before creating the main frame in order to
     * display the real hotkeys in menus or tool tips */
    ReadHotkeyConfig( wxT( "PcbFrame" ), g_Board_Editor_Hokeys_Descr );

    frame = new PCB_EDIT_FRAME( NULL, wxT( "Pcbnew" ), wxPoint( 0, 0 ), wxSize( 600, 400 ) );

    #ifdef KICAD_SCRIPTING    
    ScriptingSetPcbEditFrame(frame); /* give the scripting helpers access to our frame */
    #endif    
    
    frame->UpdateTitle();

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
        /* Note the first time Pcbnew is called after creating a new project
         * the board file may not exist so we load settings only.
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
    frame->GetCanvas()->SetFocus();
    
    DIALOG_SCRIPTING* sw = new DIALOG_SCRIPTING(frame);
    sw->Show(true);
    return true;
}
