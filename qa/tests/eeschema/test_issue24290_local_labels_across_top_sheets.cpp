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
 * https://www.gnu.org/licenses/gpl-3.0.en.html
 */

/**
 * Test for issue #24290: Local labels create connections between top-level sheets.
 *
 * Local labels with identical text placed on different top-level sheets are
 * being merged into one net.  Each top-level sheet should have its own
 * isolated namespace for local labels.
 */

#include <boost/test/unit_test.hpp>

#include <connection_graph.h>
#include <sch_label.h>
#include <sch_screen.h>
#include <sch_sheet.h>
#include <schematic.h>
#include <settings/settings_manager.h>


struct ISSUE24290_FIXTURE
{
    ISSUE24290_FIXTURE() :
            m_settingsManager()
    {
        m_settingsManager.LoadProject( "" );
        m_schematic = std::make_unique<SCHEMATIC>( nullptr );
        m_schematic->SetProject( &m_settingsManager.Prj() );
    }

    ~ISSUE24290_FIXTURE() { m_schematic.reset(); }

    SCH_SHEET* makeTopLevelSheet( const wxString& aName, const wxString& aFileName )
    {
        SCH_SHEET*  sheet = new SCH_SHEET( m_schematic.get() );
        SCH_SCREEN* screen = new SCH_SCREEN( m_schematic.get() );

        const_cast<KIID&>( sheet->m_Uuid ) = screen->GetUuid();
        sheet->SetScreen( screen );
        sheet->GetField( FIELD_T::SHEET_NAME )->SetText( aName );
        screen->SetFileName( aFileName );

        return sheet;
    }

    void addLocalLabel( SCH_SHEET* aSheet, const wxString& aText, const VECTOR2I& aPos )
    {
        SCH_LABEL* label = new SCH_LABEL( aPos, aText );
        aSheet->GetScreen()->Append( label );
    }

    SETTINGS_MANAGER           m_settingsManager;
    std::unique_ptr<SCHEMATIC> m_schematic;
};


BOOST_FIXTURE_TEST_CASE( Issue24290_LocalLabelsDontCrossTopLevelSheets, ISSUE24290_FIXTURE )
{
    // Build three sibling top-level sheets, each with a local label "LOCAL".
    SCH_SHEET* sheetA = makeTopLevelSheet( "SheetA", "sheet_a.kicad_sch" );
    SCH_SHEET* sheetB = makeTopLevelSheet( "SheetB", "sheet_b.kicad_sch" );
    SCH_SHEET* sheetC = makeTopLevelSheet( "SheetC", "sheet_c.kicad_sch" );

    m_schematic->SetTopLevelSheets( { sheetA, sheetB, sheetC } );

    sheetA->GetScreen()->SetPageNumber( wxT( "1" ) );
    sheetB->GetScreen()->SetPageNumber( wxT( "2" ) );
    sheetC->GetScreen()->SetPageNumber( wxT( "3" ) );

    addLocalLabel( sheetA, "LOCAL", VECTOR2I( 0, 0 ) );
    addLocalLabel( sheetB, "LOCAL", VECTOR2I( 0, 0 ) );
    addLocalLabel( sheetC, "LOCAL", VECTOR2I( 0, 0 ) );

    m_schematic->RefreshHierarchy();
    SCH_SHEET_LIST sheets = m_schematic->Hierarchy();
    BOOST_REQUIRE_EQUAL( sheets.size(), 3u );

    m_schematic->ConnectionGraph()->Recalculate( sheets, true );

    // Walk the net map and collect the net codes of every subgraph driven by
    // a local label whose text is "LOCAL".  If the bug is present, all three
    // share the same net code; once fixed, they must each have their own.
    std::set<int>      labelNetCodes;
    std::set<wxString> labelNetNames;

    for( const auto& [key, subgraphs] : m_schematic->ConnectionGraph()->GetNetMap() )
    {
        for( CONNECTION_SUBGRAPH* sg : subgraphs )
        {
            const SCH_ITEM* driver = sg->GetDriver();

            if( !driver || driver->Type() != SCH_LABEL_T )
                continue;

            if( static_cast<const SCH_LABEL*>( driver )->GetText() == wxT( "LOCAL" ) )
            {
                labelNetCodes.insert( key.Netcode );
                labelNetNames.insert( key.Name );
            }
        }
    }

    BOOST_TEST_MESSAGE( "Distinct LOCAL net names: " << labelNetNames.size() );

    for( const wxString& n : labelNetNames )
        BOOST_TEST_MESSAGE( "  " << n );

    BOOST_CHECK_MESSAGE( labelNetCodes.size() == 3u, "Local labels 'LOCAL' on three different top-level sheets should "
                                                     "yield three distinct net codes, got "
                                                             << labelNetCodes.size() );

    BOOST_CHECK_MESSAGE( labelNetNames.size() == 3u, "Local labels 'LOCAL' on three different top-level sheets should "
                                                     "yield three distinct net names, got "
                                                             << labelNetNames.size() );
}
