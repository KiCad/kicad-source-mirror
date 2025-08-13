/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2014 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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

#include <algorithm>
#include <bitset>                             // for bitset, __bitset<>::ref...
#include <cassert>
#include <cstdarg>
#include <iostream>                           // for string, endl, basic_ost...
#include <cstddef>                            // for size_t
#include <map>

#include <core/arraydim.h>
#include <layer_ids.h>                        // for PCB_LAYER_ID
#include <layer_range.h>
#include <lseq.h>
#include <macros.h>                           // for arrayDim
#include <wx/debug.h>                         // for wxASSERT, wxASSERT_MSG
#include <wx/string.h>

#include <lset.h>


LSET::LSET( std::initializer_list<PCB_LAYER_ID> aList ) :
    LSET()
{
    for( PCB_LAYER_ID layer : aList )
    {
        if( layer >= 0 )
            set( layer );
    }
}

LSET::LSET( const std::vector<PCB_LAYER_ID>& aList ) :
    LSET()
{
    for( PCB_LAYER_ID layer : aList )
    {
        if( layer >= 0 )
            set( layer );
    }
}


LSET::LSET( const LSEQ& aSeq ) :
    LSET()
{
    for( PCB_LAYER_ID layer : aSeq )
    {
        if( layer >= 0 )
            set( layer );
    }
}


LSET::LSET( const LAYER_RANGE& aRange )
{
    for( PCB_LAYER_ID layer : aRange )
    {
        if( layer >= 0 )
            set( layer );
    }
}


int LSET::LayerCount( PCB_LAYER_ID aStart, PCB_LAYER_ID aEnd, int aCopperLayerCount )
{
    int start = aStart;
    int end = aEnd;

    // Both layers need to be copper
    wxCHECK( IsCopperLayer( aStart ) && IsCopperLayer( aEnd ), aCopperLayerCount );

    if( aStart == B_Cu )
        std::swap( start, end );

    if( aStart == aEnd )
        return 1;

    if( aStart == F_Cu )
    {
        if ( aEnd == B_Cu )
            return aCopperLayerCount;
        else
            return ( end - start ) / 2 - 1;
    }
    else if ( aEnd == B_Cu )
    {
        // Add 1 for the B_Cu layer
        return aCopperLayerCount - start / 2 + 1;
    }

    return ( end - start ) / 2;
}


int LSET::NameToLayer( wxString& aName )
{
    std::map<wxString, PCB_LAYER_ID> layerMap = {
        { "F.Cu", F_Cu },
        { "B.Cu", B_Cu },
        { "F.Adhes", F_Adhes },
        { "B.Adhes", B_Adhes },
        { "F.Paste", F_Paste },
        { "B.Paste", B_Paste },
        { "F.SilkS", F_SilkS },
        { "B.SilkS", B_SilkS },
        { "F.Mask", F_Mask },
        { "B.Mask", B_Mask },
        { "Dwgs.User", Dwgs_User },
        { "Cmts.User", Cmts_User },
        { "Eco1.User", Eco1_User },
        { "Eco2.User", Eco2_User },
        { "Edge.Cuts", Edge_Cuts },
        { "Margin", Margin },
        { "F.CrtYd", F_CrtYd },
        { "B.CrtYd", B_CrtYd },
        { "F.Fab", F_Fab },
        { "B.Fab", B_Fab },
        { "Rescue", Rescue },
        { "B.Cu", B_Cu },
    };

    if( auto it = layerMap.find( aName ); it != layerMap.end() )
        return static_cast<int>( it->second );

    if( aName.StartsWith( "User." ) )
    {
        long offset;

        if( aName.Mid( 5 ).ToLong( &offset ) && offset > 0 )
            return static_cast<int>( User_1 ) + ( offset - 1 ) * 2;
    }

    if( aName.StartsWith( "In" ) )
    {
        long offset;
        wxString str_num = aName.Mid( 2 );
        str_num.RemoveLast( 3 ); // Removes .Cu

        if( str_num.ToLong( &offset ) && offset > 0 )
            return static_cast<int>( In1_Cu ) + ( offset - 1 ) * 2;
    }

    return -1;
}


bool LSET::IsBetween( PCB_LAYER_ID aStart, PCB_LAYER_ID aEnd, PCB_LAYER_ID aLayer )
{
    if( aLayer == aStart || aLayer == aEnd )
        return true;

    int start = std::min( aStart, aEnd );
    int end = std::max( aStart, aEnd );
    int layer = aLayer;

    if( end == B_Cu )
    {
        //Reassign the end layer to the largest possible positive even number
        end = std::numeric_limits<PCB_LAYER_ID>::max() & ~1;
    }

    return !( layer & 1 ) && ( layer >= start ) && ( layer <= end );
}


wxString LSET::Name( PCB_LAYER_ID aLayerId )
{
    wxString txt;

    // using a switch to explicitly show the mapping more clearly
    switch( aLayerId )
    {
    case F_Cu:              txt = wxT( "F.Cu" );            break;
    case B_Cu:              txt = wxT( "B.Cu" );            break;

    // Technicals
    case B_Adhes:           txt = wxT( "B.Adhes" );         break;
    case F_Adhes:           txt = wxT( "F.Adhes" );         break;
    case B_Paste:           txt = wxT( "B.Paste" );         break;
    case F_Paste:           txt = wxT( "F.Paste" );         break;
    case B_SilkS:           txt = wxT( "B.SilkS" );         break;
    case F_SilkS:           txt = wxT( "F.SilkS" );         break;
    case B_Mask:            txt = wxT( "B.Mask" );          break;
    case F_Mask:            txt = wxT( "F.Mask" );          break;

    // Users
    case Dwgs_User:         txt = wxT( "Dwgs.User" );       break;
    case Cmts_User:         txt = wxT( "Cmts.User" );       break;
    case Eco1_User:         txt = wxT( "Eco1.User" );       break;
    case Eco2_User:         txt = wxT( "Eco2.User" );       break;
    case Edge_Cuts:         txt = wxT( "Edge.Cuts" );       break;
    case Margin:            txt = wxT( "Margin" );          break;

    // Footprint
    case F_CrtYd:           txt = wxT( "F.CrtYd" );         break;
    case B_CrtYd:           txt = wxT( "B.CrtYd" );         break;
    case F_Fab:             txt = wxT( "F.Fab" );           break;
    case B_Fab:             txt = wxT( "B.Fab" );           break;

    // Rescue
    case Rescue:            txt = wxT( "Rescue" );          break;

    default:
        if( aLayerId < 0 )
        {
            txt = wxT( "UNDEFINED" );
        }
        else if( static_cast<int>( aLayerId ) & 1 )
        {
            int offset = ( aLayerId - Rescue ) / 2;
            txt = wxString::Format( wxT( "User.%d" ), offset );
        }
        else
        {
            int offset = ( aLayerId - B_Cu ) / 2;
            txt = wxString::Format( wxT( "In%d.Cu" ), offset );
        }
    }

    return txt;
}


LSEQ LSET::CuStack() const
{
    LSEQ ret;

    ret.reserve( 32 );

    for( auto it = copper_layers_begin(); it != copper_layers_end(); ++it )
        ret.push_back( *it );

    return ret;
}


LSEQ LSET::TechAndUserUIOrder() const
{
    LSEQ ret;

    ret.reserve( 32 );

    ret = Seq( {
        F_Adhes,
        B_Adhes,
        F_Paste,
        B_Paste,
        F_SilkS,
        B_SilkS,
        F_Mask,
        B_Mask,
        Dwgs_User,
        Cmts_User,
        Eco1_User,
        Eco2_User,
        Edge_Cuts,
        Margin,
        F_CrtYd,
        B_CrtYd,
        F_Fab,
        B_Fab
    } );

    for( auto it = non_copper_layers_begin(); it != non_copper_layers_end(); ++it )
    {
        if( *it >= User_1 )
            ret.push_back( *it );
    }

    return ret;
}


LSEQ LSET::Seq( const LSEQ& aSequence ) const
{
    LSEQ ret;

    for( PCB_LAYER_ID layer : aSequence )
    {
        if( test( layer ) )
            ret.push_back( layer );
    }

    return ret;
}


LSEQ LSET::Seq() const
{
    LSEQ    ret;

    ret.reserve( size() );

    for( unsigned i = 0; i < size(); ++i )
    {
        if( test( i ) )
            ret.push_back( PCB_LAYER_ID( i ) );
    }

    return ret;
}


LSEQ LSET::SeqStackupTop2Bottom( PCB_LAYER_ID aSelectedLayer ) const
{
    LSEQ base_sequence = Seq( {
        Edge_Cuts,
        Margin,
        Dwgs_User,
        Cmts_User,
        Eco1_User,
        Eco2_User
    } );

    LSEQ top_tech_sequence = Seq( {
        F_Fab,
        F_SilkS,
        F_Paste,
        F_Adhes,
        F_Mask,
        F_CrtYd,
    } );

    LSEQ bottom_tech_sequence = Seq( {
        B_CrtYd,
        B_Mask,
        B_Adhes,
        B_Paste,
        B_SilkS,
        B_Fab,
    } );


    LSEQ seq = Seq( base_sequence );

    for( auto it = non_copper_layers_begin(); it != non_copper_layers_end(); ++it )
    {
        if( *it >= User_1 )
            seq.push_back( *it );
    }

    std::copy( top_tech_sequence.begin(), top_tech_sequence.end(), std::back_inserter( seq ) );

    for( auto it = copper_layers_begin(); it != copper_layers_end(); ++it )
        seq.push_back( *it );

    std::copy( bottom_tech_sequence.begin(), bottom_tech_sequence.end(),
               std::back_inserter( seq ) );

    if( aSelectedLayer != UNDEFINED_LAYER )
    {
        auto it = std::find( seq.begin(), seq.end(), aSelectedLayer );

        if( it != seq.end() )
        {
            seq.erase( it );
            seq.insert( seq.begin(), aSelectedLayer );
        }
    }

    return seq;
}


LSEQ LSET::SeqStackupForPlotting() const
{
    // bottom-to-top stack-up layers
    // Note that the bottom technical layers are flipped so that when plotting a bottom-side view,
    // they appear in the correct sequence.
    LSEQ bottom_tech_sequence = Seq( {
        B_Cu,
        B_Mask,
        B_Paste,
        B_SilkS,
        B_Adhes,
        B_CrtYd,
        B_Fab,
    } );

    // Copper layers go here

    LSEQ top_tech_sequence = Seq( {
        F_Mask,
        F_Paste,
        F_SilkS,
        F_Adhes,
        F_CrtYd,
        F_Fab,
    } );

    LSEQ user_sequence = Seq( {
        Dwgs_User,
        Cmts_User,
        Eco1_User,
        Eco2_User,
    } );

    // User layers go here

    LSEQ base_sequence = Seq( {
        Margin,
        Edge_Cuts,
    } );



    LSEQ seq = Seq( bottom_tech_sequence );

    std::vector<PCB_LAYER_ID> temp_layers;

    // We are going to reverse the copper layers and then add them to the sequence
    // because the plotting order is bottom-to-top
    for( auto it = copper_layers_begin(); it != copper_layers_end(); ++it )
    {
        // Skip B_Cu because it is already in the sequence (if it exists)
        if( *it != B_Cu )
            temp_layers.push_back( *it );
    }

    for( auto it = temp_layers.rbegin(); it != temp_layers.rend(); ++it )
        seq.push_back( *it );

    std::copy( top_tech_sequence.begin(), top_tech_sequence.end(), std::back_inserter( seq ) );

    std::copy( user_sequence.begin(), user_sequence.end(), std::back_inserter( seq ) );

    temp_layers.clear();

    for( auto it = non_copper_layers_begin(); it != non_copper_layers_end(); ++it )
    {
        if( *it >= User_1 )
            temp_layers.push_back( *it );
    }

    for( auto it = temp_layers.rbegin(); it != temp_layers.rend(); ++it )
    {
        seq.push_back( *it );
    }

    std::copy( base_sequence.begin(), base_sequence.end(), std::back_inserter( seq ) );

    return seq;
}


LSET& LSET::FlipStandardLayers( int aCopperLayersCount )
{
    LSET oldMask = *this;

    reset();

    // Mapping for Copper and Non-Copper layers
    const std::map<PCB_LAYER_ID, PCB_LAYER_ID> flip_map =
    {
        {F_Cu, B_Cu},
        {B_Cu, F_Cu},
        {F_SilkS, B_SilkS},
        {B_SilkS, F_SilkS},
        {F_Adhes, B_Adhes},
        {B_Adhes, F_Adhes},
        {F_Mask, B_Mask},
        {B_Mask, F_Mask},
        {F_Paste, B_Paste},
        {B_Paste, F_Paste},
        {F_CrtYd, B_CrtYd},
        {B_CrtYd, F_CrtYd},
        {F_Fab, B_Fab},
        {B_Fab, F_Fab}
    };

    for( const auto& pair : flip_map )
    {
        if( oldMask.test( pair.first ) )
            set( pair.second );

        oldMask.set( pair.first, false );
    }

    if( aCopperLayersCount >= 4 )
    {
        LSET internalMask = oldMask & InternalCuMask();
        int  innerLayerCount = aCopperLayersCount - 2;

        for( int ii = 1; ii <= innerLayerCount; ii++ )
        {
            if( internalMask.test( ( innerLayerCount - ii + 1 ) * 2 + B_Cu ) )
            {
                set( ii * 2 + B_Cu );
            }
        }
    }

    oldMask.ClearCopperLayers();

    // Copy across any remaining, non-side-specific layers
    for( PCB_LAYER_ID layer : oldMask )
        set( layer );

    return *this;
}


PCB_LAYER_ID LSET::ExtractLayer() const
{
    unsigned set_count = count();

    if( !set_count )
        return UNSELECTED_LAYER;
    else if( set_count > 1 )
        return UNDEFINED_LAYER;

    for( unsigned i=0; i < size(); ++i )
    {
        if( test( i ) )
            return PCB_LAYER_ID( i );
    }

    wxASSERT( 0 );  // set_count was verified as 1 above, what did you break?

    return UNDEFINED_LAYER;
}


const LSET& LSET::FrontAssembly()
{
    static LSET saved( { F_SilkS, F_Mask, F_Fab, F_CrtYd } );
    return saved;
}


const LSET& LSET::BackAssembly()
{
    static LSET saved( { B_SilkS, B_Mask, B_Fab, B_CrtYd } );
    return saved;
}


const LSET& LSET::InternalCuMask()
{
    static LSET saved( { In1_Cu,  In2_Cu,  In3_Cu,  In4_Cu,  In5_Cu,  In6_Cu,
                         In7_Cu,  In8_Cu,  In9_Cu,  In10_Cu, In11_Cu, In12_Cu,
                         In13_Cu, In14_Cu, In15_Cu, In16_Cu, In17_Cu, In18_Cu,
                         In19_Cu, In20_Cu, In21_Cu, In22_Cu, In23_Cu, In24_Cu,
                         In25_Cu, In26_Cu, In27_Cu, In28_Cu, In29_Cu, In30_Cu } );
    return saved;
}


LSET allCuMask( int aCuLayerCount )
{
    LSET ret;

    for( PCB_LAYER_ID layer : LAYER_RANGE( F_Cu, B_Cu, aCuLayerCount ) )
        ret.set( layer );

    return ret;
}


LSET LSET::AllCuMask( int aCuLayerCount )
{
    if( aCuLayerCount == MAX_CU_LAYERS )
        return AllCuMask();

    return allCuMask( aCuLayerCount );
}


LSET LSET::AllCuMask()
{
    static LSET s_savedMax = allCuMask( MAX_CU_LAYERS );

    return s_savedMax;
}


LSET allNonCuMask()
{
    LSET mask = LSET().set();

    for( auto it = mask.copper_layers_begin(); it != mask.copper_layers_end(); ++it )
        mask.reset( *it );

    return mask;
}


LSET LSET::AllNonCuMask()
{
    static LSET saved = allNonCuMask();
    return saved;
}


const LSET& LSET::ExternalCuMask()
{
    static LSET saved( { F_Cu, B_Cu } );
    return saved;
}


const LSET& LSET::AllLayersMask()
{
    static LSET saved = LSET().set();
    return saved;
}


const LSET& LSET::BackTechMask()
{
    static LSET saved( { B_SilkS, B_Mask, B_Adhes, B_Paste, B_CrtYd, B_Fab } );
    return saved;
}


const LSET& LSET::BackBoardTechMask()
{
    static LSET saved( { B_SilkS, B_Mask, B_Adhes, B_Paste } );
    return saved;
}


const LSET& LSET::FrontTechMask()
{
    static LSET saved( { F_SilkS, F_Mask, F_Adhes, F_Paste, F_CrtYd, F_Fab } );
    return saved;
}


const LSET& LSET::FrontBoardTechMask()
{
    static LSET saved( { F_SilkS, F_Mask, F_Adhes, F_Paste } );
    return saved;
}


const LSET& LSET::AllTechMask()
{
    static LSET saved = BackTechMask() | FrontTechMask();
    return saved;
}


const LSET& LSET::AllBoardTechMask()
{
    static LSET saved = BackBoardTechMask() | FrontBoardTechMask();
    return saved;
}


const LSET& LSET::UserMask()
{
    static LSET saved( { Dwgs_User, Cmts_User, Eco1_User, Eco2_User, Edge_Cuts, Margin } );
    return saved;
}


const LSET& LSET::PhysicalLayersMask()
{
    static LSET saved = AllBoardTechMask() | AllCuMask();
    return saved;
}


LSET LSET::UserDefinedLayersMask( int aUserDefinedLayerCount )
{
    LSET   ret;
    size_t layer = User_1;

    for( int ulayer = 1; ulayer <= aUserDefinedLayerCount; ulayer++ )
    {
        if( layer > ret.size() )
            break;

        ret.set( layer );
        layer += 2;
    }

    return ret;
}


const LSET& LSET::FrontMask()
{
    static LSET saved = LSET( FrontTechMask() ).set( F_Cu );
    return saved;
}


const LSET& LSET::BackMask()
{
    static LSET saved = LSET( BackTechMask() ).set( B_Cu );
    return saved;
}


const LSET& LSET::SideSpecificMask()
{
    static LSET saved = BackTechMask() | FrontTechMask() | AllCuMask();
    return saved;
}


LSEQ LSET::UIOrder() const
{
    LSEQ order = CuStack();
    LSEQ techuser = TechAndUserUIOrder();

    order.insert( order.end(), techuser.begin(), techuser.end() );

    return order;
}


PCB_LAYER_ID ToLAYER_ID( int aLayer )
{
    // We use std::numeric_limits<int>::max() to represent B_Cu for the connectivity_rtree
    if( aLayer == std::numeric_limits<int>::max() )
        return B_Cu;

    wxASSERT( aLayer < GAL_LAYER_ID_END );
    return PCB_LAYER_ID( aLayer );
}


GAL_SET::GAL_SET( const GAL_LAYER_ID* aArray, unsigned aCount ) : GAL_SET()
{
    for( unsigned i = 0; i < aCount; ++i )
        set( aArray[i] );
}


std::vector<GAL_LAYER_ID> GAL_SET::Seq() const
{
    std::vector<GAL_LAYER_ID> ret;

    for( size_t i = 0; i < size(); ++i )
    {
        if( test( i ) )
            ret.push_back( static_cast<GAL_LAYER_ID>( i + GAL_LAYER_ID_START ) );
    }

    return ret;
}


GAL_SET GAL_SET::DefaultVisible()
{
    static const GAL_LAYER_ID visible[] = {
        LAYER_VIAS,
        LAYER_VIA_MICROVIA,
        LAYER_VIA_BLIND,
        LAYER_VIA_BURIED,
        LAYER_VIA_THROUGH,
        // LAYER_HIDDEN_TEXT,    // DEPCREATED SINCE 9.0. Invisible text hidden by default
        LAYER_ANCHOR,
        LAYER_RATSNEST,
        LAYER_GRID,
        LAYER_GRID_AXES,
        LAYER_FOOTPRINTS_FR,
        LAYER_FOOTPRINTS_BK,
        LAYER_FP_TEXT,
        LAYER_FP_VALUES,
        LAYER_FP_REFERENCES,
        LAYER_TRACKS,
        LAYER_PAD_PLATEDHOLES,
        LAYER_NON_PLATEDHOLES,
        LAYER_PAD_HOLEWALLS,
        LAYER_VIA_HOLES,
        LAYER_VIA_HOLEWALLS,
        LAYER_DRC_ERROR,
        LAYER_DRC_WARNING,
        LAYER_DRC_SHAPES,
        // LAYER_DRC_EXCLUSION,      // DRC exclusions hidden by default
        LAYER_DRAWINGSHEET,
        LAYER_GP_OVERLAY,
        LAYER_SELECT_OVERLAY,
        LAYER_PCB_BACKGROUND,
        LAYER_CURSOR,
        LAYER_AUX_ITEMS,
        LAYER_DRAW_BITMAPS,
        LAYER_PADS,
        LAYER_ZONES,
        LAYER_FILLED_SHAPES,
        LAYER_LOCKED_ITEM_SHADOW,
        // LAYER_BOARD_OUTLINE_AREA,    // currently hidden by default
        LAYER_CONFLICTS_SHADOW,
        LAYER_POINTS
    };

    static const GAL_SET saved( visible, arrayDim( visible ) );
    return saved;
}


#ifndef SWIG // Skip SWIG generators for the iterators because it requires a default constructor
// Custom iterators for Copper and Non-Copper layers

LSET::copper_layers_iterator::copper_layers_iterator( const BASE_SET& set, size_t index ) :
        BASE_SET::set_bits_iterator( set, index )
{
    m_index = ( index + 1 ) & ~1;
    advance_to_next_set_copper_bit();
}


PCB_LAYER_ID LSET::copper_layers_iterator::operator*() const
{
    return static_cast<PCB_LAYER_ID>( m_index );
}


LSET::copper_layers_iterator& LSET::copper_layers_iterator::operator++()
{
    next_copper_layer();
    advance_to_next_set_copper_bit();
    return *this;
}


void LSET::copper_layers_iterator::next_copper_layer()
{
    if( m_index == F_Cu )
    {
        m_index += 4;
    }
    else if( m_index == B_Cu )
    {
        m_index = m_baseSet.size();
        return;
    }
    else
    {
        m_index += 2;

        if( m_index >= m_baseSet.size() )
            m_index = B_Cu;
    }
}


void LSET::copper_layers_iterator::advance_to_next_set_copper_bit()
{
    while( m_index < m_baseSet.size() && !m_baseSet.test( m_index ) )
        next_copper_layer();
}


LSET::non_copper_layers_iterator::non_copper_layers_iterator( const BASE_SET& set, size_t index ) :
        BASE_SET::set_bits_iterator( set, index )
{
    advance_to_next_set_non_copper_bit();
}


PCB_LAYER_ID LSET::non_copper_layers_iterator::operator*() const
{
    return static_cast<PCB_LAYER_ID>( m_index );
}


LSET::non_copper_layers_iterator& LSET::non_copper_layers_iterator::operator++()
{
    ++m_index;
    advance_to_next_set_non_copper_bit();
    return *this;
}


void LSET::non_copper_layers_iterator::advance_to_next_set_non_copper_bit()
{
    while( m_index < m_baseSet.size() && ( m_index % 2 != 1 || !m_baseSet.test( m_index ) ) )
    {
        ++m_index;
    }
}


LSET::copper_layers_iterator LSET::copper_layers_begin() const
{
    return copper_layers_iterator( *this, 0 );
}


LSET::copper_layers_iterator LSET::copper_layers_end() const
{
    return copper_layers_iterator( *this, size() );
}


LSET::non_copper_layers_iterator LSET::non_copper_layers_begin() const
{
    return non_copper_layers_iterator( *this, 0 );
}


LSET::non_copper_layers_iterator LSET::non_copper_layers_end() const
{
    return non_copper_layers_iterator( *this, size() );
}


LSET& LSET::ClearCopperLayers()
{
    for( size_t ii = 0; ii < size(); ii += 2 )
        reset( ii );

    return *this;
}


LSET& LSET::ClearNonCopperLayers()
{
    for( size_t ii = 1; ii < size(); ii += 2 )
        reset( ii );

    return *this;
}


LSET& LSET::ClearUserDefinedLayers()
{
    for( size_t ii = User_1; ii < size(); ii += 2 )
        reset( ii );

    return *this;
}

#endif
