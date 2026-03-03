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
#include <sstream>

#include <core/profile.h>

#include <board.h>
#include <board_design_settings.h>
#include <footprint.h>
#include <pad.h>
#include <pcbexpr_evaluator.h>
#include <pcb_field.h>
#include <pcb_group.h>
#include <pcb_shape.h>
#include <pcb_track.h>
#include <project/net_settings.h>
#include <zone.h>


using namespace KI_TEST;


/**
 * Format a vector of strings for display, e.g. ["GND", "VCC"]
 */
static std::string vecToString( std::span<const std::string> aVec )
{
    std::ostringstream ss;
    ss << "[";
    const char* sep = "";
    for( const std::string& s : aVec )
    {
        ss << sep << s;
        sep = ", ";
    }
    ss << "]";
    return ss.str();
}


/**
 * A constraint on a scalar value (counts, dimensions, etc.), supporting exact, min, and max bounds.
 *
 * For @c ROLE::COUNT, bare integers are literal count values.
 * For @c ROLE::DIMENSION, bare integers are interpreted as mm, and strings can specify
 * units (e.g. "5 mil", "0.127 mm"). Internally, dimensions are always stored in KiCad
 * internal units (nm).
 *
 * JSON input can be:
 *   - A bare integer: exact match (mm for DIMENSION, count for COUNT)
 *   - A string with units: exact match, e.g. "5 mil" (DIMENSION only)
 *   - An object: { "min": ..., "max": ..., "exact": ... }
 */
class SCALAR_CONSTRAINT
{
public:
    enum class ROLE
    {
        /// A simple count of items (e.g. number of footprints, nets, etc.)
        COUNT,
        /// A linear dimension (e.g. track width, clearance)
        DIMENSION,
    };

    /**
     * Parse a scalar constraint from JSON.
     *
     * @param aJson  The JSON value to parse
     * @param aKind  Whether to interpret bare integers as counts or mm dimensions
     */
    static SCALAR_CONSTRAINT FromJson( const nlohmann::json& aJson, ROLE aKind )
    {
        SCALAR_CONSTRAINT c;
        c.m_kind = aKind;

        if( aJson.is_object() )
        {
            if( aJson.contains( "exact" ) )
            {
                auto [v, units] = parseScalar( aJson.at( "exact" ), aKind );
                c.m_min = v;
                c.m_max = v;
                c.m_displayUnits = units;
            }
            else
            {
                if( aJson.contains( "min" ) )
                {
                    auto [v, units] = parseScalar( aJson.at( "min" ), aKind );
                    c.m_min = v;
                    c.m_displayUnits = units;
                }

                if( aJson.contains( "max" ) )
                {
                    auto [v, units] = parseScalar( aJson.at( "max" ), aKind );
                    c.m_max = v;

                    if( !c.m_displayUnits )
                        c.m_displayUnits = units;
                }

                if( !c.m_min && !c.m_max )
                {
                    throw std::runtime_error( "Scalar constraint object must have"
                                              " 'min', 'max', or 'exact': "
                                              + aJson.dump() );
                }
            }
        }
        else
        {
            // Shorthand: bare value means exact match
            auto [v, units] = parseScalar( aJson, aKind );
            c.m_min = v;
            c.m_max = v;
            c.m_displayUnits = units;
        }

        return c;
    }

    static SCALAR_CONSTRAINT Exact( int aValue, ROLE aKind = ROLE::COUNT )
    {
        SCALAR_CONSTRAINT c;
        c.m_kind = aKind;
        c.m_min = aValue;
        c.m_max = aValue;
        return c;
    }

    /**
     * Test if a value satisfies this constraint.
     */
    bool Match( int aValue ) const
    {
        if( m_min && aValue < *m_min )
            return false;

        if( m_max && aValue > *m_max )
            return false;

        return true;
    }

    bool IsExact() const { return m_min && m_max && *m_min == *m_max; }

    std::optional<int> GetMin() const { return m_min; }
    std::optional<int> GetMax() const { return m_max; }

    /**
     * Format a value for human-readable display.
     *
     * Counts are shown as plain integers. Dimensions are shown in the original
     * units from the JSON, with the raw internal-unit value appended for debugging:
     *   e.g. "5 mil (127000 nm)"
     */
    std::string Format( int aValue ) const
    {
        if( m_kind == ROLE::COUNT )
            return std::to_string( aValue );

        EDA_UNITS units = m_displayUnits.value_or( EDA_UNITS::MM );
        wxString  userStr = EDA_UNIT_UTILS::UI::StringFromValue( pcbIUScale, units, aValue, true /* aAddUnitsText */ );

        std::ostringstream ss;
        ss << userStr.ToStdString() << " (" << aValue << " nm)";
        return ss.str();
    }

    std::string Describe() const
    {
        if( m_min && m_max && *m_min == *m_max )
            return "exactly " + Format( *m_min );

        std::string desc;

        if( m_min )
            desc += "at least " + Format( *m_min );

        if( m_max )
        {
            if( !desc.empty() )
                desc += ", ";

            desc += "at most " + Format( *m_max );
        }

        return desc;
    }

private:
    struct PARSED_SCALAR
    {
        int                      value;
        std::optional<EDA_UNITS> units; ///< The units detected from the input, if any.
    };

    /**
     * Parse a single bound value from JSON.
     *
     * For @c COUNT: must be an integer.
     * For @c DIMENSION: integer (mm), float (mm), or string with units (e.g. "5 mil").
     *
     * @return The parsed value in internal units, and the detected display units (if a string
     *         with explicit units was provided).
     */
    static PARSED_SCALAR parseScalar( const nlohmann::json& aJson, ROLE aKind )
    {
        if( aJson.is_number_integer() )
        {
            int raw = aJson.get<int>();

            if( aKind == ROLE::DIMENSION )
                return { pcbIUScale.mmToIU( raw ), EDA_UNITS::MM };
            else
                return { raw, std::nullopt };
        }

        if( aJson.is_number_float() )
        {
            if( aKind == ROLE::COUNT )
                throw std::runtime_error( "Float value not valid for a count constraint" );

            return { KiROUND( pcbIUScale.mmToIU( aJson.get<double>() ) ), EDA_UNITS::MM };
        }

        if( aJson.is_string() )
        {
            if( aKind == ROLE::COUNT )
            {
                throw std::runtime_error( "String value not valid for a count constraint: " + aJson.dump() );
            }

            const std::string& dimStr = aJson.get<std::string>();
            double             dimIu = EDA_UNIT_UTILS::UI::DoubleValueFromString( pcbIUScale, EDA_UNITS::MM, dimStr );

            // Detect which units were written in the string (e.g. "5 mil" -> MILS)
            EDA_UNITS detectedUnits = EDA_UNITS::MM;
            EDA_UNIT_UTILS::FetchUnitsFromString( wxString( dimStr ), detectedUnits );

            return { KiROUND( dimIu ), detectedUnits };
        }

        throw std::runtime_error( "Invalid scalar constraint value: " + aJson.dump() );
    }

    std::optional<int>       m_min;
    std::optional<int>       m_max;
    std::optional<EDA_UNITS> m_displayUnits; ///< Original units for display.
    ROLE                     m_kind = ROLE::COUNT;
};


/**
 * Assert that @a aActual satisfies @a aConstraint using Boost.Test macros.
 *
 * Each bound is checked individually so that failures report the specific
 * bound that was violated, with values formatted in the constraint's original units.
 */
static void CheckConstraint( const SCALAR_CONSTRAINT& aConstraint, int aActual )
{
    if( aConstraint.IsExact() )
    {
        const int expected = *aConstraint.GetMin();
        BOOST_TEST( aActual == expected,
                    "expected " << aConstraint.Format( expected ) << ", got " << aConstraint.Format( aActual ) );
    }
    else
    {
        if( aConstraint.GetMin() )
        {
            const int minVal = *aConstraint.GetMin();
            BOOST_TEST( aActual >= minVal, "expected at least " << aConstraint.Format( minVal ) << ", got "
                                                                << aConstraint.Format( aActual ) );
        }

        if( aConstraint.GetMax() )
        {
            const int maxVal = *aConstraint.GetMax();
            BOOST_TEST( aActual <= maxVal, "expected at most " << aConstraint.Format( maxVal ) << ", got "
                                                               << aConstraint.Format( aActual ) );
        }
    }
}


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


class NET_EXPECTATION : public BOARD_EXPECTATION
{
public:
    std::optional<SCALAR_CONSTRAINT> m_Count;
    std::vector<std::string>         m_NamePatterns;

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
            CheckConstraint( *m_Count, actualCount );
        }
    }

    void RunTest( const BOARD& aBrd ) const override
    {
        if( !m_Count.has_value() && m_NamePatterns.empty() )
        {
            BOOST_FAIL( "Net expectation must have at least a count or a name pattern" );
        }

        // Optimisation - if we ONLY have a count, we have a simple test that doesn't require iterating
        // all the nets
        if( m_Count.has_value() && m_NamePatterns.empty() )
        {
            doSimpleCountTest( aBrd );
            return;
        }

        std::vector<const NETINFO_ITEM*> matches = findMatchingNets( aBrd );

        if( m_Count )
        {
            // We need to check the count of matching nets
            BOOST_TEST_CONTEXT( "Net count: " + m_Count->Describe() )
            {
                CheckConstraint( *m_Count, static_cast<int>( matches.size() ) );
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

                BOOST_TEST( found, "Expected net matching '" << pattern << "'" );
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
            desc += " " + vecToString( m_NamePatterns );
        }

        if( m_Count )
            desc += " count: " + m_Count->Describe();
        else
            desc += " exists";

        return desc;
    }
};


class NETCLASS_EXPECTATION : public BOARD_EXPECTATION
{
public:
    std::vector<std::string>         m_NetClassNames;
    std::optional<SCALAR_CONSTRAINT> m_Count;
    /// Expectation for track width constraint for all matching netclasses.
    std::optional<SCALAR_CONSTRAINT> m_TrackWidth;
    std::optional<SCALAR_CONSTRAINT> m_Clearance;
    std::optional<SCALAR_CONSTRAINT> m_DpGap;
    std::optional<SCALAR_CONSTRAINT> m_DpWidth;

    /// Expectation for number of nets matching the netclass patterns in aggregate
    std::optional<SCALAR_CONSTRAINT> m_MatchingNetCount;

private:
    void doSimpleCountTest( const BOARD& aBrd ) const
    {
        wxCHECK( m_Count.has_value(), /*void*/ );
        const std::shared_ptr<NET_SETTINGS>& netSettings = aBrd.GetDesignSettings().m_NetSettings;
        int                                  actualCount = netSettings->GetNetclasses().size();

        BOOST_TEST_CONTEXT( "Net class count: " + m_Count->Describe() )
        {
            CheckConstraint( *m_Count, actualCount );
        }
    }

    static bool nameMatches( const wxString& aName, const std::string& aPattern )
    {
        return aName.Matches( aPattern );
    }

    std::vector<const NETCLASS*> findMatchingNetclasses( const BOARD& aBrd ) const
    {
        std::vector<const NETCLASS*> matches;
        const auto                   netclasses = aBrd.GetDesignSettings().m_NetSettings->GetNetclasses();

        if( m_NetClassNames.empty() )
        {
            // No patterns = all nets
            for( const auto& [name, nc] : netclasses )
            {
                matches.push_back( nc.get() );
            }
            return matches;
        }

        for( const auto& [name, netclass] : netclasses )
        {
            for( const std::string& pattern : m_NetClassNames )
            {
                if( nameMatches( name, pattern ) )
                {
                    matches.push_back( netclass.get() );
                    break;
                }
            }
        }

        return matches;
    }

    void RunTest( const BOARD& aBrd ) const override
    {
        // Optimisation - if we ONLY have a count, we have a simple test that doesn't require iterating
        // all the nets
        if( m_Count.has_value() && m_NetClassNames.empty() )
        {
            doSimpleCountTest( aBrd );
            return;
        }

        std::vector<const NETCLASS*> matches = findMatchingNetclasses( aBrd );

        if( m_Count )
        {
            // We need to check the count of matching nets
            BOOST_TEST_CONTEXT( "Net class count: " + m_Count->Describe() )
            {
                CheckConstraint( *m_Count, static_cast<int>( matches.size() ) );
            }
        }
        else
        {
            // No count: every pattern must match at least one netclass
            for( const std::string& pattern : m_NetClassNames )
            {
                const auto& netclassMatchesPattern = [&]( const NETCLASS* nc )
                {
                    return nameMatches( nc->GetName(), pattern );
                };

                bool found = std::any_of( matches.begin(), matches.end(), netclassMatchesPattern );

                BOOST_TEST( found, "Expected netclass matching '" << pattern << "'" );
            }
        }

        for( const NETCLASS* nc : matches )
        {
            BOOST_TEST_CONTEXT( "Netclass '" << nc->GetName() << "' values" )
            {
                if( m_TrackWidth )
                {
                    BOOST_CHECK( nc->HasTrackWidth() );
                    BOOST_TEST_CONTEXT( "Track width: " + m_TrackWidth->Describe() )
                    {
                        CheckConstraint( *m_TrackWidth, nc->GetTrackWidth() );
                    }
                }

                if( m_Clearance )
                {
                    BOOST_CHECK( nc->HasClearance() );
                    BOOST_TEST_CONTEXT( "Clearance: " + m_Clearance->Describe() )
                    {
                        CheckConstraint( *m_Clearance, nc->GetClearance() );
                    }
                }

                if( m_DpGap )
                {
                    BOOST_CHECK( nc->HasDiffPairGap() );
                    BOOST_TEST_CONTEXT( "Diff pair gap: " + m_DpGap->Describe() )
                    {
                        CheckConstraint( *m_DpGap, nc->GetDiffPairGap() );
                    }
                }

                if( m_DpWidth )
                {
                    BOOST_CHECK( nc->HasDiffPairWidth() );
                    BOOST_TEST_CONTEXT( "Diff pair width: " + m_DpWidth->Describe() )
                    {
                        CheckConstraint( *m_DpWidth, nc->GetDiffPairWidth() );
                    }
                }
            }
        }

        if( m_MatchingNetCount )
        {
            int matchingNetCount = 0;
            for( const NETCLASS* nc : matches )
            {
                matchingNetCount += CountMatchingNets( aBrd, nc->GetName() );
            }

            BOOST_TEST_CONTEXT( "Matching net count: " + m_MatchingNetCount->Describe() )
            {
                CheckConstraint( *m_MatchingNetCount, matchingNetCount );
            }
        }
    }

    static int CountMatchingNets( const BOARD& aBrd, const wxString& aNetClassName )
    {
        int count = 0;

        for( NETINFO_ITEM* net : aBrd.GetNetInfo() )
        {
            if( net->GetNetCode() <= 0 )
                continue;

            NETCLASS* nc = net->GetNetClass();

            if( !nc )
                continue;

            if( nc->GetName() == aNetClassName )
                count++;
        }

        return count;
    }

    std::string GetName() const override
    {
        std::string desc = "Netclasses";

        if( !m_NetClassNames.empty() )
            desc += " " + vecToString( m_NetClassNames );

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
    std::optional<SCALAR_CONSTRAINT> m_CuCount;
    std::vector<std::string>         m_CuNames;

private:
    void RunTest( const BOARD& aBrd ) const override
    {
        int actualCount = aBrd.GetCopperLayerCount();

        if( m_CuCount.has_value() )
        {
            BOOST_TEST_CONTEXT( "Layer count: " + m_CuCount->Describe() )
            {
                CheckConstraint( *m_CuCount, actualCount );
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


class ITEM_EVAL_EXPECTATION : public BOARD_EXPECTATION
{
public:
    enum class ITEM_TYPE
    {
        ANY,
        BOARD_TRACK,
        BOARD_VIA,
        BOARD_FOOTPRINT,
        FP_PAD,
        FP_GRAPHIC,
        FP_FIELD,
        FP_ZONE,
        BOARD_GRAPHIC,
        BOARD_ZONE,
        BOARD_GROUP,
    };

    std::optional<wxString>          m_Expression;
    std::optional<ITEM_TYPE>         m_ItemType;
    std::optional<SCALAR_CONSTRAINT> m_ExpectedMatches;
    std::optional<wxString>          m_ParentExpr;

    static void reportError( const wxString& aMessage, int aOffset )
    {
        BOOST_TEST_FAIL( "Expression error: " << aMessage.ToStdString() << " at offset " << aOffset );
    }

    void RunTest( const BOARD& aBrd ) const override
    {
        PCBEXPR_COMPILER compiler( new PCBEXPR_UNIT_RESOLVER );
        PCBEXPR_UCODE    ucode, parentUcode;
        bool             ok = true;

        const auto matchItem = []( const BOARD_ITEM& aItem, PCBEXPR_UCODE& aUcode ) -> bool
        {
            LSET itemLayers = aItem.GetLayerSet();

            for( PCB_LAYER_ID layer : itemLayers.Seq() )
            {
                PCBEXPR_CONTEXT ctx( 0, layer );
                ctx.SetErrorCallback( reportError );
                ctx.SetItems( (BOARD_ITEM*) &aItem, nullptr );

                const LIBEVAL::VALUE* result = aUcode.Run( &ctx );

                if( result && result->AsDouble() != 0.0 )
                {
                    return true;
                }
            }
            return false;
        };

        if( m_Expression.has_value() )
        {
            PCBEXPR_CONTEXT preflightContext;
            preflightContext.SetErrorCallback( reportError );
            bool error = !compiler.Compile( *m_Expression, &ucode, &preflightContext );
            BOOST_REQUIRE( !error );
        }

        if( m_ParentExpr.has_value() )
        {
            PCBEXPR_CONTEXT preflightContext;
            preflightContext.SetErrorCallback( reportError );
            bool parentError = !compiler.Compile( *m_ParentExpr, &parentUcode, &preflightContext );
            BOOST_REQUIRE( !parentError );
        }

        std::vector<const BOARD_ITEM*> items;
        {
            PROF_TIMER collectTimer;

            // Gather only the items we actually need because this can be quite slow on big boards
            items = collectItemsOfType( aBrd, m_ItemType.value_or( ITEM_TYPE::ANY ) );

            BOOST_TEST_MESSAGE( "Collected " << items.size() << " items to evaluate expression against in "
                                             << collectTimer.msecs() << " ms" );
        }

        size_t matchCount = 0;

        if( !m_Expression.has_value() )
        {
            matchCount = items.size();
        }
        else
        {
            PROF_TIMER matchTimer;

            for( const BOARD_ITEM* item : items )
            {
                if( matchItem( *item, ucode ) )
                {
                    matchCount++;

                    if( m_ParentExpr.has_value() )
                    {
                        BOOST_TEST_CONTEXT( "Checking parent expression: " << *m_ParentExpr )
                        {
                            const BOARD_ITEM* parentItem = item->GetParent();
                            BOOST_REQUIRE( parentItem );

                            BOOST_CHECK( matchItem( *parentItem, parentUcode ) );
                        }
                    }
                }
            }

            BOOST_TEST_MESSAGE( "Expression '" << m_Expression->ToStdString() << "' matched " << matchCount
                                               << " items in " << matchTimer.msecs() << " ms" );
        }

        if( m_ExpectedMatches.has_value() )
        {
            BOOST_TEST_CONTEXT( "Eval matches: " + m_ExpectedMatches->Describe() )
            {
                CheckConstraint( *m_ExpectedMatches, matchCount );
            }
        }
        else
        {
            BOOST_TEST( matchCount > 0, "Expected expression to match at least one item, but it matched none" );
        }
    }

    std::string GetName() const override
    {
        std::ostringstream ss;
        ss << "Eval: " << m_Expression.value_or( "<no_expr>" );
        if( m_ExpectedMatches.has_value() )
            ss << ", expected matches: " << m_ExpectedMatches->Describe();
        return ss.str();
    }

private:
    std::vector<const BOARD_ITEM*> collectAllBoardItems( const BOARD& aBrd ) const
    {
        std::vector<const BOARD_ITEM*> items;

        for( const PCB_TRACK* track : aBrd.Tracks() )
            items.push_back( track );

        for( const FOOTPRINT* fp : aBrd.Footprints() )
        {
            items.push_back( fp );

            for( const PAD* pad : fp->Pads() )
                items.push_back( pad );

            for( const PCB_FIELD* field : fp->GetFields() )
                items.push_back( field );

            for( const BOARD_ITEM* gi : fp->GraphicalItems() )
                items.push_back( gi );

            for( const ZONE* zone : fp->Zones() )
                items.push_back( zone );
        }

        for( const BOARD_ITEM* item : aBrd.Drawings() )
            items.push_back( item );

        for( const ZONE* zone : aBrd.Zones() )
            items.push_back( zone );

        return items;
    }

    std::vector<const BOARD_ITEM*> collectItemsOfType( const BOARD& aBrd, ITEM_TYPE aType ) const
    {
        std::vector<const BOARD_ITEM*> items;

        switch( aType )
        {
        case ITEM_TYPE::BOARD_TRACK:
        {
            for( const PCB_TRACK* track : aBrd.Tracks() )
            {
                if( track->Type() != PCB_VIA_T )
                    items.push_back( track );
            }
            break;
        }
        case ITEM_TYPE::BOARD_VIA:
        {
            for( const PCB_TRACK* track : aBrd.Tracks() )
            {
                if( track->Type() == PCB_VIA_T )
                    items.push_back( track );
            }
            break;
        }
        case ITEM_TYPE::BOARD_FOOTPRINT:
        {
            for( const FOOTPRINT* fp : aBrd.Footprints() )
                items.push_back( fp );
            break;
        }
        case ITEM_TYPE::BOARD_GRAPHIC:
        {
            for( const BOARD_ITEM* item : aBrd.Drawings() )
            {
                if( item->Type() == PCB_SHAPE_T )
                    items.push_back( item );
            }
            break;
        }
        case ITEM_TYPE::BOARD_GROUP:
        {
            for( const BOARD_ITEM* item : aBrd.Groups() )
                items.push_back( item );
            break;
        }
        case ITEM_TYPE::BOARD_ZONE:
        {
            for( const ZONE* zone : aBrd.Zones() )
                items.push_back( zone );
            break;
        }
        case ITEM_TYPE::FP_PAD:
        {
            for( const FOOTPRINT* fp : aBrd.Footprints() )
            {
                for( const PAD* pad : fp->Pads() )
                    items.push_back( pad );
            }
            break;
        }
        case ITEM_TYPE::FP_GRAPHIC:
        {
            for( const FOOTPRINT* fp : aBrd.Footprints() )
            {
                for( const BOARD_ITEM* gi : fp->GraphicalItems() )
                    items.push_back( gi );
            }
            break;
        }
        case ITEM_TYPE::FP_FIELD:
        {
            for( const FOOTPRINT* fp : aBrd.Footprints() )
            {
                for( const PCB_FIELD* field : fp->GetFields() )
                    items.push_back( field );
            }
            break;
        }
        case ITEM_TYPE::FP_ZONE:
        {
            for( const FOOTPRINT* fp : aBrd.Footprints() )
            {
                for( const ZONE* zone : fp->Zones() )
                    items.push_back( zone );
            }
            break;
        }
        case ITEM_TYPE::ANY:
        {
            items = collectAllBoardItems( aBrd );
            break;
        }
        }

        return items;
    }
};


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
        netExpectation->m_Count =
                SCALAR_CONSTRAINT::FromJson( aExpectationEntry.at( "count" ), SCALAR_CONSTRAINT::ROLE::COUNT );
    }

    if( aExpectationEntry.contains( "name" ) )
    {
        const auto& expectedNetName = aExpectationEntry.at( "name" );
        netExpectation->m_NamePatterns = getStringArray( expectedNetName );
    }

    return netExpectation;
}


static std::unique_ptr<BOARD_EXPECTATION> createNetClassExpectation( const nlohmann::json& aExpectationEntry )
{
    auto netClassExpectation = std::make_unique<NETCLASS_EXPECTATION>();

    if( aExpectationEntry.contains( "count" ) )
    {
        netClassExpectation->m_Count =
                SCALAR_CONSTRAINT::FromJson( aExpectationEntry.at( "count" ), SCALAR_CONSTRAINT::ROLE::COUNT );
    }

    if( aExpectationEntry.contains( "name" ) )
    {
        const auto& expectedNetClassName = aExpectationEntry.at( "name" );
        netClassExpectation->m_NetClassNames = getStringArray( expectedNetClassName );
    }

    if( aExpectationEntry.contains( "trackWidth" ) )
    {
        netClassExpectation->m_TrackWidth =
                SCALAR_CONSTRAINT::FromJson( aExpectationEntry.at( "trackWidth" ), SCALAR_CONSTRAINT::ROLE::DIMENSION );
    }

    if( aExpectationEntry.contains( "clearance" ) )
    {
        netClassExpectation->m_Clearance =
                SCALAR_CONSTRAINT::FromJson( aExpectationEntry.at( "clearance" ), SCALAR_CONSTRAINT::ROLE::DIMENSION );
    }

    if( aExpectationEntry.contains( "dpGap" ) )
    {
        netClassExpectation->m_DpGap =
                SCALAR_CONSTRAINT::FromJson( aExpectationEntry.at( "dpGap" ), SCALAR_CONSTRAINT::ROLE::DIMENSION );
    }

    if( aExpectationEntry.contains( "dpWidth" ) )
    {
        netClassExpectation->m_DpWidth =
                SCALAR_CONSTRAINT::FromJson( aExpectationEntry.at( "dpWidth" ), SCALAR_CONSTRAINT::ROLE::DIMENSION );
    }

    if( aExpectationEntry.contains( "netCount" ) )
    {
        netClassExpectation->m_MatchingNetCount =
                SCALAR_CONSTRAINT::FromJson( aExpectationEntry.at( "netCount" ), SCALAR_CONSTRAINT::ROLE::COUNT );
    }

    return netClassExpectation;
}


static std::unique_ptr<BOARD_EXPECTATION> createLayerExpectation( const nlohmann::json& aExpectationEntry )
{
    auto layerExpectation = std::make_unique<LAYER_EXPECTATION>();

    if( aExpectationEntry.contains( "cuNames" ) )
    {
        const auto&              cuNamesEntry = aExpectationEntry.at( "cuNames" );
        std::vector<std::string> cuNames = getStringArray( cuNamesEntry );
        layerExpectation->m_CuNames = std::move( cuNames );
    }

    if( aExpectationEntry.contains( "cuCount" ) )
    {
        layerExpectation->m_CuCount =
                SCALAR_CONSTRAINT::FromJson( aExpectationEntry.at( "cuCount" ), SCALAR_CONSTRAINT::ROLE::COUNT );
    }
    else if( layerExpectation->m_CuNames.size() > 0 )
    {
        // If specific layer names are specified, we expect that many layers
        layerExpectation->m_CuCount =
                SCALAR_CONSTRAINT::Exact( static_cast<int>( layerExpectation->m_CuNames.size() ) );
    }

    return layerExpectation;
}


static std::unique_ptr<BOARD_EXPECTATION> createItemExprExpectation( const nlohmann::json& aExpectationEntry )
{
    auto evalExpectation = std::make_unique<ITEM_EVAL_EXPECTATION>();

    if( aExpectationEntry.contains( "count" ) )
    {
        evalExpectation->m_ExpectedMatches =
                SCALAR_CONSTRAINT::FromJson( aExpectationEntry.at( "count" ), SCALAR_CONSTRAINT::ROLE::COUNT );
    }

    // No expression is OK - it means all of the items of the specified type should be counted
    if( aExpectationEntry.contains( "expr" ) )
    {
        evalExpectation->m_Expression = aExpectationEntry.at( "expr" ).get<std::string>();
    }

    if( aExpectationEntry.contains( "itemType" ) )
    {
        if( !aExpectationEntry.at( "itemType" ).is_string() )
        {
            throw std::runtime_error( "Eval expectation 'itemType' field must be a string" );
        }

        const std::string itemTypeStr = aExpectationEntry.at( "itemType" ).get<std::string>();

        using ITYPE = ITEM_EVAL_EXPECTATION::ITEM_TYPE;
        const static std::unordered_map<std::string, ITYPE> itemTypeMap = {
            { "track", ITYPE::BOARD_TRACK },
            { "via", ITYPE::BOARD_VIA },
            { "footprint", ITYPE::BOARD_FOOTPRINT },
            { "board_graphic", ITYPE::BOARD_GRAPHIC },
            { "board_zone", ITYPE::BOARD_ZONE },
            { "board_group", ITYPE::BOARD_GROUP },
            { "pad", ITYPE::FP_PAD },
            { "field", ITYPE::FP_FIELD },
            { "fp_graphic", ITYPE::FP_GRAPHIC },
            { "fp_zone", ITYPE::FP_ZONE },
            { "any", ITYPE::ANY },
        };

        const auto it = itemTypeMap.find( itemTypeStr );
        if( it == itemTypeMap.end() )
        {
            throw std::runtime_error( "Unknown eval expectation item type: " + itemTypeStr );
        }

        evalExpectation->m_ItemType = it->second;
    }

    if( aExpectationEntry.contains( "parentExpr" ) )
    {
        evalExpectation->m_ParentExpr = aExpectationEntry.at( "parentExpr" ).get<std::string>();
    }

    return evalExpectation;
}


std::unique_ptr<BOARD_EXPECTATION_TEST> BOARD_EXPECTATION_TEST::CreateFromJson( const std::string&    aBrdName,
                                                                                const nlohmann::json& aBrdExpectation )
{
    using ExpectationFactoryFunc = std::unique_ptr<BOARD_EXPECTATION> ( * )( const nlohmann::json& );

    // clang-format off
    static const std::unordered_map<std::string, ExpectationFactoryFunc> factoryMap = {
        { "item",           createItemExprExpectation },
        { "net",            createNetExpectation },
        { "netclass",       createNetClassExpectation },
        { "layers",         createLayerExpectation },
    };
    // clang-format on

    std::unique_ptr<BOARD_EXPECTATION_TEST> test = std::make_unique<BOARD_EXPECTATION_TEST>( aBrdName );

    if( !aBrdExpectation.is_object() )
    {
        throw std::runtime_error( "Expectation entry for board " + aBrdName + " is not a valid JSON object" );
    }

    if( !aBrdExpectation.contains( "type" ) || !aBrdExpectation.at( "type" ).is_string() )
    {
        throw std::runtime_error( "Expectation entry for board " + aBrdName
                                  + " must have a string field named 'type'" );
    }

    const std::string expectationType = aBrdExpectation.at( "type" ).get<std::string>();

    auto it = factoryMap.find( expectationType );
    if( it == factoryMap.end() )
    {
        throw std::runtime_error( "Unsupported expectation type '" + expectationType + "' for board " + aBrdName );
    }

    if( std::unique_ptr<BOARD_EXPECTATION> expectation = it->second( aBrdExpectation ) )
    {
        // Apply common fields
        if( aBrdExpectation.contains( "comment" ) && aBrdExpectation.at( "comment" ).is_string() )
        {
            expectation->SetComment( aBrdExpectation.at( "comment" ).get<std::string>() );
        }

        if( aBrdExpectation.contains( "skip" ) && aBrdExpectation.at( "skip" ).is_boolean()
            && aBrdExpectation.at( "skip" ).get<bool>() )
        {
            test->m_skip = true;
        }

        test->m_expectation = std::move( expectation );
    }
    else
    {
        throw std::runtime_error( "Failed to create expectation for board " + aBrdName );
    }

    return test;
}


std::vector<BOARD_EXPECTATION_TEST::DESCRIPTOR>
BOARD_EXPECTATION_TEST::ExtractExpectationTestsFromJson( const nlohmann::json& aJsonArray )
{
    std::vector<DESCRIPTOR> tests;

    if( !aJsonArray.is_array() )
    {
        throw std::runtime_error( "Board expectations JSON must be an array of expectations" );
    }

    unsigned int index = 0;
    for( const auto& expectationEntry : aJsonArray )
    {
        if( !expectationEntry.is_object() )
        {
            throw std::runtime_error( "Expectation entry at index " + std::to_string( index )
                                      + " is not a valid JSON object" );
        }

        std::string              name;
        std::vector<std::string> tags;

        if( expectationEntry.contains( "testName" ) )
        {
            if( !expectationEntry.at( "testName" ).is_string() )
            {
                throw std::runtime_error( "Expectation entry 'testName' field at index " + std::to_string( index )
                                          + " must be a string, " + " but is was "
                                          + expectationEntry.at( "testName" ).type_name() );
            }

            name = expectationEntry.at( "testName" ).get<std::string>();
        }
        else
        {
            // No name - use index as identifier
            name = std::to_string( index );
        }

        if( expectationEntry.contains( "tags" ) )
        {
            if( !expectationEntry.at( "tags" ).is_array() )
            {
                throw std::runtime_error( "Expectation entry 'tags' field at index " + std::to_string( index )
                                          + " must be an array of strings" );
            }

            for( const auto& tagEntry : expectationEntry.at( "tags" ) )
            {
                if( !tagEntry.is_string() )
                {
                    throw std::runtime_error( "Expectation entry 'tags' field at index " + std::to_string( index )
                                              + " must be an array of strings" );
                }

                tags.push_back( tagEntry.get<std::string>() );
            }
        }

        tests.emplace_back( name, tags, expectationEntry );

        ++index;
    }

    return tests;
}


void BOARD_EXPECTATION_TEST::RunTest( const BOARD& aBrd ) const
{
    BOOST_TEST_CONTEXT( wxString::Format( "Checking expectation of type %s", m_expectation->GetName() ) )
    {
        const std::string& expectationComment = m_expectation->GetComment();
        if( !expectationComment.empty() )
        {
            BOOST_TEST_MESSAGE( "Expectation comment: " << expectationComment );
        }

        if( m_skip )
        {
            BOOST_TEST_MESSAGE( "Expectation skipped" );
            return;
        }

        m_expectation->RunTest( aBrd );
    }
}


/**
 * Constructs a BOARD_EXPECTATION_TEST from the given JSON definition, and runs it on the given board.
 */
void BOARD_EXPECTATION_TEST::RunFromRef( const std::string& aBrdName, const BOARD& aBoard,
                                         const DESCRIPTOR& aExpectationTestRef )
{
    BOOST_TEST_CONTEXT( "Running board expectation: " << aExpectationTestRef.m_TestName )
    {
        std::unique_ptr<BOARD_EXPECTATION_TEST> boardExpectationTest;

        // Load board expectations from the JSON file and create expectation objects
        boardExpectationTest = BOARD_EXPECTATION_TEST::CreateFromJson( aBrdName, aExpectationTestRef.m_TestJson );

        if( boardExpectationTest )
        {
            boardExpectationTest->RunTest( aBoard );
        }
    }
}
