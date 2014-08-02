/**
 * @file gerbview.cpp
 * @brief GERBVIEW main file.
 */

/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 1992-2012 KiCad Developers, see change_log.txt for contributors.
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
#include <pgm_base.h>
#include <class_drawpanel.h>
#include <confirm.h>
#include <gestfich.h>

#include <gerbview.h>
#include <gerbview_id.h>
#include <hotkeys.h>
#include <gerbview_frame.h>

#include <build_version.h>

#include <wx/file.h>
#include <wx/snglinst.h>

// Colors for layers and items
COLORS_DESIGN_SETTINGS g_ColorsSettings;
int g_Default_GERBER_Format;


const wxChar* g_GerberPageSizeList[] = {
    wxT( "GERBER" ),    // index 0: full size page selection, and do not show page limits
    wxT( "GERBER" ),    // index 1: full size page selection, and show page limits
    wxT( "A4" ),
    wxT( "A3" ),
    wxT( "A2" ),
    wxT( "A" ),
    wxT( "B" ),
    wxT( "C" ),
};


GERBER_IMAGE*  g_GERBER_List[32];


namespace GERBV {

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
        case FRAME_GERBER:
            {
                GERBVIEW_FRAME* frame = new GERBVIEW_FRAME( aKiway, aParent );
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

} kiface( "gerbview", KIWAY::FACE_GERBVIEW );

} // namespace

using namespace GERBV;

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
    ReadHotkeyConfig( wxT("GerberFrame"), s_Gerbview_Hokeys_Descr );

    return true;
}


void IFACE::OnKifaceEnd()
{
    end_common();
}
