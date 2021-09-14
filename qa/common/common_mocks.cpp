/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2018 KiCad Developers, see AUTHORS.txt for contributors.
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
 * @file common_mocks.cpp
 * @brief Mock objects for libcommon unit tests
 */

#include <pgm_base.h>
#include <kiface_base.h>


struct PGM_TEST_FRAME : public PGM_BASE
{
    void MacOpenFile( const wxString& aFileName ) override
    {}
};

PGM_BASE& Pgm()
{
    static PGM_TEST_FRAME program;
    return program;
}

static struct IFACE : public KIFACE_BASE
{
    bool OnKifaceStart( PGM_BASE* aProgram, int aCtlBits ) override
    {
        return start_common( aCtlBits );
    }

    wxWindow* CreateWindow( wxWindow* aParent, int aClassId, KIWAY* aKiway, int aCtlBits = 0 ) override
    {
        return nullptr;
    }

    void* IfaceOrAddress( int aDataId ) override
    {
        return nullptr;
    }

    IFACE( const char* aDSOname, KIWAY::FACE_T aType ) :
            KIFACE_BASE( aDSOname, aType )
    {}

} kiface( "common_test", KIWAY::KIWAY_FACE_COUNT );

KIFACE_BASE& Kiface()
{
    return kiface;
}