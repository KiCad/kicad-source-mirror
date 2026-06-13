/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
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

#include <qa_utils/wx_utils/unit_test_utils.h>
#include <pcbnew_utils/board_test_utils.h>

#include <diff_merge/pcb_differ.h>
#include <diff_merge/pcb_geometry_extractor.h>

#include <board.h>
#include <board_item.h>
#include <pad.h>
#include <pcb_track.h>
#include <footprint.h>
#include <settings/settings_manager.h>

#include <nlohmann/json.hpp>

#include <map>


using namespace KICAD_DIFF;


/**
 * Fixture: loads the same canonical board into two separate BOARD instances so
 * tests can mutate one and diff against the other.
 *
 * `complex_hierarchy.kicad_pcb` is a hierarchical board with multiple footprints,
 * tracks, vias and zones — wide-enough surface to exercise every code path of
 * PCB_DIFFER.
 */
struct PCB_DIFFER_FIXTURE
{
    PCB_DIFFER_FIXTURE()
    {
        KI_TEST::LoadBoard( m_settingsA, "complex_hierarchy", m_before );
        KI_TEST::LoadBoard( m_settingsB, "complex_hierarchy", m_after );

        BOOST_REQUIRE( m_before );
        BOOST_REQUIRE( m_after );
    }

    SETTINGS_MANAGER       m_settingsA;
    SETTINGS_MANAGER       m_settingsB;
    std::unique_ptr<BOARD> m_before;
    std::unique_ptr<BOARD> m_after;
};


BOOST_FIXTURE_TEST_SUITE( PcbDiffer, PCB_DIFFER_FIXTURE )


BOOST_AUTO_TEST_CASE( TwoFreshLoadsAreIdentical )
{
    PCB_DIFFER    differ( m_before.get(), m_after.get(), wxS( "complex_hierarchy.kicad_pcb" ) );
    DOCUMENT_DIFF result = differ.Diff();

    BOOST_CHECK_EQUAL( result.docType.ToStdString(), "kicad_pcb" );
    BOOST_CHECK_MESSAGE( result.Empty(), "Two fresh loads of the same fixture should produce no diff; "
                                         "got " << result.changes.size()
                                                << " change records" );
}


BOOST_AUTO_TEST_CASE( RemovingAnItemSurfacesAsRemoved )
{
    BOARD_ITEM_SET items = m_after->GetItemSet();
    BOOST_REQUIRE( !items.empty() );

    // Pick the first track from the fixture and remove it from the after-board.
    PCB_TRACK* victim = nullptr;

    for( BOARD_ITEM* item : items )
    {
        if( item && item->Type() == PCB_TRACE_T )
        {
            victim = static_cast<PCB_TRACK*>( item );
            break;
        }
    }

    BOOST_REQUIRE( victim );
    KIID victimUuid = victim->m_Uuid;
    m_after->Remove( victim, REMOVE_MODE::NORMAL );

    PCB_DIFFER    differ( m_before.get(), m_after.get() );
    DOCUMENT_DIFF result = differ.Diff();

    BOOST_REQUIRE_EQUAL( result.changes.size(), 1u );
    BOOST_CHECK( result.changes[0].kind == CHANGE_KIND::REMOVED );
    BOOST_CHECK_EQUAL( result.changes[0].id.back().AsString().ToStdString(), victimUuid.AsString().ToStdString() );

    delete victim;
}


BOOST_AUTO_TEST_CASE( WidthChangeProducesPropertyDelta )
{
    BOARD_ITEM_SET items = m_after->GetItemSet();
    PCB_TRACK*     subject = nullptr;

    for( BOARD_ITEM* item : items )
    {
        if( item && item->Type() == PCB_TRACE_T )
        {
            subject = static_cast<PCB_TRACK*>( item );
            break;
        }
    }

    BOOST_REQUIRE( subject );

    int originalWidth = subject->GetWidth();
    int newWidth = originalWidth + 50000;
    subject->SetWidth( newWidth );

    PCB_DIFFER    differ( m_before.get(), m_after.get() );
    DOCUMENT_DIFF result = differ.Diff();

    BOOST_REQUIRE_EQUAL( result.changes.size(), 1u );
    BOOST_CHECK( result.changes[0].kind == CHANGE_KIND::MODIFIED );
    BOOST_CHECK_EQUAL( result.changes[0].id.back().AsString().ToStdString(), subject->m_Uuid.AsString().ToStdString() );

    bool foundWidthDelta = false;

    for( const PROPERTY_DELTA& d : result.changes[0].properties )
    {
        if( d.name.Lower().Contains( wxS( "width" ) ) )
        {
            foundWidthDelta = true;
            BOOST_CHECK_EQUAL( d.before.AsInt(), originalWidth );
            BOOST_CHECK_EQUAL( d.after.AsInt(), newWidth );
        }
    }

    BOOST_CHECK_MESSAGE( foundWidthDelta, "Expected Width property delta after SetWidth() on a track" );
}


BOOST_AUTO_TEST_CASE( RoutingChangesCarryNetNamesForPresentationGrouping )
{
    BOARD_ITEM_SET                 items = m_after->GetItemSet();
    std::map<wxString, PCB_TRACK*> tracksByNet;
    wxString                       selectedNet;
    PCB_TRACK*                     first = nullptr;
    PCB_TRACK*                     second = nullptr;

    for( BOARD_ITEM* item : items )
    {
        if( !item || ( item->Type() != PCB_TRACE_T && item->Type() != PCB_VIA_T ) )
            continue;

        PCB_TRACK* track = static_cast<PCB_TRACK*>( item );

        if( track->GetNetCode() <= 0 || track->GetNetname().IsEmpty() )
            continue;

        wxString netName = track->GetNetname();

        if( tracksByNet.contains( netName ) )
        {
            selectedNet = netName;
            first = tracksByNet[netName];
            second = track;
            break;
        }

        tracksByNet[netName] = track;
    }

    BOOST_REQUIRE( first );
    BOOST_REQUIRE( second );

    first->SetWidth( first->GetWidth() + 25000 );
    second->SetWidth( second->GetWidth() + 50000 );

    PCB_DIFFER    differ( m_before.get(), m_after.get() );
    DOCUMENT_DIFF result = differ.Diff();

    std::size_t selectedNetChanges = 0;

    for( const ITEM_CHANGE& change : result.changes )
    {
        if( change.refdes == selectedNet )
        {
            ++selectedNetChanges;
            BOOST_CHECK( change.typeName == wxS( "PCB_TRACK" ) || change.typeName == wxS( "PCB_VIA" ) );
        }
    }

    BOOST_CHECK_EQUAL( selectedNetChanges, 2u );
}


BOOST_AUTO_TEST_CASE( ChildEditOnFootprintNestsUnderFootprintRecord )
{
    BOARD_ITEM_SET items = m_after->GetItemSet();
    FOOTPRINT*     fp = nullptr;

    for( BOARD_ITEM* item : items )
    {
        if( item && item->Type() == PCB_FOOTPRINT_T )
        {
            fp = static_cast<FOOTPRINT*>( item );

            if( !fp->Pads().empty() )
                break;

            fp = nullptr;
        }
    }

    BOOST_REQUIRE( fp );
    BOOST_REQUIRE( !fp->Pads().empty() );

    PAD* pad = fp->Pads().front();
    pad->SetNumber( pad->GetNumber() + wxS( "X" ) );

    PCB_DIFFER    differ( m_before.get(), m_after.get() );
    DOCUMENT_DIFF result = differ.Diff();

    // We expect exactly one parent change (the footprint), with one child change (the pad).
    BOOST_REQUIRE_EQUAL( result.changes.size(), 1u );
    BOOST_CHECK( result.changes[0].kind == CHANGE_KIND::MODIFIED );
    BOOST_CHECK_EQUAL( result.changes[0].id.back().AsString().ToStdString(), fp->m_Uuid.AsString().ToStdString() );
    BOOST_REQUIRE_EQUAL( result.changes[0].children.size(), 1u );
    BOOST_CHECK( result.changes[0].children[0].kind == CHANGE_KIND::MODIFIED );
    BOOST_CHECK_EQUAL( result.changes[0].children[0].id.back().AsString().ToStdString(),
                       pad->m_Uuid.AsString().ToStdString() );
}


BOOST_AUTO_TEST_CASE( DiffOutputIsDeterministic )
{
    BOARD_ITEM_SET items = m_after->GetItemSet();
    PCB_TRACK*     subject = nullptr;

    for( BOARD_ITEM* item : items )
    {
        if( item && item->Type() == PCB_TRACE_T )
        {
            subject = static_cast<PCB_TRACK*>( item );
            break;
        }
    }

    BOOST_REQUIRE( subject );
    subject->SetWidth( subject->GetWidth() + 25000 );

    PCB_DIFFER differ1( m_before.get(), m_after.get() );
    PCB_DIFFER differ2( m_before.get(), m_after.get() );

    DOCUMENT_DIFF r1 = differ1.Diff();
    DOCUMENT_DIFF r2 = differ2.Diff();

    BOOST_CHECK_EQUAL( r1.ToJson().dump(), r2.ToJson().dump() );
}


BOOST_AUTO_TEST_CASE( DiffJsonRoundTrip )
{
    BOARD_ITEM_SET items = m_after->GetItemSet();
    PCB_TRACK*     subject = nullptr;

    for( BOARD_ITEM* item : items )
    {
        if( item && item->Type() == PCB_TRACE_T )
        {
            subject = static_cast<PCB_TRACK*>( item );
            break;
        }
    }

    BOOST_REQUIRE( subject );
    subject->SetWidth( subject->GetWidth() + 30000 );

    PCB_DIFFER    differ( m_before.get(), m_after.get(), wxS( "complex_hierarchy.kicad_pcb" ) );
    DOCUMENT_DIFF result = differ.Diff();

    nlohmann::json j = result.ToJson();
    DOCUMENT_DIFF  back = DOCUMENT_DIFF::FromJson( j );

    BOOST_CHECK_EQUAL( back.path.ToStdString(), result.path.ToStdString() );
    BOOST_CHECK_EQUAL( back.docType.ToStdString(), result.docType.ToStdString() );
    BOOST_REQUIRE_EQUAL( back.changes.size(), result.changes.size() );
    BOOST_CHECK( back.changes[0].kind == result.changes[0].kind );
    BOOST_CHECK_EQUAL( back.changes[0].properties.size(), result.changes[0].properties.size() );
}


BOOST_AUTO_TEST_CASE( ExtractedGeometryCarriesBoardLayers )
{
    const KIGFX::COLOR4D color( 0.4, 0.4, 0.4, 0.6 );
    DOCUMENT_GEOMETRY    geometry = ExtractBoardGeometry( *m_after, color );

    BOOST_CHECK( !geometry.Empty() );

    LSET expectedTrackLayers;

    for( const PCB_TRACK* track : m_after->Tracks() )
    {
        if( track )
            expectedTrackLayers |= track->GetLayerSet();
    }

    BOOST_REQUIRE( expectedTrackLayers.any() );

    const LSET geometryLayers = GeometryLayerSet( geometry );
    BOOST_CHECK( ( geometryLayers & expectedTrackLayers ).any() );

    bool segmentWithLayer = false;

    for( const DOCUMENT_SEGMENT& segment : geometry.segments )
    {
        if( segment.layers.any() )
        {
            segmentWithLayer = true;
            break;
        }
    }

    BOOST_CHECK( segmentWithLayer );
}


BOOST_AUTO_TEST_SUITE_END()
