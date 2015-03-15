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

#ifndef CImage_h
#define CImage_h

#include <wx/image.h>

enum E_IMAGE_OP {
    COPY_RAW,
    COPY_ADD,
    COPY_SUB,
    COPY_DIF,
    COPY_MUL,
    COPY_AND,
    COPY_OR,
    COPY_XOR,
    COPY_BLEND50,
    COPY_MIN,
    COPY_MAX
};

enum E_WRAP {
    WRAP_ZERO,  ///< Coords that wraps are not evaluated
    WRAP_CLAMP, 
    WRAP_WRAP   ///< Coords are wrapped arround
};

enum E_FILTER {
    FILTER_HIPASS,
    FILTER_BLUR
};

typedef struct {
    signed char    kernel[5][5];
    unsigned char  div;
    unsigned char  offset;
}S_FILTER;


class CIMAGE
{

public:

    CIMAGE( unsigned int xsize, unsigned int ysize );

    ~CIMAGE();

    void setpixel( int x, int y, unsigned char value );
    unsigned char getpixel( int x, int y );

    void copyFull( const CIMAGE *aImgA, const CIMAGE *aImgB, E_IMAGE_OP aOperation );

    void invert();

    void efxFilter( CIMAGE *aInImg, float aGain, E_FILTER aFilterType );

    void saveAsPNG( wxString aFileName );

    void setPixelsFromNormalizedFloat( const float * aNormalizedFloatArray );
private:
    bool wrapCoords( int *xo, int *yo );

public:
    unsigned char*  m_pixels;
    unsigned int    m_width;
    unsigned int    m_height;
    unsigned int    m_wxh;
    E_WRAP          m_wraping;
};

#endif
