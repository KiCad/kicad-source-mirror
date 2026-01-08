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

#include <qa_utils/wx_utils/unit_test_utils.h>

#include <validators.h>


BOOST_AUTO_TEST_SUITE( NetnameValidator )


struct ValidatorFixture
{
    ValidatorFixture() : m_validatorAllowSpaces( true ), m_validatorNoSpaces( false )
    {
    }

    NETNAME_VALIDATOR m_validatorDefault;
    NETNAME_VALIDATOR m_validatorAllowSpaces;
    NETNAME_VALIDATOR m_validatorNoSpaces;
};


BOOST_FIXTURE_TEST_CASE( DefaultValidatorAllowsSpaces, ValidatorFixture )
{
    // Default validator should allow spaces (issue #9258)
    BOOST_CHECK( m_validatorDefault.IsValid( wxS( "Net with spaces" ) ).IsEmpty() );
    BOOST_CHECK( m_validatorDefault.IsValid( wxS( "Single_word" ) ).IsEmpty() );
    BOOST_CHECK( m_validatorDefault.IsValid( wxS( "Net name with multiple spaces" ) ).IsEmpty() );
}


BOOST_FIXTURE_TEST_CASE( ValidatorAllowsSpacesWhenConfigured, ValidatorFixture )
{
    // Validator with allowSpaces=true should allow spaces
    BOOST_CHECK( m_validatorAllowSpaces.IsValid( wxS( "Net with spaces" ) ).IsEmpty() );
    BOOST_CHECK( m_validatorAllowSpaces.IsValid( wxS( "Single_word" ) ).IsEmpty() );
}


BOOST_FIXTURE_TEST_CASE( ValidatorRejectsSpacesWhenConfigured, ValidatorFixture )
{
    // Validator with allowSpaces=false should reject spaces
    BOOST_CHECK( !m_validatorNoSpaces.IsValid( wxS( "Net with spaces" ) ).IsEmpty() );
    BOOST_CHECK( m_validatorNoSpaces.IsValid( wxS( "Single_word" ) ).IsEmpty() );
}


BOOST_FIXTURE_TEST_CASE( ValidatorRejectsControlCharacters, ValidatorFixture )
{
    // All validators should reject CR and LF
    BOOST_CHECK( !m_validatorDefault.IsValid( wxS( "Net\nname" ) ).IsEmpty() );
    BOOST_CHECK( !m_validatorDefault.IsValid( wxS( "Net\rname" ) ).IsEmpty() );
    BOOST_CHECK( !m_validatorAllowSpaces.IsValid( wxS( "Net\nname" ) ).IsEmpty() );
    BOOST_CHECK( !m_validatorAllowSpaces.IsValid( wxS( "Net\rname" ) ).IsEmpty() );
    BOOST_CHECK( !m_validatorNoSpaces.IsValid( wxS( "Net\nname" ) ).IsEmpty() );
    BOOST_CHECK( !m_validatorNoSpaces.IsValid( wxS( "Net\rname" ) ).IsEmpty() );
}


BOOST_FIXTURE_TEST_CASE( ValidatorRejectsTabsWhenSpacesDisallowed, ValidatorFixture )
{
    // Validator with allowSpaces=false should reject tabs
    BOOST_CHECK( !m_validatorNoSpaces.IsValid( wxS( "Net\twith\ttabs" ) ).IsEmpty() );

    // Default and allowSpaces=true should accept tabs (they're treated like spaces)
    BOOST_CHECK( m_validatorDefault.IsValid( wxS( "Net\twith\ttabs" ) ).IsEmpty() );
    BOOST_CHECK( m_validatorAllowSpaces.IsValid( wxS( "Net\twith\ttabs" ) ).IsEmpty() );
}


BOOST_AUTO_TEST_SUITE_END()
