/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2021 Kicad Developers, see AUTHORS.txt for contributors.
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

#include <kiface_i.h>
#include <kiway.h>
#include <pgm_base.h>
#include <settings/settings_manager.h>


//-----<KIFACE>-----------------------------------------------------------------

namespace KIPYTHON {

static struct IFACE : public KIFACE_I
{
    bool OnKifaceStart( PGM_BASE* aProgram, int aCtlBits ) override;

    wxWindow* CreateWindow( wxWindow* aParent, int aClassId, KIWAY* aKiway, int aCtlBits = 0 ) override
    {
        InitSettings( new BITMAP2CMP_SETTINGS );
        Pgm().GetSettingsManager().RegisterSettings( KifaceSettings() );
        return new BM2CMP_FRAME( aKiway, aParent );
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
        KIFACE_I( aDSOname, aType )
    {}

} kiface( "BMP2CMP", KIWAY::FACE_PYTHON );

}   // namespace KIPYTHON

using namespace KIPYTHON;

static PGM_BASE* process;

KIFACE_I& Kiface()
{
    return kiface;
}


// KIFACE_GETTER's actual spelling is a substitution macro found in kiway.h.
// KIFACE_GETTER will not have name mangling due to declaration in kiway.h.
KIFACE* KIFACE_GETTER( int* aKIFACEversion, int aKIWAYversion, PGM_BASE* aProgram )
{
    process = (PGM_BASE*) aProgram;
    return &kiface;
}


#if defined(BUILD_KIWAY_DLLS)
PGM_BASE& Pgm()
{
    wxASSERT( process );    // KIFACE_GETTER has already been called.
    return *process;
}
#endif


bool IFACE::OnKifaceStart( PGM_BASE* aProgram, int aCtlBits )
{
    return start_common( aCtlBits );
}
