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

wxString uniqueSuffix()
{
    static int counter = 0;
    return wxString::Format( wxS( "%d_%d" ),
                             static_cast<int>( wxGetUTCTimeMillis().GetValue() & 0xffffff ),
                             ++counter );
}


wxString writeProjectFile( BOARD& aBoard, const std::string& aFileName,
                            const wxString& aContent )
{
    wxFileName fn( aBoard.GetFileName() );
    fn.SetFullName( wxString::FromUTF8( aFileName ) );

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


struct LIB_TABLE_DIFF_MERGE_FIXTURE
{
    LIB_TABLE_DIFF_MERGE_FIXTURE()
    {
        KI_TEST::LoadBoard( m_settingsAnc, "complex_hierarchy", m_ancestor );
        KI_TEST::LoadBoard( m_settingsOurs, "complex_hierarchy", m_ours );
        KI_TEST::LoadBoard( m_settingsTheirs, "complex_hierarchy", m_theirs );

        BOOST_REQUIRE( m_ancestor );
        BOOST_REQUIRE( m_ours );
        BOOST_REQUIRE( m_theirs );

        wxString tmp = wxStandardPaths::Get().GetTempDir();
        m_ancDir    = tmp + wxFILE_SEP_PATH + wxS( "kicad_libtbl_anc_" ) + uniqueSuffix();
        m_oursDir   = tmp + wxFILE_SEP_PATH + wxS( "kicad_libtbl_ours_" ) + uniqueSuffix();
        m_theirsDir = tmp + wxFILE_SEP_PATH + wxS( "kicad_libtbl_theirs_" ) + uniqueSuffix();

        for( const wxString& d : { m_ancDir, m_oursDir, m_theirsDir } )
            BOOST_REQUIRE( wxFileName::Mkdir( d, wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL ) );

        m_ancestor->SetFileName( m_ancDir + wxFILE_SEP_PATH + wxS( "board.kicad_pcb" ) );
        m_ours    ->SetFileName( m_oursDir + wxFILE_SEP_PATH + wxS( "board.kicad_pcb" ) );
        m_theirs  ->SetFileName( m_theirsDir + wxFILE_SEP_PATH + wxS( "board.kicad_pcb" ) );
    }

    ~LIB_TABLE_DIFF_MERGE_FIXTURE()
    {
        for( const wxString& d : { m_ancDir, m_oursDir, m_theirsDir } )
        {
            if( !d.IsEmpty() && wxFileName::DirExists( d ) )
                wxFileName::Rmdir( d, wxPATH_RMDIR_RECURSIVE );
        }
    }

    SETTINGS_MANAGER       m_settingsAnc;
    SETTINGS_MANAGER       m_settingsOurs;
    SETTINGS_MANAGER       m_settingsTheirs;
    std::unique_ptr<BOARD> m_ancestor;
    std::unique_ptr<BOARD> m_ours;
    std::unique_ptr<BOARD> m_theirs;
    wxString               m_ancDir;
    wxString               m_oursDir;
    wxString               m_theirsDir;
};


BOOST_FIXTURE_TEST_SUITE( LibTableDiffMerge, LIB_TABLE_DIFF_MERGE_FIXTURE )


BOOST_AUTO_TEST_CASE( NoSiblingFilesEmitsNoDelta )
{
    PCB_DIFFER differ( m_ancestor.get(), m_ours.get(), wxS( "complex_hierarchy.kicad_pcb" ) );
    DOCUMENT_DIFF result = differ.Diff();

    if( const ITEM_CHANGE* docChange = findDocLevelChange( result ) )
    {
        BOOST_CHECK( findProperty( *docChange, DOC_PROP_FP_LIB_TABLE ) == nullptr );
        BOOST_CHECK( findProperty( *docChange, DOC_PROP_SYM_LIB_TABLE ) == nullptr );
    }
}


BOOST_AUTO_TEST_CASE( OursAddsFpLibTableEmitsDelta )
{
    writeProjectFile( *m_ours, FILEEXT::FootprintLibraryTableFileName,
                      wxS( "(fp_lib_table\n(version 7)\n)\n" ) );

    PCB_DIFFER differ( m_ancestor.get(), m_ours.get(), wxS( "complex_hierarchy.kicad_pcb" ) );
    DOCUMENT_DIFF result = differ.Diff();

    const ITEM_CHANGE* docChange = findDocLevelChange( result );
    BOOST_REQUIRE( docChange );

    const PROPERTY_DELTA* delta = findProperty( *docChange, DOC_PROP_FP_LIB_TABLE );
    BOOST_REQUIRE( delta );
    BOOST_CHECK_EQUAL( delta->before.ToDisplayString(), "(no fp-lib-table)" );
    BOOST_CHECK( delta->after.ToDisplayString() != "(no fp-lib-table)" );
}


BOOST_AUTO_TEST_CASE( OursAddsSymLibTableEmitsDelta )
{
    writeProjectFile( *m_ours, FILEEXT::SymbolLibraryTableFileName,
                      wxS( "(sym_lib_table\n(version 7)\n)\n" ) );

    PCB_DIFFER differ( m_ancestor.get(), m_ours.get(), wxS( "complex_hierarchy.kicad_pcb" ) );
    DOCUMENT_DIFF result = differ.Diff();

    const ITEM_CHANGE* docChange = findDocLevelChange( result );
    BOOST_REQUIRE( docChange );

    const PROPERTY_DELTA* delta = findProperty( *docChange, DOC_PROP_SYM_LIB_TABLE );
    BOOST_REQUIRE( delta );
    BOOST_CHECK_EQUAL( delta->before.ToDisplayString(), "(no sym-lib-table)" );
    BOOST_CHECK( delta->after.ToDisplayString() != "(no sym-lib-table)" );
}


BOOST_AUTO_TEST_CASE( DivergentFpLibTableEmitsDelta )
{
    writeProjectFile( *m_ancestor, FILEEXT::FootprintLibraryTableFileName,
                      wxS( "(fp_lib_table\n(version 7)\n(lib (name old))\n)\n" ) );
    writeProjectFile( *m_ours, FILEEXT::FootprintLibraryTableFileName,
                      wxS( "(fp_lib_table\n(version 7)\n(lib (name new))\n)\n" ) );

    PCB_DIFFER differ( m_ancestor.get(), m_ours.get(), wxS( "complex_hierarchy.kicad_pcb" ) );
    DOCUMENT_DIFF result = differ.Diff();

    const ITEM_CHANGE* docChange = findDocLevelChange( result );
    BOOST_REQUIRE( docChange );

    const PROPERTY_DELTA* delta = findProperty( *docChange, DOC_PROP_FP_LIB_TABLE );
    BOOST_REQUIRE( delta );
    BOOST_CHECK( delta->before.ToDisplayString() != delta->after.ToDisplayString() );
}


BOOST_AUTO_TEST_CASE( ApplierStagesOursFpLibTable )
{
    const wxString oursFp =
            wxS( "(fp_lib_table\n(version 7)\n(lib (name FromOurs))\n)\n" );
    writeProjectFile( *m_ours, FILEEXT::FootprintLibraryTableFileName, oursFp );

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
    BOOST_CHECK( applier.GetReport().fpLibTableSet );
    BOOST_CHECK_EQUAL( applier.GetReport().fpLibTable, oursFp );
}


BOOST_AUTO_TEST_CASE( ApplierStagesOursSymLibTable )
{
    const wxString oursSym =
            wxS( "(sym_lib_table\n(version 7)\n(lib (name FromOurs))\n)\n" );
    writeProjectFile( *m_ours, FILEEXT::SymbolLibraryTableFileName, oursSym );

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
    BOOST_CHECK( applier.GetReport().symLibTableSet );
    BOOST_CHECK_EQUAL( applier.GetReport().symLibTable, oursSym );
}


BOOST_AUTO_TEST_SUITE_END()
