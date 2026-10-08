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

#include <cmath>

#include <base_units.h>
#include <pin_map.h>
#include <schematic.h>
#include <sch_screen.h>
#include <sch_shape.h>
#include <sch_group.h>
#include <sch_pin.h>
#include <sch_rule_area.h>
#include <sch_sheet_path.h>
#include <sch_symbol.h>
#include <lib_symbol.h>
#include <sch_file_versions.h>
#include <downgrade/sch_downgrade.h>
#include <downgrade_target.h>
#include <qa_utils/downgrade_oracle_utils.h>


BOOST_AUTO_TEST_SUITE( SchDowngrade )

static const DOWNGRADE_TARGET& kicad9 = KI_TEST::RequireDowngradeTarget( wxT( "9.0" ) );
static const DOWNGRADE_TARGET& kicad10 = KI_TEST::RequireDowngradeTarget( wxT( "10.0" ) );


// A library symbol is downgraded through its own path. Jumper pin groups are dropped for 9.0.
BOOST_AUTO_TEST_CASE( LibSymbolJumperPinsDroppedForKicad9 )
{
    LIB_SYMBOL sym( wxT( "test" ) );
    sym.SetDuplicatePinNumbersAreJumpers( true );

    DowngradeLibSymbolInPlace( &sym, kicad9 );

    BOOST_CHECK( !sym.GetDuplicatePinNumbersAreJumpers() );
}


// The schematic ellipse primitive only exists since 20260508, so 10.0 gets an approximation.
BOOST_AUTO_TEST_CASE( EllipseIsLoweredForKicad10 )
{
    SCH_SCREEN screen;
    SCH_SHAPE* ellipse = new SCH_SHAPE( SHAPE_T::ELLIPSE );
    ellipse->SetStart( VECTOR2I( 0, 0 ) );
    ellipse->SetEnd( VECTOR2I( 500000, 0 ) );
    screen.Append( ellipse );

    COMPATIBILITY_REPORT    report = ClassifyScreenForDowngrade( &screen, kicad10 );

    BOOST_CHECK( report.IsLossy() );
    BOOST_CHECK_EQUAL( report.Count( DOWNGRADE_BUCKET::LOWER ), 1 );
}


BOOST_AUTO_TEST_CASE( EllipseTransformedToPolygon )
{
    SCH_SCREEN screen;
    SCH_SHAPE* ellipse = new SCH_SHAPE( SHAPE_T::ELLIPSE );
    ellipse->SetStart( VECTOR2I( 0, 0 ) );
    ellipse->SetEnd( VECTOR2I( 500000, 0 ) );
    screen.Append( ellipse );

    DowngradeScreenInPlace( &screen, kicad10 );

    BOOST_CHECK( ellipse->GetShape() == SHAPE_T::POLY );
}


// An unfilled ellipse must be lowered to its outline polygon, not to stroke debris.
BOOST_AUTO_TEST_CASE( UnfilledEllipseKeepsOutline )
{
    SCH_SCREEN screen;
    SCH_SHAPE* ellipse = new SCH_SHAPE( SHAPE_T::ELLIPSE );
    ellipse->SetEllipseCenter( VECTOR2I( 0, 0 ) );
    ellipse->SetEllipseMajorRadius( schIUScale.mmToIU( 5 ) );
    ellipse->SetEllipseMinorRadius( schIUScale.mmToIU( 3 ) );
    ellipse->SetFillMode( FILL_T::NO_FILL );
    screen.Append( ellipse );

    DowngradeScreenInPlace( &screen, kicad10 );

    BOOST_REQUIRE( ellipse->GetShape() == SHAPE_T::POLY );
    BOOST_REQUIRE_EQUAL( ellipse->GetPolyShape().OutlineCount(), 1 );
    BOOST_CHECK( ellipse->GetFillMode() == FILL_T::NO_FILL );

    double areaMM =
            std::abs( ellipse->GetPolyShape().Outline( 0 ).Area() ) / ( schIUScale.IU_PER_MM * schIUScale.IU_PER_MM );

    BOOST_CHECK_CLOSE( areaMM, M_PI * 5.0 * 3.0, 2.0 );
}


// An elliptical arc becomes an open polyline running from arc start to arc end.
BOOST_AUTO_TEST_CASE( EllipseArcLoweredToOpenPolyline )
{
    SCH_SCREEN screen;
    SCH_SHAPE* arc = new SCH_SHAPE( SHAPE_T::ELLIPSE_ARC );
    arc->SetEllipseCenter( VECTOR2I( 0, 0 ) );
    arc->SetEllipseMajorRadius( schIUScale.mmToIU( 5 ) );
    arc->SetEllipseMinorRadius( schIUScale.mmToIU( 3 ) );
    arc->SetEllipseStartAngle( ANGLE_0 );
    arc->SetEllipseEndAngle( ANGLE_90 );
    screen.Append( arc );

    DowngradeScreenInPlace( &screen, kicad10 );

    BOOST_REQUIRE( arc->GetShape() == SHAPE_T::POLY );
    BOOST_REQUIRE_EQUAL( arc->GetPolyShape().OutlineCount(), 1 );

    const SHAPE_LINE_CHAIN& chain = arc->GetPolyShape().COutline( 0 );
    BOOST_REQUIRE_GE( chain.PointCount(), 5 );

    // One end sits on the major axis, the other on the minor axis.
    double firstDist = chain.CPoint( 0 ).EuclideanNorm();
    double lastDist = chain.CPoint( chain.PointCount() - 1 ).EuclideanNorm();
    double tolerance = schIUScale.mmToIU( 0.05 );

    BOOST_CHECK( std::abs( firstDist - schIUScale.mmToIU( 5 ) ) < tolerance );
    BOOST_CHECK( std::abs( lastDist - schIUScale.mmToIU( 3 ) ) < tolerance );
    BOOST_CHECK( chain.CPoint( 0 ) != chain.CPoint( chain.PointCount() - 1 ) );
}


// Ungrouping must survive nested groups. Members stay on the screen, groups go.
BOOST_AUTO_TEST_CASE( NestedGroupsUngroupSafely )
{
    SCH_SCREEN screen;

    SCH_SHAPE* innerShape = new SCH_SHAPE( SHAPE_T::RECTANGLE );
    SCH_SHAPE* outerShape = new SCH_SHAPE( SHAPE_T::RECTANGLE );
    screen.Append( innerShape );
    screen.Append( outerShape );

    SCH_GROUP* inner = new SCH_GROUP( &screen );
    inner->AddItem( innerShape );
    screen.Append( inner );

    SCH_GROUP* outer = new SCH_GROUP( &screen );
    outer->AddItem( outerShape );
    outer->AddItem( inner );
    screen.Append( outer );

    BOOST_CHECK_EQUAL( ClassifyScreenForDowngrade( &screen, kicad9 ).Count( DOWNGRADE_BUCKET::LOWER ), 2 );

    DowngradeScreenInPlace( &screen, kicad9 );

    auto groups = screen.Items().OfType( SCH_GROUP_T );
    BOOST_CHECK( groups.begin() == groups.end() );

    auto shapes = screen.Items().OfType( SCH_SHAPE_T );
    BOOST_CHECK_EQUAL( std::distance( shapes.begin(), shapes.end() ), 2 );

    BOOST_CHECK( innerShape->GetParentGroup() == nullptr );
    BOOST_CHECK( outerShape->GetParentGroup() == nullptr );
}


// Hatched fills postdate 9.0, so they become a solid fill for 9.0.
BOOST_AUTO_TEST_CASE( HatchedFillLoweredForKicad9 )
{
    SCH_SCREEN screen;
    SCH_SHAPE* shape = new SCH_SHAPE( SHAPE_T::RECTANGLE );
    shape->SetFillMode( FILL_T::HATCH );
    screen.Append( shape );

    BOOST_CHECK_EQUAL( ClassifyScreenForDowngrade( &screen, kicad9 ).Count( DOWNGRADE_BUCKET::LOWER ), 1 );

    DowngradeScreenInPlace( &screen, kicad9 );

    BOOST_CHECK( shape->GetFillMode() == FILL_T::FILLED_SHAPE );
}


// Symbol bodies are written from the screen library cache, so an ellipse there must be
// counted and lowered too, not just shapes placed on the screen.
BOOST_AUTO_TEST_CASE( LibSymbolEllipseLoweredForKicad10 )
{
    SCH_SCREEN  screen;
    LIB_SYMBOL* symbol = new LIB_SYMBOL( wxT( "test" ) );
    SCH_SHAPE*  ellipse = new SCH_SHAPE( SHAPE_T::ELLIPSE );
    ellipse->SetStart( VECTOR2I( 0, 0 ) );
    ellipse->SetEnd( VECTOR2I( 500000, 0 ) );
    symbol->AddDrawItem( ellipse );
    screen.AddLibSymbol( symbol );

    BOOST_CHECK_EQUAL( ClassifyScreenForDowngrade( &screen, kicad10 ).Count( DOWNGRADE_BUCKET::LOWER ), 1 );

    DowngradeScreenInPlace( &screen, kicad10 );

    BOOST_CHECK( ellipse->GetShape() == SHAPE_T::POLY );
}


// Turning a local power symbol global merges nets across sheets. A netlist change must never
// ship silently, so the export blocks for 9.0.
BOOST_AUTO_TEST_CASE( LocalPowerSymbolBlocksKicad9 )
{
    SCH_SCREEN  screen;
    LIB_SYMBOL* symbol = new LIB_SYMBOL( wxT( "test" ) );
    symbol->SetLocalPower();
    screen.AddLibSymbol( symbol );

    BOOST_CHECK_EQUAL( ClassifyScreenForDowngrade( &screen, kicad10 ).Count( DOWNGRADE_BUCKET::BLOCK ), 0 );
    BOOST_CHECK_EQUAL( ClassifyScreenForDowngrade( &screen, kicad9 ).Count( DOWNGRADE_BUCKET::BLOCK ), 1 );

    DowngradeScreenInPlace( &screen, kicad9 );

    BOOST_CHECK( symbol->IsLocalPower() );
}


// Rule area flags (20250610) postdate 9.0, so they are dropped and reported for 9.0.
BOOST_AUTO_TEST_CASE( RuleAreaFlagsDroppedForKicad9 )
{
    SCH_SCREEN     screen;
    SCH_RULE_AREA* area = new SCH_RULE_AREA();
    area->SetExcludedFromSim( true );
    area->SetDNP( true );
    screen.Append( area );

    BOOST_CHECK_EQUAL( ClassifyScreenForDowngrade( &screen, kicad9 ).Count( DOWNGRADE_BUCKET::DROP ), 1 );

    DowngradeScreenInPlace( &screen, kicad9 );

    BOOST_CHECK( !area->GetExcludedFromSim() );
    BOOST_CHECK( !area->GetDNP() );
}


// Rounded rectangles (20250829) postdate 9.0, so 9.0 gets a polygon that keeps the rounding.
BOOST_AUTO_TEST_CASE( RoundedRectangleLoweredForKicad9 )
{
    SCH_SCREEN screen;
    SCH_SHAPE* rect = new SCH_SHAPE( SHAPE_T::RECTANGLE );
    rect->SetStart( VECTOR2I( 0, 0 ) );
    rect->SetEnd( VECTOR2I( schIUScale.mmToIU( 10 ), schIUScale.mmToIU( 6 ) ) );
    rect->SetCornerRadius( schIUScale.mmToIU( 1 ) );
    rect->SetFillMode( FILL_T::NO_FILL );
    screen.Append( rect );

    BOOST_CHECK_EQUAL( ClassifyScreenForDowngrade( &screen, kicad10 ).Count( DOWNGRADE_BUCKET::LOWER ), 0 );
    BOOST_CHECK_EQUAL( ClassifyScreenForDowngrade( &screen, kicad9 ).Count( DOWNGRADE_BUCKET::LOWER ), 1 );

    DowngradeScreenInPlace( &screen, kicad9 );

    BOOST_REQUIRE( rect->GetShape() == SHAPE_T::POLY );
    BOOST_REQUIRE_EQUAL( rect->GetPolyShape().OutlineCount(), 1 );

    // A 10 mm by 6 mm rectangle loses ( 4 - pi ) r^2 to the rounded corners.
    double areaMM =
            std::abs( rect->GetPolyShape().Outline( 0 ).Area() ) / ( schIUScale.IU_PER_MM * schIUScale.IU_PER_MM );

    BOOST_CHECK_CLOSE( areaMM, 60.0 - ( 4.0 - M_PI ), 2.0 );
}


// A stacked pin number with escaped characters (20260622) is misread by both targets, so the
// export must block instead of silently renaming pins.
BOOST_AUTO_TEST_CASE( EscapedStackedPinBlocksExport )
{
    SCH_SCREEN  screen;
    LIB_SYMBOL* symbol = new LIB_SYMBOL( wxT( "test" ) );
    SCH_PIN*    pin = new SCH_PIN( symbol );
    pin->SetNumber( wxT( "[A\\,1,B]" ) );
    symbol->AddDrawItem( pin );
    screen.AddLibSymbol( symbol );

    BOOST_CHECK_EQUAL( ClassifyScreenForDowngrade( &screen, kicad10 ).Count( DOWNGRADE_BUCKET::BLOCK ), 1 );
    BOOST_CHECK_EQUAL( ClassifyScreenForDowngrade( &screen, kicad9 ).Count( DOWNGRADE_BUCKET::BLOCK ), 1 );
}


// Plain stacked notation (20250901) is expanded by 10.0, but 9.0 reads the brackets as one
// literal pin number and the netlist silently changes. So 9.0 blocks and 10.0 does not.
BOOST_AUTO_TEST_CASE( PlainStackedPinBlocksKicad9Only )
{
    SCH_SCREEN  screen;
    LIB_SYMBOL* symbol = new LIB_SYMBOL( wxT( "test" ) );
    SCH_PIN*    pin = new SCH_PIN( symbol );
    pin->SetNumber( wxT( "[A1,B2]" ) );
    symbol->AddDrawItem( pin );
    screen.AddLibSymbol( symbol );

    BOOST_CHECK_EQUAL( ClassifyScreenForDowngrade( &screen, kicad10 ).Count( DOWNGRADE_BUCKET::BLOCK ), 0 );
    BOOST_CHECK_EQUAL( ClassifyScreenForDowngrade( &screen, kicad9 ).Count( DOWNGRADE_BUCKET::BLOCK ), 1 );
}


// Invalid stacked notation is read as one literal pin number by every KiCad, so it must not
// block. The escapes rule only fires when the notation actually parses.
BOOST_AUTO_TEST_CASE( InvalidStackedNotationDoesNotBlock )
{
    SCH_SCREEN  screen;
    LIB_SYMBOL* symbol = new LIB_SYMBOL( wxT( "test" ) );
    SCH_PIN*    pin = new SCH_PIN( symbol );
    pin->SetNumber( wxT( "[A\\,B" ) );
    symbol->AddDrawItem( pin );
    screen.AddLibSymbol( symbol );

    BOOST_CHECK_EQUAL( ClassifyScreenForDowngrade( &screen, kicad10 ).Count( DOWNGRADE_BUCKET::BLOCK ), 0 );
    BOOST_CHECK_EQUAL( ClassifyScreenForDowngrade( &screen, kicad9 ).Count( DOWNGRADE_BUCKET::BLOCK ), 0 );
}


// A pin map living only in the screen's library cache is dropped by the transform, so it must
// be counted for consent too.
BOOST_AUTO_TEST_CASE( LibCachePinMapsCountedForDrop )
{
    SCH_SCREEN  screen;
    LIB_SYMBOL* symbol = new LIB_SYMBOL( wxT( "test" ) );
    symbol->SetAssociatedFootprints( { { LIB_ID( wxT( "Lib" ), wxT( "FP" ) ), wxT( "map" ) } } );
    screen.AddLibSymbol( symbol );

    BOOST_CHECK_EQUAL( ClassifyScreenForDowngrade( &screen, kicad10 ).Count( DOWNGRADE_BUCKET::DROP ), 1 );

    DowngradeScreenInPlace( &screen, kicad10 );

    BOOST_CHECK( screen.GetLibSymbols().begin()->second->GetAssociatedFootprints().empty() );
}


// The 10.0 group format stores its own lock, so a locked group survives a 10.0 export while
// other locked items are still unlocked and reported.
BOOST_AUTO_TEST_CASE( GroupLockKeptForKicad10 )
{
    SCH_SCREEN screen;

    // A locked item outside the group, so its lock cannot be inherited from the group.
    SCH_SHAPE* shape = new SCH_SHAPE( SHAPE_T::RECTANGLE );
    shape->SetLocked( true );
    screen.Append( shape );

    SCH_SHAPE* member = new SCH_SHAPE( SHAPE_T::RECTANGLE );
    screen.Append( member );

    SCH_GROUP* group = new SCH_GROUP( &screen );
    group->AddItem( member );
    group->SetLocked( true );
    screen.Append( group );

    BOOST_CHECK_EQUAL( ClassifyScreenForDowngrade( &screen, kicad10 ).Count( DOWNGRADE_BUCKET::LOWER ), 1 );

    DowngradeScreenInPlace( &screen, kicad10 );

    BOOST_CHECK( !shape->IsLocked() );
    BOOST_CHECK( group->IsLocked() );
}


// The 9.0 target opens a single root sheet, so a second top-level sheet blocks that export.
// 10.0 has the flat hierarchy and stays clean.
BOOST_AUTO_TEST_CASE( MultipleTopLevelSheetsBlockKicad9 )
{
    SCHEMATIC schematic( nullptr );
    schematic.CreateDefaultScreens();

    SCH_SHEET* second = new SCH_SHEET( &schematic );
    second->SetScreen( new SCH_SCREEN( &schematic ) );
    schematic.AddTopLevelSheet( second );

    BOOST_CHECK_EQUAL( ClassifySchematicStructureForDowngrade( schematic, kicad10 ).Count( DOWNGRADE_BUCKET::BLOCK ),
                       0 );
    BOOST_CHECK_EQUAL( ClassifySchematicStructureForDowngrade( schematic, kicad9 ).Count( DOWNGRADE_BUCKET::BLOCK ),
                       1 );
}


// A variant selecting a different library symbol has no representation in a target that keeps
// variants but not the choice, so that export must refuse. A target that removes variants
// wholesale loses the choice along with them and needs only the drop row.
BOOST_AUTO_TEST_CASE( AlternateVariantSymbolBlocksTargetThatKeepsVariants )
{
    SCHEMATIC schematic( nullptr );
    schematic.CreateDefaultScreens();

    SCH_SHEET_LIST paths = schematic.BuildUnorderedSheetList();
    BOOST_REQUIRE( !paths.empty() );

    SCH_SHEET_PATH path = paths.at( 0 );
    SCH_SCREEN*    screen = path.LastScreen();
    BOOST_REQUIRE( screen != nullptr );

    SCH_SYMBOL* symbol = new SCH_SYMBOL();
    screen->Append( symbol );

    SCH_SYMBOL_INSTANCE instance;
    instance.m_Path = path.Path();

    SCH_SYMBOL_VARIANT variant( wxT( "Alt" ) );
    variant.m_SymbolOverride = LIB_ID( wxT( "Device" ), wxT( "Regulator_5pin" ) );
    instance.m_Variants[wxT( "Alt" )] = variant;

    symbol->AddHierarchicalReference( instance );

    BOOST_CHECK_EQUAL( ClassifySchematicStructureForDowngrade( schematic, kicad10 ).Count( DOWNGRADE_BUCKET::BLOCK ),
                       1 );
    BOOST_CHECK_EQUAL( ClassifySchematicStructureForDowngrade( schematic, kicad9 ).Count( DOWNGRADE_BUCKET::BLOCK ),
                       0 );
}


// A pin-to-pad override stored on a variant record must be counted and cleared like the base one.
BOOST_AUTO_TEST_CASE( VariantPinMapOverrideDroppedForKicad10 )
{
    SCH_SCREEN  screen;
    SCH_SYMBOL* symbol = new SCH_SYMBOL();

    SCH_SYMBOL_INSTANCE instance;
    instance.m_Path.push_back( KIID() );

    SCH_SYMBOL_VARIANT variant( wxT( "V1" ) );
    variant.m_PinMapOverride.m_Mode = PIN_MAP_OVERRIDE_MODE::FORCE_IDENTITY;
    instance.m_Variants[wxT( "V1" )] = variant;

    symbol->AddHierarchicalReference( instance );
    screen.Append( symbol );

    BOOST_CHECK_EQUAL( ClassifyScreenForDowngrade( &screen, kicad10 ).Count( DOWNGRADE_BUCKET::DROP ), 1 );

    DowngradeScreenInPlace( &screen, kicad10 );

    BOOST_REQUIRE_EQUAL( symbol->GetInstances().size(), 1 );
    const auto& variants = symbol->GetInstances()[0].m_Variants;
    BOOST_REQUIRE( variants.count( wxT( "V1" ) ) );
    BOOST_CHECK( variants.at( wxT( "V1" ) ).m_PinMapOverride.IsDefault() );
}


// Named body styles postdate 9.0, so their names are dropped and reported for 9.0.
BOOST_AUTO_TEST_CASE( NamedBodyStylesDroppedForKicad9 )
{
    SCH_SCREEN  screen;
    LIB_SYMBOL* symbol = new LIB_SYMBOL( wxT( "test" ) );
    symbol->SetBodyStyleNames( { wxT( "styleA" ), wxT( "styleB" ) } );
    screen.AddLibSymbol( symbol );

    BOOST_CHECK_EQUAL( ClassifyScreenForDowngrade( &screen, kicad9 ).Count( DOWNGRADE_BUCKET::DROP ), 1 );

    DowngradeScreenInPlace( &screen, kicad9 );

    BOOST_CHECK( symbol->GetBodyStyleNames().empty() );
}


// KiCad 9 only supports selectors 1 and 2. A used third style must retain its drawing
// and pin positions, not silently fall back to style 1 (even with --force).
BOOST_AUTO_TEST_CASE( SelectedCustomBodyStyleBakedForKicad9 )
{
    SCH_SCREEN screen;
    LIB_SYMBOL lib( wxT( "test" ) );
    lib.SetBodyStyleNames( { wxT( "first" ), wxT( "second" ), wxT( "third" ) } );

    for( int style = 1; style <= 3; ++style )
    {
        SCH_PIN* pin = new SCH_PIN( &lib );
        pin->SetNumber( wxT( "1" ) );
        pin->SetPosition( VECTOR2I( schIUScale.mmToIU( 5 * style ), 0 ) );
        pin->SetBodyStyle( style );
        pin->SetUnit( 1 );
        lib.AddDrawItem( pin );
    }

    SCH_SYMBOL* base = new SCH_SYMBOL( lib, lib.GetLibId(), nullptr, 1, 1 );
    SCH_SYMBOL* third = new SCH_SYMBOL( lib, lib.GetLibId(), nullptr, 1, 3 );
    SCH_SYMBOL* another = new SCH_SYMBOL( lib, lib.GetLibId(), nullptr, 1, 3 );
    screen.Append( base );
    screen.Append( third );
    screen.Append( another );

    BOOST_REQUIRE_EQUAL( third->GetRawPins().size(), 1 );
    const KIID pinUuid = third->GetRawPins().front()->m_Uuid;
    BOOST_CHECK_EQUAL( ClassifyScreenForDowngrade( &screen, kicad9 ).Count( DOWNGRADE_BUCKET::LOWER ), 2 );

    DowngradeScreenInPlace( &screen, kicad9 );

    BOOST_CHECK_EQUAL( third->GetBodyStyle(), BODY_STYLE::BASE );
    BOOST_CHECK_EQUAL( base->GetBodyStyle(), BODY_STYLE::BASE );
    BOOST_CHECK( third->GetSchSymbolLibraryName() != base->GetSchSymbolLibraryName() );
    BOOST_CHECK( third->GetSchSymbolLibraryName() == another->GetSchSymbolLibraryName() );
    BOOST_REQUIRE_EQUAL( third->GetRawPins().size(), 1 );
    BOOST_CHECK( third->GetRawPins().front()->m_Uuid == pinUuid );
    BOOST_CHECK( third->GetRawPins().front()->GetPosition() == VECTOR2I( schIUScale.mmToIU( 15 ), 0 ) );
    BOOST_CHECK( base->GetRawPins().front()->GetPosition() == VECTOR2I( schIUScale.mmToIU( 5 ), 0 ) );
    BOOST_CHECK_EQUAL( third->GetLibSymbolRef()->GetBodyStyleCount(), 1 );
    BOOST_CHECK_EQUAL( third->GetLibSymbolRef()->GetGraphicalPins().size(), 1 );
    BOOST_CHECK_EQUAL( screen.GetLibSymbols().size(), 2 );
}


BOOST_AUTO_TEST_CASE( SelectedCustomBodyStyleKeptForKicad10 )
{
    SCH_SCREEN screen;
    LIB_SYMBOL lib( wxT( "test" ) );
    lib.SetBodyStyleNames( { wxT( "first" ), wxT( "second" ), wxT( "third" ) } );
    SCH_SYMBOL* symbol = new SCH_SYMBOL( lib, lib.GetLibId(), nullptr, 1, 3 );
    screen.Append( symbol );

    BOOST_CHECK( !ClassifyScreenForDowngrade( &screen, kicad10 ).IsLossy() );
    DowngradeScreenInPlace( &screen, kicad10 );
    BOOST_CHECK_EQUAL( symbol->GetBodyStyle(), 3 );
    BOOST_CHECK_EQUAL( symbol->GetLibSymbolRef()->GetBodyStyleNames().size(), 3 );
}


BOOST_AUTO_TEST_CASE( PositionFileExclusionsReportedAndDroppedForKicad9 )
{
    SCH_SCREEN screen;
    LIB_SYMBOL lib( wxT( "test" ) );
    lib.SetExcludedFromPosFiles( true );
    SCH_SYMBOL* symbol = new SCH_SYMBOL( lib, lib.GetLibId(), nullptr, 1, 1 );
    screen.Append( symbol );

    BOOST_CHECK_EQUAL( ClassifyScreenForDowngrade( &screen, kicad9 ).Count( DOWNGRADE_BUCKET::DROP ), 2 );
    BOOST_CHECK_EQUAL( ClassifyLibSymbolForDowngrade( &lib, kicad9 ).Count( DOWNGRADE_BUCKET::DROP ), 1 );
    DowngradeScreenInPlace( &screen, kicad9 );
    DowngradeLibSymbolInPlace( &lib, kicad9 );
    BOOST_CHECK( !symbol->GetExcludedFromPosFiles() );
    BOOST_CHECK( !lib.GetExcludedFromPosFiles() );
    BOOST_CHECK( !screen.GetLibSymbols().begin()->second->GetExcludedFromPosFiles() );
}


BOOST_AUTO_TEST_CASE( PositionFileExclusionsKeptForKicad10 )
{
    SCH_SCREEN screen;
    LIB_SYMBOL lib( wxT( "test" ) );
    lib.SetExcludedFromPosFiles( true );
    SCH_SYMBOL* symbol = new SCH_SYMBOL( lib, lib.GetLibId(), nullptr, 1, 1 );
    screen.Append( symbol );

    BOOST_CHECK( !ClassifyScreenForDowngrade( &screen, kicad10 ).IsLossy() );
    BOOST_CHECK( !ClassifyLibSymbolForDowngrade( &lib, kicad10 ).IsLossy() );
    DowngradeScreenInPlace( &screen, kicad10 );
    DowngradeLibSymbolInPlace( &lib, kicad10 );
    BOOST_CHECK( symbol->GetExcludedFromPosFiles() );
    BOOST_CHECK( lib.GetExcludedFromPosFiles() );
}


// Item locking was introduced in format 20260326.
BOOST_AUTO_TEST_CASE( LockedItemUnlockedForKicad10 )
{
    SCH_SCREEN screen;
    SCH_SHAPE* shape = new SCH_SHAPE( SHAPE_T::RECTANGLE );
    shape->SetLocked( true );
    screen.Append( shape );

    BOOST_CHECK_EQUAL( ClassifyScreenForDowngrade( &screen, kicad10 ).Count( DOWNGRADE_BUCKET::LOWER ), 1 );

    DowngradeScreenInPlace( &screen, kicad10 );

    BOOST_CHECK( !shape->IsLocked() );
}


// The schematic fail-closed gate catches post-10.0 tokens and ignores lookalikes.
BOOST_AUTO_TEST_CASE( VerifyGateCatchesUnsupportedTokens )
{
    BOOST_CHECK( FindUnsupportedSchToken( wxT( "(symbol (pin_map_override (mode passthrough)))" ), kicad10 )
                 == wxT( "pin_map_override" ) );
    BOOST_CHECK( FindUnsupportedSchToken( wxT( "(net_chain \"n\")" ), kicad10 ) == wxT( "net_chain" ) );
    BOOST_CHECK( FindUnsupportedSchToken( wxT( "(kicad_sch (symbol (pin \"1\")))" ), kicad10 ).IsEmpty() );

    // Value tokens the node scan cannot see must still be caught for 9.0, but not inside strings.

    BOOST_CHECK( FindUnsupportedSchToken( wxT( "(fill (type hatch))" ), kicad9 ) == wxT( "hatch" ) );
    BOOST_CHECK( FindUnsupportedSchToken( wxT( "(text \"hatch pattern\")" ), kicad9 ).IsEmpty() );
}


// Coverage guard: a schematic format bump must trigger a denylist review. See the board equivalent.
BOOST_AUTO_TEST_CASE( DenylistCoversCurrentFormat )
{
    BOOST_CHECK_EQUAL( SEXPR_SCHEMATIC_FILE_VERSION, SchDowngradeCoveredVersion() );
    BOOST_CHECK_EQUAL( SEXPR_SYMBOL_LIB_FILE_VERSION, SymbolLibDowngradeCoveredVersion() );
}


BOOST_AUTO_TEST_CASE( DropApproximationsOmitsScreenAndCachedGraphics )
{
    SCH_SCREEN   screen;
    LIB_SYMBOL*  lib = new LIB_SYMBOL( wxT( "test" ) );
    const size_t fieldCount = lib->GetDrawItems().size();
    for( SHAPE_T type : { SHAPE_T::ELLIPSE, SHAPE_T::ELLIPSE_ARC, SHAPE_T::RECTANGLE } )
    {
        SCH_SHAPE* placed = new SCH_SHAPE( type );
        SCH_SHAPE* cached = new SCH_SHAPE( type );
        if( type == SHAPE_T::RECTANGLE )
        {
            placed->SetEnd( VECTOR2I( schIUScale.mmToIU( 10 ), schIUScale.mmToIU( 6 ) ) );
            cached->SetEnd( VECTOR2I( schIUScale.mmToIU( 10 ), schIUScale.mmToIU( 6 ) ) );
            placed->SetCornerRadius( schIUScale.mmToIU( 1 ) );
            cached->SetCornerRadius( schIUScale.mmToIU( 1 ) );
        }
        screen.Append( placed );
        lib->AddDrawItem( cached );
    }
    SCH_SHAPE* supported = new SCH_SHAPE( SHAPE_T::CIRCLE );
    screen.Append( supported );
    SCH_SHAPE* cachedSupported = new SCH_SHAPE( SHAPE_T::CIRCLE );
    lib->AddDrawItem( cachedSupported );
    screen.AddLibSymbol( lib );
    SCH_GROUP* group = new SCH_GROUP( &screen );
    for( SCH_ITEM* item : screen.Items().OfType( SCH_SHAPE_T ) )
        group->AddItem( item );
    screen.Append( group );
    const auto& target = kicad9;
    const auto  normal = ClassifyScreenForDowngrade( &screen, target );
    const auto  omit = ClassifyScreenForDowngrade( &screen, target, true );
    BOOST_CHECK_EQUAL( normal.Count( DOWNGRADE_BUCKET::LOWER ), 7 );
    BOOST_CHECK_EQUAL( omit.Count( DOWNGRADE_BUCKET::LOWER ), 0 );
    BOOST_CHECK_EQUAL( omit.Count( DOWNGRADE_BUCKET::DROP ), 7 );
    BOOST_REQUIRE_EQUAL( normal.Entries().size(), omit.Entries().size() );
    for( size_t i = 0; i < normal.Entries().size(); ++i )
    {
        BOOST_CHECK( normal.Entries()[i].m_feature == omit.Entries()[i].m_feature );
        BOOST_CHECK_EQUAL( normal.Entries()[i].m_count, omit.Entries()[i].m_count );
        BOOST_CHECK( normal.Entries()[i].m_detail != omit.Entries()[i].m_detail );
        BOOST_CHECK( !omit.Entries()[i].m_detail.IsEmpty() );
    }
    DowngradeScreenInPlace( &screen, target, true );
    auto shapes = screen.Items().OfType( SCH_SHAPE_T );
    BOOST_REQUIRE_EQUAL( std::distance( shapes.begin(), shapes.end() ), 1 );
    BOOST_CHECK( *shapes.begin() == supported );
    BOOST_CHECK( supported->GetParentGroup() == nullptr );
    BOOST_CHECK_EQUAL( lib->GetDrawItems().size(), fieldCount + 1 );
    for( const SCH_ITEM& item : lib->GetDrawItems() )
    {
        if( item.Type() == SCH_SHAPE_T )
            BOOST_CHECK( &item == cachedSupported );
    }
    BOOST_CHECK( !ClassifyScreenForDowngrade( &screen, target, true ).IsLossy() );
}


BOOST_AUTO_TEST_CASE( DropApproximationsOmitsLibraryGraphicsAndHatchFill )
{
    LIB_SYMBOL   lib( wxT( "test" ) );
    const size_t fieldCount = lib.GetDrawItems().size();
    lib.AddDrawItem( new SCH_SHAPE( SHAPE_T::ELLIPSE ) );
    lib.AddDrawItem( new SCH_SHAPE( SHAPE_T::ELLIPSE_ARC ) );
    SCH_SHAPE* rounded = new SCH_SHAPE( SHAPE_T::RECTANGLE );
    rounded->SetEnd( VECTOR2I( schIUScale.mmToIU( 10 ), schIUScale.mmToIU( 6 ) ) );
    rounded->SetCornerRadius( schIUScale.mmToIU( 1 ) );
    lib.AddDrawItem( rounded );
    SCH_SHAPE* hatch = new SCH_SHAPE( SHAPE_T::RECTANGLE );
    hatch->SetFillMode( FILL_T::HATCH );
    lib.AddDrawItem( hatch );
    const auto& target = kicad9;
    BOOST_CHECK_EQUAL( ClassifyLibSymbolForDowngrade( &lib, target, true ).Count( DOWNGRADE_BUCKET::DROP ), 4 );
    BOOST_CHECK_EQUAL( ClassifyLibSymbolForDowngrade( &lib, target, true ).Count( DOWNGRADE_BUCKET::LOWER ), 0 );
    DowngradeLibSymbolInPlace( &lib, target, true );
    BOOST_REQUIRE_EQUAL( lib.GetDrawItems().size(), fieldCount + 1 );
    for( const SCH_ITEM& item : lib.GetDrawItems() )
    {
        if( item.Type() == SCH_SHAPE_T )
            BOOST_CHECK( &item == hatch );
    }
    BOOST_CHECK( hatch->GetFillMode() == FILL_T::NO_FILL );
}


BOOST_AUTO_TEST_CASE( DropApproximationsKeepsOutlineTextAndLockFreeItems )
{
    SCH_SCREEN screen;
    SCH_SHAPE* hatch = new SCH_SHAPE( SHAPE_T::RECTANGLE );
    hatch->SetFillMode( FILL_T::HATCH );
    screen.Append( hatch );
    hatch->SetLocked( true );
    SCH_RULE_AREA* area = new SCH_RULE_AREA();
    area->SetFillMode( FILL_T::HATCH );
    screen.Append( area );
    const auto& target = kicad9;
    BOOST_CHECK_EQUAL( ClassifyScreenForDowngrade( &screen, target, true ).Count( DOWNGRADE_BUCKET::DROP ), 3 );
    DowngradeScreenInPlace( &screen, target, true );
    BOOST_CHECK( !hatch->IsLocked() );
    BOOST_CHECK( hatch->GetFillMode() == FILL_T::NO_FILL );
    BOOST_CHECK( area->GetFillMode() == FILL_T::NO_FILL );
    BOOST_CHECK( screen.CheckIfOnDrawList( hatch ) );
    BOOST_CHECK( screen.CheckIfOnDrawList( area ) );
}


BOOST_AUTO_TEST_CASE( DropApproximationsOmitsSelectedCustomStyleInstancesOnly )
{
    for( const auto& target : { kicad9, kicad10 } )
    {
        SCH_SCREEN screen;
        LIB_SYMBOL lib( wxT( "test" ) );
        lib.SetBodyStyleNames( { wxT( "first" ), wxT( "second" ), wxT( "third" ) } );
        for( int style = 1; style <= 3; ++style )
        {
            SCH_PIN* pin = new SCH_PIN( &lib );
            pin->SetNumber( wxT( "1" ) );
            pin->SetBodyStyle( style );
            pin->SetUnit( 1 );
            lib.AddDrawItem( pin );
        }
        SCH_SYMBOL* base = new SCH_SYMBOL( lib, lib.GetLibId(), nullptr, 1, 1 );
        SCH_SYMBOL* second = new SCH_SYMBOL( lib, lib.GetLibId(), nullptr, 1, 2 );
        SCH_SYMBOL* third = new SCH_SYMBOL( lib, lib.GetLibId(), nullptr, 1, 3 );
        screen.Append( base );
        screen.Append( second );
        screen.Append( third );
        SCH_GROUP* group = new SCH_GROUP( &screen );
        group->AddItem( third );
        screen.Append( group );
        const KIID basePin = base->GetRawPins().front()->m_Uuid;
        const KIID secondPin = second->GetRawPins().front()->m_Uuid;
        BOOST_CHECK_EQUAL( ClassifyScreenForDowngrade( &screen, target, true ).Count( DOWNGRADE_BUCKET::LOWER ), 0 );
        DowngradeScreenInPlace( &screen, target, true );
        auto symbols = screen.Items().OfType( SCH_SYMBOL_T );
        BOOST_CHECK_EQUAL( std::distance( symbols.begin(), symbols.end() ), target.m_id == wxT( "9.0" ) ? 2 : 3 );
        BOOST_CHECK( screen.CheckIfOnDrawList( base ) );
        BOOST_CHECK( screen.CheckIfOnDrawList( second ) );
        BOOST_CHECK( base->GetRawPins().front()->m_Uuid == basePin );
        BOOST_CHECK( second->GetRawPins().front()->m_Uuid == secondPin );
        BOOST_CHECK_EQUAL( screen.GetLibSymbols().size(), 1 );
        if( target.m_id == wxT( "10.0" ) )
        {
            BOOST_CHECK( screen.CheckIfOnDrawList( third ) );
            BOOST_CHECK_EQUAL( third->GetBodyStyle(), 3 );
        }
    }
}


BOOST_AUTO_TEST_CASE( DropApproximationsCannotBypassSchematicBlockRules )
{
    LIB_SYMBOL lib( wxT( "test" ) );
    lib.SetLocalPower();
    const auto& target = kicad9;
    BOOST_CHECK( ClassifyLibSymbolForDowngrade( &lib, target, true ).IsBlocked() );
    SCH_SCREEN screen;
    screen.AddLibSymbol( new LIB_SYMBOL( lib ) );
    BOOST_CHECK( ClassifyScreenForDowngrade( &screen, target, true ).IsBlocked() );
}


BOOST_AUTO_TEST_SUITE_END()
