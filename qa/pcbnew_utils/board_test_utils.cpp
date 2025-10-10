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

#include <pcbnew_utils/board_test_utils.h>

#include <iostream>
#include <filesystem>

#include <wx/filename.h>

#include <boost/test/unit_test.hpp>

#include <board.h>
#include <board_commit.h>
#include <board_design_settings.h>
#include <footprint.h>
#include <kiid.h>
#include <pad.h>
#include <pcb_shape.h>
#include <zone.h>
#include <zone_filler.h>
#include <length_delay_calculation/length_delay_calculation.h>
#include <drc/drc_cache_generator.h>
#include <pcbnew_utils/board_file_utils.h>
#include <settings/settings_manager.h>
#include <tool/tool_manager.h>

#define CHECK_ENUM_CLASS_EQUAL( L, R )                                                      \
    BOOST_CHECK_EQUAL( static_cast<int>( L ), static_cast<int>( R ) )


namespace KI_TEST
{

BOARD_DUMPER::BOARD_DUMPER() :
        m_dump_boards( true )
{
}


void BOARD_DUMPER::DumpBoardToFile( BOARD& aBoard, const std::string& aName ) const
{
    if( !m_dump_boards )
        return;

    auto path = std::filesystem::temp_directory_path() / aName;
    path += ".kicad_pcb";

    BOOST_TEST_MESSAGE( "Dumping board file: " << path.string() );
    ::KI_TEST::DumpBoardToFile( aBoard, path.string() );
}


void LoadBoard( SETTINGS_MANAGER& aSettingsManager, const wxString& aRelPath,
                std::unique_ptr<BOARD>& aBoard )
{
    if( aBoard )
    {
        aBoard->SetProject( nullptr );
        aBoard = nullptr;
    }

    std::string absPath = GetPcbnewTestDataDir() + aRelPath.ToStdString();
    wxFileName  projectFile( absPath + ".kicad_pro" );
    wxFileName  legacyProject( absPath + ".pro" );
    std::string boardPath = absPath + ".kicad_pcb";
    wxFileName  rulesFile( absPath + ".kicad_dru" );

    if( projectFile.Exists() )
    {
        aSettingsManager.LoadProject( projectFile.GetFullPath() );
        BOOST_TEST_MESSAGE( "Loading project file: " << projectFile.GetFullPath() );
    }
    else if( legacyProject.Exists() )
    {
        aSettingsManager.LoadProject( legacyProject.GetFullPath() );
        BOOST_TEST_MESSAGE( "Loading project file: " << projectFile.GetFullPath() );
    }
    else
        BOOST_TEST_MESSAGE( "Could not load project: " << projectFile.GetFullPath() );

    BOOST_TEST_MESSAGE( "Loading board file: " << boardPath );

    try {
        aBoard = ReadBoardFromFileOrStream( boardPath );
    }
    catch( const IO_ERROR& ioe )
    {
        BOOST_TEST_ERROR( ioe.What() );
    }

    BOOST_REQUIRE_MESSAGE( aBoard, "aBoard is null or invalid" );

    if( projectFile.Exists() || legacyProject.Exists() )
        aBoard->SetProject( &aSettingsManager.Prj() );

    auto m_DRCEngine = std::make_shared<DRC_ENGINE>( aBoard.get(), &aBoard->GetDesignSettings() );

    BOOST_TEST_CHECKPOINT( "Init drc engine" );

    if( rulesFile.Exists() )
        m_DRCEngine->InitEngine( rulesFile );
    else
        m_DRCEngine->InitEngine( wxFileName() );

    aBoard->GetDesignSettings().m_DRCEngine = m_DRCEngine;

    BOOST_TEST_CHECKPOINT( "Build list of nets" );
    try
    {
        aBoard->BuildListOfNets();
    }
    catch( const std::exception& e )
    {
        BOOST_TEST_ERROR( "Exception in BuildListOfNets: " << e.what() );
        return;
    }

    BOOST_TEST_CHECKPOINT( "Build connectivity" );
    try
    {
        aBoard->BuildConnectivity();
    }
    catch( const std::exception& e )
    {
        BOOST_TEST_ERROR( "Exception in BuildConnectivity: " << e.what() );
        return;
    }

    BOOST_TEST_CHECKPOINT( "Synchronize Tuning Profile Properties" );
    try
    {
        aBoard->GetLengthCalculation()->SynchronizeTuningProfileProperties();
    }
    catch( const std::exception& e )
    {
        BOOST_TEST_ERROR( "Exception in SynchronizeTimeDomainProperties: " << e.what() );
        return;
    }

    if( aBoard->GetProject() )
    {
        std::unordered_set<wxString> dummy;
        BOOST_TEST_CHECKPOINT( "Synchronize Component Classes" );
        try
        {
            aBoard->SynchronizeComponentClasses( dummy );
        }
        catch( const std::exception& e )
        {
            BOOST_TEST_ERROR( "Exception in SynchronizeComponentClasses: " << e.what() );
            return;
        }

        BOOST_TEST_CHECKPOINT( "Run DRC cache generator" );
        try
        {
            DRC_CACHE_GENERATOR cacheGenerator;
            cacheGenerator.SetDRCEngine( m_DRCEngine.get() );
            cacheGenerator.Run();
        }
        catch( const std::exception& e )
        {
            BOOST_TEST_ERROR( "Exception in DRC cache generator: " << e.what() );
            return;
        }
    }
}


BOARD_ITEM& RequireBoardItemWithTypeAndId( const BOARD& aBoard, KICAD_T aItemType, const KIID& aID )
{
    BOARD_ITEM* item = aBoard.ResolveItem( aID, true );

    BOOST_REQUIRE( item );
    BOOST_REQUIRE_EQUAL( item->Type(), aItemType );

    return *item;
}


/**
 * A temporary directory that will be deleted when it goes out of scope.
 */
class TEMPORARY_DIRECTORY
{
public:
    /**
     * Create a temporary directory with a given prefix and suffix. The directory will be
     * created in the system temporary directory, and will not be pre-existing.
     */
    TEMPORARY_DIRECTORY( const std::string& aNamePrefix, const std::string aSuffix )
    {
        int i = 0;

        // Find a unique directory name
        while( true )
        {
            m_path = std::filesystem::temp_directory_path()
                     / ( aNamePrefix + std::to_string( i ) + aSuffix );

            if( !std::filesystem::exists( m_path ) )
                break;

            i++;
        }

        wxASSERT( !std::filesystem::exists( m_path ) );
        std::filesystem::create_directories( m_path );
    }

    ~TEMPORARY_DIRECTORY() { std::filesystem::remove_all( m_path ); }

    const std::filesystem::path& GetPath() const { return m_path; }

private:
    std::filesystem::path m_path;
};


void LoadAndTestBoardFile( const wxString aRelativePath, bool aRoundtrip,
                           std::function<void( BOARD& )> aBoardTestFunction,
                           std::optional<int>            aExpectedBoardVersion )
{
    const std::string absBoardPath =
            KI_TEST::GetPcbnewTestDataDir() + aRelativePath.ToStdString() + ".kicad_pcb";

    BOOST_TEST_MESSAGE( "Loading board to test: " << absBoardPath );
    std::unique_ptr<BOARD> board1 = KI_TEST::ReadBoardFromFileOrStream( absBoardPath );

    // Should load - if it doesn't we're done for
    BOOST_REQUIRE( board1 );

    BOOST_TEST_MESSAGE( "Testing loaded board" );
    aBoardTestFunction( *board1 );

    // If we care about the board version, check it now - but not after a roundtrip
    // (as the version will be updated to the current version)
    if( aExpectedBoardVersion )
    {
        BOOST_CHECK_EQUAL( board1->GetFileFormatVersionAtLoad(), *aExpectedBoardVersion );
    }

    if( aRoundtrip )
    {
        TEMPORARY_DIRECTORY tempLib( "kicad_qa_brd_roundtrip", "" );

        const auto savePath = tempLib.GetPath() / ( aRelativePath.ToStdString() + ".kicad_pcb" );
        KI_TEST::DumpBoardToFile( *board1, savePath.string() );

        std::unique_ptr<BOARD> board2 = KI_TEST::ReadBoardFromFileOrStream( savePath.string() );

        // Should load again
        BOOST_REQUIRE( board2 );

        BOOST_TEST_MESSAGE( "Testing roundtripped (saved/reloaded) file" );
        aBoardTestFunction( *board2 );
    }
}


void LoadAndTestFootprintFile( const wxString& aLibRelativePath, const wxString& aFpName,
                               bool                              aRoundtrip,
                               std::function<void( FOOTPRINT& )> aFootprintTestFunction,
                               std::optional<int>                aExpectedFootprintVersion )
{
    const std::string absFootprintPath = KI_TEST::GetPcbnewTestDataDir()
                                         + aLibRelativePath.ToStdString() + "/"
                                         + aFpName.ToStdString() + ".kicad_mod";

    BOOST_TEST_MESSAGE( "Loading footprint to test: " << absFootprintPath );
    std::unique_ptr<FOOTPRINT> fp1 = KI_TEST::ReadFootprintFromFileOrStream( absFootprintPath );

    // Should load - if it doesn't we're done for
    BOOST_REQUIRE( fp1 );

    BOOST_TEST_MESSAGE( "Testing loaded footprint (value: " << fp1->GetValue() << ")" );
    aFootprintTestFunction( *fp1 );

    // If we care about the board version, check it now - but not after a roundtrip
    // (as the version will be updated to the current version)
    if( aExpectedFootprintVersion )
    {
        BOOST_CHECK_EQUAL( fp1->GetFileFormatVersionAtLoad(), *aExpectedFootprintVersion );
    }

    if( aRoundtrip )
    {
        /**
         * Use a temporary directory, so that if something goes wrong and the file isn't
         * written properly, we don't leave a library behind that will cause exceptions
         * when the cache is set up on future runs.
         */
        TEMPORARY_DIRECTORY tempLib( "kicad_qa_fp_roundtrip", ".pretty" );
        const wxString      fpFilename = fp1->GetFPID().GetLibItemName() + wxString( ".kicad_mod" );

        BOOST_TEST_MESSAGE( "Resaving footprint: " << fpFilename << " in " << tempLib.GetPath() );

        KI_TEST::DumpFootprintToFile( *fp1, tempLib.GetPath().string() );

        const auto fp2Path = tempLib.GetPath() / fpFilename.ToStdString();

        BOOST_TEST_MESSAGE( "Re-reading footprint: " << fpFilename << " in " << tempLib.GetPath() );

        std::unique_ptr<FOOTPRINT> fp2 = KI_TEST::ReadFootprintFromFileOrStream( fp2Path.string() );

        // Should load again
        BOOST_REQUIRE( fp2 );

        BOOST_TEST_MESSAGE( "Testing roundtripped (saved/reloaded) file" );
        aFootprintTestFunction( *fp2 );
    }
}


void FillZones( BOARD* m_board )
{
    BOOST_TEST_CHECKPOINT( "Filling zones" );

    TOOL_MANAGER toolMgr;
    toolMgr.SetEnvironment( m_board, nullptr, nullptr, nullptr, nullptr );

    KI_TEST::DUMMY_TOOL* dummyTool = new KI_TEST::DUMMY_TOOL();
    toolMgr.RegisterTool( dummyTool );

    BOARD_COMMIT       commit( dummyTool );
    ZONE_FILLER        filler( m_board, &commit );
    std::vector<ZONE*> toFill;

    for( ZONE* zone : m_board->Zones() )
        toFill.push_back( zone );

    if( filler.Fill( toFill, false, nullptr ) )
        commit.Push( _( "Fill Zone(s)" ), SKIP_UNDO | SKIP_SET_DIRTY | ZONE_FILL_OP | SKIP_CONNECTIVITY );

    BOOST_TEST_CHECKPOINT( "Building connectivity (after zone fill)" );
    m_board->BuildConnectivity();
}


#define TEST( a, b )                                                                               \
    {                                                                                              \
        if( a != b )                                                                               \
            return a < b;                                                                          \
    }
#define TEST_PT( a, b )                                                                            \
    {                                                                                              \
        if( a.x != b.x )                                                                           \
            return a.x < b.x;                                                                      \
        if( a.y != b.y )                                                                           \
            return a.y < b.y;                                                                      \
    }


struct kitest_cmp_drawings
{
    FOOTPRINT::cmp_drawings fp_comp;

    bool operator()( const BOARD_ITEM* itemA, const BOARD_ITEM* itemB ) const
    {
        TEST( itemA->Type(), itemB->Type() );

        if( itemA->GetLayerSet() != itemB->GetLayerSet() )
            return itemA->GetLayerSet().Seq() < itemB->GetLayerSet().Seq();

        if( itemA->Type() == PCB_TEXT_T )
        {
            const PCB_TEXT* textA = static_cast<const PCB_TEXT*>( itemA );
            const PCB_TEXT* textB = static_cast<const PCB_TEXT*>( itemB );

            TEST_PT( textA->GetPosition(), textB->GetPosition() );
            TEST( textA->GetTextAngle(), textB->GetTextAngle() );
        }

        return fp_comp( itemA, itemB );
    }
};


void CheckFootprint( const FOOTPRINT* expected, const FOOTPRINT* fp )
{
    CHECK_ENUM_CLASS_EQUAL( expected->Type(), fp->Type() );

    // TODO: validate those informations match the importer
    BOOST_CHECK_EQUAL( expected->GetPosition(), fp->GetPosition() );
    BOOST_CHECK_EQUAL( expected->GetOrientation(), fp->GetOrientation() );

    BOOST_CHECK_EQUAL( expected->GetReference(), fp->GetReference() );
    BOOST_CHECK_EQUAL( expected->GetValue(), fp->GetValue() );
    BOOST_CHECK_EQUAL( expected->GetLibDescription(), fp->GetLibDescription() );
    BOOST_CHECK_EQUAL( expected->GetKeywords(), fp->GetKeywords() );
    BOOST_CHECK_EQUAL( expected->GetAttributes(), fp->GetAttributes() );
    BOOST_CHECK_EQUAL( expected->GetFlag(), fp->GetFlag() );
    //BOOST_CHECK_EQUAL( expected->GetProperties(), fp->GetProperties() );
    BOOST_CHECK_EQUAL( expected->GetTypeName(), fp->GetTypeName() );

    // simple test if count matches
    BOOST_CHECK_EQUAL( expected->GetFields().size(), fp->GetFields().size() );
    BOOST_CHECK_EQUAL( expected->Pads().size(), fp->Pads().size() );
    BOOST_CHECK_EQUAL( expected->GraphicalItems().size(), fp->GraphicalItems().size() );
    BOOST_CHECK_EQUAL( expected->Zones().size(), fp->Zones().size() );
    BOOST_CHECK_EQUAL( expected->Groups().size(), fp->Groups().size() );
    BOOST_CHECK_EQUAL( expected->Models().size(), fp->Models().size() );

    std::set<PAD*, FOOTPRINT::cmp_pads> expectedPads( expected->Pads().begin(),
                                                      expected->Pads().end() );
    std::set<PAD*, FOOTPRINT::cmp_pads> fpPads( fp->Pads().begin(), fp->Pads().end() );

    for( auto itExpected = expectedPads.begin(), itFp = fpPads.begin();
         itExpected != expectedPads.end() && itFp != fpPads.end(); itExpected++, itFp++ )
    {
        CheckFpPad( *itExpected, *itFp );
    }

    std::set<BOARD_ITEM*, kitest_cmp_drawings> expectedGraphicalItems( expected->GraphicalItems().begin(),
                                                                       expected->GraphicalItems().end() );
    std::set<BOARD_ITEM*, kitest_cmp_drawings> fpGraphicalItems( fp->GraphicalItems().begin(),
                                                                 fp->GraphicalItems().end() );

    for( auto itExpected = expectedGraphicalItems.begin(), itFp = fpGraphicalItems.begin();
         itExpected != expectedGraphicalItems.end() && itFp != fpGraphicalItems.end();
         itExpected++, itFp++ )
    {
        BOOST_CHECK_EQUAL( ( *itExpected )->Type(), ( *itFp )->Type() );

        switch( ( *itExpected )->Type() )
        {
        case PCB_TEXT_T:
        {
            const PCB_TEXT* expectedText = static_cast<const PCB_TEXT*>( *itExpected );
            const PCB_TEXT* text = static_cast<const PCB_TEXT*>( *itFp );

            CheckFpText( expectedText, text );
            break;
        }

        case PCB_SHAPE_T:
        {
            const PCB_SHAPE* expectedShape = static_cast<const PCB_SHAPE*>( *itExpected );
            const PCB_SHAPE* shape = static_cast<const PCB_SHAPE*>( *itFp );

            CheckFpShape( expectedShape, shape );
            break;
        }

        case PCB_DIM_ALIGNED_T:
        case PCB_DIM_LEADER_T:
        case PCB_DIM_CENTER_T:
        case PCB_DIM_RADIAL_T:
        case PCB_DIM_ORTHOGONAL_T:
            // TODO
            break;

        case PCB_BARCODE_T:
            // TODO
            break;

        default:
            BOOST_ERROR( "KICAD_T not known" );
            break;
        }
    }

    std::set<ZONE*, FOOTPRINT::cmp_zones> expectedZones( expected->Zones().begin(),
                                                         expected->Zones().end() );
    std::set<ZONE*, FOOTPRINT::cmp_zones> fpZones( fp->Zones().begin(), fp->Zones().end() );

    for( auto itExpected = expectedZones.begin(), itFp = fpZones.begin();
         itExpected != expectedZones.end() && itFp != fpZones.end(); itExpected++, itFp++ )
    {
        CheckFpZone( *itExpected, *itFp );
    }

    // TODO: Groups

    // Use FootprintNeedsUpdate as sanity check (which should do many of the same checks as above,
    // but neither is guaranteed to be complete).
    WX_STRING_REPORTER reporter;

    if( const_cast<FOOTPRINT*>(expected)->FootprintNeedsUpdate(fp, BOARD_ITEM::COMPARE_FLAGS::DRC, &reporter) )
        BOOST_REQUIRE_MESSAGE( false,  reporter.GetMessages() );
}


void CheckFpPad( const PAD* expected, const PAD* pad )
{
    // TODO(JE) padstacks
    BOOST_TEST_CONTEXT( "Assert PAD with KIID=" << expected->m_Uuid.AsString() )
    {
        CHECK_ENUM_CLASS_EQUAL( expected->Type(), pad->Type() );

        BOOST_CHECK_EQUAL( expected->GetNumber(), pad->GetNumber() );
        CHECK_ENUM_CLASS_EQUAL( expected->GetAttribute(), pad->GetAttribute() );
        CHECK_ENUM_CLASS_EQUAL( expected->GetProperty(), pad->GetProperty() );
        CHECK_ENUM_CLASS_EQUAL( expected->GetShape( PADSTACK::ALL_LAYERS ),
                                pad->GetShape( PADSTACK::ALL_LAYERS ) );

        BOOST_CHECK_EQUAL( expected->IsLocked(), pad->IsLocked() );

        BOOST_CHECK_EQUAL( expected->GetPosition(), pad->GetPosition() );
        BOOST_CHECK_EQUAL( expected->GetSize( PADSTACK::ALL_LAYERS ),
                           pad->GetSize( PADSTACK::ALL_LAYERS ) );
        BOOST_CHECK_EQUAL( expected->GetOrientation(), pad->GetOrientation() );
        BOOST_CHECK_EQUAL( expected->GetDelta( PADSTACK::ALL_LAYERS ),
                           pad->GetDelta( PADSTACK::ALL_LAYERS ) );
        BOOST_CHECK_EQUAL( expected->GetOffset( PADSTACK::ALL_LAYERS ),
                           pad->GetOffset( PADSTACK::ALL_LAYERS ) );
        BOOST_CHECK_EQUAL( expected->GetDrillSize(), pad->GetDrillSize() );
        CHECK_ENUM_CLASS_EQUAL( expected->GetDrillShape(), pad->GetDrillShape() );

        BOOST_CHECK_EQUAL( expected->GetLayerSet(), pad->GetLayerSet() );

        BOOST_CHECK_EQUAL( expected->GetNetCode(), pad->GetNetCode() );
        BOOST_CHECK_EQUAL( expected->GetPinFunction(), pad->GetPinFunction() );
        BOOST_CHECK_EQUAL( expected->GetPinType(), pad->GetPinType() );
        BOOST_CHECK_EQUAL( expected->GetPadToDieLength(), pad->GetPadToDieLength() );
        BOOST_CHECK_EQUAL( expected->GetPadToDieDelay(), pad->GetPadToDieDelay() );
        BOOST_CHECK_EQUAL( expected->GetLocalSolderMaskMargin().value_or( 0 ),
                           pad->GetLocalSolderMaskMargin().value_or( 0 ) );
        BOOST_CHECK_EQUAL( expected->GetLocalSolderPasteMargin().value_or( 0 ),
                           pad->GetLocalSolderPasteMargin().value_or( 0 ) );
        BOOST_CHECK_EQUAL( expected->GetLocalSolderPasteMarginRatio().value_or( 0 ),
                           pad->GetLocalSolderPasteMarginRatio().value_or( 0 ) );
        BOOST_CHECK_EQUAL( expected->GetLocalClearance().value_or( 0 ),
                           pad->GetLocalClearance().value_or( 0 ) );
        CHECK_ENUM_CLASS_EQUAL( expected->GetLocalZoneConnection(), pad->GetLocalZoneConnection() );
        BOOST_CHECK_EQUAL( expected->GetLocalThermalSpokeWidthOverride().value_or( 0 ),
                           pad->GetLocalThermalSpokeWidthOverride().value_or( 0 ) );
        BOOST_CHECK_EQUAL( expected->GetThermalSpokeAngle(), pad->GetThermalSpokeAngle() );
        BOOST_CHECK_EQUAL( expected->GetThermalGap(), pad->GetThermalGap() );
        BOOST_CHECK_EQUAL( expected->GetRoundRectRadiusRatio( PADSTACK::ALL_LAYERS ),
                           pad->GetRoundRectRadiusRatio( PADSTACK::ALL_LAYERS ) );
        BOOST_CHECK_EQUAL( expected->GetChamferRectRatio( PADSTACK::ALL_LAYERS ),
                           pad->GetChamferRectRatio( PADSTACK::ALL_LAYERS ) );
        BOOST_CHECK_EQUAL( expected->GetChamferPositions( PADSTACK::ALL_LAYERS ),
                           pad->GetChamferPositions( PADSTACK::ALL_LAYERS ) );
        BOOST_CHECK_EQUAL( expected->GetRemoveUnconnected(), pad->GetRemoveUnconnected() );
        BOOST_CHECK_EQUAL( expected->GetKeepTopBottom(), pad->GetKeepTopBottom() );

        // TODO: did we check everything for complex pad shapes?
        CHECK_ENUM_CLASS_EQUAL( expected->GetAnchorPadShape( PADSTACK::ALL_LAYERS ),
                                pad->GetAnchorPadShape( PADSTACK::ALL_LAYERS ) );
        CHECK_ENUM_CLASS_EQUAL( expected->GetCustomShapeInZoneOpt(),
                                pad->GetCustomShapeInZoneOpt() );

        BOOST_CHECK_EQUAL( expected->GetPrimitives( PADSTACK::ALL_LAYERS ).size(),
                           pad->GetPrimitives( PADSTACK::ALL_LAYERS ).size() );

        if( expected->GetPrimitives( PADSTACK::ALL_LAYERS ).size()
            == pad->GetPrimitives( PADSTACK::ALL_LAYERS ).size() )
        {
            for( size_t i = 0; i < expected->GetPrimitives( PADSTACK::ALL_LAYERS ).size(); ++i )
            {
                CheckFpShape( expected->GetPrimitives( PADSTACK::ALL_LAYERS ).at( i ).get(),
                              pad->GetPrimitives( PADSTACK::ALL_LAYERS ).at( i ).get() );
            }
        }
    }
}


void CheckFpText( const PCB_TEXT* expected, const PCB_TEXT* text )
{
    BOOST_TEST_CONTEXT( "Assert PCB_TEXT with KIID=" << expected->m_Uuid.AsString() )
    {
        CHECK_ENUM_CLASS_EQUAL( expected->Type(), text->Type() );

        BOOST_CHECK_EQUAL( expected->IsLocked(), text->IsLocked() );

        BOOST_CHECK_EQUAL( expected->GetText(), text->GetText() );
        BOOST_CHECK_EQUAL( expected->GetPosition(), text->GetPosition() );
        BOOST_CHECK_EQUAL( expected->GetTextAngle(), text->GetTextAngle() );
        BOOST_CHECK_EQUAL( expected->IsKeepUpright(), text->IsKeepUpright() );

        BOOST_CHECK_EQUAL( expected->GetLayerSet(), text->GetLayerSet() );
        BOOST_CHECK_EQUAL( expected->IsVisible(), text->IsVisible() );

        BOOST_CHECK_EQUAL( expected->GetTextSize(), text->GetTextSize() );
        BOOST_CHECK_EQUAL( expected->GetLineSpacing(), text->GetLineSpacing() );
        BOOST_CHECK_EQUAL( expected->GetTextThickness(), text->GetTextThickness() );
        BOOST_CHECK_EQUAL( expected->IsBold(), text->IsBold() );
        BOOST_CHECK_EQUAL( expected->IsItalic(), text->IsItalic() );
        BOOST_CHECK_EQUAL( expected->GetHorizJustify(), text->GetHorizJustify() );
        BOOST_CHECK_EQUAL( expected->GetVertJustify(), text->GetVertJustify() );
        BOOST_CHECK_EQUAL( expected->IsMirrored(), text->IsMirrored() );
        BOOST_CHECK_EQUAL( expected->GetFontName(), text->GetFontName() );

        // TODO: render cache?
    }
}


void CheckFpShape( const PCB_SHAPE* expected, const PCB_SHAPE* shape )
{
    BOOST_TEST_CONTEXT( "Assert PCB_SHAPE with KIID=" << expected->m_Uuid.AsString() )
    {
        CHECK_ENUM_CLASS_EQUAL( expected->Type(), shape->Type() );

        CHECK_ENUM_CLASS_EQUAL( expected->GetShape(), shape->GetShape() );

        BOOST_CHECK_EQUAL( expected->IsLocked(), shape->IsLocked() );

        BOOST_CHECK_EQUAL( expected->GetStart(), shape->GetStart() );
        BOOST_CHECK_EQUAL( expected->GetEnd(), shape->GetEnd() );

        if( expected->GetShape() == SHAPE_T::ARC )
        {
            // center and position might differ as they are calculated from start/mid/end -> compare mid instead
            BOOST_CHECK_EQUAL( expected->GetArcMid(), shape->GetArcMid() );
        }
        else
        {
            BOOST_CHECK_EQUAL( expected->GetCenter(), shape->GetCenter() );
            BOOST_CHECK_EQUAL( expected->GetPosition(), shape->GetPosition() );
        }

        BOOST_CHECK_EQUAL( expected->GetBezierC1(), shape->GetBezierC1() );
        BOOST_CHECK_EQUAL( expected->GetBezierC2(), shape->GetBezierC2() );

        CheckShapePolySet( &expected->GetPolyShape(), &shape->GetPolyShape() );

        BOOST_CHECK_EQUAL( expected->GetLayerSet(), shape->GetLayerSet() );

        BOOST_CHECK_EQUAL( expected->GetStroke().GetWidth(), shape->GetStroke().GetWidth() );
        CHECK_ENUM_CLASS_EQUAL( expected->GetStroke().GetLineStyle(),
                                shape->GetStroke().GetLineStyle() );
        CHECK_ENUM_CLASS_EQUAL( expected->GetFillMode(), shape->GetFillMode() );
    }
}


void CheckFpZone( const ZONE* expected, const ZONE* zone )
{
    BOOST_TEST_CONTEXT( "Assert ZONE with KIID=" << expected->m_Uuid.AsString() )
    {
        CHECK_ENUM_CLASS_EQUAL( expected->Type(), zone->Type() );

        BOOST_CHECK_EQUAL( expected->IsLocked(), zone->IsLocked() );

        BOOST_CHECK_EQUAL( expected->GetNetCode(), zone->GetNetCode() );
        BOOST_CHECK_EQUAL( expected->GetAssignedPriority(), zone->GetAssignedPriority() );
        CHECK_ENUM_CLASS_EQUAL( expected->GetPadConnection(), zone->GetPadConnection() );
        BOOST_CHECK_EQUAL( expected->GetLocalClearance().value_or( 0 ),
                           zone->GetLocalClearance().value_or( 0 ) );
        BOOST_CHECK_EQUAL( expected->GetMinThickness(), zone->GetMinThickness() );

        BOOST_CHECK_EQUAL( expected->GetLayerSet(), zone->GetLayerSet() );

        BOOST_CHECK_EQUAL( expected->IsFilled(), zone->IsFilled() );
        CHECK_ENUM_CLASS_EQUAL( expected->GetFillMode(), zone->GetFillMode() );
        BOOST_CHECK_EQUAL( expected->GetHatchThickness(), zone->GetHatchThickness() );
        BOOST_CHECK_EQUAL( expected->GetHatchGap(), zone->GetHatchGap() );
        BOOST_CHECK_EQUAL( expected->GetHatchOrientation(), zone->GetHatchOrientation() );
        BOOST_CHECK_EQUAL( expected->GetHatchSmoothingLevel(), zone->GetHatchSmoothingLevel() );
        BOOST_CHECK_EQUAL( expected->GetHatchSmoothingValue(), zone->GetHatchSmoothingValue() );
        BOOST_CHECK_EQUAL( expected->GetHatchBorderAlgorithm(), zone->GetHatchBorderAlgorithm() );
        BOOST_CHECK_EQUAL( expected->GetHatchHoleMinArea(), zone->GetHatchHoleMinArea() );
        BOOST_CHECK_EQUAL( expected->GetThermalReliefGap(), zone->GetThermalReliefGap() );
        BOOST_CHECK_EQUAL( expected->GetThermalReliefSpokeWidth(),
                           zone->GetThermalReliefSpokeWidth() );
        BOOST_CHECK_EQUAL( expected->GetCornerSmoothingType(), zone->GetCornerSmoothingType() );
        BOOST_CHECK_EQUAL( expected->GetCornerRadius(), zone->GetCornerRadius() );
        CHECK_ENUM_CLASS_EQUAL( expected->GetIslandRemovalMode(), zone->GetIslandRemovalMode() );
        BOOST_CHECK_EQUAL( expected->GetMinIslandArea(), zone->GetMinIslandArea() );

        BOOST_CHECK_EQUAL( expected->GetIsRuleArea(), zone->GetIsRuleArea() );
        BOOST_CHECK_EQUAL( expected->GetDoNotAllowZoneFills(), zone->GetDoNotAllowZoneFills() );
        BOOST_CHECK_EQUAL( expected->GetDoNotAllowVias(), zone->GetDoNotAllowVias() );
        BOOST_CHECK_EQUAL( expected->GetDoNotAllowTracks(), zone->GetDoNotAllowTracks() );
        BOOST_CHECK_EQUAL( expected->GetDoNotAllowPads(), zone->GetDoNotAllowPads() );
        BOOST_CHECK_EQUAL( expected->GetDoNotAllowFootprints(), zone->GetDoNotAllowFootprints() );

        BOOST_CHECK_EQUAL( expected->GetZoneName(), zone->GetZoneName() );
        CHECK_ENUM_CLASS_EQUAL( expected->GetTeardropAreaType(), zone->GetTeardropAreaType() );
        BOOST_CHECK_EQUAL( expected->GetZoneName(), zone->GetZoneName() );

        CheckShapePolySet( expected->Outline(), zone->Outline() );
        // TODO: filled zones
    }
}


void CheckShapePolySet( const SHAPE_POLY_SET* expected, const SHAPE_POLY_SET* polyset )
{
    BOOST_TEST_CONTEXT( "Assert SHAPE_POLY_SET" )
    {
        BOOST_CHECK_EQUAL( expected->OutlineCount(), polyset->OutlineCount() );
        BOOST_CHECK_EQUAL( expected->TotalVertices(), polyset->TotalVertices() );

        if( expected->OutlineCount() != polyset->OutlineCount() )
            return; // don't check the rest

        if( expected->TotalVertices() != polyset->TotalVertices() )
            return; // don't check the rest

        // TODO: check all outlines and holes (just checking outlines for now)
        for( int i = 0; i < expected->OutlineCount(); ++i )
        {
            BOOST_TEST_CONTEXT( "Outline " << i )
            {
                BOOST_CHECK_EQUAL( expected->Outline( i ).ArcCount(),
                                   polyset->Outline( i ).ArcCount() );
                BOOST_CHECK_EQUAL( expected->Outline( i ).PointCount(),
                                   polyset->Outline( i ).PointCount() );


                if( expected->Outline( i ).PointCount() != polyset->Outline( i ).PointCount() )
                    return; // don't check the rest

                for( int j = 0; j < expected->Outline( i ).PointCount(); ++j )
                {
                    BOOST_CHECK_EQUAL( expected->Outline( i ).GetPoint( j ),
                                       polyset->Outline( i ).GetPoint( j ) );
                }
            }
        }
    }
}

} // namespace KI_TEST
