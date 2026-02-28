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
#include <pcb_shape.h>


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

        if( aJson.is_number() )
        {
            int v = aJson.get<int>();
            matcher.m_min = v;
            matcher.m_max = v;
        }
        else if( aJson.is_object() )
        {
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
        }
        else
        {
            throw std::runtime_error( "Invalid count expectation: " + aJson.dump() );
        }

        return matcher;
    }

    static INT_MATCHER Exact( int aValue )
    {
        INT_MATCHER matcher;
        matcher.m_min = aValue;
        matcher.m_max = aValue;
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


class LAYER_EXPECTATION : public BOARD_EXPECTATION
{
public:
    std::optional<INT_MATCHER> m_CuCount;
    std::vector<std::string>   m_CuNames;

private:
    void RunTest( const BOARD& aBrd ) const override
    {
        int actualCount = aBrd.GetCopperLayerCount();

        if( m_CuCount.has_value() )
        {
            BOOST_TEST_CONTEXT( "Layer count: " + m_CuCount->Describe() )
            {
                m_CuCount->Test( actualCount );
            }
        }

        if( !m_CuNames.empty() )
        {
            std::vector<std::string> actualNames;
            const LSET               cuLayers = aBrd.GetLayerSet() & LSET::AllCuMask();

            for( const auto& layer : cuLayers )
            {
                actualNames.push_back( aBrd.GetLayerName( layer ).ToStdString() );
            }

            BOOST_REQUIRE( actualNames.size() == m_CuNames.size() );

            for( size_t i = 0; i < m_CuNames.size(); ++i )
            {
                BOOST_TEST_CONTEXT( "Expecting Cu layer name: '" << m_CuNames[i] << "'" )
                {
                    BOOST_TEST( actualNames[i] == m_CuNames[i] );
                }
            }
        }
    }

    std::string GetName() const override
    {
        return std::string( "Layers: " ) + ( m_CuCount.has_value() ? m_CuCount->Describe() : "N/A" );
    }
};


class CIRCLE_EXPECTATION : public BOARD_EXPECTATION
{
public:
    std::optional<VECTOR2I>    m_Position;
    std::optional<int>         m_Radius;
    std::optional<std::string> m_LayerName;

    void RunTest( const BOARD& aBrd ) const override
    {
        bool found = false;

        for( const auto& drawing : aBrd.Drawings() )
        {
            if( drawing->Type() != PCB_SHAPE_T )
                continue;

            const PCB_SHAPE& shape = static_cast<const PCB_SHAPE&>( *drawing );

            if( shape.GetShape() == SHAPE_T::CIRCLE )
            {
                const VECTOR2I     actualPos = shape.GetPosition();
                const int          actualRadius = shape.GetRadius();
                const PCB_LAYER_ID actualLayer = shape.GetLayer();

                bool layerMatches = true;
                if( m_LayerName.has_value() && m_LayerName != "*" )
                {
                    wxString actualLayerName = aBrd.GetLayerName( actualLayer );
                    layerMatches = ( actualLayerName == m_LayerName );
                }

                bool positionMatches = !m_Position.has_value() || ( actualPos == m_Position );
                bool radiusMatches = !m_Radius.has_value() || ( actualRadius == m_Radius );

                if( positionMatches && radiusMatches && layerMatches )
                {
                    found = true;
                    break;
                }
            }
        }

        BOOST_TEST( found );
    }

    std::string GetName() const override
    {
        std::ostringstream ss;
        ss << "Circle:";
        if( m_Position.has_value() )
            ss << " position (" << pcbIUScale.IUTomm( m_Position->x ) << " mm, " << pcbIUScale.IUTomm( m_Position->y )
               << " mm),";

        if( m_Radius.has_value() )
            ss << " radius " << pcbIUScale.IUTomm( *m_Radius ) << " mm,";

        if( m_LayerName.has_value() )
            ss << " layer '" << *m_LayerName << "'";

        return ss.str();
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


static std::unique_ptr<BOARD_EXPECTATION> createLayerExpectation( const nlohmann::json& aExpectationEntry )
{
    auto layerExpectation = std::make_unique<LAYER_EXPECTATION>();

    if( aExpectationEntry.contains( "cuNames" ) )
    {
        const auto&              cuNamesEntry = aExpectationEntry["cuNames"];
        std::vector<std::string> cuNames = getStringArray( cuNamesEntry );
        layerExpectation->m_CuNames = std::move( cuNames );
    }

    if( aExpectationEntry.contains( "count" ) )
    {
        const auto& countEntry = aExpectationEntry["cuCount"];
        layerExpectation->m_CuCount = INT_MATCHER::FromJson( countEntry );
    }
    else if( layerExpectation->m_CuNames.size() > 0 )
    {
        // If specific layer names are specified, we expect that many layers
        layerExpectation->m_CuCount = INT_MATCHER::Exact( static_cast<int>( layerExpectation->m_CuNames.size() ) );
    }

    return layerExpectation;
}


/**
 * Parse a dimension from JSON, which can be either an integer (in mm) or a string with units (e.g. "25 mil")
 *
 * @param aJson The JSON value to parse
 * @return The parsed dimension in internal units (IU)
 */
static int parsePcbDim( const nlohmann::json& aJson )
{
    if( aJson.is_number_integer() )
    {
        return pcbIUScale.mmToIU( aJson.get<int>() );
    }
    else if( aJson.is_string() )
    {
        const std::string& dimStr = aJson.get<std::string>();
        double             dimIu = EDA_UNIT_UTILS::UI::DoubleValueFromString( pcbIUScale, EDA_UNITS::MM, dimStr );
        return KiROUND( dimIu );
    }

    throw std::runtime_error( "Expected dimension to be an integer or a string with units" );
}


static int parsePcbDim( const nlohmann::json& aJson, const std::string& aFieldName )
{
    if( !aJson.contains( aFieldName ) )
    {
        throw std::runtime_error( "Expectation entry must have a '" + aFieldName + "' field" );
    }

    return parsePcbDim( aJson[aFieldName] );
}


static VECTOR2I parsePosition( const nlohmann::json& aJson, const std::string& aFieldName )
{
    if( !aJson.contains( aFieldName ) )
    {
        throw std::runtime_error( "Expectation entry must have a '" + aFieldName + "' field" );
    }

    const auto& field = aJson[aFieldName];
    if( !field.is_array() || field.size() != 2 )
    {
        throw std::runtime_error( "Expectation entry must have a '" + aFieldName
                                  + "' field with an array of 2 entries" );
    }

    VECTOR2I pos;
    pos.x = parsePcbDim( field[0] );
    pos.y = parsePcbDim( field[1] );
    return pos;
}


static std::unique_ptr<BOARD_EXPECTATION> createCircleExpectation( const nlohmann::json& aExpectationEntry )
{
    auto circleExpectation = std::make_unique<CIRCLE_EXPECTATION>();

    circleExpectation->m_Position = parsePosition( aExpectationEntry, "position" );
    circleExpectation->m_Radius = parsePcbDim( aExpectationEntry, "radius" );

    if( !aExpectationEntry.contains( "layer" ) || !aExpectationEntry["layer"].is_string() )
    {
        throw std::runtime_error( "Circle expectation must have a 'layer' field with a string value" );
    }

    circleExpectation->m_LayerName = wxString( aExpectationEntry["layer"].get<std::string>() );

    return circleExpectation;
}


static std::unique_ptr<BOARD_EXPECTATION> createGraphicExpectation( const nlohmann::json& aExpectationEntry )
{
    const auto& shapeEntry = aExpectationEntry["shape"];

    if( !shapeEntry.is_string() )
    {
        throw std::runtime_error( "Graphic expectation must have a string 'shape' field" );
    }

    const std::string shape = shapeEntry.get<std::string>();

    if( shape == "circle" )
    {
        return createCircleExpectation( aExpectationEntry );
    }

    throw std::runtime_error( "Unsupported graphic shape: " + shape );
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
        else if( expectationType == "layers" )
        {
            expectation = createLayerExpectation( expectationEntry );
        }
        else if( expectationType == "graphic" )
        {
            expectation = createGraphicExpectation( expectationEntry );
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
