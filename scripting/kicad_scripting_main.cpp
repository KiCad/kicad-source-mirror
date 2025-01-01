/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <eda_dde.h>
#include <kiface_base.h>
#include <kiface_ids.h>
#include <kipython_frame.h>
#include <kipython_settings.h>
#include <kiway.h>
#include <pgm_base.h>
#include <python_scripting.h>
#include <settings/settings_manager.h>



//-----<KIFACE>-----------------------------------------------------------------

namespace KIPYTHON {

static struct IFACE : public KIFACE_BASE
{
    bool OnKifaceStart( PGM_BASE* aProgram, int aCtlBits, KIWAY* aKiway ) override;

    void OnKifaceEnd() override;

    wxWindow* CreateKiWindow( wxWindow* aParent, int aClassId, KIWAY* aKiway,
                              int aCtlBits = 0 ) override
    {
        KIPYTHON_FRAME* frame = new KIPYTHON_FRAME( aKiway, aParent );

        if( Kiface().IsSingle() )
        {
            // only run this under single_top, not under a project manager.
            frame->CreateServer( KICAD_PY_PORT_SERVICE_NUMBER );
        }

        return frame;
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
    void* IfaceOrAddress( int aDataId ) override
    {
        return NULL;
    }

    IFACE( const char* aDSOname, KIWAY::FACE_T aType ) :
            KIFACE_BASE( aDSOname, aType )
    {}

} kiface( "KIPYTHON", KIWAY::FACE_PYTHON );

}   // namespace KIPYTHON

using namespace KIPYTHON;

KIFACE_BASE& Kiface()
{
    return kiface;
}


// KIFACE_GETTER's actual spelling is a substitution macro found in kiway.h.
// KIFACE_GETTER will not have name mangling due to declaration in kiway.h.
KIFACE* KIFACE_GETTER( int* aKIFACEversion, int aKIWAYversion, PGM_BASE* aProgram )
{
    return &kiface;
}


bool IFACE::OnKifaceStart( PGM_BASE* aProgram, int aCtlBits, KIWAY* aKiway )
{
    InitSettings( new KIPYTHON_SETTINGS );
    Pgm().GetSettingsManager().RegisterSettings( KifaceSettings() );

    return start_common( aCtlBits );
}


void IFACE::OnKifaceEnd()
{
    end_common();
}
