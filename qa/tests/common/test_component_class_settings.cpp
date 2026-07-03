/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 3 of the License, or (at your
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

#include <project/component_class_settings.h>


BOOST_AUTO_TEST_SUITE( ComponentClassSettingsTests )


// Guards the dirty-check path used by the project save framework. The stubbed
// operator== previously threw, so any equality comparison crashed.
BOOST_AUTO_TEST_CASE( DefaultsCompareEqual )
{
    COMPONENT_CLASS_SETTINGS a( nullptr, "" );
    COMPONENT_CLASS_SETTINGS b( nullptr, "" );

    BOOST_CHECK( a == b );
    BOOST_CHECK( !( a != b ) );
}


BOOST_AUTO_TEST_CASE( SheetComponentClassFlagAffectsEquality )
{
    COMPONENT_CLASS_SETTINGS a( nullptr, "" );
    COMPONENT_CLASS_SETTINGS b( nullptr, "" );

    a.SetEnableSheetComponentClasses( true );

    BOOST_CHECK( a != b );

    b.SetEnableSheetComponentClasses( true );

    BOOST_CHECK( a == b );
}


BOOST_AUTO_TEST_CASE( AssignmentAffectsEquality )
{
    COMPONENT_CLASS_SETTINGS a( nullptr, "" );
    COMPONENT_CLASS_SETTINGS b( nullptr, "" );

    COMPONENT_CLASS_ASSIGNMENT_DATA assignment;
    assignment.SetComponentClass( wxS( "PowerNets" ) );
    assignment.AddCondition( COMPONENT_CLASS_ASSIGNMENT_DATA::CONDITION_TYPE::REFERENCE, wxS( "R1" ),
                             wxEmptyString );

    a.AddComponentClassAssignment( assignment );

    BOOST_CHECK( a != b );

    b.AddComponentClassAssignment( assignment );

    BOOST_CHECK( a == b );
}


// A single differing condition field must break equality between otherwise
// identical assignments.
BOOST_AUTO_TEST_CASE( AssignmentConditionDataAffectsEquality )
{
    COMPONENT_CLASS_SETTINGS a( nullptr, "" );
    COMPONENT_CLASS_SETTINGS b( nullptr, "" );

    COMPONENT_CLASS_ASSIGNMENT_DATA assignmentA;
    assignmentA.SetComponentClass( wxS( "PowerNets" ) );
    assignmentA.AddCondition( COMPONENT_CLASS_ASSIGNMENT_DATA::CONDITION_TYPE::REFERENCE, wxS( "R1" ),
                              wxEmptyString );

    COMPONENT_CLASS_ASSIGNMENT_DATA assignmentB;
    assignmentB.SetComponentClass( wxS( "PowerNets" ) );
    assignmentB.AddCondition( COMPONENT_CLASS_ASSIGNMENT_DATA::CONDITION_TYPE::REFERENCE, wxS( "R2" ),
                              wxEmptyString );

    a.AddComponentClassAssignment( assignmentA );
    b.AddComponentClassAssignment( assignmentB );

    BOOST_CHECK( a != b );
}


BOOST_AUTO_TEST_SUITE_END()
