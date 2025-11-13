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

#include <text_eval/text_eval_parser.h>
#include <fmt/format.h>
#include <array>
#include <cctype>
#include <wx/string.h>

namespace calc_parser
{
thread_local ERROR_COLLECTOR* g_errorCollector = nullptr;

class DATE_UTILS
{
private:
    static constexpr int                         epochYear = 1970;
    static constexpr std::array<int, 12>         daysInMonth = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
    static constexpr std::array<const char*, 12> monthNames = { "January",   "February", "March",    "April",
                                                                "May",       "June",     "July",     "August",
                                                                "September", "October",  "November", "December" };
    static constexpr std::array<const char*, 12> monthAbbrev = { "Jan", "Feb", "Mar", "Apr", "May", "Jun",
                                                                 "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };
    static constexpr std::array<const char*, 7>  weekdayNames = { "Monday", "Tuesday",  "Wednesday", "Thursday",
                                                                  "Friday", "Saturday", "Sunday" };

    static auto isLeapYear( int aYear ) -> bool
    {
        return ( aYear % 4 == 0 && aYear % 100 != 0 ) || ( aYear % 400 == 0 );
    }

    static auto daysInYear( int aYear ) -> int { return isLeapYear( aYear ) ? 366 : 365; }

    static auto daysInMonthForYear( int aMonth, int aYear ) -> int
    {
        if( aMonth == 2 && isLeapYear( aYear ) )
            return 29;

        return daysInMonth[aMonth - 1];
    }

public:
    static auto DaysToYmd( int aDaysSinceEpoch ) -> std::tuple<int, int, int>
    {
        int year = epochYear;
        int remainingDays = aDaysSinceEpoch;

        if( remainingDays >= 0 )
        {
            while( remainingDays >= daysInYear( year ) )
            {
                remainingDays -= daysInYear( year );
                year++;
            }
        }
        else
        {
            while( remainingDays < 0 )
            {
                year--;
                remainingDays += daysInYear( year );
            }
        }

        int month = 1;
        while( month <= 12 && remainingDays >= daysInMonthForYear( month, year ) )
        {
            remainingDays -= daysInMonthForYear( month, year );
            month++;
        }

        int day = remainingDays + 1;
        return { year, month, day };
    }

    static auto YmdToDays( int aYear, int aMonth, int aDay ) -> int
    {
        int totalDays = 0;

        if( aYear >= epochYear )
        {
            for( int y = epochYear; y < aYear; ++y )
                totalDays += daysInYear( y );
        }
        else
        {
            for( int y = aYear; y < epochYear; ++y )
                totalDays -= daysInYear( y );
        }

        for( int m = 1; m < aMonth; ++m )
            totalDays += daysInMonthForYear( m, aYear );

        totalDays += aDay - 1;
        return totalDays;
    }

    static auto ParseDate( const std::string& aDateStr ) -> std::optional<int>
    {
        std::istringstream iss( aDateStr );
        std::string        token;
        std::vector<int>   parts;

        char separator = 0;
        bool isCjkFormat = false;

        // Check for CJK date formats first (Chinese, Korean, or mixed)
        bool hasChineseYear = aDateStr.find( "年" ) != std::string::npos;
        bool hasChineseMonth = aDateStr.find( "月" ) != std::string::npos;
        bool hasChineseDay = aDateStr.find( "日" ) != std::string::npos;
        bool hasKoreanYear = aDateStr.find( "년" ) != std::string::npos;
        bool hasKoreanMonth = aDateStr.find( "월" ) != std::string::npos;
        bool hasKoreanDay = aDateStr.find( "일" ) != std::string::npos;

        // Check if we have any CJK date format (pure or mixed)
        if( ( hasChineseYear || hasKoreanYear ) && ( hasChineseMonth || hasKoreanMonth )
            && ( hasChineseDay || hasKoreanDay ) )
        {
            // CJK format: Support pure Chinese, pure Korean, or mixed formats
            isCjkFormat = true;

            size_t yearPos, monthPos, dayPos;

            // Find year position and marker
            if( hasChineseYear )
                yearPos = aDateStr.find( "年" );
            else
                yearPos = aDateStr.find( "년" );

            // Find month position and marker
            if( hasChineseMonth )
                monthPos = aDateStr.find( "月" );
            else
                monthPos = aDateStr.find( "월" );

            // Find day position and marker
            if( hasChineseDay )
                dayPos = aDateStr.find( "日" );
            else
                dayPos = aDateStr.find( "일" );

            try
            {
                int year = std::stoi( aDateStr.substr( 0, yearPos ) );
                int month = std::stoi(
                        aDateStr.substr( yearPos + 3, monthPos - yearPos - 3 ) ); // 3 bytes for CJK year marker
                int day = std::stoi(
                        aDateStr.substr( monthPos + 3, dayPos - monthPos - 3 ) ); // 3 bytes for CJK month marker

                parts = { year, month, day };
            }
            catch( ... )
            {
                return std::nullopt;
            }
        }
        else if( aDateStr.find( '-' ) != std::string::npos )
            separator = '-';
        else if( aDateStr.find( '/' ) != std::string::npos )
            separator = '/';
        else if( aDateStr.find( '.' ) != std::string::npos )
            separator = '.';

        if( separator )
        {
            while( std::getline( iss, token, separator ) )
            {
                try
                {
                    parts.push_back( std::stoi( token ) );
                }
                catch( ... )
                {
                    return std::nullopt;
                }
            }
        }
        else if( !isCjkFormat && aDateStr.length() == 8 )
        {
            try
            {
                int dateNum = std::stoi( aDateStr );
                int year = dateNum / 10000;
                int month = ( dateNum / 100 ) % 100;
                int day = dateNum % 100;
                return YmdToDays( year, month, day );
            }
            catch( ... )
            {
                return std::nullopt;
            }
        }
        else if( !isCjkFormat )
        {
            return std::nullopt;
        }

        if( parts.empty() || parts.size() > 3 )
            return std::nullopt;

        int year, month, day;

        if( parts.size() == 1 )
        {
            year = parts[0];
            month = 1;
            day = 1;
        }
        else if( parts.size() == 2 )
        {
            year = parts[0];
            month = parts[1];
            day = 1;
        }
        else
        {
            if( isCjkFormat )
            {
                // CJK formats are always in YYYY年MM月DD日 or YYYY년 MM월 DD일 order
                year = parts[0];
                month = parts[1];
                day = parts[2];
            }
            else if( separator == '/' && parts[0] <= 12 && parts[1] <= 31 )
            {
                month = parts[0];
                day = parts[1];
                year = parts[2];
            }
            else if( separator == '/' && parts[1] <= 12 )
            {
                day = parts[0];
                month = parts[1];
                year = parts[2];
            }
            else
            {
                year = parts[0];
                month = parts[1];
                day = parts[2];
            }
        }

        if( month < 1 || month > 12 )
            return std::nullopt;
        if( day < 1 || day > daysInMonthForYear( month, year ) )
            return std::nullopt;

        return YmdToDays( year, month, day );
    }

    static auto FormatDate( int aDaysSinceEpoch, const std::string& aFormat ) -> std::string
    {
        auto [year, month, day] = DaysToYmd( aDaysSinceEpoch );

        if( aFormat == "ISO" || aFormat == "iso" )
            return fmt::format( "{:04d}-{:02d}-{:02d}", year, month, day );
        else if( aFormat == "US" || aFormat == "us" )
            return fmt::format( "{:02d}/{:02d}/{:04d}", month, day, year );
        else if( aFormat == "EU" || aFormat == "european" )
            return fmt::format( "{:02d}/{:02d}/{:04d}", day, month, year );
        else if( aFormat == "long" )
            return fmt::format( "{} {}, {}", monthNames[month - 1], day, year );
        else if( aFormat == "short" )
            return fmt::format( "{} {}, {}", monthAbbrev[month - 1], day, year );
        else if( aFormat == "Chinese" || aFormat == "chinese" || aFormat == "CN" || aFormat == "cn"
                 || aFormat == "中文" )
            return fmt::format( "{}年{:02d}月{:02d}日", year, month, day );
        else if( aFormat == "Japanese" || aFormat == "japanese" || aFormat == "JP" || aFormat == "jp"
                 || aFormat == "日本語" )
            return fmt::format( "{}年{:02d}月{:02d}日", year, month, day );
        else if( aFormat == "Korean" || aFormat == "korean" || aFormat == "KR" || aFormat == "kr"
                 || aFormat == "한국어" )
            return fmt::format( "{}년 {:02d}월 {:02d}일", year, month, day );
        else
            return fmt::format( "{:04d}-{:02d}-{:02d}", year, month, day );
    }

    static auto GetWeekdayName( int aDaysSinceEpoch ) -> std::string
    {
        int weekday = ( ( aDaysSinceEpoch + 3 ) % 7 ); // +3 because epoch was Thursday (Monday = 0)

        if( weekday < 0 )
            weekday += 7;

        return std::string{ weekdayNames[weekday] };
    }

    static auto GetCurrentDays() -> int
    {
        auto now = std::chrono::system_clock::now();
        auto timeT = std::chrono::system_clock::to_time_t( now );
        return static_cast<int>( timeT / ( 24 * 3600 ) );
    }

    static auto GetCurrentTimestamp() -> double
    {
        auto now = std::chrono::system_clock::now();
        auto timeT = std::chrono::system_clock::to_time_t( now );
        return static_cast<double>( timeT );
    }
};


class ESERIES_UTILS
{
private:
    // E24 series values in 100-999 decade (2 significant figures)
    static constexpr std::array<uint16_t, 24> s_e24 = {
        100, 110, 120, 130, 150, 160, 180, 200, 220, 240, 270, 300,
        330, 360, 390, 430, 470, 510, 560, 620, 680, 750, 820, 910
    };

    // E192 series values in 100-999 decade (3 significant figures)
    static constexpr std::array<uint16_t, 192> s_e192 = {
        100, 101, 102, 104, 105, 106, 107, 109, 110, 111, 113, 114, 115, 117, 118, 120, 121, 123,
        124, 126, 127, 129, 130, 132, 133, 135, 137, 138, 140, 142, 143, 145, 147, 149, 150, 152,
        154, 156, 158, 160, 162, 164, 165, 167, 169, 172, 174, 176, 178, 180, 182, 184, 187, 189,
        191, 193, 196, 198, 200, 203, 205, 208, 210, 213, 215, 218, 221, 223, 226, 229, 232, 234,
        237, 240, 243, 246, 249, 252, 255, 258, 261, 264, 267, 271, 274, 277, 280, 284, 287, 291,
        294, 298, 301, 305, 309, 312, 316, 320, 324, 328, 332, 336, 340, 344, 348, 352, 357, 361,
        365, 370, 374, 379, 383, 388, 392, 397, 402, 407, 412, 417, 422, 427, 432, 437, 442, 448,
        453, 459, 464, 470, 475, 481, 487, 493, 499, 505, 511, 517, 523, 530, 536, 542, 549, 556,
        562, 569, 576, 583, 590, 597, 604, 612, 619, 626, 634, 642, 649, 657, 665, 673, 681, 690,
        698, 706, 715, 723, 732, 741, 750, 759, 768, 777, 787, 796, 806, 816, 825, 835, 845, 856,
        866, 876, 887, 898, 909, 920, 931, 942, 953, 965, 976, 988
    };

    static auto parseSeriesString( const std::string& aSeries ) -> int
    {
        if( aSeries == "E3" || aSeries == "e3" )
            return 3;
        else if( aSeries == "E6" || aSeries == "e6" )
            return 6;
        else if( aSeries == "E12" || aSeries == "e12" )
            return 12;
        else if( aSeries == "E24" || aSeries == "e24" )
            return 24;
        else if( aSeries == "E48" || aSeries == "e48" )
            return 48;
        else if( aSeries == "E96" || aSeries == "e96" )
            return 96;
        else if( aSeries == "E192" || aSeries == "e192" )
            return 192;
        else
            return -1; // Invalid series
    }

    static auto getSeriesValue( int aSeries, size_t aIndex ) -> uint16_t
    {
        // E1, E3, E6, E12, E24 are derived from E24
        if( aSeries <= 24 )
        {
            const size_t skipValue = 24 / aSeries;
            return s_e24[aIndex * skipValue];
        }
        // E48, E96, E192 are derived from E192
        else
        {
            const size_t skipValue = 192 / aSeries;
            return s_e192[aIndex * skipValue];
        }
    }

    static auto getSeriesSize( int aSeries ) -> size_t
    {
        return static_cast<size_t>( aSeries );
    }

public:
    static auto FindNearest( double aValue, const std::string& aSeries ) -> std::optional<double>
    {
        const int series = parseSeriesString( aSeries );
        if( series < 0 )
            return std::nullopt;

        if( aValue <= 0.0 )
            return std::nullopt;

        // Scale value to 100-999 decade
        const double logValue = std::log10( aValue );
        const int    decade = static_cast<int>( std::floor( logValue ) );
        const double scaledValue = aValue / std::pow( 10.0, decade );
        const double normalized = scaledValue * 100.0;

        // Find nearest value in series
        const size_t seriesSize = getSeriesSize( series );
        double       minDiff = std::numeric_limits<double>::max();
        uint16_t     nearest = 100;

        for( size_t i = 0; i < seriesSize; ++i )
        {
            const uint16_t val = getSeriesValue( series, i );
            const double   diff = std::abs( normalized - val );
            if( diff < minDiff )
            {
                minDiff = diff;
                nearest = val;
            }
        }

        // Scale back to original decade
        return ( nearest / 100.0 ) * std::pow( 10.0, decade );
    }

    static auto FindUp( double aValue, const std::string& aSeries ) -> std::optional<double>
    {
        const int series = parseSeriesString( aSeries );
        if( series < 0 )
            return std::nullopt;

        if( aValue <= 0.0 )
            return std::nullopt;

        // Scale value to 100-999 decade
        const double logValue = std::log10( aValue );
        const int    decade = static_cast<int>( std::floor( logValue ) );
        const double scaledValue = aValue / std::pow( 10.0, decade );
        const double normalized = scaledValue * 100.0;

        // Find next higher value in series
        const size_t seriesSize = getSeriesSize( series );

        // Check current decade
        for( size_t i = 0; i < seriesSize; ++i )
        {
            const uint16_t val = getSeriesValue( series, i );
            if( val > normalized )
                return ( val / 100.0 ) * std::pow( 10.0, decade );
        }

        // Wrap to next decade
        const uint16_t firstVal = getSeriesValue( series, 0 );
        return ( firstVal / 100.0 ) * std::pow( 10.0, decade + 1 );
    }

    static auto FindDown( double aValue, const std::string& aSeries ) -> std::optional<double>
    {
        const int series = parseSeriesString( aSeries );
        if( series < 0 )
            return std::nullopt;

        if( aValue <= 0.0 )
            return std::nullopt;

        // Scale value to 100-999 decade
        const double logValue = std::log10( aValue );
        const int    decade = static_cast<int>( std::floor( logValue ) );
        const double scaledValue = aValue / std::pow( 10.0, decade );
        const double normalized = scaledValue * 100.0;

        // Find next lower value in series
        const size_t seriesSize = getSeriesSize( series );

        // Check current decade (search backwards)
        for( int i = seriesSize - 1; i >= 0; --i )
        {
            const uint16_t val = getSeriesValue( series, i );
            if( val < normalized )
                return ( val / 100.0 ) * std::pow( 10.0, decade );
        }

        // Wrap to previous decade
        const uint16_t lastVal = getSeriesValue( series, seriesSize - 1 );
        return ( lastVal / 100.0 ) * std::pow( 10.0, decade - 1 );
    }
};


EVAL_VISITOR::EVAL_VISITOR( VariableCallback aVariableCallback, ERROR_COLLECTOR& aErrorCollector ) :
        m_variableCallback( std::move( aVariableCallback ) ),
        m_errors( aErrorCollector ),
        m_gen( m_rd() )
{
}

auto EVAL_VISITOR::operator()( const NODE& aNode ) const -> Result<Value>
{
    switch( aNode.type )
    {
    case NodeType::Number: return MakeValue<Value>( std::get<double>( aNode.data ) );

    case NodeType::String: return MakeValue<Value>( std::get<std::string>( aNode.data ) );

    case NodeType::Var:
    {
        const auto& varName = std::get<std::string>( aNode.data );

        // Use callback to resolve variable
        if( m_variableCallback )
            return m_variableCallback( varName );

        return MakeError<Value>( fmt::format( "No variable resolver configured for: {}", varName ) );
    }

    case NodeType::BinOp:
    {
        const auto& binop = std::get<BIN_OP_DATA>( aNode.data );
        auto        leftResult = binop.left->Accept( *this );
        if( !leftResult )
            return leftResult;

        auto rightResult = binop.right ? binop.right->Accept( *this ) : MakeValue<Value>( 0.0 );
        if( !rightResult )
            return rightResult;

        // Special handling for string concatenation with +
        if( binop.op == '+' )
        {
            const auto& leftVal = leftResult.GetValue();
            const auto& rightVal = rightResult.GetValue();

            // If either operand is a string, concatenate
            if( std::holds_alternative<std::string>( leftVal ) || std::holds_alternative<std::string>( rightVal ) )
            {
                return MakeValue<Value>( VALUE_UTILS::ConcatStrings( leftVal, rightVal ) );
            }
        }

        // Special handling for string comparisons with == and !=
        if( binop.op == 3 || binop.op == 4 ) // == or !=
        {
            const auto& leftVal = leftResult.GetValue();
            const auto& rightVal = rightResult.GetValue();

            // If both operands are strings, do string comparison
            if( std::holds_alternative<std::string>( leftVal ) && std::holds_alternative<std::string>( rightVal ) )
            {
                bool   equal = std::get<std::string>( leftVal ) == std::get<std::string>( rightVal );
                double result = ( binop.op == 3 ) ? ( equal ? 1.0 : 0.0 ) : ( equal ? 0.0 : 1.0 );
                return MakeValue<Value>( result );
            }
        }

        // Otherwise, perform arithmetic
        return VALUE_UTILS::ArithmeticOp( leftResult.GetValue(), rightResult.GetValue(), binop.op );
    }

    case NodeType::Function:
    {
        const auto& func = std::get<FUNC_DATA>( aNode.data );
        return evaluateFunction( func );
    }

    default: return MakeError<Value>( "Cannot evaluate this node type" );
    }
}

auto EVAL_VISITOR::evaluateFunction( const FUNC_DATA& aFunc ) const -> Result<Value>
{
    const auto& name = aFunc.name;
    const auto& args = aFunc.args;

    // Zero-argument functions
    if( args.empty() )
    {
        if( name == "today" )
            return MakeValue<Value>( static_cast<double>( DATE_UTILS::GetCurrentDays() ) );
        else if( name == "now" )
            return MakeValue<Value>( DATE_UTILS::GetCurrentTimestamp() );
        else if( name == "random" )
        {
            std::uniform_real_distribution<double> dis( 0.0, 1.0 );
            return MakeValue<Value>( dis( m_gen ) );
        }
    }

    // Evaluate arguments to mixed types
    std::vector<Value> argValues;
    argValues.reserve( args.size() );

    for( const auto& arg : args )
    {
        auto result = arg->Accept( *this );
        if( !result )
            return result;

        argValues.push_back( result.GetValue() );
    }

    const auto argc = argValues.size();

    // String formatting functions (return strings!)
    if( name == "format" && argc >= 1 )
    {
        auto numResult = VALUE_UTILS::ToDouble( argValues[0] );
        if( !numResult )
            return MakeError<Value>( numResult.GetError() );

        const auto value = numResult.GetValue();
        int        decimals = 2;
        if( argc > 1 )
        {
            auto decResult = VALUE_UTILS::ToDouble( argValues[1] );
            if( decResult )
                decimals = static_cast<int>( decResult.GetValue() );
        }

        return MakeValue<Value>( fmt::format( "{:.{}f}", value, decimals ) );
    }
    else if( name == "currency" && argc >= 1 )
    {
        auto numResult = VALUE_UTILS::ToDouble( argValues[0] );
        if( !numResult )
            return MakeError<Value>( numResult.GetError() );

        const auto amount = numResult.GetValue();
        const auto symbol = argc > 1 ? VALUE_UTILS::ToString( argValues[1] ) : "$";

        return MakeValue<Value>( fmt::format( "{}{:.2f}", symbol, amount ) );
    }
    else if( name == "fixed" && argc >= 1 )
    {
        auto numResult = VALUE_UTILS::ToDouble( argValues[0] );
        if( !numResult )
            return MakeError<Value>( numResult.GetError() );

        const auto value = numResult.GetValue();
        int        decimals = 2;
        if( argc > 1 )
        {
            auto decResult = VALUE_UTILS::ToDouble( argValues[1] );
            if( decResult )
                decimals = static_cast<int>( decResult.GetValue() );
        }

        return MakeValue<Value>( fmt::format( "{:.{}f}", value, decimals ) );
    }

    // Date formatting functions (return strings!)
    else if( name == "dateformat" && argc >= 1 )
    {
        auto dateResult = VALUE_UTILS::ToDouble( argValues[0] );
        if( !dateResult )
            return MakeError<Value>( dateResult.GetError() );

        const auto days = static_cast<int>( dateResult.GetValue() );
        const auto format = argc > 1 ? VALUE_UTILS::ToString( argValues[1] ) : "ISO";

        return MakeValue<Value>( DATE_UTILS::FormatDate( days, format ) );
    }
    else if( name == "datestring" && argc == 1 )
    {
        auto dateStr = VALUE_UTILS::ToString( argValues[0] );
        auto daysResult = DATE_UTILS::ParseDate( dateStr );

        if( !daysResult )
            return MakeError<Value>( "Invalid date format: " + dateStr );

        return MakeValue<Value>( static_cast<double>( daysResult.value() ) );
    }
    else if( name == "weekdayname" && argc == 1 )
    {
        auto dateResult = VALUE_UTILS::ToDouble( argValues[0] );
        if( !dateResult )
            return MakeError<Value>( dateResult.GetError() );

        const auto days = static_cast<int>( dateResult.GetValue() );
        return MakeValue<Value>( DATE_UTILS::GetWeekdayName( days ) );
    }

    // String functions (return strings!)
    else if( name == "upper" && argc == 1 )
    {
        auto str = VALUE_UTILS::ToString( argValues[0] );
        std::transform( str.begin(), str.end(), str.begin(), ::toupper );
        return MakeValue<Value>( str );
    }
    else if( name == "lower" && argc == 1 )
    {
        auto str = VALUE_UTILS::ToString( argValues[0] );
        std::transform( str.begin(), str.end(), str.begin(), ::tolower );
        return MakeValue<Value>( str );
    }
    else if( name == "concat" && argc >= 2 )
    {
        std::string result;
        for( const auto& val : argValues )
            result += VALUE_UTILS::ToString( val );

        return MakeValue<Value>( result );
    }
    else if( name == "beforefirst" && argc == 2 )
    {
        wxString result = VALUE_UTILS::ToString( argValues[0] );

        result = result.BeforeFirst( VALUE_UTILS::ToChar( argValues[1] ) );
        return MakeValue<Value>( result.ToStdString() );
    }
    else if( name == "beforelast" && argc == 2 )
    {
        wxString result = VALUE_UTILS::ToString( argValues[0] );

        result = result.BeforeLast( VALUE_UTILS::ToChar( argValues[1] ) );
        return MakeValue<Value>( result.ToStdString() );
    }
    else if( name == "afterfirst" && argc == 2 )
    {
        wxString result = VALUE_UTILS::ToString( argValues[0] );

        result = result.AfterFirst( VALUE_UTILS::ToChar( argValues[1] ) );
        return MakeValue<Value>( result.ToStdString() );
    }
    else if( name == "afterlast" && argc == 2 )
    {
        wxString result = VALUE_UTILS::ToString( argValues[0] );

        result = result.AfterLast( VALUE_UTILS::ToChar( argValues[1] ) );
        return MakeValue<Value>( result.ToStdString() );
    }

    // Conditional functions (handle mixed types)
    if( name == "if" && argc == 3 )
    {
        // Convert only the condition to a number
        auto conditionResult = VALUE_UTILS::ToDouble( argValues[0] );
        if( !conditionResult )
            return MakeError<Value>( conditionResult.GetError() );

        const auto condition = conditionResult.GetValue() != 0.0;
        return MakeValue<Value>( condition ? argValues[1] : argValues[2] );
    }

    // E-series functions (handle value as number, series as string)
    else if( ( name == "enearest" || name == "eup" || name == "edown" ) && argc >= 1 && argc <= 2 )
    {
        auto valueResult = VALUE_UTILS::ToDouble( argValues[0] );
        if( !valueResult )
            return MakeError<Value>( valueResult.GetError() );

        const auto   value = valueResult.GetValue();
        const auto   series = argc > 1 ? VALUE_UTILS::ToString( argValues[1] ) : "E24";
        std::optional<double> result;

        if( name == "enearest" )
            result = ESERIES_UTILS::FindNearest( value, series );
        else if( name == "eup" )
            result = ESERIES_UTILS::FindUp( value, series );
        else if( name == "edown" )
            result = ESERIES_UTILS::FindDown( value, series );

        if( !result )
            return MakeError<Value>( fmt::format( "Invalid E-series: {}", series ) );

        return MakeValue<Value>( result.value() );
    }

    // Mathematical functions (return numbers) - convert args to doubles first
    std::vector<double> numArgs;
    for( const auto& val : argValues )
    {
        auto numResult = VALUE_UTILS::ToDouble( val );
        if( !numResult )
            return MakeError<Value>( numResult.GetError() );

        numArgs.push_back( numResult.GetValue() );
    }

    // Mathematical function implementations
    if( name == "abs" && argc == 1 )
        return MakeValue<Value>( std::abs( numArgs[0] ) );
    else if( name == "sum" && argc >= 1 )
        return MakeValue<Value>( std::accumulate( numArgs.begin(), numArgs.end(), 0.0 ) );
    else if( name == "round" && argc >= 1 )
    {
        const auto value = numArgs[0];
        const auto precision = argc > 1 ? static_cast<int>( numArgs[1] ) : 0;
        const auto multiplier = std::pow( 10.0, precision );
        return MakeValue<Value>( std::round( value * multiplier ) / multiplier );
    }
    else if( name == "sqrt" && argc == 1 )
    {
        if( numArgs[0] < 0 )
            return MakeError<Value>( "Square root of negative number" );

        return MakeValue<Value>( std::sqrt( numArgs[0] ) );
    }
    else if( name == "pow" && argc == 2 )
        return MakeValue<Value>( std::pow( numArgs[0], numArgs[1] ) );
    else if( name == "floor" && argc == 1 )
        return MakeValue<Value>( std::floor( numArgs[0] ) );
    else if( name == "ceil" && argc == 1 )
        return MakeValue<Value>( std::ceil( numArgs[0] ) );
    else if( name == "min" && argc >= 1 )
        return MakeValue<Value>( *std::min_element( numArgs.begin(), numArgs.end() ) );
    else if( name == "max" && argc >= 1 )
        return MakeValue<Value>( *std::max_element( numArgs.begin(), numArgs.end() ) );
    else if( name == "avg" && argc >= 1 )
    {
        const auto sum = std::accumulate( numArgs.begin(), numArgs.end(), 0.0 );
        return MakeValue<Value>( sum / static_cast<double>( argc ) );
    }
    else if( name == "shunt" && argc == 2 )
    {
        const auto r1 = numArgs[0];
        const auto r2 = numArgs[1];
        const auto sum = r1 + r2;

        // Calculate parallel resistance: (r1*r2)/(r1+r2)
        // If sum is not positive, return 0.0 (handles edge cases like shunt(0,0))
        if( sum > 0.0 )
            return MakeValue<Value>( ( r1 * r2 ) / sum );
        else
            return MakeValue<Value>( 0.0 );
    }
    else if( name == "db" && argc == 1 )
    {
        // Power ratio to dB: 10*log10(ratio)
        if( numArgs[0] <= 0.0 )
            return MakeError<Value>( "db() argument must be positive" );

        return MakeValue<Value>( 10.0 * std::log10( numArgs[0] ) );
    }
    else if( name == "dbv" && argc == 1 )
    {
        // Voltage/current ratio to dB: 20*log10(ratio)
        if( numArgs[0] <= 0.0 )
            return MakeError<Value>( "dbv() argument must be positive" );

        return MakeValue<Value>( 20.0 * std::log10( numArgs[0] ) );
    }
    else if( name == "fromdb" && argc == 1 )
    {
        // dB to power ratio: 10^(dB/10)
        return MakeValue<Value>( std::pow( 10.0, numArgs[0] / 10.0 ) );
    }
    else if( name == "fromdbv" && argc == 1 )
    {
        // dB to voltage/current ratio: 10^(dB/20)
        return MakeValue<Value>( std::pow( 10.0, numArgs[0] / 20.0 ) );
    }

    return MakeError<Value>( fmt::format( "Unknown function: {} with {} arguments", name, argc ) );
}

auto DOC_PROCESSOR::Process( const DOC& aDoc, VariableCallback aVariableCallback ) -> std::pair<std::string, bool>
{
    std::string  result;
    auto         localErrors = ERROR_COLLECTOR{};
    EVAL_VISITOR evaluator{ std::move( aVariableCallback ), localErrors };
    bool         hadErrors = aDoc.HasErrors();

    for( const auto& node : aDoc.GetNodes() )
    {
        switch( node->type )
        {
        case NodeType::Text: result += std::get<std::string>( node->data ); break;

        case NodeType::Calc:
        {
            const auto& calcData = std::get<BIN_OP_DATA>( node->data );
            auto        evalResult = calcData.left->Accept( evaluator );

            if( evalResult )
                result += VALUE_UTILS::ToString( evalResult.GetValue() );
            else
            {
                // Don't add error formatting to result - errors go to error vector only
                // The higher level will return original input unchanged if there are errors
                hadErrors = true;
            }
            break;
        }

        default:
            result += "[Unknown node type]";
            hadErrors = true;
            break;
        }
    }

    return { std::move( result ), hadErrors || localErrors.HasErrors() };
}

auto DOC_PROCESSOR::ProcessWithDetails( const DOC& aDoc, VariableCallback aVariableCallback )
        -> std::tuple<std::string, std::vector<std::string>, bool>
{
    auto [result, hadErrors] = Process( aDoc, std::move( aVariableCallback ) );
    auto allErrors = aDoc.GetErrors();

    return { std::move( result ), std::move( allErrors ), hadErrors };
}

} // namespace calc_parser
