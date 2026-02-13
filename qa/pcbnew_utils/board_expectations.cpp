/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/gpl-3.0.html
 * or you may search the http://www.gnu.org website for the version 3 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include <qa_utils/wx_utils/unit_test_utils.h>

#include "pcbnew_utils/board_expectations.h"

#include <optional>

#include <board.h>

using namespace KI_TEST;


/**
 * Simple binary expectation that checks if an integer value meets the expectation
 * (exact, at least, at most).
 */
class INT_MATCHER
{
public:
    static INT_MATCHER FromJson( const nlohmann::json& aJson )
    {
        INT_MATCHER matcher;

        if( aJson.contains( "exact" ) )
        {
            int v = aJson["exact"];
            matcher.m_min = v;
            matcher.m_max = v;
        }
        else
        {
            if( aJson.contains( "min" ) )
                matcher.m_min = aJson["min"];

            if( aJson.contains( "max" ) )
                matcher.m_max = aJson["max"];
        }

        return matcher;
    }

    void Test( int aActual ) const
    {
        if( m_min )
            BOOST_TEST( aActual >= *m_min );

        if( m_max )
            BOOST_TEST( aActual <= *m_max );
    }

    std::string Describe() const
    {
        if( m_min && m_max && *m_min == *m_max )
            return "exactly " + std::to_string( *m_min );

        std::string desc;

        if( m_min )
            desc += "at least " + std::to_string( *m_min );

        if( m_max )
        {
            if( !desc.empty() )
                desc += ", ";

            desc += "at most " + std::to_string( *m_max );
        }

        return desc;
    }

private:
    std::optional<int> m_min;
    std::optional<int> m_max;
};


/**
 * Glob-like pattern matcher for strings, supporting '*' and '?' wildcards
 */
class STRING_PATTERN_MATCHER
{
public:
    explicit STRING_PATTERN_MATCHER( const std::string& aPattern ) :
            m_pattern( aPattern )
    {
    }

    static bool matchPredicate( const std::string& aStr, const std::string& aPattern )
    {
        return wxString( aStr ).Matches( aPattern );
    }

    void Test( const std::string& aStr ) const { BOOST_CHECK_PREDICATE( matchPredicate, (aStr) ( m_pattern ) ); }

private:
    std::string m_pattern;
};


class FOOTPRINT_EXPECTATION : public BOARD_EXPECTATION
{
public:
    std::optional<INT_MATCHER> m_Count;

private:
    void RunTest( const BOARD& aBrd ) const override
    {
        int actualCount = aBrd.Footprints().size();

        // TODO: filter footprints by layer, if layer filter is specified in the future

        if( m_Count.has_value() )
        {
            BOOST_TEST_CONTEXT( "Footprint count: " + m_Count->Describe() )
            {
                m_Count->Test( actualCount );
            }
        }
    }

    std::string GetName() const override
    {
        return std::string( "Footprint: " ) + ( m_Count.has_value() ? m_Count->Describe() : "N/A" );
    }
};


class NET_EXPECTATION : public BOARD_EXPECTATION
{
public:
    std::optional<INT_MATCHER> m_Count;
    std::vector<std::string>   m_NamePatterns;

private:
    static bool nameMatches( const std::string& aName, const std::string& aPattern )
    {
        return wxString( aName ).Matches( aPattern );
    }

    std::vector<const NETINFO_ITEM*> findMatchingNets( const BOARD& aBrd ) const
    {
        std::vector<const NETINFO_ITEM*> matches;

        if( m_NamePatterns.empty() )
        {
            // No patterns = all nets
            for( const NETINFO_ITEM* net : aBrd.GetNetInfo() )
            {
                matches.push_back( net );
            }
            return matches;
        }

        for( const NETINFO_ITEM* net : aBrd.GetNetInfo() )
        {
            for( const std::string& pattern : m_NamePatterns )
            {
                if( nameMatches( net->GetNetname().ToStdString(), pattern ) )
                {
                    matches.push_back( net );
                    break;
                }
            }
        }

        return matches;
    }

    void doSimpleCountTest( const BOARD& aBrd ) const
    {
        wxASSERT( m_Count.has_value() );
        int actualCount = aBrd.GetNetCount();

        BOOST_TEST_CONTEXT( "Net count: " + m_Count->Describe() )
        {
            m_Count->Test( actualCount );
        }
    }

    void RunTest( const BOARD& aBrd ) const override
    {
        // Optimisation - if we ONLY have a count, we have a simple test that doesn't require iterating
        // all the nets
        if( m_Count.has_value() && m_NamePatterns.empty() )
        {
            doSimpleCountTest( aBrd );
            return;
        }

        std::vector<const NETINFO_ITEM*> matches = findMatchingNets( aBrd );

        const NETINFO_LIST& nets = aBrd.GetNetInfo();

        if( m_Count )
        {
            // We need to check the count of matching nets
            BOOST_TEST_CONTEXT( "Net count: " + m_Count->Describe() )
            {
                m_Count->Test( static_cast<int>( matches.size() ) );
            }
        }
        else
        {
            // No count: every pattern must match at least one net
            for( const std::string& pattern : m_NamePatterns )
            {
                const auto& netMatchesPattern = [&]( const NETINFO_ITEM* n )
                {
                    return nameMatches( n->GetNetname().ToStdString(), pattern );
                };

                bool found = std::any_of( matches.begin(), matches.end(), netMatchesPattern );

                BOOST_TEST_CONTEXT( "Expected net matching '" << pattern << "'" )
                {
                    BOOST_TEST( found );
                }
            }
        }
    }

    std::string GetName() const override
    {
        std::string desc = "Net";

        if( m_NamePatterns.size() == 1 )
        {
            desc += " '" + m_NamePatterns[0] + "'";
        }
        else if( !m_NamePatterns.empty() )
        {
            std::string joined;
            for( size_t i = 0; i < m_NamePatterns.size(); ++i )
            {
                if( i > 0 )
                    joined += "', '";
                joined += m_NamePatterns[i];
            }
            desc += " ['" + joined + "']";
        }

        if( m_Count )
            desc += " count: " + m_Count->Describe();
        else
            desc += " exists";

        return desc;
    }
};


static std::unique_ptr<BOARD_EXPECTATION> createFootprintExpectation( const nlohmann::json& aExpectationEntry )
{
    auto footprintExpectation = std::make_unique<FOOTPRINT_EXPECTATION>();

    if( aExpectationEntry.contains( "count" ) )
    {
        const auto&       countEntry = aExpectationEntry["count"];
        const INT_MATCHER countMatcher = INT_MATCHER::FromJson( countEntry );
        footprintExpectation->m_Count = countMatcher;
    }

    return footprintExpectation;
}


static std::vector<std::string> getStringArray( const nlohmann::json& aJson )
{
    std::vector<std::string> result;

    if( aJson.is_string() )
    {
        result.push_back( aJson );
    }
    else if( aJson.is_array() )
    {
        for( const auto& entry : aJson )
        {
            if( !entry.is_string() )
            {
                throw std::runtime_error( "Expected a string or an array of strings" );
            }

            result.push_back( entry );
        }
    }
    else
    {
        throw std::runtime_error( "Expected a string or an array of strings" );
    }

    return result;
}


static std::unique_ptr<BOARD_EXPECTATION> createNetExpectation( const nlohmann::json& aExpectationEntry )
{
    auto netExpectation = std::make_unique<NET_EXPECTATION>();

    if( aExpectationEntry.contains( "count" ) )
    {
        const auto& countEntry = aExpectationEntry["count"];
        netExpectation->m_Count = INT_MATCHER::FromJson( countEntry );
    }

    if( aExpectationEntry.contains( "name" ) )
    {
        const auto& expectedNetName = aExpectationEntry["name"];
        netExpectation->m_NamePatterns = getStringArray( expectedNetName );
    }

    return netExpectation;
}


std::unique_ptr<BOARD_EXPECTATION_TEST> BOARD_EXPECTATION_TEST::CreateFromJson( const std::string&    aBrdName,
                                                                                const nlohmann::json& aBrdExpectations )
{
    std::unique_ptr<BOARD_EXPECTATION_TEST> test = std::make_unique<BOARD_EXPECTATION_TEST>( aBrdName );

    if( !aBrdExpectations.is_array() )
    {
        throw std::runtime_error( "Board expectations for board " + aBrdName + " are not a valid JSON object" );
    }

    for( const auto& expectationEntry : aBrdExpectations )
    {
        if( !expectationEntry.is_object() )
        {
            throw std::runtime_error( "Expectation entry for board " + aBrdName + " is not a valid JSON object" );
        }

        if( !expectationEntry.contains( "type" ) || !expectationEntry["type"].is_string() )
        {
            throw std::runtime_error( "Expectation entry for board " + aBrdName
                                      + " must have a string field named 'type'" );
        }

        const std::string expectationType = expectationEntry["type"];

        std::unique_ptr<BOARD_EXPECTATION> expectation;

        if( expectationType == "footprint" )
        {
            expectation = createFootprintExpectation( expectationEntry );
        }
        else if( expectationType == "net" )
        {
            expectation = createNetExpectation( expectationEntry );
        }
        else
        {
            throw std::runtime_error( "Unsupported expectation type '" + expectationType + "' for board " + aBrdName );
        }

        if( expectation )
            test->m_expectations.push_back( std::move( expectation ) );
    }

    return test;
}


void BOARD_EXPECTATION_TEST::RunTest( const BOARD& aBrd ) const
{
    for( const auto& expectation : m_expectations )
    {
        BOOST_TEST_CONTEXT( wxString::Format( "Checking expectation of type %s", expectation->GetName() ) )
        {
            expectation->RunTest( aBrd );
        }
    }
}
