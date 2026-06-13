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
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/gpl-3.0.html
 * or you may search the http://www.gnu.org website for the version 3 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include <boost/test/unit_test.hpp>

#include <jobs/job_pcb_diff.h>

#include <wx/file.h>
#include <wx/filename.h>
#include <wx/process.h>
#include <wx/stdpaths.h>
#include <wx/txtstrm.h>

#include <array>
#include <string>
#include <vector>


#ifndef QA_KICAD_CLI_PATH
#define QA_KICAD_CLI_PATH "kicad-cli"
#endif


namespace
{

struct TEMP_DIR
{
    TEMP_DIR()
    {
        static int counter = 0;
        m_path = wxFileName::GetTempDir() + wxFILE_SEP_PATH
                 + wxString::Format( wxS( "kicad_cli_visual_diff_%ld_%d" ),
                                     static_cast<long>( wxGetProcessId() ), ++counter );

        BOOST_REQUIRE( wxFileName::Mkdir( m_path, wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL ) );
    }

    ~TEMP_DIR()
    {
        if( !m_path.IsEmpty() && wxFileName::DirExists( m_path ) )
            wxFileName::Rmdir( m_path, wxPATH_RMDIR_RECURSIVE );
    }

    wxString Path( const wxString& aName ) const
    {
        return m_path + wxFILE_SEP_PATH + aName;
    }

    wxString m_path;
};


struct COMMAND_RESULT
{
    int      exitCode = -1;
    wxString output;
    wxString error;
};


wxString readStream( wxInputStream* aStream )
{
    wxString output;

    if( !aStream )
        return output;

    wxTextInputStream textStream( *aStream );

    while( !aStream->Eof() )
    {
        wxString line = textStream.ReadLine();

        if( !line.IsEmpty() || !aStream->Eof() )
        {
            if( !output.IsEmpty() )
                output += wxS( "\n" );

            output += line;
        }
    }

    return output;
}


COMMAND_RESULT runCli( const std::vector<wxString>& aArgs )
{
    wxProcess process;
    process.Redirect();

    std::vector<const wchar_t*> argv;
    argv.reserve( aArgs.size() + 2 );
    argv.push_back( wxS( QA_KICAD_CLI_PATH ) );

    for( const wxString& arg : aArgs )
        argv.push_back( arg.wc_str() );

    argv.push_back( nullptr );

    COMMAND_RESULT result;
    result.exitCode = static_cast<int>( wxExecute( const_cast<wchar_t**>( argv.data() ), wxEXEC_SYNC, &process ) );
    result.output = readStream( process.GetInputStream() );
    result.error = readStream( process.GetErrorStream() );

    return result;
}


void writeTextFile( const wxString& aPath, const char* aContent )
{
    wxFile file;
    BOOST_REQUIRE_MESSAGE( file.Create( aPath, true ), "Could not create " << aPath );
    BOOST_REQUIRE( file.Write( wxString::FromUTF8( aContent ) ) );
    file.Close();
}


std::string readFileBytes( const wxString& aPath )
{
    wxFile file( aPath );
    BOOST_REQUIRE_MESSAGE( file.IsOpened(), "Could not open " << aPath );

    const wxFileOffset len = file.Length();
    BOOST_REQUIRE_GE( len, 0 );

    std::string bytes( static_cast<size_t>( len ), '\0' );

    if( len > 0 )
    {
        ssize_t read = file.Read( bytes.data(), static_cast<size_t>( len ) );
        BOOST_REQUIRE_EQUAL( read, len );
    }

    return bytes;
}


void expectCleanExit( const wxString& aName, const COMMAND_RESULT& aResult, int aExpectedExitCode )
{
    BOOST_TEST_CONTEXT( aName )
    {
        BOOST_CHECK_EQUAL( aResult.exitCode, aExpectedExitCode );
        BOOST_CHECK_MESSAGE( aResult.output.IsEmpty(), "Unexpected stdout: " << aResult.output );
        BOOST_CHECK_MESSAGE( aResult.error.IsEmpty(), "Unexpected stderr: " << aResult.error );
    }
}


void expectInvalidExit( const wxString& aName, const COMMAND_RESULT& aResult, int aExpectedExitCode )
{
    BOOST_TEST_CONTEXT( aName )
    {
        BOOST_CHECK_EQUAL( aResult.exitCode, aExpectedExitCode );
    }
}


void expectSvg( const wxString& aName, const wxString& aPath )
{
    BOOST_TEST_CONTEXT( aName )
    {
        std::string bytes = readFileBytes( aPath );
        BOOST_REQUIRE( !bytes.empty() );
        BOOST_CHECK( bytes.find( "<svg" ) != std::string::npos );
    }
}


void expectPng( const wxString& aName, const wxString& aPath )
{
    static constexpr std::array<unsigned char, 8> PNG_HEADER = { 0x89, 'P', 'N', 'G', '\r', '\n', 0x1a, '\n' };

    BOOST_TEST_CONTEXT( aName )
    {
        std::string bytes = readFileBytes( aPath );
        BOOST_REQUIRE_GE( bytes.size(), PNG_HEADER.size() );

        for( size_t i = 0; i < PNG_HEADER.size(); ++i )
            BOOST_CHECK_EQUAL( static_cast<unsigned char>( bytes[i] ), PNG_HEADER[i] );
    }
}


void expectFilesDiffer( const wxString& aName, const wxString& aPathA, const wxString& aPathB )
{
    BOOST_TEST_CONTEXT( aName )
    {
        BOOST_CHECK( readFileBytes( aPathA ) != readFileBytes( aPathB ) );
    }
}


struct CLI_FIXTURES
{
    explicit CLI_FIXTURES( TEMP_DIR& aDir )
    {
        pcbA = aDir.Path( wxS( "pcb_a.kicad_pcb" ) );
        pcbB = aDir.Path( wxS( "pcb_b.kicad_pcb" ) );
        schA = aDir.Path( wxS( "sch_a.kicad_sch" ) );
        schB = aDir.Path( wxS( "sch_b.kicad_sch" ) );
        fpA = aDir.Path( wxS( "fp_a.pretty" ) );
        fpB = aDir.Path( wxS( "fp_b.pretty" ) );
        symA = aDir.Path( wxS( "sym_a.kicad_sym" ) );
        symB = aDir.Path( wxS( "sym_b.kicad_sym" ) );

        writePcbFixtures();
        writeSchFixtures();
        writeFpFixtures();
        writeSymFixtures();
    }

    void writePcbFixtures()
    {
        writeTextFile( pcbA, R"(
(kicad_pcb (version 20241228) (generator "pcbnew") (generator_version "9.0")
  (general (thickness 1.6))
  (paper "A4")
  (layers
    (0 "F.Cu" signal)
    (31 "B.Cu" signal)
    (44 "Edge.Cuts" user)
  )
)
)" );

        writeTextFile( pcbB, R"(
(kicad_pcb (version 20241228) (generator "pcbnew") (generator_version "9.0")
  (general (thickness 1.6))
  (paper "A4")
  (layers
    (0 "F.Cu" signal)
    (31 "B.Cu" signal)
    (44 "Edge.Cuts" user)
  )
  (gr_line (start 10 10) (end 40 10)
    (stroke (width 0.15) (type solid)) (layer "Edge.Cuts") (uuid "11111111-1111-1111-1111-111111111111"))
)
)" );
    }

    void writeSchFixtures()
    {
        writeTextFile( schA, R"(
(kicad_sch (version 20230121) (generator "eeschema")
  (uuid "00000000-0000-0000-0000-000000000001")
  (paper "A4")
)
)" );

        writeTextFile( schB, R"(
(kicad_sch (version 20230121) (generator "eeschema")
  (uuid "00000000-0000-0000-0000-000000000001")
  (paper "A4")
  (wire (pts (xy 25.4 25.4) (xy 50.8 25.4))
    (stroke (width 0) (type default))
    (uuid "11111111-1111-1111-1111-111111111111")
  )
)
)" );
    }

    void writeFpFixtures()
    {
        BOOST_REQUIRE( wxFileName::Mkdir( fpA, wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL ) );
        BOOST_REQUIRE( wxFileName::Mkdir( fpB, wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL ) );

        writeTextFile( fpB + wxFILE_SEP_PATH + wxS( "VisualDiff.kicad_mod" ), R"(
(footprint "VisualDiff"
  (version 20240108)
  (generator "pcbnew")
  (layer "F.Cu")
  (fp_line
    (start 0 0)
    (end 2 0)
    (stroke (width 0.12) (type solid))
    (layer "F.SilkS")
    (uuid "11111111-1111-1111-1111-111111111111")
  )
)
)" );
    }

    void writeSymFixtures()
    {
        writeTextFile( symA, R"(
(kicad_symbol_lib
  (version 20241209)
  (generator "kicad_symbol_editor")
  (generator_version "9.0")
)
)" );

        writeTextFile( symB, R"(
(kicad_symbol_lib
  (version 20241209)
  (generator "kicad_symbol_editor")
  (generator_version "9.0")
  (symbol "VisualDiff"
    (exclude_from_sim no)
    (in_bom yes)
    (on_board yes)
    (property "Reference" "U" (at 0 2.54 0) (effects (font (size 1.27 1.27))))
    (property "Value" "VisualDiff" (at 0 -2.54 0) (effects (font (size 1.27 1.27))))
    (property "Footprint" "" (at 0 0 0) (effects (font (size 1.27 1.27)) (hide yes)))
    (property "Datasheet" "" (at 0 0 0) (effects (font (size 1.27 1.27)) (hide yes)))
    (symbol "VisualDiff_1_1"
      (rectangle (start -2.54 -2.54) (end 5.08 2.54) (stroke (width 0.254) (type default)) (fill (type none)))
    )
  )
)
)" );
    }

    wxString pcbA;
    wxString pcbB;
    wxString schA;
    wxString schB;
    wxString fpA;
    wxString fpB;
    wxString symA;
    wxString symB;
};


struct CLI_CASE
{
    wxString commandGroup;
    wxString commandName;
    wxString refPath;
    wxString changedPath;
};


void expectVisualDifference( TEMP_DIR& aDir, const CLI_CASE& aCase, const wxString& aFormat )
{
    const wxString caseName = aCase.commandGroup + wxS( " " ) + aFormat;
    const wxString sameOut = aDir.Path( aCase.commandGroup + wxS( "_same." ) + aFormat );
    const wxString diffOut = aDir.Path( aCase.commandGroup + wxS( "_diff." ) + aFormat );

    std::vector<wxString> sameArgs = { aCase.commandGroup, aCase.commandName, aCase.refPath, aCase.refPath,
                                       wxS( "--format" ), aFormat, wxS( "--output" ), sameOut };
    std::vector<wxString> diffArgs = { aCase.commandGroup, aCase.commandName, aCase.refPath, aCase.changedPath,
                                       wxS( "--format" ), aFormat, wxS( "--output" ), diffOut };

    expectCleanExit( caseName + wxS( " identical" ), runCli( sameArgs ), 0 );
    expectCleanExit( caseName + wxS( " changed" ), runCli( diffArgs ), 5 );

    if( aFormat == wxS( "svg" ) )
    {
        expectSvg( caseName + wxS( " identical" ), sameOut );
        expectSvg( caseName + wxS( " changed" ), diffOut );
    }
    else
    {
        expectPng( caseName + wxS( " identical" ), sameOut );
        expectPng( caseName + wxS( " changed" ), diffOut );
    }

    expectFilesDiffer( caseName, sameOut, diffOut );
}

} // namespace


BOOST_AUTO_TEST_SUITE( DiffJobConfig )


// The diff jobs once declared their own m_outputPath, shadowing JOB::m_outputPath and
// registering a duplicate "output" JSON param alongside the base "output_filename" key.
// Serialization must now expose exactly one output key, routed through the base member.
BOOST_AUTO_TEST_CASE( DiffJobSerializesSingleOutputKey )
{
    JOB_PCB_DIFF job;
    job.SetConfiguredOutputPath( wxS( "diff-out.json" ) );

    BOOST_CHECK_EQUAL( job.GetConfiguredOutputPath(), wxString( wxS( "diff-out.json" ) ) );

    nlohmann::json j;
    job.ToJson( j );

    BOOST_CHECK( j.contains( "output_filename" ) );
    BOOST_CHECK_EQUAL( j["output_filename"].get<wxString>(), wxString( wxS( "diff-out.json" ) ) );
    BOOST_CHECK( !j.contains( "output" ) );
}


BOOST_AUTO_TEST_SUITE_END()


BOOST_AUTO_TEST_SUITE( CliVisualDiff )


BOOST_AUTO_TEST_CASE( RejectsInvalidVisualDiffArguments )
{
    TEMP_DIR     dir;
    CLI_FIXTURES fixtures( dir );

    const std::vector<CLI_CASE> cases = {
        { wxS( "pcb" ), wxS( "diff" ), fixtures.pcbA, fixtures.pcbB },
        { wxS( "sch" ), wxS( "diff" ), fixtures.schA, fixtures.schB },
        { wxS( "fp" ), wxS( "diff" ), fixtures.fpA, fixtures.fpB },
        { wxS( "sym" ), wxS( "diff" ), fixtures.symA, fixtures.symB },
    };

    for( const CLI_CASE& c : cases )
    {
        BOOST_TEST_CONTEXT( c.commandGroup )
        {
            expectInvalidExit( wxS( "invalid format" ),
                               runCli( { c.commandGroup, c.commandName, c.refPath, c.refPath, wxS( "--format" ),
                                         wxS( "bogus" ) } ),
                               1 );

            expectInvalidExit( wxS( "svg requires output" ),
                               runCli( { c.commandGroup, c.commandName, c.refPath, c.refPath, wxS( "--format" ),
                                         wxS( "svg" ) } ),
                               1 );

            expectInvalidExit( wxS( "png requires output" ),
                               runCli( { c.commandGroup, c.commandName, c.refPath, c.refPath, wxS( "--format" ),
                                         wxS( "png" ) } ),
                               1 );
        }
    }
}


BOOST_AUTO_TEST_CASE( RendersChangedPngAndSvgContentForEveryCommand )
{
    TEMP_DIR     dir;
    CLI_FIXTURES fixtures( dir );

    const std::vector<CLI_CASE> cases = {
        { wxS( "pcb" ), wxS( "diff" ), fixtures.pcbA, fixtures.pcbB },
        { wxS( "sch" ), wxS( "diff" ), fixtures.schA, fixtures.schB },
        { wxS( "fp" ), wxS( "diff" ), fixtures.fpA, fixtures.fpB },
        { wxS( "sym" ), wxS( "diff" ), fixtures.symA, fixtures.symB },
    };

    for( const CLI_CASE& c : cases )
    {
        expectVisualDifference( dir, c, wxS( "svg" ) );
        expectVisualDifference( dir, c, wxS( "png" ) );
    }
}


BOOST_AUTO_TEST_SUITE_END()
