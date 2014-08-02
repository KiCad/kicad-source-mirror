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
#include <pgm_base.h>
#include <kiface_i.h>
#include <class_drawpanel.h>
#include <gestfich.h>
#include <eda_dde.h>
#include <wxEeschemaStruct.h>
#include <libeditframe.h>
#include <viewlib_frame.h>
#include <eda_text.h>

#include <general.h>
#include <class_libentry.h>
#include <hotkeys.h>
#include <dialogs/dialog_color_config.h>
#include <transform.h>
#include <wildcards_and_files_ext.h>

#include <kiway.h>


// Global variables
wxSize  g_RepeatStep;
int     g_RepeatDeltaLabel;
int     g_DefaultBusWidth;
SCH_SHEET*  g_RootSheet = NULL;

TRANSFORM DefaultTransform = TRANSFORM( 1, 0, 0, -1 );


namespace SCH {

static struct IFACE : public KIFACE_I
{
    // Of course all are virtual overloads, implementations of the KIFACE.

    IFACE( const char* aName, KIWAY::FACE_T aType ) :
        KIFACE_I( aName, aType )
    {}

    bool OnKifaceStart( PGM_BASE* aProgram, int aCtlBits );

    void OnKifaceEnd( PGM_BASE* aProgram )
    {
        end_common();
    }

    wxWindow* CreateWindow( wxWindow* aParent, int aClassId, KIWAY* aKiway, int aCtlBits = 0 )
    {
        switch( aClassId )
        {
        case FRAME_SCH:
            {
                SCH_EDIT_FRAME* frame = new SCH_EDIT_FRAME( aKiway, aParent );

                frame->Zoom_Automatique( true );

                // Read a default config file in case no project given on command line.
                frame->LoadProjectFile( wxEmptyString, true );

                if( Kiface().IsSingle() )
                {
                    // only run this under single_top, not under a project manager.
                    CreateServer( frame, KICAD_SCH_PORT_SERVICE_NUMBER );
                }
                return frame;
            }
            break;

        case FRAME_SCH_LIB_EDITOR:
            {
                LIB_EDIT_FRAME* frame = new LIB_EDIT_FRAME( aKiway, aParent );
                return frame;
            }
            break;


        case FRAME_SCH_VIEWER:
        case FRAME_SCH_VIEWER_MODAL:
            {
                LIB_VIEW_FRAME* frame = new LIB_VIEW_FRAME( aKiway, aParent, FRAME_T( aClassId ) );
                return frame;
            }
            break;

        default:
            return NULL;
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

} kiface( "eeschema", KIWAY::FACE_SCH );

} // namespace

using namespace SCH;

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
    // This is process level, not project level, initialization of the DSO.

    // Do nothing in here pertinent to a project!

    start_common( aCtlBits );

    // Give a default colour for all layers
    // (actual color will be initialized by config)
    for( int ii = 0; ii < NB_SCH_LAYERS; ii++ )
        SetLayerColor( DARKGRAY, ii );

    // Must be called before creating the main frame in order to
    // display the real hotkeys in menus or tool tips
    ReadHotkeyConfig( wxT("SchematicFrame"), s_Eeschema_Hokeys_Descr );

    return true;
}

