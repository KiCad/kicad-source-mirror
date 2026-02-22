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

/**
 * @file test_easyedapro_v3_import.cpp
 * Lightweight regression tests for EasyEDA Pro v3 raw index and parser behavior
 */

#include <boost/test/unit_test.hpp>

#include <array>
#include <set>

#include <qa_utils/wx_utils/unit_test_utils.h>

#include <io/easyedapro/easyedapro_import_utils.h>
#include <io/easyedapro/easyedapro_v3_parser.h>

#include <sch_io/easyedapro/sch_easyedapro_v3_parser.h>

#include <pgm_base.h>
#include <schematic.h>
#include <sch_io/sch_io.h>
#include <sch_io/sch_io_mgr.h>
#include <sch_label.h>
#include <sch_line.h>
#include <sch_pin.h>
#include <sch_screen.h>
#include <sch_symbol.h>
#include <sch_text.h>

#include <settings/settings_manager.h>

#include <wx/filefn.h>
#include <wx/filename.h>


namespace
{

static wxString getEasyEdaProV3ArchivePath()
{
    return wxString::FromUTF8(
            KI_TEST::GetTestDataRootDir()
            + "pcbnew/plugins/easyedapro/ProProject_LS2K0300Core_2025-11-14.epro2" );
}

} // namespace


BOOST_AUTO_TEST_SUITE( EasyEdaProV3Import )


BOOST_AUTO_TEST_CASE( BuildProjectIndexHasSchematicSheets )
{
    EASYEDAPRO::V3_DOC_PARSER v3( getEasyEdaProV3ArchivePath() );
    BOOST_REQUIRE_NO_THROW( v3.Load() );

    nlohmann::json project = EASYEDAPRO::BuildV3ProjectIndexFromRawDocs( v3 );

    BOOST_REQUIRE( project.contains( "schematics" ) );
    BOOST_REQUIRE( project.at( "schematics" ).is_object() );
    BOOST_REQUIRE( !project.at( "schematics" ).empty() );

    bool hasAtLeastOneSheet = false;

    for( const auto& [schUuid, sch] : project.at( "schematics" ).items() )
    {
        if( sch.contains( "sheets" ) && sch.at( "sheets" ).is_array()
            && !sch.at( "sheets" ).empty() )
        {
            hasAtLeastOneSheet = true;
            break;
        }
    }

    BOOST_CHECK( hasAtLeastOneSheet );
}


BOOST_AUTO_TEST_CASE( RawSchematicPageContainsTextWireAndNetAttr )
{
    EASYEDAPRO::V3_DOC_PARSER v3( getEasyEdaProV3ArchivePath() );
    BOOST_REQUIRE_NO_THROW( v3.Load() );

    const auto& pages = v3.GetRawDocs( wxS( "SCH_PAGE" ) );
    BOOST_REQUIRE( !pages.empty() );

    bool hasTextValue = false;
    bool hasLineStartEnd = false;
    bool hasWireNetAttr = false;

    for( const auto& [pageUuid, pageDoc] : pages )
    {
        for( const EASYEDAPRO::V3_ROW& row : pageDoc.rows )
        {
            if( row.type == wxS( "TEXT" ) && row.inner.contains( "value" ) )
                hasTextValue = true;

            if( row.type == wxS( "LINE" )
                && row.inner.contains( "startX" ) && row.inner.contains( "startY" )
                && row.inner.contains( "endX" ) && row.inner.contains( "endY" ) )
            {
                hasLineStartEnd = true;
            }

            if( row.type == wxS( "ATTR" )
                && EASYEDAPRO::V3GetString( row.inner, "key" ) == wxS( "NET" ) )
            {
                hasWireNetAttr = true;
            }
        }
    }

    BOOST_CHECK( hasTextValue );
    BOOST_CHECK( hasLineStartEnd );
    BOOST_CHECK( hasWireNetAttr );
}


BOOST_AUTO_TEST_CASE( RawSymbolContainsBlobObjectReference )
{
    EASYEDAPRO::V3_DOC_PARSER v3( getEasyEdaProV3ArchivePath() );
    BOOST_REQUIRE_NO_THROW( v3.Load() );

    const auto& symbols = v3.GetRawDocs( wxS( "SYMBOL" ) );
    BOOST_REQUIRE( !symbols.empty() );

    bool hasBlobObject = false;

    for( const auto& [symbolUuid, symbolDoc] : symbols )
    {
        for( const EASYEDAPRO::V3_ROW& row : symbolDoc.rows )
        {
            if( row.type != wxS( "OBJ" ) )
                continue;

            wxString content = EASYEDAPRO::V3GetString( row.inner, "content" );

            if( content.StartsWith( wxS( "blob:" ) ) )
            {
                hasBlobObject = true;
                break;
            }
        }

        if( hasBlobObject )
            break;
    }

    BOOST_CHECK( hasBlobObject );
}


BOOST_AUTO_TEST_CASE( RawSymbolPartUnitsAndPinOrientationAreStable )
{
    EASYEDAPRO::V3_DOC_PARSER v3( getEasyEdaProV3ArchivePath() );
    BOOST_REQUIRE_NO_THROW( v3.Load() );

    nlohmann::json project = EASYEDAPRO::BuildV3ProjectIndexFromRawDocs( v3 );
    std::map<wxString, EASYEDAPRO::PRJ_DEVICE> devices = project.at( "devices" );

    auto findDevice = [&]( const wxString& aTitle ) -> const EASYEDAPRO::PRJ_DEVICE*
    {
        for( const auto& [devUuid, dev] : devices )
        {
            if( dev.title == aTitle )
                return &dev;
        }

        return nullptr;
    };

    const EASYEDAPRO::PRJ_DEVICE* ls2kDevice = findDevice( wxS( "LS2K0300" ) );
    BOOST_REQUIRE( ls2kDevice );

    const EASYEDAPRO::PRJ_DEVICE* sgmDevice = findDevice( wxS( "SGM61032BXKB6G/TR" ) );
    BOOST_REQUIRE( sgmDevice );

    SCH_EASYEDAPRO_V3_PARSER parser( nullptr, nullptr );

    wxString ls2kSymbolUuid = ls2kDevice->attributes.at( wxS( "Symbol" ) );
    const EASYEDAPRO::V3_DOC_RAW* ls2kRaw = v3.FindRawDoc( wxS( "SYMBOL" ), ls2kSymbolUuid );
    BOOST_REQUIRE( ls2kRaw );

    EASYEDAPRO::SYM_INFO ls2kInfo = parser.ParseSymbol( *ls2kRaw, ls2kDevice->attributes );
    BOOST_REQUIRE( ls2kInfo.libSymbol );
    BOOST_CHECK_EQUAL( ls2kInfo.libSymbol->GetUnitCount(), 5 );

    std::array<int, 6> unitPinCount = {};

    for( SCH_PIN* pin : ls2kInfo.libSymbol->GetPins() )
    {
        int unit = pin->GetUnit();

        if( unit >= 1 && unit <= 5 )
            unitPinCount[unit]++;
    }

    for( int unit = 1; unit <= 5; ++unit )
        BOOST_CHECK_MESSAGE( unitPinCount[unit] > 0,
                             "LS2K0300 should have pins in each unit" );

    wxString sgmSymbolUuid = sgmDevice->attributes.at( wxS( "Symbol" ) );
    const EASYEDAPRO::V3_DOC_RAW* sgmRaw = v3.FindRawDoc( wxS( "SYMBOL" ), sgmSymbolUuid );
    BOOST_REQUIRE( sgmRaw );

    EASYEDAPRO::SYM_INFO sgmInfo = parser.ParseSymbol( *sgmRaw, sgmDevice->attributes );
    BOOST_REQUIRE( sgmInfo.libSymbol );

    bool hasLeftPins = false;
    bool hasRightPins = false;

    for( SCH_PIN* pin : sgmInfo.libSymbol->GetPins() )
    {
        if( pin->GetPosition().x < 0 )
        {
            hasLeftPins = true;
            BOOST_CHECK_EQUAL( static_cast<int>( pin->GetOrientation() ),
                               static_cast<int>( PIN_ORIENTATION::PIN_RIGHT ) );
        }
        else if( pin->GetPosition().x > 0 )
        {
            hasRightPins = true;
            BOOST_CHECK_EQUAL( static_cast<int>( pin->GetOrientation() ),
                               static_cast<int>( PIN_ORIENTATION::PIN_LEFT ) );
        }
    }

    BOOST_CHECK( hasLeftPins );
    BOOST_CHECK( hasRightPins );
}


BOOST_AUTO_TEST_CASE( PluginLoadProducesWireTextAndLabel )
{
    wxString tempDir = wxFileName::CreateTempFileName( "easyedapro_v3_import" );

    BOOST_REQUIRE( wxFileExists( tempDir ) );
    BOOST_REQUIRE( wxRemoveFile( tempDir ) );
    BOOST_REQUIRE( wxMkdir( tempDir ) );

    wxFileName projectFile( tempDir, "easyedapro_v3_test", "kicad_pro" );
    wxFileName archiveFile( tempDir, "sample", "epro2" );

    BOOST_REQUIRE( wxCopyFile( getEasyEdaProV3ArchivePath(), archiveFile.GetFullPath() ) );

    Pgm().GetSettingsManager().LoadProject( projectFile.GetFullPath().ToStdString() );

    SCHEMATIC schematic( nullptr );
    schematic.SetProject( &Pgm().GetSettingsManager().Prj() );

    IO_RELEASER<SCH_IO> plugin( SCH_IO_MGR::FindPlugin( SCH_IO_MGR::SCH_EASYEDAPRO_V3 ) );
    BOOST_REQUIRE( plugin );

    SCH_SHEET* rootSheet = nullptr;
    BOOST_REQUIRE_NO_THROW( rootSheet =
                                    plugin->LoadSchematicFile( archiveFile.GetFullPath(),
                                                               &schematic, nullptr, nullptr ) );
    BOOST_REQUIRE( rootSheet );

    schematic.RefreshHierarchy();

    int wireCount = 0;
    int textCount = 0;
    int labelCount = 0;

    for( const SCH_SHEET_PATH& sheetPath : schematic.Hierarchy() )
    {
        SCH_SCREEN* screen = sheetPath.LastScreen();

        if( !screen )
            continue;

        for( SCH_ITEM* item : screen->Items().OfType( SCH_LINE_T ) )
        {
            SCH_LINE* line = static_cast<SCH_LINE*>( item );

            if( line->GetLayer() == LAYER_WIRE )
                wireCount++;
        }

        for( SCH_ITEM* item : screen->Items().OfType( SCH_TEXT_T ) )
        {
            SCH_TEXT* text = static_cast<SCH_TEXT*>( item );

            if( !text->GetText().IsEmpty() )
                textCount++;
        }

        for( SCH_ITEM* item : screen->Items().OfType( SCH_LABEL_T ) )
        {
            SCH_LABEL* label = static_cast<SCH_LABEL*>( item );

            if( !label->GetText().IsEmpty() )
                labelCount++;
        }
    }

    BOOST_CHECK( wireCount > 0 );
    BOOST_CHECK( textCount > 0 );
    BOOST_CHECK( labelCount > 0 );

    BOOST_CHECK( wxFileName::Rmdir( tempDir, wxPATH_RMDIR_RECURSIVE ) );
}


BOOST_AUTO_TEST_CASE( PluginLoadPreservesWireNetLabelAlignment )
{
    wxString tempDir = wxFileName::CreateTempFileName( "easyedapro_v3_import" );

    BOOST_REQUIRE( wxFileExists( tempDir ) );
    BOOST_REQUIRE( wxRemoveFile( tempDir ) );
    BOOST_REQUIRE( wxMkdir( tempDir ) );

    wxFileName projectFile( tempDir, "easyedapro_v3_test", "kicad_pro" );
    wxFileName archiveFile( tempDir, "sample", "epro2" );

    BOOST_REQUIRE( wxCopyFile( getEasyEdaProV3ArchivePath(), archiveFile.GetFullPath() ) );

    // Pick a stable, unique NET label from raw docs where align indicates the text is anchored
    // on the right side of the connection point.
    EASYEDAPRO::V3_DOC_PARSER v3( archiveFile.GetFullPath() );
    BOOST_REQUIRE_NO_THROW( v3.Load() );

    struct NET_ATTR
    {
        wxString value;
        wxString align;
        int      rotation;
    };

    std::vector<NET_ATTR>   candidates;
    std::map<wxString, int> valueCounts;

    for( const auto& [pageUuid, pageDoc] : v3.GetRawDocs( wxS( "SCH_PAGE" ) ) )
    {
        for( const EASYEDAPRO::V3_ROW& row : pageDoc.rows )
        {
            if( row.type != wxS( "ATTR" ) )
                continue;

            if( EASYEDAPRO::V3GetString( row.inner, "key" ) != wxS( "NET" ) )
                continue;

            if( !EASYEDAPRO::V3GetBool( row.inner, "valueVisible" ) )
                continue;

            wxString value =
                    EASYEDAPRO::V3JsonToString( row.inner.value( "value", nlohmann::json() ) );

            if( value.IsEmpty() )
                continue;

            valueCounts[value]++;

            wxString align = EASYEDAPRO::V3GetString( row.inner, "align", wxS( "LEFT_BOTTOM" ) );
            int      rotation = EASYEDAPRO::V3GetInt( row.inner, "rotation", 0 );

            if( align.StartsWith( wxS( "RIGHT" ) ) )
                candidates.push_back( { value, align, rotation } );
        }
    }

    BOOST_REQUIRE( !candidates.empty() );

    NET_ATTR selected = candidates.front();

    for( const NET_ATTR& candidate : candidates )
    {
        if( valueCounts[candidate.value] == 1 )
        {
            selected = candidate;
            break;
        }
    }

    SPIN_STYLE expectedSpinStyle = SPIN_STYLE::RIGHT;

    if( selected.align.StartsWith( wxS( "RIGHT" ) ) )
        expectedSpinStyle = SPIN_STYLE::LEFT;
    else if( selected.align.StartsWith( wxS( "LEFT" ) ) )
        expectedSpinStyle = SPIN_STYLE::RIGHT;

    int rot = selected.rotation % 360;

    if( rot < 0 )
        rot += 360;

    if( rot % 90 == 0 )
    {
        for( int i = 0; i < rot; i += 90 )
            expectedSpinStyle = expectedSpinStyle.RotateCCW();
    }

    // Load via the plugin and validate the resulting schematic keeps label spin style.
    Pgm().GetSettingsManager().LoadProject( projectFile.GetFullPath().ToStdString() );

    SCHEMATIC schematic( nullptr );
    schematic.SetProject( &Pgm().GetSettingsManager().Prj() );

    IO_RELEASER<SCH_IO> plugin( SCH_IO_MGR::FindPlugin( SCH_IO_MGR::SCH_EASYEDAPRO_V3 ) );
    BOOST_REQUIRE( plugin );

    SCH_SHEET* rootSheet = nullptr;
    BOOST_REQUIRE_NO_THROW( rootSheet =
                                    plugin->LoadSchematicFile( archiveFile.GetFullPath(),
                                                               &schematic, nullptr, nullptr ) );
    BOOST_REQUIRE( rootSheet );

    schematic.RefreshHierarchy();

    int  foundCount = 0;
    bool hasExpectedSpinStyle = false;

    for( const SCH_SHEET_PATH& sheetPath : schematic.Hierarchy() )
    {
        SCH_SCREEN* screen = sheetPath.LastScreen();

        if( !screen )
            continue;

        for( SCH_ITEM* item : screen->Items().OfType( SCH_LABEL_T ) )
        {
            SCH_LABEL* label = static_cast<SCH_LABEL*>( item );

            if( label->GetText() != selected.value )
                continue;

            foundCount++;

            if( static_cast<int>( label->GetSpinStyle() ) == static_cast<int>( expectedSpinStyle ) )
                hasExpectedSpinStyle = true;
        }
    }

    BOOST_CHECK_EQUAL( foundCount, 1 );
    BOOST_CHECK_MESSAGE( hasExpectedSpinStyle,
                         wxString::Format( "Label '%s' spin style mismatch", selected.value ) );

    BOOST_CHECK( wxFileName::Rmdir( tempDir, wxPATH_RMDIR_RECURSIVE ) );
}


BOOST_AUTO_TEST_CASE( PluginLoadConvertsNetportsToGlobalLabels )
{
    wxString tempDir = wxFileName::CreateTempFileName( "easyedapro_v3_import" );

    BOOST_REQUIRE( wxFileExists( tempDir ) );
    BOOST_REQUIRE( wxRemoveFile( tempDir ) );
    BOOST_REQUIRE( wxMkdir( tempDir ) );

    wxFileName projectFile( tempDir, "easyedapro_v3_test", "kicad_pro" );
    wxFileName archiveFile( tempDir, "sample", "epro2" );

    BOOST_REQUIRE( wxCopyFile( getEasyEdaProV3ArchivePath(), archiveFile.GetFullPath() ) );

    // Extract expected netport instance names from the raw v3 docs.
    EASYEDAPRO::V3_DOC_PARSER v3( archiveFile.GetFullPath() );
    BOOST_REQUIRE_NO_THROW( v3.Load() );

    std::set<wxString> netportSymbolUuids;

    for( const auto& [uuid, symDoc] : v3.GetRawDocs( wxS( "SYMBOL" ) ) )
    {
        for( const EASYEDAPRO::V3_ROW& row : symDoc.rows )
        {
            if( row.type == wxS( "META" )
                && EASYEDAPRO::V3GetInt( row.inner, "docType", 0 ) == 19 )
            {
                netportSymbolUuids.insert( uuid );
                break;
            }
        }
    }

    BOOST_REQUIRE( !netportSymbolUuids.empty() );

    std::set<wxString> netportComponentIds;

    for( const auto& [pageUuid, pageDoc] : v3.GetRawDocs( wxS( "SCH_PAGE" ) ) )
    {
        for( const EASYEDAPRO::V3_ROW& row : pageDoc.rows )
        {
            if( row.type != wxS( "ATTR" ) )
                continue;

            if( EASYEDAPRO::V3GetString( row.inner, "key" ) != wxS( "Symbol" ) )
                continue;

            wxString symbolUuid = EASYEDAPRO::V3GetString( row.inner, "value" );

            if( netportSymbolUuids.count( symbolUuid ) )
                netportComponentIds.insert( EASYEDAPRO::V3GetString( row.inner, "parentId" ) );
        }
    }

    BOOST_REQUIRE( !netportComponentIds.empty() );

    std::set<wxString> expectedNetportNames;

    for( const auto& [pageUuid, pageDoc] : v3.GetRawDocs( wxS( "SCH_PAGE" ) ) )
    {
        for( const EASYEDAPRO::V3_ROW& row : pageDoc.rows )
        {
            if( row.type != wxS( "ATTR" ) )
                continue;

            wxString parentId = EASYEDAPRO::V3GetString( row.inner, "parentId" );

            if( !netportComponentIds.count( parentId ) )
                continue;

            if( EASYEDAPRO::V3GetString( row.inner, "key" ) != wxS( "Name" ) )
                continue;

            wxString name = EASYEDAPRO::V3JsonToString(
                    row.inner.value( "value", nlohmann::json() ) );

            if( !name.IsEmpty() )
                expectedNetportNames.insert( name );
        }
    }

    BOOST_REQUIRE( !expectedNetportNames.empty() );

    // Load via the plugin and validate the resulting schematic contains global labels (not symbols).
    Pgm().GetSettingsManager().LoadProject( projectFile.GetFullPath().ToStdString() );

    SCHEMATIC schematic( nullptr );
    schematic.SetProject( &Pgm().GetSettingsManager().Prj() );

    IO_RELEASER<SCH_IO> plugin( SCH_IO_MGR::FindPlugin( SCH_IO_MGR::SCH_EASYEDAPRO_V3 ) );
    BOOST_REQUIRE( plugin );

    SCH_SHEET* rootSheet = nullptr;
    BOOST_REQUIRE_NO_THROW( rootSheet =
                                    plugin->LoadSchematicFile( archiveFile.GetFullPath(),
                                                               &schematic, nullptr, nullptr ) );
    BOOST_REQUIRE( rootSheet );

    schematic.RefreshHierarchy();

    int  netportSymbolCount = 0;
    int  globalLabelCount = 0;
    bool hasInputLabel = false;
    bool hasOutputLabel = false;
    bool hasBidiLabel = false;

    std::set<wxString> importedGlobalLabelNames;

    for( const SCH_SHEET_PATH& sheetPath : schematic.Hierarchy() )
    {
        SCH_SCREEN* screen = sheetPath.LastScreen();

        if( !screen )
            continue;

        for( SCH_ITEM* item : screen->Items().OfType( SCH_SYMBOL_T ) )
        {
            SCH_SYMBOL* symbol = static_cast<SCH_SYMBOL*>( item );
            wxString    ref = symbol->GetRef( &sheetPath, false );

            if( ref.StartsWith( wxS( "#NET" ) ) )
                netportSymbolCount++;
        }

        for( SCH_ITEM* item : screen->Items().OfType( SCH_GLOBAL_LABEL_T ) )
        {
            SCH_GLOBALLABEL* label = static_cast<SCH_GLOBALLABEL*>( item );

            if( label->GetText().IsEmpty() )
                continue;

            globalLabelCount++;
            importedGlobalLabelNames.insert( label->GetText() );

            switch( label->GetShape() )
            {
            case LABEL_FLAG_SHAPE::L_INPUT:  hasInputLabel = true;  break;
            case LABEL_FLAG_SHAPE::L_OUTPUT: hasOutputLabel = true; break;
            case LABEL_FLAG_SHAPE::L_BIDI:   hasBidiLabel = true;   break;
            default: break;
            }
        }
    }

    BOOST_CHECK_EQUAL( netportSymbolCount, 0 );
    BOOST_CHECK( globalLabelCount > 0 );

    // The test project contains Netport-IN/OUT/BI symbols; ensure we preserve their direction.
    BOOST_CHECK( hasInputLabel );
    BOOST_CHECK( hasOutputLabel );
    BOOST_CHECK( hasBidiLabel );

    // All netport instance names should import as global labels.
    for( const wxString& expected : expectedNetportNames )
    {
        BOOST_CHECK_MESSAGE( importedGlobalLabelNames.count( expected ) > 0,
                             wxString::Format( "Missing global label '%s'", expected ) );
    }

    BOOST_CHECK( wxFileName::Rmdir( tempDir, wxPATH_RMDIR_RECURSIVE ) );
}


BOOST_AUTO_TEST_CASE( PluginLoadKeepsSchematicVerticalOrderAndFootprints )
{
    wxString tempDir = wxFileName::CreateTempFileName( "easyedapro_v3_import" );

    BOOST_REQUIRE( wxFileExists( tempDir ) );
    BOOST_REQUIRE( wxRemoveFile( tempDir ) );
    BOOST_REQUIRE( wxMkdir( tempDir ) );

    wxFileName projectFile( tempDir, "easyedapro_v3_test", "kicad_pro" );
    wxFileName archiveFile( tempDir, "sample", "epro2" );

    BOOST_REQUIRE( wxCopyFile( getEasyEdaProV3ArchivePath(), archiveFile.GetFullPath() ) );

    Pgm().GetSettingsManager().LoadProject( projectFile.GetFullPath().ToStdString() );

    SCHEMATIC schematic( nullptr );
    schematic.SetProject( &Pgm().GetSettingsManager().Prj() );

    IO_RELEASER<SCH_IO> plugin( SCH_IO_MGR::FindPlugin( SCH_IO_MGR::SCH_EASYEDAPRO_V3 ) );
    BOOST_REQUIRE( plugin );

    SCH_SHEET* rootSheet = nullptr;
    BOOST_REQUIRE_NO_THROW( rootSheet =
                                    plugin->LoadSchematicFile( archiveFile.GetFullPath(),
                                                               &schematic, nullptr, nullptr ) );
    BOOST_REQUIRE( rootSheet );

    schematic.RefreshHierarchy();

    SCH_SYMBOL* u6 = nullptr;
    SCH_SYMBOL* u7 = nullptr;
    SCH_SYMBOL* u1 = nullptr;

    for( const SCH_SHEET_PATH& sheetPath : schematic.Hierarchy() )
    {
        SCH_SCREEN* screen = sheetPath.LastScreen();

        if( !screen )
            continue;

        for( SCH_ITEM* item : screen->Items().OfType( SCH_SYMBOL_T ) )
        {
            SCH_SYMBOL* symbol = static_cast<SCH_SYMBOL*>( item );
            wxString    ref = symbol->GetRef( &sheetPath, false );

            if( ref == wxS( "U6" ) )
                u6 = symbol;
            else if( ref == wxS( "U7" ) )
                u7 = symbol;
            else if( ref == wxS( "U1" ) )
                u1 = symbol;
        }
    }

    BOOST_REQUIRE( u6 );
    BOOST_REQUIRE( u7 );
    BOOST_REQUIRE( u1 );

    BOOST_CHECK_MESSAGE( u7->GetPosition().y < u6->GetPosition().y,
                         "U7 should remain above U6 after EasyEDA Pro v3 import" );

    wxString u6Footprint = u6->GetFootprintFieldText( false, nullptr, false );
    wxString u1Footprint = u1->GetFootprintFieldText( false, nullptr, false );

    BOOST_CHECK( !u6Footprint.IsEmpty() );
    BOOST_CHECK( u6Footprint.Contains( wxS( "SOT-563-6" ) ) );

    BOOST_CHECK( !u1Footprint.IsEmpty() );
    BOOST_CHECK( u1Footprint.Contains( wxS( "BGA-286_17x17_12.0x12.0mm" ) ) );

    BOOST_CHECK( wxFileName::Rmdir( tempDir, wxPATH_RMDIR_RECURSIVE ) );
}


BOOST_AUTO_TEST_CASE( PluginLoadPowerSymbolsKeepValueAndVisibility )
{
    wxString tempDir = wxFileName::CreateTempFileName( "easyedapro_v3_import" );

    BOOST_REQUIRE( wxFileExists( tempDir ) );
    BOOST_REQUIRE( wxRemoveFile( tempDir ) );
    BOOST_REQUIRE( wxMkdir( tempDir ) );

    wxFileName projectFile( tempDir, "easyedapro_v3_test", "kicad_pro" );
    wxFileName archiveFile( tempDir, "sample", "epro2" );

    BOOST_REQUIRE( wxCopyFile( getEasyEdaProV3ArchivePath(), archiveFile.GetFullPath() ) );

    Pgm().GetSettingsManager().LoadProject( projectFile.GetFullPath().ToStdString() );

    SCHEMATIC schematic( nullptr );
    schematic.SetProject( &Pgm().GetSettingsManager().Prj() );

    IO_RELEASER<SCH_IO> plugin( SCH_IO_MGR::FindPlugin( SCH_IO_MGR::SCH_EASYEDAPRO_V3 ) );
    BOOST_REQUIRE( plugin );

    SCH_SHEET* rootSheet = nullptr;
    BOOST_REQUIRE_NO_THROW( rootSheet =
                                    plugin->LoadSchematicFile( archiveFile.GetFullPath(),
                                                               &schematic, nullptr, nullptr ) );
    BOOST_REQUIRE( rootSheet );

    schematic.RefreshHierarchy();

    int plus5vCount = 0;
    int plus5vShownCount = 0;
    int plus5vOffsetCount = 0;
    int plus5vUprightCount = 0;
    int plus5vUprightAtSymbolXCount = 0;
    int powerValueOnPinBodySideCheckedCount = 0;
    int powerValueOnWrongPinSideCount = 0;
    int otherPowerValueCount = 0;
    int otherPowerShownCount = 0;

    for( const SCH_SHEET_PATH& sheetPath : schematic.Hierarchy() )
    {
        SCH_SCREEN* screen = sheetPath.LastScreen();

        if( !screen )
            continue;

        for( SCH_ITEM* item : screen->Items().OfType( SCH_SYMBOL_T ) )
        {
            SCH_SYMBOL* symbol = static_cast<SCH_SYMBOL*>( item );

            if( !symbol->IsGlobalPower() )
                continue;

            SCH_FIELD* valueField = symbol->GetField( FIELD_T::VALUE );

            if( !valueField )
                continue;

            wxString valueText = valueField->GetShownText( &sheetPath, false );

            if( valueField->IsVisible() && !valueText.IsEmpty() )
            {
                VECTOR2I delta = valueField->GetPosition() - symbol->GetPosition();

                if( delta != VECTOR2I() )
                {
                    std::vector<SCH_PIN*> pins = symbol->GetPins( &sheetPath );

                    if( !pins.empty() )
                    {
                        VECTOR2I dir;

                        switch( pins[0]->PinDrawOrient( symbol->GetTransform() ) )
                        {
                        case PIN_ORIENTATION::PIN_LEFT:  dir = VECTOR2I( -1, 0 ); break;
                        case PIN_ORIENTATION::PIN_RIGHT: dir = VECTOR2I( 1, 0 ); break;
                        case PIN_ORIENTATION::PIN_UP:    dir = VECTOR2I( 0, -1 ); break;
                        case PIN_ORIENTATION::PIN_DOWN:  dir = VECTOR2I( 0, 1 ); break;
                        default:                         dir = VECTOR2I(); break;
                        }

                        if( dir != VECTOR2I() )
                        {
                            long long dot = static_cast<long long>( delta.x ) * dir.x
                                            + static_cast<long long>( delta.y ) * dir.y;

                            powerValueOnPinBodySideCheckedCount++;

                            if( dot < 0 )
                                powerValueOnWrongPinSideCount++;
                        }
                    }
                }
            }

            if( valueText == wxS( "+5V" ) )
            {
                plus5vCount++;

                if( valueField->IsVisible() )
                    plus5vShownCount++;

                if( valueField->GetPosition().y != symbol->GetPosition().y )
                    plus5vOffsetCount++;

                if( symbol->GetOrientation() == SYM_ORIENT_0 )
                {
                    plus5vUprightCount++;

                    if( valueField->GetPosition().x == symbol->GetPosition().x )
                        plus5vUprightAtSymbolXCount++;

                    BOOST_CHECK_EQUAL( valueField->GetHorizJustify(), GR_TEXT_H_ALIGN_CENTER );
                    BOOST_CHECK_EQUAL( valueField->GetVertJustify(), GR_TEXT_V_ALIGN_CENTER );
                }
            }
            else if( valueText.StartsWith( wxS( "+" ) ) )
            {
                otherPowerValueCount++;

                if( valueField->IsVisible() )
                    otherPowerShownCount++;
            }
        }
    }

    BOOST_CHECK( plus5vCount > 0 );
    BOOST_CHECK_EQUAL( plus5vShownCount, plus5vCount );
    BOOST_CHECK( plus5vOffsetCount > 0 );
    BOOST_CHECK( plus5vUprightCount > 0 );
    BOOST_CHECK_EQUAL( plus5vUprightAtSymbolXCount, plus5vUprightCount );

    BOOST_CHECK( powerValueOnPinBodySideCheckedCount > 0 );
    BOOST_CHECK_EQUAL( powerValueOnWrongPinSideCount, 0 );

    BOOST_CHECK( otherPowerValueCount > 0 );
    BOOST_CHECK_EQUAL( otherPowerShownCount, otherPowerValueCount );

    BOOST_CHECK( wxFileName::Rmdir( tempDir, wxPATH_RMDIR_RECURSIVE ) );
}


BOOST_AUTO_TEST_SUITE_END()
