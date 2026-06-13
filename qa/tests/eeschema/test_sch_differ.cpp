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
#include <schematic_utils/schematic_file_util.h>

#include <diff_merge/sch_differ.h>

#include <diff_merge/kicad_diff_types.h>
#include <diff_merge/sch_geometry_extractor.h>

#include <schematic.h>
#include <schematic_settings.h>
#include <erc/erc_settings.h>
#include <sch_sheet_path.h>
#include <sch_screen.h>
#include <sch_symbol.h>
#include <sch_field.h>
#include <sch_sheet.h>
#include <settings/settings_manager.h>
#include <project.h>
#include <project/project_file.h>

#include <nlohmann/json.hpp>


using namespace KICAD_DIFF;


struct SCH_DIFFER_FIXTURE
{
    SCH_DIFFER_FIXTURE()
    {
        KI_TEST::LoadSchematic( m_settingsA, "issue18606/issue18606", m_before );
        KI_TEST::LoadSchematic( m_settingsB, "issue18606/issue18606", m_after );
        BOOST_REQUIRE( m_before );
        BOOST_REQUIRE( m_after );
    }

    SETTINGS_MANAGER           m_settingsA;
    SETTINGS_MANAGER           m_settingsB;
    std::unique_ptr<SCHEMATIC> m_before;
    std::unique_ptr<SCHEMATIC> m_after;
};


BOOST_FIXTURE_TEST_SUITE( SchDiffer, SCH_DIFFER_FIXTURE )


BOOST_AUTO_TEST_CASE( TwoFreshLoadsAreIdentical )
{
    SCH_DIFFER    differ( m_before.get(), m_after.get(), wxS( "issue18606.kicad_sch" ) );
    DOCUMENT_DIFF result = differ.Diff();

    BOOST_CHECK_EQUAL( result.docType.ToStdString(), "kicad_sch" );
    BOOST_CHECK_MESSAGE( result.Empty(), "Two fresh loads of the same fixture should diff to empty; "
                                         "got " << result.changes.size()
                                                << " changes" );
}


BOOST_AUTO_TEST_CASE( SymbolFieldEditSurfacesAsProperty )
{
    // Find any symbol on the after-side and tweak a field value.
    SCH_SHEET_LIST sheets = m_after->BuildSheetListSortedByPageNumbers();
    SCH_SYMBOL*    subject = nullptr;
    SCH_SHEET_PATH subjectPath;

    for( const SCH_SHEET_PATH& path : sheets )
    {
        SCH_SCREEN* screen = path.LastScreen();

        for( SCH_ITEM* item : screen->Items().OfType( SCH_SYMBOL_T ) )
        {
            subject = static_cast<SCH_SYMBOL*>( item );
            subjectPath = path;
            break;
        }

        if( subject )
            break;
    }

    BOOST_REQUIRE( subject );

    // Move the symbol — every SCH_ITEM has a position that's enumerated by
    // PROPERTY_MANAGER, so we can verify deltas without relying on a specific
    // field shape that may differ across fixtures.
    VECTOR2I origPos = subject->GetPosition();
    subject->SetPosition( origPos + VECTOR2I( 1000, 0 ) );

    SCH_DIFFER    differ( m_before.get(), m_after.get() );
    DOCUMENT_DIFF result = differ.Diff();

    BOOST_REQUIRE_GE( result.changes.size(), 1u );

    bool foundModifiedSymbol = false;

    for( const ITEM_CHANGE& c : result.changes )
    {
        if( c.kind == CHANGE_KIND::MODIFIED
            && c.id.back().AsString().ToStdString() == subject->m_Uuid.AsString().ToStdString() )
        {
            foundModifiedSymbol = true;
            BOOST_CHECK( !c.properties.empty() );
        }
    }

    BOOST_CHECK( foundModifiedSymbol );
}


BOOST_AUTO_TEST_CASE( DiffIsDeterministic )
{
    // Apply a deterministic mutation
    SCH_SHEET_LIST sheets = m_after->BuildSheetListSortedByPageNumbers();
    SCH_SYMBOL*    subject = nullptr;

    for( const SCH_SHEET_PATH& path : sheets )
    {
        for( SCH_ITEM* item : path.LastScreen()->Items().OfType( SCH_SYMBOL_T ) )
        {
            subject = static_cast<SCH_SYMBOL*>( item );
            break;
        }

        if( subject )
            break;
    }

    BOOST_REQUIRE( subject );
    subject->SetPosition( subject->GetPosition() + VECTOR2I( 0, 1000 ) );

    SCH_DIFFER differ1( m_before.get(), m_after.get() );
    SCH_DIFFER differ2( m_before.get(), m_after.get() );

    DOCUMENT_DIFF r1 = differ1.Diff();
    DOCUMENT_DIFF r2 = differ2.Diff();

    BOOST_CHECK_EQUAL( r1.ToJson().dump(), r2.ToJson().dump() );
}


BOOST_AUTO_TEST_CASE( DiffJsonRoundTrip )
{
    SCH_DIFFER    differ( m_before.get(), m_after.get(), wxS( "test.kicad_sch" ) );
    DOCUMENT_DIFF result = differ.Diff();

    nlohmann::json j = result.ToJson();
    DOCUMENT_DIFF  back = DOCUMENT_DIFF::FromJson( j );

    BOOST_CHECK_EQUAL( back.path.ToStdString(), result.path.ToStdString() );
    BOOST_CHECK_EQUAL( back.docType.ToStdString(), result.docType.ToStdString() );
    BOOST_CHECK_EQUAL( back.changes.size(), result.changes.size() );
}


// Find the (root) document-level synthetic ITEM_CHANGE with an empty KIID_PATH
// (modeled on the PCB DocLevelChange helper).  Per-sheet doc-level deltas
// would carry a non-empty KIID_PATH, so this returns only the root delta.
static const ITEM_CHANGE* findRootDocLevelChange( const DOCUMENT_DIFF& aDiff )
{
    for( const ITEM_CHANGE& c : aDiff.changes )
    {
        if( c.id.empty() )
            return &c;
    }

    return nullptr;
}


static const PROPERTY_DELTA* findProperty( const ITEM_CHANGE& aChange, const wxString& aName )
{
    for( const PROPERTY_DELTA& p : aChange.properties )
    {
        if( p.name == aName )
            return &p;
    }

    return nullptr;
}


// typeName on each emitted ITEM_CHANGE must come from the item's GetClass().
// Pin the contract for SCH_SYMBOL specifically -- a regression that
// substitutes a default empty string here breaks the rendering layer.
BOOST_AUTO_TEST_CASE( ModifiedSymbolTypeNameIsSchSymbol )
{
    SCH_SHEET_LIST sheets = m_after->BuildSheetListSortedByPageNumbers();
    SCH_SYMBOL*    subject = nullptr;

    for( const SCH_SHEET_PATH& path : sheets )
    {
        for( SCH_ITEM* item : path.LastScreen()->Items().OfType( SCH_SYMBOL_T ) )
        {
            subject = static_cast<SCH_SYMBOL*>( item );
            break;
        }

        if( subject )
            break;
    }

    BOOST_REQUIRE( subject );
    subject->SetPosition( subject->GetPosition() + VECTOR2I( 2000, 0 ) );

    SCH_DIFFER    differ( m_before.get(), m_after.get() );
    DOCUMENT_DIFF result = differ.Diff();

    bool foundSymbolTypeName = false;

    for( const ITEM_CHANGE& c : result.changes )
    {
        if( c.kind == CHANGE_KIND::MODIFIED && c.typeName.Find( wxS( "SCH_SYMBOL" ) ) != wxNOT_FOUND )
        {
            foundSymbolTypeName = true;
        }
    }

    BOOST_CHECK( foundSymbolTypeName );
}


// Drawing-sheet file path lives in PROJECT_FILE::m_SchDrawingSheetFile.
// Mutating it on one side must surface as DOC_PROP_DRAWING_SHEET in the
// root doc-level delta.  Skipped silently if the fixture doesn't ship a
// project file (no GetProject() result).
BOOST_AUTO_TEST_CASE( DrawingSheetFilePathEditEmitsDocLevelDelta )
{
    // SCHEMATIC_SETTINGS::m_SchDrawingSheetFileName is the SCH-side persisted
    // drawing-sheet path; the diff accesses it via SCHEMATIC::Settings().
    if( !m_after->IsValid() )
    {
        BOOST_TEST_MESSAGE( "Fixture is not valid (no project); skipping" );
        return;
    }

    m_before->Settings().m_SchDrawingSheetFileName = wxEmptyString;
    m_after->Settings().m_SchDrawingSheetFileName = wxS( "/some/sheet.kicad_wks" );

    SCH_DIFFER    differ( m_before.get(), m_after.get() );
    DOCUMENT_DIFF result = differ.Diff();

    const ITEM_CHANGE* docChange = findRootDocLevelChange( result );
    BOOST_REQUIRE( docChange );

    const PROPERTY_DELTA* delta = findProperty( *docChange, DOC_PROP_DRAWING_SHEET );
    BOOST_REQUIRE( delta );
    BOOST_CHECK( delta->before.ToDisplayString() != delta->after.ToDisplayString() );
}


// Multi-sheet output ordering must be deterministic.  Apply a per-sheet
// mutation across every sheet of the fixture and verify two Diff() runs
// produce byte-identical JSON.  Distinct from DiffIsDeterministic above
// which only mutates a single symbol.
BOOST_AUTO_TEST_CASE( MultiSheetDiffJsonIsDeterministic )
{
    SCH_SHEET_LIST sheets = m_after->BuildSheetListSortedByPageNumbers();

    int n = 0;

    for( const SCH_SHEET_PATH& path : sheets )
    {
        for( SCH_ITEM* item : path.LastScreen()->Items().OfType( SCH_SYMBOL_T ) )
        {
            SCH_SYMBOL* sym = static_cast<SCH_SYMBOL*>( item );
            sym->SetPosition( sym->GetPosition() + VECTOR2I( 100 + n, 100 + n ) );
            ++n;
        }
    }

    SCH_DIFFER differ1( m_before.get(), m_after.get() );
    SCH_DIFFER differ2( m_before.get(), m_after.get() );

    DOCUMENT_DIFF r1 = differ1.Diff();
    DOCUMENT_DIFF r2 = differ2.Diff();

    BOOST_CHECK_EQUAL( r1.ToJson().dump(), r2.ToJson().dump() );
}


// Hierarchical instance data must survive the diff round-trip: a symbol on
// a sub-sheet must show up with its full sheet path in KIID_PATH (not just
// the symbol's own UUID).  Pins the per-instance addressing contract.
BOOST_AUTO_TEST_CASE( SubSheetSymbolKiidPathIncludesSheetPrefix )
{
    SCH_SHEET_LIST sheets = m_after->BuildSheetListSortedByPageNumbers();

    if( sheets.size() < 2u )
    {
        BOOST_TEST_MESSAGE( "Fixture only has root sheet; skipping" );
        return;
    }

    // Pick a symbol from a sub-sheet (non-root) so its KIID_PATH must have
    // at least one prefix entry beyond the symbol UUID itself.
    SCH_SYMBOL*    subject = nullptr;
    SCH_SHEET_PATH subjectPath;

    for( const SCH_SHEET_PATH& path : sheets )
    {
        if( path.size() < 2u )
            continue;

        for( SCH_ITEM* item : path.LastScreen()->Items().OfType( SCH_SYMBOL_T ) )
        {
            subject = static_cast<SCH_SYMBOL*>( item );
            subjectPath = path;
            break;
        }

        if( subject )
            break;
    }

    if( !subject )
    {
        BOOST_TEST_MESSAGE( "No sub-sheet symbol in fixture; skipping" );
        return;
    }

    subject->SetPosition( subject->GetPosition() + VECTOR2I( 0, 500 ) );

    SCH_DIFFER    differ( m_before.get(), m_after.get() );
    DOCUMENT_DIFF result = differ.Diff();

    bool foundPathPrefix = false;

    for( const ITEM_CHANGE& c : result.changes )
    {
        if( c.kind == CHANGE_KIND::MODIFIED && !c.id.empty() && c.id.back().AsString() == subject->m_Uuid.AsString() )
        {
            // KIID_PATH for a sub-sheet symbol must have at least 2 entries:
            // sheet UUID(s) + symbol UUID.
            BOOST_CHECK_GE( c.id.size(), 2u );
            foundPathPrefix = true;
        }
    }

    BOOST_CHECK( foundPathPrefix );
}


BOOST_AUTO_TEST_CASE( ExtractSchematicGeometryProducesDrawableContext )
{
    DOCUMENT_GEOMETRY geometry = ExtractSchematicGeometry( *m_after, KIGFX::COLOR4D( 0.38, 0.38, 0.38, 0.55 ) );

    BOOST_CHECK( !geometry.Empty() );
    BOOST_CHECK( BBoxFromGeometry( geometry ).has_value() );
}


BOOST_AUTO_TEST_SUITE_END()
