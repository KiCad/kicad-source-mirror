/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015 Mario Luzeiro <mrluzeiro@gmail.com>
 * Copyright (C) 1992-2015 KiCad Developers, see AUTHORS.txt for contributors.
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
 * @file CImage.cpp
 * @brief one 8bit-channel image implementation
 */

#include "CImage.h"
#include <wx/image.h>                                                           // Used for save an image to disk
#include <string.h>                                                             // For memcpy

#ifndef CLAMP
#define CLAMP(n, min, max) {if (n < min) n=min; else if (n > max) n = max;}
#endif


CIMAGE::CIMAGE( unsigned int aXsize, unsigned int aYsize )
{
    m_wxh     = aXsize * aYsize;
    m_pixels  = (unsigned char*)malloc( m_wxh );
    m_width   = aXsize;
    m_height  = aYsize;
    m_wraping = (E_WRAP)WRAP_CLAMP;
}


CIMAGE::~CIMAGE()
{
    free( m_pixels );
}


unsigned char* CIMAGE::GetBuffer() const
{
    return m_pixels;
}


bool CIMAGE::wrapCoords( int *aXo, int *aYo ) const
{
    int x = *aXo;
    int y = *aYo;

    switch(m_wraping)
    {
    case WRAP_CLAMP:
        x =  (x < 0 )?0:x;
        x =  (x >= (int)(m_width - 1))?(m_width - 1):x;
        y =  (y < 0)?0:y;
        y =  (y >= (int)(m_height - 1))?(m_height - 1):y;
        break;
    case WRAP_WRAP:
        x = (x < 0)?((m_width - 1)+x):x;
        x = (x >= (int)(m_width - 1))?(x - m_width):x;
        y = (y < 0)?((m_height - 1)+y):y;
        y = (y >= (int)(m_height - 1))?(y - m_height):y;
        break;
    default:
        break;
    }
    
    if( (x < 0) || (x >= (int)m_width) ||
        (y < 0) || (y >= (int)m_height) )
        return false;
    
    *aXo = x;
    *aYo = y;

    return true;
}


void CIMAGE::Setpixel( int aX, int aY, unsigned char aValue )
{
    if( wrapCoords( &aX, &aY ) )
        m_pixels[aX + aY * m_width] = aValue;
}


unsigned char CIMAGE::Getpixel( int aX, int aY ) const
{
    if( wrapCoords( &aX, &aY ) )
        return m_pixels[aX + aY * m_width];
    else
        return 0;
}


void CIMAGE::Invert()
{
    for( unsigned int it = 0; it < m_wxh; it++ )
        m_pixels[it] = 255 - m_pixels[it];
}


void CIMAGE::CopyFull( const CIMAGE *aImgA, const CIMAGE *aImgB, E_IMAGE_OP aOperation )
{
    int aV, bV;

    if( aOperation == COPY_RAW )
    {
        if ( aImgA == NULL )
            return;
    }
    else
    {
        if ( (aImgA == NULL) || (aImgB == NULL) )
            return; 
    }

    switch(aOperation)
    {
    case COPY_RAW:
        memcpy( m_pixels, aImgA->m_pixels, m_wxh );
    break;

    case COPY_ADD:
        for( unsigned int it = 0;it < m_wxh; it++ )
        {
            aV = aImgA->m_pixels[it];
            bV = aImgB->m_pixels[it];

            aV = (aV + bV);
            aV = (aV > 255)?255:aV;

            m_pixels[it] = aV;
        }
    break;

    case COPY_SUB:
        for( unsigned int it = 0;it < m_wxh; it++ )
        {
            aV = aImgA->m_pixels[it];
            bV = aImgB->m_pixels[it];

            aV = (aV - bV);
            aV = (aV < 0)?0:aV;

            m_pixels[it] = aV;
        }
    break;

    case COPY_DIF:
        for( unsigned int it = 0;it < m_wxh; it++ )
        {
            aV = aImgA->m_pixels[it];
            bV = aImgB->m_pixels[it];

            m_pixels[it] = abs(aV - bV);
        }
    break;

    case COPY_MUL:
        for( unsigned int it = 0;it < m_wxh; it++ )
        {
            aV = aImgA->m_pixels[it];
            bV = aImgB->m_pixels[it];

            m_pixels[it] = (unsigned char)((((float)aV / 255.0f) * ((float)bV / 255.0f)) * 255);
        }
    break;

    case COPY_AND:
        for( unsigned int it = 0;it < m_wxh; it++ )
        {
            m_pixels[it] = aImgA->m_pixels[it] & aImgB->m_pixels[it];
        }
    break;

    case COPY_OR:
        for( unsigned int it = 0;it < m_wxh; it++ )
        {
            m_pixels[it] = aImgA->m_pixels[it] | aImgB->m_pixels[it];
        }
    break;

    case COPY_XOR:
        for( unsigned int it = 0;it < m_wxh; it++ )
        {
            m_pixels[it] = aImgA->m_pixels[it] ^ aImgB->m_pixels[it];
        }
    break;
    
    case COPY_BLEND50:
        for( unsigned int it = 0;it < m_wxh; it++ )
        {
            aV = aImgA->m_pixels[it];
            bV = aImgB->m_pixels[it];

            m_pixels[it] = (aV + bV) / 2;
        }
    break;

    case COPY_MIN:
        for( unsigned int it = 0;it < m_wxh; it++ )
        {
            aV = aImgA->m_pixels[it];
            bV = aImgB->m_pixels[it];

            m_pixels[it] = (aV < bV)?aV:bV;
        }
    break;

    case COPY_MAX:
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
static const S_FILTER FILTERS[] =   {
    // Hi Pass
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
    
    // Blur
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

    // Blur Invert
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

    //
    {
    {   { 0,  2,  4,  2,  0},
        { 2, -2,  1, -2,  2},
        { 4,  1, -8,  1,  4},
        { 2, -2,  1, -2,  2},
        { 0,  2,  4,  2,  0}
    },
        20,
        0
    },

    // Cartoon
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

    // Emboss
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

    // Sharpen
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

    // Melt
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

    // Sobel Gx
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

    // Sobel Gy
    {
    {   { 1,  2,  4,  2,  1},
        {-1, -1,  0,  1,  1},
        {-2, -2,  0,  2,  2},
        {-1, -1,  0,  1,  1},
        {-1, -2, -4, -2, -1},
    },
        1,
        0
    }
};// Filters


//!TODO: This functions can be optimized slipting it between the edges and
//       do it without use the getpixel function.
//       Optimization can be done to m_pixels[ix + iy * m_width]
//       but keep in mind the parallel process of the algorithm
void CIMAGE::EfxFilter( CIMAGE *aInImg, E_FILTER aFilterType )
{
    S_FILTER filter = FILTERS[aFilterType];

    aInImg->m_wraping = WRAP_CLAMP;
    m_wraping = WRAP_CLAMP;

    #ifdef USE_OPENMP
    #pragma omp parallel for
    #endif /* USE_OPENMP */
    
    for( int iy = 0; iy < (int)m_height; iy++)
    {
        for( int ix = 0; ix < (int)m_width; ix++ )
        {
            int v = 0;

            for( int sy = 0; sy < 5; sy++ )
            {
                for( int sx = 0; sx < 5; sx++ )
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


void CIMAGE::SetPixelsFromNormalizedFloat( const float * aNormalizedFloatArray )
{
    for( unsigned int i = 0; i < m_wxh; i++ )
    {
        int v = aNormalizedFloatArray[i] * 255;
        CLAMP(v, 0, 255);
        m_pixels[i] = v;
    }
}


void CIMAGE::SaveAsPNG( wxString aFileName ) const
{
    unsigned char* pixelbuffer = (unsigned char*) malloc( m_wxh * 3 );
   
    wxImage image( m_width, m_height );

    for( unsigned int i = 0; i < m_wxh; i++)
    {
        unsigned char v = m_pixels[i];
        // Set RGB value with all same values intensities
        pixelbuffer[i * 3 + 0] = v;
        pixelbuffer[i * 3 + 1] = v;
        pixelbuffer[i * 3 + 2] = v;
    }

    image.SetData( pixelbuffer );
    image = image.Mirror( false );
    image.SaveFile( aFileName + ".png", wxBITMAP_TYPE_PNG );
    image.Destroy();
}
