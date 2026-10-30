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

#include <boost/test/unit_test.hpp>

#include <qa_utils/file_utils.h>

#include <jobs/job_export_pcb_drill.h>

#include <wx/filename.h>


BOOST_AUTO_TEST_SUITE( JobOutputPath )


// A jobset drill job keeps an absolute output directory like the file based jobs do
BOOST_AUTO_TEST_CASE( DrillAbsoluteDirectoryIsKept )
{
    KI_TEST::SCOPED_TEMP_DIR projectRoot( wxS( "kicad_job_output_path_prj" ) );
    KI_TEST::SCOPED_TEMP_DIR tempRoot( wxS( "kicad_job_output_path_tmp" ) );

    wxString dataDir = wxFileName::DirName( wxString::FromUTF8( ( projectRoot.Path() / "data" ).string().c_str() ) )
                               .GetFullPath();

    JOB_EXPORT_PCB_DRILL drill;
    BOOST_REQUIRE( drill.GetOutputPathIsDirectory() );

    drill.SetConfiguredOutputPath( dataDir );
    drill.SetTempOutputDirectory( wxString::FromUTF8( tempRoot.Path().string().c_str() ) );

    BOOST_CHECK_EQUAL( drill.GetFullOutputPath( nullptr ), dataDir );
}


BOOST_AUTO_TEST_SUITE_END()
