/*
 * This program source code file is part of KiCad, a free EDA CAD application.
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
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */
#define BOOST_TEST_NO_MAIN
#include <boost/test/unit_test.hpp>

#include <pgm_base.h>

// PGM_BASE::DesktopAppIdForProgram maps argv[0] to the freedesktop launcher id used for the
// X11 WM_CLASS and the Wayland application id.  A wrong or empty mapping is what let window
// managers mis-group KiCad windows under an unrelated application (issue 70216).

BOOST_AUTO_TEST_SUITE( DesktopAppId )

BOOST_AUTO_TEST_CASE( NonGuiProgramsHaveNoId )
{
    // Executables without an installed .desktop launcher must not claim an application id.
    BOOST_CHECK( PGM_BASE::DesktopAppIdForProgram( wxT( "pl_editor" ) ).IsEmpty() );
    BOOST_CHECK( PGM_BASE::DesktopAppIdForProgram( wxT( "kicad-cli" ) ).IsEmpty() );
    BOOST_CHECK( PGM_BASE::DesktopAppIdForProgram( wxT( "something-else" ) ).IsEmpty() );
}

// On GTK/Linux, pgm_base.cpp is compiled with the desktop-id macros, so the GUI programs
// resolve to real launcher ids.  Elsewhere the mapping is intentionally empty.
#if defined( __WXGTK__ )

BOOST_AUTO_TEST_CASE( GuiProgramsGetDistinctIds )
{
    const wxString kicad    = PGM_BASE::DesktopAppIdForProgram( wxT( "kicad" ) );
    const wxString eeschema = PGM_BASE::DesktopAppIdForProgram( wxT( "eeschema" ) );
    const wxString pcbnew   = PGM_BASE::DesktopAppIdForProgram( wxT( "pcbnew" ) );

    // Every GUI app resolves to an id, and the ids are distinct so each groups on its own
    // launcher rather than collapsing together.
    BOOST_CHECK( !kicad.IsEmpty() );
    BOOST_CHECK( !eeschema.IsEmpty() );
    BOOST_CHECK( !pcbnew.IsEmpty() );

    BOOST_CHECK_NE( kicad, eeschema );
    BOOST_CHECK_NE( eeschema, pcbnew );

#if defined( KICAD_DESKTOP_APP_NAME )
    // The ids must equal the launcher basenames that carry the matching StartupWMClass, for both
    // the regular (org.kicad) and Flatpak (org.kicad.KiCad) prefixes.
    const wxString prefix( wxT( KICAD_DESKTOP_APP_PREFIX ) );

    BOOST_CHECK_EQUAL( kicad, wxString( wxT( KICAD_DESKTOP_APP_NAME ) ) );
    BOOST_CHECK_EQUAL( eeschema, prefix + wxT( ".eeschema" ) );
    BOOST_CHECK_EQUAL( pcbnew, prefix + wxT( ".pcbnew" ) );
#endif
}

BOOST_AUTO_TEST_CASE( PcbCalculatorUsesFreedesktopName )
{
    // pcb_calculator installs as pcbcalculator; the id must follow the launcher, not argv[0].
    const wxString id = PGM_BASE::DesktopAppIdForProgram( wxT( "pcb_calculator" ) );

    BOOST_CHECK( id.EndsWith( wxT( ".pcbcalculator" ) ) );
    BOOST_CHECK( !id.EndsWith( wxT( ".pcb_calculator" ) ) );

#if defined( KICAD_DESKTOP_APP_NAME )
    BOOST_CHECK_EQUAL( id, wxString( wxT( KICAD_DESKTOP_APP_PREFIX ) ) + wxT( ".pcbcalculator" ) );
#endif
}

#endif

BOOST_AUTO_TEST_SUITE_END()
