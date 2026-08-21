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

/**
 * @file
 * Regression test for https://gitlab.com/kicad/code/kicad/-/issues/25209
 */

#include <qa_utils/wx_utils/unit_test_utils.h>

#include <lib_symbol.h>
#include <sch_io/sch_io_mgr.h>
#include <sch_io/kicad_sexpr/sch_io_kicad_sexpr.h>

#include <wx/filename.h>


class DERIVED_BODY_STYLES_FIXTURE
{
public:
    DERIVED_BODY_STYLES_FIXTURE()
    {
        m_tempDir = wxFileName::CreateTempFileName( wxS( "kicad_test_" ) );
        wxRemoveFile( m_tempDir );
        wxFileName::Mkdir( m_tempDir );

        m_libPath = wxFileName( m_tempDir, wxS( "test_lib.kicad_sym" ) ).GetFullPath();
    }

    ~DERIVED_BODY_STYLES_FIXTURE()
    {
        if( wxFileName::DirExists( m_tempDir ) )
            wxFileName::Rmdir( m_tempDir, wxPATH_RMDIR_RECURSIVE );
    }

    const wxString& GetLibPath() const { return m_libPath; }

private:
    wxString m_tempDir;
    wxString m_libPath;
};


BOOST_FIXTURE_TEST_SUITE( DerivedSymbolBodyStyles, DERIVED_BODY_STYLES_FIXTURE )


/**
 * A derived symbol owns no drawings, so its body styles are its root symbol's.  The symbol
 * properties dialog let the user set De Morgan on a derived symbol; the body style select box
 * then offered a second style with nothing in it, and the change silently evaporated on save
 * because the derived symbol is written as a bare (extends ...) with no body_styles token.
 */
BOOST_AUTO_TEST_CASE( BodyStylesResolveThroughParent )
{
    LIB_SYMBOL root( wxS( "Root" ) );
    LIB_SYMBOL derived( wxS( "Derived" ) );

    derived.SetParent( &root );

    BOOST_CHECK_EQUAL( derived.GetBodyStyleCount(), 1 );

    // What the dialog used to do to a derived symbol
    derived.SetHasDeMorganBodyStyles( true );

    BOOST_CHECK( !derived.HasDeMorganBodyStyles() );
    BOOST_CHECK_EQUAL( derived.GetBodyStyleCount(), 1 );

    derived.SetBodyStyleNames( { wxS( "Bogus1" ), wxS( "Bogus2" ), wxS( "Bogus3" ) } );

    BOOST_CHECK_EQUAL( derived.GetBodyStyleCount(), 1 );

    // The root gains De Morgan styles, and the derived symbol follows it
    root.SetBodyStyleCount( 2, false, true );
    root.SetHasDeMorganBodyStyles( true );

    BOOST_CHECK( derived.HasDeMorganBodyStyles() );
    BOOST_CHECK( derived.IsMultiBodyStyle() );
    BOOST_CHECK_EQUAL( derived.GetBodyStyleCount(), 2 );

    // Custom body styles resolve the same way, names included
    root.SetHasDeMorganBodyStyles( false );
    root.SetBodyStyleNames( { wxS( "Standard" ), wxS( "Alternate" ), wxS( "Simplified" ) } );

    BOOST_CHECK( !derived.HasDeMorganBodyStyles() );
    BOOST_CHECK_EQUAL( derived.GetBodyStyleCount(), 3 );
    BOOST_CHECK_EQUAL( derived.GetBodyStyleDescription( 3, false ), wxS( "Simplified" ) );

    // Draw items common to every body style carry a body style of 0, which must not index
    // one before the start of the names
    BOOST_CHECK_EQUAL( derived.GetBodyStyleDescription( 0, false ), wxS( "?" ) );
    BOOST_CHECK_EQUAL( root.GetBodyStyleDescription( 0, false ), wxS( "?" ) );
}


/**
 * Everything the disabled Units & Body Styles controls display has to resolve to the root
 * symbol's value, or a derived symbol is shown settings it does not have and can no longer
 * correct.
 */
BOOST_AUTO_TEST_CASE( UnitMetadataResolvesThroughParent )
{
    LIB_SYMBOL root( wxS( "Root" ) );
    LIB_SYMBOL derived( wxS( "Derived" ) );

    derived.SetParent( &root );

    root.SetUnitCount( 4, true );
    root.LockUnits( true );
    root.GetUnitDisplayNames()[2] = wxS( "Buffer" );

    // GetUnitCount() always resolved through the parent; these did not
    BOOST_CHECK_EQUAL( derived.GetUnitCount(), 4 );
    BOOST_CHECK( derived.IsMultiUnit() );
    BOOST_CHECK( derived.UnitsLocked() );
    BOOST_CHECK_EQUAL( derived.GetUnitDisplayName( 2, false ), wxS( "Buffer" ) );

    // Units the root has not named still fall back to the sub-reference letter
    BOOST_CHECK_EQUAL( derived.GetUnitDisplayName( 3, false ), wxS( "C" ) );
}


/**
 * The in-memory state of a derived symbol must survive a save/reload, which it cannot do if it
 * reports body styles the file format has no way to store for it.
 */
BOOST_AUTO_TEST_CASE( BodyStylesSurviveRoundTrip )
{
    {
        LIB_SYMBOL root( wxS( "Root" ) );
        root.SetBodyStyleCount( 2, false, true );
        root.SetHasDeMorganBodyStyles( true );

        LIB_SYMBOL derived( wxS( "Derived" ) );
        derived.SetParent( &root );

        // The dialog's write-back for the "single body style" radio button
        derived.SetBodyStyleCount( 1, false, false );
        derived.SetHasDeMorganBodyStyles( false );
        derived.SetBodyStyleNames( {} );

        BOOST_CHECK_EQUAL( derived.GetBodyStyleCount(), 2 );

        IO_RELEASER<SCH_IO> plugin( SCH_IO_MGR::FindPlugin( SCH_IO_MGR::SCH_KICAD ) );
        plugin->CreateLibrary( GetLibPath() );
        plugin->SaveSymbol( GetLibPath(), new LIB_SYMBOL( root ) );
        plugin->SaveSymbol( GetLibPath(), new LIB_SYMBOL( derived ) );
        plugin->SaveLibrary( GetLibPath() );
    }

    IO_RELEASER<SCH_IO> plugin( SCH_IO_MGR::FindPlugin( SCH_IO_MGR::SCH_KICAD ) );
    LIB_SYMBOL*         loadedRoot = plugin->LoadSymbol( GetLibPath(), wxS( "Root" ) );
    LIB_SYMBOL*         loadedDerived = plugin->LoadSymbol( GetLibPath(), wxS( "Derived" ) );

    BOOST_REQUIRE( loadedRoot );
    BOOST_REQUIRE( loadedDerived );

    // LoadSymbol does not link parents without a library table
    loadedDerived->SetParent( loadedRoot );

    BOOST_CHECK( loadedRoot->HasDeMorganBodyStyles() );
    BOOST_CHECK( loadedDerived->HasDeMorganBodyStyles() );
    BOOST_CHECK_EQUAL( loadedDerived->GetBodyStyleCount(), 2 );
}


BOOST_AUTO_TEST_SUITE_END()
