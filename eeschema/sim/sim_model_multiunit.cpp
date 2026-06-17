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

#include <sim/sim_model_multiunit.h>

#include <cstdint>
#include <map>
#include <set>
#include <utility>
#include <fmt/core.h>
#include <ki_exception.h>
#include <wx/intl.h>
#include <wx/tokenzr.h>


SIM_DECOMPOSITION SIM_DECOMPOSITION::Parse( const wxString& aField )
{
    SIM_DECOMPOSITION     result;   // defaults to WHOLE_DEVICE
    std::vector<wxString> shared;
    bool                  repeat = false;

    wxStringTokenizer     tokenizer( aField, wxS( " \t\r\n" ), wxTOKEN_STRTOK );

    while( tokenizer.HasMoreTokens() )
    {
        wxString token = tokenizer.GetNextToken();

        if( token.StartsWith( wxS( "mode=" ) ) )
        {
            if( token.Mid( 5 ).IsSameAs( wxS( "repeat" ), false ) )
                repeat = true;
        }
        else if( token.StartsWith( wxS( "shared=" ) ) )
        {
            wxStringTokenizer pins( token.Mid( 7 ), wxS( "," ), wxTOKEN_STRTOK );

            while( pins.HasMoreTokens() )
                shared.push_back( pins.GetNextToken() );
        }
    }

    // Shared pins only have meaning in repeat mode.  Anything that does not
    // explicitly select repeat (empty field, unknown mode) stays whole-device so
    // it round-trips back to an empty field.
    if( repeat )
    {
        result.mode            = MODE::REPEAT_PER_UNIT;
        result.sharedModelPins = std::move( shared );
    }

    return result;
}


wxString SIM_DECOMPOSITION::Format() const
{
    if( mode != MODE::REPEAT_PER_UNIT )
        return wxEmptyString;

    wxString result = wxS( "mode=repeat" );

    if( !sharedModelPins.empty() )
    {
        result += wxS( " shared=" );

        for( size_t ii = 0; ii < sharedModelPins.size(); ++ii )
        {
            if( ii > 0 )
                result += wxS( "," );

            result += sharedModelPins[ii];
        }
    }

    return result;
}


std::vector<std::pair<wxString, wxString>> ParseSimPinsTokens( const wxString& aPins,
                                                               const wxString& aRef )
{
    std::vector<std::pair<wxString, wxString>> pairs;
    std::map<wxString, wxString>               seen;   // symbolPin -> modelPin

    wxStringTokenizer tokenizer( aPins, wxS( " \t\r\n" ), wxTOKEN_STRTOK );

    while( tokenizer.HasMoreTokens() )
    {
        wxString token = tokenizer.GetNextToken();
        int      pos = token.Find( wxS( '=' ) );

        if( pos == wxNOT_FOUND || pos == 0 || pos == (int) token.length() - 1 )
        {
            THROW_IO_ERROR( wxString::Format(
                    _( "Symbol '%s' has a malformed Sim.Pins entry '%s'." ), aRef, token ) );
        }

        wxString symbolPin = token.Left( pos );
        wxString modelPin  = token.Mid( pos + 1 );

        auto [it, inserted] = seen.try_emplace( symbolPin, modelPin );

        if( !inserted )
        {
            if( it->second != modelPin )
            {
                THROW_IO_ERROR( wxString::Format(
                        _( "Symbol '%s' maps pin '%s' to both '%s' and '%s'." ),
                        aRef, symbolPin, it->second, modelPin ) );
            }

            continue;
        }

        pairs.emplace_back( symbolPin, modelPin );
    }

    return pairs;
}


// Encodes a string into an injective, SPICE-legal identifier: ASCII alphanumerics are kept as-is
// (so the common numeric pin "3" stays readable as "3"), every other byte becomes "_XX".  Because
// only escapes introduce '_', distinct inputs never collide on the output.
static wxString encodeIdentifier( const wxString& aRaw )
{
    std::string encoded;

    for( unsigned char c : aRaw.ToStdString( wxConvUTF8 ) )
    {
        if( ( c >= '0' && c <= '9' ) || ( c >= 'A' && c <= 'Z' ) || ( c >= 'a' && c <= 'z' ) )
            encoded += static_cast<char>( c );
        else
            encoded += fmt::format( "_{:02X}", c );
    }

    return wxString::FromUTF8( encoded.c_str() );
}


// Maps a symbol pin number to a unique subcircuit node name.  The "n" prefix keeps it a legal
// identifier distinct from the global ground node "0"; encodeIdentifier() guarantees that distinct
// pin numbers (even "1-2" vs "1_2") map to distinct nodes.
static wxString nodeName( const wxString& aSymbolPin )
{
    return wxS( "n" ) + encodeIdentifier( aSymbolPin );
}


// Deterministic 64-bit FNV-1a hash.  std::hash is not stable across standard-library
// implementations, but the wrapper signature is netlist-visible and must be reproducible.
static uint64_t stableHash64( const std::string& aText )
{
    uint64_t hash = 14695981039346656037ull;

    for( unsigned char c : aText )
    {
        hash ^= c;
        hash *= 1099511628211ull;
    }

    return hash;
}


SIM_MODEL_MULTIUNIT::SIM_MODEL_MULTIUNIT( const SIM_MODEL& aBaseModel, const wxString& aBaseModelName,
                                          const std::vector<UNIT_PIN_MAP>& aUnitMaps,
                                          const std::vector<wxString>& aSharedModelPins ) :
        SIM_MODEL_SPICE( TYPE::SUBCKT, std::make_unique<SPICE_GENERATOR_MULTIUNIT>( *this ) ),
        m_baseModelName( aBaseModelName )
{
    // Subcircuit instance parameters would have to be declared on the wrapper header and forwarded
    // to each inner instance; that is out of scope for v1, so refuse rather than drop them.
    for( int ii = 0; ii < aBaseModel.GetParamCount(); ++ii )
    {
        if( aBaseModel.GetParam( ii ).info.isSpiceInstanceParam )
        {
            THROW_IO_ERROR( wxString::Format(
                    _( "Repeat-per-unit decomposition does not support model '%s' because it has "
                       "subcircuit parameters." ),
                    aBaseModelName ) );
        }
    }

    std::vector<wxString> basePinOrder;   // model-pin names in base header order
    std::set<wxString>    basePinSet;

    for( const SIM_MODEL_PIN& pin : aBaseModel.GetPins() )
    {
        wxString name( pin.modelPinName );
        basePinOrder.push_back( name );
        basePinSet.insert( name );
    }

    std::set<wxString> sharedSet;

    for( const wxString& shared : aSharedModelPins )
    {
        if( !basePinSet.count( shared ) )
        {
            THROW_IO_ERROR( wxString::Format(
                    _( "Shared pin '%s' is not a pin of model '%s'." ), shared, aBaseModelName ) );
        }

        sharedSet.insert( shared );
    }

    // Reverse each unit's map to model-pin -> symbol-pin (within-unit conflicts already rejected).
    struct UNIT_INFO
    {
        int                          unit = 0;
        std::map<wxString, wxString> modelToSymbol;
    };

    std::vector<UNIT_INFO> units;

    for( const UNIT_PIN_MAP& unitMap : aUnitMaps )
    {
        UNIT_INFO info;
        info.unit = unitMap.unit;

        for( const auto& [symbolPin, modelPin] : unitMap.pins )
        {
            // An unknown model pin (typically a typo) would otherwise be silently ignored while
            // still counting the unit as an instance, so reject it.
            if( !basePinSet.count( modelPin ) )
            {
                THROW_IO_ERROR( wxString::Format(
                        _( "Unit %d maps to unknown pin '%s' of model '%s'." ), unitMap.unit,
                        modelPin, aBaseModelName ) );
            }

            auto [it, inserted] = info.modelToSymbol.emplace( modelPin, symbolPin );

            if( !inserted && it->second != symbolPin )
            {
                THROW_IO_ERROR( wxString::Format(
                        _( "Unit %d maps model pin '%s' to both symbol pins '%s' and '%s'." ),
                        unitMap.unit, modelPin, it->second, symbolPin ) );
            }
        }

        units.push_back( std::move( info ) );
    }

    auto mapsNonShared =
            [&]( const UNIT_INFO& aUnit )
            {
                for( const auto& [modelPin, symbolPin] : aUnit.modelToSymbol )
                {
                    if( !sharedSet.count( modelPin ) )
                        return true;
                }

                return false;
            };

    // Resolve each shared model pin to exactly one node (taken from whichever unit maps it).
    std::map<wxString, wxString> sharedNode;

    for( const wxString& shared : aSharedModelPins )
    {
        std::set<wxString> symbolPins;

        for( const UNIT_INFO& unit : units )
        {
            if( auto it = unit.modelToSymbol.find( shared ); it != unit.modelToSymbol.end() )
                symbolPins.insert( it->second );
        }

        if( symbolPins.empty() )
        {
            THROW_IO_ERROR( wxString::Format(
                    _( "Shared pin '%s' is not connected on any unit." ), shared ) );
        }

        if( symbolPins.size() > 1 )
        {
            THROW_IO_ERROR( wxString::Format(
                    _( "Shared pin '%s' resolves to more than one net." ), shared ) );
        }

        sharedNode[shared] = nodeName( *symbolPins.begin() );
    }

    // Every non-shared base pin must be mapped by at least one unit, else the inner instances
    // would carry a dangling node.
    for( const wxString& basePin : basePinOrder )
    {
        if( sharedSet.count( basePin ) )
            continue;

        bool mapped = false;

        for( const UNIT_INFO& unit : units )
        {
            if( unit.modelToSymbol.count( basePin ) )
                mapped = true;
        }

        if( !mapped )
        {
            THROW_IO_ERROR( wxString::Format(
                    _( "Model '%s' pin '%s' is neither shared nor assigned to any unit." ),
                    aBaseModelName, basePin ) );
        }
    }

    // Instances are the units mapping at least one non-shared pin (a supply-only unit is shared,
    // not an instance).  For each, resolve every base pin to its wrapper node.
    for( const UNIT_INFO& unit : units )
    {
        if( !mapsNonShared( unit ) )
            continue;

        INSTANCE instance;
        instance.unit = unit.unit;

        for( const wxString& basePin : basePinOrder )
        {
            if( sharedSet.count( basePin ) )
            {
                instance.nodes.push_back( sharedNode.at( basePin ) );
            }
            else if( auto it = unit.modelToSymbol.find( basePin ); it != unit.modelToSymbol.end() )
            {
                instance.nodes.push_back( nodeName( it->second ) );
            }
            else
            {
                // Per-instance pin not mapped for this instance: leave it not-connected.
                instance.nodes.push_back( wxString::Format( wxS( "nc_%zu_%s" ), m_instances.size(),
                                                            encodeIdentifier( basePin ) ) );
            }
        }

        m_instances.push_back( std::move( instance ) );
    }

    if( m_instances.empty() )
        THROW_IO_ERROR( _( "Repeat-per-unit decomposition produced no instances." ) );

    // Build the synthetic outer pin list (also the subckt header order): each instance's mapped
    // non-shared pins in base order, then the shared pins in base order.  ItemPins() reads the
    // symbol pin number to emit the outer net; the generator reads the node name for the header.
    std::set<wxString> addedNodes;

    auto addOuterPin =
            [&]( const wxString& aNode, const wxString& aSymbolPin )
            {
                if( addedNodes.insert( aNode ).second )
                    AddPin( { aNode.ToStdString(), aSymbolPin } );
            };

    for( const UNIT_INFO& unit : units )
    {
        if( !mapsNonShared( unit ) )
            continue;

        for( const wxString& basePin : basePinOrder )
        {
            if( sharedSet.count( basePin ) )
                continue;

            if( auto it = unit.modelToSymbol.find( basePin ); it != unit.modelToSymbol.end() )
                addOuterPin( nodeName( it->second ), it->second );
        }
    }

    for( const wxString& basePin : basePinOrder )
    {
        if( !sharedSet.count( basePin ) )
            continue;

        for( const UNIT_INFO& unit : units )
        {
            if( auto it = unit.modelToSymbol.find( basePin ); it != unit.modelToSymbol.end() )
            {
                addOuterPin( sharedNode.at( basePin ), it->second );
                break;
            }
        }
    }

    m_signature = computeSignature();
}


wxString SIM_MODEL_MULTIUNIT::computeSignature() const
{
    std::string canon = m_baseModelName.ToStdString();
    canon += "|";

    for( const SIM_MODEL_PIN& pin : GetPins() )
        canon += pin.modelPinName + ",";

    canon += "|";

    for( const INSTANCE& instance : m_instances )
    {
        for( const wxString& node : instance.nodes )
            canon += node.ToStdString() + ",";

        canon += ";";
    }

    uint64_t hash = stableHash64( canon );

    return wxString::Format( wxS( "kicad_mu_%s_%zuu_%016llx" ), encodeIdentifier( m_baseModelName ),
                             m_instances.size(), static_cast<unsigned long long>( hash ) );
}


const SIM_MODEL_MULTIUNIT& SPICE_GENERATOR_MULTIUNIT::multiunit() const
{
    return static_cast<const SIM_MODEL_MULTIUNIT&>( m_model );
}


std::string SPICE_GENERATOR_MULTIUNIT::ModelName( const SPICE_ITEM& aItem ) const
{
    return multiunit().m_signature.ToStdString();
}


std::string SPICE_GENERATOR_MULTIUNIT::ModelLine( const SPICE_ITEM& aItem ) const
{
    const SIM_MODEL_MULTIUNIT& model = multiunit();

    std::string result = fmt::format( ".subckt {}", model.m_signature.ToStdString() );

    for( const SIM_MODEL_PIN& pin : GetPins() )
        result += " " + pin.modelPinName;

    result += "\n";

    int index = 1;

    for( const SIM_MODEL_MULTIUNIT::INSTANCE& instance : model.m_instances )
    {
        result += fmt::format( "X{}", index++ );

        for( const wxString& node : instance.nodes )
            result += " " + node.ToStdString();

        result += " " + model.m_baseModelName.ToStdString() + "\n";
    }

    result += ".ends\n";

    return result;
}


std::vector<std::string> SPICE_GENERATOR_MULTIUNIT::CurrentNames( const SPICE_ITEM& aItem ) const
{
    std::vector<std::string> currentNames;

    if( GetPins().size() == 2 )
    {
        currentNames.push_back( fmt::format( "I({})", ItemName( aItem ) ) );
    }
    else
    {
        for( const SIM_MODEL_PIN& pin : GetPins() )
            currentNames.push_back( fmt::format( "I({}:{})", ItemName( aItem ), pin.modelPinName ) );
    }

    return currentNames;
}
