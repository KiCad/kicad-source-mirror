/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2007 Jean-Pierre Charras, jp..charras at wanadoo.fr
 * Copyright (C) 2014 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <footprint.h>
#include <footprint_library_adapter.h>
#include <kiface_base.h>
#include <libraries/library_manager.h>
#include <pad.h>
#include <pgm_base.h>
#include <settings/settings_manager.h>

#include <cvpcb_mainframe.h>
#include <settings/cvpcb_settings.h>
#include <display_footprints_frame.h>
#include <kiface_ids.h>

#include <memory>
#include <optional>
#include <set>
#include <project_pcb.h>

namespace CV {

int testFootprintLink( const wxString& aFootprint, PROJECT* aProject )
{
    FOOTPRINT_LIBRARY_ADAPTER* adapter = PROJECT_PCB::FootprintLibAdapter( aProject );
    LIB_ID                  fpID;

    fpID.Parse( aFootprint );

    wxString libName = fpID.GetLibNickname();
    wxString fpName = fpID.GetLibItemName();

    // TODO(JE) this is a bit of a hack; CLI lazy-loading of libraries should be more unified
    // Libraries may not be present at this point if running in kicad-cli because nothings will
    // have triggered a load up to this point
    if( !adapter->GetLibraryStatus( libName ) )
        adapter->LoadOne( libName );

    if( !adapter->HasLibrary( libName, false ) )
        return KIFACE_TEST_FOOTPRINT_LINK_NO_LIBRARY;
    else if( !adapter->HasLibrary( libName, true ) )
        return KIFACE_TEST_FOOTPRINT_LINK_LIBRARY_NOT_ENABLED;
    else if( !adapter->FootprintExists( libName, fpName ) )
        return KIFACE_TEST_FOOTPRINT_LINK_NO_FOOTPRINT;

    return 0;
}


void getFootprintPadNumbers( const wxString& aFootprint, PROJECT* aProject, std::set<wxString>& aPadNumbers )
{
    LIB_ID fpID;

    if( fpID.Parse( aFootprint ) >= 0 )
        return;

    if( std::optional<LIBRARY_MANAGER_ADAPTER*> mgr =
                Pgm().GetLibraryManager().Adapter( LIBRARY_TABLE_TYPE::FOOTPRINT ) )
    {
        ( *mgr )->BlockUntilLoaded();
    }

    FOOTPRINT_LIBRARY_ADAPTER* adapter = PROJECT_PCB::FootprintLibAdapter( aProject );

    if( !adapter )
        return;

    adapter->LoadOne( fpID.GetLibNickname() );

    std::unique_ptr<FOOTPRINT> footprint;

    // This runs across the kiface boundary, so let nothing escape into the caller's ERC loop.
    try
    {
        footprint.reset( adapter->LoadFootprint( fpID, false ) );
    }
    catch( ... )
    {
    }

    if( !footprint )
        return;

    for( const PAD* pad : footprint->Pads() )
    {
        if( !pad->GetNumber().IsEmpty() )
            aPadNumbers.insert( pad->GetNumber() );
    }
}


static struct IFACE : public KIFACE_BASE
{
    // Of course all are virtual overloads, implementations of the KIFACE.

    IFACE( const char* aName, KIWAY::FACE_T aType ) :
            KIFACE_BASE( aName, aType )
    {}

    bool OnKifaceStart( PGM_BASE* aProgram, int aCtlBits, KIWAY* aKiway ) override;

    void OnKifaceEnd() override;

    wxWindow* CreateKiWindow( wxWindow* aParent, int aClassId, KIWAY* aKiway,
                              int aCtlBits = 0 ) override
    {
        switch( aClassId )
        {
        case FRAME_CVPCB:         return new CVPCB_MAINFRAME( aKiway, aParent );
        case FRAME_CVPCB_DISPLAY: return new DISPLAY_FOOTPRINTS_FRAME( aKiway, aParent );
        default:                  return nullptr;
        }
    }

    /**
     * Return a pointer to the requested object.
     *
     * The safest way to use this is to retrieve a pointer to a static instance of an interface,
     * similar to how the KIFACE interface is exported.  But if you know what you are doing use
     * it to retrieve anything you want.
     *
     * @param aDataId identifies which object you want the address of.
     * @return the object requested and must be cast into the known type.
     */
    void* IfaceOrAddress( int aDataId ) override
    {
        switch( aDataId )
        {
        case KIFACE_TEST_FOOTPRINT_LINK:
            return (void*) testFootprintLink;

        case KIFACE_FOOTPRINT_PAD_NUMBERS: return (void*) getFootprintPadNumbers;

        default:
            return nullptr;
        }
    }

} kiface( "cvpcb", KIWAY::FACE_CVPCB );

} // namespace

using namespace CV;


KIFACE_BASE& Kiface() { return kiface; }


// KIFACE_GETTER's actual spelling is a substitution macro found in kiway.h.
// KIFACE_GETTER will not have name mangling due to declaration in kiway.h.
KIFACE_API KIFACE* KIFACE_GETTER(  int* aKIFACEversion, int aKIWAYversion, PGM_BASE* aProgram )
{
    return &kiface;
}


//!!!!!!!!!!!!!!! This code is obsolete because of the merge into Pcbnew, don't bother with it.

// A short lived implementation.  cvpcb will get combine into Pcbnew shortly, so
// we skip setting KICAD7_FOOTPRINT_DIR here for now.  User should set the environment
// variable.
bool IFACE::OnKifaceStart( PGM_BASE* aProgram, int aCtlBits, KIWAY* aKiway )
{
    // This is process level, not project level, initialization of the DSO.

    // Do nothing in here pertinent to a project!

    InitSettings( new CVPCB_SETTINGS );
    aProgram->GetSettingsManager().RegisterSettings( KifaceSettings() );

    start_common( aCtlBits );

    return true;
}


void IFACE::OnKifaceEnd()
{
    end_common();
}
