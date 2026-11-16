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
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <boost/test/unit_test.hpp>
#include <eda_item.h>
#include <properties/property.h>


namespace
{
class TEST_EDA_ITEM : public EDA_ITEM
{
public:
    TEST_EDA_ITEM() : EDA_ITEM( NOT_USED ) {}

    wxString GetClass() const override { return wxT( "TEST_EDA_ITEM" ); }

    EDA_ITEM* Clone() const override { return new TEST_EDA_ITEM(); }
};


PROPERTY_BASE* findCustomProperty( const EDA_ITEM& aItem, const wxString& aKey )
{
    for( PROPERTY_BASE* prop : aItem.GetCustomPropertiesAsInspectables() )
    {
        if( prop->Name() == aKey )
            return prop;
    }

    return nullptr;
}
} // namespace


BOOST_AUTO_TEST_SUITE( EdaItem )


BOOST_AUTO_TEST_CASE( RemovedCustomPropertyKeepsItsDescriptor )
{
    TEST_EDA_ITEM item;

    item.SetCustomProperty( wxS( "Foo" ), wxS( "1" ) );
    PROPERTY_BASE* foo = findCustomProperty( item, wxS( "Foo" ) );
    BOOST_REQUIRE( foo );

    item.RemoveCustomProperty( wxS( "Foo" ) );

    // A freed descriptor's chunk goes straight to the next one allocated, so a pointer check alone cannot fail
    item.SetCustomProperty( wxS( "Bar" ), wxS( "2" ) );
    BOOST_REQUIRE( findCustomProperty( item, wxS( "Bar" ) ) );

    item.SetCustomProperty( wxS( "Foo" ), wxS( "1" ) );

    BOOST_CHECK( findCustomProperty( item, wxS( "Foo" ) ) == foo );
    BOOST_CHECK( foo->Name() == wxS( "Foo" ) );
}


BOOST_AUTO_TEST_SUITE_END()
