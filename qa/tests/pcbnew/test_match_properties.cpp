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


#include <board.h>
#include <footprint.h>
#include <pad.h>
#include <pcb_barcode.h>
#include <pcb_dimension.h>
#include <pcb_group.h>
#include <pcb_reference_image.h>
#include <pcb_shape.h>
#include <pcb_table.h>
#include <pcb_tablecell.h>
#include <pcb_target.h>
#include <pcb_text.h>
#include <pcb_textbox.h>
#include <pcb_track.h>
#include <properties/property_mgr.h>
#include <tools/match_properties.h>
#include <zone.h>


BOOST_AUTO_TEST_SUITE( MatchProperties )


// Tracks and arcs are one family.  A shape is not.  CompatibleTargets() drops nulls, the source
// itself and other families.
BOOST_AUTO_TEST_CASE( CompatibleFamiliesAndTargets )
{
    PCB_TRACK trackSource( nullptr );
    PCB_TRACK trackTarget( nullptr );
    PCB_ARC   arcTarget( nullptr );
    PCB_SHAPE shape;

    BOOST_CHECK( MATCH_PROPERTIES_CATALOG::Compatible( trackSource, trackTarget ) );
    BOOST_CHECK( MATCH_PROPERTIES_CATALOG::Compatible( trackSource, arcTarget ) );

    // A track and a graphic both understand a line width, so they meet over that.
    BOOST_CHECK( MATCH_PROPERTIES_CATALOG::Compatible( trackSource, shape ) );

    std::vector<EDA_ITEM*> targets = MATCH_PROPERTIES_CATALOG::CompatibleTargets(
            trackSource, { &shape, &trackTarget, nullptr, &trackSource, &arcTarget } );

    BOOST_REQUIRE_EQUAL( targets.size(), 3 );
    BOOST_CHECK_EQUAL( targets[0], &shape );
    BOOST_CHECK_EQUAL( targets[1], &trackTarget );
    BOOST_CHECK_EQUAL( targets[2], &arcTarget );
}


// Kinds with nothing in common stay apart, or hovering would light up the whole board.
BOOST_AUTO_TEST_CASE( UnrelatedKindsAreNotCompatible )
{
    PCB_VIA  via( nullptr );
    PCB_TEXT text( static_cast<BOARD_ITEM*>( nullptr ) );

    BOOST_CHECK( !MATCH_PROPERTIES_CATALOG::Compatible( via, text ) );
    BOOST_CHECK( !MATCH_PROPERTIES_CATALOG::Compatible( text, via ) );
}


// The width of a track and the line width of a graphic are the same idea under two names.
BOOST_AUTO_TEST_CASE( CommonPropertyCopiesAcrossKinds )
{
    PCB_TRACK source( nullptr );
    PCB_SHAPE target;

    source.SetWidth( 400000 );
    target.SetWidth( 200000 );

    MATCH_PROPERTIES_RESULT result =
            MATCH_PROPERTIES_CATALOG::Copy( source, target, { wxS( "common/Line Width" ) } );

    BOOST_CHECK( result );
    BOOST_CHECK_EQUAL( target.GetWidth(), 400000 );
    BOOST_CHECK_EQUAL( result.m_Changed, 1 );
}


// A common key still carries within one kind, which is how it replaced the per-family keys.
BOOST_AUTO_TEST_CASE( CommonPropertyCopiesWithinOneKind )
{
    PCB_TRACK source( nullptr );
    PCB_TRACK target( nullptr );

    source.SetWidth( 400000 );
    target.SetWidth( 200000 );

    MATCH_PROPERTIES_RESULT result =
            MATCH_PROPERTIES_CATALOG::Copy( source, target, { wxS( "common/Line Width" ) } );

    BOOST_CHECK( result );
    BOOST_CHECK_EQUAL( target.GetWidth(), 400000 );
}


// Crossing kinds carries only what is common; the source's own keys stay behind.
BOOST_AUTO_TEST_CASE( CrossKindCopyIgnoresFamilyOnlyKeys )
{
    PCB_TRACK source( nullptr );
    PCB_SHAPE target;

    source.SetWidth( 400000 );
    target.SetWidth( 200000 );

    // A family key of the source names nothing the target should take.
    MATCH_PROPERTIES_RESULT result = MATCH_PROPERTIES_CATALOG::Copy( source, target, { wxS( "route/Width" ) } );

    BOOST_CHECK( result );
    BOOST_CHECK_EQUAL( result.m_Changed, 0 );
    BOOST_CHECK_EQUAL( target.GetWidth(), 200000 );
}


// A common key stands in for the per-family keys it covers, so the same idea is offered once.
BOOST_AUTO_TEST_CASE( CommonKeysReplaceTheFamilyKeysTheyCover )
{
    const std::set<wxString>& keys = MATCH_PROPERTIES_CATALOG::AllSafeKeys();

    BOOST_CHECK( keys.contains( wxS( "common/Line Width" ) ) );
    BOOST_CHECK( !keys.contains( wxS( "route/Width" ) ) );
    BOOST_CHECK( !keys.contains( wxS( "shape/Line Width" ) ) );

    // Reaching two kinds is what makes it common; the mapping must name both.
    std::set<wxString> families = MATCH_PROPERTIES_CATALOG::FamiliesFor( wxS( "common/Line Width" ) );

    BOOST_CHECK( families.contains( wxS( "route" ) ) );
    BOOST_CHECK( families.contains( wxS( "shape" ) ) );

    // A family key still answers for itself.
    BOOST_CHECK( MATCH_PROPERTIES_CATALOG::FamiliesFor( wxS( "via/Diameter" ) )
                 == std::set<wxString>{ wxS( "via" ) } );
}


// The keys are persisted settings strings.  Pinning them is behaviour.
BOOST_AUTO_TEST_CASE( KeysAreCanonicalAndLabelsAreFriendly )
{
    PCB_TRACK      track( nullptr );
    const wxString label = MATCH_PROPERTIES_CATALOG::DisplayLabel( wxS( "route/Width" ) );

    BOOST_CHECK_EQUAL( MATCH_PROPERTIES_CATALOG::Family( track ), wxString( wxS( "route" ) ) );
    BOOST_CHECK( label.Contains( _( "Tracks and Arcs" ) ) );
    BOOST_CHECK( label.Contains( wxGetTranslation( wxS( "Width" ) ) ) );
    BOOST_CHECK( !label.Contains( wxS( "route/" ) ) );
}


// The settings tree groups by family and labels the rows with the property alone, so the two
// halves of DisplayLabel() have to stand on their own.
BOOST_AUTO_TEST_CASE( FamilyAndPropertyLabelsSplitTheKey )
{
    BOOST_CHECK_EQUAL( MATCH_PROPERTIES_CATALOG::FamilyLabel( wxS( "route" ) ), _( "Tracks and Arcs" ) );
    BOOST_CHECK_EQUAL( MATCH_PROPERTIES_CATALOG::PropertyLabel( wxS( "route/Width" ) ),
                       wxGetTranslation( wxS( "Width" ) ) );

    // The property name carries no family, so a group heading can never repeat it.
    BOOST_CHECK( !MATCH_PROPERTIES_CATALOG::PropertyLabel( wxS( "route/Width" ) ).Contains( wxS( "route" ) ) );

    // An unknown family still names itself rather than coming back empty.
    BOOST_CHECK_EQUAL( MATCH_PROPERTIES_CATALOG::FamilyLabel( wxS( "nonesuch" ) ), wxString( wxS( "nonesuch" ) ) );
}


// BOARD_CONNECTED_ITEM replaces BOARD_ITEM's Layer with a copper-restricted one.  That
// replacement has to carry the copyable flag too, or every connected item silently loses Layer.
BOOST_AUTO_TEST_CASE( ConnectedItemsOfferTheirLayer )
{
    PCB_TRACK track( nullptr );

    BOOST_CHECK( MATCH_PROPERTIES_CATALOG::AllSafeKeys().contains( wxS( "route/Layer" ) ) );

    PCB_TRACK source( nullptr );
    PCB_TRACK target( nullptr );

    source.SetLayer( In1_Cu );
    target.SetLayer( F_Cu );

    MATCH_PROPERTIES_RESULT result = MATCH_PROPERTIES_CATALOG::Copy( source, target, { wxS( "route/Layer" ) } );

    BOOST_CHECK( result );
    BOOST_CHECK( target.GetLayer() == In1_Cu );
}


// A track's layer must be a copper one, so it is not offered as something kinds share.  A
// graphic's silkscreen layer would otherwise land on a track.
BOOST_AUTO_TEST_CASE( LayerIsNotSharedWithConnectedItems )
{
    std::set<wxString> families = MATCH_PROPERTIES_CATALOG::FamiliesFor( wxS( "common/Layer" ) );

    BOOST_CHECK( families.contains( wxS( "shape" ) ) );
    BOOST_CHECK( !families.contains( wxS( "route" ) ) );
}


// Fill is registered on text boxes and table cells but their availability func refuses it.
// Pairing kinds over it would say they are compatible and then copy nothing.
BOOST_AUTO_TEST_CASE( FillIsNotSharedWhereItCannotApply )
{
    std::set<wxString> families = MATCH_PROPERTIES_CATALOG::FamiliesFor( wxS( "common/Fill" ) );

    BOOST_CHECK( !families.contains( wxS( "textbox" ) ) );
    BOOST_CHECK( !families.contains( wxS( "table_cell" ) ) );

    // With nothing left in common the two kinds must not read as compatible.
    PCB_SHAPE     shape;
    PCB_TABLE     table( nullptr, 0 );
    PCB_TABLECELL cell( &table );

    BOOST_CHECK( !MATCH_PROPERTIES_CATALOG::Compatible( shape, cell ) );
}


// An unsafe default never copies, and says nothing.
BOOST_AUTO_TEST_CASE( DefaultKeysAreAllSafe )
{
    for( const wxString& key : MATCH_PROPERTIES_CATALOG::DefaultKeys() )
    {
        BOOST_CHECK_MESSAGE( MATCH_PROPERTIES_CATALOG::AllSafeKeys().contains( key ), "Unsafe default: " << key );
    }
}


BOOST_AUTO_TEST_CASE( AnyEnabledForMatchesOnlyTheItemsOwnFamily )
{
    PCB_TRACK track( nullptr );
    PCB_TABLE table( nullptr, 0 );
    PCB_GROUP group( nullptr );

    BOOST_CHECK( MATCH_PROPERTIES_CATALOG::AnyEnabledFor( track, { wxS( "route/Width" ) } ) );
    BOOST_CHECK( !MATCH_PROPERTIES_CATALOG::AnyEnabledFor( track, { wxS( "shape/Layer" ) } ) );
    BOOST_CHECK( !MATCH_PROPERTIES_CATALOG::AnyEnabledFor( group, MATCH_PROPERTIES_CATALOG::AllSafeKeys() ) );

    // "table/" must not pick up "table_cell/" keys.
    BOOST_CHECK( !MATCH_PROPERTIES_CATALOG::AnyEnabledFor( table, { wxS( "table_cell/Font" ) } ) );
    BOOST_CHECK( MATCH_PROPERTIES_CATALOG::AnyEnabledFor( table, { wxS( "table/Border Width" ) } ) );
}


// The enabled set is persisted and hand-editable.  An unsafe key must not write identity.
BOOST_AUTO_TEST_CASE( CopyIgnoresEnabledKeysOutsideTheCatalog )
{
    PCB_TRACK source( nullptr );
    PCB_TRACK target( nullptr );

    source.SetStart( VECTOR2I( 100000, 100000 ) );
    target.SetStart( VECTOR2I( 900000, 900000 ) );

    MATCH_PROPERTIES_RESULT result =
            MATCH_PROPERTIES_CATALOG::Copy( source, target, { wxS( "route/Start X" ), wxS( "route/Net" ) } );

    BOOST_CHECK( result );
    BOOST_CHECK_EQUAL( result.m_Changed, 0 );
    BOOST_CHECK_EQUAL( target.GetStart(), VECTOR2I( 900000, 900000 ) );
}


BOOST_AUTO_TEST_CASE( CopyLeavesTheSourceUntouched )
{
    PCB_TRACK source( nullptr );
    PCB_TRACK target( nullptr );

    source.SetWidth( 400000 );
    target.SetWidth( 200000 );

    BOOST_CHECK( MATCH_PROPERTIES_CATALOG::Copy( source, target, MATCH_PROPERTIES_CATALOG::AllSafeKeys() ) );
    BOOST_CHECK_EQUAL( source.GetWidth(), 400000 );
}


BOOST_AUTO_TEST_CASE( CopyTableCellTextProperties )
{
    PCB_TABLE     table( nullptr, 0 );
    PCB_TABLECELL source( &table );
    PCB_TABLECELL target( &table );

    source.SetTextThickness( 50000 );
    target.SetTextThickness( 10000 );

    MATCH_PROPERTIES_RESULT result =
            MATCH_PROPERTIES_CATALOG::Copy( source, target, { wxS( "table_cell/Thickness" ) } );

    BOOST_CHECK( result );
    BOOST_CHECK_EQUAL( target.GetTextThickness(), 50000 );
    BOOST_CHECK_EQUAL( result.m_Changed, 1 );
}


BOOST_AUTO_TEST_CASE( CopySafeProperties )
{
    PCB_TRACK source( nullptr );
    PCB_TRACK target( nullptr );

    source.SetWidth( 400000 );
    target.SetWidth( 200000 );

    MATCH_PROPERTIES_RESULT result = MATCH_PROPERTIES_CATALOG::Copy( source, target, { wxS( "route/Width" ) } );

    BOOST_CHECK( result );
    BOOST_CHECK_EQUAL( target.GetWidth(), 400000 );
    BOOST_CHECK_EQUAL( result.m_Changed, 1 );
}


// Fonts are the awkward case.  The value compares by name, not identity.
BOOST_AUTO_TEST_CASE( CopyDoesNotCountIdenticalValuesAsChanges )
{
    PCB_TRACK source( nullptr );
    PCB_TRACK target( nullptr );
    PCB_TEXT  textSource( static_cast<BOARD_ITEM*>( nullptr ) );
    PCB_TEXT  textTarget( static_cast<BOARD_ITEM*>( nullptr ) );

    source.SetWidth( 400000 );
    target.SetWidth( 400000 );

    BOOST_CHECK_EQUAL( MATCH_PROPERTIES_CATALOG::Copy( source, target, { wxS( "route/Width" ) } ).m_Changed, 0 );
    BOOST_CHECK_EQUAL(
            MATCH_PROPERTIES_CATALOG::Copy( textSource, textTarget, { wxS( "text/Font" ) } ).m_Changed, 0 );
}


BOOST_AUTO_TEST_CASE( CopyRejectsIncompatibleItemsWithoutMutation )
{
    PCB_TRACK source( nullptr );
    PCB_TEXT  target( static_cast<BOARD_ITEM*>( nullptr ) );

    source.SetWidth( 400000 );
    target.SetTextSize( VECTOR2I( 100000, 100000 ) );

    MATCH_PROPERTIES_RESULT result = MATCH_PROPERTIES_CATALOG::Copy( source, target, { wxS( "route/Width" ) } );

    BOOST_CHECK( !result );
    BOOST_CHECK_EQUAL( target.GetTextSize(), VECTOR2I( 100000, 100000 ) );
}


BOOST_AUTO_TEST_CASE( CopyZonePropertiesStagesHatchedModeBeforeHatchProperties )
{
    BOARD board;
    ZONE  source( &board );
    ZONE  target( &board );

    source.SetLayer( F_Cu );
    target.SetLayer( F_Cu );
    source.SetFillMode( ZONE_FILL_MODE::HATCH_PATTERN );
    source.SetHatchThickness( 500000 );
    target.SetFillMode( ZONE_FILL_MODE::POLYGONS );
    target.SetHatchThickness( 200000 );

    MATCH_PROPERTIES_RESULT result =
            MATCH_PROPERTIES_CATALOG::Copy( source, target, { wxS( "zone/Fill Mode" ), wxS( "zone/Hatch Width" ) } );

    BOOST_CHECK( result );
    BOOST_CHECK( target.GetFillMode() == ZONE_FILL_MODE::HATCH_PATTERN );
    BOOST_CHECK_EQUAL( target.GetHatchThickness(), 500000 );
    BOOST_CHECK_EQUAL( result.m_Changed, 2 );
}


BOOST_AUTO_TEST_CASE( CopyZonePropertiesFromSolidModeSkipsUnavailableHatchProperties )
{
    BOARD board;
    ZONE  source( &board );
    ZONE  target( &board );

    source.SetLayer( F_Cu );
    target.SetLayer( F_Cu );
    source.SetFillMode( ZONE_FILL_MODE::POLYGONS );
    source.SetHatchThickness( 500000 );
    target.SetFillMode( ZONE_FILL_MODE::HATCH_PATTERN );
    target.SetHatchThickness( 200000 );

    MATCH_PROPERTIES_RESULT result =
            MATCH_PROPERTIES_CATALOG::Copy( source, target, { wxS( "zone/Fill Mode" ), wxS( "zone/Hatch Width" ) } );

    BOOST_CHECK( result );
    BOOST_CHECK( target.GetFillMode() == ZONE_FILL_MODE::POLYGONS );
    BOOST_CHECK_EQUAL( target.GetHatchThickness(), 200000 );
    BOOST_CHECK_EQUAL( result.m_Changed, 1 );
}


BOOST_AUTO_TEST_CASE( CoupledPropertiesValidateAgainstFinalState )
{
    PCB_VIA source( nullptr );
    PCB_VIA target( nullptr );

    source.SetWidth( PADSTACK::ALL_LAYERS, 400000 );
    source.SetDrill( 200000 );
    target.SetWidth( PADSTACK::ALL_LAYERS, 600000 );
    target.SetDrill( 500000 );

    MATCH_PROPERTIES_RESULT result =
            MATCH_PROPERTIES_CATALOG::Copy( source, target, { wxS( "via/Diameter" ), wxS( "via/Hole" ) } );

    BOOST_CHECK( result );
    BOOST_CHECK_EQUAL( target.GetWidth( PADSTACK::ALL_LAYERS ), 400000 );
    BOOST_CHECK_EQUAL( target.GetDrillValue(), 200000 );
}


// The catalog comes from the copyable flag.  A family with nothing flagged is a missed
// registration, not a family with nothing to copy.
BOOST_AUTO_TEST_CASE( EveryFamilyAdvertisesSomething )
{
    std::set<wxString> advertised;

    for( const wxString& key : MATCH_PROPERTIES_CATALOG::AllSafeKeys() )
    {
        // A family whose every property is shared has no key of its own left.
        for( const wxString& family : MATCH_PROPERTIES_CATALOG::FamiliesFor( key ) )
            advertised.insert( family );
    }

    for( const wxString& family : { wxS( "route" ), wxS( "via" ), wxS( "pad" ), wxS( "shape" ), wxS( "text" ),
                                    wxS( "textbox" ), wxS( "dimension" ), wxS( "zone" ), wxS( "rule_area" ),
                                    wxS( "footprint" ), wxS( "table" ), wxS( "table_cell" ), wxS( "target" ),
                                    wxS( "reference_image" ), wxS( "barcode" ) } )
    {
        BOOST_CHECK_MESSAGE( advertised.contains( family ), "Nothing copyable for family: " << family );
    }
}


// Zones and rule areas share one item type.  Only the availability funcs split their keys.
BOOST_AUTO_TEST_CASE( ZoneAndRuleAreaKeysDoNotOverlap )
{
    const std::set<wxString>& keys = MATCH_PROPERTIES_CATALOG::AllSafeKeys();

    BOOST_CHECK( keys.contains( wxS( "zone/Fill Mode" ) ) );
    BOOST_CHECK( !keys.contains( wxS( "rule_area/Fill Mode" ) ) );
    BOOST_CHECK( keys.contains( wxS( "rule_area/Keep Out Tracks" ) ) );
    BOOST_CHECK( !keys.contains( wxS( "zone/Keep Out Tracks" ) ) );
}


// Nothing that moves, renames or rewires an item may be copyable.
BOOST_AUTO_TEST_CASE( CopyableFlagExcludesIdentityGeometryAndConnectivity )
{
    PROPERTY_MANAGER& manager = PROPERTY_MANAGER::Instance();

    for( TYPE_ID type : { TYPE_HASH( PCB_TRACK ), TYPE_HASH( PCB_VIA ), TYPE_HASH( PAD ), TYPE_HASH( PCB_SHAPE ),
                          TYPE_HASH( PCB_TEXT ), TYPE_HASH( ZONE ), TYPE_HASH( FOOTPRINT ) } )
    {
        for( PROPERTY_BASE* property : manager.GetProperties( type ) )
        {
            if( !property->IsCopyable() )
                continue;

            const wxString name = property->Name();

            BOOST_CHECK_MESSAGE( name != wxS( "Net" ) && name != wxS( "Net Class" ) && name != wxS( "Locked" )
                                         && !name.StartsWith( wxS( "Position" ) ) && !name.EndsWith( wxS( " X" ) )
                                         && !name.EndsWith( wxS( " Y" ) ) && name != wxS( "Orientation" )
                                         && name != wxS( "Reference" ) && name != wxS( "Value" ),
                                 "Unsafe property flagged copyable: " << name );
        }
    }
}


// Both pairs look like one family.  Zones and rule areas share a type.  A cell has a table
// parent.
BOOST_AUTO_TEST_CASE( RelatedItemsAreNotCompatible )
{
    BOARD         board;
    ZONE          zone( &board );
    ZONE          ruleArea( &board );
    PCB_TABLE     table( nullptr, 0 );
    PCB_TABLECELL cell( &table );
    ruleArea.SetIsRuleArea( true );

    BOOST_CHECK_EQUAL( MATCH_PROPERTIES_CATALOG::Family( ruleArea ), wxString( wxS( "rule_area" ) ) );
    BOOST_CHECK_EQUAL( MATCH_PROPERTIES_CATALOG::Family( cell ), wxString( wxS( "table_cell" ) ) );
    BOOST_CHECK( !MATCH_PROPERTIES_CATALOG::Compatible( zone, ruleArea ) );
    BOOST_CHECK( !MATCH_PROPERTIES_CATALOG::Compatible( table, cell ) );
}


BOOST_AUTO_TEST_SUITE_END()
