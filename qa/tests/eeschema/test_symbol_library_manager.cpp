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
    LIB_SYMBOL* symbol = new LIB_SYMBOL( wxS( "Symbol" ) );
    std::unique_ptr<SCH_SCREEN> screen( new SCH_SCREEN() );

    SYMBOL_BUFFER buffer( symbol, std::move( screen ) );

    BOOST_CHECK( !buffer.IsModified() );
    BOOST_CHECK( buffer.GetSymbol() == symbol );
    BOOST_CHECK( *buffer.GetOriginal() == *symbol );

    buffer.GetScreen()->SetContentModified();
    BOOST_CHECK( buffer.IsModified() );

    LIB_SYMBOL* originalSymbol = new LIB_SYMBOL( wxS( "OriginalSymbol" ) );
    buffer.SetOriginal( originalSymbol );

    BOOST_CHECK( buffer.GetOriginal() == originalSymbol );
    BOOST_CHECK( *buffer.GetOriginal() == *originalSymbol );
    BOOST_CHECK( *buffer.GetSymbol() != *buffer.GetOriginal() );

    // Allocated memory is cleaned up by the SYMBOL_BUFFER object dtor.
}


/**
 * Test the LIB_BUFFER object.
 */
BOOST_AUTO_TEST_CASE( LibBuffer )
{
    wxArrayString symbolNames;
    LIB_BUFFER libBuffer( wxS( "TestLibrary" ) );

    BOOST_CHECK( !libBuffer.IsModified() );
    BOOST_CHECK_EQUAL( libBuffer.GetHash(), 1 );

    LIB_SYMBOL* parentSymbol1 = new LIB_SYMBOL( wxS( "Parent1" ) );

    BOOST_CHECK_EQUAL( libBuffer.GetSymbol( parentSymbol1->GetName() ), nullptr );

    parentSymbol1->GetValueField().SetText( parentSymbol1->GetName() );
    libBuffer.CreateBuffer( parentSymbol1, new SCH_SCREEN() );
    BOOST_CHECK( libBuffer.GetSymbol( parentSymbol1->GetName() ) == parentSymbol1 );
    BOOST_CHECK( !libBuffer.HasDerivedSymbols( parentSymbol1->GetName() ) );
    BOOST_CHECK_EQUAL( libBuffer.GetHash(), 2 );

    libBuffer.GetSymbolNames( symbolNames );
    BOOST_CHECK_EQUAL( symbolNames.GetCount(), 1 );
    BOOST_CHECK_EQUAL( symbolNames[0], parentSymbol1->GetName() );

    symbolNames.Clear();
    libBuffer.GetSymbolNames( symbolNames, SYMBOL_NAME_FILTER::ROOT_ONLY );
    BOOST_CHECK_EQUAL( symbolNames.GetCount(), 1 );
    BOOST_CHECK_EQUAL( symbolNames[0], parentSymbol1->GetName() );

    symbolNames.Clear();
    libBuffer.GetSymbolNames( symbolNames, SYMBOL_NAME_FILTER::DERIVED_ONLY );
    BOOST_CHECK_EQUAL( symbolNames.GetCount(), 0 );

    LIB_SYMBOL* childSymbol1 = new LIB_SYMBOL( wxS( "Child1" ) );
    childSymbol1->SetParent( parentSymbol1 );
    childSymbol1->GetValueField().SetText( childSymbol1->GetName() );
    libBuffer.CreateBuffer( childSymbol1, new SCH_SCREEN() );
    BOOST_CHECK( libBuffer.GetSymbol( childSymbol1->GetName() ) == childSymbol1 );
    BOOST_CHECK( libBuffer.HasDerivedSymbols( parentSymbol1->GetName() ) );
    BOOST_CHECK_EQUAL( libBuffer.GetHash(), 3 );

    symbolNames.Clear();
    libBuffer.GetSymbolNames( symbolNames );
    BOOST_CHECK_EQUAL( symbolNames.GetCount(), 2 );
    BOOST_CHECK_EQUAL( symbolNames[0], parentSymbol1->GetName() );
    BOOST_CHECK_EQUAL( symbolNames[1], childSymbol1->GetName() );

    symbolNames.Clear();
    libBuffer.GetSymbolNames( symbolNames, SYMBOL_NAME_FILTER::DERIVED_ONLY );
    BOOST_CHECK_EQUAL( symbolNames.GetCount(), 1 );
    BOOST_CHECK_EQUAL( symbolNames[0], childSymbol1->GetName() );

    symbolNames.Clear();
    BOOST_CHECK_EQUAL( libBuffer.GetDerivedSymbolNames( parentSymbol1->GetName(),
                                                        symbolNames ), 1 );
    BOOST_CHECK_EQUAL( symbolNames[0], childSymbol1->GetName() );

    std::shared_ptr<SYMBOL_BUFFER> buf = libBuffer.GetBuffer( parentSymbol1->GetName() );
    LIB_SYMBOL* tmp = new LIB_SYMBOL( *parentSymbol1 );
    tmp->GetDescriptionField().SetText( wxS( "Description" ) );
    libBuffer.UpdateBuffer( buf, tmp );
    BOOST_CHECK_EQUAL( libBuffer.GetHash(), 4 );
    BOOST_CHECK( *libBuffer.GetSymbol( parentSymbol1->GetName() ) == *tmp );

    const bool deletedOk = libBuffer.DeleteBuffer( *buf );
    BOOST_CHECK( deletedOk );
    BOOST_CHECK( libBuffer.GetBuffers().empty() );
}


BOOST_AUTO_TEST_SUITE_END()
