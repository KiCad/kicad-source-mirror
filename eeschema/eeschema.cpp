/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2004 Jean-Pierre Charras, jaen-pierre.charras@gipsa-lab.inpg.com
 * Copyright (C) 2008-2011 Wayne Stambaugh <stambaughw@verizon.net>
 * Copyright (C) 2004-2011 KiCad Developers, see change_log.txt for contributors.
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
 * @file eeschema.cpp
 */

#include <fctsys.h>
#include <appl_wxstruct.h>
#include <class_drawpanel.h>
#include <confirm.h>
#include <gestfich.h>
#include <eda_dde.h>
#include <id.h>
#include <wxEeschemaStruct.h>
#include <eda_text.h>

#include <general.h>
#include <protos.h>
#include <hotkeys.h>
#include <dialogs/dialog_color_config.h>
#include <transform.h>
#include <wildcards_and_files_ext.h>

#include <wx/snglinst.h>


// Global variables
bool      g_OptNetListUseNames; /* true to use names rather than net
                                 * The numbers (PSPICE netlist only) */
wxSize    g_RepeatStep;
int       g_RepeatDeltaLabel;

bool                     g_HVLines = true;      // Bool: force H or V
                                                // directions (Wires, Bus ..)

int g_DefaultTextLabelSize = DEFAULT_SIZE_TEXT;

HPGL_Pen_Descr_Struct g_HPGL_Pen_Descr;

SCH_SHEET*     g_RootSheet = NULL;

wxString       g_NetCmpExtBuffer( wxT( "cmp" ) );

const wxString SymbolFileExtension( wxT( "sym" ) );
const wxString CompLibFileExtension( wxT( "lib" ) );
const wxString g_SchematicBackupFileExtension( wxT( "bak" ) );

const wxString SymbolFileWildcard( wxT( "KiCad drawing symbol file (*.sym)|*.sym" ) );
const wxString CompLibFileWildcard( wxT( "KiCad component library file (*.lib)|*.lib" ) );

LayerStruct g_LayerDescr;            /* layer colors. */

int         g_DrawDefaultLineThickness = 6; /* Default line thickness in
                                             * Eeschema units used to
                                             * draw/plot items having a
                                             * default thickness line value
                                             * (i.e. = 0 ). 0 = single pixel
                                             * line width */

// Color to draw selected items
int g_ItemSelectetColor = BROWN;

// Color to draw items flagged invisible, in libedit (they are invisible
// in Eeschema
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

    InitEDA_Appl( wxT( "Eeschema" ), APP_EESCHEMA_T );

    if( m_Checker && m_Checker->IsAnotherRunning() )
    {
        if( !IsOK( NULL, _( "Eeschema is already running, Continue?" ) ) )
            return false;
    }

    if( argc > 1 )
        filename = argv[1];

    // Init Eeschema
    SeedLayers();

    // read current setup and reopen last directory if no filename to open in
    // command line
    bool reopenLastUsedDirectory = argc == 1;
    GetSettings( reopenLastUsedDirectory );

   /* Must be called before creating the main frame in order to
    * display the real hotkeys in menus or tool tips */
    ReadHotkeyConfig( wxT("SchematicFrame"), s_Eeschema_Hokeys_Descr );

    // Create main frame (schematic frame) :
    frame = new SCH_EDIT_FRAME( NULL, wxT( "Eeschema" ), wxPoint( 0, 0 ), wxSize( 600, 400 ) );

    SetTopWindow( frame );
    frame->Show( true );

    if( CreateServer( frame, KICAD_SCH_PORT_SERVICE_NUMBER ) )
    {
        // RemoteCommand is in controle.cpp and is called when Pcbnew sends Eeschema a command.
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
            frame->GetCanvas()->Refresh( true );
    }
    else
    {
        // Read a default config file if no file to load.
        frame->LoadProjectFile( wxEmptyString, true );
        frame->GetCanvas()->Refresh( true );
    }

    return true;
}
