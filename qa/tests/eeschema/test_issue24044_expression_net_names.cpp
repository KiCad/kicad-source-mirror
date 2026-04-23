/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.TXT for contributors.
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
 * https://www.gnu.org/licenses/gpl-3.0.en.html
 * or you may search the http://www.gnu.org website for the version 32 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include <qa_utils/wx_utils/unit_test_utils.h>
#include <schematic_utils/schematic_file_util.h>

#include <schematic.h>
#include <sch_sheet.h>
#include <sch_sheet_path.h>
#include <sch_sheet_pin.h>
#include <sch_screen.h>
#include <sch_label.h>
#include <settings/settings_manager.h>
#include <locale_io.h>


struct ISSUE_24044_FIXTURE
{
    SETTINGS_MANAGER           m_settingsManager;
    std::unique_ptr<SCHEMATIC> m_schematic;
};


/**
 * Test for issue #24044: Inconsistent internal evaluation of net names when using expressions.
 *
 * When a hierarchical sheet pin uses an expression that depends on the sheet instance
 * (e.g. "@{(${#}-2)*2+1}" where ${#} is the page number), the net name resolved for the
 * sheet pin on the parent sheet must match the name resolved for the matching
 * hierarchical label inside the child sheet.
 *
 * Schematic structure:
 *   Root sheet (page 1) with connector J1
 *     - Sheet "Channels 0-1" (page 2, channels.kicad_sch)
 *     - Sheet "Channels 2-3" (page 3, channels.kicad_sch)
 *   Child sheet channels.kicad_sch:
 *     - Hierarchical labels "Ch@{(${#}-2)*2+0}" and "Ch@{(${#}-2)*2+1}"
 *     - Resistors R1/R3 and R2/R4 driven by the labels
 *
 * Expected resolved names (pin on parent must equal label inside child):
 *   Instance "Channels 0-1" (page 2): Ch0, Ch1
 *   Instance "Channels 2-3" (page 3): Ch2, Ch3
 *
 * The bug caused the parent-side sheet pin to resolve with a doubly-nested path that
 * yielded an empty page number, producing bogus names (e.g. Ch-4/Ch-3) and breaking
 * connectivity across the hierarchy.
 */
BOOST_FIXTURE_TEST_CASE( Issue24044ExpressionNetNames, ISSUE_24044_FIXTURE )
{
    LOCALE_IO dummy;

    KI_TEST::LoadSchematic( m_settingsManager, "issue24044/issue24044", m_schematic );

    SCH_SHEET_LIST hierarchy = m_schematic->Hierarchy();

    // Collect the two sub-sheet instance paths (both reference channels.kicad_sch but on
    // different pages).
    std::vector<SCH_SHEET_PATH> childPaths;

    for( const SCH_SHEET_PATH& path : hierarchy )
    {
        if( path.size() == 2 )
            childPaths.push_back( path );
    }

    BOOST_REQUIRE_EQUAL( childPaths.size(), 2U );

    for( const SCH_SHEET_PATH& childPath : childPaths )
    {
        SCH_SHEET* childSheet = childPath.Last();
        BOOST_REQUIRE( childSheet );

        // Build the lookup on the child sheet's hierarchical labels
        std::set<wxString> labelTexts;

        for( SCH_ITEM* item : childSheet->GetScreen()->Items() )
        {
            if( item->Type() == SCH_HIER_LABEL_T )
            {
                SCH_HIERLABEL* label = static_cast<SCH_HIERLABEL*>( item );
                labelTexts.insert( label->GetShownText( &childPath, false ) );
            }
        }

        wxString labelList;

        for( const wxString& t : labelTexts )
        {
            if( !labelList.IsEmpty() )
                labelList += wxT( "," );

            labelList += t;
        }

        // Every sheet pin's resolved name must appear in the child's hierarchical labels
        for( SCH_SHEET_PIN* pin : childSheet->GetPins() )
        {
            wxString pinText = pin->GetShownText( &childPath, false );

            BOOST_CHECK_MESSAGE( !pinText.Contains( wxT( "@{" ) )
                                         && !pinText.Contains( wxT( "${" ) ),
                                 wxString::Format( "Sheet pin '%s' on path '%s' did not fully "
                                                   "resolve expression variables",
                                                   pinText,
                                                   childPath.PathHumanReadable() ) );

            BOOST_CHECK_MESSAGE( labelTexts.count( pinText ) == 1,
                                 wxString::Format( "Sheet pin '%s' on path '%s' has no matching "
                                                   "hierarchical label inside the child sheet. "
                                                   "Labels: [%s]",
                                                   pinText,
                                                   childPath.PathHumanReadable(),
                                                   labelList ) );
        }
    }

    // Each sheet instance should produce a different resolved name (the expression depends
    // on the page number, so the two instances must disambiguate).
    std::set<wxString> allPinTexts;

    for( const SCH_SHEET_PATH& childPath : childPaths )
    {
        for( SCH_SHEET_PIN* pin : childPath.Last()->GetPins() )
            allPinTexts.insert( pin->GetShownText( &childPath, false ) );
    }

    BOOST_CHECK_EQUAL( allPinTexts.size(), 4U );

    // Verify the computed names match the expected formula for each page number
    for( const SCH_SHEET_PATH& childPath : childPaths )
    {
        long page = 0;
        BOOST_REQUIRE( childPath.GetPageNumber().ToLong( &page ) );

        std::set<wxString> expected = {
            wxString::Format( "Ch%ld", ( page - 2 ) * 2 + 0 ),
            wxString::Format( "Ch%ld", ( page - 2 ) * 2 + 1 )
        };

        std::set<wxString> actual;

        for( SCH_SHEET_PIN* pin : childPath.Last()->GetPins() )
            actual.insert( pin->GetShownText( &childPath, false ) );

        wxString actualList;

        for( const wxString& t : actual )
        {
            if( !actualList.IsEmpty() )
                actualList += wxT( "," );

            actualList += t;
        }

        BOOST_CHECK_MESSAGE( actual == expected,
                             wxString::Format( "Page %ld expected {Ch%ld, Ch%ld} but got {%s}",
                                               page,
                                               ( page - 2 ) * 2 + 0,
                                               ( page - 2 ) * 2 + 1,
                                               actualList ) );
    }
}


/**
 * Companion regression test that pins the contract SCH_LABEL_BASE::ResolveTextVar relies on.
 *
 * Callers use one of two path forms when resolving a sheet pin's text:
 *   1. The child-sheet path (path.Last() is the sheet that owns the pin) — used by e.g.
 *      connection_graph.cpp driverName() and erc.cpp hier_label checks.
 *   2. The parent-screen path (path.Last() is the parent screen, child sheet not yet pushed)
 *      — used by e.g. sch_selection_tool.cpp pin-trick label generation.
 *
 * Both forms must resolve to the same text for the same sheet instance.  The bug fixed by
 * issue #24044 broke form (1) by unconditionally pushing the owner sheet again, yielding a
 * doubly-nested path whose page-number lookup failed.
 */
BOOST_FIXTURE_TEST_CASE( Issue24044PathFormEquivalence, ISSUE_24044_FIXTURE )
{
    LOCALE_IO dummy;

    KI_TEST::LoadSchematic( m_settingsManager, "issue24044/issue24044", m_schematic );

    std::vector<SCH_SHEET_PATH> childPaths;

    for( const SCH_SHEET_PATH& path : m_schematic->Hierarchy() )
    {
        if( path.size() == 2 )
            childPaths.push_back( path );
    }

    BOOST_REQUIRE_EQUAL( childPaths.size(), 2U );

    for( const SCH_SHEET_PATH& childPath : childPaths )
    {
        SCH_SHEET* childSheet = childPath.Last();
        BOOST_REQUIRE( childSheet );

        // Build the parent-screen path (drop the trailing child sheet).  This is the path
        // shape that callers such as sch_selection_tool pass in.
        SCH_SHEET_PATH parentPath = childPath;
        parentPath.pop_back();

        BOOST_REQUIRE_EQUAL( parentPath.size() + 1U, childPath.size() );

        for( SCH_SHEET_PIN* pin : childSheet->GetPins() )
        {
            wxString fromChildPath  = pin->GetShownText( &childPath, false );
            wxString fromParentPath = pin->GetShownText( &parentPath, false );

            BOOST_CHECK_MESSAGE( fromChildPath == fromParentPath,
                                 wxString::Format( "Sheet pin resolution diverged between "
                                                   "path forms: child='%s' parent='%s'",
                                                   fromChildPath,
                                                   fromParentPath ) );
        }
    }
}
