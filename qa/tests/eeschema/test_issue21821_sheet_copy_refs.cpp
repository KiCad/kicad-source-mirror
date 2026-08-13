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
 * or you may search the http://www.gnu.org website for the version 3 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

/**
 * @file test_issue21821_sheet_copy_refs.cpp
 *
 * Test for issue #21821: copying a hierarchical sheet reassigns every diode the same reference.
 *
 * Pasting a sheet whose file is already in the design reuses the live screen, so the paste
 * cleanup runs over symbols that were never on the clipboard.  In the reporter's design the
 * LED_Dual symbol stores its per-sheet references under an empty project name while the
 * resistors store theirs under the project name, which is why only the diodes lost their
 * annotation.
 */

#include <qa_utils/wx_utils/unit_test_utils.h>
#include "eeschema_test_utils.h"

#include <locale_io.h>
#include <schematic.h>
#include <sch_screen.h>
#include <sch_sheet.h>
#include <sch_sheet_path.h>
#include <sch_symbol.h>
#include <tools/sch_tool_utils.h>


/// Sheet path of the "dual gpio LED0" instance, taken from the reporter's files.
static const wxString LED0_PATH = wxS( "/17c4bf89-456c-4209-aa21-31be7397f5ca"
                                       "/adf4c8f0-c33f-488a-8e8d-4ca6638d0792"
                                       "/fd71c97e-9deb-4a95-a605-8e0c9eaa9729" );

/// Sheet path of the "dual gpio LED8" instance, which shares LED0's screen.
static const wxString LED8_PATH = wxS( "/17c4bf89-456c-4209-aa21-31be7397f5ca"
                                       "/adf4c8f0-c33f-488a-8e8d-4ca6638d0792"
                                       "/af5bb00b-cab5-44ce-9731-12dfdbf0dd8b" );


struct ISSUE21821_FIXTURE : public KI_TEST::SCHEMATIC_TEST_FIXTURE
{
    ISSUE21821_FIXTURE()
    {
        LOCALE_IO dummy;

        wxFileName fn( KI_TEST::GetEeschemaTestDataDir() );
        fn.AppendDir( wxS( "issue21821" ) );
        fn.SetName( wxS( "issue21821" ) );
        fn.SetExt( FILEEXT::KiCadSchematicFileExtension );

        LoadSchematic( fn );

        SCH_SHEET_LIST hierarchy = m_schematic->BuildSheetListSortedByPageNumbers();

        std::optional<SCH_SHEET_PATH> led0 = hierarchy.GetSheetPathByKIIDPath( KIID_PATH( LED0_PATH ) );
        std::optional<SCH_SHEET_PATH> led8 = hierarchy.GetSheetPathByKIIDPath( KIID_PATH( LED8_PATH ) );

        BOOST_REQUIRE( led0.has_value() && led8.has_value() );

        m_led0Path = *led0;
        m_led8Path = *led8;

        BOOST_REQUIRE_MESSAGE( m_led0Path.LastScreen() == m_led8Path.LastScreen(),
                               "The two sub-sheets must share one screen for this test to mean "
                               "anything" );

        for( SCH_ITEM* item : m_led0Path.LastScreen()->Items().OfType( SCH_SYMBOL_T ) )
        {
            SCH_SYMBOL* symbol = static_cast<SCH_SYMBOL*>( item );

            if( symbol->GetField( FIELD_T::VALUE )->GetText() == wxS( "LED_Dual_AAKK" ) )
                m_diode = symbol;
        }

        BOOST_REQUIRE( m_diode );
    }

    SCH_SHEET_PATH m_led0Path;
    SCH_SHEET_PATH m_led8Path;
    SCH_SYMBOL*    m_diode = nullptr;
};


/**
 * The diode's instances are stored with no project name, so keying the paste cleanup on the
 * project name discarded them and every sheet fell back to the same reference field.
 */
BOOST_FIXTURE_TEST_CASE( SharedScreenInstancesSurvivePaste, ISSUE21821_FIXTURE )
{
    // Without an empty project name on disk there is no bug left to reproduce
    for( const SCH_SYMBOL_INSTANCE& instance : m_diode->GetInstances() )
        BOOST_REQUIRE( instance.m_ProjectName.IsEmpty() );

    BOOST_REQUIRE_EQUAL( m_diode->GetRef( &m_led0Path ), wxS( "D3" ) );
    BOOST_REQUIRE_EQUAL( m_diode->GetRef( &m_led8Path ), wxS( "D33" ) );

    PrunePastedSymbolInstances( m_diode, *m_schematic );

    BOOST_CHECK_EQUAL( m_diode->GetRef( &m_led0Path ), wxS( "D3" ) );
    BOOST_CHECK_EQUAL( m_diode->GetRef( &m_led8Path ), wxS( "D33" ) );

    // Claiming the instances is what lets the orphan pruning that follows a paste see them
    for( const SCH_SYMBOL_INSTANCE& instance : m_diode->GetInstances() )
        BOOST_CHECK_EQUAL( instance.m_ProjectName, m_schematic->Project().GetProjectName() );
}


/**
 * Symbols whose instances already name the project must come through the cleanup untouched.
 */
BOOST_FIXTURE_TEST_CASE( ProjectOwnedInstancesUntouched, ISSUE21821_FIXTURE )
{
    size_t examined = 0;

    for( SCH_ITEM* item : m_led0Path.LastScreen()->Items().OfType( SCH_SYMBOL_T ) )
    {
        SCH_SYMBOL* symbol = static_cast<SCH_SYMBOL*>( item );

        if( symbol == m_diode )
            continue;

        size_t   countBefore = symbol->GetInstances().size();
        wxString led0Before = symbol->GetRef( &m_led0Path );
        wxString led8Before = symbol->GetRef( &m_led8Path );

        PrunePastedSymbolInstances( symbol, *m_schematic );

        BOOST_CHECK_EQUAL( symbol->GetInstances().size(), countBefore );
        BOOST_CHECK_EQUAL( symbol->GetRef( &m_led0Path ), led0Before );
        BOOST_CHECK_EQUAL( symbol->GetRef( &m_led8Path ), led8Before );
        examined++;
    }

    // The resistors are the whole point of this case, an empty walk would pass for free
    BOOST_CHECK( examined > 0 );
}


/**
 * A stored instance path may name the virtual root, which SCH_SHEET_PATH::Path() leaves out.
 * The cleanup has to read that form the way the s-expression writer does, or it condemns the
 * design's own annotation.
 */
BOOST_FIXTURE_TEST_CASE( VirtualRootInstancesSurvivePaste, ISSUE21821_FIXTURE )
{
    BOOST_REQUIRE( m_schematic->Root().m_Uuid == niluuid );

    std::vector<SCH_SYMBOL_INSTANCE> stored = m_diode->GetInstances();

    BOOST_REQUIRE_EQUAL( stored.size(), 2 );

    for( SCH_SYMBOL_INSTANCE& instance : stored )
    {
        KIID_PATH withVirtualRoot;
        withVirtualRoot.push_back( niluuid );

        for( const KIID& uuid : instance.m_Path )
            withVirtualRoot.push_back( uuid );

        m_diode->RemoveInstance( instance.m_Path );
        instance.m_Path = withVirtualRoot;
    }

    for( const SCH_SYMBOL_INSTANCE& instance : stored )
        m_diode->AddHierarchicalReference( instance );

    PrunePastedSymbolInstances( m_diode, *m_schematic );

    BOOST_REQUIRE_EQUAL( m_diode->GetInstances().size(), 2 );

    wxString led0Ref;
    wxString led8Ref;

    for( const SCH_SYMBOL_INSTANCE& instance : m_diode->GetInstances() )
    {
        // The stored path is left alone; only the ownership test normalizes it
        BOOST_CHECK( instance.m_Path.front() == niluuid );
        BOOST_CHECK_EQUAL( instance.m_ProjectName, m_schematic->Project().GetProjectName() );

        if( instance.m_Path.back() == m_led0Path.Path().back() )
            led0Ref = instance.m_Reference;
        else if( instance.m_Path.back() == m_led8Path.Path().back() )
            led8Ref = instance.m_Reference;
    }

    BOOST_CHECK_EQUAL( led0Ref, wxS( "D3" ) );
    BOOST_CHECK_EQUAL( led8Ref, wxS( "D33" ) );
}


/**
 * The cleanup still has to throw away what a paste really did drag in: paths rooted in another
 * design, and the zero length paths that crash on save (issue #16300).
 */
BOOST_FIXTURE_TEST_CASE( ForeignAndPathlessInstancesPruned, ISSUE21821_FIXTURE )
{
    SCH_SYMBOL_INSTANCE foreign;
    foreign.m_Path = KIID_PATH( wxS( "/2b1cb0b9-6a97-4b3a-9c9e-24d5f2a0a9d1"
                                     "/9d0f1e5c-4a6b-4f0d-8f4a-1c2d3e4f5a6b" ) );
    foreign.m_ProjectName = wxS( "some_other_project" );
    foreign.m_Reference = wxS( "D99" );
    foreign.m_Unit = 1;
    m_diode->AddHierarchicalReference( foreign );

    SCH_SYMBOL_INSTANCE pathless;
    pathless.m_ProjectName = m_schematic->Project().GetProjectName();
    pathless.m_Reference = wxS( "D98" );
    pathless.m_Unit = 1;
    m_diode->AddHierarchicalReference( pathless );

    BOOST_REQUIRE_EQUAL( m_diode->GetInstances().size(), 4 );

    PrunePastedSymbolInstances( m_diode, *m_schematic );

    BOOST_CHECK_EQUAL( m_diode->GetInstances().size(), 2 );

    for( const SCH_SYMBOL_INSTANCE& instance : m_diode->GetInstances() )
        BOOST_CHECK( instance.m_Path == m_led0Path.Path() || instance.m_Path == m_led8Path.Path() );
}
