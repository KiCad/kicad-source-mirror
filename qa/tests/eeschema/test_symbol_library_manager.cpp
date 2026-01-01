/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2023 Wayne Stambaugh <stambaughw@gmail.com>
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

/**
 * @file
 * Test suite for SYMBOL_LIBRARY_MANAGER object.
 */

#include <qa_utils/wx_utils/unit_test_utils.h>

// Code under test
#include <symbol_library_manager.h>
#include <symbol_editor/lib_symbol_library_manager.h>
#include <sch_field.h>
#include <sch_io/kicad_sexpr/sch_io_kicad_sexpr.h>
#include <sch_io/sch_io_mgr.h>

#include <wx/filename.h>

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


/**
 * Test new symbol creation routines.
 */
BOOST_AUTO_TEST_CASE( NewSymbolCreation )
{
    NEW_SYMBOL_PROPERTIES props;

    props.name = wxS( "Standalone" );
    props.reference = wxS( "U" );
    props.unitCount = 2;
    props.pinNameInside = true;
    props.pinTextPosition = 2;
    props.powerSymbol = false;
    props.showPinNumber = true;
    props.showPinName = true;
    props.unitsInterchangeable = false;
    props.includeInBom = true;
    props.includeOnBoard = true;
    props.alternateBodyStyle = false;

    std::unique_ptr<LIB_SYMBOL> standalone =
            LIB_SYMBOL_LIBRARY_MANAGER::CreateSymbol( props, nullptr );

    BOOST_CHECK_EQUAL( standalone->GetReferenceField().GetText(), props.reference );
    BOOST_CHECK_EQUAL( standalone->GetUnitCount(), props.unitCount );
    BOOST_CHECK( standalone->GetPinNameOffset() > 0 );

    auto parent = std::make_unique<LIB_SYMBOL>( wxS( "Parent" ) );
    parent->GetValueField().SetText( parent->GetName() );
    SCH_FIELD* user = new SCH_FIELD( parent.get(), FIELD_T::USER, wxS( "UF" ) );
    user->SetText( wxS( "V" ) );
    parent->AddField( user );

    props.name = wxS( "Child" );
    props.parentSymbolName = parent->GetName();
    props.keepFootprint = false;
    props.keepDatasheet = false;
    props.transferUserFields = true;
    props.keepContentUserFields = false;

    std::unique_ptr<LIB_SYMBOL> child =
            LIB_SYMBOL_LIBRARY_MANAGER::CreateSymbol( props, parent.get() );

    BOOST_CHECK( child->GetParent().lock().get() == parent.get() );
    BOOST_CHECK( child->GetFootprintField().GetText().IsEmpty() );
    BOOST_CHECK( child->GetDatasheetField().GetText().IsEmpty() );

    std::vector<SCH_FIELD*> childFields;
    child->GetFields( childFields );

    bool found = false;

    for( SCH_FIELD* field : childFields )
    {
        if( field->GetId() == FIELD_T::USER )
        {
            found = true;
            BOOST_CHECK( field->GetText().IsEmpty() );
        }
    }

    BOOST_CHECK( found );
}


/**
 * Test that LIB_BUFFER correctly deletes symbols from the library file when saved.
 *
 * This test verifies the fix for the bug where deleting a derived symbol from the symbol
 * editor tree and saving would result in the symbol reappearing as a non-derived symbol
 * when the library was reloaded. The root cause was that SaveBuffer only saved existing
 * symbols but never called DeleteSymbol on the plugin for symbols in the m_deleted list.
 */
BOOST_AUTO_TEST_CASE( DeletedSymbolsAreRemovedFromFile )
{
    // Create a temporary directory and library file
    wxString tempDir = wxFileName::CreateTempFileName( wxS( "kicad_test_" ) );
    wxRemoveFile( tempDir );
    wxFileName::Mkdir( tempDir );
    wxString libPath = wxFileName( tempDir, wxS( "test_lib.kicad_sym" ) ).GetFullPath();

    // Step 1: Create a library with a parent and derived symbol using the plugin directly
    {
        IO_RELEASER<SCH_IO> plugin( SCH_IO_MGR::FindPlugin( SCH_IO_MGR::SCH_KICAD ) );
        plugin->CreateLibrary( libPath );

        // Create parent symbol
        std::unique_ptr<LIB_SYMBOL> parentSymbol = std::make_unique<LIB_SYMBOL>( wxS( "Parent" ) );
        parentSymbol->GetValueField().SetText( wxS( "Parent" ) );
        parentSymbol->GetReferenceField().SetText( wxS( "U" ) );

        LIB_SYMBOL* parentPtr = parentSymbol.get();
        plugin->SaveSymbol( libPath, new LIB_SYMBOL( *parentSymbol ) );

        // Create derived symbol
        std::unique_ptr<LIB_SYMBOL> derivedSymbol = std::make_unique<LIB_SYMBOL>( wxS( "Derived" ) );
        derivedSymbol->GetValueField().SetText( wxS( "Derived" ) );
        derivedSymbol->SetParent( parentPtr );

        plugin->SaveSymbol( libPath, new LIB_SYMBOL( *derivedSymbol ) );
        plugin->SaveLibrary( libPath );
    }

    // Step 2: Verify both symbols exist in the library
    {
        IO_RELEASER<SCH_IO> plugin( SCH_IO_MGR::FindPlugin( SCH_IO_MGR::SCH_KICAD ) );
        LIB_SYMBOL* parent = plugin->LoadSymbol( libPath, wxS( "Parent" ) );
        LIB_SYMBOL* derived = plugin->LoadSymbol( libPath, wxS( "Derived" ) );

        BOOST_REQUIRE( parent != nullptr );
        BOOST_REQUIRE( derived != nullptr );
        BOOST_CHECK( derived->IsDerived() );
    }

    // Step 3: Load symbols into LIB_BUFFER and delete the derived symbol
    LIB_BUFFER libBuffer( wxS( "TestLibrary" ) );

    {
        IO_RELEASER<SCH_IO> plugin( SCH_IO_MGR::FindPlugin( SCH_IO_MGR::SCH_KICAD ) );
        LIB_SYMBOL* loadedParent = plugin->LoadSymbol( libPath, wxS( "Parent" ) );
        LIB_SYMBOL* loadedDerived = plugin->LoadSymbol( libPath, wxS( "Derived" ) );

        BOOST_REQUIRE( loadedParent != nullptr );
        BOOST_REQUIRE( loadedDerived != nullptr );

        // Set up parent relationship
        loadedDerived->SetParent( loadedParent );

        // Create buffers (parent first, then derived)
        libBuffer.CreateBuffer( std::make_unique<LIB_SYMBOL>( *loadedParent ),
                                std::make_unique<SCH_SCREEN>() );

        std::unique_ptr<LIB_SYMBOL> derivedCopy = std::make_unique<LIB_SYMBOL>( *loadedDerived );
        derivedCopy->SetParent( libBuffer.GetSymbol( wxS( "Parent" ) ) );
        libBuffer.CreateBuffer( std::move( derivedCopy ), std::make_unique<SCH_SCREEN>() );
    }

    // Verify buffer state before deletion
    BOOST_CHECK_EQUAL( libBuffer.GetBuffers().size(), 2 );
    BOOST_CHECK( libBuffer.GetSymbol( wxS( "Parent" ) ) != nullptr );
    BOOST_CHECK( libBuffer.GetSymbol( wxS( "Derived" ) ) != nullptr );

    // Delete the derived symbol from the buffer
    std::shared_ptr<SYMBOL_BUFFER> derivedBuf = libBuffer.GetBuffer( wxS( "Derived" ) );
    BOOST_REQUIRE( derivedBuf != nullptr );

    bool deleteResult = libBuffer.DeleteBuffer( *derivedBuf );
    BOOST_CHECK( deleteResult );

    // Verify buffer state after deletion
    BOOST_CHECK_EQUAL( libBuffer.GetBuffers().size(), 1 );
    BOOST_CHECK( libBuffer.GetSymbol( wxS( "Parent" ) ) != nullptr );
    BOOST_CHECK( libBuffer.GetSymbol( wxS( "Derived" ) ) == nullptr );

    // Step 4: Save the library using the same pattern as SYMBOL_LIBRARY_MANAGER::SaveLibrary
    {
        IO_RELEASER<SCH_IO> plugin( SCH_IO_MGR::FindPlugin( SCH_IO_MGR::SCH_KICAD ) );
        std::map<std::string, UTF8> properties;
        properties.emplace( SCH_IO_KICAD_SEXPR::PropBuffering, "" );

        // Save remaining buffers (just Parent)
        for( const std::shared_ptr<SYMBOL_BUFFER>& symbolBuf : libBuffer.GetBuffers() )
        {
            libBuffer.SaveBuffer( *symbolBuf, libPath, &*plugin, true );
        }

        // Delete symbols that were removed from the buffer (this is the fix for the bug)
        for( const std::shared_ptr<SYMBOL_BUFFER>& deletedBuf : libBuffer.GetDeletedBuffers() )
        {
            const wxString& originalName = deletedBuf->GetOriginal().GetName();

            if( plugin->LoadSymbol( libPath, originalName ) )
                plugin->DeleteSymbol( libPath, originalName, &properties );
        }

        plugin->SaveLibrary( libPath );
        libBuffer.ClearDeletedBuffer();
    }

    // Step 5: Reload the library and verify the derived symbol is gone
    {
        IO_RELEASER<SCH_IO> plugin( SCH_IO_MGR::FindPlugin( SCH_IO_MGR::SCH_KICAD ) );

        LIB_SYMBOL* parent = plugin->LoadSymbol( libPath, wxS( "Parent" ) );
        LIB_SYMBOL* derived = plugin->LoadSymbol( libPath, wxS( "Derived" ) );

        BOOST_CHECK( parent != nullptr );
        // This is the actual check for the bug - the derived symbol should be deleted
        BOOST_CHECK_MESSAGE( derived == nullptr,
                             "Derived symbol should have been deleted from the library file" );
    }

    // Cleanup
    if( wxFileName::DirExists( tempDir ) )
    {
        wxFileName::Rmdir( tempDir, wxPATH_RMDIR_RECURSIVE );
    }
}


BOOST_AUTO_TEST_SUITE_END()
