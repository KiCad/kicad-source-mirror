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
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include <wx/utils.h>
#include <qa_utils/wx_utils/unit_test_utils.h>

std::ostream& boost_test_print_type( std::ostream& os, wxPoint const& aPt )
{
    os << "WXPOINT[ x=\"" << aPt.x << "\" y=\"" << aPt.y << "\" ]";
    return os;
}


std::string KI_TEST::GetEeschemaTestDataDir()
{
    const char* env = std::getenv( "KICAD_TEST_EESCHEMA_DATA_DIR" );
    std::string fn;

    if( !env )
    {
        // Use the compiled-in location of the data dir
        // (i.e. where the files were at build time)
        fn = GetTestDataRootDir();
        fn += "eeschema";
    }
    else
    {
        // Use whatever was given in the env var
        fn = env;
    }

    // Ensure the string ends in / to force a directory interpretation
    fn += "/";

    return fn;
}


#ifndef QA_DATA_ROOT
#define QA_DATA_ROOT "???"
#endif

std::string KI_TEST::GetTestDataRootDir()
{
    const char* env = std::getenv( "QA_DATA_ROOT" );
    std::string fn;

    if( !env )
    {
        // Use the compiled-in location of the data dir
        // (i.e. where the files were at build time)
        fn = QA_DATA_ROOT;
    }
    else
    {
        // Use whatever was given in the env var
        fn = env;
    }

    // Ensure the string ends in / to force a directory interpretation
    fn += "/";

    return fn;
}


void KI_TEST::SetMockConfigDir()
{
    if( !wxGetEnv( wxT( "KICAD_CONFIG_HOME" ), nullptr ) )
    {
        wxString path( GetTestDataRootDir() );
        path += wxT( "/config/" );
        wxSetEnv( wxT( "KICAD_CONFIG_HOME" ), path );
        wxSetEnv( wxT( "KICAD_CONFIG_HOME_IS_QA" ), wxT( "1" ) );
    }
}
