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
#include <settings/settings_manager.h>
#include <wildcards_and_files_ext.h>

#include <wx/file.h>
#include <wx/filename.h>
#include <wx/stdpaths.h>


using namespace KICAD_DIFF;


namespace
{

/// Write @p aContent into the sibling .kicad_dru of @p aBoard.  Returns the full
/// path of the written file so the test can later delete it.
wxString writeSiblingRules( BOARD& aBoard, const wxString& aContent )
{
    wxFileName fn( aBoard.GetFileName() );
    fn.SetExt( FILEEXT::DesignRulesFileExtension );

    wxFile f;
    BOOST_REQUIRE( f.Open( fn.GetFullPath(), wxFile::write ) );
    BOOST_REQUIRE( f.Write( aContent ) );
    f.Close();
    return fn.GetFullPath();
}


const ITEM_CHANGE* findDocLevelChange( const DOCUMENT_DIFF& aDiff )
{
    for( const ITEM_CHANGE& c : aDiff.changes )
    {
        if( c.id.empty() )
            return &c;
    }

    return nullptr;
}


const PROPERTY_DELTA* findProperty( const ITEM_CHANGE& aChange, const wxString& aName )
{
    for( const PROPERTY_DELTA& p : aChange.properties )
    {
        if( p.name == aName )
            return &p;
    }

    return nullptr;
}

} // namespace


struct CUSTOM_DRC_DIFF_MERGE_FIXTURE
{
    CUSTOM_DRC_DIFF_MERGE_FIXTURE()
    {
        KI_TEST::LoadBoard( m_settingsAnc, "complex_hierarchy", m_ancestor );
        KI_TEST::LoadBoard( m_settingsOurs, "complex_hierarchy", m_ours );
        KI_TEST::LoadBoard( m_settingsTheirs, "complex_hierarchy", m_theirs );

        BOOST_REQUIRE( m_ancestor );
        BOOST_REQUIRE( m_ours );
        BOOST_REQUIRE( m_theirs );

        // Each board's GetFileName() default points at the same fixture file.
        // Writing a sibling .kicad_dru would then collide across all three
        // sides.  Move each board to its own temp directory so the sibling
        // rule files live in distinct places.
        wxString tmp = wxStandardPaths::Get().GetTempDir();
        m_ancDir   = ( wxFileName( tmp, wxEmptyString ).GetPath() + wxFILE_SEP_PATH
                     + wxS( "kicad_drc_anc_" ) + uniqueSuffix() );
        m_oursDir  = ( wxFileName( tmp, wxEmptyString ).GetPath() + wxFILE_SEP_PATH
                     + wxS( "kicad_drc_ours_" ) + uniqueSuffix() );
        m_theirsDir = ( wxFileName( tmp, wxEmptyString ).GetPath() + wxFILE_SEP_PATH
                      + wxS( "kicad_drc_theirs_" ) + uniqueSuffix() );

        for( const wxString& d : { m_ancDir, m_oursDir, m_theirsDir } )
            BOOST_REQUIRE( wxFileName::Mkdir( d, wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL ) );

        m_ancestor->SetFileName( m_ancDir + wxFILE_SEP_PATH + wxS( "board.kicad_pcb" ) );
        m_ours    ->SetFileName( m_oursDir + wxFILE_SEP_PATH + wxS( "board.kicad_pcb" ) );
        m_theirs  ->SetFileName( m_theirsDir + wxFILE_SEP_PATH + wxS( "board.kicad_pcb" ) );
    }

    ~CUSTOM_DRC_DIFF_MERGE_FIXTURE()
    {
        for( const wxString& path : m_createdFiles )
        {
            if( wxFileExists( path ) )
                wxRemoveFile( path );
        }

        for( const wxString& d : { m_ancDir, m_oursDir, m_theirsDir } )
        {
            if( !d.IsEmpty() && wxFileName::DirExists( d ) )
                wxFileName::Rmdir( d, wxPATH_RMDIR_RECURSIVE );
        }
    }

    void recordCreatedFile( const wxString& aPath )
    {
        m_createdFiles.push_back( aPath );
    }

    static wxString uniqueSuffix()
    {
        static int counter = 0;
        return wxString::Format( wxS( "%d_%d" ),
                                 static_cast<int>( wxGetUTCTimeMillis().GetValue() & 0xffffff ),
                                 ++counter );
    }

    SETTINGS_MANAGER       m_settingsAnc;
    SETTINGS_MANAGER       m_settingsOurs;
    SETTINGS_MANAGER       m_settingsTheirs;
    std::unique_ptr<BOARD> m_ancestor;
    std::unique_ptr<BOARD> m_ours;
    std::unique_ptr<BOARD> m_theirs;
    std::vector<wxString>  m_createdFiles;
    wxString               m_ancDir;
    wxString               m_oursDir;
    wxString               m_theirsDir;
};


BOOST_FIXTURE_TEST_SUITE( CustomDrcDiffMerge, CUSTOM_DRC_DIFF_MERGE_FIXTURE )


// No sibling .kicad_dru file present on either side -> no DOC_PROP_CUSTOM_RULES
// delta in the diff.  This is the typical git-mergetool temp-blob case.
BOOST_AUTO_TEST_CASE( NoSiblingFileEmitsNoDelta )
{
    PCB_DIFFER differ( m_ancestor.get(), m_ours.get(), wxS( "complex_hierarchy.kicad_pcb" ) );
    DOCUMENT_DIFF result = differ.Diff();

    if( const ITEM_CHANGE* docChange = findDocLevelChange( result ) )
    {
        BOOST_CHECK( findProperty( *docChange, DOC_PROP_CUSTOM_RULES ) == nullptr );
    }
}


// Ours has a rules file, ancestor doesn't -> delta emitted with non-empty
// after-summary and "(no custom rules)" before-summary.
BOOST_AUTO_TEST_CASE( OursAddsRulesEmitsDelta )
{
    recordCreatedFile( writeSiblingRules( *m_ours,
                                          wxS( "(version 1)\n(rule \"R\" (constraint clearance "
                                               "(min 0.3mm)) (condition \"true\"))\n" ) ) );

    PCB_DIFFER differ( m_ancestor.get(), m_ours.get(), wxS( "complex_hierarchy.kicad_pcb" ) );
    DOCUMENT_DIFF result = differ.Diff();

    const ITEM_CHANGE* docChange = findDocLevelChange( result );
    BOOST_REQUIRE( docChange );

    const PROPERTY_DELTA* delta = findProperty( *docChange, DOC_PROP_CUSTOM_RULES );
    BOOST_REQUIRE( delta );

    BOOST_CHECK_EQUAL( delta->before.ToDisplayString(), "(no custom rules)" );
    BOOST_CHECK( delta->after.ToDisplayString() != "(no custom rules)" );
}


// Both sides have rules but with different content -> delta with distinct
// before / after summaries.
BOOST_AUTO_TEST_CASE( DivergentRulesContentEmitsDelta )
{
    recordCreatedFile( writeSiblingRules( *m_ancestor,
                                          wxS( "(version 1)\n(rule \"A\" (constraint clearance "
                                               "(min 0.1mm)) (condition \"true\"))\n" ) ) );
    recordCreatedFile( writeSiblingRules( *m_ours,
                                          wxS( "(version 1)\n(rule \"B\" (constraint clearance "
                                               "(min 0.3mm)) (condition \"true\"))\n" ) ) );

    PCB_DIFFER differ( m_ancestor.get(), m_ours.get(), wxS( "complex_hierarchy.kicad_pcb" ) );
    DOCUMENT_DIFF result = differ.Diff();

    const ITEM_CHANGE* docChange = findDocLevelChange( result );
    BOOST_REQUIRE( docChange );

    const PROPERTY_DELTA* delta = findProperty( *docChange, DOC_PROP_CUSTOM_RULES );
    BOOST_REQUIRE( delta );

    BOOST_CHECK( delta->before.ToDisplayString() != delta->after.ToDisplayString() );
}


// Identical rules content on both sides -> no delta.
BOOST_AUTO_TEST_CASE( IdenticalRulesContentEmitsNoDelta )
{
    const wxString sameContent =
            wxS( "(version 1)\n(rule \"R\" (constraint clearance (min 0.2mm)) "
                 "(condition \"true\"))\n" );

    recordCreatedFile( writeSiblingRules( *m_ancestor, sameContent ) );

    // Both BOARDs point at the same fixture file, so writeSiblingRules with the
    // ours BOARD overwrites the same .kicad_dru path.  Read the produced path
    // off ours so cleanup still works.
    recordCreatedFile( writeSiblingRules( *m_ours, sameContent ) );

    PCB_DIFFER differ( m_ancestor.get(), m_ours.get(), wxS( "complex_hierarchy.kicad_pcb" ) );
    DOCUMENT_DIFF result = differ.Diff();

    if( const ITEM_CHANGE* docChange = findDocLevelChange( result ) )
    {
        BOOST_CHECK( findProperty( *docChange, DOC_PROP_CUSTOM_RULES ) == nullptr );
    }
}


// Applier path: a one-sided rules change is staged on the report for the
// handler to write out.  Ours owns the divergence -> the engine routes to
// the side that has it.
BOOST_AUTO_TEST_CASE( ApplierStagesOursRulesOnReport )
{
    const wxString oursRules =
            wxS( "(version 1)\n(rule \"FromOurs\" (constraint clearance (min 0.5mm)) "
                 "(condition \"true\"))\n" );
    recordCreatedFile( writeSiblingRules( *m_ours, oursRules ) );

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
    BOOST_CHECK( applier.GetReport().customDrcRulesSet );
    BOOST_CHECK_EQUAL( applier.GetReport().customDrcRules, oursRules );
}


BOOST_AUTO_TEST_SUITE_END()
