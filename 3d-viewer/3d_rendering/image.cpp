/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015-2016 Mario Luzeiro <mrluzeiro@ua.pt>
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

 /**
 * @file image.cpp
 * @brief one 8bit-channel image implementation.
 */

#include "image.h"
#include "buffers_debug.h"
#include <cstring> // For memcpy

#include <algorithm>
#include <atomic>
#include <thread>
#include <chrono>


#ifndef CLAMP
#define CLAMP( n, min, max ) {if( n < min ) n=min; else if( n > max ) n = max;}
#endif


IMAGE::IMAGE( unsigned int aXsize, unsigned int aYsize )
{
    m_wxh    = aXsize * aYsize;
    m_pixels = new unsigned char[m_wxh];
    memset( m_pixels, 0, m_wxh );
    m_width   = aXsize;
    m_height  = aYsize;
    m_wraping = IMAGE_WRAP::CLAMP;
}


IMAGE::IMAGE( const IMAGE& aSrcImage )
{
    m_wxh    = aSrcImage.GetWidth() * aSrcImage.GetHeight();
    m_pixels = new unsigned char[m_wxh];
    memcpy( m_pixels, aSrcImage.GetBuffer(), m_wxh );
    m_width   = aSrcImage.GetWidth();
    m_height  = aSrcImage.GetHeight();
    m_wraping = IMAGE_WRAP::CLAMP;
}


IMAGE::~IMAGE()
{
    delete[] m_pixels;
}


unsigned char* IMAGE::GetBuffer() const
{
    return m_pixels;
}


bool IMAGE::wrapCoords( int* aXo, int* aYo ) const
{
    int x = *aXo;
    int y = *aYo;

    switch( m_wraping )
    {
    case IMAGE_WRAP::CLAMP:
        x = ( x < 0 ) ? 0 : x;
        x = ( x >= (int) ( m_width - 1 ) ) ? ( m_width - 1 ) : x;
        y = ( y < 0 ) ? 0 : y;
        y = ( y >= (int) ( m_height - 1 ) ) ? ( m_height - 1 ) : y;
        break;

    case IMAGE_WRAP::WRAP:
        x = ( x < 0 ) ? ( ( m_width - 1 ) + x ) : x;
        x = ( x >= (int) ( m_width - 1 ) ) ? ( x - m_width ) : x;
        y = ( y < 0 ) ? ( ( m_height - 1 ) + y ) : y;
        y = ( y >= (int) ( m_height - 1 ) ) ? ( y - m_height ) : y;
        break;

    default:
        break;
    }

    if( ( x < 0 ) || ( x >= (int) m_width ) || ( y < 0 ) || ( y >= (int) m_height ) )
        return false;

    *aXo = x;
    *aYo = y;

    return true;
}


void IMAGE::plot8CircleLines( int aCx, int aCy, int aX, int aY, unsigned char aValue )
{
    Hline( aCx - aX, aCx + aX, aCy + aY, aValue );
    Hline( aCx - aX, aCx + aX, aCy - aY, aValue );
    Hline( aCx - aY, aCx + aY, aCy + aX, aValue );
    Hline( aCx - aY, aCx + aY, aCy - aX, aValue );
}


void IMAGE::Setpixel( int aX, int aY, unsigned char aValue )
{
    if( wrapCoords( &aX, &aY ) )
        m_pixels[aX + aY * m_width] = aValue;
}


unsigned char IMAGE::Getpixel( int aX, int aY ) const
{
    if( wrapCoords( &aX, &aY ) )
        return m_pixels[aX + aY * m_width];
    else
        return 0;
}


void IMAGE::Hline( int aXStart, int aXEnd, int aY, unsigned char aValue )
{
    if( ( aY < 0 ) || ( aY >= (int) m_height ) || ( ( aXStart < 0 ) && ( aXEnd < 0 ) )
      || ( ( aXStart >= (int) m_width ) && ( aXEnd >= (int) m_width ) ) )
        return;

    if( aXStart > aXEnd )
    {
        int swap = aXStart;

        aXStart = aXEnd;
        aXEnd = swap;
    }

    // Clamp line
    if( aXStart < 0 )
        aXStart = 0;

    if( aXEnd >= (int)m_width )
        aXEnd = m_width - 1;

    unsigned char* pixelPtr = &m_pixels[aXStart + aY * m_width];
    unsigned char* pixelPtrEnd = pixelPtr + (unsigned int) ( ( aXEnd - aXStart ) + 1 );

    while( pixelPtr < pixelPtrEnd )
    {
        *pixelPtr = aValue;
        pixelPtr++;
    }
}


// Based on paper
// http://web.engr.oregonstate.edu/~sllu/bcircle.pdf
void IMAGE::CircleFilled( int aCx, int aCy, int aRadius, unsigned char aValue )
{
    int x = aRadius;
    int y = 0;
    int xChange = 1 - 2 * aRadius;
    int yChange = 0;
    int radiusError = 0;

    while( x >= y )
    {
        plot8CircleLines( aCx, aCy, x, y, aValue );
        y++;
        radiusError += yChange;
        yChange += 2;

        if( ( 2 * radiusError + xChange ) > 0 )
        {
            x--;
            radiusError += xChange;
            xChange += 2;
        }
    }
}


void IMAGE::Invert()
{
    for( unsigned int it = 0; it < m_wxh; it++ )
        m_pixels[it] = 255 - m_pixels[it];
}


void IMAGE::CopyFull( const IMAGE* aImgA, const IMAGE* aImgB, IMAGE_OP aOperation )
{
    int aV, bV;

    if( aOperation == IMAGE_OP::RAW )
    {
        if( aImgA == nullptr )
            return;
    }
    else
    {
        if( ( aImgA == nullptr ) || ( aImgB == nullptr ) )
            return;
    }

    switch( aOperation )
    {
    case IMAGE_OP::RAW:
        memcpy( m_pixels, aImgA->m_pixels, m_wxh );
        break;

    case IMAGE_OP::ADD:
        for( unsigned int it = 0;it < m_wxh; it++ )
        {
            aV = aImgA->m_pixels[it];
            bV = aImgB->m_pixels[it];

            aV = (aV + bV);
            aV = (aV > 255)?255:aV;

            m_pixels[it] = aV;
        }
        break;

    case IMAGE_OP::SUB:
        for( unsigned int it = 0;it < m_wxh; it++ )
        {
            aV = aImgA->m_pixels[it];
            bV = aImgB->m_pixels[it];

            aV = (aV - bV);
            aV = (aV < 0)?0:aV;

            m_pixels[it] = aV;
        }
        break;

    case IMAGE_OP::DIF:
        for( unsigned int it = 0;it < m_wxh; it++ )
        {
            aV = aImgA->m_pixels[it];
            bV = aImgB->m_pixels[it];

            m_pixels[it] = abs( aV - bV );
        }
        break;

    case IMAGE_OP::MUL:
        for( unsigned int it = 0;it < m_wxh; it++ )
        {
            aV = aImgA->m_pixels[it];
            bV = aImgB->m_pixels[it];

            m_pixels[it] =
                    (unsigned char) ( ( ( (float) aV / 255.0f ) * ( (float) bV / 255.0f ) ) * 255 );
        }
        break;

    case IMAGE_OP::AND:
        for( unsigned int it = 0;it < m_wxh; it++ )
        {
            m_pixels[it] = aImgA->m_pixels[it] & aImgB->m_pixels[it];
        }
        break;

    case IMAGE_OP::OR:
        for( unsigned int it = 0;it < m_wxh; it++ )
        {
            m_pixels[it] = aImgA->m_pixels[it] | aImgB->m_pixels[it];
        }
        break;

    case IMAGE_OP::XOR:
        for( unsigned int it = 0;it < m_wxh; it++ )
        {
            m_pixels[it] = aImgA->m_pixels[it] ^ aImgB->m_pixels[it];
        }
        break;

    case IMAGE_OP::BLEND50:
        for( unsigned int it = 0;it < m_wxh; it++ )
        {
            aV = aImgA->m_pixels[it];
            bV = aImgB->m_pixels[it];

            m_pixels[it] = (aV + bV) / 2;
        }
        break;

    case IMAGE_OP::MIN:
        for( unsigned int it = 0;it < m_wxh; it++ )
        {
            aV = aImgA->m_pixels[it];
            bV = aImgB->m_pixels[it];

            m_pixels[it] = (aV < bV)?aV:bV;
        }
        break;

    case IMAGE_OP::MAX:
        for( unsigned int it = 0;it < m_wxh; it++ )
        {
            aV = aImgA->m_pixels[it];
            bV = aImgB->m_pixels[it];

            m_pixels[it] = (aV > bV)?aV:bV;
        }
        break;

    default:
        break;
    }
}


// TIP: If you want create or test filters you can use GIMP
// with a generic convolution matrix and get the values from there.
// http://docs.gimp.org/nl/plug-in-convmatrix.html
// clang-format off
static const S_FILTER FILTERS[] =   {
    // IMAGE_FILTER::HIPASS
    {
    {   { 0, -1, -1, -1,  0},
        {-1,  2, -4,  2, -1},
        {-1, -4, 13, -4, -1},
        {-1,  2, -4,  2, -1},
        { 0, -1, -1, -1,  0}
    },
        7,
        255
    },

    // IMAGE_FILTER::GAUSSIAN_BLUR
    {
    {   { 3,  5,  7,  5,  3},
        { 5,  9, 12,  9,  5},
        { 7, 12, 20, 12,  7},
        { 5,  9, 12,  9,  5},
        { 3,  5,  7,  5,  3}
    },
        182,
        0
    },

    // IMAGE_FILTER::GAUSSIAN_BLUR2
    {
    {   { 1,  4,  7,  4,  1},
        { 4, 16, 26, 16,  4},
        { 7, 26, 41, 26,  7},
        { 4, 16, 26, 16,  4},
        { 1,  4,  7,  4,  1}
    },
        273,
        0
    },

    // IMAGE_FILTER::INVERT_BLUR
    {
    {   { 0,  0,  0,  0,  0},
        { 0,  0, -1,  0,  0},
        { 0, -1,  0, -1,  0},
        { 0,  0, -1,  0,  0},
        { 0,  0,  0,  0,  0}
    },
        4,
        255
    },

    // IMAGE_FILTER::CARTOON
    {
    {   {-1, -1, -1, -1,  0},
        {-1,  0,  0,  0,  0},
        {-1,  0,  4,  0,  0},
        { 0,  0,  0,  1,  0},
        { 0,  0,  0,  0,  4}
    },
        3,
        0
    },

    // IMAGE_FILTER::EMBOSS
    {
    {   {-1, -1, -1, -1,  0},
        {-1, -1, -1,  0,  1},
        {-1, -1,  0,  1,  1},
        {-1,  0,  1,  1,  1},
        { 0,  1,  1,  1,  1}
    },
        1,
        128
    },

    // IMAGE_FILTER::SHARPEN
    {
    {   {-1, -1, -1, -1, -1},
        {-1,  2,  2,  2, -1},
        {-1,  2,  8,  2, -1},
        {-1,  2,  2,  2, -1},
        {-1, -1, -1, -1, -1}
    },
        8,
        0
    },

    // IMAGE_FILTER::MELT
    {
    {   { 4,  2,  6,  8,  1},
        { 1,  2,  5,  4,  2},
        { 0, -1,  1, -1,  0},
        { 0,  0, -2,  0,  0},
        { 0,  0,  0,  0,  0}
    },
        32,
        0
    },

    // IMAGE_FILTER::SOBEL_GX
    {
    {   { 0,  0,  0,  0,  0},
        { 0, -1,  0,  1,  0},
        { 0, -2,  0,  2,  0},
        { 0, -1,  0,  1,  0},
        { 0,  0,  0,  0,  0}
    },
        1,
        0
    },

    // IMAGE_FILTER::SOBEL_GY
    {
    {   { 1,  2,  4,  2,  1},
        {-1, -1,  0,  1,  1},
        {-2, -2,  0,  2,  2},
        {-1, -1,  0,  1,  1},
        {-1, -2, -4, -2, -1},
    },
        1,
        0
    },

    // IMAGE_FILTER::BLUR_3X3
    {
    {   { 0,  0,  0,  0,  0},
        { 0,  1,  2,  1,  0},
        { 0,  2,  4,  2,  0},
        { 0,  1,  2,  1,  0},
        { 0,  0,  0,  0,  0},
    },
        16,
        0
    }
};// Filters
// clang-format on


/// @todo: This function can be optimized slipping it between the edges and
///        do it without use the getpixel function.
///        Optimization can be done to m_pixels[ix + iy * m_width]
///        but keep in mind the parallel process of the algorithm
void IMAGE::EfxFilter( IMAGE* aInImg, IMAGE_FILTER aFilterType )
{
    S_FILTER filter = FILTERS[static_cast<int>( aFilterType )];

    aInImg->m_wraping = IMAGE_WRAP::CLAMP;
    m_wraping         = IMAGE_WRAP::CLAMP;

    std::atomic<size_t> nextRow( 0 );
    std::atomic<size_t> threadsFinished( 0 );

    size_t parallelThreadCount = std::max<size_t>( std::thread::hardware_concurrency(), 2 );

    for( size_t ii = 0; ii < parallelThreadCount; ++ii )
    {
        std::thread t = std::thread( [&]()
        {
            for( size_t iy = nextRow.fetch_add( 1 ); iy < m_height; iy = nextRow.fetch_add( 1 ) )
            {
                for( size_t ix = 0; ix < m_width; ix++ )
                {
                    int v = 0;

                    for( size_t sy = 0; sy < 5; sy++ )
                    {
                        for( size_t sx = 0; sx < 5; sx++ )
                        {
                            int factor = filter.kernel[sx][sy];
                            unsigned char pixelv = aInImg->Getpixel( ix + sx - 2, iy + sy - 2 );

                            v += pixelv * factor;
                        }
                    }

                    v /= filter.div;
                    v += filter.offset;
                    CLAMP(v, 0, 255);

                    /// @todo This needs to write to a separate buffer.
                    m_pixels[ix + iy * m_width] = v;
                }
            }

            threadsFinished++;
        } );

        t.detach();
    }

    while( threadsFinished < parallelThreadCount )
        std::this_thread::sleep_for( std::chrono::milliseconds( 10 ) );
}


void IMAGE::EfxFilter_SkipCenter( IMAGE* aInImg, IMAGE_FILTER aFilterType, unsigned int aRadius )
{
    if( ( !aInImg ) || ( m_width != aInImg->m_width ) || ( m_height != aInImg->m_height ) )
        return;

    S_FILTER filter = FILTERS[static_cast<int>( aFilterType )];

    aInImg->m_wraping = IMAGE_WRAP::ZERO;

    const unsigned int radiusSquared = aRadius * aRadius;

    const unsigned int xCenter = m_width / 2;
    const unsigned int yCenter = m_height / 2;

    for( size_t iy = 0; iy < m_height; iy++ )
    {
        int yc = iy - yCenter;

        unsigned int ycsq = yc * yc;

        for( size_t ix = 0; ix < m_width; ix++ )
        {
            int xc = ix - xCenter;

            unsigned int xcsq = xc * xc;

            if( ( xcsq + ycsq ) < radiusSquared )
            {
                const unsigned int offset = ix + iy * m_width;

                m_pixels[offset] = aInImg->m_pixels[offset];

                continue;
            }

            int v = 0;

            for( size_t sy = 0; sy < 5; sy++ )
            {
                for( size_t sx = 0; sx < 5; sx++ )
                {
                    int factor = filter.kernel[sx][sy];
                    unsigned char pixelv = aInImg->Getpixel( ix + sx - 2, iy + sy - 2 );

                    v += pixelv * factor;
                }
            }

            v /= filter.div;
            v += filter.offset;
            CLAMP(v, 0, 255);

            m_pixels[ix + iy * m_width] = v;
        }
    }
}


void IMAGE::SetPixelsFromNormalizedFloat( const float* aNormalizedFloatArray )
{
    for( unsigned int i = 0; i < m_wxh; i++ )
    {
        int v = aNormalizedFloatArray[i] * 255;

        CLAMP( v, 0, 255 );
        m_pixels[i] = v;
    }
}


void IMAGE::SaveAsPNG( const wxString& aFileName ) const
{
    DBG_SaveBuffer( aFileName, m_pixels, m_width, m_height );
}
