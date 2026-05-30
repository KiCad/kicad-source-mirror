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
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <qa_utils/wx_utils/unit_test_utils.h>
#include "eeschema_test_utils.h"

#include <core/profile.h>
#include <sch_sheet_path.h>
#include <sch_symbol.h>
#include <wildcards_and_files_ext.h>


class TEST_ISSUE23020_FIXTURE : public KI_TEST::SCHEMATIC_TEST_FIXTURE
{
protected:
    wxFileName SchematicQAPath( const wxString& aRelativePath ) override
    {
        wxFileName fn( KI_TEST::GetEeschemaTestDataDir() );
        fn.SetName( aRelativePath );
        fn.SetExt( FILEEXT::KiCadSchematicFileExtension );

        return fn;
    }

    /// Reference implementation: resolve a symbol instance by a linear scan over the
    /// raw instance vector, mirroring the pre-index behaviour of the accessors.
    static const SCH_SYMBOL_INSTANCE* NaiveFindInstance( const SCH_SYMBOL* aSymbol,
                                                         const KIID_PATH&  aPath )
    {
        for( const SCH_SYMBOL_INSTANCE& instance : aSymbol->GetInstances() )
        {
            if( instance.m_Path == aPath )
                return &instance;
        }

        return nullptr;
    }
};


BOOST_FIXTURE_TEST_SUITE( Issue23020DeepHierarchy, TEST_ISSUE23020_FIXTURE )


// The optimization for issue #23020 replaces the per-lookup linear scan of a symbol's
// instance list with a path-keyed index.  The index must return exactly what the linear
// scan did for every symbol on every sheet path; this test pins that equivalence against
// the real (deep, duplicate-sheet) hierarchy from the issue.
BOOST_AUTO_TEST_CASE( InstanceLookupMatchesLinearScan )
{
    wxFileName fn( KI_TEST::GetEeschemaTestDataDir() );
    fn.AppendDir( "issue23020" );
    fn.SetName( "issue23020" );
    fn.SetExt( FILEEXT::KiCadSchematicFileExtension );

    if( !fn.FileExists() )
    {
        BOOST_TEST_MESSAGE( "Skipping test: issue23020 test data not found at " << fn.GetFullPath() );
        return;
    }

    PROF_TIMER loadTimer;

    LoadSchematic( fn );

    loadTimer.Stop();
    BOOST_TEST_MESSAGE( "Deep hierarchy load time: " << loadTimer.msecs() << " ms" );

    SCH_SHEET_LIST sheets = m_schematic->BuildSheetListSortedByPageNumbers();

    BOOST_TEST_MESSAGE( "Total sheet paths: " << sheets.size() );

    // The point of the issue is a deep hierarchy with many duplicated sheets.
    BOOST_CHECK( sheets.size() > 100 );

    size_t     symbolsChecked = 0;
    PROF_TIMER lookupTimer;

    for( const SCH_SHEET_PATH& sheet : sheets )
    {
        KIID_PATH path = sheet.Path();

        for( SCH_ITEM* item : sheet.LastScreen()->Items().OfType( SCH_SYMBOL_T ) )
        {
            SCH_SYMBOL* symbol = static_cast<SCH_SYMBOL*>( item );

            const SCH_SYMBOL_INSTANCE* expected = NaiveFindInstance( symbol, path );

            SCH_SYMBOL_INSTANCE indexed;
            bool                found = symbol->GetInstance( indexed, path );

            BOOST_CHECK_EQUAL( found, expected != nullptr );

            if( expected )
            {
                BOOST_CHECK( indexed.m_Path == expected->m_Path );
                BOOST_CHECK_EQUAL( indexed.m_Reference, expected->m_Reference );
                BOOST_CHECK_EQUAL( indexed.m_Unit, expected->m_Unit );

                // GetRef()/GetUnitSelection() must agree with the raw instance data.
                BOOST_CHECK_EQUAL( symbol->GetRef( &sheet, false ), expected->m_Reference );
                BOOST_CHECK_EQUAL( symbol->GetUnitSelection( &sheet ), expected->m_Unit );
            }

            symbolsChecked++;
        }
    }

    lookupTimer.Stop();

    BOOST_TEST_MESSAGE( "Verified " << symbolsChecked << " indexed lookups in "
                                    << lookupTimer.msecs() << " ms" );

    BOOST_CHECK( symbolsChecked > 0 );
}


// Guards the index-maintenance code paths (add/remove/rebuild) directly: after a sequence
// of mutations the index must still resolve every surviving path to the correct instance
// and must not resolve a removed path.  A desync (e.g. a missed rebuild after erase) breaks
// this even though the underlying data is intact.
BOOST_AUTO_TEST_CASE( IndexStaysConsistentThroughMutations )
{
    SCH_SYMBOL symbol;

    auto makePath = []( int aTag ) -> KIID_PATH
    {
        KIID_PATH path;
        path.push_back( KIID() );
        path.push_back( KIID() );
        path.push_back( KIID() );

        // Tag the last element deterministically so lookups are reproducible.
        path.back() = KIID( wxString::Format( "abcd1234-0000-0000-0000-%012d", aTag ) );

        return path;
    };

    std::vector<KIID_PATH> paths;

    for( int ii = 0; ii < 8; ++ii )
    {
        KIID_PATH path = makePath( ii );
        paths.push_back( path );
        symbol.AddHierarchicalReference( path, wxString::Format( "R%d", ii ), ii % 3 + 1 );
    }

    auto checkAll = [&]( const std::vector<size_t>& aLive )
    {
        for( size_t idx : aLive )
        {
            SCH_SYMBOL_INSTANCE instance;
            BOOST_CHECK( symbol.GetInstance( instance, paths[idx] ) );
            BOOST_CHECK( instance.m_Path == paths[idx] );
            BOOST_CHECK_EQUAL( instance.m_Reference, wxString::Format( "R%zu", idx ) );
            BOOST_CHECK_EQUAL( instance.m_Unit, (int) ( idx % 3 + 1 ) );
        }
    };

    checkAll( { 0, 1, 2, 3, 4, 5, 6, 7 } );

    // Remove a couple of interior entries; the index must be rebuilt so the survivors still
    // map to their (now shifted) positions.
    symbol.RemoveInstance( paths[2] );
    symbol.RemoveInstance( paths[5] );

    SCH_SYMBOL_INSTANCE gone;
    BOOST_CHECK( !symbol.GetInstance( gone, paths[2] ) );
    BOOST_CHECK( !symbol.GetInstance( gone, paths[5] ) );

    checkAll( { 0, 1, 3, 4, 6, 7 } );

    // Re-add a removed path with a new reference; it must resolve to the new value.
    symbol.AddHierarchicalReference( paths[2], "R2b", 2 );

    SCH_SYMBOL_INSTANCE readded;
    BOOST_CHECK( symbol.GetInstance( readded, paths[2] ) );
    BOOST_CHECK_EQUAL( readded.m_Reference, "R2b" );

    checkAll( { 0, 1, 3, 4, 6, 7 } );
}


BOOST_AUTO_TEST_SUITE_END()
