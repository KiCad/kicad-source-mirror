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
 * http://www.gnu.org/licenses/gpl-3.0.en.html
 * or you may search the http://www.gnu.org website for the version 3 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

/**
 * @file
 * Test suite for PROPERTY_HOLDER and PROPERTY_MIXIN classes
 *
 * Tests the core functionality of the property system including type safety,
 * storage, retrieval, and all utility functions.
 */

#include <qa_utils/wx_utils/unit_test_utils.h>

// Code under test
#include <property_holder.h>

#include <string>
#include <vector>
#include <memory>

namespace
{

/**
 * Test class to verify PROPERTY_MIXIN functionality
 */
class TestWidget : public PROPERTY_MIXIN
{
public:
    TestWidget( const std::string& name ) : m_name( name )
    {
        SetProperty( "widget_name", name );
        SetProperty( "default_enabled", true );
    }

    const std::string& GetName() const { return m_name; }

private:
    std::string m_name;
};

/**
 * Custom type for testing complex object storage
 */
struct CustomData
{
    int value;
    std::string text;

    CustomData( int v, const std::string& t ) : value( v ), text( t ) {}

    bool operator==( const CustomData& other ) const
    {
        return value == other.value && text == other.text;
    }
};

/**
 * Test fixture with pre-configured PROPERTY_HOLDER instances
 */
struct PROPERTY_HOLDER_TEST_FIXTURE
{
    PROPERTY_HOLDER_TEST_FIXTURE()
    {
        // Set up a holder with various types
        m_populatedHolder.SetProperty( "bool_prop", true );
        m_populatedHolder.SetProperty( "int_prop", 42 );
        m_populatedHolder.SetProperty( "double_prop", 3.14159 );
        m_populatedHolder.SetProperty( "string_prop", std::string( "hello world" ) );
        m_populatedHolder.SetProperty( "cstring_prop", "c-style string" );

        // Add a custom object
        m_populatedHolder.SetProperty( "custom_prop", CustomData( 100, "custom" ) );

        // Add a vector
        std::vector<int> numbers = { 1, 2, 3, 4, 5 };
        m_populatedHolder.SetProperty( "vector_prop", numbers );
    }

    PROPERTY_HOLDER m_emptyHolder;
    PROPERTY_HOLDER m_populatedHolder;
};

} // namespace

/**
 * Declare the test suite
 */
BOOST_FIXTURE_TEST_SUITE( TEST_PROPERTY_HOLDER, PROPERTY_HOLDER_TEST_FIXTURE )

/**
 * Test basic property setting and getting
 */
BOOST_AUTO_TEST_CASE( BasicSetGet )
{
    PROPERTY_HOLDER holder;

    // Test various basic types
    holder.SetProperty( "test_bool", true );
    holder.SetProperty( "test_int", 123 );
    holder.SetProperty( "test_double", 2.71828 );
    holder.SetProperty( "test_string", std::string( "test value" ) );

    // Verify retrieval with correct types
    auto boolResult = holder.GetProperty<bool>( "test_bool" );
    BOOST_CHECK( boolResult.has_value() );
    BOOST_CHECK_EQUAL( *boolResult, true );

    auto intResult = holder.GetProperty<int>( "test_int" );
    BOOST_CHECK( intResult.has_value() );
    BOOST_CHECK_EQUAL( *intResult, 123 );

    auto doubleResult = holder.GetProperty<double>( "test_double" );
    BOOST_CHECK( doubleResult.has_value() );
    BOOST_CHECK_CLOSE( *doubleResult, 2.71828, 0.0001 );

    auto stringResult = holder.GetProperty<std::string>( "test_string" );
    BOOST_CHECK( stringResult.has_value() );
    BOOST_CHECK_EQUAL( *stringResult, "test value" );
}

/**
 * Test type safety - getting with wrong type should return nullopt
 */
BOOST_AUTO_TEST_CASE( TypeSafety )
{
    PROPERTY_HOLDER holder;
    holder.SetProperty( "int_value", 42 );

    // Correct type retrieval
    auto intResult = holder.GetProperty<int>( "int_value" );
    BOOST_CHECK( intResult.has_value() );
    BOOST_CHECK_EQUAL( *intResult, 42 );

    // Wrong type retrieval should return nullopt
    auto stringResult = holder.GetProperty<std::string>( "int_value" );
    BOOST_CHECK( !stringResult.has_value() );

    auto doubleResult = holder.GetProperty<double>( "int_value" );
    BOOST_CHECK( !doubleResult.has_value() );

    auto boolResult = holder.GetProperty<bool>( "int_value" );
    BOOST_CHECK( !boolResult.has_value() );
}

/**
 * Test GetPropertyOr with default values
 */
BOOST_AUTO_TEST_CASE( DefaultValues )
{
    PROPERTY_HOLDER holder;
    holder.SetProperty( "existing_prop", 100 );

    // Existing property should return stored value
    int existingValue = holder.GetPropertyOr( "existing_prop", 999 );
    BOOST_CHECK_EQUAL( existingValue, 100 );

    // Non-existing property should return default
    int nonExistingValue = holder.GetPropertyOr( "missing_prop", 777 );
    BOOST_CHECK_EQUAL( nonExistingValue, 777 );

    // Wrong type should return default
    std::string wrongTypeValue = holder.GetPropertyOr<std::string>( "existing_prop", "default" );
    BOOST_CHECK_EQUAL( wrongTypeValue, "default" );

    // Test with different types
    bool defaultBool = holder.GetPropertyOr( "missing_bool", false );
    BOOST_CHECK_EQUAL( defaultBool, false );

    std::string defaultString = holder.GetPropertyOr<std::string>( "missing_string", "fallback" );
    BOOST_CHECK_EQUAL( defaultString, "fallback" );
}

/**
 * Test property existence checking
 */
BOOST_AUTO_TEST_CASE( ExistenceChecking )
{
    // Empty holder
    BOOST_CHECK_EQUAL( false, m_emptyHolder.HasProperty( "any_key" ) );

    // Populated holder
    BOOST_CHECK( m_populatedHolder.HasProperty( "bool_prop" ) );
    BOOST_CHECK( m_populatedHolder.HasProperty( "int_prop" ) );
    BOOST_CHECK( m_populatedHolder.HasProperty( "string_prop" ) );
    BOOST_CHECK_EQUAL( false, m_populatedHolder.HasProperty( "missing_prop" ) );
}

/**
 * Test type-specific existence checking
 */
BOOST_AUTO_TEST_CASE( TypeSpecificExistence )
{
    // Check correct type detection
    BOOST_CHECK( m_populatedHolder.HasPropertyOfType<bool>( "bool_prop" ) );
    BOOST_CHECK( m_populatedHolder.HasPropertyOfType<int>( "int_prop" ) );
    BOOST_CHECK( m_populatedHolder.HasPropertyOfType<double>( "double_prop" ) );
    BOOST_CHECK( m_populatedHolder.HasPropertyOfType<std::string>( "string_prop" ) );

    // Check wrong type detection
    BOOST_CHECK_EQUAL( false, m_populatedHolder.HasPropertyOfType<int>( "bool_prop" ) );
    BOOST_CHECK_EQUAL( false, m_populatedHolder.HasPropertyOfType<bool>( "int_prop" ) );
    BOOST_CHECK_EQUAL( false, m_populatedHolder.HasPropertyOfType<std::string>( "int_prop" ) );

    // Non-existing property
    BOOST_CHECK_EQUAL( false, m_populatedHolder.HasPropertyOfType<bool>( "missing_prop" ) );
}

/**
 * Test property removal
 */
BOOST_AUTO_TEST_CASE( PropertyRemoval )
{
    PROPERTY_HOLDER holder;
    holder.SetProperty( "temp_prop", 42 );

    // Property should exist
    BOOST_CHECK( holder.HasProperty( "temp_prop" ) );
    BOOST_CHECK_EQUAL( 1, holder.Size() );

    // Remove existing property
    bool removed = holder.RemoveProperty( "temp_prop" );
    BOOST_CHECK( removed );
    BOOST_CHECK_EQUAL( false, holder.HasProperty( "temp_prop" ) );
    BOOST_CHECK_EQUAL( 0, holder.Size() );

    // Try to remove non-existing property
    bool notRemoved = holder.RemoveProperty( "non_existing" );
    BOOST_CHECK_EQUAL( false, notRemoved );
}

/**
 * Test clearing all properties
 */
BOOST_AUTO_TEST_CASE( ClearProperties )
{
    PROPERTY_HOLDER holder;
    holder.SetProperty( "prop1", 1 );
    holder.SetProperty( "prop2", "two" );
    holder.SetProperty( "prop3", true );

    BOOST_CHECK_EQUAL( 3, holder.Size() );
    BOOST_CHECK_EQUAL( false, holder.Empty() );

    holder.Clear();

    BOOST_CHECK_EQUAL( 0, holder.Size() );
    BOOST_CHECK( holder.Empty() );
    BOOST_CHECK_EQUAL( false, holder.HasProperty( "prop1" ) );
    BOOST_CHECK_EQUAL( false, holder.HasProperty( "prop2" ) );
    BOOST_CHECK_EQUAL( false, holder.HasProperty( "prop3" ) );
}

/**
 * Test size and empty functionality
 */
BOOST_AUTO_TEST_CASE( SizeAndEmpty )
{
    // Empty holder
    BOOST_CHECK_EQUAL( 0, m_emptyHolder.Size() );
    BOOST_CHECK( m_emptyHolder.Empty() );

    // Populated holder (from fixture: 6 properties)
    BOOST_CHECK_EQUAL( 7, m_populatedHolder.Size() );
    BOOST_CHECK_EQUAL( false, m_populatedHolder.Empty() );

    // Add one more
    m_populatedHolder.SetProperty( "new_prop", 999 );
    BOOST_CHECK_EQUAL( 8, m_populatedHolder.Size() );

    // Remove one
    m_populatedHolder.RemoveProperty( "new_prop" );
    BOOST_CHECK_EQUAL( 7, m_populatedHolder.Size() );
}

/**
 * Test key retrieval
 */
BOOST_AUTO_TEST_CASE( KeyRetrieval )
{
    PROPERTY_HOLDER holder;
    holder.SetProperty( "key1", 1 );
    holder.SetProperty( "key2", "two" );
    holder.SetProperty( "key3", true );

    auto keys = holder.GetKeys();
    BOOST_CHECK_EQUAL( 3, keys.size() );

    // Keys should contain all our added keys (order not guaranteed)
    std::sort( keys.begin(), keys.end() );
    std::vector<std::string> expected = { "key1", "key2", "key3" };
    std::sort( expected.begin(), expected.end() );

    BOOST_CHECK_EQUAL_COLLECTIONS( keys.begin(), keys.end(), expected.begin(), expected.end() );

    // Empty holder should return empty vector
    auto emptyKeys = m_emptyHolder.GetKeys();
    BOOST_CHECK_EQUAL( 0, emptyKeys.size() );
}

/**
 * Test type information retrieval
 */
BOOST_AUTO_TEST_CASE( TypeInformation )
{
    PROPERTY_HOLDER holder;
    holder.SetProperty( "int_val", 42 );
    holder.SetProperty( "string_val", std::string( "hello" ) );

    // Existing properties should return type info
    auto intTypeInfo = holder.GetPropertyType( "int_val" );
    BOOST_CHECK( intTypeInfo.has_value() );
    BOOST_CHECK( intTypeInfo->get() == typeid( int ) );

    auto stringTypeInfo = holder.GetPropertyType( "string_val" );
    BOOST_CHECK( stringTypeInfo.has_value() );
    BOOST_CHECK( stringTypeInfo->get() == typeid( std::string ) );

    // Non-existing property should return nullopt
    auto missingTypeInfo = holder.GetPropertyType( "missing" );
    BOOST_CHECK( !missingTypeInfo.has_value() );
}

/**
 * Test complex object storage and retrieval
 */
BOOST_AUTO_TEST_CASE( ComplexObjects )
{
    PROPERTY_HOLDER holder;

    // Store custom object
    CustomData original( 123, "test data" );
    holder.SetProperty( "custom", original );

    // Retrieve and verify
    auto retrieved = holder.GetProperty<CustomData>( "custom" );
    BOOST_CHECK( retrieved.has_value() );
    BOOST_CHECK( *retrieved == original );

    // Store vector
    std::vector<std::string> stringVec = { "one", "two", "three" };
    holder.SetProperty( "string_vector", stringVec );

    auto vecRetrieved = holder.GetProperty<std::vector<std::string>>( "string_vector" );
    BOOST_CHECK( vecRetrieved.has_value() );
    BOOST_CHECK_EQUAL_COLLECTIONS( vecRetrieved->begin(), vecRetrieved->end(),
                                   stringVec.begin(), stringVec.end() );

    // Store smart pointer
    auto smartPtr = std::make_shared<int>( 456 );
    holder.SetProperty( "smart_ptr", smartPtr );

    auto ptrRetrieved = holder.GetProperty<std::shared_ptr<int>>( "smart_ptr" );
    BOOST_CHECK( ptrRetrieved.has_value() );
    BOOST_CHECK_EQUAL( **ptrRetrieved, 456 );
    BOOST_CHECK_EQUAL( ptrRetrieved->use_count(), 3 ); // Original + stored + retrieved copy
}

/**
 * Test property overwriting
 */
BOOST_AUTO_TEST_CASE( PropertyOverwriting )
{
    PROPERTY_HOLDER holder;

    // Set initial value
    holder.SetProperty( "changeable", 100 );
    BOOST_CHECK_EQUAL( holder.GetPropertyOr( "changeable", 0 ), 100 );
    BOOST_CHECK_EQUAL( 1, holder.Size() );

    // Overwrite with same type
    holder.SetProperty( "changeable", 200 );
    BOOST_CHECK_EQUAL( holder.GetPropertyOr( "changeable", 0 ), 200 );
    BOOST_CHECK_EQUAL( 1, holder.Size() ); // Size shouldn't change

    // Overwrite with different type
    holder.SetProperty( "changeable", std::string( "now a string" ) );
    BOOST_CHECK_EQUAL( holder.GetPropertyOr<std::string>( "changeable", "default" ), "now a string" );
    BOOST_CHECK_EQUAL( 1, holder.Size() ); // Size still shouldn't change

    // Original type should no longer work
    auto intResult = holder.GetProperty<int>( "changeable" );
    BOOST_CHECK( !intResult.has_value() );
}

BOOST_AUTO_TEST_SUITE_END()

/**
 * Test suite for PROPERTY_MIXIN
 */
BOOST_AUTO_TEST_SUITE( TEST_PROPERTY_MIXIN )

/**
 * Test basic PROPERTY_MIXIN functionality
 */
BOOST_AUTO_TEST_CASE( BasicMixin )
{
    TestWidget widget( "test_widget" );

    // Properties set in constructor should be available
    BOOST_CHECK_EQUAL( widget.GetPropertyOr<std::string>( "widget_name", "" ), "test_widget" );
    BOOST_CHECK_EQUAL( widget.GetPropertyOr( "default_enabled", false ), true );

    // Should be able to add new properties
    widget.SetProperty( "runtime_prop", 42 );
    BOOST_CHECK_EQUAL( widget.GetPropertyOr( "runtime_prop", 0 ), 42 );

    // Should be able to check existence
    BOOST_CHECK( widget.HasProperty( "widget_name" ) );
    BOOST_CHECK( widget.HasProperty( "default_enabled" ) );
    BOOST_CHECK( widget.HasProperty( "runtime_prop" ) );
    BOOST_CHECK_EQUAL( false, widget.HasProperty( "missing_prop" ) );
}

/**
 * Test PROPERTY_MIXIN with multiple instances
 */
BOOST_AUTO_TEST_CASE( MultipleInstances )
{
    TestWidget widget1( "widget1" );
    TestWidget widget2( "widget2" );

    // Each should have independent properties
    widget1.SetProperty( "unique_to_1", 100 );
    widget2.SetProperty( "unique_to_2", 200 );

    BOOST_CHECK_EQUAL( widget1.GetPropertyOr( "unique_to_1", 0 ), 100 );
    BOOST_CHECK_EQUAL( widget1.GetPropertyOr( "unique_to_2", 0 ), 0 ); // Should get default

    BOOST_CHECK_EQUAL( widget2.GetPropertyOr( "unique_to_1", 0 ), 0 ); // Should get default
    BOOST_CHECK_EQUAL( widget2.GetPropertyOr( "unique_to_2", 0 ), 200 );

    // Names should be different
    BOOST_CHECK_EQUAL( widget1.GetPropertyOr<std::string>( "widget_name", "" ), "widget1" );
    BOOST_CHECK_EQUAL( widget2.GetPropertyOr<std::string>( "widget_name", "" ), "widget2" );
}

/**
 * Test access to underlying PROPERTY_HOLDER
 */
BOOST_AUTO_TEST_CASE( PropertyHolderAccess )
{
    TestWidget widget( "access_test" );

    // Should be able to access the holder directly
    PROPERTY_HOLDER& holder = widget.GetPropertyHolder();
    const PROPERTY_HOLDER& constHolder = widget.GetPropertyHolder();

    // Operations on holder should affect widget properties
    holder.SetProperty( "direct_access", 999 );
    BOOST_CHECK_EQUAL( widget.GetPropertyOr( "direct_access", 0 ), 999 );

    // Should be able to use holder methods
    BOOST_CHECK( constHolder.HasProperty( "widget_name" ) );
    auto keys = constHolder.GetKeys();
    BOOST_CHECK( keys.size() >= 2 ); // At least widget_name and default_enabled
}

BOOST_AUTO_TEST_SUITE_END()

/**
 * Test suite for edge cases and error conditions
 */
BOOST_AUTO_TEST_SUITE( PropertyHolderEdgeCases )

/**
 * Test with empty string keys
 */
BOOST_AUTO_TEST_CASE( EmptyKeys )
{
    PROPERTY_HOLDER holder;

    // Empty key should work (though not recommended)
    holder.SetProperty( "", 42 );
    BOOST_CHECK( holder.HasProperty( "" ) );
    BOOST_CHECK_EQUAL( holder.GetPropertyOr( "", 0 ), 42 );

    // Very long key should work
    std::string longKey( 1000, 'x' );
    holder.SetProperty( longKey, "long key value" );
    BOOST_CHECK( holder.HasProperty( longKey ) );
}

/**
 * Test move semantics
 */
BOOST_AUTO_TEST_CASE( MoveSemantics )
{
    PROPERTY_HOLDER holder;

    // Test move construction of value
    std::vector<int> bigVector( 1000, 42 );
    holder.SetProperty( "big_vector", std::move( bigVector ) );

    // Original vector should be moved from (implementation detail, but worth testing)
    auto retrieved = holder.GetProperty<std::vector<int>>( "big_vector" );
    BOOST_CHECK( retrieved.has_value() );
    BOOST_CHECK_EQUAL( retrieved->size(), 1000 );
    BOOST_CHECK_EQUAL( (*retrieved)[0], 42 );
}

/**
 * Test with null/invalid scenarios
 */
BOOST_AUTO_TEST_CASE( NullScenarios )
{
    PROPERTY_HOLDER holder;

    // Getting from empty holder
    auto result = holder.GetProperty<int>( "anything" );
    BOOST_CHECK( !result.has_value() );

    // Getting with empty key
    auto emptyResult = holder.GetProperty<int>( "" );
    BOOST_CHECK( !emptyResult.has_value() );

    // Type info for non-existing property
    auto typeInfo = holder.GetPropertyType( "missing" );
    BOOST_CHECK( !typeInfo.has_value() );

    // Removing non-existing property
    BOOST_CHECK_EQUAL( false, holder.RemoveProperty( "non_existing" ) );
}

BOOST_AUTO_TEST_SUITE_END()

/**
 * Test suite for magic value validation and memory safety
 */
BOOST_AUTO_TEST_SUITE( PropertyHolderMagicValue )

/**
 * Test magic value validation
 */
BOOST_AUTO_TEST_CASE( MagicValueValidation )
{
    PROPERTY_HOLDER holder;

    // New holder should be valid
    BOOST_CHECK( holder.IsValid() );
    BOOST_CHECK_EQUAL( PROPERTY_HOLDER::MAGIC_VALUE, 0x50524F5048444C52ULL );

    // Should be able to use all functions when valid
    BOOST_CHECK( holder.SetProperty( "test", 42 ) );
    BOOST_CHECK( holder.HasProperty( "test" ) );
    BOOST_CHECK_EQUAL( holder.GetPropertyOr( "test", 0 ), 42 );
}

/**
 * Test safe casting functionality
 */
BOOST_AUTO_TEST_CASE( SafeCasting )
{
    // Create a PROPERTY_HOLDER
    PROPERTY_HOLDER* original = new PROPERTY_HOLDER();
    original->SetProperty( "test_prop", 123 );

    // Cast to void* (simulating client data storage)
    void* clientData = static_cast<void*>( original );

    // Safe cast should succeed
    PROPERTY_HOLDER* casted = PROPERTY_HOLDER::SafeCast( clientData );
    BOOST_CHECK_NE( nullptr, casted );
    BOOST_CHECK( casted->IsValid() );
    BOOST_CHECK_EQUAL( casted->GetPropertyOr( "test_prop", 0 ), 123 );

    // Should be the same object
    BOOST_CHECK_EQUAL( original, casted );

    // Const version should also work
    const void* constClientData = static_cast<const void*>( original );
    const PROPERTY_HOLDER* constCasted = PROPERTY_HOLDER::SafeCast( constClientData );
    BOOST_CHECK_NE( nullptr, constCasted );
    BOOST_CHECK( constCasted->IsValid() );

    delete original;
}

/**
 * Test safe casting with invalid pointers
 */
BOOST_AUTO_TEST_CASE( SafeCastingInvalid )
{
    // Null pointer should return null
    BOOST_CHECK_EQUAL( nullptr, PROPERTY_HOLDER::SafeCast( static_cast<void*>( nullptr ) ) );
    BOOST_CHECK_EQUAL( nullptr, PROPERTY_HOLDER::SafeCast( static_cast<const void*>( nullptr ) ) );

    // Random memory should return null. Use uint64_t to match the size of the magic value
    // that SafeCast reads, avoiding buffer overflow when reading the magic field.
    uint64_t randomValue = 12345;
    void* randomPtr = &randomValue;
    BOOST_CHECK_EQUAL( nullptr, PROPERTY_HOLDER::SafeCast( randomPtr ) );

    // Memory with wrong magic value should return null
    struct FakeHolder {
        uint64_t wrong_magic = 0x1234567890ABCDEFULL;
        int some_data = 42;
    };

    FakeHolder fake;
    void* fakePtr = &fake;
    BOOST_CHECK_EQUAL( nullptr, PROPERTY_HOLDER::SafeCast( fakePtr ) );
}

/**
 * Test use-after-free detection.
 *
 * This test is disabled when running with AddressSanitizer because the test
 * deliberately accesses freed memory to verify the magic value detection works.
 * ASAN will catch the use-after-free before SafeCast can check the magic value.
 */
// Detect AddressSanitizer - must use nested #if because __has_feature() can't appear
// in a preprocessor expression on compilers that don't define it
#if defined( __has_feature )
    #if __has_feature( address_sanitizer )
        #define KICAD_ASAN_ENABLED 1
    #endif
#endif

#if defined( __SANITIZE_ADDRESS__ )
    #define KICAD_ASAN_ENABLED 1
#endif

#ifndef KICAD_ASAN_ENABLED
BOOST_AUTO_TEST_CASE( UseAfterFreeDetection )
{
    PROPERTY_HOLDER* holder = new PROPERTY_HOLDER();
    holder->SetProperty( "test", 42 );

    // Should be valid before deletion
    BOOST_CHECK( holder->IsValid() );

    void* ptr = static_cast<void*>( holder );
    BOOST_CHECK_NE( nullptr, PROPERTY_HOLDER::SafeCast( ptr ) );

    // Delete the holder
    delete holder;

    // Now safe cast should fail (magic value was cleared in destructor)
    BOOST_CHECK_EQUAL( nullptr, PROPERTY_HOLDER::SafeCast( ptr ) );
}
#endif

#undef KICAD_ASAN_ENABLED

/**
 * Test client data creation and deletion helpers
 */
BOOST_AUTO_TEST_CASE( ClientDataHelpers )
{
    PROPERTY_HOLDER* holder = new PROPERTY_HOLDER();
    BOOST_CHECK_NE( nullptr, holder );
    BOOST_CHECK( holder->IsValid() );

    // Should be able to use it
    BOOST_CHECK( holder->SetProperty( "client_test", 999 ) );
    BOOST_CHECK_EQUAL( holder->GetPropertyOr( "client_test", 0 ), 999 );

    // Safe delete should succeed
    BOOST_CHECK( PROPERTY_HOLDER::SafeDelete( reinterpret_cast<void*>( holder ) ) );

    // Safe delete of invalid pointer should fail. Use uint64_t to match the size of the magic
    // value that SafeCast reads, avoiding buffer overflow when reading the magic field.
    uint64_t randomData = 42;
    BOOST_CHECK_EQUAL( false, PROPERTY_HOLDER::SafeDelete( &randomData ) );
    BOOST_CHECK_EQUAL( false, PROPERTY_HOLDER::SafeDelete( static_cast<void*>( nullptr ) ) );
}

/**
 * Test copy and move constructors preserve magic value
 */
BOOST_AUTO_TEST_CASE( CopyMoveConstructors )
{
    PROPERTY_HOLDER original;
    original.SetProperty( "test", 42 );
    BOOST_CHECK( original.IsValid() );

    // Copy constructor
    PROPERTY_HOLDER copied( original );
    BOOST_CHECK( copied.IsValid() );
    BOOST_CHECK_EQUAL( copied.GetPropertyOr( "test", 0 ), 42 );

    // Move constructor
    PROPERTY_HOLDER moved( std::move( original ) );
    BOOST_CHECK( moved.IsValid() );
    BOOST_CHECK_EQUAL( moved.GetPropertyOr( "test", 0 ), 42 );

    // Original should still be valid (magic value preserved)
    BOOST_CHECK( original.IsValid() );
}

/**
 * Test assignment operators preserve magic value
 */
BOOST_AUTO_TEST_CASE( AssignmentOperators )
{
    PROPERTY_HOLDER source;
    source.SetProperty( "source_prop", 123 );

    PROPERTY_HOLDER target;
    target.SetProperty( "target_prop", 456 );

    BOOST_CHECK( source.IsValid() );
    BOOST_CHECK( target.IsValid() );

    // Copy assignment
    target = source;
    BOOST_CHECK( target.IsValid() );
    BOOST_CHECK_EQUAL( target.GetPropertyOr( "source_prop", 0 ), 123 );
    BOOST_CHECK_EQUAL( false, target.HasProperty( "target_prop" ) ); // Should be overwritten

    // Move assignment
    PROPERTY_HOLDER another;
    another.SetProperty( "another_prop", 789 );

    target = std::move( another );
    BOOST_CHECK( target.IsValid() );
    BOOST_CHECK_EQUAL( target.GetPropertyOr( "another_prop", 0 ), 789 );
    BOOST_CHECK( another.IsValid() ); // Magic value preserved
}

/**
 * Test behavior of methods when object is invalid
 */
BOOST_AUTO_TEST_CASE( InvalidObjectBehavior )
{
    // Create a fake PROPERTY_HOLDER with wrong magic value
    struct FakePropertyHolder {
        uint64_t wrong_magic = 0xDEADBEEF;
        std::unordered_map<std::string, std::any> fake_properties;
    };

    FakePropertyHolder fake;
    PROPERTY_HOLDER* fakeHolder = reinterpret_cast<PROPERTY_HOLDER*>( &fake );

    // All methods should fail gracefully
    BOOST_CHECK_EQUAL( false, fakeHolder->IsValid() );
    BOOST_CHECK_EQUAL( false, fakeHolder->SetProperty( "test", 42 ) );
    BOOST_CHECK_EQUAL( false, fakeHolder->HasProperty( "anything" ) );
    BOOST_CHECK_EQUAL( false, fakeHolder->RemoveProperty( "anything" ) );
    BOOST_CHECK_EQUAL( false, fakeHolder->Clear() );
    BOOST_CHECK_EQUAL( 0, fakeHolder->Size() );
    BOOST_CHECK( fakeHolder->Empty() ); // Returns true for invalid objects
    BOOST_CHECK_EQUAL( 0, fakeHolder->GetKeys().size() );
    BOOST_CHECK_EQUAL( false, fakeHolder->HasPropertyOfType<int>( "anything" ) );

    // GetProperty should return nullopt
    auto result = fakeHolder->GetProperty<int>( "anything" );
    BOOST_CHECK( !result.has_value() );

    // GetPropertyOr should return default
    BOOST_CHECK_EQUAL( fakeHolder->GetPropertyOr( "anything", 999 ), 999 );

    // GetPropertyType should return nullopt
    auto typeInfo = fakeHolder->GetPropertyType( "anything" );
    BOOST_CHECK( !typeInfo.has_value() );
}

BOOST_AUTO_TEST_SUITE_END()