/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 1992-2010 jean-pierre.charras
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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

#include <bitmap2cmp_frame.h>
#include <bitmap2cmp_settings.h>
#include <kiface_base.h>
#include <kiway.h>
#include <pgm_base.h>
#include <settings/settings_manager.h>


namespace BMP2CMP {

static struct IFACE : public KIFACE_BASE
{
    bool OnKifaceStart( PGM_BASE* aProgram, int aCtlBits, KIWAY* aKiway ) override;

    wxWindow* CreateKiWindow( wxWindow* aParent, int aClassId, KIWAY* aKiway,
                              int aCtlBits = 0 ) override
    {
        InitSettings( new BITMAP2CMP_SETTINGS );
        Pgm().GetSettingsManager().RegisterSettings( KifaceSettings() );
        return new BITMAP2CMP_FRAME( aKiway, aParent );
    }

    /**
     * Return a pointer to the requested object.
     *
     * The safest way to use this is to retrieve a pointer to a static instance of an interface,
     * similar to how the KIFACE interface is exported.  But if you know what you are doing use
     * it to retrieve anything you want.
     *
     * @param aDataId identifies which object you want the address of.
     * @return the object which must be cast into the known type.
     */
    void* IfaceOrAddress( int aDataId ) override
    {
        return nullptr;
    }

    IFACE( const char* aDSOname, KIWAY::FACE_T aType ) :
            KIFACE_BASE( aDSOname, aType )
    {}

} kiface( "BMP2CMP", KIWAY::FACE_BMP2CMP );

}   // namespace BMP2CMP

using namespace BMP2CMP;


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
    return start_common( aCtlBits );
}
