/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.TXT for contributors.
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

/**
 * @file
 * Test utils (e.g. print helpers and test predicates for SCH_FIELD objects
 */

#ifndef QA_EESCHEMA_LIB_FIELD_TEST_UTILS__H
#define QA_EESCHEMA_LIB_FIELD_TEST_UTILS__H

#include <qa_utils/wx_utils/unit_test_utils.h>

#include <template_fieldnames.h>
#include <sch_field.h>


std::ostream& boost_test_print_type( std::ostream& os, SCH_FIELD const& f )
{
    os << "SCH_FIELD[ " << f.GetCanonicalName() << " ]";
    return os;
}

std::ostream& boost_test_print_type( std::ostream& os, std::vector<SCH_FIELD> const& f )
{
    os << "SCH_FIELDS[ " << f.size() << " ]";
    return os;
}


namespace KI_TEST
{

/**
 * Predicate to check a field name is as expected
 * @param  aField SCH_FIELD to check the name
 * @param  aExpectedName the expected field name
 * @param  aExpectedId the expected field id
 * @return  true if match
 */
bool FieldNameIdMatches( const SCH_FIELD& aField, const std::string& aExpectedName, int aExpectedId )
{
    bool       ok = true;
    const auto gotName = aField.GetCanonicalName();

    if( gotName != aExpectedName )
    {
        BOOST_TEST_INFO( "Field name: got '" << gotName << "', expected '" << aExpectedName );
        ok = false;
    }

    const int gotId = (int) aField.GetId();

    if( gotId != aExpectedId )
    {
        BOOST_TEST_INFO( "Field ID: got '" << gotId << "', expected '" << aExpectedId );
        ok = false;
    }

    return ok;
}

/**
 * Predicate to check that the mandatory fields look sensible
 */
bool AreDefaultFieldsCorrect( const std::vector<SCH_FIELD>& aFields )
{
    const unsigned expectedCount = 5;

    if( aFields.size() < expectedCount )
    {
        BOOST_TEST_INFO( "Expected at least " << expectedCount << " fields, got " << aFields.size() );
        return false;
    }

    bool ok = true;

    ok &= FieldNameIdMatches( aFields[0], "Reference", (int) FIELD_T::REFERENCE );
    ok &= FieldNameIdMatches( aFields[1], "Value",     (int) FIELD_T::VALUE );
    ok &= FieldNameIdMatches( aFields[2], "Footprint", (int) FIELD_T::FOOTPRINT );
    ok &= FieldNameIdMatches( aFields[3], "Datasheet", (int) FIELD_T::DATASHEET );

    return ok;
}

} // namespace KI_TEST

#endif // QA_EESCHEMA_LIB_FIELD_TEST_UTILS__H
