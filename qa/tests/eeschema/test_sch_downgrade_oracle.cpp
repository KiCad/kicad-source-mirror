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

// Schematic version of the board oracle in pcbnew. A downgraded schematic must use only node heads
// the target release knows, taken from its real keyword files under qa/data/downgrade.

#include <boost/test/unit_test.hpp>

#include <algorithm>
#include <array>
#include <cctype>
#include <fstream>
#include <functional>
#include <iterator>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

#include <bitmap_base.h>
#include <embedded_files.h>
#include <font/font.h>
#include <base_units.h>
#include <mmh3_hash.h>
#include <schematic.h>
#include <sch_bitmap.h>
#include <sch_connection.h>
#include <sch_group.h>
#include <sch_rule_area.h>
#include <sch_screen.h>
#include <sch_sheet.h>
#include <lib_symbol.h>
#include <sch_symbol.h>
#include <sch_pin.h>
#include <sch_shape.h>
#include <sch_line.h>
#include <sch_textbox.h>
#include <connection_graph.h>
#include <eeschema_jobs_handler.h>
#include <jobs/job_sch_downgrade.h>
#include <cli/exit_codes.h>
#include <netlist_exporters/netlist_exporter_kicad.h>
#include <richio.h>
#include <downgrade/sch_downgrade.h>
#include <downgrade_scan.h>
#include <downgrade_target.h>
#include <sch_io/kicad_sexpr/sch_io_kicad_sexpr.h>
#include <project/project_file.h>
#include <settings/settings_manager.h>

#include <wx/filefn.h>
#include <wx/filename.h>
#include <wx/image.h>
#include <wx/imagpng.h>
#include <wx/mstream.h>

#include <qa_utils/downgrade_oracle_utils.h>
#include <qa_utils/downgrade_golden_utils.h>
#include <qa_utils/temporary_directory.h>
#include <qa_utils/wx_utils/unit_test_utils.h>
#include <eeschema_test_utils.h>
#include <schematic_file_util.h>


BOOST_AUTO_TEST_SUITE( SchDowngradeOracle )

static const DOWNGRADE_TARGET& kicad9 = KI_TEST::RequireDowngradeTarget( wxT( "9.0" ) );
static const DOWNGRADE_TARGET& kicad10 = KI_TEST::RequireDowngradeTarget( wxT( "10.0" ) );


static void checkExportUsesOnlyKnownTokens( const DOWNGRADE_TARGET& aTarget, const std::string& aTokenFileName )
{
    SETTINGS_MANAGER settingsManager;

    std::unique_ptr<SCHEMATIC> schematic;
    KI_TEST::LoadSchematic( settingsManager, wxT( "NoConnectOnPin" ), schematic );
    BOOST_REQUIRE( schematic != nullptr );

    SCH_SCREENS allScreens( schematic->Root() );

    for( SCH_SCREEN* screen = allScreens.GetFirst(); screen; screen = allScreens.GetNext() )
        DowngradeScreenInPlace( screen, aTarget );

    wxString dest = wxFileName::CreateTempFileName( wxT( "kicad_qa_sch_downgrade" ) );
    wxRemoveFile( dest );
    dest += wxT( ".kicad_sch" );

    SaveSchematicForTarget( schematic->GetTopLevelSheet(), schematic.get(), dest, aTarget );

    std::ifstream file( dest.ToStdString() );
    std::string   content( ( std::istreambuf_iterator<char>( file ) ), std::istreambuf_iterator<char>() );
    file.close();
    wxRemoveFile( dest );

    // The stamp proves the dispatch picked the right writer, not just a parseable one.
    BOOST_CHECK_MESSAGE( content.find( "(version " + std::to_string( aTarget.m_schVersion ) + ")" )
                                 != std::string::npos,
                         "Downgraded schematic does not carry the target version stamp" );

    const std::string           tokenPath = KI_TEST::GetEeschemaTestDataDir() + "/../downgrade/" + aTokenFileName;
    const std::set<std::string> known = KI_TEST::LoadTokenSet( tokenPath );
    BOOST_REQUIRE( !known.empty() );

    std::vector<std::string> unsupported = KI_TEST::UnknownSexprHeads( content, known );

    if( !unsupported.empty() )
    {
        std::string msg = "Downgraded schematic uses tokens " + aTarget.m_name.ToStdString() + " cannot parse:";

        for( const std::string& token : unsupported )
            msg += " " + token;

        BOOST_ERROR( msg );
    }
}


BOOST_AUTO_TEST_CASE( SchToKicad9UsesOnlyKnownTokens )
{
    checkExportUsesOnlyKnownTokens( kicad9, "kicad9_sch_tokens.txt" );
}


BOOST_AUTO_TEST_CASE( SchToKicad10UsesOnlyKnownTokens )
{
    checkExportUsesOnlyKnownTokens( kicad10, "kicad10_sch_tokens.txt" );
}


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


static SCH_BITMAP* addTransformFeatures( SCH_SCREEN& aScreen )
{
    LIB_SYMBOL lib( wxT( "TransformSymbol" ) );
    lib.SetBodyStyleNames( { wxT( "first" ), wxT( "second" ), wxT( "third" ) } );
    lib.SetAssociatedFootprints( { { LIB_ID( wxT( "Lib" ), wxT( "FP" ) ), wxT( "map" ) } } );
    lib.SetDuplicatePinNumbersAreJumpers( true );
    lib.SetExcludedFromPosFiles( true );
    lib.SetCustomProperty( wxT( "Vendor" ), wxT( "Acme" ) );
    lib.GetField( FIELD_T::VALUE )->SetCustomProperty( wxT( "Source" ), wxT( "Library" ) );

    for( int style = 1; style <= 3; ++style )
    {
        SCH_PIN* pin = new SCH_PIN( &lib );
        pin->SetNumber( wxT( "1" ) );
        pin->SetName( wxT( "SIGNAL" ) );
        pin->SetBodyStyle( style );
        pin->SetUnit( 1 );
        pin->SetPosition( VECTOR2I( schIUScale.mmToIU( 5 * style ), 0 ) );
        pin->SetCustomProperty( wxT( "Role" ), wxT( "Signal" ) );
        lib.AddDrawItem( pin );
    }

    for( bool inLibrary : { false, true } )
    {
        auto addShape = [&]( SCH_SHAPE* aShape )
        {
            if( inLibrary )
                lib.AddDrawItem( aShape );
            else
                aScreen.Append( aShape );
        };

        if( inLibrary )
        {
            SCH_SHAPE* ended = new SCH_SHAPE( SHAPE_T::POLY );
            ended->SetPolyPoints( { VECTOR2I(), VECTOR2I( schIUScale.mmToIU( 10 ), 0 ) } );
            ended->SetStroke( STROKE_PARAMS( schIUScale.mmToIU( 0.2 ), LINE_STYLE::SOLID ) );
            ended->SetEndEnding( LINE_ENDING( LINE_ENDING_STYLE::ARROW ) );
            addShape( ended );
        }
        else
        {
            SCH_LINE* ended = new SCH_LINE( VECTOR2I(), LAYER_NOTES );
            ended->SetEndPoint( VECTOR2I( schIUScale.mmToIU( 10 ), 0 ) );
            ended->SetStroke( STROKE_PARAMS( schIUScale.mmToIU( 0.2 ), LINE_STYLE::SOLID ) );
            ended->SetEndEnding( LINE_ENDING( LINE_ENDING_STYLE::ARROW ) );
            aScreen.Append( ended );
        }

        for( SHAPE_T type : { SHAPE_T::ELLIPSE, SHAPE_T::ELLIPSE_ARC } )
        {
            SCH_SHAPE* ellipse = new SCH_SHAPE( type );
            ellipse->SetEllipseCenter( VECTOR2I( 0, 0 ) );
            ellipse->SetEllipseMajorRadius( schIUScale.mmToIU( 5 ) );
            ellipse->SetEllipseMinorRadius( schIUScale.mmToIU( 3 ) );
            ellipse->SetEllipseStartAngle( ANGLE_0 );
            ellipse->SetEllipseEndAngle( ANGLE_90 );
            addShape( ellipse );
        }

        SCH_SHAPE* rounded = new SCH_SHAPE( SHAPE_T::RECTANGLE );
        rounded->SetStart( VECTOR2I( 0, 0 ) );
        rounded->SetEnd( VECTOR2I( schIUScale.mmToIU( 10 ), schIUScale.mmToIU( 6 ) ) );
        rounded->SetCornerRadius( schIUScale.mmToIU( 1 ) );
        addShape( rounded );

        SCH_SHAPE* hatch = new SCH_SHAPE( SHAPE_T::RECTANGLE );
        hatch->SetStart( VECTOR2I( 0, 0 ) );
        hatch->SetEnd( VECTOR2I( schIUScale.mmToIU( 10 ), schIUScale.mmToIU( 6 ) ) );
        hatch->SetFillMode( FILL_T::HATCH );
        addShape( hatch );
    }

    SCH_SYMBOL* third = nullptr;

    for( int style = 1; style <= 3; ++style )
    {
        SCH_SYMBOL* symbol = new SCH_SYMBOL( lib, lib.GetLibId(), nullptr, 1, style );
        symbol->SetExcludedFromPosFiles( true );
        symbol->SetCustomProperty( wxT( "Owner" ), wxT( "Placed" ) );
        aScreen.Append( symbol );

        if( style == 3 )
            third = symbol;
    }

    SCH_SHAPE* locked = new SCH_SHAPE( SHAPE_T::RECTANGLE );
    locked->SetStart( VECTOR2I( 0, 0 ) );
    locked->SetEnd( VECTOR2I( schIUScale.mmToIU( 2 ), schIUScale.mmToIU( 2 ) ) );
    locked->SetLocked( true );
    aScreen.Append( locked );

    SCH_TEXTBOX* textbox = new SCH_TEXTBOX();
    textbox->SetText( wxT( "Hatched text box" ) );
    textbox->SetStart( VECTOR2I( 0, 0 ) );
    textbox->SetEnd( VECTOR2I( schIUScale.mmToIU( 10 ), schIUScale.mmToIU( 6 ) ) );
    textbox->SetFillMode( FILL_T::HATCH );
    aScreen.Append( textbox );

    SCH_RULE_AREA* area = new SCH_RULE_AREA();
    area->SetPolyPoints( { VECTOR2I( 0, 0 ), VECTOR2I( schIUScale.mmToIU( 10 ), 0 ),
                           VECTOR2I( schIUScale.mmToIU( 10 ), schIUScale.mmToIU( 10 ) ), VECTOR2I( 0, 0 ) } );
    area->SetDNP( true );
    area->SetExcludedFromSim( true );
    area->SetFillMode( FILL_T::HATCH );
    aScreen.Append( area );

    SCH_GROUP* group = new SCH_GROUP( &aScreen );
    group->AddItem( third );
    group->AddItem( textbox );
    aScreen.Append( group );

    SCH_BITMAP*    bitmap = new SCH_BITMAP();
    wxMemoryBuffer png = KI_TEST::MakePngWithFractionalPixelsPerCm();
    BOOST_REQUIRE( bitmap->GetReferenceImage().ReadImageFile( png ) );
    bitmap->GetReferenceImage().SetImageScale( 2.0 );
    aScreen.Append( bitmap );
    return bitmap;
}


static void checkSchematicTransformCoverage( const COMPATIBILITY_REPORT& aReport, const DOWNGRADE_TARGET& aTarget,
                                             bool aDropInsteadOfApproximate )
{
    KI_TEST::CheckTransformCoverage(
            aReport, aTarget.m_schVersion,
            {
                    { wxT( "Selected custom body styles" ), 20250827, DOWNGRADE_BUCKET::LOWER },
                    { wxT( "Locked items" ), 20260326, DOWNGRADE_BUCKET::LOWER },
                    { wxT( "Ellipse graphics" ), 20260508, DOWNGRADE_BUCKET::LOWER },
                    { wxT( "Hatched shape fills" ), 20250222, DOWNGRADE_BUCKET::LOWER },
                    { wxT( "Pin-to-pad maps" ), 20260629, DOWNGRADE_BUCKET::DROP },
                    { wxT( "Groups" ), 20250513, DOWNGRADE_BUCKET::LOWER },
                    { wxT( "Jumper pin groups" ), 20250324, DOWNGRADE_BUCKET::DROP },
                    { wxT( "Named body styles" ), 20250827, DOWNGRADE_BUCKET::DROP },
                    { wxT( "Position file exclusions" ), 20260101, DOWNGRADE_BUCKET::DROP },
                    { wxT( "Rounded rectangles" ), 20250829, DOWNGRADE_BUCKET::LOWER },
                    { wxT( "Rule area flags" ), 20250610, DOWNGRADE_BUCKET::DROP },
                    { wxT( "Line endings" ), 20260818, DOWNGRADE_BUCKET::LOWER },
                    { wxT( "Custom user properties" ), 20260830, DOWNGRADE_BUCKET::DROP },
            },
            aDropInsteadOfApproximate );
}


static void checkTransformedSchematicUsesOnlyKnownTokens( const DOWNGRADE_TARGET& aTarget,
                                                          const std::string&      aTokenFileName )
{
    const auto known = KI_TEST::LoadTokenSet( downgradeDataPath() + aTokenFileName );
    BOOST_REQUIRE( !known.empty() );

    for( bool drop : { false, true } )
    {
        BOOST_TEST_CONTEXT( "Drop approximations: " << drop )
        {
            SETTINGS_MANAGER           settings;
            std::unique_ptr<SCHEMATIC> schematic;
            KI_TEST::LoadSchematic( settings, wxT( "NoConnectOnPin" ), schematic );
            BOOST_REQUIRE( schematic );
            SCH_BITMAP*            bitmap = addTransformFeatures( *schematic->RootScreen() );
            const REFERENCE_IMAGE& ref = bitmap->GetReferenceImage();
            BOOST_REQUIRE_NE( ref.GetImage().GetPPI(), ref.GetImage().GetLegacyPPI() );
            const double expectedScale = ref.GetImageScale() * ref.GetImage().GetLegacyPPI() / ref.GetImage().GetPPI();
            checkSchematicTransformCoverage( ClassifyScreenForDowngrade( schematic->RootScreen(), aTarget, drop ),
                                             aTarget, drop );

            KI_TEST::TEMPORARY_DIRECTORY tmp( "kicad_qa_sch_transform_oracle" );
            const std::string            src = ( tmp.GetPath() / "source.kicad_sch" ).string();
            SCH_IO_KICAD_SEXPR           io;
            io.SaveSchematicFile( src, schematic->GetTopLevelSheet(), schematic.get() );
            const std::string sourceContent = KI_TEST::ReadGoldenText( src );

            JOB_SCH_DOWNGRADE job;
            job.m_filename = src;
            job.m_outputDir = ( tmp.GetPath() / "export" ).string();
            job.m_target = aTarget.m_id;
            job.m_force = true;
            job.m_dropInsteadOfApproximate = drop;
            WX_STRING_REPORTER reporter;
            BOOST_REQUIRE_MESSAGE( runDowngradeJob( job, reporter ) == CLI::EXIT_CODES::SUCCESS,
                                   reporter.GetMessages().ToStdString() );
            checkSchematicTransformCoverage( job.m_report, aTarget, drop );

            const std::string dest = ( tmp.GetPath() / "export/source.kicad_sch" ).string();
            const std::string content = KI_TEST::ReadGoldenText( dest );
            const auto        unsupported = KI_TEST::UnknownSexprHeads( content, known );

            for( const std::string& token : unsupported )
                BOOST_ERROR( "Transformed schematic uses unknown token: " + token );

            BOOST_CHECK( content.find( "(version " + std::to_string( aTarget.m_schVersion ) + ")" )
                         != std::string::npos );
            BOOST_CHECK_CLOSE( KI_TEST::SerializedImageScale( content, bitmap->m_Uuid.AsStdString() ), expectedScale,
                               0.1 );
            std::ifstream outputFile( dest );
            auto          reloaded = KI_TEST::ReadSchematicFromStream( outputFile, &settings.Prj() );
            BOOST_REQUIRE( reloaded );
            BOOST_CHECK( !ClassifyScreenForDowngrade( reloaded->RootScreen(), aTarget, drop ).IsLossy() );
            BOOST_CHECK( KI_TEST::ReadGoldenText( src ) == sourceContent );
        }
    }
}


BOOST_AUTO_TEST_CASE( TransformedSchematicToKicad9UsesOnlyKnownTokens )
{
    checkTransformedSchematicUsesOnlyKnownTokens( kicad9, "kicad9_sch_tokens.txt" );
}


BOOST_AUTO_TEST_CASE( TransformedSchematicToKicad10UsesOnlyKnownTokens )
{
    checkTransformedSchematicUsesOnlyKnownTokens( kicad10, "kicad10_sch_tokens.txt" );
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
    job.m_outputDir = ( tmp.GetPath() / "export" ).string();
    job.m_target = aTarget.m_id;
    job.m_force = true;
    job.m_dropInsteadOfApproximate = aDropInsteadOfApproximate;
    WX_STRING_REPORTER reporter;
    BOOST_REQUIRE_EQUAL( runDowngradeJob( job, reporter ), CLI::EXIT_CODES::SUCCESS );
    const std::string output = ( tmp.GetPath() / "export/body_styles.kicad_sch" ).string();

    JOB_SCH_DOWNGRADE repeated;
    repeated.m_filename = job.m_filename;
    repeated.m_outputDir = ( tmp.GetPath() / "repeated" ).string();
    repeated.m_target = job.m_target;
    repeated.m_force = true;
    repeated.m_dropInsteadOfApproximate = aDropInsteadOfApproximate;
    BOOST_REQUIRE_EQUAL( runDowngradeJob( repeated, reporter ), CLI::EXIT_CODES::SUCCESS );
    BOOST_CHECK( KI_TEST::ReadGoldenText( output )
                 == KI_TEST::ReadGoldenText( ( tmp.GetPath() / "repeated/body_styles.kicad_sch" ).string() ) );

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


// A standalone symbol library never reaches the screen pass, so the symbol-scoped subset of the
// rule table gets its own native reference. The derived symbol sorts before its parent by name,
// which the format forbids, so the reference also pins the parents-first order.
static void checkNativeSymbolLibraryGolden( const DOWNGRADE_TARGET& aTarget, const std::string& aRelease )
{
    KI_TEST::TEMPORARY_DIRECTORY tmp( "kicad_qa_native_symlib_golden", "" );
    const wxString               dest = ( tmp.GetPath() / "metadata.kicad_sym" ).wstring();
    const wxString               src = wxString::FromUTF8( downgradeDataPath() + "golden/current/metadata.kicad_sym" );

    SCH_IO_KICAD_SEXPR       pi;
    std::vector<LIB_SYMBOL*> symbols;
    pi.EnumerateSymbolLib( symbols, src );
    BOOST_REQUIRE_EQUAL( symbols.size(), 3 );

    std::stable_sort( symbols.begin(), symbols.end(),
                      []( const LIB_SYMBOL* a, const LIB_SYMBOL* b )
                      {
                          return a->GetInheritanceDepth() < b->GetInheritanceDepth();
                      } );

    for( LIB_SYMBOL* symbol : symbols )
        DowngradeLibSymbolInPlace( symbol, aTarget );

    SaveSymbolLibraryForTarget( symbols, dest, aTarget );

    const std::string difference = KI_TEST::GoldenFileDifference(
            downgradeDataPath() + "golden/" + aRelease + "/metadata.kicad_sym", dest.ToStdString() );
    BOOST_CHECK_MESSAGE( difference.empty(), difference );
}


BOOST_AUTO_TEST_CASE( SymbolLibraryMatchesNativeKicad9Golden )
{
    checkNativeSymbolLibraryGolden( kicad9, "v9" );
}


BOOST_AUTO_TEST_CASE( SymbolLibraryMatchesNativeKicad10Golden )
{
    checkNativeSymbolLibraryGolden( kicad10, "v10" );
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


BOOST_FIXTURE_TEST_CASE( StandaloneKicad10ExportKeepsProjectBusAliasConnectivity, KI_TEST::SCHEMATIC_TEST_FIXTURE )
{
    KI_TEST::TEMPORARY_DIRECTORY tmp( "kicad_qa_bus_alias_export" );
    const std::string            fixture = downgradeDataPath() + "regressions/bus_alias_connectivity/";
    const std::string            source = fixture + "prefix_bus_alias.kicad_sch";
    const std::string            original = KI_TEST::ReadGoldenText( source );
    const std::string            originalProject = KI_TEST::ReadGoldenText( fixture + "prefix_bus_alias.kicad_pro" );
    JOB_SCH_DOWNGRADE            job;
    job.m_filename = source;
    job.m_outputDir = ( tmp.GetPath() / "export" ).string();
    job.m_target = kicad10.m_id;
    WX_STRING_REPORTER reporter;
    BOOST_REQUIRE_MESSAGE( runDowngradeJob( job, reporter ) == CLI::EXIT_CODES::SUCCESS,
                           reporter.GetMessages().ToStdString() );
    BOOST_CHECK( !job.m_report.IsLossy() );
    const std::string project = ( tmp.GetPath() / "export/prefix_bus_alias.kicad_pro" ).string();
    BOOST_REQUIRE( std::filesystem::exists( project ) );
    const auto document = nlohmann::json::parse( KI_TEST::ReadGoldenText( project ) );
    BOOST_CHECK( document.at( "schematic" ).at( "bus_aliases" ).at( "Bus1" )
                 == nlohmann::json( { "D[0..7]", "CTL1", "CTL2" } ) );

    wxFileName output( ( tmp.GetPath() / "export/prefix_bus_alias.kicad_sch" ).string() );
    LoadSchematic( output );
    BOOST_REQUIRE( m_schematic );
    SCH_SHEET_LIST                                          sheets = m_schematic->BuildSheetListSortedByPageNumbers();
    std::map<std::pair<std::string, std::string>, wxString> connections;

    for( const SCH_SHEET_PATH& sheet : sheets )
    {
        for( SCH_ITEM* item : sheet.LastScreen()->Items().OfType( SCH_SYMBOL_T ) )
        {
            SCH_SYMBOL*       symbol = static_cast<SCH_SYMBOL*>( item );
            const std::string reference = symbol->GetRef( &sheet ).ToStdString();

            for( SCH_PIN* pin : symbol->GetPins( &sheet ) )
            {
                const SCH_CONNECTION* connection = pin->Connection( &sheet );
                BOOST_REQUIRE( connection );
                BOOST_REQUIRE( connection->IsNet() );
                BOOST_CHECK( !connection->Name().IsEmpty() );
                BOOST_REQUIRE( connections
                                       .emplace( std::make_pair( reference, pin->GetNumber().ToStdString() ),
                                                 connection->Name() )
                                       .second );
            }
        }
    }

    BOOST_REQUIRE_EQUAL( connections.size(), 20 );
    std::set<wxString> netNames;

    for( int pin = 1; pin <= 10; ++pin )
    {
        const std::string number = std::to_string( pin );
        const auto&       first = connections.at( { "J1", number } );
        const auto&       second = connections.at( { "J2", number } );
        BOOST_CHECK( first == second );
        netNames.insert( first );
    }

    BOOST_CHECK_EQUAL( netNames.size(), 10 );
    BOOST_CHECK( KI_TEST::ReadGoldenText( source ) == original );
    BOOST_CHECK( KI_TEST::ReadGoldenText( fixture + "prefix_bus_alias.kicad_pro" ) == originalProject );
}


BOOST_FIXTURE_TEST_CASE( SelectedVariantPreviewMatchesExportedPositionExclusionLosses, KI_TEST::SCHEMATIC_TEST_FIXTURE )
{
    const std::string fixture = KI_TEST::GetEeschemaTestDataDir() + "/NoConnectOnPin.kicad_sch";
    const std::string originalFixture = KI_TEST::ReadGoldenText( fixture );

    for( bool baseExcluded : { false, true } )
    {
        BOOST_TEST_CONTEXT( "Base position-file exclusion: " << baseExcluded )
        {
            KI_TEST::TEMPORARY_DIRECTORY tmp( "kicad_qa_variant_preview" );
            const std::string            source = ( tmp.GetPath() / "variant.kicad_sch" ).string();
            BOOST_REQUIRE( wxCopyFile( fixture, source ) );
            LoadSchematic( wxFileName( source ) );
            SCH_SHEET_LIST paths = m_schematic->BuildUnorderedSheetList();
            BOOST_REQUIRE_EQUAL( paths.size(), 1 );
            auto symbols = paths[0].LastScreen()->Items().OfType( SCH_SYMBOL_T );
            BOOST_REQUIRE_EQUAL( std::distance( symbols.begin(), symbols.end() ), 1 );
            SCH_SYMBOL* symbol = static_cast<SCH_SYMBOL*>( *symbols.begin() );
            symbol->SetExcludedFromPosFiles( baseExcluded );
            SCH_SYMBOL_VARIANT variant( wxT( "V1" ) );
            variant.InitializeAttributes( *symbol );
            variant.m_ExcludedFromPosFiles = !baseExcluded;
            symbol->AddVariant( paths[0], variant );
            BOOST_REQUIRE( symbol->GetVariant( paths[0], variant.m_Name ).has_value() );
            m_schematic->AddVariant( variant.m_Name );
            SCH_IO_KICAD_SEXPR io;
            io.SaveSchematicFile( source, paths[0].Last(), m_schematic.get() );
            const std::string original = KI_TEST::ReadGoldenText( source );
            BOOST_REQUIRE( original.find( "(variant" ) != std::string::npos );

            JOB_SCH_DOWNGRADE baseJob;
            baseJob.m_filename = source;
            baseJob.m_outputDir = ( tmp.GetPath() / "base-preview" ).string();
            baseJob.m_target = kicad9.m_id;
            baseJob.m_dryRun = true;
            WX_STRING_REPORTER reporter;
            BOOST_REQUIRE_EQUAL( runDowngradeJob( baseJob, reporter ), CLI::EXIT_CODES::SUCCESS );
            BOOST_CHECK_EQUAL( baseJob.m_report.Count( DOWNGRADE_BUCKET::DROP ), baseExcluded ? 2 : 1 );
            BOOST_CHECK( !std::filesystem::exists( tmp.GetPath() / "base-preview" ) );

            JOB_SCH_DOWNGRADE selectedJob;
            selectedJob.m_filename = source;
            selectedJob.m_outputDir = ( tmp.GetPath() / "selected" ).string();
            selectedJob.m_target = kicad9.m_id;
            selectedJob.m_variant = variant.m_Name;
            selectedJob.m_dryRun = true;
            BOOST_REQUIRE_EQUAL( runDowngradeJob( selectedJob, reporter ), CLI::EXIT_CODES::SUCCESS );
            const int selectedDrops = baseExcluded ? 1 : 2;
            BOOST_CHECK_EQUAL( selectedJob.m_report.Count( DOWNGRADE_BUCKET::DROP ), selectedDrops );
            BOOST_CHECK_EQUAL( selectedJob.m_report.Count( DOWNGRADE_BUCKET::LOWER ), 0 );
            BOOST_CHECK( !selectedJob.m_report.IsBlocked() );
            BOOST_CHECK_EQUAL( selectedJob.m_foundVariants.size(), 1 );
            BOOST_CHECK( !std::filesystem::exists( tmp.GetPath() / "selected" ) );

            selectedJob.m_dryRun = false;
            selectedJob.m_force = true;
            BOOST_REQUIRE_MESSAGE( runDowngradeJob( selectedJob, reporter ) == CLI::EXIT_CODES::SUCCESS,
                                   reporter.GetMessages().ToStdString() );
            BOOST_CHECK_EQUAL( selectedJob.m_report.Count( DOWNGRADE_BUCKET::DROP ), selectedDrops );
            BOOST_CHECK( std::filesystem::exists( tmp.GetPath() / "selected/variant.kicad_sch" ) );
            BOOST_CHECK( KI_TEST::ReadGoldenText( source ) == original );
            BOOST_CHECK( !std::filesystem::exists( tmp.GetPath() / "variant.kicad_pro" ) );
        }
    }

    BOOST_CHECK( KI_TEST::ReadGoldenText( fixture ) == originalFixture );
}


BOOST_FIXTURE_TEST_CASE( SharedSheetVariantConflictRefusesPreviewAndExportBeforeWriting,
                         KI_TEST::SCHEMATIC_TEST_FIXTURE )
{
    KI_TEST::TEMPORARY_DIRECTORY tmp( "kicad_qa_variant_conflict" );
    const std::string            fixture = KI_TEST::GetEeschemaTestDataDir() + "/NoConnectOnPin.kicad_sch";
    const std::string            originalFixture = KI_TEST::ReadGoldenText( fixture );
    const std::string            source = ( tmp.GetPath() / "variant.kicad_sch" ).string();
    const std::string            childSource = ( tmp.GetPath() / "shared.kicad_sch" ).string();
    BOOST_REQUIRE( wxCopyFile( fixture, source ) );
    LoadSchematic( wxFileName( source ) );
    SCH_SHEET*  root = m_schematic->GetTopLevelSheet( 0 );
    SCH_SCREEN* sharedScreen = root->GetScreen();
    auto        symbols = sharedScreen->Items().OfType( SCH_SYMBOL_T );
    BOOST_REQUIRE_EQUAL( std::distance( symbols.begin(), symbols.end() ), 1 );
    SCH_SYMBOL*    symbol = static_cast<SCH_SYMBOL*>( *symbols.begin() );
    SCH_SHEET_PATH rootPath = m_schematic->BuildUnorderedSheetList()[0];
    symbol->RemoveInstance( rootPath );
    SCH_SHEET* first = new SCH_SHEET( m_schematic.get() );
    SCH_SHEET* second = new SCH_SHEET( m_schematic.get() );
    first->SetScreen( sharedScreen );
    second->SetScreen( sharedScreen );
    first->SetName( wxT( "First" ) );
    second->SetName( wxT( "Second" ) );
    first->SetFileName( wxT( "shared.kicad_sch" ) );
    second->SetFileName( wxT( "shared.kicad_sch" ) );
    sharedScreen->SetFileName( childSource );
    root->SetScreen( new SCH_SCREEN( m_schematic.get() ) );
    const_cast<KIID&>( root->m_Uuid ) = root->GetScreen()->GetUuid();
    root->GetScreen()->SetFileName( source );
    root->GetScreen()->Append( first );
    root->GetScreen()->Append( second );
    m_schematic->RefreshHierarchy();
    SCH_SHEET_LIST paths = m_schematic->BuildUnorderedSheetList();
    BOOST_REQUIRE_EQUAL( paths.size(), 3 );
    int reference = 1;

    for( SCH_SHEET_PATH& path : paths )
    {
        if( path.LastScreen() != sharedScreen )
            continue;

        path.SetPageNumber( wxString::Format( wxT( "%d" ), reference + 1 ) );
        SCH_SYMBOL_INSTANCE instance;
        instance.m_Path = path.Path();
        instance.m_Reference = wxString::Format( wxT( "TP%d" ), reference++ );
        symbol->AddHierarchicalReference( instance );

        if( path.Last() == first )
        {
            SCH_SYMBOL_VARIANT variant( wxT( "V1" ) );
            variant.InitializeAttributes( *symbol );
            variant.m_DNP = true;
            symbol->AddVariant( path, variant );
        }
    }

    BOOST_REQUIRE_EQUAL( reference, 3 );
    m_schematic->AddVariant( wxT( "V1" ) );
    SCH_IO_KICAD_SEXPR io;
    io.SaveSchematicFile( source, root, m_schematic.get() );
    io.SaveSchematicFile( childSource, first, m_schematic.get() );
    const std::string original = KI_TEST::ReadGoldenText( source );
    const std::string originalChild = KI_TEST::ReadGoldenText( childSource );
    LoadSchematic( wxFileName( source ) );
    wxString conflict;
    BOOST_REQUIRE( !FlattenSchematicVariant( *m_schematic, wxT( "V1" ), &conflict ) );

    for( bool dryRun : { true, false } )
    {
        JOB_SCH_DOWNGRADE job;
        job.m_filename = source;
        job.m_outputDir = ( tmp.GetPath() / "out" ).string();
        job.m_target = kicad9.m_id;
        job.m_variant = wxT( "V1" );
        job.m_force = true;
        job.m_dryRun = dryRun;
        WX_STRING_REPORTER reporter;
        BOOST_CHECK_EQUAL( runDowngradeJob( job, reporter ), CLI::EXIT_CODES::ERR_UNKNOWN );
        BOOST_CHECK( reporter.HasMessageOfSeverity( RPT_SEVERITY_ERROR ) );
        BOOST_CHECK( !std::filesystem::exists( tmp.GetPath() / "out" ) );
        BOOST_CHECK( KI_TEST::ReadGoldenText( source ) == original );
        BOOST_CHECK( KI_TEST::ReadGoldenText( childSource ) == originalChild );
        BOOST_CHECK( !std::filesystem::exists( tmp.GetPath() / "variant.kicad_pro" ) );
    }

    BOOST_CHECK( KI_TEST::ReadGoldenText( fixture ) == originalFixture );
}


BOOST_AUTO_TEST_CASE( DropApproximationsDryRunAndConsentDoNotWrite )
{
    KI_TEST::TEMPORARY_DIRECTORY        tmp( "kicad_qa_sch_downgrade_golden" );
    JOB_SCH_DOWNGRADE                   job;
    BOOST_CHECK( !job.m_dropInsteadOfApproximate );
    job.m_filename = downgradeDataPath() + "golden/current/body_styles.kicad_sch";
    const std::string original = KI_TEST::ReadGoldenText( job.m_filename.ToStdString() );
    job.m_outputDir = ( tmp.GetPath() / "export" ).string();
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


BOOST_AUTO_TEST_CASE( PublicSchematicJobsRequireFreshOutputsBeforeAnyWrite )
{
    KI_TEST::TEMPORARY_DIRECTORY tmp( "kicad_qa_sch_copy_only" );
    const auto                   source = tmp.GetPath() / "source.kicad_sch";
    const auto                   existing = tmp.GetPath() / "existing";
    const auto fixture = std::filesystem::path( KI_TEST::GetEeschemaTestDataDir() ) / "NoConnectOnPin.kicad_sch";
    std::filesystem::copy_file( fixture, source );
    std::filesystem::create_directory( existing );
    std::filesystem::copy_file( fixture, existing / source.filename() );
    const std::string original = KI_TEST::ReadGoldenText( source.string() );

    for( const auto& target : { kicad9, kicad10 } )
    {
        for( bool drop : { false, true } )
        {
            JOB_SCH_DOWNGRADE job;
            BOOST_CHECK( !job.m_inPlace );
            job.m_filename = source.string();
            job.m_target = target.m_id;
            job.m_force = true;
            job.m_dropInsteadOfApproximate = drop;
            WX_STRING_REPORTER reporter;
            BOOST_CHECK_EQUAL( runDowngradeJob( job, reporter ), CLI::EXIT_CODES::ERR_ARGS );
            BOOST_CHECK( KI_TEST::ReadGoldenText( source.string() ) == original );
            job.m_outputDir = existing.string();
            BOOST_CHECK_EQUAL( runDowngradeJob( job, reporter ), CLI::EXIT_CODES::ERR_ARGS );
            BOOST_CHECK( KI_TEST::ReadGoldenText( source.string() ) == original );
            BOOST_CHECK( KI_TEST::ReadGoldenText( ( existing / source.filename() ).string() ) == original );
            BOOST_CHECK_EQUAL( std::distance( std::filesystem::directory_iterator( existing ),
                                              std::filesystem::directory_iterator() ),
                               1 );
        }
    }
}


BOOST_AUTO_TEST_CASE( PositionFileExclusionRequiresConsent )
{
    KI_TEST::TEMPORARY_DIRECTORY        tmp( "kicad_qa_sch_downgrade_golden" );
    JOB_SCH_DOWNGRADE                   job;
    job.m_filename = downgradeDataPath() + "regressions/position_exclusion.kicad_sch";
    const std::string original = KI_TEST::ReadGoldenText( job.m_filename.ToStdString() );
    job.m_outputDir = ( tmp.GetPath() / "export" ).string();
    job.m_target = wxT( "9.0" );
    WX_STRING_REPORTER reporter;
    BOOST_CHECK_EQUAL( runDowngradeJob( job, reporter ), CLI::EXIT_CODES::ERR_UNKNOWN );
    BOOST_CHECK_EQUAL( job.m_report.Count( DOWNGRADE_BUCKET::DROP ), 1 );
    BOOST_CHECK( job.m_report.IsLossy() );
    BOOST_CHECK( std::filesystem::is_empty( tmp.GetPath() ) );

    job.m_force = true;
    BOOST_REQUIRE_EQUAL( runDowngradeJob( job, reporter ), CLI::EXIT_CODES::SUCCESS );
    const std::string output = ( tmp.GetPath() / "export/position_exclusion.kicad_sch" ).string();
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


static std::vector<const SEXPR::SEXPR*> rawNodes( const SEXPR::SEXPR& aRoot, const std::string& aHead )
{
    std::vector<const SEXPR::SEXPR*>           nodes;
    std::function<void( const SEXPR::SEXPR& )> visit = [&]( const SEXPR::SEXPR& node )
    {
        if( !node.IsList() )
            return;
        if( KI_TEST::GoldenNodeHead( node ) == aHead )
            nodes.push_back( &node );
        for( const SEXPR::SEXPR* child : *node.GetChildren() )
            visit( *child );
    };
    visit( aRoot );
    return nodes;
}


static double rawNumber( const SEXPR::SEXPR& aNode )
{
    return aNode.IsInteger() ? aNode.GetLongInteger() : aNode.GetDouble();
}


BOOST_AUTO_TEST_CASE( NormalizedClosedPolygonsKeepClosingEdgeInEveryWriterPath )
{
    for( const auto& target : { kicad9, kicad10 } )
    {
        KI_TEST::TEMPORARY_DIRECTORY tmp( "kicad_qa_closed_polygon_writer" );
        SETTINGS_MANAGER             settings;
        std::unique_ptr<SCHEMATIC>   schematic;
        KI_TEST::LoadSchematic( settings, wxT( "NoConnectOnPin" ), schematic );
        SCH_SHAPE polygon( SHAPE_T::POLY );
        polygon.SetPolyPoints(
                { VECTOR2I(), VECTOR2I( schIUScale.mmToIU( 10 ), 0 ), VECTOR2I( 0, schIUScale.mmToIU( 10 ) ) } );
        polygon.GetPolyShape().Outline( 0 ).SetClosed( true );
        polygon.SetFillMode( FILL_T::NO_FILL );
        schematic->RootScreen()->Append( new SCH_SHAPE( polygon ) );
        LIB_SYMBOL* lib = new LIB_SYMBOL( wxT( "closed_polygon" ) );
        lib->AddDrawItem( new SCH_SHAPE( polygon ) );
        schematic->RootScreen()->AddLibSymbol( lib );
        const std::string libraryPath = ( tmp.GetPath() / "polygon.kicad_sym" ).string();
        const std::string schematicPath = ( tmp.GetPath() / "polygon.kicad_sch" ).string();
        SaveSymbolLibraryForTarget( { lib }, libraryPath, target );
        SaveSchematicForTarget( schematic->GetTopLevelSheet(), schematic.get(), schematicPath, target );
        for( const auto& path : { libraryPath, schematicPath } )
        {
            SEXPR::PARSER parser;
            auto          root = parser.Parse( KI_TEST::ReadGoldenText( path ) );
            const auto    polylines = rawNodes( *root, "polyline" );
            BOOST_REQUIRE( !polylines.empty() );
            size_t closedCount = 0;
            for( const auto* polyline : polylines )
            {
                const auto* points = KI_TEST::FindGoldenChild( *polyline, "pts" );
                if( !points || points->GetNumberOfChildren() < 4 )
                    continue;
                BOOST_REQUIRE_EQUAL( points->GetNumberOfChildren(), 5 );
                const auto* first = points->GetChild( 1 );
                const auto* last = points->GetChild( 4 );
                BOOST_CHECK_EQUAL( rawNumber( *first->GetChild( 1 ) ), rawNumber( *last->GetChild( 1 ) ) );
                BOOST_CHECK_EQUAL( rawNumber( *first->GetChild( 2 ) ), rawNumber( *last->GetChild( 2 ) ) );
                ++closedCount;
            }
            BOOST_CHECK_EQUAL( closedCount, path == libraryPath ? 1 : 2 );
        }
    }
}


BOOST_AUTO_TEST_CASE( ExplicitBoldStrokeThicknessUsesLegacyMeaningWithoutMutatingSource )
{
    for( const auto& target : { kicad9, kicad10 } )
    {
        KI_TEST::TEMPORARY_DIRECTORY tmp( "kicad_qa_bold_writer" );
        LIB_SYMBOL                   lib( wxT( "bold_text" ) );
        SCH_FIELD*                   field = lib.GetField( FIELD_T::VALUE );
        field->SetBold( true );
        field->SetTextThickness( 100000 );
        const std::string path = ( tmp.GetPath() / "bold.kicad_sym" ).string();
        SaveSymbolLibraryForTarget( { &lib }, path, target );
        SEXPR::PARSER parser;
        auto          root = parser.Parse( KI_TEST::ReadGoldenText( path ) );
        auto          widths = rawNodes( *root, "thickness" );
        BOOST_REQUIRE_EQUAL( widths.size(), 1 );
        BOOST_CHECK_CLOSE( rawNumber( *widths.front()->GetChild( 1 ) ), 160000.0 / schIUScale.IU_PER_MM, 0.00001 );
        BOOST_CHECK_EQUAL( field->GetTextThickness(), 100000 );
        field->SetTextThickness( 0 );
        SaveSymbolLibraryForTarget( { &lib }, path, target );
        auto automatic = parser.Parse( KI_TEST::ReadGoldenText( path ) );
        BOOST_CHECK( rawNodes( *automatic, "thickness" ).empty() );
        BOOST_CHECK_EQUAL( field->GetTextThickness(), 0 );
        KIFONT::FONT* outline = KIFONT::FONT::GetFont( wxT( "Arial" ), true );
        if( !outline->IsStroke() )
        {
            field->SetFont( outline );
            field->SetTextThickness( 100000 );
            SaveSymbolLibraryForTarget( { &lib }, path, target );
            auto outlined = parser.Parse( KI_TEST::ReadGoldenText( path ) );
            auto outlineWidths = rawNodes( *outlined, "thickness" );
            BOOST_REQUIRE_EQUAL( outlineWidths.size(), 1 );
            BOOST_CHECK_CLOSE( rawNumber( *outlineWidths.front()->GetChild( 1 ) ), 100000.0 / schIUScale.IU_PER_MM,
                               0.00001 );
            BOOST_CHECK_EQUAL( field->GetTextThickness(), 100000 );
        }
    }
}


BOOST_AUTO_TEST_CASE( DerivedSymbolKeywordsPreserveRawOverridesInsteadOfInheritedValues )
{
    for( const auto& target : { kicad9, kicad10 } )
    {
        KI_TEST::TEMPORARY_DIRECTORY tmp( "kicad_qa_derived_keywords" );
        LIB_SYMBOL                   parent( wxT( "parent" ) );
        parent.SetKeyWords( wxT( "parent inherited keywords" ) );
        LIB_SYMBOL empty( wxT( "empty_derived" ) );
        empty.SetParent( &parent );
        LIB_SYMBOL explicitKeywords( wxT( "explicit_derived" ) );
        explicitKeywords.SetParent( &parent );
        explicitKeywords.SetKeyWords( wxT( "child raw keywords" ) );
        BOOST_REQUIRE( empty.GetRawKeyWords().IsEmpty() );
        BOOST_REQUIRE( empty.GetKeyWords() == parent.GetKeyWords() );
        const std::string path = ( tmp.GetPath() / "derived.kicad_sym" ).string();
        SaveSymbolLibraryForTarget( { &parent, &empty, &explicitKeywords }, path, target );
        SCH_IO_KICAD_SEXPR io;
        LIB_SYMBOL*        reloadedEmpty = io.LoadSymbol( path, empty.GetName() );
        LIB_SYMBOL*        reloadedExplicit = io.LoadSymbol( path, explicitKeywords.GetName() );
        BOOST_REQUIRE( reloadedEmpty );
        BOOST_REQUIRE( reloadedExplicit );
        BOOST_CHECK( reloadedEmpty->GetRawKeyWords().IsEmpty() );
        BOOST_CHECK( reloadedExplicit->GetRawKeyWords() == wxT( "child raw keywords" ) );
    }
}


BOOST_AUTO_TEST_CASE( EmbeddedAttachmentsUseLegacyChecksumsInSchematicCacheAndLibraryFiles )
{
    for( const auto& target : { kicad9, kicad10 } )
    {
        for( size_t payloadSize : { size_t( 13 ), size_t( 4099 ) } )
        {
            BOOST_TEST_CONTEXT( "target=" << target.m_id.ToStdString() << " payload_size=" << payloadSize )
            {
                KI_TEST::TEMPORARY_DIRECTORY tmp( "kicad_qa_embedded_schematic_writer" );
                SETTINGS_MANAGER             settings;
                std::unique_ptr<SCHEMATIC>   schematic;
                KI_TEST::LoadSchematic( settings, wxT( "NoConnectOnPin" ), schematic );
                BOOST_REQUIRE( schematic );

                auto attachment = std::make_shared<EMBEDDED_FILES::EMBEDDED_FILE>();
                attachment->name = wxT( "nonaligned.bin" );

                for( size_t i = 0; i < payloadSize; ++i )
                    attachment->decompressedData.push_back( static_cast<char>( ( i * 131 + i / 251 ) % 256 ) );

                BOOST_REQUIRE( EMBEDDED_FILES::CompressAndEncode( *attachment ) == EMBEDDED_FILES::RETURN_CODE::OK );
                BOOST_REQUIRE( attachment->Validate() );
                const auto        originalPayload = attachment->decompressedData;
                const std::string originalEncoded = attachment->compressedEncodedData;
                const std::string originalHash = attachment->data_hash;
                MMH3_HASH         legacyHash( EMBEDDED_FILES::Seed() );
                legacyHash.addDataV1( reinterpret_cast<const uint8_t*>( originalPayload.data() ),
                                      originalPayload.size() );
                const std::string expectedHash = legacyHash.digest().ToString();
                BOOST_REQUIRE_NE( expectedHash, originalHash );

                schematic->GetEmbeddedFiles()->AddFile( attachment );
                LIB_SYMBOL* lib = new LIB_SYMBOL( wxT( "attached_symbol" ) );
                lib->GetEmbeddedFiles()->AddFile( attachment );
                const wxString cacheKey = lib->GetLibId().Format().wx_str();
                schematic->RootScreen()->AddLibSymbol( lib );

                auto checkSourceUnchanged = [&]()
                {
                    BOOST_CHECK( attachment->decompressedData == originalPayload );
                    BOOST_CHECK_EQUAL( attachment->compressedEncodedData, originalEncoded );
                    BOOST_CHECK_EQUAL( attachment->data_hash, originalHash );
                    BOOST_CHECK( attachment->is_valid );
                };

                auto checkSerializedChecksum = [&]( const std::string& path, size_t expectedCount )
                {
                    const std::string content = KI_TEST::ReadGoldenText( path );
                    const std::string checksum = "(checksum \"" + expectedHash + "\")";
                    size_t            count = 0;

                    for( size_t pos = content.find( checksum ); pos != std::string::npos;
                         pos = content.find( checksum, pos + checksum.size() ) )
                    {
                        ++count;
                    }

                    BOOST_CHECK_EQUAL( count, expectedCount );
                    BOOST_CHECK( content.find( "(checksum \"" + originalHash + "\")" ) == std::string::npos );
                };

                auto checkReloadedAttachment = [&]( const EMBEDDED_FILES& files )
                {
                    auto* reloaded = files.GetEmbeddedFile( attachment->name );
                    BOOST_REQUIRE( reloaded );
                    BOOST_CHECK( reloaded->Validate() );
                    BOOST_CHECK( reloaded->decompressedData == originalPayload );
                    BOOST_CHECK_EQUAL( reloaded->compressedEncodedData, originalEncoded );
                    BOOST_CHECK_EQUAL( reloaded->data_hash, originalHash );
                };

                const std::string libraryPath = ( tmp.GetPath() / "attached_symbol.kicad_sym" ).string();
                const std::string schematicPath = ( tmp.GetPath() / "attached_schematic.kicad_sch" ).string();
                SaveSymbolLibraryForTarget( { lib }, libraryPath, target );
                checkSourceUnchanged();
                checkSerializedChecksum( libraryPath, 1 );
                SCH_IO_KICAD_SEXPR io;
                LIB_SYMBOL*        reloadedLibrary = io.LoadSymbol( libraryPath, lib->GetName() );
                BOOST_REQUIRE( reloadedLibrary );
                checkReloadedAttachment( *reloadedLibrary->GetEmbeddedFiles() );

                SaveSchematicForTarget( schematic->GetTopLevelSheet(), schematic.get(), schematicPath, target );
                checkSourceUnchanged();
                checkSerializedChecksum( schematicPath, 2 );
                std::ifstream outputFile( schematicPath );
                auto          reloadedSchematic = KI_TEST::ReadSchematicFromStream( outputFile, &settings.Prj() );
                BOOST_REQUIRE( reloadedSchematic );
                checkReloadedAttachment( *reloadedSchematic->GetEmbeddedFiles() );
                const auto& reloadedCache = reloadedSchematic->RootScreen()->GetLibSymbols();
                const auto  cachedSymbol = reloadedCache.find( cacheKey );
                BOOST_REQUIRE( cachedSymbol != reloadedCache.end() );
                checkReloadedAttachment( *cachedSymbol->second->GetEmbeddedFiles() );
                checkSourceUnchanged();
            }
        }
    }
}


BOOST_AUTO_TEST_SUITE_END()
