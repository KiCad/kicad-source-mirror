/**
 * @file pl_editor.cpp
 * @brief page layout editor main file.
 */

/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013 CERN
 * @author Jean-Pierre Charras, jp.charras at wanadoo.fr
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

#include <fctsys.h>
#include <kiface_i.h>
#include <confirm.h>
#include <gestfich.h>
#include <worksheet_shape_builder.h>
#include <pl_editor_frame.h>
#include <hotkeys.h>

#include <build_version.h>

#include <wx/file.h>
#include <wx/snglinst.h>


namespace PGE {

static struct IFACE : public KIFACE_I
{
    // Of course all are virtual overloads, implementations of the KIFACE.

    IFACE( const char* aName, KIWAY::FACE_T aType ) :
        KIFACE_I( aName, aType )
    {}

    bool OnKifaceStart( PGM_BASE* aProgram, int aCtlBits );

    void OnKifaceEnd();

    wxWindow* CreateWindow( wxWindow* aParent, int aClassId, KIWAY* aKiway, int aCtlBits = 0 )
    {
        switch( aClassId )
        {
        case FRAME_PL_EDITOR:
            {
                PL_EDITOR_FRAME* frame = new PL_EDITOR_FRAME( aKiway, aParent );
                return frame;
            }
            break;

        default:
            ;
        }

        return NULL;
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

} kiface( "pl_editor", KIWAY::FACE_PL_EDITOR );

} // namespace

using namespace PGE;

static PGM_BASE* process;

KIFACE_I& Kiface() { return kiface; }


// KIFACE_GETTER's actual spelling is a substitution macro found in kiway.h.
// KIFACE_GETTER will not have name mangling due to declaration in kiway.h.
MY_API( KIFACE* ) KIFACE_GETTER(  int* aKIFACEversion, int aKiwayVersion, PGM_BASE* aProgram )
{
    process = (PGM_BASE*) aProgram;
    return &kiface;
}


PGM_BASE& Pgm()
{
    wxASSERT( process );    // KIFACE_GETTER has already been called.
    return *process;
}


bool IFACE::OnKifaceStart( PGM_BASE* aProgram, int aCtlBits )
{
    start_common( aCtlBits );

    // Must be called before creating the main frame in order to
    // display the real hotkeys in menus or tool tips
    ReadHotkeyConfig( PL_EDITOR_FRAME_NAME, PlEditorHokeysDescr );

    g_UserUnit = MILLIMETRES;

    return true;
}


void IFACE::OnKifaceEnd()
{
    end_common();
}


#if 0
bool MYFACE::OnKifaceStart( PGM_BASE* aProgram )

{
    wxFileName          fn;

    InitEDA_Appl( wxT( "pl_editor" ), APP_PL_EDITOR_T );

    if( m_Checker && m_Checker->IsAnotherRunning() )
    {
        if( !IsOK( NULL, _( "pl_editor is already running. Continue?" ) ) )
            return false;
    }

    g_UserUnit = MILLIMETRES;

    // read current setup and reopen last directory if no filename to open in
    // command line
    bool reopenLastUsedDirectory = argc == 1;
    GetSettings( reopenLastUsedDirectory );

    // Must be called before creating the main frame in order to
    // display the real hotkeys in menus or tool tips
    ReadHotkeyConfig( PL_EDITOR_FRAME_NAME, s_PlEditor_Hokeys_Descr );

    PL_EDITOR_FRAME * frame = new PL_EDITOR_FRAME( NULL, wxT( "PlEditorFrame" ), wxPoint( 0, 0 ), wxSize( 600, 400 ) );

    // frame title:
    frame->SetTitle( GetTitle() + wxT( " " ) + GetBuildVersion() );

    SetTopWindow( frame );
    frame->Show( true );
    frame->Zoom_Automatique( true );        // Zoom fit in frame
    frame->GetScreen()->m_FirstRedraw = false;


    bool descrLoaded = false;
    if( argc > 1 )
    {
        fn = argv[1];

        if( fn.IsOk() )
        {
            bool success = frame->LoadPageLayoutDescrFile( fn.GetFullPath() );
            if( !success )
            {
                wxString msg;
                msg.Printf( _("Error when loading file <%s>"),
                            fn.GetFullPath().GetData() );
                wxMessageBox( msg );
            }
            else
            {
                descrLoaded = true;
                frame->OnNewPageLayout();
            }
        }
    }

    if( !descrLoaded )
    {
        WORKSHEET_LAYOUT::GetTheInstance().SetPageLayout();
        frame->OnNewPageLayout();
    }

    return true;
}
#endif
