/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2024 The KiCad Developers, see AUTHORS.txt for contributors.
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

#include <qa_utils/wx_utils/unit_test_utils.h>
#include <boost/test/unit_test.hpp>

#include <pcb_shape.h>
#include <properties/property_mgr.h>
#include <properties/property.h>
#include <i18n_utility.h>

BOOST_AUTO_TEST_CASE( PCBShapeCornerRadius )
{
    PCB_SHAPE shape( nullptr, SHAPE_T::RECTANGLE );
    shape.SetStart( VECTOR2I( 0, 0 ) );
    shape.SetEnd( VECTOR2I( 1000, 600 ) );
    shape.SetCornerRadius( 200 );
    BOOST_CHECK_EQUAL( shape.GetCornerRadius(), 200 );

    shape.SetCornerRadius( 400 );
    BOOST_CHECK_EQUAL( shape.GetCornerRadius(), 300 );

    shape.SetCornerRadius( -10 );
    BOOST_CHECK_EQUAL( shape.GetCornerRadius(), 0 );
}

BOOST_AUTO_TEST_CASE( PCBShapeCornerRadiusValidation )
{
    PCB_SHAPE shape( nullptr, SHAPE_T::RECTANGLE );
    shape.SetStart( VECTOR2I( 0, 0 ) );
    shape.SetEnd( VECTOR2I( 1000, 500 ) );

    PROPERTY_MANAGER& propMgr = PROPERTY_MANAGER::Instance();
    propMgr.Rebuild();

    PROPERTY_BASE* prop = propMgr.GetProperty( TYPE_HASH( EDA_SHAPE ), _HKI( "Corner Radius" ) );

    auto resultTooLarge = prop->Validate( wxAny( 400 ), &shape );
    BOOST_CHECK( resultTooLarge.has_value() );

    auto resultOK = prop->Validate( wxAny( 200 ), &shape );
    BOOST_CHECK( !resultOK.has_value() );
}
