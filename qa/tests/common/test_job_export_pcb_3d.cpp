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

#include <qa_utils/wx_utils/unit_test_utils.h>

#include <base_units.h>
#include <jobs/job_export_pcb_3d.h>
#include <jobs/jobset.h>
#include <kiid.h>
#include <richio.h>

#include <filesystem>
#include <system_error>
#include <utility>
#include <vector>

namespace fs = std::filesystem;


/**
 * The 3D model export job settings must survive a real save/load cycle through a .kicad_jobset
 * on disk.  An unmapped field is invisible in memory: the dialog reads it back from the still
 * live job object and only a reload exposes the loss.
 *
 * Regression tests for https://gitlab.com/kicad/code/kicad/-/issues/25107
 */
class JOB_EXPORT_PCB_3D_TEST_FIXTURE
{
public:
    JOB_EXPORT_PCB_3D_TEST_FIXTURE()
    {
        m_tempDir = fs::temp_directory_path()
                    / ( "kicad_job_export_pcb_3d_test_" + KIID().AsString().ToStdString() );
        BOOST_REQUIRE( fs::create_directories( m_tempDir ) );

        fs::path source = fs::path( KI_TEST::GetTestDataRootDir() ) / "pcbnew" / "issue25107"
                          / "issue25107.kicad_jobset";

        m_jobsetPath = m_tempDir / "issue25107.kicad_jobset";
        fs::copy_file( source, m_jobsetPath );
    }

    ~JOB_EXPORT_PCB_3D_TEST_FIXTURE()
    {
        std::error_code ec;
        fs::remove_all( m_tempDir, ec );
    }

    /// The reporter's second job, "3D model export (resistors only)"
    JOB_EXPORT_PCB_3D* loadFilteredJob( JOBSET& aJobset )
    {
        BOOST_REQUIRE( aJobset.LoadFromFile() );
        BOOST_REQUIRE_EQUAL( aJobset.GetJobs().size(), 2 );

        JOB_EXPORT_PCB_3D* job = dynamic_cast<JOB_EXPORT_PCB_3D*>( aJobset.GetJobs()[1].m_job.get() );
        BOOST_REQUIRE( job );

        return job;
    }

    wxString jobsetPath() const { return wxString( m_jobsetPath.string() ); }

    wxString jobsetContents() const { return SafeReadFile( jobsetPath(), wxS( "rt" ) ); }

    fs::path m_tempDir;
    fs::path m_jobsetPath;
};


/// Every boolean setting of the job, so that adding a field without a JSON mapping fails the
/// round trip below instead of silently resetting itself on the user's next reload
static std::vector<std::pair<const char*, bool*>> boolSettings( JOB_EXPORT_PCB_3D& aJob )
{
    EXPORTER_STEP_PARAMS& p = aJob.m_3dparams;

    return { { "overwrite", &p.m_Overwrite },
             { "use_grid_origin", &p.m_UseGridOrigin },
             { "use_drill_origin", &p.m_UseDrillOrigin },
             { "use_defined_origin", &p.m_UseDefinedOrigin },
             { "use_pcb_center_origin", &p.m_UsePcbCenterOrigin },
             { "board_only", &p.m_BoardOnly },
             { "include_unspecified", &p.m_IncludeUnspecified },
             { "include_dnp", &p.m_IncludeDNP },
             { "subst_models", &p.m_SubstModels },
             { "optimize_step", &p.m_OptimizeStep },
             { "cut_vias_in_body", &p.m_CutViasInBody },
             { "export_board_body", &p.m_ExportBoardBody },
             { "export_components", &p.m_ExportComponents },
             { "export_tracks", &p.m_ExportTracksVias },
             { "export_pads", &p.m_ExportPads },
             { "export_zones", &p.m_ExportZones },
             { "export_inner_copper", &p.m_ExportInnerCopper },
             { "export_silkscreen", &p.m_ExportSilkscreen },
             { "export_soldermask", &p.m_ExportSoldermask },
             { "fuse_shapes", &p.m_FuseShapes },
             { "fill_all_vias", &p.m_FillAllVias },
             { "extra_pad_thickness", &p.m_ExtraPadThickness },
             { "has_user_origin", &aJob.m_hasUserOrigin },
             { "vrml_relative_paths", &aJob.m_vrmlRelativePaths } };
}


BOOST_FIXTURE_TEST_SUITE( JobExportPcb3d, JOB_EXPORT_PCB_3D_TEST_FIXTURE )


/**
 * The component filter and the net filter are the two fields called out in the issue.  Neither
 * was written to the jobset, so the reporter's second job carries no filter at all.
 */
BOOST_AUTO_TEST_CASE( FiltersSurviveDiskRoundTrip )
{
    {
        JOBSET jobset( jobsetPath() );
        JOB_EXPORT_PCB_3D* job = loadFilteredJob( jobset );

        job->m_3dparams.m_ComponentFilter = wxS( "R1,R2,R3" );
        job->m_3dparams.m_NetFilter = wxS( "GND,+3V3" );

        BOOST_REQUIRE( jobset.SaveToFile( wxEmptyString, true ) );
    }

    wxString contents = jobsetContents();

    BOOST_CHECK( contents.Contains( wxS( "component_filter" ) ) );
    BOOST_CHECK( contents.Contains( wxS( "net_filter" ) ) );

    JOBSET reloaded( jobsetPath() );
    JOB_EXPORT_PCB_3D* job = loadFilteredJob( reloaded );

    BOOST_CHECK_EQUAL( job->m_3dparams.m_ComponentFilter, wxS( "R1,R2,R3" ) );
    BOOST_CHECK_EQUAL( job->m_3dparams.m_NetFilter, wxS( "GND,+3V3" ) );
}


BOOST_AUTO_TEST_CASE( BooleanSettingsSurviveDiskRoundTrip )
{
    std::vector<bool> expected;

    {
        JOBSET jobset( jobsetPath() );
        JOB_EXPORT_PCB_3D* job = loadFilteredJob( jobset );

        // Invert each flag so that a dropped mapping cannot pass by landing back on its default
        for( const auto& [key, flag] : boolSettings( *job ) )
        {
            *flag = !*flag;
            expected.push_back( *flag );
        }

        BOOST_REQUIRE( jobset.SaveToFile( wxEmptyString, true ) );
    }

    JOBSET reloaded( jobsetPath() );
    JOB_EXPORT_PCB_3D* job = loadFilteredJob( reloaded );
    std::vector<std::pair<const char*, bool*>> settings = boolSettings( *job );

    for( size_t ii = 0; ii < settings.size(); ++ii )
    {
        BOOST_TEST_CONTEXT( settings[ii].first )
        {
            BOOST_CHECK_EQUAL( *settings[ii].second, expected[ii] );
        }
    }
}


/// The remaining non-boolean settings, which the table above cannot flip
BOOST_AUTO_TEST_CASE( ValueSettingsSurviveDiskRoundTrip )
{
    {
        JOBSET jobset( jobsetPath() );
        JOB_EXPORT_PCB_3D* job = loadFilteredJob( jobset );

        job->SetStepFormat( EXPORTER_STEP_PARAMS::FORMAT::GLB );
        job->m_3dparams.m_Origin = VECTOR2D( pcbIUScale.mmToIU( 12.5 ), pcbIUScale.mmToIU( -7.25 ) );
        job->m_3dparams.m_BoardOutlinesChainingEpsilon = 0.001;
        job->m_variant = wxS( "assembly" );
        job->m_vrmlUnits = JOB_EXPORT_PCB_3D::VRML_UNITS::INCH;
        job->m_vrmlModelDir = wxS( "shapes3D" );
        job->SetConfiguredOutputPath( wxS( "out/model.glb" ) );

        BOOST_REQUIRE( jobset.SaveToFile( wxEmptyString, true ) );
    }

    JOBSET reloaded( jobsetPath() );
    JOB_EXPORT_PCB_3D* job = loadFilteredJob( reloaded );

    BOOST_CHECK( job->m_format == JOB_EXPORT_PCB_3D::FORMAT::GLB );
    BOOST_CHECK( job->m_3dparams.m_Format == EXPORTER_STEP_PARAMS::FORMAT::GLB );
    // Held in internal units, but the jobset has always stored millimetres
    BOOST_CHECK_EQUAL( job->m_3dparams.m_Origin.x, pcbIUScale.mmToIU( 12.5 ) );
    BOOST_CHECK_EQUAL( job->m_3dparams.m_Origin.y, pcbIUScale.mmToIU( -7.25 ) );
    BOOST_CHECK( jobsetContents().Contains( wxS( "\"user_origin.x\": 12.5" ) ) );
    BOOST_CHECK_EQUAL( job->m_3dparams.m_BoardOutlinesChainingEpsilon, 0.001 );
    BOOST_CHECK_EQUAL( job->m_variant, wxS( "assembly" ) );
    BOOST_CHECK( job->m_vrmlUnits == JOB_EXPORT_PCB_3D::VRML_UNITS::INCH );
    BOOST_CHECK_EQUAL( job->m_vrmlModelDir, wxS( "shapes3D" ) );
    BOOST_CHECK_EQUAL( job->GetConfiguredOutputPath(), wxS( "out/model.glb" ) );
}


BOOST_AUTO_TEST_SUITE_END()
