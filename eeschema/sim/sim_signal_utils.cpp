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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <sim/sim_signal_utils.h>

#include <algorithm>
#include <set>

#include <wx/wxcrt.h>


namespace
{

bool isIdentChar( wxUniChar aChar )
{
    return wxIsalnum( aChar ) || aChar == '_';
}


bool isGround( const wxString& aNet )
{
    return aNet.IsSameAs( wxS( "GND" ), false ) || aNet == wxS( "0" );
}

} // namespace


namespace SIM_SIGNAL
{

bool IsSingleVoltageRef( const wxString& aExpression )
{
    if( aExpression.Length() < 4 || !aExpression.Upper().StartsWith( wxS( "V(" ) ) )
        return false;

    int depth = 0;

    for( size_t ii = 1; ii < aExpression.Length(); ++ii )
    {
        if( aExpression[ii] == '(' )
        {
            depth++;
        }
        else if( aExpression[ii] == ')' && --depth == 0 )
        {
            return ii == aExpression.Length() - 1;
        }
    }

    return false;
}


bool IsMeasVector( const wxString& aSignal )
{
    static const std::set<wxString> refs = { wxS( "V" ),  wxS( "VDB" ), wxS( "VM" ), wxS( "VP" ),
                                             wxS( "VR" ), wxS( "VI" ),  wxS( "I" ) };

    int open = aSignal.Find( '(' );

    if( open == wxNOT_FOUND )
        return !aSignal.IsEmpty() && aSignal.find_first_of( wxS( " +-*/^,<>=" ) ) == wxString::npos;

    if( !refs.count( aSignal.Left( open ).Upper() ) )
        return false;

    int  depth = 0;
    bool quoted = false;

    for( size_t ii = open; ii < aSignal.Length(); ++ii )
    {
        if( aSignal[ii] == '"' )
            quoted = !quoted;
        else if( quoted )
            continue;
        else if( aSignal[ii] == '(' )
            depth++;
        else if( aSignal[ii] == ')' && --depth == 0 )
            return ii == aSignal.Length() - 1;
    }

    return false;
}


bool MeasurableSignal( const wxString& aBase, PART aPart, wxString& aResult )
{
    switch( aPart )
    {
    case PART::PHASE:
        return false;

    case PART::GAIN:
        if( IsSingleVoltageRef( aBase ) )
            aResult = wxS( "vdb" ) + aBase.Mid( 1 );
        else
            aResult = wxS( "db(" ) + aBase + wxS( ")" );

        return true;

    case PART::AMPLITUDE:
        aResult = wxS( "mag(" ) + aBase + wxS( ")" );
        return true;

    case PART::VALUE:
        break;
    }

    aResult = aBase;
    return true;
}


void SplitMeasurement( const wxString& aCmd, wxString& aFunc, wxString& aSignal, wxString& aParams )
{
    wxString cmd = aCmd;
    cmd.Trim( false ).Trim( true );

    aFunc = cmd;
    aSignal.clear();
    aParams.clear();

    size_t space = cmd.find_first_of( wxS( " \t" ) );

    if( space == wxString::npos )
        return;

    aFunc = cmd.Left( space );

    wxString rest = cmd.Mid( space );
    rest.Trim( false );

    size_t len = rest.Length();
    size_t paramStart = wxString::npos;
    int    depth = 0;
    bool   quoted = false;

    for( size_t ii = 0; ii < len && paramStart == wxString::npos; ++ii )
    {
        wxUniChar ch = rest[ii];

        if( ch == '"' )
            quoted = !quoted;

        if( quoted || ch == '"' )
            continue;

        if( ch == '(' )
        {
            depth++;
        }
        else if( ch == ')' )
        {
            depth--;
        }
        else if( depth == 0 && ii > 0 && wxIsspace( rest[ii - 1] ) && wxIsalpha( ch ) )
        {
            size_t jj = ii;

            while( jj < len && isIdentChar( rest[jj] ) )
                jj++;

            while( jj < len && wxIsspace( rest[jj] ) )
                jj++;

            // "==" is a comparison, not an assignment
            if( jj < len && rest[jj] == '=' && ( jj + 1 == len || rest[jj + 1] != '=' ) )
                paramStart = ii;
        }
    }

    aSignal = rest.Left( paramStart );
    aSignal.Trim( true );

    if( paramStart != wxString::npos )
    {
        aParams = rest.Mid( paramStart );
        aParams.Trim( true );
    }
}


wxString ExpandDeviceVoltages( const wxString& aExpression, const std::map<wxString, wxString>& aDeviceExprs )
{
    if( aDeviceExprs.empty() )
        return aExpression;

    // VDB(R1), VM(R1), ... wrap the difference in the ngspice function they stand for
    static const std::pair<wxString, wxString> forms[] = {
        { wxS( "VDB(" ), wxS( "db" ) }, { wxS( "VM(" ), wxS( "mag" ) },  { wxS( "VP(" ), wxS( "ph" ) },
        { wxS( "VR(" ), wxS( "real" ) }, { wxS( "VI(" ), wxS( "imag" ) }, { wxS( "V(" ), wxS( "" ) }
    };

    std::map<wxString, wxString> byUpper;

    for( const auto& [ref, expr] : aDeviceExprs )
        byUpper[ref.Upper()] = expr;

    wxString upper = aExpression.Upper();
    wxString result;
    size_t   len = aExpression.Length();
    bool     quoted = false;

    for( size_t ii = 0; ii < len; )
    {
        if( aExpression[ii] == '"' )
            quoted = !quoted;

        bool matched = false;

        if( !quoted && ( ii == 0 || !isIdentChar( aExpression[ii - 1] ) ) )
        {
            for( const auto& [prefix, function] : forms )
            {
                if( upper.compare( ii, prefix.Length(), prefix ) != 0 )
                    continue;

                size_t open = ii + prefix.Length();
                size_t close = aExpression.find( ')', open );

                if( close == wxString::npos )
                    break;

                wxString ref = upper.Mid( open, close - open );
                ref.Trim( true ).Trim( false );

                auto it = byUpper.find( ref );

                if( it == byUpper.end() )
                    break;

                result << function << wxS( "(" ) << it->second << wxS( ")" );
                ii = close + 1;
                matched = true;
                break;
            }
        }

        if( !matched )
            result << aExpression[ii++];
    }

    return result;
}


wxString QuoteNetNames( const wxString& aExpression, const std::vector<wxString>& aNetnames )
{
    size_t            len = aExpression.length();
    std::vector<bool> mask( len, false );
    std::vector<bool> userQuoted( len, false );
    wxString          lower = aExpression.Lower();

    auto isNetnameChar = []( wxUniChar aChar ) -> bool
    {
        wxUint32 value = aChar.GetValue();

        if( ( value >= '0' && value <= '9' ) || ( value >= 'A' && value <= 'Z' ) || ( value >= 'a' && value <= 'z' ) )
            return true;

        switch( value )
        {
        case '_':
        case '/':
        case '+':
        case '-':
        case '~':
        case '.':
            return true;

        default:
            return false;
        }
    };

    bool inQuote = false;

    for( size_t ii = 0; ii < len; ++ii )
    {
        if( aExpression[ii] == '"' )
            inQuote = !inQuote;

        userQuoted[ii] = inQuote || aExpression[ii] == '"';
    }

    for( const wxString& netname : aNetnames )
    {
        if( netname.IsEmpty() )
            continue;

        wxString needle = netname.Lower();
        size_t   pos = lower.find( needle );

        while( pos != wxString::npos )
        {
            size_t end = pos + needle.length();

            // Whole names only, so not "IN" in "SIN(" nor "A" in net "A-B"
            bool whole = ( pos == 0 || !isNetnameChar( aExpression[pos - 1] ) )
                         && ( end == len || !isNetnameChar( aExpression[end] ) );

            if( whole && !userQuoted[pos] )
            {
                for( size_t ii = pos; ii < end; ++ii )
                    mask[ii] = true;
            }

            pos = lower.find( needle, pos + 1 );
        }
    }

    wxString quoted;
    bool     open = false;

    for( size_t ii = 0; ii < len; ++ii )
    {
        if( mask[ii] != open )
        {
            quoted << '"';
            open = mask[ii];
        }

        quoted << aExpression[ii];
    }

    if( open )
        quoted << '"';

    return quoted;
}


bool ParensBalanced( const wxString& aExpression )
{
    int  depth = 0;
    bool inQuote = false;

    for( wxUniChar ch : aExpression )
    {
        // Quoted net names may contain parentheses
        if( ch == '"' )
        {
            inQuote = !inQuote;
            continue;
        }

        if( inQuote )
            continue;

        if( ch == '(' )
        {
            depth++;
        }
        else if( ch == ')' )
        {
            if( --depth < 0 )
                return false;
        }
    }

    return depth == 0;
}


std::vector<wxString> ExtractSignalRefs( const wxString& aExpression )
{
    std::vector<wxString> refs;
    size_t                len = aExpression.length();

    for( size_t ii = 0; ii < len; ++ii )
    {
        // Whole identifiers only, since the 'v' in "div(" is not a voltage
        if( ii > 0 && isIdentChar( aExpression[ii - 1] ) )
            continue;

        wxUniChar prefix = wxToupper( aExpression[ii] );

        if( prefix != 'V' && prefix != 'I' && prefix != 'P' )
            continue;

        size_t open = ii + 1;

        // Only voltages have VDB(), VM(), VP(), VR() and VI(); "im(" is ngspice's imaginary part
        if( prefix == 'V' )
        {
            static const wxString variants[] = { wxS( "DB" ), wxS( "M" ), wxS( "P" ), wxS( "R" ), wxS( "I" ) };

            for( const wxString& variant : variants )
            {
                size_t after = open + variant.length();

                if( after < len && aExpression.Mid( open, variant.length() ).Upper() == variant
                    && aExpression[after] == '(' )
                {
                    open = after;
                    break;
                }
            }
        }

        if( open >= len || aExpression[open] != '(' )
            continue;

        std::vector<wxString> args;
        wxString              arg;
        int                   depth = 0;
        bool                  inQuote = false;
        size_t                jj = open;

        for( ; jj < len; ++jj )
        {
            wxUniChar ch = aExpression[jj];

            if( ch == '"' )
                inQuote = !inQuote;

            // Quoted net names may contain parentheses and commas
            if( inQuote || ch == '"' )
            {
                arg += ch;
                continue;
            }

            if( ch == '(' )
            {
                if( ++depth == 1 )
                    continue;
            }
            else if( ch == ')' )
            {
                if( --depth == 0 )
                {
                    args.push_back( arg );
                    break;
                }
            }
            else if( ch == ',' && depth == 1 )
            {
                args.push_back( arg );
                arg.clear();
                continue;
            }

            arg += ch;
        }

        for( wxString& each : args )
        {
            each.Replace( wxS( "\"" ), wxEmptyString );
            each.Trim( true ).Trim( false );

            if( each.IsEmpty() || isGround( each ) )
                continue;

            refs.push_back( wxString( prefix ) + wxS( "(" ) + each + wxS( ")" ) );
        }

        ii = jj;
    }

    return refs;
}


wxString DeviceVoltageExpression( const std::vector<std::string>& aModelPins,
                                  const std::vector<std::string>& aPinNumbers,
                                  const std::vector<std::string>& aPinNetNames )
{
    if( aModelPins.size() != 2 )
        return wxEmptyString;

    wxString nets[2];

    for( size_t ii = 0; ii < 2; ++ii )
    {
        auto it = std::find( aPinNumbers.begin(), aPinNumbers.end(), aModelPins[ii] );

        if( it == aPinNumbers.end() )
            return wxEmptyString;

        size_t index = std::distance( aPinNumbers.begin(), it );

        if( index >= aPinNetNames.size() )
            return wxEmptyString;

        nets[ii] = wxString( aPinNetNames[index] );
    }

    const wxString& netA = nets[0];
    const wxString& netB = nets[1];

    if( netA.IsEmpty() || netB.IsEmpty() || netA == netB )
        return wxEmptyString;

    if( isGround( netA ) && isGround( netB ) )
        return wxEmptyString;

    if( isGround( netB ) )
        return wxString::Format( wxS( "V(%s)" ), netA );

    if( isGround( netA ) )
        return wxString::Format( wxS( "-V(%s)" ), netB );

    return wxString::Format( wxS( "V(%s) - V(%s)" ), netA, netB );
}


bool IsDerivedSignalVector( const wxString& aVectorName )
{
    wxString lower = aVectorName.Lower();
    wxString digits;

    if( !lower.StartsWith( wxS( "user" ), &digits ) && !lower.StartsWith( DEVICE_VECTOR_PREFIX, &digits ) )
        return false;

    return !digits.IsEmpty() && digits.find_first_not_of( wxS( "0123456789" ) ) == wxString::npos;
}

} // namespace SIM_SIGNAL
