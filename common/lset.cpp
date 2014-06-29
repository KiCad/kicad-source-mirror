/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2014 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2014 KiCad Developers, see change_log.txt for contributors.
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


#include <stdarg.h>
#include <assert.h>

#include <layers_id_colors_and_visibility.h>
#include <class_board.h>


LSET::LSET( const LAYER_ID* aArray, unsigned aCount )
{
    for( unsigned i=0; i<aCount; ++i )
        set( aArray[i] );
}


LSET::LSET( size_t aIdCount, ... )
{
    va_list ap;

    va_start( ap, aIdCount );

    for( size_t i=0;  i<aIdCount;  ++i )
    {
        LAYER_ID id = (LAYER_ID) va_arg( ap, int );

        // printf( "%s: id:%d LAYER_ID_COUNT:%d\n", __func__, id, LAYER_ID_COUNT );

        assert( unsigned( id ) < LAYER_ID_COUNT );

        set( id );
    }

    va_end( ap );
}


const wxChar* LSET::Name( LAYER_ID aLayerId )
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
    case F_CrtYd:           txt = wxT( "F_CrtYd" );         break;
    case B_CrtYd:           txt = wxT( "B_CrtYd" );         break;
    case F_Fab:             txt = wxT( "F_Fab" );           break;
    case B_Fab:             txt = wxT( "B_Fab" );           break;

    default:
        wxASSERT_MSG( 0, wxT( "aLayerId out of range" ) );
                            txt = wxT( "BAD INDEX!" );      break;
    }

    return txt;
}


LSEQ LSET::CuStack() const
{
    // desired sequence
    static const LAYER_ID sequence[] = {
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

    return Seq( sequence, DIM( sequence ) );
}


LSEQ LSET::Technicals( LSET aSetToOmit ) const
{
    // desired sequence
    static const LAYER_ID sequence[] = {
        B_Adhes,
        F_Adhes,
        B_Paste,
        F_Paste,
        B_SilkS,
        F_SilkS,
        B_Mask,
        F_Mask,
        B_CrtYd,
        F_CrtYd,
        B_Fab,
        F_Fab,
    };

    LSET subset = ~aSetToOmit & *this;

    return subset.Seq( sequence, DIM( sequence ) );
}


LSEQ LSET::Users() const
{
    // desired
    static const LAYER_ID sequence[] = {
        Dwgs_User,
        Cmts_User,
        Eco1_User,
        Eco2_User,
        Edge_Cuts,
        Margin,
   };

   return Seq( sequence, DIM( sequence ) );
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

    int     nibble_count = ( size() + 3 ) / 4;

    for( int nibble=0;  nibble<nibble_count;  ++nibble )
    {
        unsigned ndx = 0;

        // test 4 consecutive bits and set ndx to 0-15:
        for( int nibble_bit=0;  nibble_bit<4;  ++nibble_bit )
        {
            if( (*this)[nibble_bit + nibble*4] )
                ndx |= (1 << nibble_bit);
        }

        if( nibble && !( nibble % 8 ) )
            ret += '_';

        assert( ndx < DIM( hex ) );

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


LSEQ LSET::Seq( const LAYER_ID* aWishListSequence, unsigned aCount ) const
{
    LSEQ ret;

#if defined(DEBUG) && 0
    LSET    dup_detector;

    for( unsigned i=0; i<aCount;  ++i )
    {
        LAYER_ID id = aWishListSequence[i];

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
        LAYER_ID id = aWishListSequence[i];

        if( test( id ) )
            ret.push_back( id );
    }
#endif

    return ret;
}


LSEQ LSET::Seq() const
{
    LSEQ    ret;

    for( unsigned i=0;  i<size();  ++i )
    {
        if( test(i) )
            ret.push_back( LAYER_ID( i ) );
    }

    return ret;
}


LAYER_ID FlipLayer( LAYER_ID aLayerId )
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

    // No change for the other layers
    default:
        return aLayerId;
    }
}


LSET FlipLayerMask( LSET aMask )
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

    return newMask;
}


LAYER_ID LSET::ExtractLayer() const
{
    unsigned set_count = count();

    if( !set_count )
        return UNSELECTED_LAYER;
    else if( set_count > 1 )
        return UNDEFINED_LAYER;

    for( unsigned i=0; i < size(); ++i )
    {
        if( test( i ) )
            return LAYER_ID( i );
    }

    wxASSERT( 0 );  // set_count was verified as 1 above, what did you break?

    return UNDEFINED_LAYER;
}


LSET LSET::InternalCuMask()
{
    static const LAYER_ID cu_internals[] = {
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

    static const LSET saved( cu_internals, DIM( cu_internals ) );
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

    for( LAYER_NUM elem=In30_Cu;  clear_count; --elem, --clear_count )
    {
        ret.set( elem, false );
    }

    return ret;
}


LSET LSET::AllNonCuMask()
{
    static const LSET saved = LSET().set() & ~AllCuMask();
    return saved;
}


LSET LSET::AllLayersMask()
{
    static const LSET saved = LSET().set();
    return saved;
}


LSET LSET::BackTechMask()
{
    // (SILKSCREEN_LAYER_BACK | SOLDERMASK_LAYER_BACK | ADHESIVE_LAYER_BACK | SOLDERPASTE_LAYER_BACK)
    static const LSET saved( 6, B_SilkS, B_Mask, B_Adhes, B_Paste, B_CrtYd, B_Fab );
    return saved;
}


LSET LSET::FrontTechMask()
{
    // (SILKSCREEN_LAYER_FRONT | SOLDERMASK_LAYER_FRONT | ADHESIVE_LAYER_FRONT | SOLDERPASTE_LAYER_FRONT)
    static const LSET saved( 6, F_SilkS, F_Mask, F_Adhes, F_Paste, F_CrtYd, F_Fab );
    return saved;
}


LSET LSET::AllTechMask()
{
    static const LSET saved = BackTechMask() | FrontTechMask();
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


LSEQ LSET::UIOrder() const
{
    LAYER_ID order[Margin+1];

    // Assmuming that the LAYER_ID order is according to preferred UI order, as of
    // today this is true.  When that becomes not true, its easy to change the order
    // in here to compensate.

    for( unsigned i=0;  i<DIM(order);  ++i )
        order[i] = LAYER_ID( i );

    return Seq( order, DIM( order ) );
}

LAYER_ID ToLAYER_ID( int aLayer );
