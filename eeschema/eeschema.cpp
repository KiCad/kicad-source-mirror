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
 * @brief the main file
 */

#include <fctsys.h>
#include <appl_wxstruct.h>
#include <class_drawpanel.h>
#include <confirm.h>
#include <gestfich.h>
#include <eda_dde.h>
#include <wxEeschemaStruct.h>
#include <eda_text.h>

#include <general.h>
#include <class_libentry.h>
//#include <sch_junction.h>
#include <hotkeys.h>
#include <dialogs/dialog_color_config.h>
#include <transform.h>
#include <wildcards_and_files_ext.h>

#include <wx/snglinst.h>


#if defined( USE_KIWAY_DLLS )

#include <kiway.h>
#include <import_export.h>

static struct SCH_FACE : public KIFACE
{
    wxWindow* CreateWindow( int aClassId, KIWAY* aKIWAY, int aCtlBits = 0 )
    {
        switch( aClassId )
        {
        default:
            return new SCH_EDIT_FRAME( NULL, wxT( "Eeschema" ),
                wxPoint( 0, 0 ), wxSize( 600, 400 ) );
        }
    }

    /**
     * Function IfaceOrAddress
     * return a pointer to the requested object.  The safest way to use this
     * is to retrieve a pointer to a static instance of an interface, similar to
     * how the KIFACE interface is exported.  But if you know what you are doing
     * use it to retrieve anything you want.
     *
     * @param aDataId identifies which object you want the address of.
     *
     * @return void* - and must be cast into the know type.
     */
    void* IfaceOrAddress( int aDataId )
    {
        return NULL;
    }

} kiface;

static EDA_APP* process;

// KIFACE_GETTER's actual spelling is a substitution macro found in kiway.h.
// KIFACE_GETTER will not have name mangling due to declaration in kiway.h.
MY_API( KIFACE* ) KIFACE_GETTER(  int* aKIFACEversion, int aKIWAYversion, wxApp* aProcess )
{
    process = (EDA_APP*) aProcess;
    return &kiface;
}


EDA_APP& wxGetApp()
{
    wxASSERT( process );    // KIFACE_GETTER has already been called.
    return *process;
}

#else

// Create a new application object: this macro will allow wxWindows to create
// the application object during program execution (it's better than using a
// static object for many reasons) and also declares the accessor function
// wxGetApp() which will return the reference of the right type (i.e. MyApp and
// not wxApp)
IMPLEMENT_APP( EDA_APP )

#endif


// Global variables
wxSize  g_RepeatStep;
int     g_RepeatDeltaLabel;
int     g_DefaultBusWidth;
SCH_SHEET*  g_RootSheet = NULL;

TRANSFORM DefaultTransform = TRANSFORM( 1, 0, 0, -1 );


/************************************/
/* Called to initialize the program */
/************************************/

/* MacOSX: Needed for file association
 * http://wiki.wxwidgets.org/WxMac-specific_topics
 */
void EDA_APP::MacOpenFile( const wxString& aFileName )
{
    wxFileName      filename = aFileName;
    SCH_EDIT_FRAME* frame = ((SCH_EDIT_FRAME*) GetTopWindow());

    if( !frame )
        return;

    if( !filename.FileExists() )
        return;

    frame->LoadOneEEProject( aFileName, false );
}


bool EDA_APP::OnInit()
{
    wxFileName      filename;
    SCH_EDIT_FRAME* frame = NULL;
    bool fileReady = false;

    InitEDA_Appl( wxT( "Eeschema" ), APP_EESCHEMA_T );

    if( argc > 1 )
        filename = argv[1];

    if( filename.IsOk() )
    {
        if( filename.GetExt() != SchematicFileExtension )
            filename.SetExt( SchematicFileExtension );

        if( !wxGetApp().LockFile( filename.GetFullPath() ) )
        {
            DisplayError( NULL, _( "This file is already open." ) );
            return false;
        }

        fileReady = true;
    }

    if( m_Checker && m_Checker->IsAnotherRunning() )
    {
        if( !IsOK( NULL, _( "Eeschema is already running, Continue?" ) ) )
            return false;
    }

    // Give a default colour for all layers
    // (actual color will be initialized by config)
    for( int ii = 0; ii < NB_SCH_LAYERS; ii++ )
        SetLayerColor( DARKGRAY, ii );

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

    CreateServer( frame, KICAD_SCH_PORT_SERVICE_NUMBER );

    frame->Zoom_Automatique( true );

    // Load file specified in the command line:
    if( fileReady )
    {
        if( !filename.GetPath().IsEmpty() )
            // wxSetWorkingDirectory does not like empty paths
            wxSetWorkingDirectory( filename.GetPath() );

        if( frame->LoadOneEEProject( filename.GetFullName(), false ) )
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
