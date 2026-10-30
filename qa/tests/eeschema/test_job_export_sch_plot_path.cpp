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
#include <qa_utils/wx_utils/unit_test_utils.h>

#include <cli/exit_codes.h>
#include <eeschema_jobs_handler.h>
#include <jobs/job_export_sch_plot.h>
#include <locale_io.h>
#include <reporter.h>

#include <wx/filename.h>

#include <filesystem>


// A schematic PDF job whose output is a folder names the file after the schematic
BOOST_AUTO_TEST_CASE( SchPlotJobOutputFolderGetsDefaultName )
{
    LOCALE_IO dummy;

    namespace fs = std::filesystem;

    KI_TEST::SCOPED_TEMP_DIR outputDir( wxS( "kicad_sch_plot_folder" ) );

    fs::path schematic = fs::path( KI_TEST::GetTestDataRootDir() ) / "eeschema" / "issue24217" / "issue24217.kicad_sch";

    JOB_EXPORT_SCH_PLOT_PDF job;
    job.m_filename = wxString::FromUTF8( schematic.string().c_str() );

    // A path such as ${KIPRJMOD}/ resolves to a folder with a trailing separator
    job.SetConfiguredOutputPath( wxString::FromUTF8( outputDir.Path().string().c_str() )
                                 + wxFileName::GetPathSeparator() );

    EESCHEMA_JOBS_HANDLER handler( nullptr );
    WX_STRING_REPORTER    reporter;
    handler.SetReporter( &reporter );

    BOOST_CHECK_EQUAL( handler.JobExportPlot( &job ), CLI::EXIT_CODES::OK );
    BOOST_CHECK( fs::is_regular_file( outputDir.Path() / "issue24217.pdf" ) );
}
