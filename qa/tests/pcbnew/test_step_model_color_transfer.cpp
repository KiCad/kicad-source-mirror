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

/*
 * Regression test for https://gitlab.com/kicad/code/kicad/-/issues/24026
 *
 * Some default KiCad STEP models (e.g. SOT-89-3, TO-252-2, TO-263-2, TO-263-5_TabPin6)
 * lost their colors when exported to STEP or 3D PDF because the XDE label tree was
 * transferred via TDocStd_XLinkTool::Copy, which only copies a single label closure.
 * These models store their colored "_part" labels as siblings of the assembly root
 * (under the "Shapes" folder) rather than as descendants, so the closure did not
 * include them and the downstream color-transfer pass found no matching shape.
 *
 * The fix routes transferModel() through XCAFDoc_Editor::Extract(), which rebuilds
 * the XDE label tree by walking the component reference graph and automatically
 * clones color/name metadata along with the shapes.
 */

#include <filesystem>
#include <fstream>
#include <sstream>

#include <qa_utils/wx_utils/unit_test_utils.h>
#include <boost/test/unit_test.hpp>

#include <pcbnew_utils/board_file_utils.h>
#include <reporter.h>
#include <geometry/shape_poly_set.h>
#include <geometry/shape_segment.h>
#include <layer_ids.h>
#include <footprint.h>

#include <exporters/step/step_pcb_model.h>

#include <wx/filename.h>


namespace
{

// Count occurrences of a substring in a text file.  Used to verify that colors
// survived the round trip into the exported STEP file.
static int countEntities( const std::filesystem::path& aFile, const std::string& aToken )
{
    std::ifstream in( aFile );
    BOOST_REQUIRE_MESSAGE( in.good(), "Cannot open exported STEP file for inspection" );

    std::stringstream buf;
    buf << in.rdbuf();
    const std::string contents = buf.str();

    int count = 0;
    size_t pos = 0;

    while( ( pos = contents.find( aToken, pos ) ) != std::string::npos )
    {
        ++count;
        pos += aToken.size();
    }

    return count;
}

} // namespace


BOOST_AUTO_TEST_SUITE( StepModelColorTransfer )


/**
 * Load one of the default STEP models that previously lost color information
 * (TO-252-2.step, see issue #24026) via STEP_PCB_MODEL::AddComponent and
 * write the result to STEP.  The exported file must contain at least one
 * STYLED_ITEM entity and at least one COLOUR_RGB definition; without the fix
 * the output contained zero of each because transferModel() never copied
 * the colored part labels into the destination document.
 */
BOOST_AUTO_TEST_CASE( TO252_2_ColorsPreserved )
{
    const std::filesystem::path sourceModel =
            std::filesystem::path( KI_TEST::GetPcbnewTestDataDir() )
            / "step_model_colors" / "TO-252-2.step";

    BOOST_REQUIRE_MESSAGE( std::filesystem::exists( sourceModel ),
                           "Missing test model " << sourceModel.string() );

    const std::filesystem::path outputFile =
            std::filesystem::temp_directory_path() / "kicad_step_model_color_test.step";

    if( std::filesystem::exists( outputFile ) )
        std::filesystem::remove( outputFile );

    NULL_REPORTER reporter;

    STEP_PCB_MODEL model( wxT( "test" ), &reporter );
    model.SpecializeVariant( OUTPUT_FORMAT::FMT_OUT_STEP );
    model.SetCopperColor( 0.75, 0.61, 0.23 );
    model.SetPadColor( 0.9, 0.9, 0.9 );

    // Emit a trivial board outline so CreatePCB has something to extrude.
    SHAPE_POLY_SET outline;
    outline.NewOutline();
    outline.Append( VECTOR2I( 0, 0 ) );
    outline.Append( VECTOR2I( 10'000'000, 0 ) );
    outline.Append( VECTOR2I( 10'000'000, 10'000'000 ) );
    outline.Append( VECTOR2I( 0, 10'000'000 ) );

    BOOST_REQUIRE( model.CreatePCB( outline, VECTOR2D( 0, 0 ), true ) );

    const wxString modelPath = wxString::FromUTF8( sourceModel.string().c_str() );

    const bool added = model.AddComponent( wxT( "TO-252-2" ), modelPath, {}, wxT( "U1" ), false,
                                           VECTOR2D( 5.0, 5.0 ), 0.0, VECTOR3D( 0.0, 0.0, 0.0 ),
                                           VECTOR3D( 0.0, 0.0, 0.0 ), VECTOR3D( 1.0, 1.0, 1.0 ),
                                           false );

    BOOST_REQUIRE_MESSAGE( added, "AddComponent failed for TO-252-2.step" );

    const wxString outputPath = wxString::FromUTF8( outputFile.string().c_str() );

    BOOST_REQUIRE_MESSAGE( model.WriteSTEP( outputPath, false, false ),
                           "WriteSTEP returned failure" );

    BOOST_REQUIRE( std::filesystem::exists( outputFile ) );

    const int styled = countEntities( outputFile, "STYLED_ITEM" );
    const int colors = countEntities( outputFile, "COLOUR_RGB" );

    BOOST_TEST_MESSAGE( "Exported STEP: " << styled << " STYLED_ITEM, " << colors
                                           << " COLOUR_RGB entities" );

    // The component model carries two distinct colors; the board body adds more.  A healthy
    // export must retain at least the component's colors (> 2 COLOUR_RGB); without the fix
    // the component contribution is zero so COLOUR_RGB drops to whatever the board body emits.
    BOOST_CHECK_MESSAGE(
            styled > 0,
            "Exported STEP has no STYLED_ITEM entities; component colors were dropped" );
    BOOST_CHECK_MESSAGE(
            colors > 2,
            "Expected > 2 COLOUR_RGB entities (component + board); got " << colors
            << ".  Component model colors were not transferred." );

    // Keep the file around for inspection when the test fails.
    if( styled > 0 && colors > 2 && std::filesystem::exists( outputFile ) )
        std::filesystem::remove( outputFile );
}


BOOST_AUTO_TEST_SUITE_END()
