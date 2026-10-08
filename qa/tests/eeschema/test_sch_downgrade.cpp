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
#include <climits>
#include <fstream>
#include <iterator>
#include <map>

#include <base_units.h>
#include <common.h>
#include <embedded_files.h>
#include <pin_map.h>
#include <schematic.h>
#include <sch_screen.h>
#include <sch_shape.h>
#include <sch_group.h>
#include <sch_line.h>
#include <sch_pin.h>
#include <sch_rule_area.h>
#include <sch_sheet_path.h>
#include <sch_symbol.h>
#include <sch_label.h>
#include <sch_text.h>
#include <bus_alias.h>
#include <project/net_settings.h>
#include <project.h>
#include <project/project_file_downgrade.h>
#include <nlohmann/json.hpp>
#include <settings/settings_manager.h>
#include <lib_symbol.h>
#include <sch_file_versions.h>
#include <downgrade/sch_downgrade.h>
#include <downgrade_target.h>
#include <qa_utils/downgrade_oracle_utils.h>
#include <qa_utils/temporary_directory.h>
#include <qa_utils/wx_utils/unit_test_utils.h>
#include <eeschema_test_utils.h>
#include <schematic_file_util.h>
#include <pgm_base.h>
#include <project_sch.h>
#include <libraries/library_manager.h>
#include <libraries/library_table.h>
#include <libraries/symbol_library_adapter.h>
#include <wx/file.h>

#include <wx/filefn.h>


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


// A sheet's variant overrides must flatten like a symbol's: exclusion flags and field
// overrides bake into the base sheet, not just DNP.
BOOST_AUTO_TEST_CASE( SheetVariantFlattenedIntoSchematic )
{
    SCHEMATIC schematic( nullptr );
    schematic.CreateDefaultScreens();

    SCH_SHEET_LIST rootPaths = schematic.BuildUnorderedSheetList();
    BOOST_REQUIRE( !rootPaths.empty() );

    SCH_SHEET_PATH rootPath = rootPaths.at( 0 );
    SCH_SCREEN*    rootScreen = rootPath.LastScreen();
    BOOST_REQUIRE( rootScreen != nullptr );

    SCH_SHEET* child = new SCH_SHEET( &schematic );
    child->SetScreen( new SCH_SCREEN( &schematic ) );
    child->SetFileName( wxT( "child.kicad_sch" ) );
    rootScreen->Append( child );

    child->AddField( SCH_FIELD( child, FIELD_T::USER, wxT( "Note" ) ) );
    child->GetFields().back().SetText( wxT( "base note" ) );

    SCH_SHEET_PATH childPath = rootPath;
    childPath.push_back( child );

    SCH_SHEET_INSTANCE instance;
    instance.m_Path = childPath.Path();

    SCH_SHEET_VARIANT variant( wxT( "V1" ) );
    variant.InitializeAttributes( *child );
    variant.m_ExcludedFromBOM = true;
    variant.m_Fields[wxT( "Note" )] = wxT( "variant note" );
    instance.m_Variants[wxT( "V1" )] = variant;

    child->AddInstance( instance );

    BOOST_CHECK( !child->GetExcludedFromBOM() );

    wxString conflict;
    BOOST_REQUIRE( FlattenSchematicVariant( schematic, wxT( "V1" ), &conflict ) );

    BOOST_CHECK( child->GetExcludedFromBOM() );
    BOOST_CHECK( child->GetFieldText( wxT( "Note" ) ) == wxT( "variant note" ) );
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


// Flattening bakes the chosen variant into the base symbol attributes, so the exported
// schematic matches that variant after the registry is dropped.
BOOST_AUTO_TEST_CASE( VariantFlattenedIntoSchematic )
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
    instance.m_Reference = wxT( "R1" );

    SCH_SYMBOL_VARIANT variant( wxT( "V1" ) );
    variant.InitializeAttributes( *symbol );
    variant.m_DNP = true;
    instance.m_Variants[wxT( "V1" )] = variant;

    symbol->AddHierarchicalReference( instance );

    BOOST_CHECK( !symbol->GetDNP() );

    wxString conflict;
    BOOST_REQUIRE( FlattenSchematicVariant( schematic, wxT( "V1" ), &conflict ) );

    BOOST_CHECK( symbol->GetDNP() );
}


// Coverage guard: a schematic format bump must trigger a denylist review. See the board equivalent.
BOOST_AUTO_TEST_CASE( DenylistCoversCurrentFormat )
{
    BOOST_CHECK_EQUAL( SEXPR_SCHEMATIC_FILE_VERSION, SchDowngradeCoveredVersion() );
    BOOST_CHECK_EQUAL( SEXPR_SYMBOL_LIB_FILE_VERSION, SymbolLibDowngradeCoveredVersion() );
}


BOOST_FIXTURE_TEST_CASE( EmbeddedFilesSurviveMissingRootInstanceForBothTargets, KI_TEST::SCHEMATIC_TEST_FIXTURE )
{
    const std::string fixture = KI_TEST::GetEeschemaTestDataDir() + "/../downgrade/regressions/embedded_files/";
    auto              readFile = []( const std::string& aPath )
    {
        std::ifstream file( aPath, std::ios::binary );
        BOOST_REQUIRE( file.is_open() );
        return std::string( std::istreambuf_iterator<char>( file ), std::istreambuf_iterator<char>() );
    };
    std::map<std::string, std::string> originals;

    for( const std::string& name : { "embedded_files.kicad_pro", "embedded_files.kicad_sch", "child.kicad_sch" } )
    {
        originals.emplace( name, readFile( fixture + name ) );
    }

    for( const DOWNGRADE_TARGET& target : { kicad9, kicad10 } )
    {
        BOOST_TEST_CONTEXT( target.m_id )
        {
            KI_TEST::TEMPORARY_DIRECTORY tmp( "kicad_qa_embedded_schematic" );
            LoadSchematic( wxFileName( fixture + "embedded_files.kicad_sch" ) );
            BOOST_REQUIRE( m_schematic );
            SCH_SHEET* root = m_schematic->GetTopLevelSheet( 0 );
            BOOST_REQUIRE( root );
            BOOST_REQUIRE( root->HasRootInstance() );
            EMBEDDED_FILES expected( *m_schematic->GetEmbeddedFiles(), true );
            BOOST_REQUIRE( !expected.IsEmpty() );

            // Global payload ownership does not depend on optional sheet-instance bookkeeping.
            root->RemoveInstance( KIID_PATH() );
            BOOST_REQUIRE( !root->HasRootInstance() );
            auto children = root->GetScreen()->Items().OfType( SCH_SHEET_T );
            BOOST_REQUIRE_EQUAL( std::distance( children.begin(), children.end() ), 1 );
            SCH_SHEET*        child = static_cast<SCH_SHEET*>( *children.begin() );
            const std::string output = ( tmp.GetPath() / "embedded_files.kicad_sch" ).string();
            const std::string childOutput = ( tmp.GetPath() / "child.kicad_sch" ).string();
            SaveSchematicForTarget( root, m_schematic.get(), output, target );
            SaveSchematicForTarget( child, m_schematic.get(), childOutput, target );
            BOOST_CHECK( readFile( childOutput ).find( "(embedded_files" ) == std::string::npos );
            BOOST_REQUIRE( wxCopyFile( fixture + "embedded_files.kicad_pro",
                                       ( tmp.GetPath() / "embedded_files.kicad_pro" ).string() ) );

            LoadSchematic( wxFileName( output ) );
            const EMBEDDED_FILES& actual = *m_schematic->GetEmbeddedFiles();
            BOOST_REQUIRE_EQUAL( actual.EmbeddedFileMap().size(), expected.EmbeddedFileMap().size() );

            for( const auto& [name, expectedFile] : expected.EmbeddedFileMap() )
            {
                EMBEDDED_FILES::EMBEDDED_FILE* actualFile = actual.GetEmbeddedFile( name );
                BOOST_REQUIRE( actualFile );
                // Parsing verifies the checksum but does not populate the cached validity flag.
                BOOST_REQUIRE( expectedFile->Validate() );
                BOOST_REQUIRE( actualFile->Validate() );
                BOOST_CHECK( actualFile->is_valid );
                BOOST_CHECK( actualFile->type == expectedFile->type );
                BOOST_CHECK_EQUAL( actualFile->data_hash, expectedFile->data_hash );
                BOOST_CHECK_EQUAL( actualFile->compressedEncodedData, expectedFile->compressedEncodedData );
                BOOST_CHECK( actualFile->decompressedData == expectedFile->decompressedData );
            }
        }
    }

    for( const auto& [name, original] : originals )
        BOOST_CHECK( readFile( fixture + name ) == original );
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


BOOST_AUTO_TEST_CASE( PostMasterLineEndingsAndPropertiesCoverStandaloneAndCachedSymbols )
{
    for( const auto& target : { kicad9, kicad10 } )
    {
        for( bool drop : { false, true } )
        {
            SCH_SCREEN  screen;
            LIB_SYMBOL* lib = new LIB_SYMBOL( wxT( "post_master" ) );
            lib->SetCustomProperty( wxT( "Vendor" ), wxT( "Acme" ) );
            lib->GetField( FIELD_T::VALUE )->SetCustomProperty( wxT( "Owner" ), wxT( "Library" ) );
            SCH_PIN* pin = new SCH_PIN( lib );
            pin->SetNumber( wxT( "1" ) );
            pin->SetCustomProperty( wxT( "Role" ), wxT( "Signal" ) );
            lib->AddDrawItem( pin );
            SCH_SHAPE* line = new SCH_SHAPE( SHAPE_T::SEGMENT );
            line->SetEnd( VECTOR2I( schIUScale.mmToIU( 10 ), 0 ) );
            line->SetStroke( STROKE_PARAMS( schIUScale.mmToIU( 0.2 ), LINE_STYLE::SOLID ) );
            line->SetEndEnding( LINE_ENDING( LINE_ENDING_STYLE::ARROW ) );
            lib->AddDrawItem( line );
            screen.AddLibSymbol( lib );
            LIB_SYMBOL standalone( *lib );

            const auto report = ClassifyLibSymbolForDowngrade( &standalone, target, drop );
            BOOST_CHECK( !report.IsBlocked() );
            BOOST_CHECK_EQUAL( report.Count( DOWNGRADE_BUCKET::LOWER ), drop ? 0 : 1 );
            BOOST_CHECK_GE( report.Count( DOWNGRADE_BUCKET::DROP ), drop ? 4 : 3 );
            BOOST_CHECK_EQUAL( ClassifyScreenForDowngrade( &screen, target, drop ).Entries().size(),
                               report.Entries().size() );
            DowngradeLibSymbolInPlace( &standalone, target, drop );
            DowngradeScreenInPlace( &screen, target, drop );
            BOOST_CHECK( !ClassifyLibSymbolForDowngrade( &standalone, target, drop ).IsLossy() );
            BOOST_CHECK( !ClassifyScreenForDowngrade( &screen, target, drop ).IsLossy() );
            BOOST_CHECK( !standalone.HasCustomProperties() );
            BOOST_CHECK( !standalone.GetField( FIELD_T::VALUE )->HasCustomProperties() );
            BOOST_CHECK( !standalone.GetGraphicalPins().front()->HasCustomProperties() );
            if( drop )
            {
                BOOST_CHECK( line->GetShape() == SHAPE_T::SEGMENT );
                BOOST_CHECK( line->GetEndEndingStyle() == LINE_ENDING_STYLE::NONE );
                BOOST_CHECK( line->GetEnd() == VECTOR2I( schIUScale.mmToIU( 10 ), 0 ) );
            }
        }
    }
}


BOOST_AUTO_TEST_CASE( GraphicLineEndingsKeepGroupsAndDropKeepsOriginalBody )
{
    for( bool drop : { false, true } )
    {
        SCH_SCREEN screen;
        SCH_LINE*  line = new SCH_LINE( VECTOR2I(), LAYER_NOTES );
        line->SetEndPoint( VECTOR2I( schIUScale.mmToIU( 10 ), 0 ) );
        line->SetStroke( STROKE_PARAMS( schIUScale.mmToIU( 0.2 ), LINE_STYLE::SOLID ) );
        line->SetStartEnding( LINE_ENDING( LINE_ENDING_STYLE::ARROW_OPEN ) );
        screen.Append( line );
        SCH_GROUP* group = new SCH_GROUP( &screen );
        group->AddItem( line );
        screen.Append( group );
        BOOST_CHECK_EQUAL( ClassifyScreenForDowngrade( &screen, kicad10, drop )
                                   .Count( drop ? DOWNGRADE_BUCKET::DROP : DOWNGRADE_BUCKET::LOWER ),
                           1 );
        DowngradeScreenInPlace( &screen, kicad10, drop );
        BOOST_CHECK( !ClassifyScreenForDowngrade( &screen, kicad10, drop ).IsLossy() );
        BOOST_CHECK( !group->GetItems().empty() );
        for( EDA_ITEM* member : group->GetItems() )
            BOOST_CHECK( member->GetParentGroup() == group );
        if( drop )
        {
            BOOST_CHECK( screen.CheckIfOnDrawList( line ) );
            BOOST_CHECK( line->GetStartEndingStyle() == LINE_ENDING_STYLE::NONE );
            BOOST_CHECK( line->GetEndPoint() == VECTOR2I( schIUScale.mmToIU( 10 ), 0 ) );
        }
    }
}


// An ending that strokes to no ink leaves nothing to put back, so the graphic stays and only the
// ending goes. The board path already guards this.
BOOST_AUTO_TEST_CASE( DegenerateLineEndingKeepsItsGraphic )
{
    for( const DOWNGRADE_TARGET* target : { &kicad9, &kicad10 } )
    {
        BOOST_TEST_CONTEXT( target->m_id.ToStdString() )
        {
            SCH_SCREEN screen;
            SCH_SHAPE* shape = new SCH_SHAPE( SHAPE_T::BEZIER, LAYER_NOTES );
            shape->SetStroke( STROKE_PARAMS( schIUScale.mmToIU( 0.2 ), LINE_STYLE::SOLID ) );
            shape->SetEndEnding( LINE_ENDING( LINE_ENDING_STYLE::ARROW ) );
            screen.Append( shape );

            DowngradeScreenInPlace( &screen, *target );

            auto shapes = screen.Items().OfType( SCH_SHAPE_T );
            BOOST_REQUIRE_EQUAL( std::distance( shapes.begin(), shapes.end() ), 1 );
            BOOST_CHECK( shape->GetEndEndingStyle() == LINE_ENDING_STYLE::NONE );

            LIB_SYMBOL symbol( wxT( "degenerate" ) );
            SCH_SHAPE* drawing = new SCH_SHAPE( SHAPE_T::BEZIER, LAYER_DEVICE );
            drawing->SetStroke( STROKE_PARAMS( schIUScale.mmToIU( 0.2 ), LINE_STYLE::SOLID ) );
            drawing->SetEndEnding( LINE_ENDING( LINE_ENDING_STYLE::ARROW ) );
            symbol.AddDrawItem( drawing );

            DowngradeLibSymbolInPlace( &symbol, *target );

            int kept = 0;

            for( const SCH_ITEM& drawing : symbol.GetDrawItems() )
            {
                if( drawing.Type() != SCH_SHAPE_T )
                    continue;

                kept++;
                BOOST_CHECK( static_cast<const SCH_SHAPE&>( drawing ).GetEndEndingStyle()
                             == LINE_ENDING_STYLE::NONE );
            }

            BOOST_CHECK_EQUAL( kept, 1 );
        }
    }
}


BOOST_AUTO_TEST_CASE( StyledLineEndingsBlockApproximationAndAllowExplicitDrop )
{
    SCH_SCREEN screen;
    SCH_SHAPE* shape = new SCH_SHAPE( SHAPE_T::SEGMENT );
    shape->SetEnd( VECTOR2I( schIUScale.mmToIU( 10 ), 0 ) );
    shape->SetStroke( STROKE_PARAMS( schIUScale.mmToIU( 0.2 ), LINE_STYLE::DASH ) );
    shape->SetEndEnding( LINE_ENDING( LINE_ENDING_STYLE::CIRCLE ) );
    screen.Append( shape );
    BOOST_CHECK( ClassifyScreenForDowngrade( &screen, kicad10 ).IsBlocked() );
    BOOST_CHECK( !ClassifyScreenForDowngrade( &screen, kicad10, true ).IsBlocked() );
    DowngradeScreenInPlace( &screen, kicad10, true );
    BOOST_CHECK( shape->GetStroke().GetLineStyle() == LINE_STYLE::DASH );
    BOOST_CHECK( shape->GetEndEndingStyle() == LINE_ENDING_STYLE::NONE );
}


BOOST_AUTO_TEST_CASE( UnsupportedPropertyTextBlocksWithoutBlockingEscapedLiterals )
{
    SCH_SCREEN screen;
    SCH_TEXT*  text = new SCH_TEXT();
    text->SetText( wxT( "${U1:PROPERTY.Vendor}" ) );
    screen.Append( text );
    BOOST_CHECK( ClassifyScreenForDowngrade( &screen, kicad9 ).IsBlocked() );
    BOOST_CHECK( ClassifyScreenForDowngrade( &screen, kicad10, true ).IsBlocked() );
    text->SetText( wxT( "\\${PROPERTY.Vendor}" ) );
    BOOST_CHECK( !ClassifyScreenForDowngrade( &screen, kicad10 ).IsBlocked() );
    text->SetText( wxT( "PROPERTY.Vendor" ) );
    BOOST_CHECK( !ClassifyScreenForDowngrade( &screen, kicad10 ).IsBlocked() );
}


BOOST_AUTO_TEST_CASE( PropertyTextAndUsedProjectDependenciesBakeBeforeMetadataRemoval )
{
    for( const auto& target : { kicad9, kicad10 } )
    {
        for( bool drop : { false, true } )
        {
            SETTINGS_MANAGER           settings;
            std::unique_ptr<SCHEMATIC> schematic;
            KI_TEST::LoadSchematic( settings, wxT( "NoConnectOnPin" ), schematic );
            auto symbols = schematic->RootScreen()->Items().OfType( SCH_SYMBOL_T );
            BOOST_REQUIRE( symbols.begin() != symbols.end() );
            SCH_SYMBOL* symbol = static_cast<SCH_SYMBOL*>( *symbols.begin() );
            symbol->SetCustomProperty( wxT( "Vendor" ), wxT( "Acme" ) );
            symbol->GetField( FIELD_T::VALUE )->SetText( wxT( "${Alias} / ${REFERENCE}" ) );
            schematic->Project().GetTextVars()[wxT( "Alias" )] = wxT( "${PROPERTY.Vendor}" );
            schematic->Project().GetTextVars()[wxT( "Unused" )] = wxT( "${PROPERTY.Missing}" );
            auto report = ClassifyScreenForDowngrade( schematic->RootScreen(), target, drop );
            BOOST_CHECK( !report.IsBlocked() );
            BOOST_CHECK_EQUAL( report.Count( DOWNGRADE_BUCKET::LOWER ), 0 );
            BOOST_CHECK_GE( report.Count( DOWNGRADE_BUCKET::DROP ), 2 );
            DowngradeScreenInPlace( schematic->RootScreen(), target, drop );
            BOOST_CHECK( symbol->GetField( FIELD_T::VALUE )->GetText() == wxT( "Acme / ${REFERENCE}" ) );
            BOOST_CHECK( !symbol->HasCustomProperties() );
            BOOST_CHECK( !ClassifyScreenForDowngrade( schematic->RootScreen(), target, drop ).IsLossy() );
        }
    }
}


BOOST_AUTO_TEST_CASE( ExpressionsBakeForNative9AndRemainDynamicForNative10 )
{
    for( const auto& target : { kicad9, kicad10 } )
    {
        for( bool drop : { false, true } )
        {
            SETTINGS_MANAGER           settings;
            std::unique_ptr<SCHEMATIC> schematic;
            KI_TEST::LoadSchematic( settings, wxT( "NoConnectOnPin" ), schematic );
            SCH_TEXT* text = new SCH_TEXT();
            text->SetText( wxT( "@{1 + 2} / ${PROJECTNAME}" ) );
            schematic->RootScreen()->Append( text );
            auto report = ClassifyScreenForDowngrade( schematic->RootScreen(), target, drop );
            BOOST_CHECK( !report.IsBlocked() );
            BOOST_CHECK_EQUAL( report.Count( DOWNGRADE_BUCKET::LOWER ), 0 );
            BOOST_CHECK_EQUAL( report.Count( DOWNGRADE_BUCKET::DROP ), target.m_id == wxT( "9.0" ) ? 1 : 0 );
            DowngradeScreenInPlace( schematic->RootScreen(), target, drop );
            BOOST_CHECK( text->GetText()
                         == ( target.m_id == wxT( "9.0" ) ? wxT( "3 / ${PROJECTNAME}" )
                                                          : wxT( "@{1 + 2} / ${PROJECTNAME}" ) ) );
            text->SetText( wxT( "\\@{1 + 2}" ) );
            BOOST_CHECK_EQUAL( ClassifyScreenForDowngrade( schematic->RootScreen(), target ).IsBlocked(),
                               target.m_id == wxT( "9.0" ) );
        }
    }
}


BOOST_AUTO_TEST_CASE( PropertyTextBakingPreservesEscapedLiteralsAndLiteralReplacementSyntax )
{
    SETTINGS_MANAGER           settings;
    std::unique_ptr<SCHEMATIC> schematic;
    KI_TEST::LoadSchematic( settings, wxT( "NoConnectOnPin" ), schematic );
    auto symbols = schematic->RootScreen()->Items().OfType( SCH_SYMBOL_T );
    BOOST_REQUIRE( symbols.begin() != symbols.end() );
    SCH_SYMBOL* symbol = static_cast<SCH_SYMBOL*>( *symbols.begin() );
    symbol->SetCustomProperty( wxT( "Vendor" ), wxT( "\\${REFERENCE} \\@{1 + 2}" ) );
    SCH_FIELD* field = symbol->GetField( FIELD_T::VALUE );
    field->SetText( wxT( "\\${PROPERTY.literal} ${PROPERTY.Vendor}" ) );
    const SCH_SHEET_PATH& path = schematic->CurrentSheet();
    const wxString        shownBefore = field->GetShownText( &path, FOR_CANVAS, wxEmptyString );
    wxString              resolvedBefore = field->ResolveText( field->GetText(), &path, 0, wxEmptyString );
    FinalizeTextVarExpansion( resolvedBefore, INTERNAL );
    BOOST_CHECK_EQUAL( resolvedBefore.ToStdString(), std::string( "\\${PROPERTY.literal} \\${REFERENCE} \\3" ) );
    BOOST_REQUIRE( !ClassifyScreenForDowngrade( schematic->RootScreen(), kicad10 ).IsBlocked() );
    DowngradeScreenInPlace( schematic->RootScreen(), kicad10 );
    BOOST_CHECK_EQUAL( field->GetText().ToStdString(), resolvedBefore.ToStdString() );
    BOOST_CHECK_EQUAL( field->GetShownText( &path, FOR_CANVAS, wxEmptyString ).ToStdString(),
                       shownBefore.ToStdString() );
    BOOST_CHECK( !ClassifyScreenForDowngrade( schematic->RootScreen(), kicad10 ).IsLossy() );
    BOOST_CHECK( ClassifyScreenForDowngrade( schematic->RootScreen(), kicad9 ).IsBlocked() );
}


BOOST_AUTO_TEST_CASE( ContextualTitleShadowsUnusedUnsupportedProjectDefinition )
{
    SETTINGS_MANAGER           settings;
    std::unique_ptr<SCHEMATIC> schematic;
    KI_TEST::LoadSchematic( settings, wxT( "NoConnectOnPin" ), schematic );
    schematic->RootScreen()->GetTitleBlock().SetTitle( wxT( "Ordinary title" ) );
    schematic->Project().GetTextVars()[wxT( "TITLE" )] = wxT( "${PROPERTY.Unknown}" );
    SCH_TEXT* text = new SCH_TEXT();
    text->SetText( wxT( "${TITLE}" ) );
    schematic->RootScreen()->Append( text );
    const SCH_SHEET_PATH& path = schematic->CurrentSheet();
    BOOST_REQUIRE_EQUAL( text->GetShownText( &path, FOR_CANVAS ).ToStdString(), std::string( "Ordinary title" ) );

    for( const auto& target : { kicad9, kicad10 } )
    {
        for( bool drop : { false, true } )
            BOOST_CHECK( !ClassifyScreenForDowngrade( schematic->RootScreen(), target, drop ).IsLossy() );
    }

    BOOST_CHECK_EQUAL( text->GetText().ToStdString(), std::string( "${TITLE}" ) );
    BOOST_CHECK_EQUAL( text->GetShownText( &path, FOR_CANVAS ).ToStdString(), std::string( "Ordinary title" ) );
}


BOOST_AUTO_TEST_CASE( PageBorderDependenciesBlockUnsupportedContextWithoutBlockingOrdinaryText )
{
    SETTINGS_MANAGER           settings;
    std::unique_ptr<SCHEMATIC> schematic;
    KI_TEST::LoadSchematic( settings, wxT( "NoConnectOnPin" ), schematic );
    schematic->Project().GetTextVars()[wxT( "BorderAlias" )] = wxT( "${PROPERTY.Unknown}" );
    schematic->RootScreen()->GetTitleBlock().SetTitle( wxT( "${BorderAlias}" ) );
    SCH_TEXT* text = new SCH_TEXT();
    text->SetText( wxT( "${TITLE}" ) );
    schematic->RootScreen()->Append( text );
    BOOST_CHECK( ClassifyScreenForDowngrade( schematic->RootScreen(), kicad10 ).IsBlocked() );
    schematic->RootScreen()->GetTitleBlock().SetTitle( wxT( "Ordinary title" ) );
    BOOST_CHECK( !ClassifyScreenForDowngrade( schematic->RootScreen(), kicad10 ).IsLossy() );
    schematic->RootScreen()->GetTitleBlock().SetComment( 8, wxT( "@{1 + 2}" ) );
    BOOST_CHECK( ClassifyScreenForDowngrade( schematic->RootScreen(), kicad9 ).IsBlocked() );
    BOOST_CHECK( !ClassifyScreenForDowngrade( schematic->RootScreen(), kicad10 ).IsLossy() );
}


BOOST_AUTO_TEST_CASE( PaddedBusVectorsBlockBeforeNetNamesChange )
{
    std::vector<wxString> members;
    BOOST_REQUIRE( NET_SETTINGS::ParseBusVector( wxT( "D[00..02]" ), nullptr, &members ) );
    BOOST_REQUIRE_EQUAL( members.size(), 3 );
    BOOST_CHECK( members.front() == wxT( "D00" ) );
    SCH_SCREEN screen;
    SCH_LABEL* label = new SCH_LABEL();
    label->SetText( wxT( "D[00..02]" ) );
    screen.Append( label );
    for( const auto& target : { kicad9, kicad10 } )
        BOOST_CHECK( ClassifyScreenForDowngrade( &screen, target ).IsBlocked() );
    label->SetText( wxT( "D[0..2]" ) );
    BOOST_CHECK( !ClassifyScreenForDowngrade( &screen, kicad10 ).IsBlocked() );
    SCHEMATIC schematic( nullptr );
    schematic.CreateDefaultScreens();
    auto alias = std::make_shared<BUS_ALIAS>();
    alias->SetName( wxT( "Data" ) );
    alias->SetMembers( { wxT( "D[00..02]" ) } );
    schematic.AddBusAlias( alias );
    BOOST_CHECK( ClassifySchematicStructureForDowngrade( schematic, kicad10 ).IsBlocked() );
}


BOOST_AUTO_TEST_CASE( RetainedAlternateVariantDefinitionsReportTheirRemoval )
{
    SCH_SCREEN          screen;
    SCH_SYMBOL*         symbol = new SCH_SYMBOL();
    SCH_SYMBOL_INSTANCE instance;
    instance.m_Path.push_back( KIID() );
    SCH_SYMBOL_VARIANT variant( wxT( "Alt" ) );
    variant.m_SymbolOverride = LIB_ID( wxT( "Device" ), wxT( "Alternate" ) );
    instance.m_Variants[variant.m_Name] = variant;
    symbol->AddHierarchicalReference( instance );
    screen.Append( symbol );
    BOOST_CHECK_EQUAL( ClassifyScreenForDowngrade( &screen, kicad10 ).Count( DOWNGRADE_BUCKET::DROP ), 1 );
    DowngradeScreenInPlace( &screen, kicad10 );
    BOOST_CHECK( !symbol->GetInstances().front().m_Variants.at( variant.m_Name ).m_SymbolOverride );
    BOOST_CHECK( !ClassifyScreenForDowngrade( &screen, kicad10 ).IsLossy() );
}


BOOST_AUTO_TEST_CASE( FrozenBusParsersCannotReinterpretMalformedOrOverflowingVectors )
{
    SCH_SCREEN screen;
    SCH_LABEL* label = new SCH_LABEL();
    screen.Append( label );
    for( const auto& target : { kicad9, kicad10 } )
    {
        for( const wxString& malformed : { wxT( "D[..2]" ), wxT( "D[1..]" ), wxT( "D[1..2" ) } )
        {
            BOOST_REQUIRE( !NET_SETTINGS::ParseBusVector( malformed, nullptr, nullptr ) );
            label->SetText( malformed );
            BOOST_CHECK( ClassifyScreenForDowngrade( &screen, target ).IsBlocked() );
        }
        label->SetText( wxString::Format( wxT( "D[%ld..%ld]" ), LONG_MAX - 1, LONG_MAX ) );
        BOOST_REQUIRE( NET_SETTINGS::ParseBusVector( label->GetText(), nullptr, nullptr ) );
        BOOST_CHECK( ClassifyScreenForDowngrade( &screen, target ).IsBlocked() );
        for( const wxString& ordinary :
             { wxT( "D[1..2]" ), wxT( "D[foo]" ), wxT( "D[..0]" ), wxT( "text [literal]" ) } )
        {
            label->SetText( ordinary );
            BOOST_CHECK( !ClassifyScreenForDowngrade( &screen, target ).IsLossy() );
        }
    }
    label->SetText( wxT( "D[1..2]+" ) );
    BOOST_CHECK( ClassifyScreenForDowngrade( &screen, kicad9 ).IsBlocked() );
    BOOST_CHECK( !ClassifyScreenForDowngrade( &screen, kicad10 ).IsLossy() );
    label->SetText( wxT( "I^{2}C[1..2]" ) );
    BOOST_CHECK( !ClassifyScreenForDowngrade( &screen, kicad9 ).IsLossy() );
    BOOST_CHECK( ClassifyScreenForDowngrade( &screen, kicad10 ).IsBlocked() );
}


BOOST_AUTO_TEST_CASE( UsedProjectBusVariablesAreCheckedWithNativeNetnameResolution )
{
    for( const auto& target : { kicad9, kicad10 } )
    {
        SETTINGS_MANAGER           settings;
        std::unique_ptr<SCHEMATIC> schematic;
        KI_TEST::LoadSchematic( settings, wxT( "NoConnectOnPin" ), schematic );
        SCH_LABEL* label = new SCH_LABEL();
        label->SetText( wxT( "${BUS}" ) );
        schematic->RootScreen()->Append( label );
        for( const wxString& bus : { wxT( "D[01..3]" ), wxT( "D[..2]" ), wxT( "D[1..2" ) } )
        {
            schematic->Project().GetTextVars()[wxT( "BUS" )] = bus;
            BOOST_CHECK( ClassifyScreenForDowngrade( schematic->RootScreen(), target ).IsBlocked() );
        }
        for( const wxString& ordinary : { wxT( "D[1..3]" ), wxT( "D[foo]" ) } )
        {
            schematic->Project().GetTextVars()[wxT( "BUS" )] = ordinary;
            BOOST_CHECK( !ClassifyScreenForDowngrade( schematic->RootScreen(), target ).IsLossy() );
        }
    }
}


BOOST_AUTO_TEST_CASE( UnresolvedSelectedAlternateRefusesFlatteningBeforeMutation )
{
    SCHEMATIC schematic( nullptr );
    schematic.CreateDefaultScreens();
    SCH_SHEET_PATH path = schematic.BuildUnorderedSheetList().front();
    SCH_SYMBOL*    symbol = new SCH_SYMBOL();
    path.LastScreen()->Append( symbol );
    SCH_SYMBOL_INSTANCE instance;
    instance.m_Path = path.Path();
    SCH_SYMBOL_VARIANT variant( wxT( "Alt" ) );
    variant.m_DNP = true;
    variant.m_SymbolOverride = LIB_ID( wxT( "Missing" ), wxT( "Alternate" ) );
    instance.m_Variants[variant.m_Name] = variant;
    symbol->AddHierarchicalReference( instance );
    wxString reason;
    BOOST_CHECK( !FlattenSchematicVariant( schematic, variant.m_Name, &reason ) );
    BOOST_CHECK( reason.Contains( wxT( "Missing:Alternate" ) ) );
    BOOST_CHECK( !symbol->GetDNP() );
}


BOOST_AUTO_TEST_CASE( SelectedAlternateBakesNativeLibraryFieldsPinMapAndPinIdentities )
{
    SETTINGS_MANAGER           settings;
    std::unique_ptr<SCHEMATIC> schematic;
    KI_TEST::LoadSchematic( settings, wxT( "NoConnectOnPin" ), schematic );
    KI_TEST::TEMPORARY_DIRECTORY tmp( "kicad_qa_variant_library" );
    SYMBOL_LIBRARY_ADAPTER*      adapter = PROJECT_SCH::SymbolLibAdapter( &schematic->Project() );
    BOOST_REQUIRE( adapter );
    const auto previousTable = adapter->ProjectTable();
    struct TABLE_RESTORE
    {
        LIBRARY_MANAGER& manager;
        wxString         directory;
        ~TABLE_RESTORE() { manager.LoadProjectTables( directory, { LIBRARY_TABLE_TYPE::SYMBOL } ); }
    } restore{ Pgm().GetLibraryManager(),
               previousTable && *previousTable ? wxFileName( ( *previousTable )->Path() ).GetPath() : wxString() };
    const wxString directory = tmp.GetPath().string();
    wxFile         tableFile( directory + wxT( "/sym-lib-table" ), wxFile::write );
    BOOST_REQUIRE( tableFile.IsOpened() );
    BOOST_REQUIRE( tableFile.Write( wxT( "(sym_lib_table (version 7))\n" ) ) );
    tableFile.Close();
    restore.manager.LoadProjectTables( directory, { LIBRARY_TABLE_TYPE::SYMBOL } );
    LIBRARY_TABLE* table = adapter->ProjectTable().value_or( nullptr );
    BOOST_REQUIRE( table );
    LIBRARY_TABLE_ROW& row = table->InsertRow();
    row.SetNickname( wxT( "VariantLibrary" ) );
    row.SetURI( wxString( KI_TEST::GetEeschemaTestDataDir() ) + wxT( "libs/4xxx.kicad_sym" ) );
    row.SetType( wxT( "KiCad" ) );
    row.SetScope( LIBRARY_TABLE_SCOPE::PROJECT );
    adapter->LoadOne( row.Nickname() );
    LIB_SYMBOL* alternate = adapter->LoadSymbol( row.Nickname(), wxT( "4001" ) );
    BOOST_REQUIRE( alternate );
    LIB_SYMBOL base( *alternate );
    base.SetName( wxT( "base" ) );
    base.SetLibId( LIB_ID( wxT( "Embedded" ), base.GetName() ) );
    base.GetField( FIELD_T::VALUE )->SetText( wxT( "Base value" ) );
    SCH_SYMBOL* symbol = new SCH_SYMBOL( base, base.GetLibId(), nullptr, 1, BODY_STYLE::BASE );
    schematic->RootScreen()->Append( symbol );
    const SCH_SHEET_PATH path = schematic->BuildUnorderedSheetList().front();
    symbol->AddHierarchicalReference( path.Path(), wxT( "U99" ), 1 );
    SCH_SYMBOL_VARIANT variant( wxT( "Alt" ) );
    variant.InitializeAttributes( *symbol );
    variant.m_SymbolOverride = LIB_ID( row.Nickname(), wxT( "4001" ) );
    variant.m_Fields[wxT( "Footprint" )] = wxEmptyString;
    variant.m_PinMapOverride.m_Mode = PIN_MAP_OVERRIDE_MODE::FORCE_IDENTITY;
    symbol->AddVariant( path, variant );
    std::vector<KIID> pins;
    for( const auto& pin : symbol->GetRawPins() )
        pins.push_back( pin->m_Uuid );
    const wxString value = symbol->GetFieldText( wxT( "Value" ), &path, variant.m_Name );
    BOOST_REQUIRE( value != wxT( "Base value" ) );
    wxString reason;
    BOOST_REQUIRE_MESSAGE( FlattenSchematicVariant( *schematic, variant.m_Name, &reason ), reason );
    BOOST_CHECK( symbol->GetLibId() == *variant.m_SymbolOverride );
    BOOST_CHECK( symbol->GetField( FIELD_T::VALUE )->GetText() == value );
    BOOST_CHECK( symbol->GetField( FIELD_T::FOOTPRINT )->GetText().IsEmpty() );
    BOOST_CHECK( symbol->GetPinMapOverride() == variant.m_PinMapOverride );
    BOOST_REQUIRE_EQUAL( symbol->GetRawPins().size(), pins.size() );
    for( size_t i = 0; i < pins.size(); ++i )
        BOOST_CHECK( symbol->GetRawPins()[i]->m_Uuid == pins[i] );
    auto cached = schematic->RootScreen()->GetLibSymbols().find( symbol->GetSchSymbolLibraryName() );
    BOOST_REQUIRE( cached != schematic->RootScreen()->GetLibSymbols().end() );
    BOOST_CHECK( cached->second->GetField( FIELD_T::VALUE )->GetText()
                 == alternate->GetField( FIELD_T::VALUE )->GetText() );
}


BOOST_AUTO_TEST_CASE( PostMasterUnsupportedTokenBackstop )
{
    for( const auto& target : { kicad9, kicad10 } )
    {
        for( const wxString& token :
             { wxT( "start_shape" ), wxT( "end_shape" ), wxT( "symbol_override" ), wxT( "custom_property" ) } )
        {
            BOOST_CHECK( FindUnsupportedSchToken( wxT( "(" ) + token + wxT( " \"x\")" ), target ) == token );
            BOOST_CHECK( FindUnsupportedSchToken( wxT( "(text \"(" ) + token + wxT( " x)\")" ), target ).IsEmpty() );
        }
    }
}


static nlohmann::json currentErcExclusionDocument()
{
    return {
        { "erc",
          { { "meta", { { "version", 1 } } },
            { "erc_exclusions",
              nlohmann::json::array(
                      { { { "marker",
                            { { "error_type", "ERCET_PIN_NOT_CONNECTED" },
                              { "position", { { "x_nm", "12300" }, { "y_nm", "-45600" } } },
                              { "items", nlohmann::json::array(
                                                 { { { "value", "11111111-1111-1111-1111-111111111111" } } } ) } } },
                          { "comment", "accepted" } } } ) } } }
    };
}


BOOST_AUTO_TEST_CASE( CurrentErcExclusionsMigrateExactlyAndRemainIdempotent )
{
    for( const auto& target : { kicad9, kicad10 } )
    {
        nlohmann::json       document = currentErcExclusionDocument();
        COMPATIBILITY_REPORT report;
        DowngradeProjectFileJson( document, target, report );
        BOOST_CHECK_EQUAL( document["erc"]["meta"]["version"].get<int>(), 0 );
        const auto& exclusion = document["erc"]["erc_exclusions"].at( 0 );
        BOOST_CHECK( exclusion
                     == nlohmann::json::array( { "pin_not_connected|123|-456|11111111-1111-1111-1111-111111111111|"
                                                 "00000000-0000-0000-0000-000000000000|||",
                                                 "accepted" } ) );
        BOOST_CHECK( !report.IsLossy() );
        const nlohmann::json once = document;
        COMPATIBILITY_REPORT repeated;
        DowngradeProjectFileJson( document, target, repeated );
        BOOST_CHECK( document == once );
        BOOST_CHECK( !repeated.IsLossy() );
    }
}


BOOST_AUTO_TEST_CASE( ErcExclusionsPreserveInstancePathsAndChildTextKeys )
{
    for( const auto& target : { kicad9, kicad10 } )
    {
        nlohmann::json       document = currentErcExclusionDocument();
        auto&                marker = document["erc"]["erc_exclusions"][0]["marker"];
        const nlohmann::json path = {
            { "path", nlohmann::json::array( { { { "value", "22222222-2222-2222-2222-222222222222" } } } ) }
        };
        marker["sheet_specific_path"] = path;
        marker["main_item_sheet_path"] = path;
        marker["aux_item_sheet_path"] = path;
        COMPATIBILITY_REPORT pathReport;
        nlohmann::json       allPaths = document;
        DowngradeProjectFileJson( allPaths, target, pathReport );
        BOOST_REQUIRE_EQUAL( allPaths["erc"]["erc_exclusions"].size(), 1 );
        BOOST_CHECK_EQUAL( allPaths["erc"]["erc_exclusions"][0][0].get<std::string>(),
                           "pin_not_connected|123|-456|11111111-1111-1111-1111-111111111111|"
                           "00000000-0000-0000-0000-000000000000|/22222222-2222-2222-2222-222222222222|"
                           "/22222222-2222-2222-2222-222222222222|/22222222-2222-2222-2222-222222222222" );
        BOOST_CHECK( !pathReport.IsLossy() );
        marker.erase( "aux_item_sheet_path" );
        marker["error_type"] = "ERCET_GENERIC_WARNING";
        marker["child"] = { { "text_value", "raw warning text" } };
        COMPATIBILITY_REPORT report;
        DowngradeProjectFileJson( document, target, report );
        BOOST_REQUIRE_EQUAL( document["erc"]["erc_exclusions"].size(), 1 );
        BOOST_CHECK_EQUAL( document["erc"]["erc_exclusions"][0][0].get<std::string>(),
                           "generic-warning|123|-456|11111111-1111-1111-1111-111111111111|raw warning "
                           "text|/22222222-2222-2222-2222-222222222222|/22222222-2222-2222-2222-222222222222|" );
        BOOST_CHECK( !report.IsLossy() );
    }
}


BOOST_AUTO_TEST_CASE( UnrepresentableErcExclusionsAreReportedAndDropped )
{
    for( const auto& target : { kicad9, kicad10 } )
    {
        for( int invalid = 0; invalid < 4; ++invalid )
        {
            nlohmann::json document = currentErcExclusionDocument();
            auto&          marker = document["erc"]["erc_exclusions"][0]["marker"];
            if( invalid == 0 )
                marker["error_type"] = "ERCET_EMPTY_LABEL_NAME";
            else if( invalid == 1 )
                marker["items"][0]["value"] = "malformed-id";
            else if( invalid == 2 )
                marker["position"]["x_nm"] = "12301";
            else
            {
                marker["error_type"] = "ERCET_GENERIC_WARNING";
                marker["child"] = { { "text_value", "cannot|encode" } };
            }
            COMPATIBILITY_REPORT report;
            DowngradeProjectFileJson( document, target, report );
            BOOST_CHECK( !report.IsBlocked() );
            BOOST_CHECK_EQUAL( report.Count( DOWNGRADE_BUCKET::DROP ), 1 );
            BOOST_CHECK( document["erc"]["erc_exclusions"].empty() );
        }
    }
}


BOOST_AUTO_TEST_CASE( UnreviewedProjectSchemasBlockWithoutMutation )
{
    for( const auto& target : { kicad9, kicad10 } )
    {
        for( int schema = 0; schema < 5; ++schema )
        {
            nlohmann::json document = currentErcExclusionDocument();
            if( schema == 0 )
                document["meta"]["version"] = 5;
            else if( schema == 1 )
                document["board"]["design_settings"]["meta"]["version"] = 4;
            else if( schema == 2 )
                document["erc"]["meta"]["version"] = 2;
            else if( schema == 3 )
                document["tuning_profiles"]["meta"]["version"] = 3;
            else
                document["net_settings"]["meta"]["version"] = 6;
            const nlohmann::json original = document;
            COMPATIBILITY_REPORT report;
            DowngradeProjectFileJson( document, target, report );
            BOOST_CHECK( report.IsBlocked() );
            BOOST_CHECK( document == original );
        }
    }
}


BOOST_AUTO_TEST_SUITE_END()
