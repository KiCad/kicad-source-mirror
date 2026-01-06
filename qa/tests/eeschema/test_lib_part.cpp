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
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
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
    BOOST_CHECK_EQUAL( m_part_no_data.GetPins().size(), 0 );
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
    BOOST_CHECK_EQUAL( m_part_no_data.Compare( testPart, SCH_ITEM::COMPARE_FLAGS::EQUALITY ), 0 );

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
    BOOST_CHECK_EQUAL( m_part_no_data.Compare( testPart, SCH_ITEM::COMPARE_FLAGS::EQUALITY ), 0 );
    m_part_no_data.RemoveDrawItem( &m_part_no_data.GetDrawItems()[SCH_SHAPE_T].front() );
    BOOST_CHECK( m_part_no_data.Compare( testPart, SCH_ITEM::COMPARE_FLAGS::EQUALITY ) < 0 );
    testPart.RemoveDrawItem( &testPart.GetDrawItems()[SCH_SHAPE_T].front() );
    m_part_no_data.AddDrawItem( new SCH_SHAPE( SHAPE_T::RECTANGLE, LAYER_DEVICE ) );
    BOOST_CHECK( m_part_no_data.Compare( testPart, SCH_ITEM::COMPARE_FLAGS::EQUALITY ) > 0 );
    m_part_no_data.RemoveDrawItem( &m_part_no_data.GetDrawItems()[SCH_SHAPE_T].front() );

    // Draw item list contents comparison tests.
    testPart.AddDrawItem( new SCH_SHAPE( SHAPE_T::RECTANGLE, LAYER_DEVICE ) );
    m_part_no_data.AddDrawItem( new SCH_SHAPE( SHAPE_T::ARC, LAYER_DEVICE ) );
    BOOST_CHECK( m_part_no_data.Compare( testPart, SCH_ITEM::COMPARE_FLAGS::EQUALITY ) > 0 );
    m_part_no_data.RemoveDrawItem( &m_part_no_data.GetDrawItems()[SCH_SHAPE_T].front() );
    testPart.RemoveDrawItem( &testPart.GetDrawItems()[SCH_SHAPE_T].front() );
    m_part_no_data.AddDrawItem( new SCH_PIN( &m_part_no_data ) );
    BOOST_CHECK( m_part_no_data.Compare( testPart, SCH_ITEM::COMPARE_FLAGS::EQUALITY ) > 0 );
    m_part_no_data.RemoveDrawItem( &m_part_no_data.GetDrawItems()[SCH_PIN_T].front() );

    // Footprint filter array comparison tests.
    wxArrayString footPrintFilters;
    BOOST_CHECK( m_part_no_data.GetFPFilters() == footPrintFilters );
    footPrintFilters.Add( "b" );
    testPart.SetFPFilters( footPrintFilters );
    BOOST_CHECK( m_part_no_data.Compare( testPart, SCH_ITEM::COMPARE_FLAGS::EQUALITY ) < 0 );
    m_part_no_data.SetFPFilters( footPrintFilters );
    footPrintFilters.Clear();
    testPart.SetFPFilters( footPrintFilters );
    BOOST_CHECK( m_part_no_data.Compare( testPart, SCH_ITEM::COMPARE_FLAGS::EQUALITY ) > 0 );
    footPrintFilters.Clear();
    m_part_no_data.SetFPFilters( footPrintFilters );
    testPart.SetFPFilters( footPrintFilters );

    // Description string tests.
    m_part_no_data.SetDescription( "b" );
    testPart.SetDescription( "b" );
    BOOST_CHECK_EQUAL( m_part_no_data.Compare( testPart, SCH_ITEM::COMPARE_FLAGS::EQUALITY ), 0 );
    m_part_no_data.SetDescription( "a" );
    BOOST_CHECK( m_part_no_data.Compare( testPart, SCH_ITEM::COMPARE_FLAGS::EQUALITY ) < 0 );
    m_part_no_data.SetDescription( "c" );
    BOOST_CHECK( m_part_no_data.Compare( testPart, SCH_ITEM::COMPARE_FLAGS::EQUALITY ) > 0 );
    m_part_no_data.SetDescription( wxEmptyString );
    testPart.SetDescription( wxEmptyString );

    // Key word string tests.
    m_part_no_data.SetKeyWords( "b" );
    testPart.SetKeyWords( "b" );
    BOOST_CHECK_EQUAL( m_part_no_data.Compare( testPart, SCH_ITEM::COMPARE_FLAGS::EQUALITY ), 0 );
    m_part_no_data.SetKeyWords( "a" );
    BOOST_CHECK( m_part_no_data.Compare( testPart, SCH_ITEM::COMPARE_FLAGS::EQUALITY ) < 0 );
    m_part_no_data.SetKeyWords( "c" );
    BOOST_CHECK( m_part_no_data.Compare( testPart, SCH_ITEM::COMPARE_FLAGS::EQUALITY ) > 0 );
    m_part_no_data.SetKeyWords( wxEmptyString );
    testPart.SetKeyWords( wxEmptyString );

    // Pin name offset comparison tests.
    testPart.SetPinNameOffset( testPart.GetPinNameOffset() + 1 );
    BOOST_CHECK( m_part_no_data.Compare( testPart, SCH_ITEM::COMPARE_FLAGS::EQUALITY ) < 0 );
    testPart.SetPinNameOffset( testPart.GetPinNameOffset() - 2 );
    BOOST_CHECK( m_part_no_data.Compare( testPart, SCH_ITEM::COMPARE_FLAGS::EQUALITY ) > 0 );
    testPart.SetPinNameOffset( testPart.GetPinNameOffset() + 1 );

    // Units locked flag comparison tests.
    testPart.LockUnits( true );
    BOOST_CHECK( m_part_no_data.Compare( testPart, SCH_ITEM::COMPARE_FLAGS::EQUALITY ) < 0 );
    testPart.LockUnits( false );
    m_part_no_data.LockUnits( true );
    BOOST_CHECK( m_part_no_data.Compare( testPart, SCH_ITEM::COMPARE_FLAGS::EQUALITY ) > 0 );
    m_part_no_data.LockUnits( false );

    // Include in BOM support tests.
    testPart.SetExcludedFromBOM( true );
    BOOST_CHECK( m_part_no_data.Compare( testPart, SCH_ITEM::COMPARE_FLAGS::EQUALITY ) > 0 );
    testPart.SetExcludedFromBOM( false );
    m_part_no_data.SetExcludedFromBOM( true );
    BOOST_CHECK( m_part_no_data.Compare( testPart, SCH_ITEM::COMPARE_FLAGS::EQUALITY ) < 0 );
    m_part_no_data.SetExcludedFromBOM( false );

    // Include on board support tests.
    testPart.SetExcludedFromBoard( true );
    BOOST_CHECK( m_part_no_data.Compare( testPart, SCH_ITEM::COMPARE_FLAGS::EQUALITY ) > 0 );
    testPart.SetExcludedFromBoard( false );
    m_part_no_data.SetExcludedFromBoard( true );
    BOOST_CHECK( m_part_no_data.Compare( testPart, SCH_ITEM::COMPARE_FLAGS::EQUALITY ) < 0 );
    m_part_no_data.SetExcludedFromBoard( false );

    // Include in position files support tests.
    testPart.SetExcludedFromPosFiles( true );
    BOOST_CHECK( m_part_no_data.Compare( testPart, SCH_ITEM::COMPARE_FLAGS::EQUALITY ) > 0 );
    testPart.SetExcludedFromPosFiles( false );
    m_part_no_data.SetExcludedFromPosFiles( true );
    BOOST_CHECK( m_part_no_data.Compare( testPart, SCH_ITEM::COMPARE_FLAGS::EQUALITY ) < 0 );
    m_part_no_data.SetExcludedFromPosFiles( false );

    // Show pin names flag comparison tests.
    m_part_no_data.SetShowPinNames( false );
    BOOST_CHECK( m_part_no_data.Compare( testPart, SCH_ITEM::COMPARE_FLAGS::EQUALITY ) < 0 );
    m_part_no_data.SetShowPinNames( true );
    testPart.SetShowPinNames( false );
    BOOST_CHECK( m_part_no_data.Compare( testPart, SCH_ITEM::COMPARE_FLAGS::EQUALITY ) > 0 );
    testPart.SetShowPinNames( true );

    // Show pin numbers flag comparison tests.
    m_part_no_data.SetShowPinNumbers( false );
    BOOST_CHECK( m_part_no_data.Compare( testPart, SCH_ITEM::COMPARE_FLAGS::EQUALITY ) < 0 );
    m_part_no_data.SetShowPinNumbers( true );
    testPart.SetShowPinNumbers( false );
    BOOST_CHECK( m_part_no_data.Compare( testPart, SCH_ITEM::COMPARE_FLAGS::EQUALITY ) > 0 );
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
    // There are no unit draw items in the empty LIB_SYMBOL object.
    BOOST_CHECK( m_part_no_data.GetUnitDrawItems().size() == 0 );

    // A single unique unit with 1 pin common to all units and all body styles.
    SCH_PIN* pin1 = new SCH_PIN( &m_part_no_data );
    pin1->SetNumber( "1" );
    m_part_no_data.AddDrawItem( pin1 );
    std::vector<struct LIB_SYMBOL_UNIT> units = m_part_no_data.GetUnitDrawItems();
    BOOST_CHECK( units.size() == 1 );
    BOOST_CHECK( units[0].m_unit == 0 );
    BOOST_CHECK( units[0].m_bodyStyle == 0 );
    BOOST_CHECK( units[0].m_items[0] == pin1 );
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


BOOST_AUTO_TEST_SUITE_END()
