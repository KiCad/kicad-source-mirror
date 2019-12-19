/*
 *  The MIT License (MIT)
 *
 *  Copyright (c) 2016 Marcel Steinbeck
 *
 *  Permission is hereby granted, free of charge, to any person obtaining a copy
 *  of this software and associated documentation files (the "Software"), to deal
 *  in the Software without restriction, including without limitation the rights
 *  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 *  copies of the Software, and to permit persons to whom the Software is
 *  furnished to do so, subject to the following conditions:
 *
 *  The above copyright notice and this permission notice shall be included in all
 *  copies or substantial portions of the Software.
 *
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 *  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 *  SOFTWARE.
 */

#include "tinysplinecpp.h"
#include <stdexcept>

/********************************************************
*                                                       *
* DeBoorNet                                             *
*                                                       *
********************************************************/
tinyspline::DeBoorNet::DeBoorNet()
{
    ts_deboornet_default( &deBoorNet );
}


tinyspline::DeBoorNet::DeBoorNet( const tinyspline::DeBoorNet& other )
{
    const tsError err = ts_deboornet_copy( &other.deBoorNet, &deBoorNet );

    if( err < 0 )
        throw std::runtime_error( ts_enum_str( err ) );
}


tinyspline::DeBoorNet::~DeBoorNet()
{
    ts_deboornet_free( &deBoorNet );
}


tinyspline::DeBoorNet& tinyspline::DeBoorNet::operator=( const tinyspline::DeBoorNet& other )
{
    if( &other != this )
    {
        const tsError err = ts_deboornet_copy(
                &other.deBoorNet, &deBoorNet );

        if( err < 0 )
            throw std::runtime_error( ts_enum_str( err ) );
    }

    return *this;
}


tinyspline::real tinyspline::DeBoorNet::u() const
{
    return deBoorNet.u;
}


size_t tinyspline::DeBoorNet::k() const
{
    return deBoorNet.k;
}


size_t tinyspline::DeBoorNet::s() const
{
    return deBoorNet.s;
}


size_t tinyspline::DeBoorNet::h() const
{
    return deBoorNet.h;
}


size_t tinyspline::DeBoorNet::dim() const
{
    return deBoorNet.dim;
}


size_t tinyspline::DeBoorNet::nPoints() const
{
    return deBoorNet.n_points;
}


std::vector<tinyspline::real> tinyspline::DeBoorNet::points() const
{
    const tinyspline::real* begin = deBoorNet.points;
    const tinyspline::real* end = begin + deBoorNet.n_points * deBoorNet.dim;

    return std::vector<tinyspline::real>( begin, end );
}


std::vector<tinyspline::real> tinyspline::DeBoorNet::result() const
{
    const tinyspline::real* begin = deBoorNet.result;
    const tinyspline::real* end = begin + deBoorNet.dim;

    return std::vector<tinyspline::real>( begin, end );
}


tsDeBoorNet* tinyspline::DeBoorNet::data()
{
    return &deBoorNet;
}


#ifndef TINYSPLINE_DISABLE_CXX11_FEATURES
tinyspline::DeBoorNet::DeBoorNet( tinyspline::DeBoorNet&& other ) noexcept
{
    ts_deboornet_default( &deBoorNet );
    swap( other );
}


tinyspline::DeBoorNet& tinyspline::DeBoorNet::operator=( tinyspline::DeBoorNet&& other ) noexcept
{
    if( &other != this )
    {
        ts_deboornet_free( &deBoorNet );
        swap( other );
    }

    return *this;
}


void tinyspline::DeBoorNet::swap( tinyspline::DeBoorNet& other )
{
    if( &other != this )
    {
        std::swap( deBoorNet.u, other.deBoorNet.u );
        std::swap( deBoorNet.k, other.deBoorNet.k );
        std::swap( deBoorNet.s, other.deBoorNet.s );
        std::swap( deBoorNet.h, other.deBoorNet.h );
        std::swap( deBoorNet.dim, other.deBoorNet.dim );
        std::swap( deBoorNet.n_points, other.deBoorNet.n_points );
        std::swap( deBoorNet.points, other.deBoorNet.points );
        std::swap( deBoorNet.result, other.deBoorNet.result );
    }
}


#endif


/********************************************************
*                                                       *
* BSpline                                               *
*                                                       *
********************************************************/
tinyspline::BSpline::BSpline()
{
    ts_bspline_default( &bspline );
}


tinyspline::BSpline::BSpline( const tinyspline::BSpline& other )
{
    const tsError err = ts_bspline_copy( &other.bspline, &bspline );

    if( err < 0 )
        throw std::runtime_error( ts_enum_str( err ) );
}


tinyspline::BSpline::BSpline( const size_t nCtrlp, const size_t dim,
        const size_t deg, const tinyspline::BSpline::type type )
{
    const tsError err = ts_bspline_new( nCtrlp, dim, deg, type, &bspline );

    if( err < 0 )
        throw std::runtime_error( ts_enum_str( err ) );
}


tinyspline::BSpline::~BSpline()
{
    ts_bspline_free( &bspline );
}


tinyspline::BSpline& tinyspline::BSpline::operator=( const tinyspline::BSpline& other )
{
    if( &other != this )
    {
        const tsError err = ts_bspline_copy( &other.bspline, &bspline );

        if( err < 0 )
            throw std::runtime_error( ts_enum_str( err ) );
    }

    return *this;
}


tinyspline::DeBoorNet tinyspline::BSpline::operator()( const tinyspline::real u ) const
{
    return evaluate( u );
}


size_t tinyspline::BSpline::deg() const
{
    return bspline.deg;
}


size_t tinyspline::BSpline::order() const
{
    return bspline.order;
}


size_t tinyspline::BSpline::dim() const
{
    return bspline.dim;
}


size_t tinyspline::BSpline::nCtrlp() const
{
    return bspline.n_ctrlp;
}


size_t tinyspline::BSpline::nKnots() const
{
    return bspline.n_knots;
}


std::vector<tinyspline::real> tinyspline::BSpline::ctrlp() const
{
    const tinyspline::real* begin = bspline.ctrlp;
    const tinyspline::real* end = begin + bspline.n_ctrlp * bspline.dim;

    return std::vector<tinyspline::real>( begin, end );
}


std::vector<tinyspline::real> tinyspline::BSpline::knots() const
{
    const tinyspline::real* begin = bspline.knots;
    const tinyspline::real* end = begin + bspline.n_knots;

    return std::vector<tinyspline::real>( begin, end );
}


tsBSpline* tinyspline::BSpline::data()
{
    return &bspline;
}


tinyspline::DeBoorNet tinyspline::BSpline::evaluate( const tinyspline::real u ) const
{
    tinyspline::DeBoorNet deBoorNet;
    const tsError err = ts_bspline_evaluate( &bspline, u, deBoorNet.data() );

    if( err < 0 )
        throw std::runtime_error( ts_enum_str( err ) );

    return deBoorNet;
}


void tinyspline::BSpline::setCtrlp( const std::vector<tinyspline::real>& ctrlp )
{
    if( ctrlp.size() != nCtrlp() * dim() )
    {
        throw std::runtime_error( "The number of values must be equals"
                                  "to the spline's number of control points multiplied"
                                  "by the dimension of each control point." );
    }

    const tsError err = ts_bspline_set_ctrlp(
            &bspline, ctrlp.data(), &bspline );

    if( err < 0 )
        throw std::runtime_error( ts_enum_str( err ) );
}


void tinyspline::BSpline::setKnots( const std::vector<tinyspline::real>& knots )
{
    if( knots.size() != nKnots() )
    {
        throw std::runtime_error( "The number of values must be equals"
                                  "to the spline's number of knots." );
    }

    const tsError err = ts_bspline_set_knots(
            &bspline, knots.data(), &bspline );

    if( err < 0 )
        throw std::runtime_error( ts_enum_str( err ) );
}


tinyspline::BSpline tinyspline::BSpline::fillKnots( const tsBSplineType type,
        const tinyspline::real min,
        const tinyspline::real max ) const
{
    tinyspline::BSpline bs;
    const tsError err = ts_bspline_fill_knots(
            &bspline, type, min, max, &bs.bspline );

    if( err < 0 )
        throw std::runtime_error( ts_enum_str( err ) );

    return bs;
}


tinyspline::BSpline tinyspline::BSpline::insertKnot( const tinyspline::real u,
        const size_t n ) const
{
    tinyspline::BSpline bs;
    size_t k;
    const tsError err = ts_bspline_insert_knot(
            &bspline, u, n, &bs.bspline, &k );

    if( err < 0 )
        throw std::runtime_error( ts_enum_str( err ) );

    return bs;
}


tinyspline::BSpline tinyspline::BSpline::resize( const int n, const int back ) const
{
    tinyspline::BSpline bs;
    const tsError err = ts_bspline_resize( &bspline, n, back, &bs.bspline );

    if( err < 0 )
        throw std::runtime_error( ts_enum_str( err ) );

    return bs;
}


tinyspline::BSpline tinyspline::BSpline::split( const tinyspline::real u ) const
{
    tinyspline::BSpline bs;
    size_t k;
    const tsError err = ts_bspline_split( &bspline, u, &bs.bspline, &k );

    if( err < 0 )
        throw std::runtime_error( ts_enum_str( err ) );

    return bs;
}


tinyspline::BSpline tinyspline::BSpline::buckle( const tinyspline::real b ) const
{
    tinyspline::BSpline bs;
    const tsError err = ts_bspline_buckle( &bspline, b, &bs.bspline );

    if( err < 0 )
        throw std::runtime_error( ts_enum_str( err ) );

    return bs;
}


tinyspline::BSpline tinyspline::BSpline::toBeziers() const
{
    tinyspline::BSpline bs;
    const tsError err = ts_bspline_to_beziers( &bspline, &bs.bspline );

    if( err < 0 )
        throw std::runtime_error( ts_enum_str( err ) );

    return bs;
}


tinyspline::BSpline tinyspline::BSpline::derive() const
{
    tinyspline::BSpline bs;
    const tsError err = ts_bspline_derive( &bspline, &bs.bspline );

    if( err < 0 )
        throw std::runtime_error( ts_enum_str( err ) );

    return bs;
}


#ifndef TINYSPLINE_DISABLE_CXX11_FEATURES
tinyspline::BSpline::BSpline( tinyspline::BSpline&& other ) noexcept
{
    ts_bspline_default( &bspline );
    swap( other );
}


tinyspline::BSpline& tinyspline::BSpline::operator=( tinyspline::BSpline&& other ) noexcept
{
    if( &other != this )
    {
        ts_bspline_free( &bspline );
        swap( other );
    }

    return *this;
}


void tinyspline::BSpline::swap( tinyspline::BSpline& other )
{
    if( &other != this )
    {
        std::swap( bspline.deg, other.bspline.deg );
        std::swap( bspline.order, other.bspline.order );
        std::swap( bspline.dim, other.bspline.dim );
        std::swap( bspline.n_ctrlp, other.bspline.n_ctrlp );
        std::swap( bspline.n_knots, other.bspline.n_knots );
        std::swap( bspline.ctrlp, other.bspline.ctrlp );
        std::swap( bspline.knots, other.bspline.knots );
    }
}


#endif


/********************************************************
*                                                       *
* Utils                                                 *
*                                                       *
********************************************************/
tinyspline::BSpline tinyspline::Utils::interpolateCubic(
        const std::vector<tinyspline::real>* points,
        const size_t dim )
{
    if( dim == 0 )
        throw std::runtime_error( ts_enum_str( TS_DIM_ZERO ) );

    if( points->size() % dim != 0 )
        throw std::runtime_error( "#points % dim == 0 failed" );

    tinyspline::BSpline bspline;
    const tsError err = ts_bspline_interpolate_cubic(
            points->data(), points->size() / dim, dim, bspline.data() );

    if( err < 0 )
        throw std::runtime_error( ts_enum_str( err ) );

    return bspline;
}


bool tinyspline::Utils::fequals( const tinyspline::real x, const tinyspline::real y )
{
    return ts_fequals( x, y ) == 1;
}


std::string tinyspline::Utils::enum_str( const tsError err )
{
    return std::string( ts_enum_str( err ) );
}


tsError tinyspline::Utils::str_enum( const std::string str )
{
    return ts_str_enum( str.c_str() );
}
