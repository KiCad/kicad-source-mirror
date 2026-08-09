/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

/**
 * @file
 * Test suite for LIB_SYMBOL
 */

#include <qa_utils/wx_utils/unit_test_utils.h>

// Code under test
#include <sch_shape.h>
#include <sch_pin.h>
#include <lib_symbol.h>
#include <sch_file_versions.h>
#include <sch_io/sch_io.h>
#include <sch_io/sch_io_mgr.h>

#include <wx/file.h>
#include <wx/filename.h>

#include "lib_field_test_utils.h"

class TEST_LIB_SYMBOL_FIXTURE
{
public:
    TEST_LIB_SYMBOL_FIXTURE() :
        m_part_no_data( "part_name", nullptr )
    {
    }

    ///< Part with no extra data set
    LIB_SYMBOL m_part_no_data;
};


/**
 * A temporary symbol library that is removed when it goes out of scope.
 */
class SCOPED_TEMP_LIB
{
public:
    SCOPED_TEMP_LIB()
    {
        m_dir = wxFileName::CreateTempFileName( wxS( "kicad_lib_part_" ) );
        wxRemoveFile( m_dir );
        wxFileName::Mkdir( m_dir );

        m_path = wxFileName( m_dir, wxS( "test_lib.kicad_sym" ) ).GetFullPath();
    }

    ~SCOPED_TEMP_LIB()
    {
        if( wxFileName::DirExists( m_dir ) )
            wxFileName::Rmdir( m_dir, wxPATH_RMDIR_RECURSIVE );
    }

    const wxString& GetPath() const { return m_path; }

private:
    wxString m_dir;
    wxString m_path;
};


/**
 * Return the number of draw items belonging to a body style beyond the standard one.
 */
static int alternateBodyStyleItemCount( LIB_SYMBOL& aSymbol )
{
    int count = 0;

    for( SCH_ITEM& item : aSymbol.GetDrawItems() )
    {
        if( item.GetBodyStyle() > BODY_STYLE::BASE )
            count++;
    }

    return count;
}


/**
 * Load 4001, a four unit NOR gate carrying a De Morgan alternate body style.
 */
static std::unique_ptr<LIB_SYMBOL> loadDeMorganSymbol()
{
    wxFileName libPath( KI_TEST::GetEeschemaTestDataDir() );
    libPath.AppendDir( "libs" );
    libPath.SetFullName( "4xxx.kicad_sym" );

    IO_RELEASER<SCH_IO> pi( SCH_IO_MGR::FindPlugin( SCH_IO_MGR::SCH_KICAD ) );

    // The plugin cache owns the returned symbol, so hand back a copy
    LIB_SYMBOL* cached = pi->LoadSymbol( libPath.GetFullPath(), wxS( "4001" ) );

    return cached ? std::make_unique<LIB_SYMBOL>( *cached ) : nullptr;
}


/**
 * Write a symbol to its own library file.
 */
static void saveToLib( const wxString& aLibPath, const LIB_SYMBOL& aSymbol )
{
    IO_RELEASER<SCH_IO> pi( SCH_IO_MGR::FindPlugin( SCH_IO_MGR::SCH_KICAD ) );

    pi->CreateLibrary( aLibPath );
    pi->SaveSymbol( aLibPath, new LIB_SYMBOL( aSymbol ) );
    pi->SaveLibrary( aLibPath );
}


/**
 * Declare the test suite
 */
BOOST_FIXTURE_TEST_SUITE( LibPart, TEST_LIB_SYMBOL_FIXTURE )


/**
 * Check that we can get the basic properties out as expected
 */
BOOST_AUTO_TEST_CASE( DefaultProperties )
{
    BOOST_CHECK_EQUAL( m_part_no_data.GetName(), "part_name" );

    // Didn't set a library, so this is empty
    BOOST_CHECK_EQUAL( m_part_no_data.GetLibraryName(), "" );
    BOOST_CHECK_EQUAL( m_part_no_data.GetLib(), nullptr );

    // only get the root
    BOOST_CHECK_EQUAL( m_part_no_data.IsRoot(), true );
    BOOST_CHECK_EQUAL( m_part_no_data.IsDerived(), false );
    BOOST_CHECK_EQUAL( m_part_no_data.SharedPtr().use_count(), 2 );

    // no sub units
    BOOST_CHECK_EQUAL( m_part_no_data.GetUnitCount(), 1 );
    BOOST_CHECK_EQUAL( m_part_no_data.IsMultiUnit(), false );

    // single body style
    BOOST_CHECK_EQUAL( m_part_no_data.HasDeMorganBodyStyles(), false );
}


/**
 * Check the drawings on a "blank" LIB_SYMBOL
 */
BOOST_AUTO_TEST_CASE( DefaultDrawings )
{
    // default drawings exist
    BOOST_CHECK_EQUAL( m_part_no_data.GetDrawItems().size(), 5 );
    BOOST_CHECK_EQUAL( m_part_no_data.GetGraphicalPins( 0, 0 ).size(), 0 );
}


/**
 * Check the default fields are present as expected
 */
BOOST_AUTO_TEST_CASE( DefaultFields )
{
    std::vector<SCH_FIELD> fields;
    m_part_no_data.CopyFields( fields );

    // Should get the 4 default fields
    BOOST_CHECK_PREDICATE( KI_TEST::AreDefaultFieldsCorrect, ( fields ) );

    // but no more (we didn't set them)
    BOOST_CHECK_EQUAL( fields.size(), 5 );

    // also check the default field accessors
    BOOST_CHECK_PREDICATE( KI_TEST::FieldNameIdMatches,
            ( m_part_no_data.GetReferenceField() )( "Reference" )( (int) FIELD_T::REFERENCE ) );
    BOOST_CHECK_PREDICATE( KI_TEST::FieldNameIdMatches,
            ( m_part_no_data.GetValueField() )( "Value" )( (int) FIELD_T::VALUE ) );
    BOOST_CHECK_PREDICATE( KI_TEST::FieldNameIdMatches,
            ( m_part_no_data.GetFootprintField() )( "Footprint" )( (int) FIELD_T::FOOTPRINT ) );
    BOOST_CHECK_PREDICATE( KI_TEST::FieldNameIdMatches,
            ( m_part_no_data.GetDatasheetField() )( "Datasheet" )( (int) FIELD_T::DATASHEET ) );
    BOOST_CHECK_PREDICATE( KI_TEST::FieldNameIdMatches,
            ( m_part_no_data.GetDescriptionField() )( "Description" )( (int) FIELD_T::DESCRIPTION ) );
}


/**
 * Test adding fields to a LIB_SYMBOL
 */
BOOST_AUTO_TEST_CASE( AddedFields )
{
    std::vector<SCH_FIELD> fields;
    m_part_no_data.CopyFields( fields );

    // Ctor takes non-const ref (?!)
    const std::string newFieldName = "new_field";
    wxString          nonConstNewFieldName = newFieldName;
    fields.push_back( SCH_FIELD( nullptr, FIELD_T::USER, nonConstNewFieldName ) );

    // fairly roundabout way to add a field, but it is what it is
    m_part_no_data.SetFields( fields );

    // Should get the 4 default fields
    BOOST_CHECK_PREDICATE( KI_TEST::AreDefaultFieldsCorrect, ( fields ) );

    // and our new one
    BOOST_REQUIRE_EQUAL( fields.size(), 6 );

    // Check by-name lookup

    SCH_FIELD* gotNewField = m_part_no_data.GetField( newFieldName );

    BOOST_REQUIRE_NE( gotNewField, nullptr );
    BOOST_CHECK_PREDICATE( KI_TEST::FieldNameIdMatches, ( *gotNewField )( newFieldName )( 0 ) );
}


/**
 * Test adding draw items to a LIB_SYMBOL
 */
BOOST_AUTO_TEST_CASE( AddedDrawItems )
{
    const size_t defaultCount = m_part_no_data.GetDrawItems().size();

    SCH_PIN* pin = new SCH_PIN( &m_part_no_data );
    pin->SetNumber( "1" );
    m_part_no_data.AddDrawItem( pin );

    BOOST_CHECK_EQUAL( m_part_no_data.GetDrawItems().size(), defaultCount + 1 );
    BOOST_CHECK( pin->GetParentSymbol() == &m_part_no_data );
    BOOST_CHECK_EQUAL( m_part_no_data.GetPinCount(), 1 );

    m_part_no_data.RemoveDrawItem( pin );

    BOOST_CHECK_EQUAL( m_part_no_data.GetDrawItems().size(), defaultCount );
    BOOST_CHECK_EQUAL( m_part_no_data.GetPinCount(), 0 );

    // Mandatory fields are never removable, so the accessors can't be left dangling
    m_part_no_data.RemoveDrawItem( &m_part_no_data.GetReferenceField() );

    BOOST_CHECK_EQUAL( m_part_no_data.GetDrawItems().size(), defaultCount );
}


/**
 * A stacked pin is one drawn pin standing for several numbered contacts, written as a
 * bracketed list.  Both the list itself and each contact it names are pins of the symbol.
 */
BOOST_AUTO_TEST_CASE( StackedPinNumberLookup )
{
    SCH_PIN* pin = new SCH_PIN( &m_part_no_data );
    pin->SetNumber( "[A1,A12,B1,B12]" );
    m_part_no_data.AddDrawItem( pin );

    BOOST_CHECK( m_part_no_data.HasPinNumber( "[A1,A12,B1,B12]" ) );

    BOOST_CHECK( m_part_no_data.HasPinNumber( "A1" ) );
    BOOST_CHECK( m_part_no_data.HasPinNumber( "A12" ) );
    BOOST_CHECK( m_part_no_data.HasPinNumber( "B1" ) );
    BOOST_CHECK( m_part_no_data.HasPinNumber( "B12" ) );

    BOOST_CHECK( !m_part_no_data.HasPinNumber( "A2" ) );
    BOOST_CHECK( !m_part_no_data.HasPinNumber( "[A1" ) );
}


/**
 * The same, for range notation.  The bracketed form must keep matching: it is the spelling
 * that works today and is stored in existing symbols.
 */
BOOST_AUTO_TEST_CASE( RangeStackedPinNumberLookup )
{
    SCH_PIN* pin = new SCH_PIN( &m_part_no_data );
    pin->SetNumber( "[1-4]" );
    m_part_no_data.AddDrawItem( pin );

    BOOST_CHECK( m_part_no_data.HasPinNumber( "[1-4]" ) );

    BOOST_CHECK( m_part_no_data.HasPinNumber( "1" ) );
    BOOST_CHECK( m_part_no_data.HasPinNumber( "3" ) );

    BOOST_CHECK( !m_part_no_data.HasPinNumber( "5" ) );
}


struct TEST_LIB_SYMBOL_SUBREF_CASE
{
    int         m_index;
    bool        m_addSep;
    std::string m_expSubRef;
};


/**
 * Test the subreference indexing
 */
BOOST_AUTO_TEST_CASE( SubReference )
{
    const std::vector<TEST_LIB_SYMBOL_SUBREF_CASE> cases = {
        {
            1,
            false,
            "A",
        },
        {
            2,
            false,
            "B",
        },
        {
            26,
            false,
            "Z",
        },
        {
            27,
            false,
            "AA",
        },
        {
            28,
            false,
            "AB",
        },
        {
            53,
            false,
            "BA",
        },
        {
            79,
            false,
            "CA",
        },
        {
            105,
            false,
            "DA",
        },
        {
            131,
            false,
            "EA",
        },
        {
            157,
            false,
            "FA",
        },
        {
            183,
            false,
            "GA",
        },
        {
            209,
            false,
            "HA",
        },
        {
            235,
            false,
            "IA",
        },
        {
            261,
            false,
            "JA",
        },
        {
            287,
            false,
            "KA",
        },
        {
            313,
            false,
            "LA",
        },
        {
            339,
            false,
            "MA",
        },
        {
            365,
            false,
            "NA",
        },
        {
            391,
            false,
            "OA",
        },
        {
            417,
            false,
            "PA",
        },
        {
            443,
            false,
            "QA",
        },
        {
            469,
            false,
            "RA",
        },
        {
            495,
            false,
            "SA",
        },
        {
            521,
            false,
            "TA",
        },
        {
            547,
            false,
            "UA",
        },
        {
            573,
            false,
            "VA",
        },
        {
            599,
            false,
            "WA",
        },
        {
            625,
            false,
            "XA",
        },
        {
            651,
            false,
            "YA",
        },
        {
            677,
            false,
            "ZA",
        },
        {
            702,
            false,
            "ZZ",
        },
        {
            703,
            false,
            "AAA",
        },
        {
            728,
            false,
            "AAZ",
        },
    };

    for( const auto& c : cases )
    {
        BOOST_TEST_CONTEXT(
                "Subref: " << c.m_index << ", " << c.m_addSep << " -> '" << c.m_expSubRef << "'" )
        {
            const auto subref = LIB_SYMBOL::LetterSubReference( c.m_index, 'A' );
            BOOST_CHECK_EQUAL( subref, c.m_expSubRef );
        }
    }
}


/**
 * Check the compare method.
 */
BOOST_AUTO_TEST_CASE( Compare )
{
    // Identical root part to m_part_no_data sans time stamp.
    LIB_SYMBOL testPart( "part_name" );

    // Self comparison test.
    BOOST_CHECK_EQUAL( m_part_no_data.Compare( m_part_no_data ), 0 );

    // Test for identical LIB_SYMBOL.
    BOOST_CHECK_EQUAL( m_part_no_data.Compare( testPart, ~SCH_ITEM::COMPARE_FLAGS::UUID ), 0 );

    // Test name.
    testPart.SetName( "tart_name" );
    BOOST_CHECK( m_part_no_data.Compare( testPart ) < 0 );
    testPart.SetName( "cart_name" );
    BOOST_CHECK( m_part_no_data.Compare( testPart ) > 0 );
    testPart.SetName( "part_name" );

    // LIB_ID comparison tests.
    LIB_ID id = testPart.GetLibId();
    id.SetLibItemName( "tart_name" );
    testPart.SetLibId( id );
    BOOST_CHECK( m_part_no_data.Compare( testPart ) < 0 );
    id.SetLibItemName( "cart_name" );
    testPart.SetLibId( id );
    BOOST_CHECK( m_part_no_data.Compare( testPart ) > 0 );
    id.SetLibItemName( "part_name" );
    testPart.SetLibId( id );

    // Unit count comparison tests.
    testPart.SetUnitCount( 2, true );
    BOOST_CHECK( m_part_no_data.Compare( testPart ) < 0 );
    testPart.SetUnitCount( 1, true );
    m_part_no_data.SetUnitCount( 2, true );
    BOOST_CHECK( m_part_no_data.Compare( testPart ) > 0 );
    m_part_no_data.SetUnitCount( 1, true );

    // Options flag comparison tests.
    testPart.SetGlobalPower();
    BOOST_CHECK( m_part_no_data.Compare( testPart ) < 0 );
    testPart.SetNormal();
    m_part_no_data.SetGlobalPower();
    BOOST_CHECK( m_part_no_data.Compare( testPart ) > 0 );
    m_part_no_data.SetNormal();

    // Draw item list size comparison tests.
    testPart.AddDrawItem( new SCH_SHAPE( SHAPE_T::RECTANGLE, LAYER_DEVICE ) );
    m_part_no_data.AddDrawItem( new SCH_SHAPE( SHAPE_T::RECTANGLE, LAYER_DEVICE ) );
    BOOST_CHECK_EQUAL( m_part_no_data.Compare( testPart, ~SCH_ITEM::COMPARE_FLAGS::UUID ), 0 );
    m_part_no_data.RemoveDrawItem( &m_part_no_data.GetDrawItems()[SCH_SHAPE_T].front() );
    BOOST_CHECK( m_part_no_data.Compare( testPart, ~SCH_ITEM::COMPARE_FLAGS::UUID ) < 0 );
    testPart.RemoveDrawItem( &testPart.GetDrawItems()[SCH_SHAPE_T].front() );
    m_part_no_data.AddDrawItem( new SCH_SHAPE( SHAPE_T::RECTANGLE, LAYER_DEVICE ) );
    BOOST_CHECK( m_part_no_data.Compare( testPart, ~SCH_ITEM::COMPARE_FLAGS::UUID ) > 0 );
    m_part_no_data.RemoveDrawItem( &m_part_no_data.GetDrawItems()[SCH_SHAPE_T].front() );

    // Draw item list contents comparison tests.
    testPart.AddDrawItem( new SCH_SHAPE( SHAPE_T::RECTANGLE, LAYER_DEVICE ) );
    m_part_no_data.AddDrawItem( new SCH_SHAPE( SHAPE_T::ARC, LAYER_DEVICE ) );
    BOOST_CHECK( m_part_no_data.Compare( testPart, ~SCH_ITEM::COMPARE_FLAGS::UUID ) > 0 );
    m_part_no_data.RemoveDrawItem( &m_part_no_data.GetDrawItems()[SCH_SHAPE_T].front() );
    testPart.RemoveDrawItem( &testPart.GetDrawItems()[SCH_SHAPE_T].front() );
    m_part_no_data.AddDrawItem( new SCH_PIN( &m_part_no_data ) );
    BOOST_CHECK( m_part_no_data.Compare( testPart, ~SCH_ITEM::COMPARE_FLAGS::UUID ) > 0 );
    m_part_no_data.RemoveDrawItem( &m_part_no_data.GetDrawItems()[SCH_PIN_T].front() );

    // Footprint filter array comparison tests.
    wxArrayString footPrintFilters;
    BOOST_CHECK( m_part_no_data.GetFPFilters() == footPrintFilters );
    footPrintFilters.Add( "b" );
    testPart.SetFPFilters( footPrintFilters );
    BOOST_CHECK( m_part_no_data.Compare( testPart, ~SCH_ITEM::COMPARE_FLAGS::UUID ) < 0 );
    m_part_no_data.SetFPFilters( footPrintFilters );
    footPrintFilters.Clear();
    testPart.SetFPFilters( footPrintFilters );
    BOOST_CHECK( m_part_no_data.Compare( testPart, ~SCH_ITEM::COMPARE_FLAGS::UUID ) > 0 );
    footPrintFilters.Clear();
    m_part_no_data.SetFPFilters( footPrintFilters );
    testPart.SetFPFilters( footPrintFilters );

    // Description string tests.
    m_part_no_data.SetDescription( "b" );
    testPart.SetDescription( "b" );
    BOOST_CHECK_EQUAL( m_part_no_data.Compare( testPart, ~SCH_ITEM::COMPARE_FLAGS::UUID ), 0 );
    m_part_no_data.SetDescription( "a" );
    BOOST_CHECK( m_part_no_data.Compare( testPart, ~SCH_ITEM::COMPARE_FLAGS::UUID ) < 0 );
    m_part_no_data.SetDescription( "c" );
    BOOST_CHECK( m_part_no_data.Compare( testPart, ~SCH_ITEM::COMPARE_FLAGS::UUID ) > 0 );
    m_part_no_data.SetDescription( wxEmptyString );
    testPart.SetDescription( wxEmptyString );

    // Key word string tests.
    m_part_no_data.SetKeyWords( "b" );
    testPart.SetKeyWords( "b" );
    BOOST_CHECK_EQUAL( m_part_no_data.Compare( testPart, ~SCH_ITEM::COMPARE_FLAGS::UUID ), 0 );
    m_part_no_data.SetKeyWords( "a" );
    BOOST_CHECK( m_part_no_data.Compare( testPart, ~SCH_ITEM::COMPARE_FLAGS::UUID ) < 0 );
    m_part_no_data.SetKeyWords( "c" );
    BOOST_CHECK( m_part_no_data.Compare( testPart, ~SCH_ITEM::COMPARE_FLAGS::UUID ) > 0 );
    m_part_no_data.SetKeyWords( wxEmptyString );
    testPart.SetKeyWords( wxEmptyString );

    // Pin name offset comparison tests.
    testPart.SetPinNameOffset( testPart.GetPinNameOffset() + 1 );
    BOOST_CHECK( m_part_no_data.Compare( testPart, ~SCH_ITEM::COMPARE_FLAGS::UUID ) < 0 );
    testPart.SetPinNameOffset( testPart.GetPinNameOffset() - 2 );
    BOOST_CHECK( m_part_no_data.Compare( testPart, ~SCH_ITEM::COMPARE_FLAGS::UUID ) > 0 );
    testPart.SetPinNameOffset( testPart.GetPinNameOffset() + 1 );

    // Units locked flag comparison tests.
    testPart.LockUnits( true );
    BOOST_CHECK( m_part_no_data.Compare( testPart, ~SCH_ITEM::COMPARE_FLAGS::UUID ) < 0 );
    testPart.LockUnits( false );
    m_part_no_data.LockUnits( true );
    BOOST_CHECK( m_part_no_data.Compare( testPart, ~SCH_ITEM::COMPARE_FLAGS::UUID ) > 0 );
    m_part_no_data.LockUnits( false );

    // Include in BOM support tests.
    testPart.SetExcludedFromBOM( true );
    BOOST_CHECK( m_part_no_data.Compare( testPart, ~SCH_ITEM::COMPARE_FLAGS::UUID ) > 0 );
    testPart.SetExcludedFromBOM( false );
    m_part_no_data.SetExcludedFromBOM( true );
    BOOST_CHECK( m_part_no_data.Compare( testPart, ~SCH_ITEM::COMPARE_FLAGS::UUID ) < 0 );
    m_part_no_data.SetExcludedFromBOM( false );

    // Include on board support tests.
    testPart.SetExcludedFromBoard( true );
    BOOST_CHECK( m_part_no_data.Compare( testPart, ~SCH_ITEM::COMPARE_FLAGS::UUID ) > 0 );
    testPart.SetExcludedFromBoard( false );
    m_part_no_data.SetExcludedFromBoard( true );
    BOOST_CHECK( m_part_no_data.Compare( testPart, ~SCH_ITEM::COMPARE_FLAGS::UUID ) < 0 );
    m_part_no_data.SetExcludedFromBoard( false );

    // Include in position files support tests.
    testPart.SetExcludedFromPosFiles( true );
    BOOST_CHECK( m_part_no_data.Compare( testPart, ~SCH_ITEM::COMPARE_FLAGS::UUID ) > 0 );
    testPart.SetExcludedFromPosFiles( false );
    m_part_no_data.SetExcludedFromPosFiles( true );
    BOOST_CHECK( m_part_no_data.Compare( testPart, ~SCH_ITEM::COMPARE_FLAGS::UUID ) < 0 );
    m_part_no_data.SetExcludedFromPosFiles( false );

    // Show pin names flag comparison tests.
    m_part_no_data.SetShowPinNames( false );
    BOOST_CHECK( m_part_no_data.Compare( testPart, ~SCH_ITEM::COMPARE_FLAGS::UUID ) < 0 );
    m_part_no_data.SetShowPinNames( true );
    testPart.SetShowPinNames( false );
    BOOST_CHECK( m_part_no_data.Compare( testPart, ~SCH_ITEM::COMPARE_FLAGS::UUID ) > 0 );
    testPart.SetShowPinNames( true );

    // Show pin numbers flag comparison tests.
    m_part_no_data.SetShowPinNumbers( false );
    BOOST_CHECK( m_part_no_data.Compare( testPart, ~SCH_ITEM::COMPARE_FLAGS::UUID ) < 0 );
    m_part_no_data.SetShowPinNumbers( true );
    testPart.SetShowPinNumbers( false );
    BOOST_CHECK( m_part_no_data.Compare( testPart, ~SCH_ITEM::COMPARE_FLAGS::UUID ) > 0 );
    testPart.SetShowPinNumbers( true );

    // Time stamp comparison tests.

    // Check to see if we broke the copy ctor.
    LIB_SYMBOL copy( testPart );
    BOOST_CHECK( testPart.Compare( copy ) == 0 );
}


/**
 * Check the fetch unit items code.
 */
BOOST_AUTO_TEST_CASE( GetUnitItems )
{
    // There are no unit draw items in the empty LIB_SYMBOL object.
    BOOST_CHECK( m_part_no_data.GetUnitDrawItems( 1, 1 ).size() == 0 );

    // A single unique unit with 1 pin common to all units and all body styles.
    SCH_PIN* pin1 = new SCH_PIN( &m_part_no_data );
    m_part_no_data.AddDrawItem( pin1 );
    BOOST_CHECK( m_part_no_data.GetUnitDrawItems( 0, 0 ).size() == 1 );

    // A single unique unit with 1 pin in unit 1 and common to all body styles.
    pin1->SetUnit( 1 );
    BOOST_CHECK( m_part_no_data.GetUnitDrawItems( 1, 0 ).size() == 1 );

    // A single unique unit with 1 pin in unit 1 and body style 1.
    pin1->SetBodyStyle( 1 );
    BOOST_CHECK( m_part_no_data.GetUnitDrawItems( 1, 1 ).size() == 1 );

    // Two unique units with pin 1 assigned to unit 1 and body style 1 and pin 2 assigned to
    // unit 2 and body style 1.
    SCH_PIN* pin2 = new SCH_PIN( &m_part_no_data );
    m_part_no_data.SetUnitCount( 2, true );
    pin2->SetUnit( 2 );
    pin2->SetBodyStyle( 2 );
    pin2->SetNumber( "4" );
    m_part_no_data.AddDrawItem( pin2 );
    BOOST_CHECK( m_part_no_data.GetUnitDrawItems( 2, 2 ).size() == 1 );

    // Make pin 1 body style common to all units.
    pin1->SetBodyStyle( 0 );
    BOOST_CHECK( m_part_no_data.GetUnitDrawItems( 1, 1 ).size() == 0 );
    BOOST_CHECK( m_part_no_data.GetUnitDrawItems( 2, 1 ).size() == 1 );

    m_part_no_data.RemoveDrawItem( pin2 );
    m_part_no_data.RemoveDrawItem( pin1 );
    m_part_no_data.RemoveDrawItem( &*m_part_no_data.GetDrawItems().begin() );
}


/**
 * Check the fetch unit draw items code.
 */
BOOST_AUTO_TEST_CASE( GetUnitDrawItems )
{
    // An empty symbol still reports its unit and body style so the saver writes them out
    std::vector<struct LIB_SYMBOL_UNIT> units = m_part_no_data.GetUnitDrawItems();

    BOOST_REQUIRE_EQUAL( units.size(), 1u );
    BOOST_CHECK_EQUAL( units[0].m_unit, 1 );
    BOOST_CHECK_EQUAL( units[0].m_bodyStyle, 1 );
    BOOST_CHECK( units[0].m_items.empty() );

    // A pin common to all units and all body styles matches no numbered unit and gets a
    // record of its own rather than being dropped
    SCH_PIN* pin1 = new SCH_PIN( &m_part_no_data );
    pin1->SetNumber( "1" );
    m_part_no_data.AddDrawItem( pin1 );

    units = m_part_no_data.GetUnitDrawItems();

    BOOST_REQUIRE_EQUAL( units.size(), 2u );
    BOOST_CHECK_EQUAL( units[0].m_unit, 0 );
    BOOST_CHECK_EQUAL( units[0].m_bodyStyle, 0 );
    BOOST_REQUIRE_EQUAL( units[0].m_items.size(), 1u );
    BOOST_CHECK_EQUAL( units[0].m_items[0], pin1 );
    BOOST_CHECK_EQUAL( units[1].m_unit, 1 );
    BOOST_CHECK_EQUAL( units[1].m_bodyStyle, 1 );
    BOOST_CHECK( units[1].m_items.empty() );

    // Units without draw items of their own are still reported
    m_part_no_data.SetUnitCount( 3, true );

    units = m_part_no_data.GetUnitDrawItems();

    BOOST_REQUIRE_EQUAL( units.size(), 4u );
    BOOST_CHECK_EQUAL( units[0].m_unit, 0 );
    BOOST_CHECK_EQUAL( units[1].m_unit, 1 );
    BOOST_CHECK_EQUAL( units[2].m_unit, 2 );
    BOOST_CHECK_EQUAL( units[3].m_unit, 3 );
    BOOST_CHECK( units[1].m_items.empty() );
    BOOST_CHECK( units[2].m_items.empty() );
    BOOST_CHECK( units[3].m_items.empty() );

    // Both body styles of every unit are reported once De Morgan is enabled
    m_part_no_data.SetHasDeMorganBodyStyles( true );

    units = m_part_no_data.GetUnitDrawItems();

    BOOST_REQUIRE_EQUAL( units.size(), 7u );
    BOOST_CHECK_EQUAL( units[1].m_bodyStyle, 1 );
    BOOST_CHECK_EQUAL( units[2].m_bodyStyle, 2 );
}


/**
 * Check inheritance support.
 */
BOOST_AUTO_TEST_CASE( Inheritance )
{
    std::unique_ptr<LIB_SYMBOL> parent = std::make_unique<LIB_SYMBOL>( "parent" );
    BOOST_CHECK( parent->IsRoot() );
    BOOST_CHECK_EQUAL( parent->GetInheritanceDepth(), 0 );

    std::unique_ptr<LIB_SYMBOL> ref = std::make_unique<LIB_SYMBOL>( *parent );

    std::unique_ptr<LIB_SYMBOL> child = std::make_unique<LIB_SYMBOL>( "child", parent.get() );
    BOOST_CHECK( child->IsDerived() );
    BOOST_CHECK_EQUAL( child->GetInheritanceDepth(), 1 );

    std::unique_ptr<LIB_SYMBOL> grandChild = std::make_unique<LIB_SYMBOL>( "grandchild", child.get() );
    BOOST_CHECK( grandChild->IsDerived() );
    BOOST_CHECK_EQUAL( grandChild->GetInheritanceDepth(), 2 );

    BOOST_CHECK( parent->GetRootSymbol().get() == parent.get() );
    BOOST_CHECK( child->GetRootSymbol().get() == parent.get() );
    BOOST_CHECK( grandChild->GetRootSymbol().get() == parent.get() );

    std::shared_ptr<LIB_SYMBOL> parentRef = child->GetParent().lock();
    BOOST_CHECK( parentRef );
    BOOST_CHECK( parentRef == parent->SharedPtr() );
    BOOST_CHECK_EQUAL( parent->SharedPtr().use_count(), 3 );

    std::shared_ptr<LIB_SYMBOL> childRef = grandChild->GetParent().lock();
    BOOST_CHECK( childRef );
    BOOST_CHECK( childRef == child->SharedPtr() );
    BOOST_CHECK_EQUAL( child->SharedPtr().use_count(), 3 );

    BOOST_CHECK_EQUAL( child->GetUnitCount(), 1 );
    parent->SetUnitCount( 4, true );
    BOOST_CHECK_EQUAL( child->GetUnitCount(), 4 );
    parent->SetUnitCount( 1, true );

    parent->GetField( FIELD_T::DATASHEET )->SetText( "https://kicad/resistors.pdf" );
    ref->GetField( FIELD_T::DATASHEET )->SetText( "https://kicad/resistors.pdf" );

    BOOST_CHECK( *parent == *ref );

    ref->SetName( "child" );
    SCH_FIELD* field = new SCH_FIELD( nullptr, FIELD_T::USER, "Manufacturer" );
    field->SetText( "KiCad" );
    child->AddField( field );
    field->SetParent( child.get() );

    field = new SCH_FIELD( nullptr, FIELD_T::USER, "Manufacturer" );
    field->SetText( "KiCad" );
    ref->AddField( field );
    field->SetParent( ref.get() );

    BOOST_CHECK( *ref == *child->Flatten() );

    ref->SetName( "grandchild" );
    field = new SCH_FIELD( nullptr, FIELD_T::USER, "MPN" );
    field->SetText( "123456" );
    grandChild->AddField( field );
    field->SetParent( grandChild.get() );

    field = new SCH_FIELD( nullptr, FIELD_T::USER, "MPN" );
    field->SetText( "123456" );
    ref->AddField( field );
    field->SetParent( ref.get() );

    BOOST_CHECK( *ref == *grandChild->Flatten() );

    BOOST_CHECK_EQUAL( grandChild->Flatten()->GetField( FIELD_T::DATASHEET )->GetText(),
                       "https://kicad/resistors.pdf" );

    child->SetParent();
    BOOST_CHECK_EQUAL( child->GetUnitCount(), 1 );

    parentRef.reset();
    BOOST_CHECK_EQUAL( parent->SharedPtr().use_count(), 2 );
}


/**
 * Check the copy constructor.
 */
BOOST_AUTO_TEST_CASE( CopyConstructor )
{
    std::shared_ptr<LIB_SYMBOL> copy = std::make_shared<LIB_SYMBOL>( m_part_no_data );
    BOOST_CHECK( m_part_no_data == *copy.get() );
}


/**
 * Check the power and legacy power symbol tests.
 */
BOOST_AUTO_TEST_CASE( IsPowerTest )
{
    std::unique_ptr<LIB_SYMBOL> symbol = std::make_unique<LIB_SYMBOL>( "power" );
    SCH_PIN* pin = new SCH_PIN( symbol.get() );
    pin->SetNumber( "1" );
    pin->SetType( ELECTRICAL_PINTYPE::PT_POWER_IN );
    pin->SetVisible( false );
    symbol->AddDrawItem( pin );

    BOOST_CHECK( !symbol->IsPower() );
    BOOST_CHECK( symbol->IsNormal() );

    symbol->SetGlobalPower();
    BOOST_CHECK( symbol->IsPower() );
    BOOST_CHECK( !symbol->IsNormal() );

    // symbol->SetNormal();
    // symbol->GetReferenceField().SetText( wxS( "#PWR" ) );
    // BOOST_CHECK( symbol->IsPower() );

    // Legacy power symbols are limited to a single pin.
    // pin = new LIB_PIN( symbol.get() );
    // pin->SetNumber( "2" );
    // pin->SetType( ELECTRICAL_PINTYPE::PT_POWER_IN );
    // pin->SetVisible( false );
    // symbol->AddDrawItem( pin );
    // BOOST_CHECK( !symbol->IsPower() );
}


/**
 * Regression test for https://gitlab.com/kicad/code/kicad/-/issues/23788
 *
 * SetUnitCount() must reject values less than 1. Previously, an importer (Altium .schdoc)
 * could pass a negative or zero unit count, which caused the deletion loop to erase the
 * mandatory fields (REFERENCE, VALUE, etc.) because their m_unit (== 0) was greater than
 * the requested aCount. The symbol then crashed later when GetReferenceField() returned
 * a null pointer that was implicitly dereferenced.
 */
BOOST_AUTO_TEST_CASE( SetUnitCountRejectsInvalidValues )
{
    auto checkMandatoryFields = []( LIB_SYMBOL& aSymbol )
    {
        BOOST_CHECK_EQUAL( aSymbol.GetUnitCount(), 1 );
        BOOST_CHECK_NE( aSymbol.GetField( FIELD_T::REFERENCE ), nullptr );
        BOOST_CHECK_NE( aSymbol.GetField( FIELD_T::VALUE ), nullptr );
        BOOST_CHECK_NE( aSymbol.GetField( FIELD_T::FOOTPRINT ), nullptr );
        BOOST_CHECK_NE( aSymbol.GetField( FIELD_T::DATASHEET ), nullptr );
        BOOST_CHECK_NE( aSymbol.GetField( FIELD_T::DESCRIPTION ), nullptr );

        // Reading the reference field text must not crash. This was the original SIGSEGV
        // path through CONNECTION_SUBGRAPH::GetDriverPriority() reported in issue 23788.
        BOOST_CHECK_NO_THROW( (void) aSymbol.GetReferenceField().GetText() );
    };

    // Sanity check the freshly constructed symbol.
    LIB_SYMBOL baseline( wxS( "test_part" ) );
    checkMandatoryFields( baseline );

    // When wxDEBUG_LEVEL > 0 the wxCHECK fires (caught by CHECK_WX_ASSERT) and the function
    // does not modify the symbol. With wxDEBUG_LEVEL == 0 the wxCHECK is silent and the
    // function returns early without modification. Cover both call paths so the test
    // exercises the actual SetUnitCount entry on every build configuration.
    LIB_SYMBOL zeroCount( wxS( "test_part" ) );
    CHECK_WX_ASSERT( zeroCount.SetUnitCount( 0, true ) );
#if wxDEBUG_LEVEL == 0
    zeroCount.SetUnitCount( 0, true );
#endif
    checkMandatoryFields( zeroCount );

    LIB_SYMBOL negativeCount( wxS( "test_part" ) );
    CHECK_WX_ASSERT( negativeCount.SetUnitCount( -1, true ) );
#if wxDEBUG_LEVEL == 0
    negativeCount.SetUnitCount( -1, true );
#endif
    checkMandatoryFields( negativeCount );
}


/**
 * Regression test for https://gitlab.com/kicad/code/kicad/-/issues/25004
 *
 * Dropping a symbol back to a single body style must delete the alternate drawings whichever
 * order the caller updates the body style metadata in.  DIALOG_LIB_SYMBOL_PROPERTIES cleared
 * the De Morgan flag first, which made SetBodyStyleCount() see a previous count of 1 and skip
 * the deletion.  The orphans then drew on top of the standard body style because renderers ask
 * for body style 0 (all) when a symbol has only one.
 */
BOOST_AUTO_TEST_CASE( DeleteDeMorganBodyStyleDrawItems )
{
    for( bool clearFlagFirst : { false, true } )
    {
        std::unique_ptr<LIB_SYMBOL> symbol = loadDeMorganSymbol();
        BOOST_REQUIRE( symbol );
        BOOST_REQUIRE( symbol->HasDeMorganBodyStyles() );
        BOOST_REQUIRE( alternateBodyStyleItemCount( *symbol ) > 0 );

        if( clearFlagFirst )
        {
            symbol->SetHasDeMorganBodyStyles( false );
            symbol->SetBodyStyleCount( 1, false, false );
        }
        else
        {
            symbol->SetBodyStyleCount( 1, false, false );
            symbol->SetHasDeMorganBodyStyles( false );
        }

        symbol->SetBodyStyleNames( {} );

        BOOST_CHECK_EQUAL( symbol->GetBodyStyleCount(), 1 );
        BOOST_CHECK_EQUAL( alternateBodyStyleItemCount( *symbol ), 0 );
    }
}


/**
 * Libraries already written with orphaned alternate drawings must load without them.  The
 * declared body style count is authoritative for V10 and later files.
 */
BOOST_AUTO_TEST_CASE( OrphanedBodyStyleItemsAreNotLoaded )
{
    std::unique_ptr<LIB_SYMBOL> symbol = loadDeMorganSymbol();
    BOOST_REQUIRE( symbol );

    // Reproduce what a broken save left on disk: no De Morgan declaration, alternate drawings
    symbol->SetHasDeMorganBodyStyles( false );
    BOOST_REQUIRE( alternateBodyStyleItemCount( *symbol ) > 0 );

    SCOPED_TEMP_LIB tempLib;
    saveToLib( tempLib.GetPath(), *symbol );

    IO_RELEASER<SCH_IO> pi( SCH_IO_MGR::FindPlugin( SCH_IO_MGR::SCH_KICAD ) );
    LIB_SYMBOL* reloaded = pi->LoadSymbol( tempLib.GetPath(), wxS( "4001" ) );

    BOOST_REQUIRE( reloaded );
    BOOST_CHECK_EQUAL( reloaded->GetBodyStyleCount(), 1 );
    BOOST_CHECK_EQUAL( alternateBodyStyleItemCount( *reloaded ), 0 );

    // The standard body style must survive the pruning
    BOOST_CHECK( !reloaded->GetUnitDrawItems( 1, BODY_STYLE::BASE ).empty() );
    BOOST_CHECK_EQUAL( reloaded->GetUnitCount(), symbol->GetUnitCount() );
}


BOOST_AUTO_TEST_SUITE_END()
