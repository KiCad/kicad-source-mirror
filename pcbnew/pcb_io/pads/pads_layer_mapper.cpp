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

#include "pads_layer_mapper.h"

#include <algorithm>
#include <cctype>

#include <wx/string.h>


PADS_LAYER_MAPPER::PADS_LAYER_MAPPER() :
    m_copperLayerCount( 2 )
{
    // Initialize standard PADS layer name mappings (case-insensitive)
    m_layerNameMap["silkscreen top"] = PADS_LAYER_TYPE::SILKSCREEN_TOP;
    m_layerNameMap["silk top"] = PADS_LAYER_TYPE::SILKSCREEN_TOP;
    m_layerNameMap["sst"] = PADS_LAYER_TYPE::SILKSCREEN_TOP;
    m_layerNameMap["top silk"] = PADS_LAYER_TYPE::SILKSCREEN_TOP;
    m_layerNameMap["top overlay"] = PADS_LAYER_TYPE::SILKSCREEN_TOP;

    m_layerNameMap["silkscreen bottom"] = PADS_LAYER_TYPE::SILKSCREEN_BOTTOM;
    m_layerNameMap["silk bottom"] = PADS_LAYER_TYPE::SILKSCREEN_BOTTOM;
    m_layerNameMap["ssb"] = PADS_LAYER_TYPE::SILKSCREEN_BOTTOM;
    m_layerNameMap["bottom silk"] = PADS_LAYER_TYPE::SILKSCREEN_BOTTOM;
    m_layerNameMap["bottom overlay"] = PADS_LAYER_TYPE::SILKSCREEN_BOTTOM;

    m_layerNameMap["solder mask top"] = PADS_LAYER_TYPE::SOLDERMASK_TOP;
    m_layerNameMap["soldermask top"] = PADS_LAYER_TYPE::SOLDERMASK_TOP;
    m_layerNameMap["smt"] = PADS_LAYER_TYPE::SOLDERMASK_TOP;
    m_layerNameMap["top mask"] = PADS_LAYER_TYPE::SOLDERMASK_TOP;
    m_layerNameMap["top solder mask"] = PADS_LAYER_TYPE::SOLDERMASK_TOP;

    m_layerNameMap["solder mask bottom"] = PADS_LAYER_TYPE::SOLDERMASK_BOTTOM;
    m_layerNameMap["soldermask bottom"] = PADS_LAYER_TYPE::SOLDERMASK_BOTTOM;
    m_layerNameMap["smb"] = PADS_LAYER_TYPE::SOLDERMASK_BOTTOM;
    m_layerNameMap["bottom mask"] = PADS_LAYER_TYPE::SOLDERMASK_BOTTOM;
    m_layerNameMap["bottom solder mask"] = PADS_LAYER_TYPE::SOLDERMASK_BOTTOM;

    m_layerNameMap["paste top"] = PADS_LAYER_TYPE::PASTE_TOP;
    m_layerNameMap["paste mask top"] = PADS_LAYER_TYPE::PASTE_TOP;
    m_layerNameMap["solder paste top"] = PADS_LAYER_TYPE::PASTE_TOP;
    m_layerNameMap["top paste"] = PADS_LAYER_TYPE::PASTE_TOP;

    m_layerNameMap["paste bottom"] = PADS_LAYER_TYPE::PASTE_BOTTOM;
    m_layerNameMap["paste mask bottom"] = PADS_LAYER_TYPE::PASTE_BOTTOM;
    m_layerNameMap["solder paste bottom"] = PADS_LAYER_TYPE::PASTE_BOTTOM;
    m_layerNameMap["bottom paste"] = PADS_LAYER_TYPE::PASTE_BOTTOM;

    m_layerNameMap["assembly top"] = PADS_LAYER_TYPE::ASSEMBLY_TOP;
    m_layerNameMap["top assembly"] = PADS_LAYER_TYPE::ASSEMBLY_TOP;
    m_layerNameMap["assy top"] = PADS_LAYER_TYPE::ASSEMBLY_TOP;
    m_layerNameMap["component outline top"] = PADS_LAYER_TYPE::ASSEMBLY_TOP;

    m_layerNameMap["assembly bottom"] = PADS_LAYER_TYPE::ASSEMBLY_BOTTOM;
    m_layerNameMap["bottom assembly"] = PADS_LAYER_TYPE::ASSEMBLY_BOTTOM;
    m_layerNameMap["assy bottom"] = PADS_LAYER_TYPE::ASSEMBLY_BOTTOM;
    m_layerNameMap["component outline bottom"] = PADS_LAYER_TYPE::ASSEMBLY_BOTTOM;

    m_layerNameMap["board outline"] = PADS_LAYER_TYPE::BOARD_OUTLINE;
    m_layerNameMap["board"] = PADS_LAYER_TYPE::BOARD_OUTLINE;
    m_layerNameMap["outline"] = PADS_LAYER_TYPE::BOARD_OUTLINE;
    m_layerNameMap["board geometry"] = PADS_LAYER_TYPE::BOARD_OUTLINE;

    m_layerNameMap["documentation"] = PADS_LAYER_TYPE::DOCUMENTATION;
    m_layerNameMap["doc"] = PADS_LAYER_TYPE::DOCUMENTATION;
}


void PADS_LAYER_MAPPER::SetCopperLayerCount( int aLayerCount )
{
    if( aLayerCount < 1 )
        aLayerCount = 1;

    m_copperLayerCount = aLayerCount;
}


std::string PADS_LAYER_MAPPER::normalizeLayerName( const std::string& aName ) const
{
    std::string normalized;
    normalized.reserve( aName.size() );

    for( char c : aName )
    {
        normalized += static_cast<char>( std::tolower( static_cast<unsigned char>( c ) ) );
    }

    return normalized;
}


PADS_LAYER_TYPE PADS_LAYER_MAPPER::GetLayerType( int aPadsLayer ) const
{
    // Pad stack special values
    if( aPadsLayer == LAYER_PAD_STACK_TOP )
        return PADS_LAYER_TYPE::COPPER_TOP;

    if( aPadsLayer == LAYER_PAD_STACK_BOTTOM )
        return PADS_LAYER_TYPE::COPPER_BOTTOM;

    if( aPadsLayer == LAYER_PAD_STACK_INNER )
        return PADS_LAYER_TYPE::COPPER_INNER;

    // Copper layers: 1 = Top, m_copperLayerCount = Bottom, 2 to N-1 = Inner
    if( aPadsLayer == 1 )
        return PADS_LAYER_TYPE::COPPER_TOP;

    if( aPadsLayer == m_copperLayerCount && m_copperLayerCount > 1 )
        return PADS_LAYER_TYPE::COPPER_BOTTOM;

    if( aPadsLayer > 1 && aPadsLayer < m_copperLayerCount )
        return PADS_LAYER_TYPE::COPPER_INNER;

    // Non-copper layers by number
    if( aPadsLayer == LAYER_SILKSCREEN_TOP )
        return PADS_LAYER_TYPE::SILKSCREEN_TOP;

    if( aPadsLayer == LAYER_SILKSCREEN_BOTTOM )
        return PADS_LAYER_TYPE::SILKSCREEN_BOTTOM;

    if( aPadsLayer == LAYER_SOLDERMASK_TOP )
        return PADS_LAYER_TYPE::SOLDERMASK_TOP;

    if( aPadsLayer == LAYER_SOLDERMASK_BOTTOM )
        return PADS_LAYER_TYPE::SOLDERMASK_BOTTOM;

    if( aPadsLayer == LAYER_PASTE_TOP )
        return PADS_LAYER_TYPE::PASTE_TOP;

    if( aPadsLayer == LAYER_PASTE_BOTTOM )
        return PADS_LAYER_TYPE::PASTE_BOTTOM;

    if( aPadsLayer == LAYER_ASSEMBLY_TOP )
        return PADS_LAYER_TYPE::ASSEMBLY_TOP;

    if( aPadsLayer == LAYER_ASSEMBLY_BOTTOM )
        return PADS_LAYER_TYPE::ASSEMBLY_BOTTOM;

    return PADS_LAYER_TYPE::UNKNOWN;
}


PADS_LAYER_TYPE PADS_LAYER_MAPPER::ParseLayerName( const std::string& aLayerName ) const
{
    std::string normalized = normalizeLayerName( aLayerName );

    auto it = m_layerNameMap.find( normalized );

    if( it != m_layerNameMap.end() )
        return it->second;

    // Check for copper layer naming patterns like "Layer 1", "Inner 1", etc.
    if( normalized.find( "top" ) != std::string::npos ||
        normalized.find( "layer 1" ) != std::string::npos ||
        normalized == "1" )
    {
        return PADS_LAYER_TYPE::COPPER_TOP;
    }

    if( normalized.find( "bottom" ) != std::string::npos ||
        normalized.find( "bot" ) != std::string::npos )
    {
        return PADS_LAYER_TYPE::COPPER_BOTTOM;
    }

    if( normalized.find( "inner" ) != std::string::npos ||
        normalized.find( "mid" ) != std::string::npos ||
        normalized.find( "internal" ) != std::string::npos )
    {
        return PADS_LAYER_TYPE::COPPER_INNER;
    }

    return PADS_LAYER_TYPE::UNKNOWN;
}


PCB_LAYER_ID PADS_LAYER_MAPPER::mapInnerCopperLayer( int aPadsLayer ) const
{
    // PADS inner layers are numbered 2 through (m_copperLayerCount - 1)
    // KiCad inner layers are In1_Cu, In2_Cu, etc. with IDs spaced by 2
    int innerIndex = aPadsLayer - 2;

    if( innerIndex < 0 )
        innerIndex = 0;

    if( innerIndex >= 30 )
        innerIndex = 29;

    return static_cast<PCB_LAYER_ID>( In1_Cu + innerIndex * 2 );
}


PCB_LAYER_ID PADS_LAYER_MAPPER::GetAutoMapLayer( int aPadsLayer, PADS_LAYER_TYPE aType ) const
{
    // If type is not provided, determine it from layer number
    if( aType == PADS_LAYER_TYPE::UNKNOWN )
        aType = GetLayerType( aPadsLayer );

    switch( aType )
    {
    case PADS_LAYER_TYPE::COPPER_TOP:
        return F_Cu;

    case PADS_LAYER_TYPE::COPPER_BOTTOM:
        return B_Cu;

    case PADS_LAYER_TYPE::COPPER_INNER:
        return mapInnerCopperLayer( aPadsLayer );

    case PADS_LAYER_TYPE::SILKSCREEN_TOP:
        return F_SilkS;

    case PADS_LAYER_TYPE::SILKSCREEN_BOTTOM:
        return B_SilkS;

    case PADS_LAYER_TYPE::SOLDERMASK_TOP:
        return F_Mask;

    case PADS_LAYER_TYPE::SOLDERMASK_BOTTOM:
        return B_Mask;

    case PADS_LAYER_TYPE::PASTE_TOP:
        return F_Paste;

    case PADS_LAYER_TYPE::PASTE_BOTTOM:
        return B_Paste;

    case PADS_LAYER_TYPE::ASSEMBLY_TOP:
        return F_Fab;

    case PADS_LAYER_TYPE::ASSEMBLY_BOTTOM:
        return B_Fab;

    case PADS_LAYER_TYPE::BOARD_OUTLINE:
        return Edge_Cuts;

    case PADS_LAYER_TYPE::DOCUMENTATION:
        return Cmts_User;

    case PADS_LAYER_TYPE::DRILL_DRAWING:
        return Dwgs_User;

    case PADS_LAYER_TYPE::UNKNOWN:
    default:
        return UNDEFINED_LAYER;
    }
}


LSET PADS_LAYER_MAPPER::GetPermittedLayers( PADS_LAYER_TYPE aType ) const
{
    switch( aType )
    {
    case PADS_LAYER_TYPE::COPPER_TOP:
    case PADS_LAYER_TYPE::COPPER_BOTTOM:
    case PADS_LAYER_TYPE::COPPER_INNER:
        return LSET::AllCuMask();

    case PADS_LAYER_TYPE::SILKSCREEN_TOP:
    case PADS_LAYER_TYPE::SILKSCREEN_BOTTOM:
        return LSET( { F_SilkS, B_SilkS } );

    case PADS_LAYER_TYPE::SOLDERMASK_TOP:
    case PADS_LAYER_TYPE::SOLDERMASK_BOTTOM:
        return LSET( { F_Mask, B_Mask } );

    case PADS_LAYER_TYPE::PASTE_TOP:
    case PADS_LAYER_TYPE::PASTE_BOTTOM:
        return LSET( { F_Paste, B_Paste } );

    case PADS_LAYER_TYPE::ASSEMBLY_TOP:
    case PADS_LAYER_TYPE::ASSEMBLY_BOTTOM:
        return LSET( { F_Fab, B_Fab, F_CrtYd, B_CrtYd } );

    case PADS_LAYER_TYPE::BOARD_OUTLINE:
        return LSET( { Edge_Cuts } );

    case PADS_LAYER_TYPE::DOCUMENTATION:
        return LSET::AllNonCuMask();

    case PADS_LAYER_TYPE::DRILL_DRAWING:
        return LSET( { Dwgs_User, F_Fab, B_Fab, Cmts_User } );

    case PADS_LAYER_TYPE::UNKNOWN:
    default:
        return LSET::AllLayersMask();
    }
}


std::vector<INPUT_LAYER_DESC> PADS_LAYER_MAPPER::BuildInputLayerDescriptions(
        const std::vector<PADS_LAYER_INFO>& aLayerInfos ) const
{
    std::vector<INPUT_LAYER_DESC> descs;
    descs.reserve( aLayerInfos.size() );

    for( const PADS_LAYER_INFO& info : aLayerInfos )
    {
        INPUT_LAYER_DESC desc;

        desc.Name = wxString::FromUTF8( info.name );
        desc.PermittedLayers = GetPermittedLayers( info.type );
        desc.AutoMapLayer = GetAutoMapLayer( info.padsLayerNum, info.type );
        desc.Required = info.required;

        descs.push_back( desc );
    }

    return descs;
}


void PADS_LAYER_MAPPER::AddLayerNameMapping( const std::string& aName, PADS_LAYER_TYPE aType )
{
    std::string normalized = normalizeLayerName( aName );
    m_layerNameMap[normalized] = aType;
}


std::string PADS_LAYER_MAPPER::LayerTypeToString( PADS_LAYER_TYPE aType )
{
    switch( aType )
    {
    case PADS_LAYER_TYPE::COPPER_TOP:        return "Copper Top";
    case PADS_LAYER_TYPE::COPPER_BOTTOM:     return "Copper Bottom";
    case PADS_LAYER_TYPE::COPPER_INNER:      return "Copper Inner";
    case PADS_LAYER_TYPE::SILKSCREEN_TOP:    return "Silkscreen Top";
    case PADS_LAYER_TYPE::SILKSCREEN_BOTTOM: return "Silkscreen Bottom";
    case PADS_LAYER_TYPE::SOLDERMASK_TOP:    return "Solder Mask Top";
    case PADS_LAYER_TYPE::SOLDERMASK_BOTTOM: return "Solder Mask Bottom";
    case PADS_LAYER_TYPE::PASTE_TOP:         return "Paste Top";
    case PADS_LAYER_TYPE::PASTE_BOTTOM:      return "Paste Bottom";
    case PADS_LAYER_TYPE::ASSEMBLY_TOP:      return "Assembly Top";
    case PADS_LAYER_TYPE::ASSEMBLY_BOTTOM:   return "Assembly Bottom";
    case PADS_LAYER_TYPE::DOCUMENTATION:     return "Documentation";
    case PADS_LAYER_TYPE::BOARD_OUTLINE:     return "Board Outline";
    case PADS_LAYER_TYPE::DRILL_DRAWING:     return "Drill Drawing";
    case PADS_LAYER_TYPE::UNKNOWN:
    default:                                 return "Unknown";
    }
}
