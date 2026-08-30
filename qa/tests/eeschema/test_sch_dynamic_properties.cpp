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

#include <qa_utils/wx_utils/unit_test_utils.h>

#include <diff_merge/property_diff.h>

#include <schematic.h>
#include <sch_field.h>
#include <sch_screen.h>
#include <sch_sheet.h>
#include <sch_symbol.h>
#include <settings/settings_manager.h>

#include <algorithm>
#include <optional>
#include <vector>


using namespace KICAD_DIFF;


static const PROPERTY_DELTA* findDelta( const std::vector<PROPERTY_DELTA>& aDeltas, const wxString& aName )
{
    for( const PROPERTY_DELTA& d : aDeltas )
    {
        if( d.name == aName )
            return &d;
    }

    return nullptr;
}


struct SCH_DYNAMIC_FIXTURE
{
    SCH_DYNAMIC_FIXTURE()
    {
        m_mgr.LoadProject( "" );
        m_schematic = std::make_unique<SCHEMATIC>( &m_mgr.Prj() );
        m_schematic->Reset();

        m_screen = new SCH_SCREEN( m_schematic.get() );
        m_sheet  = new SCH_SHEET( m_schematic.get() );
        m_sheet->SetScreen( m_screen );
        m_schematic->AddTopLevelSheet( m_sheet );
    }

    SCH_SYMBOL* makeSymbol( const std::optional<wxString>& aFieldValue )
    {
        SCH_SYMBOL* symbol = new SCH_SYMBOL();
        m_screen->Append( symbol );

        if( aFieldValue.has_value() )
        {
            symbol->AddField( SCH_FIELD( symbol, FIELD_T::USER, wxS( "Manufacturer" ) ) );
            symbol->GetField( wxS( "Manufacturer" ) )->SetText( *aFieldValue );
        }

        return symbol;
    }

    SETTINGS_MANAGER           m_mgr;
    std::unique_ptr<SCHEMATIC> m_schematic;
    SCH_SCREEN*                m_screen;
    SCH_SHEET*                 m_sheet;
};


BOOST_FIXTURE_TEST_SUITE( SchDynamicProperties, SCH_DYNAMIC_FIXTURE )


BOOST_AUTO_TEST_CASE( SymbolDynamicPropertyOrder )
{
    SCH_SYMBOL symbol;

    symbol.AddField( SCH_FIELD( &symbol, FIELD_T::USER, wxS( "Zulu" ) ) );
    symbol.AddField( SCH_FIELD( &symbol, FIELD_T::USER, wxS( "alpha" ) ) );
    symbol.AddField( SCH_FIELD( &symbol, FIELD_T::USER, wxS( "Mike" ) ) );

    SCH_FIELD privateField( &symbol, FIELD_T::USER, wxS( "Secret" ) );
    privateField.SetPrivate( true );
    symbol.AddField( privateField );

    std::vector<PROPERTY_BASE*> dynamicProps = symbol.GetDynamicProperties();

    std::vector<wxString> names;
    names.reserve( dynamicProps.size() );

    for( PROPERTY_BASE* prop : dynamicProps )
        names.push_back( prop->Name() );

    BOOST_CHECK( std::ranges::none_of( names,
                                       []( const wxString& n ) { return n == wxS( "Secret" ); } ) );

    // Mandatory non-private fields come first (in GetFields() order), then the
    // user fields sorted case-insensitively by name.
    bool                  seenUser = false;
    wxString              lastUser;
    std::vector<wxString> userNames;

    for( const wxString& name : names )
    {
        const SCH_FIELD* field = symbol.GetField( name );

        if( field && field->IsMandatory() )
        {
            BOOST_CHECK_MESSAGE( !seenUser, "Mandatory field '" + name + "' appears after a user field" );
        }
        else
        {
            if( field )
                BOOST_CHECK( !field->IsPrivate() );

            if( seenUser )
                BOOST_CHECK_MESSAGE( lastUser.CmpNoCase( name ) <= 0,
                                     "User fields not sorted: " + lastUser + " before " + name );

            lastUser = name;
            userNames.push_back( name );
            seenUser = true;
        }
    }

    const std::vector<wxString> expectedUser = { wxS( "alpha" ), wxS( "Mike" ), wxS( "Zulu" ) };
    BOOST_CHECK( userNames == expectedUser );
}


BOOST_AUTO_TEST_CASE( SheetDynamicPropertyOrder )
{
    SCH_SHEET sheet;

    sheet.AddField( SCH_FIELD( &sheet, FIELD_T::SHEET_USER, wxS( "delta" ) ) );
    sheet.AddField( SCH_FIELD( &sheet, FIELD_T::SHEET_USER, wxS( "Alpha" ) ) );
    sheet.AddField( SCH_FIELD( &sheet, FIELD_T::SHEET_USER, wxS( "charlie" ) ) );

    std::vector<PROPERTY_BASE*> dynamicProps = sheet.GetDynamicProperties();

    std::vector<wxString> names;
    names.reserve( dynamicProps.size() );

    for( PROPERTY_BASE* prop : dynamicProps )
        names.push_back( prop->Name() );

    bool                  seenUser = false;
    wxString              lastUser;
    std::vector<wxString> userNames;

    for( const wxString& name : names )
    {
        const SCH_FIELD* field = sheet.GetField( name );

        if( field && field->IsMandatory() )
        {
            BOOST_CHECK_MESSAGE( !seenUser,
                                 "Mandatory field '" + name + "' appears after a user field" );
        }
        else
        {
            if( seenUser )
            {
                BOOST_CHECK_MESSAGE( lastUser.CmpNoCase( name ) <= 0,
                                     "User fields not sorted: " + lastUser + " before " + name );
            }

            lastUser = name;
            userNames.push_back( name );
            seenUser = true;
        }
    }

    const std::vector<wxString> expectedUser = { wxS( "Alpha" ), wxS( "charlie" ), wxS( "delta" ) };
    BOOST_CHECK( userNames == expectedUser );
}


BOOST_AUTO_TEST_CASE( DynamicSkipsStaticFields )
{
    SCH_SYMBOL symbol;

    PROPERTY_MANAGER& pm = PROPERTY_MANAGER::Instance();
    const std::vector<PROPERTY_BASE*>& staticProps = pm.GetProperties( TYPE_HASH( SCH_SYMBOL ) );

    for( PROPERTY_BASE* dynamic : symbol.GetDynamicProperties() )
    {
        std::vector<wxString> staticNames;

        for( PROPERTY_BASE* staticProp : staticProps )
            staticNames.push_back( staticProp->Name() );

        BOOST_CHECK_MESSAGE( std::ranges::find( staticNames, dynamic->Name() ) == staticNames.end(),
                             "Dynamic property '" + dynamic->Name() + "' duplicates a statically-registered property" );
    }
}


BOOST_AUTO_TEST_CASE( FieldGetterReturnsNullForAbsentField )
{
    SCH_SYMBOL* symWith    = makeSymbol( wxString( wxS( "ACME" ) ) );
    SCH_SYMBOL* symWithout = makeSymbol( std::nullopt );

    std::vector<PROPERTY_DELTA> deltas = DiffItemProperties( symWithout, symWith );
    const PROPERTY_DELTA*       d = findDelta( deltas, wxS( "Manufacturer" ) );

    BOOST_REQUIRE( d );
    BOOST_CHECK( d->before.GetType() == DIFF_VALUE::T::NONE );
    BOOST_CHECK( d->after.GetType() == DIFF_VALUE::T::STRING );
}


BOOST_AUTO_TEST_CASE( FieldGetterReturnsEmptyStringForEmptyField )
{
    SCH_SYMBOL* symA = makeSymbol( wxString( wxS( "" ) ) );     // Manufacturer == ""
    SCH_SYMBOL* symB = makeSymbol( wxString( wxS( "ACME" ) ) ); // Manufacturer == "ACME"

    std::vector<PROPERTY_DELTA> deltas = DiffItemProperties( symA, symB );
    const PROPERTY_DELTA*       d = findDelta( deltas, wxS( "Manufacturer" ) );

    BOOST_REQUIRE( d );

    BOOST_CHECK( d->before.GetType() == DIFF_VALUE::T::STRING );
    BOOST_CHECK( d->after.GetType() == DIFF_VALUE::T::STRING );
    BOOST_CHECK( d->before.GetType() != DIFF_VALUE::T::NONE );
}


BOOST_AUTO_TEST_CASE( BothSidesAbsentFieldProducesNoDelta )
{
    SCH_SYMBOL* symA = makeSymbol( std::nullopt );
    SCH_SYMBOL* symB = makeSymbol( std::nullopt );

    std::vector<PROPERTY_DELTA> deltas = DiffItemProperties( symA, symB );
    BOOST_CHECK( !findDelta( deltas, wxS( "Manufacturer" ) ) );
}


BOOST_AUTO_TEST_CASE( BothSidesEmptyFieldProducesNoDelta )
{
    SCH_SYMBOL* symA = makeSymbol( wxString( wxS( "" ) ) );
    SCH_SYMBOL* symB = makeSymbol( wxString( wxS( "" ) ) );

    std::vector<PROPERTY_DELTA> deltas = DiffItemProperties( symA, symB );
    BOOST_CHECK( !findDelta( deltas, wxS( "Manufacturer" ) ) );
}


BOOST_AUTO_TEST_CASE( DynamicFieldSetterCreatesField )
{
    SCH_SYMBOL symbol;

    symbol.AddField( SCH_FIELD( &symbol, FIELD_T::USER, wxS( "TempField" ) ) );

    PROPERTY_BASE* prop = nullptr;

    for( PROPERTY_BASE* p : symbol.GetDynamicProperties() )
    {
        if( p->Name() == wxS( "TempField" ) )
            prop = p;
    }

    BOOST_REQUIRE( prop );

    symbol.RemoveField( wxS( "TempField" ) );
    BOOST_REQUIRE( !symbol.GetField( wxS( "TempField" ) ) );

    // Re-applying the cached property must recreate the removed field.
    wxAny value( wxString( wxS( "newval" ) ) );
    BOOST_CHECK( symbol.Set( prop, value ) );

    SCH_FIELD* recreated = symbol.GetField( wxS( "TempField" ) );
    BOOST_REQUIRE( recreated );
    BOOST_CHECK_EQUAL( recreated->GetText(), wxS( "newval" ) );
}


BOOST_AUTO_TEST_CASE( AddedFieldSurfacesAsDelta )
{
    SCH_SYMBOL* symA = makeSymbol( std::nullopt );           // no Manufacturer
    SCH_SYMBOL* symB = makeSymbol( wxString( wxS( "ACME" ) ) );

    // Field added between revisions: present only on the "after" side.
    std::vector<PROPERTY_DELTA> deltas = DiffItemProperties( symA, symB );
    const PROPERTY_DELTA*       d = findDelta( deltas, wxS( "Manufacturer" ) );

    BOOST_REQUIRE( d );
    BOOST_CHECK( d->before.GetType() == DIFF_VALUE::T::NONE );
    BOOST_CHECK( d->after.GetType() == DIFF_VALUE::T::STRING );

    // Reverse direction (field removed): present on the "before" side.
    std::vector<PROPERTY_DELTA> reverse = DiffItemProperties( symB, symA );
    const PROPERTY_DELTA*       rd = findDelta( reverse, wxS( "Manufacturer" ) );

    BOOST_REQUIRE( rd );
    BOOST_CHECK( rd->before.GetType() == DIFF_VALUE::T::STRING );
    BOOST_CHECK( rd->after.GetType() == DIFF_VALUE::T::NONE );
}


BOOST_AUTO_TEST_CASE( CustomPropertiesExposedAsDynamic )
{
    SCH_SYMBOL symbol;

    symbol.SetCustomProperty( wxS( "MPN" ), wxS( "12345" ) );

    PROPERTY_BASE* mpn = nullptr;

    for( PROPERTY_BASE* p : symbol.GetDynamicProperties() )
    {
        if( p->Name() == wxS( "MPN" ) )
            mpn = p;
    }

    BOOST_REQUIRE( mpn );

    wxAny v = symbol.Get( mpn );
    wxString text;

    BOOST_REQUIRE( v.GetAs( &text ) );
    BOOST_CHECK_EQUAL( text, wxS( "12345" ) );

    wxAny newVal( wxString( wxS( "99999" ) ) );
    BOOST_CHECK( symbol.Set( mpn, newVal ) );
    BOOST_CHECK_EQUAL( symbol.GetCustomProperties().at( wxS( "MPN" ) ), wxS( "99999" ) );
}


BOOST_AUTO_TEST_SUITE_END()
