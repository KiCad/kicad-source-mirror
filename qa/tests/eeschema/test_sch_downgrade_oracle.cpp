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

#include <boost/test/unit_test.hpp>

#include <algorithm>
#include <array>
#include <fstream>
#include <iterator>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include <bitmap_base.h>
#include <schematic.h>
#include <sch_bitmap.h>
#include <sch_screen.h>
#include <sch_sheet.h>
#include <lib_symbol.h>
#include <sch_symbol.h>
#include <sch_pin.h>
#include <sch_shape.h>
#include <connection_graph.h>
#include <eeschema_jobs_handler.h>
#include <jobs/job_sch_downgrade.h>
#include <cli/exit_codes.h>
#include <netlist_exporters/netlist_exporter_kicad.h>
#include <richio.h>
#include <downgrade/sch_downgrade.h>
#include <downgrade_target.h>
#include <sch_io/kicad_sexpr/sch_io_kicad_sexpr.h>
#include <project/project_file.h>
#include <settings/settings_manager.h>

#include <wx/filefn.h>
#include <wx/filename.h>

#include <qa_utils/downgrade_oracle_utils.h>
#include <qa_utils/downgrade_golden_utils.h>
#include <qa_utils/temporary_directory.h>
#include <qa_utils/wx_utils/unit_test_utils.h>
#include <eeschema_test_utils.h>
#include <schematic_file_util.h>


BOOST_AUTO_TEST_SUITE( SchDowngradeOracle )

static const DOWNGRADE_TARGET& kicad9 = KI_TEST::RequireDowngradeTarget( wxT( "9.0" ) );
static const DOWNGRADE_TARGET& kicad10 = KI_TEST::RequireDowngradeTarget( wxT( "10.0" ) );


// Exercise the real .kicad_sym write path used to downgrade a symbol library, and confirm the result
// carries the target stamp and reloads cleanly.
BOOST_AUTO_TEST_CASE( SymbolLibraryFileDowngradeRoundTrips )
{
    wxString dest = wxFileName::CreateTempFileName( wxT( "kicad_qa_symlib" ) );
    wxRemoveFile( dest );
    dest += wxT( ".kicad_sym" );

    const wxString src = KI_TEST::GetEeschemaTestDataDir() + "/../libraries/test_project/Device.kicad_sym";

    {
        SCH_IO_KICAD_SEXPR       pi;
        std::vector<LIB_SYMBOL*> symbols;
        pi.EnumerateSymbolLib( symbols, src );
        BOOST_REQUIRE( !symbols.empty() );

        for( LIB_SYMBOL* symbol : symbols )
            DowngradeLibSymbolInPlace( symbol, kicad9 );

        SaveSymbolLibraryForTarget( symbols, dest, kicad9 );
    }

    // The written file must declare the target version and reload without error.
    std::ifstream file( dest.ToStdString() );
    std::string   content( ( std::istreambuf_iterator<char>( file ) ), std::istreambuf_iterator<char>() );
    file.close();

    BOOST_CHECK( content.find( "(version " + std::to_string( kicad9.m_symLibVersion ) + ")" ) != std::string::npos );

    SCH_IO_KICAD_SEXPR       reader;
    std::vector<LIB_SYMBOL*> reloaded;
    BOOST_CHECK_NO_THROW( reader.EnumerateSymbolLib( reloaded, dest ) );
    BOOST_CHECK( !reloaded.empty() );

    wxRemoveFile( dest );
}


// The v10 symbol library path gets the same round-trip check as v9.
BOOST_AUTO_TEST_CASE( SymbolLibraryV10RoundTrips )
{
    SETTINGS_MANAGER settingsManager;

    wxString dest = wxFileName::CreateTempFileName( wxT( "kicad_qa_symlib10" ) );
    wxRemoveFile( dest );
    dest += wxT( ".kicad_sym" );

    const wxString src = KI_TEST::GetEeschemaTestDataDir() + "/../libraries/test_project/Device.kicad_sym";

    {
        SCH_IO_KICAD_SEXPR       pi;
        std::vector<LIB_SYMBOL*> symbols;
        pi.EnumerateSymbolLib( symbols, src );
        BOOST_REQUIRE( !symbols.empty() );

        for( LIB_SYMBOL* symbol : symbols )
            DowngradeLibSymbolInPlace( symbol, kicad10 );

        SaveSymbolLibraryForTarget( symbols, dest, kicad10 );
    }

    std::ifstream file( dest.ToStdString() );
    std::string   content( ( std::istreambuf_iterator<char>( file ) ), std::istreambuf_iterator<char>() );
    file.close();
    wxRemoveFile( dest );

    BOOST_CHECK( content.find( "(version " + std::to_string( kicad10.m_symLibVersion ) + ")" ) != std::string::npos );
}


// The write must put a parent before symbols that inherit from it. In this fixture "C" extends
// "CAP" but sorts before it by name.
BOOST_AUTO_TEST_CASE( SymbolLibraryWritesParentBeforeChild )
{
    wxString dest = wxFileName::CreateTempFileName( wxT( "kicad_qa_symlib_inherit" ) );
    wxRemoveFile( dest );
    dest += wxT( ".kicad_sym" );

    const wxString src =
            KI_TEST::GetEeschemaTestDataDir() + "/spice_netlists/legacy_pspice/schematic_libspice.kicad_sym";

    {
        SCH_IO_KICAD_SEXPR       pi;
        std::vector<LIB_SYMBOL*> symbols;
        pi.EnumerateSymbolLib( symbols, src );
        BOOST_REQUIRE( !symbols.empty() );

        std::stable_sort( symbols.begin(), symbols.end(),
                          []( const LIB_SYMBOL* a, const LIB_SYMBOL* b )
                          {
                              return a->GetInheritanceDepth() < b->GetInheritanceDepth();
                          } );

        for( LIB_SYMBOL* symbol : symbols )
            DowngradeLibSymbolInPlace( symbol, kicad9 );

        SaveSymbolLibraryForTarget( symbols, dest, kicad9 );
    }

    std::ifstream file( dest.ToStdString() );
    std::string   content( ( std::istreambuf_iterator<char>( file ) ), std::istreambuf_iterator<char>() );
    file.close();
    wxRemoveFile( dest );

    size_t parentPos = content.find( "(symbol \"CAP\"" );
    size_t childRef = content.find( "(extends \"CAP\")" );

    BOOST_REQUIRE( parentPos != std::string::npos );
    BOOST_REQUIRE( childRef != std::string::npos );
    BOOST_CHECK( parentPos < childRef );
}


// The target computes the image PPI the old truncating way, so the stored scale must be
// migrated back or the image renders at the wrong size there.
BOOST_AUTO_TEST_CASE( ImageScaleMigratedForV9 )
{
    SETTINGS_MANAGER settingsManager;

    std::unique_ptr<SCHEMATIC> schematic;
    KI_TEST::LoadSchematic( settingsManager, wxT( "NoConnectOnPin" ), schematic );
    BOOST_REQUIRE( schematic != nullptr );

    SCH_SHEET*  sheet = schematic->GetTopLevelSheet();
    SCH_BITMAP*    otherBitmap = new SCH_BITMAP();
    wxMemoryBuffer buf = KI_TEST::MakePngWithFractionalPixelsPerCm();
    BOOST_REQUIRE( otherBitmap->GetReferenceImage().ReadImageFile( buf ) );
    otherBitmap->GetReferenceImage().SetImageScale( 0.5 );
    sheet->GetScreen()->Append( otherBitmap );

    SCH_BITMAP* bitmap = new SCH_BITMAP();

    REFERENCE_IMAGE& ref = bitmap->GetReferenceImage();
    BOOST_REQUIRE( ref.ReadImageFile( buf ) );
    ref.SetImageScale( 2.0 );
    sheet->GetScreen()->Append( bitmap );

    int ppi = ref.GetImage().GetPPI();
    int legacyPPI = ref.GetImage().GetLegacyPPI();
    BOOST_REQUIRE( legacyPPI > 0 && ppi != legacyPPI );

    double originalScale = ref.GetImageScale();

    wxString dest = wxFileName::CreateTempFileName( wxT( "kicad_qa_sch_imgscale" ) );
    wxRemoveFile( dest );
    dest += wxT( ".kicad_sch" );

    DowngradeScreenInPlace( sheet->GetScreen(), kicad9 );
    SaveSchematicForTarget( sheet, schematic.get(), dest, kicad9 );

    std::ifstream file( dest.ToStdString() );
    std::string   content( ( std::istreambuf_iterator<char>( file ) ), std::istreambuf_iterator<char>() );
    file.close();
    wxRemoveFile( dest );

    const double written = KI_TEST::SerializedImageScale( content, bitmap->m_Uuid.AsStdString() );
    BOOST_CHECK_CLOSE( written, originalScale * legacyPPI / ppi, 0.1 );
    BOOST_CHECK_CLOSE( KI_TEST::SerializedImageScale( content, otherBitmap->m_Uuid.AsStdString() ),
                       0.5 * legacyPPI / ppi, 0.1 );
}


// The downgrade job snapshots and restores a live project's settings pointers around a transient
// SCHEMATIC. That is only sound while ~SCHEMATIC leaves the project alone, so pin that here.
BOOST_AUTO_TEST_CASE( SchematicDtorLeavesProjectSettingsAlone )
{
    SETTINGS_MANAGER settingsManager;

    std::unique_ptr<SCHEMATIC> schematic;
    KI_TEST::LoadSchematic( settingsManager, wxT( "NoConnectOnPin" ), schematic );
    BOOST_REQUIRE( schematic != nullptr );

    PROJECT&      project = schematic->Project();
    PROJECT_FILE& projectFile = project.GetProjectFile();

    ERC_SETTINGS*       erc = projectFile.m_ErcSettings;
    SCHEMATIC_SETTINGS* settings = projectFile.m_SchematicSettings;

    schematic.reset();

    BOOST_CHECK( projectFile.m_ErcSettings == erc );
    BOOST_CHECK( projectFile.m_SchematicSettings == settings );
}


static std::string downgradeDataPath()
{
    return KI_TEST::GetEeschemaTestDataDir() + "/../downgrade/";
}


static int runDowngradeJob( JOB_SCH_DOWNGRADE& aJob, REPORTER& aReporter )
{
    EESCHEMA_JOBS_HANDLER handler( nullptr );
    handler.SetReporter( &aReporter );
    return handler.JobDowngrade( &aJob );
}


using GOLDEN_NET_CONNECTIONS = std::map<std::pair<std::string, std::string>, std::array<std::string, 4>>;


static std::string netlistValue( const SEXPR::SEXPR& aNode, const std::string& aHead )
{
    for( const SEXPR::SEXPR* child : *aNode.GetChildren() )
    {
        if( KI_TEST::GoldenNodeHead( *child ) == aHead && child->GetNumberOfChildren() == 2 )
            return child->GetChild( 1 )->GetString();
    }

    throw std::runtime_error( "Missing netlist field " + aHead );
}


static GOLDEN_NET_CONNECTIONS netlistConnections( const std::string& aText )
{
    SEXPR::PARSER          parser;
    auto                   root = parser.Parse( aText );
    GOLDEN_NET_CONNECTIONS result;

    for( const SEXPR::SEXPR* nets : *root->GetChildren() )
    {
        if( KI_TEST::GoldenNodeHead( *nets ) != "nets" )
            continue;

        for( const SEXPR::SEXPR* net : *nets->GetChildren() )
        {
            if( KI_TEST::GoldenNodeHead( *net ) != "net" )
                continue;

            const std::string name = netlistValue( *net, "name" );
            const std::string netClass = netlistValue( *net, "class" );

            for( const SEXPR::SEXPR* node : *net->GetChildren() )
            {
                if( KI_TEST::GoldenNodeHead( *node ) != "node" )
                    continue;

                const auto key = std::make_pair( netlistValue( *node, "ref" ), netlistValue( *node, "pin" ) );
                if( !result.emplace( key,
                                     std::array<std::string, 4>{ name, netClass, netlistValue( *node, "pinfunction" ),
                                                                 netlistValue( *node, "pintype" ) } )
                             .second )
                {
                    throw std::runtime_error( "Duplicate reference/pin in golden netlist" );
                }
            }
        }
    }

    return result;
}


static void checkNativeSchematicGolden( const DOWNGRADE_TARGET& aTarget, const std::string& aRelease,
                                        bool aDropInsteadOfApproximate = false )
{
    KI_TEST::TEMPORARY_DIRECTORY        tmp( "kicad_qa_sch_downgrade_golden" );
    JOB_SCH_DOWNGRADE                   job;
    job.m_filename = downgradeDataPath() + "golden/current/body_styles.kicad_sch";
    job.m_outputDir = tmp.GetPath().string();
    job.m_target = aTarget.m_id;
    job.m_force = true;
    job.m_dropInsteadOfApproximate = aDropInsteadOfApproximate;
    WX_STRING_REPORTER reporter;
    BOOST_REQUIRE_EQUAL( runDowngradeJob( job, reporter ), CLI::EXIT_CODES::SUCCESS );
    const std::string output = ( tmp.GetPath() / "body_styles.kicad_sch" ).string();

    if( aRelease == "v10" )
    {
        BOOST_CHECK( !job.m_report.IsLossy() );
        const std::string difference =
                KI_TEST::GoldenFileDifference( downgradeDataPath() + "golden/v10/body_styles.kicad_sch", output );
        BOOST_CHECK_MESSAGE( difference.empty(), difference );
    }
    else
    {
        BOOST_CHECK_EQUAL( job.m_report.Count( DOWNGRADE_BUCKET::LOWER ), aDropInsteadOfApproximate ? 0 : 1 );
        BOOST_CHECK_EQUAL( job.m_report.Count( DOWNGRADE_BUCKET::DROP ), aDropInsteadOfApproximate ? 3 : 2 );
    }

    SETTINGS_MANAGER settings;
    settings.LoadProject( ( tmp.GetPath() / "body_styles.kicad_pro" ).string() );
    std::ifstream outputFile( output );
    auto          schematic = KI_TEST::ReadSchematicFromStream( outputFile, &settings.Prj() );
    schematic->RootScreen()->SetFileName( output );
    SCH_SHEET_LIST sheets = schematic->BuildSheetListSortedByPageNumbers();
    schematic->ConnectionGraph()->Recalculate( sheets, true );
    NETLIST_EXPORTER_KICAD exporter( schematic.get(), nullptr );
    STRING_FORMATTER       netlist;
    exporter.Format( &netlist, GNL_NETS );
    const auto actualConnections = netlistConnections( netlist.GetString() );
    auto       expectedConnections = netlistConnections(
            KI_TEST::ReadGoldenText( downgradeDataPath() + "golden/" + aRelease + "/body_styles.net" ) );

    // KiCad 9 records just the pin name. Newer netlist writers append its number.
    // Normalize only that known native-version difference in the reference. The
    // raw pin names are also checked directly against the independent design.
    if( aRelease == "v9" )
    {
        for( auto& [key, connection] : expectedConnections )
        {
            if( !connection[2].empty() )
                connection[2] += "_" + key.second;
        }
    }

    if( aRelease == "v9" && aDropInsteadOfApproximate )
    {
        // The independent native design still defines the unchanged supported subset.
        expectedConnections.erase( { "U1", "1" } );
        expectedConnections.erase( { "U1", "2" } );
        BOOST_CHECK( KI_TEST::ReadGoldenText( output ).find( "__KiCad9_body_" ) == std::string::npos );
    }
    BOOST_REQUIRE_EQUAL( expectedConnections.size(), aRelease == "v9" && aDropInsteadOfApproximate ? 3 : 5 );
    BOOST_CHECK( actualConnections == expectedConnections );

    // The independently authored v9 design uses plain symbols for the same drawings.
    // Compare active shapes and pin UUIDs as well as the real v9-generated netlist.
    SETTINGS_MANAGER           referenceSettings;
    std::unique_ptr<SCHEMATIC> reference;
    KI_TEST::LoadSchematic( referenceSettings, wxT( "../downgrade/golden/v9/body_styles" ), reference );
    std::map<KIID, SCH_SYMBOL*> expectedSymbols;
    for( SCH_ITEM* item : reference->RootScreen()->Items().OfType( SCH_SYMBOL_T ) )
        expectedSymbols.emplace( item->m_Uuid, static_cast<SCH_SYMBOL*>( item ) );

    size_t checked = 0;
    for( SCH_ITEM* item : schematic->RootScreen()->Items().OfType( SCH_SYMBOL_T ) )
    {
        SCH_SYMBOL* symbol = static_cast<SCH_SYMBOL*>( item );
        BOOST_REQUIRE( expectedSymbols.count( symbol->m_Uuid ) );
        SCH_SYMBOL* expected = expectedSymbols.at( symbol->m_Uuid );
        BOOST_CHECK( symbol->GetPosition() == expected->GetPosition() );
        BOOST_CHECK_EQUAL( symbol->GetOrientation(), expected->GetOrientation() );
        BOOST_CHECK_EQUAL( symbol->GetRawPins().size(), expected->GetRawPins().size() );

        for( const auto& pin : symbol->GetRawPins() )
        {
            const SCH_PIN* expectedPin = expected->GetPin( pin->GetNumber() );
            BOOST_REQUIRE( expectedPin != nullptr );
            BOOST_CHECK( pin->m_Uuid == expectedPin->m_Uuid );
            BOOST_CHECK( pin->GetPosition() == expectedPin->GetPosition() );
            BOOST_CHECK_EQUAL( pin->GetName(), expectedPin->GetName() );
        }

        auto activeShapes = []( SCH_SYMBOL* aSymbol )
        {
            std::vector<const SCH_SHAPE*> shapes;
            for( const SCH_ITEM& drawing : aSymbol->GetLibSymbolRef()->GetDrawItems() )
            {
                if( drawing.Type() == SCH_SHAPE_T
                    && ( !drawing.GetBodyStyle() || drawing.GetBodyStyle() == aSymbol->GetBodyStyle() ) )
                {
                    shapes.push_back( static_cast<const SCH_SHAPE*>( &drawing ) );
                }
            }
            return shapes;
        };

        const auto shapes = activeShapes( symbol );
        const auto expectedShapes = activeShapes( expected );
        BOOST_REQUIRE_EQUAL( shapes.size(), expectedShapes.size() );
        for( size_t i = 0; i < shapes.size(); ++i )
        {
            BOOST_CHECK( shapes[i]->GetShape() == expectedShapes[i]->GetShape() );
            BOOST_CHECK( shapes[i]->GetStart() == expectedShapes[i]->GetStart() );
            BOOST_CHECK( shapes[i]->GetEnd() == expectedShapes[i]->GetEnd() );
            BOOST_CHECK_EQUAL( shapes[i]->GetStroke().GetWidth(), expectedShapes[i]->GetStroke().GetWidth() );
            BOOST_CHECK( shapes[i]->GetStroke().GetLineStyle() == expectedShapes[i]->GetStroke().GetLineStyle() );
            BOOST_CHECK( shapes[i]->GetStroke().GetColor() == expectedShapes[i]->GetStroke().GetColor() );
            BOOST_CHECK( shapes[i]->GetFillMode() == expectedShapes[i]->GetFillMode() );
        }
        checked++;
    }
    BOOST_CHECK_EQUAL( checked, aRelease == "v9" && aDropInsteadOfApproximate ? 2 : 3 );
}


BOOST_AUTO_TEST_CASE( SchematicMatchesNativeKicad10Golden )
{
    checkNativeSchematicGolden( kicad10, "v10" );
}


BOOST_AUTO_TEST_CASE( BakedStylesMatchIndependentKicad9DesignAndNetlist )
{
    checkNativeSchematicGolden( kicad9, "v9" );
}


BOOST_AUTO_TEST_CASE( DroppedStylesKeepIndependentKicad9SupportedDesignAndNetlist )
{
    checkNativeSchematicGolden( kicad9, "v9", true );
}


BOOST_AUTO_TEST_CASE( DropApproximationsKeepsNativeKicad10SchematicGolden )
{
    checkNativeSchematicGolden( kicad10, "v10", true );
}


BOOST_AUTO_TEST_CASE( DropApproximationsDryRunAndConsentDoNotWrite )
{
    KI_TEST::TEMPORARY_DIRECTORY        tmp( "kicad_qa_sch_downgrade_golden" );
    JOB_SCH_DOWNGRADE                   job;
    BOOST_CHECK( !job.m_dropInsteadOfApproximate );
    job.m_filename = downgradeDataPath() + "golden/current/body_styles.kicad_sch";
    const std::string original = KI_TEST::ReadGoldenText( job.m_filename.ToStdString() );
    job.m_outputDir = tmp.GetPath().string();
    job.m_target = wxT( "9.0" );
    job.m_dropInsteadOfApproximate = true;
    job.m_dryRun = true;
    WX_STRING_REPORTER reporter;
    BOOST_REQUIRE_EQUAL( runDowngradeJob( job, reporter ), CLI::EXIT_CODES::SUCCESS );
    BOOST_CHECK_EQUAL( job.m_report.Count( DOWNGRADE_BUCKET::LOWER ), 0 );
    BOOST_CHECK_EQUAL( job.m_report.Count( DOWNGRADE_BUCKET::DROP ), 3 );
    BOOST_CHECK( std::filesystem::is_empty( tmp.GetPath() ) );
    job.m_dryRun = false;
    BOOST_CHECK_EQUAL( runDowngradeJob( job, reporter ), CLI::EXIT_CODES::ERR_UNKNOWN );
    BOOST_CHECK( std::filesystem::is_empty( tmp.GetPath() ) );
    BOOST_CHECK( KI_TEST::ReadGoldenText( job.m_filename.ToStdString() ) == original );
}


BOOST_AUTO_TEST_CASE( PositionFileExclusionRequiresConsent )
{
    KI_TEST::TEMPORARY_DIRECTORY        tmp( "kicad_qa_sch_downgrade_golden" );
    JOB_SCH_DOWNGRADE                   job;
    job.m_filename = downgradeDataPath() + "regressions/position_exclusion.kicad_sch";
    const std::string original = KI_TEST::ReadGoldenText( job.m_filename.ToStdString() );
    job.m_outputDir = tmp.GetPath().string();
    job.m_target = wxT( "9.0" );
    WX_STRING_REPORTER reporter;
    BOOST_CHECK_EQUAL( runDowngradeJob( job, reporter ), CLI::EXIT_CODES::ERR_UNKNOWN );
    BOOST_CHECK_EQUAL( job.m_report.Count( DOWNGRADE_BUCKET::DROP ), 1 );
    BOOST_CHECK( job.m_report.IsLossy() );
    BOOST_CHECK( std::filesystem::is_empty( tmp.GetPath() ) );

    job.m_force = true;
    BOOST_REQUIRE_EQUAL( runDowngradeJob( job, reporter ), CLI::EXIT_CODES::SUCCESS );
    const std::string output = ( tmp.GetPath() / "position_exclusion.kicad_sch" ).string();
    BOOST_CHECK( KI_TEST::ReadGoldenText( output ).find( "in_pos_files" ) == std::string::npos );
    BOOST_CHECK( KI_TEST::ReadGoldenText( job.m_filename.ToStdString() ) == original );
}


BOOST_AUTO_TEST_CASE( IncompleteSecondaryHierarchyFailsBeforeWriting )
{
    const std::string fixture = downgradeDataPath() + "regressions/incomplete_hierarchy/";
    for( int failure = 0; failure < 3; ++failure )
    {
        KI_TEST::TEMPORARY_DIRECTORY tmp( "kicad_qa_sch_downgrade_golden" );
        for( const std::string& name :
             { "project.kicad_pro", "project.kicad_sch", "sibling.kicad_sch", "broken.kicad_sch" } )
        {
            // 0: malformed child, 1: missing child, 2: missing secondary top-level root.
            if( ( failure >= 1 && name == "broken.kicad_sch" ) || ( failure == 2 && name == "sibling.kicad_sch" ) )
            {
                continue;
            }
            BOOST_REQUIRE( wxCopyFile( fixture + name, ( tmp.GetPath() / name ).string() ) );
        }

        for( bool dryRun : { false, true } )
        {
            JOB_SCH_DOWNGRADE job;
            job.m_filename = ( tmp.GetPath() / "project.kicad_sch" ).string();
            job.m_outputDir = ( tmp.GetPath() / "out" ).string();
            job.m_target = wxT( "10.0" );
            job.m_force = true;
            job.m_dryRun = dryRun;
            WX_STRING_REPORTER reporter;
            BOOST_CHECK_EQUAL( runDowngradeJob( job, reporter ), CLI::EXIT_CODES::ERR_INVALID_INPUT_FILE );
            BOOST_CHECK( reporter.HasMessageOfSeverity( RPT_SEVERITY_ERROR ) );
            BOOST_CHECK( !std::filesystem::exists( tmp.GetPath() / "out" ) );
            BOOST_CHECK( KI_TEST::ReadGoldenText( ( tmp.GetPath() / "project.kicad_sch" ).string() )
                         == KI_TEST::ReadGoldenText( fixture + "project.kicad_sch" ) );
        }
    }
}


BOOST_AUTO_TEST_SUITE_END()
