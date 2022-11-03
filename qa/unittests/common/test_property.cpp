/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2020 CERN
 * @author Maciej Suminski <maciej.suminski@cern.ch>
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
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <qa_utils/wx_utils/unit_test_utils.h>
#include <wx/gdicmn.h>      // wxPoint

#include <inspectable.h>
#include <properties/property_mgr.h>

using namespace std;

class A : public INSPECTABLE
{
public:
    virtual void setA( int a ) = 0;
    virtual int getA() const = 0;
    virtual const int& getA2() const { return m_a; }

    virtual void setPoint( const wxPoint& p ) { m_p = p; }
    void setPoint2( wxPoint& p ) { m_p = p; }
    void setPoint3( wxPoint p ) { m_p = p; }
    void setPoint4( wxPoint p ) { m_p = p; }

    const wxPoint& getPoint() const { return m_p; }
    wxPoint getPoint2() const { return m_p; }
    wxPoint getPoint3() { return m_p; }
    const wxPoint& getPoint4() const { return m_p; }

protected:
    int m_a = 0;
    wxPoint m_p;
};

class B : public A
{
public:
    void setA( int a ) override { m_a = a; }
    int getA() const override { return m_a; }

    void setC( int a ) { m_c = a; }
    int getC() const { return m_c; }

private:
    int m_c = 0;
};

class C 
{
public:
    bool getBool() const { return m_bool; }
    void setBool( bool a ) { m_bool = a; }

    int getNew() const { return m_m; }
    void setNew( int m ) { m_m = m; }

    int m_m     = 0;
    bool m_bool = false;
};

enum enum_glob { TEST1 = 0, TEST2 = 1, TEST3 = 4 };

class D : public A, public C
{
public:
    enum enum_class { TESTA = 0, TESTB = 1, TESTC = 4 };

    // note 2x factor
    virtual void setA( int a ) override { m_aa = 2 * a; }
    virtual int getA() const override { return m_aa; }

    enum_glob getGlobEnum() const { return m_enum_glob; }
    void setGlobEnum( enum_glob val ) { m_enum_glob = val; }

    enum_class getClassEnum() const { return m_enum_class; }
    void setClassEnum( enum_class val ) { m_enum_class = val; }

    void setCond( int a ) { m_cond = a; }
    int getCond() const { return m_cond; }

    enum_glob m_enum_glob;
    enum_class m_enum_class;
    int m_aa   = 0;
    int m_cond = 0;
};

class E : public D
{
};

static struct ENUM_GLOB_DESC
{
    ENUM_GLOB_DESC()
    {
        ENUM_MAP<enum_glob>::Instance()
                .Map( enum_glob::TEST1, "TEST1" )
                .Map( enum_glob::TEST2, "TEST2" )
                .Map( enum_glob::TEST3, "TEST3" );
    }
} _ENUM_GLOB_DESC;

ENUM_TO_WXANY( enum_glob );

static struct CLASS_A_DESC
{
    CLASS_A_DESC()
    {
        PROPERTY_MANAGER& propMgr = PROPERTY_MANAGER::Instance();
        propMgr.AddProperty( new PROPERTY<A, int>( "A", &A::setA, &A::getA ) );
        propMgr.AddProperty( new PROPERTY<A, int>( "A2", &A::setA, &A::getA2 ) );
        propMgr.AddProperty( new PROPERTY<A, wxPoint>( "point", &A::setPoint, &A::getPoint ) );
        propMgr.AddProperty( new PROPERTY<A, wxPoint>( "point2", &A::setPoint, &A::getPoint2 ) );

        // TODO non-const getters are not supported
        //propMgr.AddProperty( new PROPERTY<A, wxPoint>( "point3", &A::setPoint3, &A::getPoint3 ) );
        propMgr.AddProperty( new PROPERTY<A, wxPoint>( "point4", &A::setPoint4, &A::getPoint4 ) );
    }
} _CLASS_A_DESC;

static struct CLASS_B_DESC
{
    CLASS_B_DESC()
    {
        PROPERTY_MANAGER& propMgr = PROPERTY_MANAGER::Instance();
        propMgr.InheritsAfter( TYPE_HASH( B ), TYPE_HASH( A ) );
        propMgr.AddProperty( new PROPERTY<B, int>( "C", &B::setC, &B::getC ) );
    }
} _CLASS_B_DESC;

static struct CLASS_C_DESC
{
    CLASS_C_DESC()
    {
        PROPERTY_MANAGER& propMgr = PROPERTY_MANAGER::Instance();
        propMgr.AddProperty( new PROPERTY<C, bool>( "bool", &C::setBool, &C::getBool ) );
        propMgr.AddProperty( new PROPERTY<C, int>( "new", &C::setNew, &C::getNew ) );
    }
} _CLASS_C_DESC;

static struct CLASS_D_DESC
{
    CLASS_D_DESC()
    {
        ENUM_MAP<D::enum_class>::Instance()
                .Map( D::enum_class::TESTA, "TESTA" )
                .Map( D::enum_class::TESTB, "TESTB" )
                .Map( D::enum_class::TESTC, "TESTC" );

        PROPERTY_MANAGER& propMgr = PROPERTY_MANAGER::Instance();
        propMgr.AddProperty( new PROPERTY_ENUM<D, enum_glob>( "enumGlob", &D::setGlobEnum, &D::getGlobEnum ) );
        propMgr.AddProperty( new PROPERTY_ENUM<D, D::enum_class>( "enumClass", &D::setClassEnum, &D::getClassEnum ) );
        propMgr.AddProperty( new PROPERTY<D, wxPoint, A>( "point_alias", &D::setPoint, &D::getPoint ) );

        propMgr.ReplaceProperty( TYPE_HASH( C ), "bool",
                new PROPERTY<D, bool, C>( "replaced_bool", &D::setBool, &D::getBool ) );

        // lines below are needed to indicate multiple inheritance
        propMgr.AddTypeCast( new TYPE_CAST<D, A> );
        propMgr.AddTypeCast( new TYPE_CAST<D, C> );
        propMgr.InheritsAfter( TYPE_HASH( D ), TYPE_HASH( A ) );
        propMgr.InheritsAfter( TYPE_HASH( D ), TYPE_HASH( C ) );

        auto cond = new PROPERTY<D, int>( "cond", &D::setCond, &D::getCond );
        cond->SetAvailableFunc( [=](INSPECTABLE* aItem)->bool {
                return *aItem->Get<enum_glob>( "enumGlob" ) == enum_glob::TEST1;
        } );
        propMgr.AddProperty( cond );
    }
} _CLASS_D_DESC;

static struct CLASS_E_DESC
{
    CLASS_E_DESC()
    {
        wxArrayInt values;
        values.Add( enum_glob::TEST1 );
        values.Add( enum_glob::TEST3 );
        wxArrayString labels;
        labels.Add( "T1" );
        labels.Add( "T3" );
        wxPGChoices newChoices( labels, values );

        PROPERTY_MANAGER& propMgr = PROPERTY_MANAGER::Instance();
        auto prop = new PROPERTY_ENUM<E, enum_glob, D>( "enumGlob",
                &E::setGlobEnum, &E::getGlobEnum );
        prop->SetChoices( newChoices );
        propMgr.ReplaceProperty( TYPE_HASH( D ), "enumGlob", prop );
    }
} _CLASS_E_DESC;

ENUM_TO_WXANY( D::enum_class );


struct PropertiesFixture
{
    PropertiesFixture() :
        ptr( nullptr ),
        propMgr( PROPERTY_MANAGER::Instance() )
    {
    }

    B b;
    D d;
    A* ptr;
    PROPERTY_MANAGER& propMgr;
};

BOOST_FIXTURE_TEST_SUITE( Properties, PropertiesFixture )

BOOST_AUTO_TEST_CASE( Init )
{
    propMgr.Rebuild();
}

// Basic Set() & Get()
BOOST_AUTO_TEST_CASE( SetGet )
{
    ptr = &b;
    ptr->Set( "A", 100 );
    ptr->Set( "point", wxPoint( 100, 200 ) );
    BOOST_CHECK_EQUAL( *ptr->Get<int>( "A" ), 100 );
    BOOST_CHECK_EQUAL( *ptr->Get<wxPoint>( "point" ), wxPoint( 100, 200 ) );

    ptr = &d;
    ptr->Set( "enumGlob", enum_glob::TEST2 );
    ptr->Set( "enumClass", D::enum_class::TESTC );
    BOOST_CHECK_EQUAL( *ptr->Get<enum_glob>( "enumGlob" ), enum_glob::TEST2 );
    BOOST_CHECK_EQUAL( *ptr->Get<D::enum_class>( "enumClass" ), D::enum_class::TESTC );
}

// Virtual methods
BOOST_AUTO_TEST_CASE( VirtualMethods )
{
    // D::setA() saves a doubled value, while B::setA() saves unmodified value
    ptr = &b;
    ptr->Set( "A", 23 );
    BOOST_CHECK_EQUAL( *ptr->Get<int>( "A" ), 23 ); // unmodified == 23

    ptr = &d;
    ptr->Set( "A", 23 );
    BOOST_CHECK_EQUAL( *ptr->Get<int>( "A" ), 46 ); // doubled == 46
}

// Non-existing properties
BOOST_AUTO_TEST_CASE( NotexistingProperties )
{
    ptr = &d;
    BOOST_CHECK_EQUAL( ptr->Set<int>( "does not exist", 5 ), false );
    BOOST_CHECK_EQUAL( static_cast<bool>( ptr->Get<int>( "neither" ) ), false );
}

// Request data using incorrect type
BOOST_AUTO_TEST_CASE( IncorrectType )
{
    ptr = &d;
    BOOST_CHECK_THROW( ptr->Get<wxPoint>( "A" ), std::invalid_argument );
}

// Type-casting (for types with multiple inheritance)
BOOST_AUTO_TEST_CASE( TypeCasting )
{
    ptr = &d;
    A* D_to_A = static_cast<A*>( propMgr.TypeCast( ptr, TYPE_HASH( D ), TYPE_HASH( A ) ) );
    BOOST_CHECK_EQUAL( D_to_A, dynamic_cast<A*>( ptr ) );

    C* D_to_C = static_cast<C*>( propMgr.TypeCast( ptr, TYPE_HASH( D ), TYPE_HASH( C ) ) );
    BOOST_CHECK_EQUAL( D_to_C, dynamic_cast<C*>( ptr ) );
}

BOOST_AUTO_TEST_CASE( EnumGlob )
{
    PROPERTY_BASE* prop = propMgr.GetProperty( TYPE_HASH( D ), "enumGlob" );
    BOOST_CHECK( prop->HasChoices() );

    wxArrayInt values;
    values.Add( enum_glob::TEST1 );
    values.Add( enum_glob::TEST2 );
    values.Add( enum_glob::TEST3 );
    wxArrayString labels;
    labels.Add( "TEST1" );
    labels.Add( "TEST2" );
    labels.Add( "TEST3" );

    const wxPGChoices& v = prop->Choices();
    BOOST_CHECK_EQUAL( v.GetCount(), values.GetCount() );
    BOOST_CHECK_EQUAL( v.GetCount(), labels.GetCount() );

    for (unsigned int i = 0; i < values.GetCount(); ++i )
    {
        BOOST_CHECK_EQUAL( v.GetValue( i ), values[i] );
    }

    for (unsigned int i = 0; i < labels.GetCount(); ++i )
    {
        BOOST_CHECK_EQUAL( v.GetLabel( i ), labels[i] );
    }

    D item ;
    wxString str;

    item.setGlobEnum( static_cast<enum_glob>( -1 ) );
    wxAny any = item.Get( prop );
    BOOST_CHECK_EQUAL( any.GetAs<wxString>( &str ), false );

    item.setGlobEnum( enum_glob::TEST1 );
    any = item.Get( prop );
    BOOST_CHECK_EQUAL( any.GetAs<wxString>( &str ), true );
}

BOOST_AUTO_TEST_CASE( EnumClass )
{
    PROPERTY_BASE* prop = propMgr.GetProperty( TYPE_HASH( D ), "enumClass" );
    BOOST_CHECK( prop->HasChoices() );

    wxArrayInt values;
    values.Add( D::enum_class::TESTA );
    values.Add( D::enum_class::TESTB );
    values.Add( D::enum_class::TESTC );
    wxArrayString labels;
    labels.Add( "TESTA" );
    labels.Add( "TESTB" );
    labels.Add( "TESTC" );

    const wxPGChoices& v = prop->Choices();
    BOOST_CHECK_EQUAL( v.GetCount(), values.GetCount() );
    BOOST_CHECK_EQUAL( v.GetCount(), labels.GetCount() );

    for (unsigned int i = 0; i < values.GetCount(); ++i )
    {
        BOOST_CHECK_EQUAL( v.GetValue( i ), values[i] );
    }

    for (unsigned int i = 0; i < labels.GetCount(); ++i )
    {
        BOOST_CHECK_EQUAL( v.GetLabel( i ), labels[i] );
    }
}

// Tests conditional properties (which may depend on values or other properties)
BOOST_AUTO_TEST_CASE( Availability )
{
    PROPERTY_BASE* propCond = propMgr.GetProperty( TYPE_HASH( D ), "cond" );
    ptr = &d;

    // "cond" property is available only when "a" field is greater than 50  //TODO fix desc
    d.setGlobEnum( enum_glob::TEST3 );
    BOOST_CHECK( !propCond->Available( ptr ) );

    d.setGlobEnum( enum_glob::TEST1 );
    BOOST_CHECK( propCond->Available( ptr ) );
}

// Using a different name for a parent property
BOOST_AUTO_TEST_CASE( Alias )
{
    ptr = &d;

    ptr->Set( "point", wxPoint( 100, 100 ) );
    BOOST_CHECK_EQUAL( *ptr->Get<wxPoint>( "point" ), wxPoint( 100, 100 ) );
    BOOST_CHECK_EQUAL( *ptr->Get<wxPoint>( "point_alias" ), wxPoint( 100, 100 ) );

    ptr->Set( "point_alias", wxPoint( 300, 300 ) );
    BOOST_CHECK_EQUAL( *ptr->Get<wxPoint>( "point" ), wxPoint( 300, 300 ) );
    BOOST_CHECK_EQUAL( *ptr->Get<wxPoint>( "point_alias" ), wxPoint( 300, 300 ) );
}

// Property renaming
BOOST_AUTO_TEST_CASE( Rename )
{
    PROPERTY_BASE* prop;

    prop = propMgr.GetProperty( TYPE_HASH( D ), "bool" );
    BOOST_CHECK_EQUAL( prop, nullptr );

    prop = propMgr.GetProperty( TYPE_HASH( D ), "replaced_bool" );
    BOOST_CHECK( prop );
}

// Different subset of enum values for a property
BOOST_AUTO_TEST_CASE( AlternativeEnum )
{
    PROPERTY_BASE* prop = propMgr.GetProperty( TYPE_HASH( E ), "enumGlob" );
    BOOST_CHECK( prop->HasChoices() );

    wxArrayInt values;
    values.Add( enum_glob::TEST1 );
    values.Add( enum_glob::TEST3 );
    wxArrayString labels;
    labels.Add( "T1" );
    labels.Add( "T3" );

    const wxPGChoices& v = prop->Choices();
    BOOST_CHECK_EQUAL( v.GetCount(), values.GetCount() );
    BOOST_CHECK_EQUAL( v.GetCount(), labels.GetCount() );

    for (unsigned int i = 0; i < values.GetCount(); ++i )
    {
        BOOST_CHECK_EQUAL( v.GetValue( i ), values[i] );
    }

    for (unsigned int i = 0; i < labels.GetCount(); ++i )
    {
        BOOST_CHECK_EQUAL( v.GetLabel( i ), labels[i] );
    }
}

BOOST_AUTO_TEST_SUITE_END()
