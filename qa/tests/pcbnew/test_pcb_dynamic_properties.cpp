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
#include <diff_merge/kicad_diff_types.h>

#include <board.h>
#include <footprint.h>
#include <pcb_field.h>

#include <algorithm>
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


BOOST_AUTO_TEST_SUITE( PcbDynamicProperties )


BOOST_AUTO_TEST_CASE( FootprintDynamicPropertyOrder )
{
    BOARD     board;
    FOOTPRINT fp( &board );

    fp.Add( new PCB_FIELD( &fp, FIELD_T::USER, wxS( "Supplier" ) ) );
    fp.Add( new PCB_FIELD( &fp, FIELD_T::USER, wxS( "MPN" ) ) );
    fp.Add( new PCB_FIELD( &fp, FIELD_T::USER, wxS( "Color" ) ) );

    PCB_FIELD* privateField = new PCB_FIELD( &fp, FIELD_T::USER, wxS( "Secret" ) );
    privateField->SetPrivate( true );
    fp.Add( privateField );

    std::vector<PROPERTY_BASE*> dynamicProps = fp.GetDynamicProperties();

    std::vector<wxString> names;
    names.reserve( dynamicProps.size() );

    for( PROPERTY_BASE* prop : dynamicProps )
        names.push_back( prop->Name() );

    BOOST_CHECK( std::ranges::none_of( names,
                                       []( const wxString& n ) { return n == wxS( "Secret" ); } ) );

    bool                  seenUser = false;
    wxString              lastUser;
    std::vector<wxString> userNames;

    for( const wxString& name : names )
    {
        PCB_FIELD* field = fp.GetField( name );

        if( field && field->IsMandatory() )
        {
            BOOST_CHECK_MESSAGE( !seenUser,
                                 "Mandatory field '" + name + "' appears after a user field" );
        }
        else
        {
            if( field )
                BOOST_CHECK( !field->IsPrivate() );

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

    const std::vector<wxString> expectedUser = { wxS( "Color" ), wxS( "MPN" ), wxS( "Supplier" ) };
    BOOST_CHECK( userNames == expectedUser );
}


BOOST_AUTO_TEST_CASE( DynamicSkipsStaticFields )
{
    BOARD     board;
    FOOTPRINT fp( &board );

    PROPERTY_MANAGER& pm = PROPERTY_MANAGER::Instance();
    const std::vector<PROPERTY_BASE*>& staticProps = pm.GetProperties( TYPE_HASH( FOOTPRINT ) );

    std::vector<wxString> staticNames;

    for( PROPERTY_BASE* staticProp : staticProps )
        staticNames.push_back( staticProp->Name() );

    for( PROPERTY_BASE* dynamic : fp.GetDynamicProperties() )
    {
        BOOST_CHECK_MESSAGE( std::ranges::find( staticNames, dynamic->Name() ) == staticNames.end(),
                             "Dynamic property '" + dynamic->Name() + "' duplicates a statically-registered property" );
    }
}


BOOST_AUTO_TEST_CASE( FootprintFieldGetterAbsentVsEmpty )
{
    BOARD     boardA;
    BOARD     boardB;
    FOOTPRINT fpA( &boardA );
    FOOTPRINT fpB( &boardB );

    // Field absent on A, present on B -> delta with before=NONE, after=STRING.
    PCB_FIELD* fieldB = new PCB_FIELD( &fpB, FIELD_T::USER, wxS( "Supplier" ) );
    fieldB->SetText( wxS( "ACME" ) );
    fpB.Add( fieldB );

    std::vector<PROPERTY_DELTA> deltas = DiffItemProperties( &fpA, &fpB );
    const PROPERTY_DELTA*       d = findDelta( deltas, wxS( "Supplier" ) );

    BOOST_REQUIRE( d );
    BOOST_CHECK( d->before.GetType() == DIFF_VALUE::T::NONE );
    BOOST_CHECK( d->after.GetType() == DIFF_VALUE::T::STRING );

    // Field empty on A, non-empty on B -> delta with before=STRING, after=STRING.
    PCB_FIELD* fieldA = new PCB_FIELD( &fpA, FIELD_T::USER, wxS( "Supplier" ) );
    fieldA->SetText( wxS( "" ) );
    fpA.Add( fieldA );

    std::vector<PROPERTY_DELTA> deltas2 = DiffItemProperties( &fpA, &fpB );
    const PROPERTY_DELTA*       d2 = findDelta( deltas2, wxS( "Supplier" ) );

    BOOST_REQUIRE( d2 );
    BOOST_CHECK( d2->before.GetType() == DIFF_VALUE::T::STRING );
    BOOST_CHECK( d2->after.GetType() == DIFF_VALUE::T::STRING );
}


BOOST_AUTO_TEST_CASE( ApplyResolvesDynamicFieldProperty )
{
    BOARD     boardSrc;
    BOARD     boardTgt;
    FOOTPRINT src( &boardSrc );
    FOOTPRINT tgt( &boardTgt );

    PCB_FIELD* srcField = new PCB_FIELD( &src, FIELD_T::USER, wxS( "MPN" ) );
    srcField->SetText( wxS( "12345" ) );
    src.Add( srcField );

    PCB_FIELD* tgtField = new PCB_FIELD( &tgt, FIELD_T::USER, wxS( "MPN" ) );
    tgtField->SetText( wxS( "original" ) );
    tgt.Add( tgtField );

    PROPERTY_RESOLUTION res;
    res.name = wxS( "MPN" );
    res.kind = PROP_RES::OURS;

    PROPERTY_APPLY_COUNTS counts = ApplyPropertyResolutions( &tgt, { res }, &src, nullptr, nullptr );

    BOOST_CHECK_EQUAL( counts.applied, 1u );
    BOOST_CHECK_EQUAL( counts.failed, 0u );

    PCB_FIELD* resolved = tgt.GetField( wxS( "MPN" ) );
    BOOST_REQUIRE( resolved );
    BOOST_CHECK_EQUAL( resolved->GetText(), wxS( "12345" ) );
}


BOOST_AUTO_TEST_CASE( ApplyFailsForNonexistentField )
{
    BOARD     boardTgt;
    BOARD     boardSrc;
    FOOTPRINT tgt( &boardTgt );
    FOOTPRINT src( &boardSrc );

    PROPERTY_RESOLUTION res;
    res.name = wxS( "NoSuchField" );
    res.kind = PROP_RES::OURS;

    PROPERTY_APPLY_COUNTS counts = ApplyPropertyResolutions( &tgt, { res }, &src, nullptr, nullptr );

    BOOST_CHECK_EQUAL( counts.applied, 0u );
    BOOST_CHECK_EQUAL( counts.failed, 1u );
}


BOOST_AUTO_TEST_CASE( DynamicPropertyCacheStable )
{
    BOARD     board;
    FOOTPRINT fp( &board );

    fp.Add( new PCB_FIELD( &fp, FIELD_T::USER, wxS( "MPN" ) ) );

    PROPERTY_BASE* first  = nullptr;
    PROPERTY_BASE* second = nullptr;

    for( PROPERTY_BASE* p : fp.GetDynamicProperties() )
    {
        if( p->Name() == wxS( "MPN" ) )
            first = p;
    }

    for( PROPERTY_BASE* p : fp.GetDynamicProperties() )
    {
        if( p->Name() == wxS( "MPN" ) )
            second = p;
    }

    BOOST_REQUIRE( first );
    BOOST_REQUIRE( second );

    // The per-object cache must return the same descriptor; recreating it would
    // invalidate pointers callers (e.g. the properties panel) hold onto.
    BOOST_CHECK_EQUAL( first, second );
}


BOOST_AUTO_TEST_CASE( DynamicPropertyLookupCaseInsensitive )
{
    BOARD     board;
    FOOTPRINT fp( &board );

    fp.Add( new PCB_FIELD( &fp, FIELD_T::USER, wxS( "MPN" ) ) );

    PROPERTY_MANAGER& pm = PROPERTY_MANAGER::Instance();

    PROPERTY_BASE* lower = pm.GetProperty( &fp, wxS( "mpn" ) );
    PROPERTY_BASE* upper = pm.GetProperty( &fp, wxS( "MPN" ) );

    BOOST_REQUIRE( lower );
    BOOST_REQUIRE( upper );
    BOOST_CHECK_EQUAL( lower, upper );

    // The manager's dynamic lookup must agree with the object's own cache.
    PROPERTY_BASE* dyn = nullptr;

    for( PROPERTY_BASE* p : fp.GetDynamicProperties() )
    {
        if( p->Name() == wxS( "MPN" ) )
            dyn = p;
    }

    BOOST_REQUIRE( dyn );
    BOOST_CHECK_EQUAL( lower, dyn );
}


BOOST_AUTO_TEST_CASE( CustomPropertiesExposedAsDynamic )
{
    BOARD     board;
    FOOTPRINT fp( &board );

    fp.SetCustomProperty( wxS( "MPN" ), wxS( "12345" ) );
    fp.SetCustomProperty( wxS( "Supplier" ), wxS( "ACME" ) );

    PROPERTY_BASE* mpn      = nullptr;
    PROPERTY_BASE* supplier = nullptr;

    for( PROPERTY_BASE* p : fp.GetDynamicProperties() )
    {
        if( p->Name() == wxS( "MPN" ) )
            mpn = p;
        else if( p->Name() == wxS( "Supplier" ) )
            supplier = p;
    }

    BOOST_REQUIRE( mpn );
    BOOST_REQUIRE( supplier );

    wxAny v = fp.Get( mpn );
    wxString text;

    BOOST_REQUIRE( v.GetAs( &text ) );
    BOOST_CHECK_EQUAL( text, wxS( "12345" ) );

    wxAny newVal( wxString( wxS( "99999" ) ) );
    BOOST_CHECK( fp.Set( supplier, newVal ) );
    BOOST_CHECK_EQUAL( fp.GetCustomProperties().at( wxS( "Supplier" ) ), wxS( "99999" ) );
}


BOOST_AUTO_TEST_SUITE_END()
