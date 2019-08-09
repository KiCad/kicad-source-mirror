/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2007 Jean-Pierre Charras, jp..charras at wanadoo.fr
 * Copyright (C) 2014 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 1992-2014 KiCad Developers, see AUTHORS.txt for contributors.
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
 * @file cvpcb.cpp
 */

#include <confirm.h>
#include <fp_lib_table.h>
#include <kiface_i.h>
#include <pgm_base.h>

#include <cvpcb_mainframe.h>
#include <display_footprints_frame.h>


namespace CV {

static struct IFACE : public KIFACE_I
{
    // Of course all are virtual overloads, implementations of the KIFACE.

    IFACE( const char* aName, KIWAY::FACE_T aType ) :
        KIFACE_I( aName, aType )
    {}

    bool OnKifaceStart( PGM_BASE* aProgram, int aCtlBits ) override;

    void OnKifaceEnd() override;

    wxWindow* CreateWindow( wxWindow* aParent, int aClassId, KIWAY* aKiway, int aCtlBits = 0 ) override
    {
        switch( aClassId )
        {
        case FRAME_CVPCB:
            return new CVPCB_MAINFRAME( aKiway, aParent );

        case FRAME_CVPCB_DISPLAY:
            return new DISPLAY_FOOTPRINTS_FRAME( aKiway, aParent );

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
    void* IfaceOrAddress( int aDataId ) override
    {
        return NULL;
    }

} kiface( "cvpcb", KIWAY::FACE_CVPCB );

} // namespace

using namespace CV;


static PGM_BASE* process;


KIFACE_I& Kiface() { return kiface; }


// KIFACE_GETTER's actual spelling is a substitution macro found in kiway.h.
// KIFACE_GETTER will not have name mangling due to declaration in kiway.h.
MY_API( KIFACE* ) KIFACE_GETTER(  int* aKIFACEversion, int aKIWAYversion, PGM_BASE* aProgram )
{
    process = (PGM_BASE*) aProgram;
    return &kiface;
}


PGM_BASE& Pgm()
{
    wxASSERT( process );    // KIFACE_GETTER has already been called.
    return *process;
}


//!!!!!!!!!!!!!!! This code is obsolete because of the merge into pcbnew, don't bother with it.

FP_LIB_TABLE GFootprintTable;


// A short lived implementation.  cvpcb will get combine into pcbnew shortly, so
// we skip setting KISYSMOD here for now.  User should set the environment
// variable.

bool IFACE::OnKifaceStart( PGM_BASE* aProgram, int aCtlBits )
{
    // This is process level, not project level, initialization of the DSO.

    // Do nothing in here pertinent to a project!

    start_common( aCtlBits );

    /*  Now that there are no *.mod files in the standard library, this function
        has no utility.  User should simply set the variable manually.
        Looking for *.mod files which do not exist is fruitless.

    // SetFootprintLibTablePath();
    */

    try
    {
        // The global table is not related to a specific project.  All projects
        // will use the same global table.  So the KIFACE::OnKifaceStart() contract
        // of avoiding anything project specific is not violated here.

        if( !FP_LIB_TABLE::LoadGlobalTable( GFootprintTable ) )
        {
            DisplayInfoMessage( NULL, _(
                "You have run CvPcb for the first time using the "
                "new footprint library table method for finding "
                "footprints.\nCvPcb has either copied the default "
                "table or created an empty table in your home "
                "folder.\nYou must first configure the library "
                "table to include all footprint libraries not "
                "included with KiCad.\nSee the \"Footprint Library "
                "Table\" section of the CvPcb documentation for "
                "more information." ) );
        }
    }
    catch( const IO_ERROR& ioe )
    {
        DisplayErrorMessage(
            nullptr,
            _( "An error occurred attempting to load the global footprint library table" ),
            ioe.What() );
        return false;
    }

    return true;
}

void IFACE::OnKifaceEnd()
{
    end_common();
}
