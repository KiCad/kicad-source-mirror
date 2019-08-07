/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2018-2019 KiCad Developers, see CHANGELOG.TXT for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

/**
 * @file
 * Test suite for LIB_TABLE_BASE
 *
 * This test is of a abstract class, so we will implement a cut-down
 * version and only test the core logic. Tests of the concrete implementations's
 * own logic should be done in the relevant tests.
 */

#include <unit_test_utils/unit_test_utils.h>

// Code under test
#include <lib_table_base.h>


/**
 * A concrete implementation of #LIB_TABLE_ROW that implements
 * the minimum interface.
 */
class TEST_LIB_TABLE_ROW : public LIB_TABLE_ROW
{
public:
    TEST_LIB_TABLE_ROW( const wxString& aNick, const wxString& aURI, const wxString& aOptions,
            const wxString& aDescr )
            : LIB_TABLE_ROW( aNick, aURI, aOptions, aDescr )
    {
    }

    const wxString GetType() const override
    {
        return m_type;
    }

    void SetType( const wxString& aType ) override
    {
        m_type = aType;
    }

private:
    LIB_TABLE_ROW* do_clone() const override
    {
        return new TEST_LIB_TABLE_ROW( *this );
    }

    wxString m_type;
};


/**
 * A concrete implementation of #LIB_TABLE that implements
 * the minimum interface for testing.
 *
 * Notably, the Parse/Format functions are not used, as there is no "real"
 * format to read/write.
 */
class TEST_LIB_TABLE : public LIB_TABLE
{
public:
    TEST_LIB_TABLE( LIB_TABLE* aFallback = nullptr ) : LIB_TABLE( aFallback )
    {
    }

    KICAD_T Type() override // from _ELEM
    {
        // Doesn't really matter what this is
        return FP_LIB_TABLE_T;
    }

private:
    void Parse( LIB_TABLE_LEXER* aLexer ) override
    {
        // Do nothing, we won't parse anything. Parse testing of actual data
        // will happen in the relevant other tests.
    }

    void Format( OUTPUTFORMATTER* aOutput, int aIndentLevel ) const override
    {
        // do nothing, we don't need to test this function
    }
};


/**
 * Simple structure to contain data to set up a single #TEST_LIB_TABLE_ROW
 */
struct LIB_ROW_DEFINITION
{
    std::string m_nickname;
    std::string m_uri;
    std::string m_description;
    bool        m_enabled;
};


// clang-format off
/**
 * Set-up data for the re-used library row definitions.
 */
static const std::vector<LIB_ROW_DEFINITION> main_lib_defs = {
    {
        "Lib1",
        "://lib/1",
        "The first library",
        true,
    },
    {
        "Lib2",
        "://lib/2",
        "The second library",
        true,
    },
    {
        "Lib3",
        "://lib/3",
        "The third library",
        false,
    },
};

static const std::vector<LIB_ROW_DEFINITION> fallback_lib_defs = {
    {
        "FallbackLib1",
        "://lib/fb1",
        "The first fallback library",
        true,
    },
    {
        "FallbackLib2",
        "://lib/fb2",
        "The second fallback library",
        false,
    },
};
// clang-format on


/**
 * Reusable test fixture with some basic pre-filled tables.
 */
struct LIB_TABLE_TEST_FIXTURE
{
    LIB_TABLE_TEST_FIXTURE() : m_mainTableWithFb( &m_fallbackTable )
    {
        for( const auto& lib : main_lib_defs )
        {
            m_mainTableNoFb.InsertRow( makeRowFromDef( lib ).release() );
            m_mainTableWithFb.InsertRow( makeRowFromDef( lib ).release() );
        }

        for( const auto& lib : fallback_lib_defs )
        {
            m_fallbackTable.InsertRow( makeRowFromDef( lib ).release() );
        }
    }

    /**
     * Helper to construct a new #TEST_LIB_TABLE_ROW from a definition struct
     */
    std::unique_ptr<TEST_LIB_TABLE_ROW> makeRowFromDef( const LIB_ROW_DEFINITION& aDef )
    {
        auto row = std::make_unique<TEST_LIB_TABLE_ROW>(
                aDef.m_nickname, aDef.m_uri, "", aDef.m_description );

        row->SetEnabled( aDef.m_enabled );

        return row;
    }

    /// Table with some enabled and disabled libs, no fallback provided
    TEST_LIB_TABLE m_mainTableNoFb;

    /// Identical to m_mainTableNoFb, but with a fallback
    TEST_LIB_TABLE m_mainTableWithFb;

    /// The table that m_mainTableWithFb falls back to.
    TEST_LIB_TABLE m_fallbackTable;
};

/**
 * Declare the test suite
 */
BOOST_FIXTURE_TEST_SUITE( LibTable, LIB_TABLE_TEST_FIXTURE )

/**
 * Check an empty table behaves correctly
 */
BOOST_AUTO_TEST_CASE( Empty )
{
    TEST_LIB_TABLE table;

    // Tables start out empty
    BOOST_CHECK_EQUAL( table.GetCount(), 0 );
    BOOST_CHECK_EQUAL( true, table.IsEmpty() );
}


/**
 * Check size and emptiness on tables with fallback
 */
BOOST_AUTO_TEST_CASE( EmptyWithFallback )
{
    // Fall back though another empty table to the real fallback
    TEST_LIB_TABLE interposer_table( &m_fallbackTable );
    TEST_LIB_TABLE table( &interposer_table );

    // Table has no elements...
    BOOST_CHECK_EQUAL( table.GetCount(), 0 );

    // But it's not empty if we include the fallback
    BOOST_CHECK_EQUAL( false, table.IsEmpty( true ) );
}


/**
 * Check table clearing function
 */
BOOST_AUTO_TEST_CASE( Clear )
{
    m_mainTableNoFb.Clear();

    // Tables start out empty
    BOOST_CHECK_EQUAL( m_mainTableNoFb.GetCount(), 0 );
    BOOST_CHECK_EQUAL( true, m_mainTableNoFb.IsEmpty() );
}


/**
 * Check table equality function
 */
BOOST_AUTO_TEST_CASE( Equal )
{
    // writing a boot print is a bit of faff, so just use BOOST_CHECK_EQUAL and bools

    // These two are identical, except the fallback (which isn't checked)
    BOOST_CHECK_EQUAL( true, m_mainTableNoFb == m_mainTableWithFb );
    BOOST_CHECK_EQUAL( false, m_mainTableNoFb != m_mainTableWithFb );

    // Modify one of them
    m_mainTableWithFb.At( 1 ).SetNickName( "NewNickname" );
    BOOST_CHECK_EQUAL( false, m_mainTableNoFb == m_mainTableWithFb );
    BOOST_CHECK_EQUAL( true, m_mainTableNoFb != m_mainTableWithFb );

    // And check unequal (against empty)
    TEST_LIB_TABLE empty_table;
    BOOST_CHECK_EQUAL( false, m_mainTableNoFb == empty_table );
    BOOST_CHECK_EQUAL( true, m_mainTableNoFb != empty_table );
}


/**
 * Test indexing into the main table
 */
BOOST_AUTO_TEST_CASE( Indexing )
{
    // Filled with the right row count
    BOOST_CHECK_EQUAL( m_mainTableNoFb.GetCount(), 3 );

    const auto& row0 = m_mainTableNoFb.At( 0 );
    BOOST_CHECK_EQUAL( row0.GetNickName(), "Lib1" );

    const auto& row1 = m_mainTableNoFb.At( 1 );
    BOOST_CHECK_EQUAL( row1.GetNickName(), "Lib2" );

    // disable, but still in the index
    const auto& row2 = m_mainTableNoFb.At( 2 );
    BOOST_CHECK_EQUAL( row2.GetNickName(), "Lib3" );

    // check correct handling of out-of-bounds
    // TODO: this doesn't work with boost::ptr_vector - that only asserts
    // BOOST_CHECK_THROW( m_mainTableNoFb.At( 3 ), std::out_of_range );
}


/**
 * Test retrieval of libs by nickname
 */
BOOST_AUTO_TEST_CASE( HasLibrary )
{
    BOOST_CHECK_EQUAL( true, m_mainTableNoFb.HasLibrary( "Lib1" ) );

    // disabled lib can be "not found" if checkEnabled is set
    BOOST_CHECK_EQUAL( true, m_mainTableNoFb.HasLibrary( "Lib3" ) );
    BOOST_CHECK_EQUAL( false, m_mainTableNoFb.HasLibrary( "Lib3", true ) );

    BOOST_CHECK_EQUAL( false, m_mainTableNoFb.HasLibrary( "NotPresent" ) );
}


/**
 * Test retrieval of libs by nickname
 */
BOOST_AUTO_TEST_CASE( Descriptions )
{
    BOOST_CHECK_EQUAL( "The first library", m_mainTableNoFb.GetDescription( "Lib1" ) );

    // disabled lib works
    BOOST_CHECK_EQUAL( "The third library", m_mainTableNoFb.GetDescription( "Lib3" ) );
}


/**
 * Test retrieval of libs by URI
 */
BOOST_AUTO_TEST_CASE( URIs )
{
    BOOST_CHECK_EQUAL( "://lib/1", m_mainTableNoFb.GetFullURI( "Lib1" ) );

    const LIB_TABLE_ROW* row = m_mainTableNoFb.FindRowByURI( "://lib/1" );

    // should be found
    BOOST_CHECK_NE( nullptr, row );

    if( row )
    {
        BOOST_CHECK_EQUAL( "Lib1", row->GetNickName() );
    }

    row = m_mainTableNoFb.FindRowByURI( "this_uri_is_not_found" );

    BOOST_CHECK_EQUAL( nullptr, row );
}


/**
 * Test retrieval of the logical libs function
 */
BOOST_AUTO_TEST_CASE( LogicalLibs )
{
    auto logical_libs = m_mainTableNoFb.GetLogicalLibs();

    // The enabled library nicknames
    const std::vector<wxString> exp_libs = {
        "Lib1",
        "Lib2",
    };

    BOOST_CHECK_EQUAL_COLLECTIONS(
            logical_libs.begin(), logical_libs.end(), exp_libs.begin(), exp_libs.end() );
}

BOOST_AUTO_TEST_SUITE_END()
