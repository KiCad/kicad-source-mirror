/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2014 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2014-2023 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <bitset>                             // for bitset, __bitset<>::ref...
#include <cassert>
#include <cstdarg>
#include <iostream>                           // for string, endl, basic_ost...
#include <stddef.h>                           // for size_t

#include <core/arraydim.h>
#include <math/util.h>                        // for Clamp
#include <layer_ids.h>                        // for LSET, PCB_LAYER_ID, LSEQ
#include <macros.h>                           // for arrayDim
#include <wx/debug.h>                         // for wxASSERT, wxASSERT_MSG
#include <wx/string.h>


LSET::LSET( const PCB_LAYER_ID* aArray, unsigned aCount ) :
    BASE_SET()
{
    for( unsigned i=0; i<aCount; ++i )
        set( aArray[i] );
}


LSET::LSET( unsigned aIdCount, int aFirst, ... ) :
    BASE_SET()
{
    // The constructor, without the mandatory aFirst argument, could have been confused
    // by the compiler with the LSET( PCB_LAYER_ID ).  With aFirst, that ambiguity is not
    // present.  Therefore aIdCount must always be >=1.
    wxASSERT_MSG( aIdCount > 0, wxT( "aIdCount must be >= 1" ) );

    set( aFirst );

    if( --aIdCount )
    {
        va_list ap;

        va_start( ap, aFirst );

        for( unsigned i=0;  i<aIdCount;  ++i )
        {
            PCB_LAYER_ID id = (PCB_LAYER_ID) va_arg( ap, int );

            assert( unsigned( id ) < PCB_LAYER_ID_COUNT );

            set( id );
        }

        va_end( ap );
    }
}


LSET::LSET( const LSEQ& aSeq )
{
    for( PCB_LAYER_ID layer : aSeq )
        set( layer );
}


/**
 * NOTE: These names must not be translated or changed.  They are used as tokens in the board
 * file format because the ordinal value of the PCB_LAYER_ID enum was not stable over time.
 * @see LayerName() for what should be used to display the default name of a layer in the GUI.
 */
const wxChar* LSET::Name( PCB_LAYER_ID aLayerId )
{
    const wxChar* txt;

    // using a switch to explicitly show the mapping more clearly
    switch( aLayerId )
    {
    case F_Cu:              txt = wxT( "F.Cu" );            break;
    case In1_Cu:            txt = wxT( "In1.Cu" );          break;
    case In2_Cu:            txt = wxT( "In2.Cu" );          break;
    case In3_Cu:            txt = wxT( "In3.Cu" );          break;
    case In4_Cu:            txt = wxT( "In4.Cu" );          break;
    case In5_Cu:            txt = wxT( "In5.Cu" );          break;
    case In6_Cu:            txt = wxT( "In6.Cu" );          break;
    case In7_Cu:            txt = wxT( "In7.Cu" );          break;
    case In8_Cu:            txt = wxT( "In8.Cu" );          break;
    case In9_Cu:            txt = wxT( "In9.Cu" );          break;
    case In10_Cu:           txt = wxT( "In10.Cu" );         break;
    case In11_Cu:           txt = wxT( "In11.Cu" );         break;
    case In12_Cu:           txt = wxT( "In12.Cu" );         break;
    case In13_Cu:           txt = wxT( "In13.Cu" );         break;
    case In14_Cu:           txt = wxT( "In14.Cu" );         break;
    case In15_Cu:           txt = wxT( "In15.Cu" );         break;
    case In16_Cu:           txt = wxT( "In16.Cu" );         break;
    case In17_Cu:           txt = wxT( "In17.Cu" );         break;
    case In18_Cu:           txt = wxT( "In18.Cu" );         break;
    case In19_Cu:           txt = wxT( "In19.Cu" );         break;
    case In20_Cu:           txt = wxT( "In20.Cu" );         break;
    case In21_Cu:           txt = wxT( "In21.Cu" );         break;
    case In22_Cu:           txt = wxT( "In22.Cu" );         break;
    case In23_Cu:           txt = wxT( "In23.Cu" );         break;
    case In24_Cu:           txt = wxT( "In24.Cu" );         break;
    case In25_Cu:           txt = wxT( "In25.Cu" );         break;
    case In26_Cu:           txt = wxT( "In26.Cu" );         break;
    case In27_Cu:           txt = wxT( "In27.Cu" );         break;
    case In28_Cu:           txt = wxT( "In28.Cu" );         break;
    case In29_Cu:           txt = wxT( "In29.Cu" );         break;
    case In30_Cu:           txt = wxT( "In30.Cu" );         break;
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

    // User definable layers.
    case User_1:            txt = wxT( "User.1" );          break;
    case User_2:            txt = wxT( "User.2" );          break;
    case User_3:            txt = wxT( "User.3" );          break;
    case User_4:            txt = wxT( "User.4" );          break;
    case User_5:            txt = wxT( "User.5" );          break;
    case User_6:            txt = wxT( "User.6" );          break;
    case User_7:            txt = wxT( "User.7" );          break;
    case User_8:            txt = wxT( "User.8" );          break;
    case User_9:            txt = wxT( "User.9" );          break;

    // Rescue
    case Rescue:            txt = wxT( "Rescue" );          break;

    default:
        std::cout << aLayerId << std::endl;
        wxASSERT_MSG( 0, wxT( "aLayerId out of range" ) );
                            txt = wxT( "BAD INDEX!" );      break;
    }

    return txt;
}


LSEQ LSET::CuStack() const
{
    // desired sequence
    static const PCB_LAYER_ID sequence[] = {
        F_Cu,
        In1_Cu,
        In2_Cu,
        In3_Cu,
        In4_Cu,
        In5_Cu,
        In6_Cu,
        In7_Cu,
        In8_Cu,
        In9_Cu,
        In10_Cu,
        In11_Cu,
        In12_Cu,
        In13_Cu,
        In14_Cu,
        In15_Cu,
        In16_Cu,
        In17_Cu,
        In18_Cu,
        In19_Cu,
        In20_Cu,
        In21_Cu,
        In22_Cu,
        In23_Cu,
        In24_Cu,
        In25_Cu,
        In26_Cu,
        In27_Cu,
        In28_Cu,
        In29_Cu,
        In30_Cu,
        B_Cu,           // 31
    };

    return Seq( sequence, arrayDim( sequence ) );
}


LSEQ LSET::Technicals( LSET aSetToOmit ) const
{
    // desired sequence
    static const PCB_LAYER_ID sequence[] = {
        F_Adhes,
        B_Adhes,
        F_Paste,
        B_Paste,
        F_SilkS,
        B_SilkS,
        F_Mask,
        B_Mask,
        F_CrtYd,
        B_CrtYd,
        F_Fab,
        B_Fab,
    };

    LSET subset = ~aSetToOmit & *this;

    return subset.Seq( sequence, arrayDim( sequence ) );
}


LSEQ LSET::Users() const
{
    // desired
    static const PCB_LAYER_ID sequence[] = {
        Dwgs_User,
        Cmts_User,
        Eco1_User,
        Eco2_User,
        Edge_Cuts,
        Margin,
        User_1,
        User_2,
        User_3,
        User_4,
        User_5,
        User_6,
        User_7,
        User_8,
        User_9
   };

   return Seq( sequence, arrayDim( sequence ) );
}


LSEQ LSET::TechAndUserUIOrder() const
{
    static const PCB_LAYER_ID sequence[] = {
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
        B_Fab,
        User_1,
        User_2,
        User_3,
        User_4,
        User_5,
        User_6,
        User_7,
        User_8,
        User_9
   };

   return Seq( sequence, arrayDim( sequence ) );
}


std::string LSET::FmtBin() const
{
    std::string ret;

    int     bit_count = size();

    for( int bit=0;  bit<bit_count;  ++bit )
    {
        if( bit )
        {
            if( !( bit % 8 ) )
                ret += '|';
            else if( !( bit % 4 ) )
                ret += '_';
        }

        ret += (*this)[bit] ? '1' :  '0';
    }

    // reverse of string
    return std::string( ret.rbegin(), ret.rend() );
}


std::string LSET::FmtHex() const
{
    std::string ret;

    static const char hex[] = "0123456789abcdef";

    size_t nibble_count = ( size() + 3 ) / 4;

    for( size_t nibble = 0; nibble < nibble_count; ++nibble )
    {
        unsigned int ndx = 0;

        // test 4 consecutive bits and set ndx to 0-15
        for( size_t nibble_bit = 0; nibble_bit < 4; ++nibble_bit )
        {
            size_t nibble_pos = nibble_bit + ( nibble * 4 );
            // make sure it's not extra bits that don't exist in the bitset but need to in the
            // hex format
            if( nibble_pos >= size() )
                break;

            if( ( *this )[nibble_pos] )
                ndx |= ( 1 << nibble_bit );
        }

        if( nibble && !( nibble % 8 ) )
            ret += '_';

        assert( ndx < arrayDim( hex ) );

        ret += hex[ndx];
    }

    // reverse of string
    return std::string( ret.rbegin(), ret.rend() );
}


int LSET::ParseHex( const char* aStart, int aCount )
{
    LSET tmp;

    const char* rstart = aStart + aCount - 1;
    const char* rend   = aStart - 1;

    const int bitcount = size();

    int nibble_ndx = 0;

    while( rstart > rend )
    {
        int cc = *rstart--;

        if( cc == '_' )
            continue;

        int nibble;

        if( cc >= '0' && cc <= '9' )
            nibble = cc - '0';
        else if( cc >= 'a' && cc <= 'f' )
            nibble = cc - 'a' + 10;
        else if( cc >= 'A' && cc <= 'F' )
            nibble = cc - 'A' + 10;
        else
            break;

        int bit = nibble_ndx * 4;

        for( int ndx=0; bit<bitcount && ndx<4; ++bit, ++ndx )
            if( nibble & (1<<ndx) )
                tmp.set( bit );

        if( bit >= bitcount )
            break;

        ++nibble_ndx;
    }

    int byte_count = aStart + aCount - 1 - rstart;

    assert( byte_count >= 0 );

    if( byte_count > 0 )
        *this = tmp;

    return byte_count;
}


LSEQ LSET::Seq( const PCB_LAYER_ID* aWishListSequence, unsigned aCount ) const
{
    LSEQ ret;

#if defined(DEBUG) && 0
    LSET    dup_detector;

    for( unsigned i=0; i<aCount;  ++i )
    {
        PCB_LAYER_ID id = aWishListSequence[i];

        if( test( id ) )
        {
            wxASSERT_MSG( !dup_detector[id], wxT( "Duplicate in aWishListSequence" ) );
            dup_detector[id] = true;

            ret.push_back( id );
        }
    }
#else

    for( unsigned i=0; i<aCount;  ++i )
    {
        PCB_LAYER_ID id = aWishListSequence[i];

        if( test( id ) )
            ret.push_back( id );
    }
#endif

    return ret;
}


LSEQ LSET::Seq( const LSEQ& aSequence ) const
{
    LSEQ ret;

    for( LSEQ seq = aSequence; seq; ++seq )
    {
        if( test( *seq ) )
            ret.push_back( *seq );
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
    static const PCB_LAYER_ID sequence[] = {
        Edge_Cuts,
        Margin,
        Dwgs_User,
        Cmts_User,
        Eco1_User,
        Eco2_User,
        User_1,
        User_2,
        User_3,
        User_4,
        User_5,
        User_6,
        User_7,
        User_8,
        User_9,
        F_Fab,
        F_SilkS,
        F_Paste,
        F_Adhes,
        F_Mask,
        F_CrtYd,
        F_Cu,
        In1_Cu,
        In2_Cu,
        In3_Cu,
        In4_Cu,
        In5_Cu,
        In6_Cu,
        In7_Cu,
        In8_Cu,
        In9_Cu,
        In10_Cu,
        In11_Cu,
        In12_Cu,
        In13_Cu,
        In14_Cu,
        In15_Cu,
        In16_Cu,
        In17_Cu,
        In18_Cu,
        In19_Cu,
        In20_Cu,
        In21_Cu,
        In22_Cu,
        In23_Cu,
        In24_Cu,
        In25_Cu,
        In26_Cu,
        In27_Cu,
        In28_Cu,
        In29_Cu,
        In30_Cu,
        B_Cu,
        B_CrtYd,
        B_Mask,
        B_Adhes,
        B_Paste,
        B_SilkS,
        B_Fab,
    };

    LSEQ seq = Seq( sequence, arrayDim( sequence ) );

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
    static const PCB_LAYER_ID sequence[] = {
        B_Cu,
        B_Mask,
        B_Paste,
        B_SilkS,
        B_Adhes,
        B_CrtYd,
        B_Fab,
        In30_Cu,
        In29_Cu,
        In28_Cu,
        In27_Cu,
        In26_Cu,
        In25_Cu,
        In24_Cu,
        In23_Cu,
        In22_Cu,
        In21_Cu,
        In20_Cu,
        In19_Cu,
        In18_Cu,
        In17_Cu,
        In16_Cu,
        In15_Cu,
        In14_Cu,
        In13_Cu,
        In12_Cu,
        In11_Cu,
        In10_Cu,
        In9_Cu,
        In8_Cu,
        In7_Cu,
        In6_Cu,
        In5_Cu,
        In4_Cu,
        In3_Cu,
        In2_Cu,
        In1_Cu,
        F_Cu,
        F_Mask,
        F_Paste,
        F_SilkS,
        F_Adhes,
        F_CrtYd,
        F_Fab,
        Dwgs_User,
        Cmts_User,
        Eco1_User,
        Eco2_User,
        User_1,
        User_2,
        User_3,
        User_4,
        User_5,
        User_6,
        User_7,
        User_8,
        User_9,
        Margin,
        Edge_Cuts,
    };

    return Seq( sequence, arrayDim( sequence ) );
}


PCB_LAYER_ID FlipLayer( PCB_LAYER_ID aLayerId, int aCopperLayersCount )
{
    switch( aLayerId )
    {
    case B_Cu:              return F_Cu;
    case F_Cu:              return B_Cu;

    case B_SilkS:           return F_SilkS;
    case F_SilkS:           return B_SilkS;

    case B_Adhes:           return F_Adhes;
    case F_Adhes:           return B_Adhes;

    case B_Mask:            return F_Mask;
    case F_Mask:            return B_Mask;

    case B_Paste:           return F_Paste;
    case F_Paste:           return B_Paste;

    case B_CrtYd:           return F_CrtYd;
    case F_CrtYd:           return B_CrtYd;

    case B_Fab:             return F_Fab;
    case F_Fab:             return B_Fab;

    default:    // change internal layer if aCopperLayersCount is >= 4
        if( IsCopperLayer( aLayerId ) && aCopperLayersCount >= 4 )
        {
            // internal copper layers count is aCopperLayersCount-2
            PCB_LAYER_ID fliplayer = PCB_LAYER_ID(aCopperLayersCount - 2 - ( aLayerId - In1_Cu ) );
            // Ensure fliplayer has a value which does not crash Pcbnew:
            if( fliplayer < F_Cu )
                fliplayer = F_Cu;

            if( fliplayer > B_Cu )
                fliplayer = B_Cu;

            return fliplayer;
        }

        // No change for the other layers
        return aLayerId;
    }
}


LSET FlipLayerMask( LSET aMask, int aCopperLayersCount )
{
    // layers on physical outside of a board:
    const static LSET and_mask( 16,     // !! update count
                B_Cu,       F_Cu,
                B_SilkS,    F_SilkS,
                B_Adhes,    F_Adhes,
                B_Mask,     F_Mask,
                B_Paste,    F_Paste,
                B_Adhes,    F_Adhes,
                B_CrtYd,    F_CrtYd,
                B_Fab,      F_Fab
                );

    LSET newMask = aMask & ~and_mask;

    if( aMask[B_Cu] )
        newMask.set( F_Cu );

    if( aMask[F_Cu] )
        newMask.set( B_Cu );

    if( aMask[B_SilkS] )
        newMask.set( F_SilkS );

    if( aMask[F_SilkS] )
        newMask.set( B_SilkS );

    if( aMask[B_Adhes] )
        newMask.set( F_Adhes );

    if( aMask[F_Adhes] )
        newMask.set( B_Adhes );

    if( aMask[B_Mask] )
        newMask.set( F_Mask );

    if( aMask[F_Mask] )
        newMask.set( B_Mask );

    if( aMask[B_Paste] )
        newMask.set( F_Paste );

    if( aMask[F_Paste] )
        newMask.set( B_Paste );

    if( aMask[B_Adhes] )
        newMask.set( F_Adhes );

    if( aMask[F_Adhes] )
        newMask.set( B_Adhes );

    if( aMask[B_CrtYd] )
        newMask.set( F_CrtYd );

    if( aMask[F_CrtYd] )
        newMask.set( B_CrtYd );

    if( aMask[B_Fab] )
        newMask.set( F_Fab );

    if( aMask[F_Fab] )
        newMask.set( B_Fab );

    if( aCopperLayersCount >= 4 )   // Internal layers exist
    {
        LSET internalMask = aMask & LSET::InternalCuMask();

        if( internalMask != LSET::InternalCuMask() )
        {
            // the mask does not include all internal layers. Therefore
            // the flipped mask for internal copper layers must be built
            int innerLayerCnt = aCopperLayersCount -2;

            // the flipped mask is the innerLayerCnt bits rewritten in reverse order
            // ( bits innerLayerCnt to 1 rewritten in bits 1 to innerLayerCnt )
            for( int ii = 0; ii < innerLayerCnt; ii++ )
            {
                if( internalMask[innerLayerCnt - ii] )
                {
                    newMask.set( ii + In1_Cu );
                }
                else
                {
                    newMask.reset( ii + In1_Cu );
                }
            }
        }
    }

    return newMask;
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


LSET LSET::FrontAssembly()
{
    static const PCB_LAYER_ID front_assembly[] = {
        F_SilkS,
        F_Mask,
        F_Fab,
        F_CrtYd
    };

    static const LSET saved( front_assembly, arrayDim( front_assembly ) );
    return saved;
}


LSET LSET::BackAssembly()
{
    static const PCB_LAYER_ID back_assembly[] = {
        B_SilkS,
        B_Mask,
        B_Fab,
        B_CrtYd
    };

    static const LSET saved( back_assembly, arrayDim( back_assembly ) );
    return saved;
}


LSET LSET::InternalCuMask()
{
    static const PCB_LAYER_ID cu_internals[] = {
        In1_Cu,
        In2_Cu,
        In3_Cu,
        In4_Cu,
        In5_Cu,
        In6_Cu,
        In7_Cu,
        In8_Cu,
        In9_Cu,
        In10_Cu,
        In11_Cu,
        In12_Cu,
        In13_Cu,
        In14_Cu,
        In15_Cu,
        In16_Cu,
        In17_Cu,
        In18_Cu,
        In19_Cu,
        In20_Cu,
        In21_Cu,
        In22_Cu,
        In23_Cu,
        In24_Cu,
        In25_Cu,
        In26_Cu,
        In27_Cu,
        In28_Cu,
        In29_Cu,
        In30_Cu,
    };

    static const LSET saved( cu_internals, arrayDim( cu_internals ) );
    return saved;
}


LSET LSET::AllCuMask( int aCuLayerCount )
{
    // retain all in static as the full set, which is a common case.
    static const LSET  all = InternalCuMask().set( F_Cu ).set( B_Cu );

    if( aCuLayerCount == MAX_CU_LAYERS )
        return all;

    // subtract out some Cu layers not wanted in the mask.
    LSET    ret = all;
    int     clear_count = MAX_CU_LAYERS - aCuLayerCount;

    clear_count = Clamp( 0, clear_count, MAX_CU_LAYERS - 2 );

    for( int elem = In30_Cu;  clear_count; --elem, --clear_count )
        ret.set( elem, false );

    return ret;
}


LSET LSET::AllNonCuMask()
{
    static const LSET saved = LSET().set() & ~AllCuMask();
    return saved;
}


LSET LSET::ExternalCuMask()
{
    static const LSET saved( 2, F_Cu, B_Cu );
    return saved;
}


LSET LSET::AllLayersMask()
{
    static const LSET saved = LSET().set();
    return saved;
}


LSET LSET::BackTechMask()
{
    static const LSET saved( 6, B_SilkS, B_Mask, B_Adhes, B_Paste, B_CrtYd, B_Fab );
    return saved;
}

LSET LSET::BackBoardTechMask()
{
    static const LSET saved( 4, B_SilkS, B_Mask, B_Adhes, B_Paste );
    return saved;
}

LSET LSET::FrontTechMask()
{
    static const LSET saved( 6, F_SilkS, F_Mask, F_Adhes, F_Paste, F_CrtYd, F_Fab );
    return saved;
}


LSET LSET::FrontBoardTechMask()
{
    static const LSET saved( 4, F_SilkS, F_Mask, F_Adhes, F_Paste );
    return saved;
}


LSET LSET::AllTechMask()
{
    static const LSET saved = BackTechMask() | FrontTechMask();
    return saved;
}


LSET LSET::AllBoardTechMask()
{
    static const LSET saved = BackBoardTechMask() | FrontBoardTechMask();
    return saved;
}


LSET LSET::UserMask()
{
    static const LSET saved( 6,
        Dwgs_User,
        Cmts_User,
        Eco1_User,
        Eco2_User,
        Edge_Cuts,
        Margin
        );

    return saved;
}


LSET LSET::PhysicalLayersMask()
{
    static const LSET saved = AllBoardTechMask() | AllCuMask();
    return saved;
}


LSET LSET::UserDefinedLayers()
{
    static const LSET saved( 9,
        User_1,
        User_2,
        User_3,
        User_4,
        User_5,
        User_6,
        User_7,
        User_8,
        User_9
        );

    return saved;
}


LSET LSET::FrontMask()
{
    static const LSET saved = FrontTechMask().set( F_Cu );
    return saved;
}


LSET LSET::BackMask()
{
    static const LSET saved = BackTechMask().set( B_Cu );
    return saved;
}

LSET LSET::SideSpecificMask()
{
    static const LSET saved = BackTechMask() | FrontTechMask() | AllCuMask();
    return saved;
}


LSET LSET::ForbiddenFootprintLayers()
{
    static const LSET saved = InternalCuMask();
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
        LAYER_VIA_BBLIND,
        LAYER_VIA_THROUGH,
     // LAYER_HIDDEN_TEXT,    // Invisible text hidden by default
        LAYER_ANCHOR,
        LAYER_PADS_SMD_FR,
        LAYER_PADS_SMD_BK,
        LAYER_RATSNEST,
        LAYER_GRID,
        LAYER_GRID_AXES,
        LAYER_FOOTPRINTS_FR,
        LAYER_FOOTPRINTS_BK,
        LAYER_FP_TEXT,
        LAYER_FP_VALUES,
        LAYER_FP_REFERENCES,
        LAYER_TRACKS,
        LAYER_PADS_TH,
        LAYER_PAD_PLATEDHOLES,
        LAYER_NON_PLATEDHOLES,
        LAYER_PAD_HOLEWALLS,
        LAYER_VIA_HOLES,
        LAYER_VIA_HOLEWALLS,
        LAYER_DRC_ERROR,
        LAYER_DRC_WARNING,
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
        LAYER_LOCKED_ITEM_SHADOW,
        LAYER_CONFLICTS_SHADOW
    };

    static const GAL_SET saved( visible, arrayDim( visible ) );
    return saved;
}


/// The layers the user has control over the visibility of, stored in project local settings
GAL_SET GAL_SET::UserVisbilityLayers()
{
    static const GAL_LAYER_ID layers[] = {
        LAYER_TRACKS,
        LAYER_VIAS,
        LAYER_PADS,
        LAYER_ZONES,
        LAYER_DRAW_BITMAPS,
        LAYER_FOOTPRINTS_FR,
        LAYER_FOOTPRINTS_BK,
        LAYER_PADS_TH,
        LAYER_FP_VALUES,
        LAYER_FP_REFERENCES,
        LAYER_FP_TEXT,
        LAYER_HIDDEN_TEXT,
        LAYER_ANCHOR,
        LAYER_RATSNEST,
        LAYER_DRC_WARNING,
        LAYER_DRC_ERROR,
        LAYER_DRC_EXCLUSION,
        LAYER_LOCKED_ITEM_SHADOW,
        LAYER_CONFLICTS_SHADOW,
        LAYER_DRAWINGSHEET,
        LAYER_GRID,
    };

    static const GAL_SET saved( layers, arrayDim( layers ) );
    return saved;
}
