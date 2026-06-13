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
#include <diff_merge/kicad_diff_types.h>
#include <diff_merge/kicad_merge_engine.h>
#include "../../../pcbnew/diff_merge/pcb_merge_applier.h"

#include <board.h>
#include <board_design_settings.h>
#include <project/net_settings.h>
#include <netclass.h>
#include <settings/settings_manager.h>


using namespace KICAD_DIFF;


/**
 * Fixture: three independent loads of the same canonical board, used to play
 * the role of ancestor / ours / theirs in 3-way merge tests.  Each load gets
 * its own SETTINGS_MANAGER so the NET_SETTINGS instances are distinct, which
 * matters because the applier's CopyFrom path must NOT alias the source
 * shared_ptr.
 */
struct NET_CLASSES_DIFF_MERGE_FIXTURE
{
    NET_CLASSES_DIFF_MERGE_FIXTURE()
    {
        KI_TEST::LoadBoard( m_settingsAnc, "complex_hierarchy", m_ancestor );
        KI_TEST::LoadBoard( m_settingsOurs, "complex_hierarchy", m_ours );
        KI_TEST::LoadBoard( m_settingsTheirs, "complex_hierarchy", m_theirs );

        BOOST_REQUIRE( m_ancestor );
        BOOST_REQUIRE( m_ours );
        BOOST_REQUIRE( m_theirs );
        BOOST_REQUIRE( m_ancestor->GetDesignSettings().m_NetSettings );
        BOOST_REQUIRE( m_ours->GetDesignSettings().m_NetSettings );
        BOOST_REQUIRE( m_theirs->GetDesignSettings().m_NetSettings );
    }

    SETTINGS_MANAGER       m_settingsAnc;
    SETTINGS_MANAGER       m_settingsOurs;
    SETTINGS_MANAGER       m_settingsTheirs;
    std::unique_ptr<BOARD> m_ancestor;
    std::unique_ptr<BOARD> m_ours;
    std::unique_ptr<BOARD> m_theirs;
};


/// Find the synthetic doc-level ITEM_CHANGE (empty KIID_PATH) in a diff result,
/// returning nullptr if none was emitted.
static const ITEM_CHANGE* findDocLevelChange( const DOCUMENT_DIFF& aDiff )
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


BOOST_FIXTURE_TEST_SUITE( NetClassesDiffMerge, NET_CLASSES_DIFF_MERGE_FIXTURE )


// Two fresh loads should produce no net-classes delta, because the persisted
// NET_SETTINGS content is identical.
BOOST_AUTO_TEST_CASE( NoChangeEmitsNoNetClassesDelta )
{
    PCB_DIFFER differ( m_ancestor.get(), m_ours.get(), wxS( "complex_hierarchy.kicad_pcb" ) );
    DOCUMENT_DIFF result = differ.Diff();

    if( const ITEM_CHANGE* docChange = findDocLevelChange( result ) )
    {
        BOOST_CHECK( findProperty( *docChange, DOC_PROP_NET_CLASSES ) == nullptr );
    }
}


// An edit to the default netclass's clearance must surface as a
// DOC_PROP_NET_CLASSES delta with different before / after summary strings.
BOOST_AUTO_TEST_CASE( DefaultClearanceEditEmitsDelta )
{
    m_ours->GetDesignSettings().m_NetSettings->GetDefaultNetclass()->SetClearance( 350000 );

    PCB_DIFFER differ( m_ancestor.get(), m_ours.get(), wxS( "complex_hierarchy.kicad_pcb" ) );
    DOCUMENT_DIFF result = differ.Diff();

    const ITEM_CHANGE* docChange = findDocLevelChange( result );
    BOOST_REQUIRE( docChange );

    const PROPERTY_DELTA* delta = findProperty( *docChange, DOC_PROP_NET_CLASSES );
    BOOST_REQUIRE( delta );

    BOOST_CHECK( delta->before.ToDisplayString() != delta->after.ToDisplayString() );
}


// Adding a new named netclass on one side must surface as a delta.
BOOST_AUTO_TEST_CASE( NewNamedNetclassEmitsDelta )
{
    std::shared_ptr<NETCLASS> hs = std::make_shared<NETCLASS>( wxS( "HighSpeed" ), false );
    hs->SetClearance( 150000 );
    hs->SetTrackWidth( 127000 );

    std::map<wxString, std::shared_ptr<NETCLASS>> classes =
            m_ours->GetDesignSettings().m_NetSettings->GetNetclasses();
    classes[wxS( "HighSpeed" )] = hs;
    m_ours->GetDesignSettings().m_NetSettings->SetNetclasses( classes );

    PCB_DIFFER differ( m_ancestor.get(), m_ours.get(), wxS( "complex_hierarchy.kicad_pcb" ) );
    DOCUMENT_DIFF result = differ.Diff();

    const ITEM_CHANGE* docChange = findDocLevelChange( result );
    BOOST_REQUIRE( docChange );

    const PROPERTY_DELTA* delta = findProperty( *docChange, DOC_PROP_NET_CLASSES );
    BOOST_REQUIRE( delta );

    BOOST_CHECK( delta->before.ToDisplayString() != delta->after.ToDisplayString() );
}


// Applier path: a TAKE_OURS resolution of DOC_PROP_NET_CLASSES via MERGE_PROPS
// copies ours' net settings into result without aliasing ours' shared_ptr.
// Editing ours after Apply must NOT bleed through into the merged board.
BOOST_AUTO_TEST_CASE( ApplierTakeOursCopiesWithoutAliasing )
{
    m_ours->GetDesignSettings().m_NetSettings->GetDefaultNetclass()->SetClearance( 350000 );

    PCB_DIFFER diffAO( m_ancestor.get(), m_ours.get(), wxS( "complex_hierarchy.kicad_pcb" ) );
    PCB_DIFFER diffAT( m_ancestor.get(), m_theirs.get(), wxS( "complex_hierarchy.kicad_pcb" ) );
    DOCUMENT_DIFF docAO = diffAO.Diff();
    DOCUMENT_DIFF docAT = diffAT.Diff();

    KICAD_MERGE_ENGINE engine;
    MERGE_PLAN         plan = engine.Plan( docAO, docAT );

    PCB_MERGE_APPLIER      applier( m_ancestor.get(), m_ours.get(), m_theirs.get(),
                                    std::move( plan ) );
    std::unique_ptr<BOARD> merged = applier.Apply();

    BOOST_REQUIRE( merged );
    BOOST_REQUIRE( merged->GetDesignSettings().m_NetSettings );
    BOOST_CHECK( applier.GetReport().projectFileTouched );

    BOOST_CHECK_EQUAL(
            merged->GetDesignSettings().m_NetSettings->GetDefaultNetclass()->GetClearance(),
            350000 );

    // Pointer-distinct from ours: detach + CopyFrom path must produce a
    // standalone NET_SETTINGS instance.
    BOOST_CHECK( merged->GetDesignSettings().m_NetSettings.get()
                 != m_ours->GetDesignSettings().m_NetSettings.get() );

    // Mutating ours after Apply must not affect merged.
    m_ours->GetDesignSettings().m_NetSettings->GetDefaultNetclass()->SetClearance( 999999 );

    BOOST_CHECK_EQUAL(
            merged->GetDesignSettings().m_NetSettings->GetDefaultNetclass()->GetClearance(),
            350000 );
}


// Applier path: a TAKE_THEIRS-equivalent resolution copies theirs' settings.
// The engine routes one-sided edits to the side that made them, so editing
// only theirs and planning gives a theirs-side resolution.
BOOST_AUTO_TEST_CASE( ApplierTakeTheirsCopiesTheirsSettings )
{
    m_theirs->GetDesignSettings().m_NetSettings->GetDefaultNetclass()->SetTrackWidth( 300000 );

    PCB_DIFFER diffAO( m_ancestor.get(), m_ours.get(), wxS( "complex_hierarchy.kicad_pcb" ) );
    PCB_DIFFER diffAT( m_ancestor.get(), m_theirs.get(), wxS( "complex_hierarchy.kicad_pcb" ) );
    DOCUMENT_DIFF docAO = diffAO.Diff();
    DOCUMENT_DIFF docAT = diffAT.Diff();

    KICAD_MERGE_ENGINE engine;
    MERGE_PLAN         plan = engine.Plan( docAO, docAT );

    PCB_MERGE_APPLIER      applier( m_ancestor.get(), m_ours.get(), m_theirs.get(),
                                    std::move( plan ) );
    std::unique_ptr<BOARD> merged = applier.Apply();

    BOOST_REQUIRE( merged );
    BOOST_CHECK( applier.GetReport().projectFileTouched );
    BOOST_CHECK_EQUAL(
            merged->GetDesignSettings().m_NetSettings->GetDefaultNetclass()->GetTrackWidth(),
            300000 );
}


BOOST_AUTO_TEST_SUITE_END()
