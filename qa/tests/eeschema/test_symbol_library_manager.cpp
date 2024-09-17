/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2023 Wayne Stambaugh <stambaughw@gmail.com>
 * Copyright (C) 2023 KiCad Developers, see AUTHORS.txt for contributors.
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

/**
 * @file
 * Test suite for SYMBOL_LIBRARY_MANAGER object.
 */

#include <qa_utils/wx_utils/unit_test_utils.h>

// Code under test
#include <symbol_library_manager.h>

class SYMBOL_LIBRARY_MANAGER_TEST_FIXTURE
{
public:
    SYMBOL_LIBRARY_MANAGER_TEST_FIXTURE()
    {
    }
};


BOOST_FIXTURE_TEST_SUITE( SymbolLibraryManager, SYMBOL_LIBRARY_MANAGER_TEST_FIXTURE )


/**
 * Test the SYMBOL_BUFFER object.
 */
BOOST_AUTO_TEST_CASE( SymbolBuffer )
{
    std::unique_ptr<LIB_SYMBOL> symbol = std::make_unique<LIB_SYMBOL>( wxS( "Symbol" ) );
    std::unique_ptr<SCH_SCREEN> screen = std::make_unique<SCH_SCREEN>();

    LIB_SYMBOL& symbolRef = *symbol;

    SYMBOL_BUFFER buffer( std::move( symbol ), std::move( screen ) );

    BOOST_CHECK( !buffer.IsModified() );
    BOOST_CHECK( &buffer.GetSymbol() == &symbolRef );
    BOOST_CHECK( buffer.GetOriginal() == symbolRef );

    buffer.GetScreen()->SetContentModified();
    BOOST_CHECK( buffer.IsModified() );

    std::unique_ptr<LIB_SYMBOL> originalSymbol =
            std::make_unique<LIB_SYMBOL>( wxS( "OriginalSymbol" ) );
    LIB_SYMBOL& originalSymbolRef = *originalSymbol;

    buffer.SetOriginal( std::move( originalSymbol ) );

    BOOST_CHECK( &buffer.GetOriginal() == &originalSymbolRef );
    BOOST_CHECK( buffer.GetOriginal() == originalSymbolRef );
    BOOST_CHECK( &buffer.GetSymbol() != &buffer.GetOriginal() );
}


/**
 * Test the LIB_BUFFER object.
 */
BOOST_AUTO_TEST_CASE( LibBuffer )
{
    wxArrayString symbolNames;
    LIB_BUFFER    libBuffer( wxS( "TestLibrary" ) );

    BOOST_CHECK( !libBuffer.IsModified() );
    BOOST_CHECK_EQUAL( libBuffer.GetHash(), 1 );

    auto        parentSymbol1 = std::make_unique<LIB_SYMBOL>( wxS( "Parent1" ) );
    // A but clunky, but real code would get symbol from the LIB_BUFFER
    // via GetSymbol etc, rather than retaining a reference after construction.
    LIB_SYMBOL& parentSymbol1Ref = *parentSymbol1;

    BOOST_CHECK_EQUAL( libBuffer.GetSymbol( parentSymbol1->GetName() ), nullptr );

    parentSymbol1->GetValueField().SetText( parentSymbol1->GetName() );
    libBuffer.CreateBuffer( std::move( parentSymbol1 ), std::make_unique<SCH_SCREEN>() );
    BOOST_CHECK( libBuffer.GetSymbol( parentSymbol1Ref.GetName() ) == &parentSymbol1Ref );
    BOOST_CHECK( !libBuffer.HasDerivedSymbols( parentSymbol1Ref.GetName() ) );
    BOOST_CHECK_EQUAL( libBuffer.GetHash(), 2 );

    libBuffer.GetSymbolNames( symbolNames );
    BOOST_CHECK_EQUAL( symbolNames.GetCount(), 1 );
    BOOST_CHECK_EQUAL( symbolNames[0], parentSymbol1Ref.GetName() );

    symbolNames.Clear();
    libBuffer.GetSymbolNames( symbolNames, SYMBOL_NAME_FILTER::ROOT_ONLY );
    BOOST_CHECK_EQUAL( symbolNames.GetCount(), 1 );
    BOOST_CHECK_EQUAL( symbolNames[0], parentSymbol1Ref.GetName() );

    symbolNames.Clear();
    libBuffer.GetSymbolNames( symbolNames, SYMBOL_NAME_FILTER::DERIVED_ONLY );
    BOOST_CHECK_EQUAL( symbolNames.GetCount(), 0 );

    auto        childSymbol1 = std::make_unique<LIB_SYMBOL>( wxS( "Child1" ) );
    LIB_SYMBOL& childSymbol1Ref = *childSymbol1;

    childSymbol1->SetParent( &parentSymbol1Ref );
    childSymbol1->GetValueField().SetText( childSymbol1->GetName() );
    libBuffer.CreateBuffer( std::move( childSymbol1 ), std::make_unique<SCH_SCREEN>() );
    BOOST_CHECK( libBuffer.GetSymbol( childSymbol1Ref.GetName() ) == &childSymbol1Ref );
    BOOST_CHECK( libBuffer.HasDerivedSymbols( parentSymbol1Ref.GetName() ) );
    BOOST_CHECK_EQUAL( libBuffer.GetHash(), 3 );

    symbolNames.Clear();
    libBuffer.GetSymbolNames( symbolNames );
    BOOST_CHECK_EQUAL( symbolNames.GetCount(), 2 );
    BOOST_CHECK_EQUAL( symbolNames[0], parentSymbol1Ref.GetName() );
    BOOST_CHECK_EQUAL( symbolNames[1], childSymbol1Ref.GetName() );

    symbolNames.Clear();
    libBuffer.GetSymbolNames( symbolNames, SYMBOL_NAME_FILTER::DERIVED_ONLY );
    BOOST_CHECK_EQUAL( symbolNames.GetCount(), 1 );
    BOOST_CHECK_EQUAL( symbolNames[0], childSymbol1Ref.GetName() );

    symbolNames.Clear();
    BOOST_CHECK_EQUAL( libBuffer.GetDerivedSymbolNames( parentSymbol1Ref.GetName(), symbolNames ),
                       1 );
    BOOST_CHECK_EQUAL( symbolNames[0], childSymbol1Ref.GetName() );

    std::shared_ptr<SYMBOL_BUFFER> buf = libBuffer.GetBuffer( parentSymbol1Ref.GetName() );

    LIB_SYMBOL tmp( parentSymbol1Ref );
    tmp.GetDescriptionField().SetText( wxS( "Description" ) );

    libBuffer.UpdateBuffer( *buf, tmp );
    BOOST_CHECK_EQUAL( libBuffer.GetHash(), 4 );
    BOOST_CHECK( *libBuffer.GetSymbol( parentSymbol1Ref.GetName() ) == tmp );

    const bool deletedOk = libBuffer.DeleteBuffer( *buf );
    BOOST_CHECK( deletedOk );
    BOOST_CHECK( libBuffer.GetBuffers().empty() );
}


BOOST_AUTO_TEST_SUITE_END()
