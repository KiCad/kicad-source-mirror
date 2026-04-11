/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
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

#pragma once
#include <qa_utils/wx_utils/unit_test_utils.h>
#include <kiface_base.h>
#include <config.h>

// Suppress a warning that the mock methods don't override the base class methods because
// turtlemocks doesn't seem to provide a way to actually annotate the methods with override.
#ifdef HAVE_WINCONSISTENT_MISSING_OVERRIDE
    _Pragma( "GCC diagnostic push" ) \
    _Pragma( "GCC diagnostic ignored \"-Winconsistent-missing-override\"" )
#endif

MOCK_BASE_CLASS( MOCK_KIFACE_BASE, KIFACE_BASE )
{
    MOCK_KIFACE_BASE() : KIFACE_BASE( "common_test", KIWAY::KIWAY_FACE_COUNT ) {};
    virtual ~MOCK_KIFACE_BASE() {};

    MOCK_METHOD( OnKifaceStart, 3, bool( PGM_BASE*, int, KIWAY* ) );
    MOCK_METHOD( OnKifaceEnd, 0, void() );
    MOCK_METHOD( CreateKiWindow, 4, wxWindow*( wxWindow*, int, KIWAY*, int ) );
    MOCK_METHOD( IfaceOrAddress, 1, void*( int ) );
};

#ifdef HAVE_WINCONSISTENT_MISSING_OVERRIDE
    _Pragma( "GCC diagnostic pop" )
#endif
